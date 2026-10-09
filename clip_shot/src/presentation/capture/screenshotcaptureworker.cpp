#include "screenshotcaptureworker.h"
#include "clip_shot/diagnostics/diagnostics.h"
#include "captureframeimage.h"
#include "captureframegeometry.h"

#include "screenshotcaptureperfinstrumentation.h"
#include "clip_shot/presentation/screenshotcapturecoordinator.h"
#include "clip_shot/presentation/capture/screenshotcapturepolicy.h"
#include "clip_shot/storage/settingsadapters.h"

#include "clip_capture.h"

#include <QDebug>
#include <QImage>
#include <QMetaObject>

#include <limits>
#include <algorithm>
#include <cmath>
#include <utility>

namespace {
using clip_shot::presentation::capture::imageFromFrameLease;
using clip_shot::presentation::capture::validFrameInfo;

ScreenshotCaptureBackend backendFromNative(std::uint8_t backend) {
    switch (backend) {
    case CLIP_CAPTURE_BACKEND_DXGI:
        return ScreenshotCaptureBackend::Dxgi;
    case CLIP_CAPTURE_BACKEND_WGC:
        return ScreenshotCaptureBackend::WindowsGraphicsCapture;
    case CLIP_CAPTURE_BACKEND_SCREEN_CAPTURE_KIT:
        return ScreenshotCaptureBackend::ScreenCaptureKit;
    case CLIP_CAPTURE_BACKEND_GDI:
        return ScreenshotCaptureBackend::Gdi;
    default:
        return ScreenshotCaptureBackend::Auto;
    }
}

QString nativeCaptureError(const char* fallback) {
    const char* message = clip_capture_last_error_message();
    return QString::fromUtf8(message != nullptr && *message != '\0' ? message : fallback);
}
} // namespace

ScreenshotCaptureWorker::~ScreenshotCaptureWorker() {
    if (m_session != nullptr) {
        clip_capture_desktop_session_destroy(m_session);
        m_session = nullptr;
    }
}

void ScreenshotCaptureWorker::prepare(quint64 requestId,
                                      const QPointer<ScreenshotCaptureCoordinator>& coordinator) {
    const bool ok = prepareSessionIfNeeded();
    postPrepared(requestId, coordinator, ok);
}

void ScreenshotCaptureWorker::refreshLayout(
    quint64 requestId, const QPointer<ScreenshotCaptureCoordinator>& coordinator) {
    const bool ok = ensureSession() && clip_capture_desktop_session_refresh_layout(m_session) != 0;
    if (!coordinator.isNull()) {
        QMetaObject::invokeMethod(
            coordinator,
            [coordinator, requestId, ok]() {
                if (!coordinator.isNull()) {
                    emit coordinator->layoutRefreshed(requestId, ok);
                }
            },
            Qt::QueuedConnection);
    }
}

void ScreenshotCaptureWorker::capture(const ScreenshotCaptureRequest& request,
                                      const QPointer<ScreenshotCaptureCoordinator>& coordinator,
                                      ClipCaptureCancellationToken* cancellationToken) {
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("capture.worker_entry");
    ScreenshotCaptureResult captureResult;
    captureResult.requestId = request.requestId;
    captureResult.purpose = request.purpose;
    if (!ensureSession(request.excludedWindowIds)) {
        captureResult.errorMessage = nativeCaptureError("Failed to create desktop capture session");
        postCaptureResult(coordinator, std::move(captureResult));
        return;
    }
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("capture.session_ready");

    std::unique_ptr<ClipCaptureDesktopLayout, decltype(&clip_capture_desktop_layout_destroy)>
        layout(nullptr, clip_capture_desktop_layout_destroy);
    if (request.purpose == ScreenshotCapturePurpose::Initial) {
        CLIP_SHOT_CAPTURE_PERF_SCOPE("capture.layout_snapshot");
        layout.reset(
            clip_capture_desktop_session_layout_snapshot(m_session, request.refreshLayout ? 1 : 0));
        if (!layout) {
            captureResult.errorMessage = nativeCaptureError("Failed to snapshot desktop layout");
            postCaptureResult(coordinator, std::move(captureResult));
            return;
        }
        ScreenshotCaptureLayout prepared;
        prepared.requestId = request.requestId;
        prepared.generation = request.requestId;
        const size_t displayCount = clip_capture_desktop_layout_count(layout.get());
        bool validLayout = displayCount > 0 && displayCount <= 128;
        for (size_t i = 0; validLayout && i < displayCount; ++i) {
            ClipCaptureDisplayDescriptor d{};
            d.version = 1;
            d.struct_size = sizeof(d);
            if (!clip_capture_desktop_layout_display(layout.get(), i, &d) || !std::isfinite(d.x) ||
                !std::isfinite(d.y) || !std::isfinite(d.width) || !std::isfinite(d.height) ||
                !std::isfinite(d.backing_scale) || d.backing_scale <= 0 || d.coordinate_space > 1 ||
                d.width < 1 || d.height < 1 || d.width > std::numeric_limits<int>::max() ||
                d.height > std::numeric_limits<int>::max() ||
                d.x < std::numeric_limits<int>::min() || d.y < std::numeric_limits<int>::min() ||
                d.x + std::max(d.width, static_cast<double>(d.pixel_width)) >
                    std::numeric_limits<int>::max() ||
                d.y + std::max(d.height, static_cast<double>(d.pixel_height)) >
                    std::numeric_limits<int>::max() ||
                d.pixel_width == 0 || d.pixel_height == 0 ||
                d.pixel_width > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
                d.pixel_height > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
                validLayout = false;
                break;
            }
            CapturedDisplayModel display;
            display.stableId = QString::fromUtf8(d.stable_id);
            display.name = QString::fromUtf8(d.name);
            display.nativeDisplayId = d.display_id;
            display.physicalRect = QRect(qRound(d.x), qRound(d.y), static_cast<int>(d.pixel_width),
                                         static_cast<int>(d.pixel_height));
            display.canvasUsesPoints = d.coordinate_space == 1;
            if (display.canvasUsesPoints) {
                display.capturedLogicalRect =
                    QRect(qRound(d.x), qRound(d.y), qRound(d.width), qRound(d.height));
                display.backingScale = d.backing_scale;
            }
            display.primary = d.is_primary != 0;
            display.active = true;
            prepared.displays.push_back(std::move(display));
        }
        if (!validLayout) {
            captureResult.errorMessage = QStringLiteral("Invalid desktop layout snapshot");
            postCaptureResult(coordinator, std::move(captureResult));
            return;
        }
        CLIP_SHOT_CAPTURE_PERF_MILESTONE("capture.layout_ready");
        QMetaObject::invokeMethod(
            coordinator,
            [coordinator, prepared = std::move(prepared)]() {
                if (coordinator)
                    emit coordinator->layoutReady(prepared);
            },
            Qt::QueuedConnection);
    }

    ClipCaptureScreenshotRequest nativeRequest{};
    nativeRequest.version = CLIP_CAPTURE_SCREENSHOT_REQUEST_VERSION;
    nativeRequest.struct_size = sizeof(nativeRequest);
    nativeRequest.flags =
        request.refreshLayout && !layout ? CLIP_CAPTURE_SCREENSHOT_REQUEST_REFRESH_LAYOUT : 0;
    if (request.restoreOriginalScreenColors) {
        nativeRequest.flags |= CLIP_CAPTURE_SCREENSHOT_REQUEST_RESTORE_ORIGINAL_COLORS;
    }
    nativeRequest.cancellation_token = cancellationToken;

    ClipCaptureScreenshotResult* nativeResult = nullptr;
    {
        CLIP_SHOT_CAPTURE_PERF_SCOPE("capture.native_ffi");
        nativeResult = layout ? clip_capture_desktop_session_capture_with_layout(
                                    m_session, &nativeRequest, layout.get())
                              : clip_capture_desktop_session_capture(m_session, &nativeRequest);
    }
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("capture.native_returned");
    if (nativeResult == nullptr) {
        const QString captureError = nativeCaptureError("Screenshot capture failed");
        static_cast<void>(clip_capture_desktop_session_reset_to_prepared(m_session));
        captureResult.errorMessage = captureError;
        postCaptureResult(coordinator, std::move(captureResult));
        return;
    }

    const size_t count = clip_capture_screenshot_result_display_count(nativeResult);
    captureResult.displays.reserve(static_cast<int>(count));
    bool valid = count != 0;
    for (size_t index = 0; index < count; ++index) {
        ClipCaptureFrameInfo info{};
        if (clip_capture_screenshot_result_display_info(nativeResult, index, &info) == 0 ||
            !validFrameInfo(info)) {
            valid = false;
            break;
        }

        ClipCaptureFrameLease* lease =
            clip_capture_screenshot_result_display_retain(nativeResult, index);
        QImage image = imageFromFrameLease(
            lease, info.rgba_bytes, info.rgba_len, info.width, info.height, info.stride_bytes,
            info.pixel_format, clip_shot::presentation::capture::FrameAlphaMode::Opaque);
        if (image.isNull()) {
            valid = false;
            break;
        }

        CapturedDisplayModel display;
        display.stableId = QString::fromUtf8(info.stable_id != nullptr ? info.stable_id : "");
        display.name = QString::fromUtf8(info.name != nullptr ? info.name : "");
        display.physicalRect =
            QRect(info.x, info.y, static_cast<int>(info.width), static_cast<int>(info.height));
#ifdef Q_OS_MACOS
        ClipCaptureFrameGeometry geometry{};
        geometry.version = CLIP_CAPTURE_FRAME_GEOMETRY_VERSION;
        geometry.struct_size = sizeof(geometry);
        if (!clip_capture_screenshot_result_display_geometry(nativeResult, index, &geometry)) {
            valid = false;
            break;
        }
        const auto logicalRect = clip_shot::presentation::capture::logicalFrameRect(geometry);
        if (!logicalRect || geometry.display_id == 0) {
            valid = false;
            break;
        }
        display.capturedLogicalRect = *logicalRect;
        display.physicalRect.moveTopLeft(display.capturedLogicalRect.topLeft());
        display.nativeDisplayId = geometry.display_id;
        display.backingScale = geometry.backing_scale;
        display.canvasUsesPoints = true;
#endif
        display.canvasRect = display.physicalRect;
        display.image = std::move(image);
        if (request.captureCursor && request.cursorSnapshot) {
            ClipCaptureFrameInfo patchInfo{};
            ClipCaptureFrameLease* patchLease = nullptr;
            if (clip_capture_screenshot_result_cursor_patch(
                    nativeResult, request.cursorSnapshot.get(), index, &patchInfo, &patchLease) &&
                patchLease != nullptr) {
                display.cursorPatch = imageFromFrameLease(
                    patchLease, patchInfo.rgba_bytes, patchInfo.rgba_len, patchInfo.width,
                    patchInfo.height, patchInfo.stride_bytes, patchInfo.pixel_format,
                    clip_shot::presentation::capture::FrameAlphaMode::Opaque);
                display.cursorPixelRect =
                    QRect(patchInfo.x, patchInfo.y, static_cast<int>(patchInfo.width),
                          static_cast<int>(patchInfo.height));
                captureResult.cursorAvailable |= !display.cursorPatch.isNull();
            }
        }
        display.active = true;
        display.backend = backendFromNative(info.backend_kind);
        captureResult.displays.push_back(std::move(display));
    }

    if (!valid && captureResult.errorMessage.isEmpty()) {
        captureResult.errorMessage = nativeCaptureError("Screenshot capture returned invalid data");
    }
    if (!valid) {
        captureResult.displays.clear();
    }
    captureResult.succeeded = valid;
    clip_capture_screenshot_result_destroy(nativeResult);
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("capture.frames_wrapped");
    CLIP_SHOT_CAPTURE_PERF_MILESTONE("capture.result_posted");
    postCaptureResult(coordinator, std::move(captureResult));
}

bool ScreenshotCaptureWorker::ensureSession(const QVector<std::uint32_t>& excludedWindowIds) {
    const QString apiMode = clip_shot::storage::ScreenshotSettings().apiMode();
    auto requested = clip_shot::presentation::capture::screenshotApiModeFromValue(
        apiMode.toLatin1().constData());
    if (requested == clip_shot::presentation::capture::ScreenshotApiMode::Auto) {
        requested = clip_shot::presentation::capture::resolveAutoScreenshotApiMode();
    }
    const auto backend =
        clip_shot::presentation::capture::nativeBackendForNormalScreenshot(requested);
    if (m_session != nullptr && m_sessionBackend == backend &&
        m_sessionExcludedWindowIds == excludedWindowIds) {
        return true;
    }

    // Backend policy and exclusions are fixed at creation, including for prewarmed sessions.
    if (m_session != nullptr) {
        clip_capture_desktop_session_destroy(m_session);
        m_session = nullptr;
    }

    ClipCaptureDesktopSessionConfig config{};
    config.capture_retry_count = 1;
    config.capture_backend = backend;
    config.exclusions.windows = excludedWindowIds.constData();
    config.exclusions.window_count = static_cast<size_t>(excludedWindowIds.size());
#if defined(Q_OS_WIN) || defined(_WIN32)
    config.pixel_format = CLIP_CAPTURE_PIXEL_FORMAT_BGRA8;
#endif
    m_session = clip_capture_desktop_session_create(&config);
    if (m_session == nullptr) {
        qWarning("Failed to create desktop capture session: %s", clip_capture_last_error_message());
        return false;
    }
    m_sessionBackend = backend;
    m_sessionExcludedWindowIds = excludedWindowIds;
    clip_shot::diagnostics::logEvent(
        QStringLiteral("clip_shot.capture"), QStringLiteral("capture.backend_ready"),
        {{QStringLiteral("backend"), static_cast<int>(backend)},
         {QStringLiteral("count"), static_cast<qint64>(config.capture_retry_count)}});
    return true;
}

bool ScreenshotCaptureWorker::sessionPrepared() const {
    if (m_session == nullptr) {
        return false;
    }

    ClipCaptureDesktopSessionState state{};
    if (clip_capture_desktop_session_state(m_session, &state) == 0) {
        return false;
    }
    return state.prepared != 0;
}

bool ScreenshotCaptureWorker::prepareSessionIfNeeded() {
    if (!ensureSession()) {
        return false;
    }
    if (sessionPrepared()) {
        return true;
    }
    return clip_capture_desktop_session_prepare(m_session) != 0;
}

void ScreenshotCaptureWorker::postPrepared(
    quint64 requestId, const QPointer<ScreenshotCaptureCoordinator>& coordinator, bool ok) {
    if (coordinator.isNull()) {
        return;
    }

    QMetaObject::invokeMethod(
        coordinator,
        [coordinator, requestId, ok]() {
            if (!coordinator.isNull()) {
                emit coordinator->prepared(requestId, ok);
            }
        },
        Qt::QueuedConnection);
}

void ScreenshotCaptureWorker::postCaptureResult(
    const QPointer<ScreenshotCaptureCoordinator>& coordinator, ScreenshotCaptureResult result) {
    clip_shot::diagnostics::logEvent(
        QStringLiteral("clip_shot.capture"), QStringLiteral("capture.finished"),
        {{QStringLiteral("operation"), QString::number(result.requestId)},
         {QStringLiteral("count"), result.displays.size()},
         {QStringLiteral("outcome"), result.succeeded       ? QStringLiteral("succeeded")
                                     : coordinator.isNull() ? QStringLiteral("cancelled")
                                                            : QStringLiteral("failed")}},
        result.succeeded || coordinator.isNull() ? QtInfoMsg : QtWarningMsg);
    if (coordinator.isNull()) {
        return;
    }

    QMetaObject::invokeMethod(
        coordinator,
        [coordinator, result = std::move(result)]() mutable {
            if (!coordinator.isNull()) {
                emit coordinator->captureFinished(std::move(result));
            }
        },
        Qt::QueuedConnection);
}
