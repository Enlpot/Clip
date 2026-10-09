#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTCAPTUREDISPLAYMODELRECONCILER_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTCAPTUREDISPLAYMODELRECONCILER_H

#include "clip_shot/presentation/screenshottypes.h"

#include <QVector>

class ScreenshotDisplaySession;

class ScreenshotCaptureDisplayModelReconciler final {
  public:
    ScreenshotCaptureDisplayModelReconciler() = delete;

    static void applySnapshots(ScreenshotDisplaySession& displaySession,
                               const QVector<CapturedDisplayModel>& snapshots);
    static void clearCaptureMetadata(CapturedDisplayModel& display);
};

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTCAPTUREDISPLAYMODELRECONCILER_H
