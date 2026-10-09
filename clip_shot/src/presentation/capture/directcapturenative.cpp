#include "directcapturenative.h"
#include "captureframeimage.h"
#include "captureframegeometry.h"

#include <memory>
#include <QCoreApplication>
#include <utility>

namespace clip_shot::presentation {
namespace {
QString nativeError() {
    return QString::fromUtf8(clip_capture_last_error_message());
}

} // namespace

DirectCaptureFrame captureDirectTarget(const DirectCaptureRequest& request) {
    DirectCaptureFrame result;
    if ((request.target == DirectCaptureTarget::FocusedWindow && request.window == 0) ||
        (request.target == DirectCaptureTarget::CurrentMonitor && request.monitorName.isEmpty())) {
        return result;
    }
    ClipCaptureDesktopSessionConfig config{};
    config.capture_retry_count = 1;
    config.pixel_format = CLIP_CAPTURE_PIXEL_FORMAT_BGRA8;
    const std::unique_ptr<ClipCaptureDesktopSession,
                          decltype(&clip_capture_desktop_session_destroy)>
        session(clip_capture_desktop_session_create(&config), clip_capture_desktop_session_destroy);
    ClipCaptureScreenshotRequest nativeRequest{};
    nativeRequest.version = CLIP_CAPTURE_SCREENSHOT_REQUEST_VERSION;
    nativeRequest.struct_size = sizeof(nativeRequest);
    if (request.captureCursor)
        nativeRequest.flags |= CLIP_CAPTURE_SCREENSHOT_REQUEST_INCLUDE_CURSOR;
    if (request.restoreOriginalScreenColors) {
        nativeRequest.flags |= CLIP_CAPTURE_SCREENSHOT_REQUEST_RESTORE_ORIGINAL_COLORS;
    }
    nativeRequest.focused_window = request.target == DirectCaptureTarget::FocusedWindow
                                       ? static_cast<intptr_t>(request.window)
                                       : 0;
    const std::unique_ptr<ClipCaptureScreenshotResult,
                          decltype(&clip_capture_screenshot_result_destroy)>
        snapshot(session ? clip_capture_desktop_session_capture(session.get(), &nativeRequest)
                         : nullptr,
                 clip_capture_screenshot_result_destroy);
    if (!snapshot) {
        result.error = nativeError();
        return result;
    }
    const size_t count = clip_capture_screenshot_result_display_count(snapshot.get());
    for (size_t index = 0; index < count; ++index) {
        ClipCaptureFrameInfo info{};
        if (!clip_capture_screenshot_result_display_info(snapshot.get(), index, &info) ||
            !capture::validFrameInfo(info)) {
            result.error = nativeError();
            break;
        }
        DirectCaptureDisplay display;
        display.image = capture::imageFromFrameLease(
            clip_capture_screenshot_result_display_retain(snapshot.get(), index), info.rgba_bytes,
            info.rgba_len, info.width, info.height, info.stride_bytes, info.pixel_format,
            capture::FrameAlphaMode::Opaque);
        display.physicalBounds =
            QRect(info.x, info.y, static_cast<int>(info.width), static_cast<int>(info.height));
#ifdef Q_OS_MACOS
        ClipCaptureFrameGeometry geometry{};
        geometry.version = CLIP_CAPTURE_FRAME_GEOMETRY_VERSION;
        geometry.struct_size = sizeof(geometry);
        if (!clip_capture_screenshot_result_display_geometry(snapshot.get(), index, &geometry)) {
            result.error = nativeError();
            break;
        }
        const auto logicalBounds = capture::logicalFrameRect(geometry);
        if (!logicalBounds) {
            result.error = QCoreApplication::translate("DirectCaptureController",
                                                       "The captured display geometry is invalid");
            break;
        }
        display.logicalBounds = *logicalBounds;
        display.nativeDisplayId = geometry.display_id;
#endif
        display.stableId = QString::fromUtf8(info.stable_id);
        display.name = QString::fromUtf8(info.name);
        bool currentMonitor =
            display.name == request.monitorName || display.stableId == request.monitorName;
#ifdef Q_OS_MACOS
        currentMonitor =
            currentMonitor ||
            request.monitorName == QStringLiteral("display:%1").arg(display.nativeDisplayId);
#endif
        if (request.target == DirectCaptureTarget::CurrentMonitor && currentMonitor) {
            result.image = display.image;
            result.logicalBounds = display.logicalBounds;
            result.physicalBounds = display.physicalBounds;
            result.identity = display.stableId;
            result.backend = info.backend_kind;
        }
        if (display.image.isNull())
            break;
        result.displays.push_back(std::move(display));
    }
    if (result.displays.isEmpty() || static_cast<size_t>(result.displays.size()) != count) {
        result.image = {};
        result.displays.clear();
        return result;
    }
    if (request.target == DirectCaptureTarget::FocusedWindow) {
        ClipCaptureWindowFrameInfo info{};
        info.version = CLIP_CAPTURE_WINDOW_FRAME_INFO_VERSION;
        info.struct_size = sizeof(info);
        if (!clip_capture_screenshot_result_focused_window_info(snapshot.get(), &info)) {
            result.error = nativeError();
            return result;
        }
        result.image = capture::imageFromFrameLease(
            clip_capture_screenshot_result_focused_window_retain(snapshot.get()), info.rgba_bytes,
            info.rgba_len, info.width, info.height, info.stride_bytes, info.pixel_format,
            info.backend_kind == CLIP_CAPTURE_BACKEND_SCREEN_CAPTURE_KIT
                ? capture::FrameAlphaMode::Premultiplied
                : capture::FrameAlphaMode::Preserve);
        result.physicalBounds =
            QRect(info.x, info.y, static_cast<int>(info.width), static_cast<int>(info.height));
#ifdef Q_OS_MACOS
        ClipCaptureFrameGeometry geometry{};
        geometry.version = CLIP_CAPTURE_FRAME_GEOMETRY_VERSION;
        geometry.struct_size = sizeof(geometry);
        if (!clip_capture_screenshot_result_focused_window_geometry(snapshot.get(), &geometry)) {
            result.error = nativeError();
            return result;
        }
        const auto logicalBounds = capture::logicalFrameRect(geometry);
        if (!logicalBounds) {
            result.error = QCoreApplication::translate("DirectCaptureController",
                                                       "The captured window geometry is invalid");
            return result;
        }
        result.logicalBounds = *logicalBounds;
#endif
        result.identity = QStringLiteral("window:%1").arg(request.window);
        result.backend = info.backend_kind;
    }
    return result;
}
} // namespace clip_shot::presentation
