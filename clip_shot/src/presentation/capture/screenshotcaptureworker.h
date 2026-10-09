#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTCAPTUREWORKER_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTCAPTUREWORKER_H

#include "clip_shot/presentation/screenshottypes.h"

#include <QObject>
#include <QPointer>

#include <cstdint>

typedef struct ClipCaptureDesktopSessionImpl ClipCaptureDesktopSession;
typedef struct ClipCaptureCancellationTokenImpl ClipCaptureCancellationToken;

class ScreenshotCaptureCoordinator;

class ScreenshotCaptureWorker final : public QObject {
  public:
    ~ScreenshotCaptureWorker() override;

    void prepare(quint64 requestId, const QPointer<ScreenshotCaptureCoordinator>& coordinator);
    void refreshLayout(quint64 requestId,
                       const QPointer<ScreenshotCaptureCoordinator>& coordinator);
    void capture(const ScreenshotCaptureRequest& request,
                 const QPointer<ScreenshotCaptureCoordinator>& coordinator,
                 ClipCaptureCancellationToken* cancellationToken);

  private:
    bool ensureSession(const QVector<std::uint32_t>& excludedWindowIds = {});
    bool sessionPrepared() const;
    bool prepareSessionIfNeeded();
    static void postPrepared(quint64 requestId,
                             const QPointer<ScreenshotCaptureCoordinator>& coordinator, bool ok);
    static void postCaptureResult(const QPointer<ScreenshotCaptureCoordinator>& coordinator,
                                  ScreenshotCaptureResult result);

    ClipCaptureDesktopSession* m_session = nullptr;
    std::uint8_t m_sessionBackend = 0;
    QVector<std::uint32_t> m_sessionExcludedWindowIds;
};

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTCAPTUREWORKER_H
