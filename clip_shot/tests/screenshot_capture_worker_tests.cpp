#include "screenshotcaptureworker.h"
#include "clip_shot/presentation/capture/screenshotcapturepolicy.h"
#include "clip_shot/presentation/screenshotcapturecoordinator.h"
#include "clip_shot/storage/applicationstorage.h"
#include "clip_shot/storage/settingsadapters.h"

#include "clip_capture.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QSemaphore>
#include <QTemporaryDir>
#include <QTimer>

#include <array>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <memory>

struct ClipCaptureDesktopLayoutImpl {};

struct ClipCaptureDesktopSessionImpl {
    ClipCaptureDesktopSessionConfig config{};
    bool prepared = false;
    QVector<std::uint32_t> excludedWindowIds;
};

struct ClipCaptureFrameLeaseImpl {
    std::shared_ptr<std::array<uint8_t, 4>> pixels =
        std::make_shared<std::array<uint8_t, 4>>(std::array<uint8_t, 4>{10, 20, 30, 255});
};

struct ClipCaptureScreenshotResultImpl {
    uint8_t backend = CLIP_CAPTURE_BACKEND_AUTO;
    uint8_t pixelFormat = CLIP_CAPTURE_PIXEL_FORMAT_RGBA8;
    ClipCaptureFrameLease frame;
};

struct ClipCaptureCancellationTokenImpl {
    std::atomic<bool> canceled{false};
};

struct ClipCaptureCursorSnapshotImpl {
    uint8_t pixel = 0;
};

namespace {
int created = 0;
int destroyed = 0;
int captured = 0;
int leases = 0;
bool failCreation = false;
bool failCapture = false;
bool invalidLayout = false;
uint8_t preparedBackend = CLIP_CAPTURE_BACKEND_AUTO;
uint8_t refreshedBackend = CLIP_CAPTURE_BACKEND_AUTO;
uint32_t capturedFlags = 0;
QVector<std::uint32_t> capturedWindowIds;
std::atomic<uint8_t> liveCursorPixel{71};
std::atomic<int> cursorSnapshots{0};
std::atomic<int> cursorCompositions{0};
bool failCursorSnapshot = false;
bool failCursorComposition = false;
bool blockCapture = false;
QSemaphore captureEntered;
QSemaphore resumeCapture;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void setMode(const char* mode) {
    require(clip_shot::storage::ScreenshotSettings().setApiMode(QString::fromLatin1(mode)),
            "failed to update screenshot API mode");
}

ScreenshotCaptureResult capture(ScreenshotCaptureWorker& worker, bool restoreColors = false,
                                bool captureCursor = false,
                                std::shared_ptr<ClipCaptureCursorSnapshot> cursorSnapshot = {},
                                const QVector<std::uint32_t>& excludedWindowIds = {}) {
    ScreenshotCaptureCoordinator coordinator;
    ScreenshotCaptureResult result;
    bool received = false;
    int layouts = 0;
    QObject::connect(
        &coordinator, &ScreenshotCaptureCoordinator::layoutReady, &coordinator,
        [&](const ScreenshotCaptureLayout& layout) {
            require(!received && layout.requestId == 42 && layout.generation == 42 &&
                        layout.displays.size() == 1,
                    "native layout must be queued before frames with the same request identity");
            ++layouts;
        });
    QObject::connect(&coordinator, &ScreenshotCaptureCoordinator::captureFinished, &coordinator,
                     [&](const ScreenshotCaptureResult& value) {
                         result = value;
                         received = true;
                     });
    ScreenshotCaptureRequest request;
    request.requestId = 42;
    request.restoreOriginalScreenColors = restoreColors;
    request.captureCursor = captureCursor;
    request.cursorSnapshot = std::move(cursorSnapshot);
    request.excludedWindowIds = excludedWindowIds;
    worker.capture(request, &coordinator, nullptr);
    QCoreApplication::sendPostedEvents(&coordinator);
    require(received && result.requestId == request.requestId, "capture result was not delivered");
    require(!result.succeeded || layouts == 1,
            "successful initial capture must publish exactly one layout");
    return result;
}

void recaptureExclusionsReplaceSessionAndDoNotLeak() {
    ScreenshotCaptureWorker worker;
    worker.prepare(1, {});
    const int before = created;
    const QVector<std::uint32_t> windows{101, 202};
    require(capture(worker, false, false, {}, windows).succeeded && capturedWindowIds == windows &&
                created == before + 1,
            "recapture must replace a prewarmed session to exclude overlay and toolbar");
    require(capture(worker, false, false, {}, windows).succeeded && created == before + 1,
            "unchanged exclusions must reuse the native capture session");
    const QVector<std::uint32_t> replacedWindows{303, 404};
    require(capture(worker, false, false, {}, replacedWindows).succeeded &&
                capturedWindowIds == replacedWindows,
            "recapture must replace stale native window IDs");
    failCreation = true;
    const int capturesBeforeFailure = captured;
    require(!capture(worker, false, false, {}, windows).succeeded &&
                captured == capturesBeforeFailure,
            "failed exclusion changes must not capture using stale exclusions");
    failCreation = false;
    require(capture(worker, false, false, {}, windows).succeeded && capturedWindowIds == windows,
            "recapture must recover with the requested exclusions");
    require(capture(worker).succeeded && capturedWindowIds.isEmpty(),
            "ordinary capture must clear recapture exclusions");
}

void invalidLayoutDoesNotAcquireFrames() {
    ScreenshotCaptureWorker worker;
    const int before = captured;
    invalidLayout = true;
    require(!capture(worker).succeeded && captured == before,
            "invalid native layout must fail before frame acquisition");
    invalidLayout = false;
    require(capture(worker).succeeded, "valid layout must recover on the next invocation");
}

void requireBackend(ScreenshotCaptureWorker& worker, uint8_t expected) {
    const auto result = capture(worker);
    require(result.succeeded && result.displays.size() == 1, "capture did not return a display");
    require(static_cast<uint8_t>(result.displays.front().backend) == expected,
            "regular screenshot used the previous API mode's backend");
    require(!result.displays.front().image.isNull(), "capture did not retain the frame");
}

uint8_t backendFor(const char* mode) {
    namespace policy = clip_shot::presentation::capture;
    auto requested = policy::screenshotApiModeFromValue(mode);
    if (requested == policy::ScreenshotApiMode::Auto) {
        requested = policy::resolveAutoScreenshotApiMode();
    }
    return policy::nativeBackendForNormalScreenshot(requested);
}

void changedModeReplacesPrewarmedSession() {
    setMode("dxgi");
    ScreenshotCaptureWorker worker;
    worker.prepare(1, {});
    require(preparedBackend == backendFor("dxgi"), "DXGI session was not prewarmed");
    setMode("gdi");
    requireBackend(worker, backendFor("gdi"));
}

void allModeTransitionsApplyWithoutRestart() {
    const std::array<const char*, 4> modes{"dxgi", "wgc", "gdi", "auto"};
    for (const auto* before : modes) {
        for (const auto* after : modes) {
            setMode(before);
            ScreenshotCaptureWorker worker;
            requireBackend(worker, backendFor(before));
            const int previousCreated = created;
            const int previousDestroyed = destroyed;
            setMode(after);
            requireBackend(worker, backendFor(after));
            const int replacements = backendFor(before) == backendFor(after) ? 0 : 1;
            require(created == previousCreated + replacements &&
                        destroyed == previousDestroyed + replacements,
                    "session reuse must depend on the resolved capture backend");
            requireBackend(worker, backendFor(after));
            require(created == previousCreated + replacements,
                    "unchanged API mode discarded the warm capture session");
        }
    }
}

void colorRestorationAppliesAcrossBackendChanges() {
    ScreenshotCaptureWorker worker;
    for (const auto* mode : {"dxgi", "wgc", "gdi", "auto"}) {
        setMode(mode);
        for (const bool restoreColors : {true, false}) {
            require(capture(worker, restoreColors).succeeded, "color restoration capture failed");
            require(((capturedFlags & CLIP_CAPTURE_SCREENSHOT_REQUEST_RESTORE_ORIGINAL_COLORS) !=
                     0) == restoreColors,
                    "API selection must preserve the requested color restoration policy");
        }
    }
}

void cursorCaptureAppliesAcrossBackendChanges() {
    ScreenshotCaptureWorker worker;
    for (const auto* mode : {"dxgi", "wgc", "gdi", "auto"}) {
        setMode(mode);
        for (const bool captureCursor : {false, true}) {
            require(capture(worker, false, captureCursor).succeeded,
                    "cursor policy capture failed");
            require(
                ((capturedFlags & CLIP_CAPTURE_SCREENSHOT_REQUEST_INCLUDE_CURSOR) != 0) == false &&
                    (capturedFlags & CLIP_CAPTURE_SCREENSHOT_REQUEST_RESTORE_ORIGINAL_COLORS) == 0,
                "cursor policy must use its independent native request flag");
        }
    }
}

void preparationAndLayoutRefreshUseCurrentMode() {
    setMode("dxgi");
    ScreenshotCaptureWorker worker;
    worker.prepare(1, {});
    setMode("gdi");
    worker.prepare(2, {});
    require(preparedBackend == backendFor("gdi"),
            "preparation reused a session with an obsolete backend");
    setMode("wgc");
    worker.refreshLayout(3, {});
    require(refreshedBackend == backendFor("wgc"),
            "layout refresh reused a session with an obsolete backend");
    requireBackend(worker, backendFor("wgc"));
}

void failedReplacementDoesNotCaptureWithOldBackend() {
    setMode("dxgi");
    ScreenshotCaptureWorker worker;
#ifndef Q_OS_MACOS
    requireBackend(worker, backendFor("dxgi"));
#endif
    setMode("gdi");
    const int previousCaptured = captured;
    failCreation = true;
    const auto failed = capture(worker);
    failCreation = false;
    require(!failed.succeeded && !failed.errorMessage.isEmpty() && failed.displays.isEmpty(),
            "failed backend replacement was not reported");
    require(captured == previousCaptured, "failed replacement captured with the stale backend");
    requireBackend(worker, backendFor("gdi"));
}

void modeChangeAfterCaptureFailurePreservesRetainedFrame() {
    setMode("dxgi");
    ScreenshotCaptureWorker worker;
    const auto retained = capture(worker);
    require(retained.succeeded, "initial capture failed");
    failCapture = true;
    require(!capture(worker).succeeded, "native capture failure was not reported");
    failCapture = false;
    setMode("gdi");
    requireBackend(worker, backendFor("gdi"));
    require(retained.displays.front().image.constBits()[0] == 10,
            "replacing a session invalidated an already delivered image");
}

#if defined(Q_OS_WIN) || defined(_WIN32)
void captureOwnsCursorBeforeDispatch(ScreenshotCapturePurpose purpose, bool cancel) {
    setMode("dxgi");
    ScreenshotCaptureResult result;
    bool received = false;
    const int compositionsBefore = cursorCompositions;
    {
        ScreenshotCaptureCoordinator coordinator;
        QEventLoop loop;
        QObject::connect(&coordinator, &ScreenshotCaptureCoordinator::captureFinished, &loop,
                         [&](const ScreenshotCaptureResult& value) {
                             result = value;
                             received = true;
                             loop.quit();
                         });
        ScreenshotCaptureRequest request;
        request.requestId = 91;
        request.purpose = purpose;
        request.captureCursor = true;
        liveCursorPixel = 71;
        blockCapture = true;
        coordinator.captureAsync(request);
        require(cursorSnapshots == 1, "capture must own its cursor before returning to the UI");
        require(captureEntered.tryAcquire(1, 3000), "capture worker did not reach the barrier");
        require((capturedFlags & CLIP_CAPTURE_SCREENSHOT_REQUEST_INCLUDE_CURSOR) == 0,
                "snapshot capture must disable backend cursor composition");
        // Simulate a cursor change while desktop capture is still running.
        liveCursorPixel = 29;
        if (cancel) {
            coordinator.cancelActiveCapture();
        }
        resumeCapture.release();
        QTimer::singleShot(3000, &loop, &QEventLoop::quit);
        loop.exec();
        coordinator.shutdown();
        blockCapture = false;
    }
    require(received && result.requestId == 91 && result.purpose == purpose,
            "asynchronous capture must deliver its original identity");
    require(cursorSnapshots == 0, "completion and cancellation must release the owned cursor");
    if (cancel) {
        require(!result.succeeded && cursorCompositions == compositionsBefore,
                "canceled capture must not composite a cursor or publish an image");
    } else {
        require(result.succeeded && result.displays.front().image.constBits()[0] == 10 &&
                    result.displays.front().cursorPatch.constBits()[0] == 71 &&
                    result.cursorAvailable && cursorCompositions == compositionsBefore + 1,
                "capture must use the saved cursor, never the cursor changed during capture");
    }
}

void cursorSnapshotFailureDoesNotDispatchCapture() {
    for (const auto purpose :
         {ScreenshotCapturePurpose::Initial, ScreenshotCapturePurpose::Recapture}) {
        ScreenshotCaptureCoordinator coordinator;
        QEventLoop loop;
        ScreenshotCaptureResult capturedResult;
        bool received = false;
        QObject::connect(&coordinator, &ScreenshotCaptureCoordinator::captureFinished, &loop,
                         [&](const ScreenshotCaptureResult& result) {
                             capturedResult = result;
                             received = true;
                             loop.quit();
                         });
        ScreenshotCaptureRequest request;
        request.purpose = purpose;
        request.captureCursor = true;
        failCursorSnapshot = true;
        coordinator.captureAsync(request);
        failCursorSnapshot = false;
        QTimer::singleShot(3000, &loop, &QEventLoop::quit);
        loop.exec();
        coordinator.shutdown();
        require(received && capturedResult.succeeded && !capturedResult.cursorAvailable &&
                    capturedResult.displays.front().image.constBits()[0] == 10 &&
                    cursorSnapshots == 0,
                "missing cursor data must preserve clean capture without backend substitution");
    }
}

#endif

void cursorCompositionFailureDoesNotPublishAnImage() {
    ScreenshotCaptureWorker worker;
    auto snapshot = std::shared_ptr<ClipCaptureCursorSnapshot>(
        clip_capture_cursor_snapshot_create(), clip_capture_cursor_snapshot_destroy);
    failCursorComposition = true;
    const auto failed = capture(worker, false, true, snapshot);
    require(failed.succeeded && !failed.cursorAvailable &&
                failed.displays.front().image.constBits()[0] == 10,
            "failed patch generation must preserve the clean screenshot");
    const auto disabled = capture(worker, false, false, snapshot);
    require(disabled.succeeded && disabled.displays.front().image.constBits()[0] == 10,
            "disabled cursor capture must ignore an attached snapshot");
    failCursorComposition = false;
}
} // namespace

// Substitute only the native API; settings, policy, worker, and result delivery are production
// code.
extern "C" {
ClipCaptureDesktopLayout*
clip_capture_desktop_session_layout_snapshot(ClipCaptureDesktopSession* session, uint8_t refresh) {
    if (refresh)
        clip_capture_desktop_session_refresh_layout(session);
    return new ClipCaptureDesktopLayout;
}
void clip_capture_desktop_layout_destroy(ClipCaptureDesktopLayout* layout) {
    delete layout;
}
size_t clip_capture_desktop_layout_count(const ClipCaptureDesktopLayout*) {
    return 1;
}
uint8_t clip_capture_desktop_layout_display(const ClipCaptureDesktopLayout*, size_t,
                                            ClipCaptureDisplayDescriptor* d) {
    d->stable_id = "test";
    d->name = "test";
    d->x = 0;
    d->y = 0;
    d->width = 1;
    d->height = 1;
    d->pixel_width = 1;
    d->pixel_height = 1;
    d->backing_scale = invalidLayout ? 0 : 1;
#ifdef Q_OS_MACOS
    d->coordinate_space = 1;
    d->display_id = 1;
#endif
    return 1;
}
ClipCaptureScreenshotResult*
clip_capture_desktop_session_capture_with_layout(ClipCaptureDesktopSession* session,
                                                 const ClipCaptureScreenshotRequest* request,
                                                 const ClipCaptureDesktopLayout*) {
    return clip_capture_desktop_session_capture(session, request);
}

ClipCaptureDesktopSession*
clip_capture_desktop_session_create(const ClipCaptureDesktopSessionConfig* config) {
    if (failCreation) {
        return nullptr;
    }
    ++created;
    auto* session = new ClipCaptureDesktopSession{*config, false, {}};
    for (size_t index = 0; index < config->exclusions.window_count; ++index) {
        session->excludedWindowIds.push_back(config->exclusions.windows[index]);
    }
    return session;
}

void clip_capture_desktop_session_destroy(ClipCaptureDesktopSession* session) {
    ++destroyed;
    delete session;
}

uint8_t clip_capture_desktop_session_prepare(ClipCaptureDesktopSession* session) {
    preparedBackend = session->config.capture_backend;
    session->prepared = true;
    return 1;
}

uint8_t clip_capture_desktop_session_state(ClipCaptureDesktopSession* session,
                                           ClipCaptureDesktopSessionState* state) {
    *state = {};
    state->prepared = session->prepared ? 1 : 0;
    return 1;
}

uint8_t clip_capture_desktop_session_refresh_layout(ClipCaptureDesktopSession* session) {
    refreshedBackend = session->config.capture_backend;
    return 1;
}

uint8_t clip_capture_desktop_session_reset_to_prepared(ClipCaptureDesktopSession* session) {
    session->prepared = true;
    return 1;
}

ClipCaptureScreenshotResult*
clip_capture_desktop_session_capture(ClipCaptureDesktopSession* session,
                                     const ClipCaptureScreenshotRequest* request) {
    capturedFlags = request->flags;
    capturedWindowIds = session->excludedWindowIds;
    ++captured;
    if (blockCapture) {
        captureEntered.release();
        resumeCapture.acquire();
    }
    if (failCapture ||
        (request->cancellation_token != nullptr && request->cancellation_token->canceled)) {
        return nullptr;
    }
    return new ClipCaptureScreenshotResult{
        session->config.capture_backend, session->config.pixel_format, {}};
}

ClipCaptureCursorSnapshot* clip_capture_cursor_snapshot_create() {
    if (failCursorSnapshot) {
        return nullptr;
    }
    ++cursorSnapshots;
    return new ClipCaptureCursorSnapshot{liveCursorPixel.load()};
}

void clip_capture_cursor_snapshot_destroy(ClipCaptureCursorSnapshot* snapshot) {
    if (snapshot != nullptr) {
        --cursorSnapshots;
        delete snapshot;
    }
}

uint8_t clip_capture_screenshot_result_composite_cursor(ClipCaptureScreenshotResult* result,
                                                        const ClipCaptureCursorSnapshot* snapshot) {
    if (failCursorComposition) {
        return 0;
    }
    ++cursorCompositions;
    (*result->frame.pixels)[0] = snapshot->pixel;
    return 1;
}

uint8_t clip_capture_screenshot_result_cursor_patch(const ClipCaptureScreenshotResult* result,
                                                    const ClipCaptureCursorSnapshot* snapshot,
                                                    size_t index, ClipCaptureFrameInfo* info,
                                                    ClipCaptureFrameLease** lease) {
    *lease = nullptr;
    if (failCursorComposition)
        return 0;
    ++cursorCompositions;
    ++leases;
    *lease = new ClipCaptureFrameLease;
    (*(*lease)->pixels)[0] = snapshot->pixel;
    clip_capture_screenshot_result_display_info(result, index, info);
    info->rgba_bytes = (*lease)->pixels->data();
    return 1;
}

size_t clip_capture_screenshot_result_display_count(const ClipCaptureScreenshotResult*) {
    return 1;
}

uint8_t clip_capture_screenshot_result_display_info(const ClipCaptureScreenshotResult* result,
                                                    size_t, ClipCaptureFrameInfo* info) {
    *info = {};
    info->width = 1;
    info->height = 1;
    info->stride_bytes = 4;
    info->rgba_bytes = result->frame.pixels->data();
    info->rgba_len = result->frame.pixels->size();
    info->backend_kind = result->backend;
    info->pixel_format = result->pixelFormat;
    return 1;
}

uint8_t clip_capture_screenshot_result_display_geometry(const ClipCaptureScreenshotResult*,
                                                        size_t index,
                                                        ClipCaptureFrameGeometry* geometry) {
    geometry->coordinate_space = 1;
    geometry->display_id = static_cast<uint32_t>(index + 1);
    geometry->x = 0;
    geometry->y = 0;
    geometry->width = 1;
    geometry->height = 1;
    geometry->backing_scale = 1;
    return 1;
}
uint8_t clip_capture_screenshot_result_focused_window_geometry(const ClipCaptureScreenshotResult*,
                                                               ClipCaptureFrameGeometry* geometry) {
    return clip_capture_screenshot_result_display_geometry(nullptr, 0, geometry);
}

ClipCaptureFrameLease*
clip_capture_screenshot_result_display_retain(const ClipCaptureScreenshotResult* result, size_t) {
    ++leases;
    return new ClipCaptureFrameLease(result->frame);
}

void clip_capture_frame_lease_release(ClipCaptureFrameLease* lease) {
    --leases;
    delete lease;
}

void clip_capture_screenshot_result_destroy(ClipCaptureScreenshotResult* result) {
    delete result;
}

const char* clip_capture_last_error_message() {
    return "injected native capture failure";
}

ClipCaptureCancellationToken* clip_capture_cancellation_token_create() {
    return new ClipCaptureCancellationToken;
}
void clip_capture_cancellation_token_cancel(ClipCaptureCancellationToken* token) {
    token->canceled = true;
}
void clip_capture_cancellation_token_destroy(ClipCaptureCancellationToken* token) {
    delete token;
}
} // extern "C"

int main(int argc, char** argv) {
    QCoreApplication application(argc, argv);
    QTemporaryDir temporary;
    require(temporary.isValid(), "failed to create isolated settings directory");
    auto& storage = clip_shot::storage::ApplicationStorage::instance();
    static_cast<void>(storage.initialize({temporary.filePath(QStringLiteral("bin")),
                                          temporary.filePath(QStringLiteral("data")), 60000}));
    recaptureExclusionsReplaceSessionAndDoNotLeak();
    invalidLayoutDoesNotAcquireFrames();
    changedModeReplacesPrewarmedSession();
    allModeTransitionsApplyWithoutRestart();
    colorRestorationAppliesAcrossBackendChanges();
    cursorCaptureAppliesAcrossBackendChanges();
    preparationAndLayoutRefreshUseCurrentMode();
    failedReplacementDoesNotCaptureWithOldBackend();
    modeChangeAfterCaptureFailurePreservesRetainedFrame();
#if defined(Q_OS_WIN) || defined(_WIN32)
    captureOwnsCursorBeforeDispatch(ScreenshotCapturePurpose::Initial, false);
    captureOwnsCursorBeforeDispatch(ScreenshotCapturePurpose::Initial, true);
    captureOwnsCursorBeforeDispatch(ScreenshotCapturePurpose::Recapture, false);
    captureOwnsCursorBeforeDispatch(ScreenshotCapturePurpose::Recapture, true);
    cursorSnapshotFailureDoesNotDispatchCapture();
#endif
    cursorCompositionFailureDoesNotPublishAnImage();
    require(cursorSnapshots == 0, "worker leaked a cursor snapshot");
    require(created == destroyed, "worker leaked a native capture session");
    require(leases == 0, "worker leaked a native frame lease");
    storage.shutdown();
    return 0;
}
