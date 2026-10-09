#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTSELECTIONRESIZEWORKFLOW_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTSELECTIONRESIZEWORKFLOW_H

#include "clip_shot/presentation/screenshotselectioneditworkflowports.h"
#include "clip_shot/presentation/screenshotselectionparams.h"

#include <QCoreApplication>

class QObject;
class ScreenshotSelectionSettingsStore;

class ScreenshotSelectionResizeWorkflow final {
    Q_DECLARE_TR_FUNCTIONS(ScreenshotSelectionResizeWorkflow)

  public:
    using ApplySelectionCallback = ScreenshotApplySelectionCallback;

    explicit ScreenshotSelectionResizeWorkflow(ScreenshotSelectionSettingsStore& settingsStore);

    [[nodiscard]] bool open(QObject* modalParent, const ScreenshotSelectionResizeRequest& request,
                            ApplySelectionCallback applySelection) const;

  private:
    ScreenshotSelectionSettingsStore& m_settingsStore;
};

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTSELECTIONRESIZEWORKFLOW_H
