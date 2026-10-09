#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTSELECTIONEXPORTWORKFLOW_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTSELECTIONEXPORTWORKFLOW_H

#include "clip_shot/presentation/screenshotgeometry.h"
#include "clip_shot/presentation/screenshotselectionexportworkflowports.h"
#include "clip_shot/presentation/screenshotselectionparams.h"
#include "clip_shot/presentation/screenshottypes.h"

#include <QRect>

#include <functional>

class QObject;
class ScreenshotSelectionModel;
struct ScreenshotCaptureState;

struct ScreenshotSelectionExportWorkflowContext {
    ScreenshotCaptureState& captureState;
    const ScreenshotGeometryMapper& geometry;
    ScreenshotSelectionModel& selection;
    ScreenshotSelectionImageComposerPort& imageComposer;
    ScreenshotSelectionExportDestinationPort& destination;
    ScreenshotSelectionParamsStorePort& selectionSettings;
    QObject& callbackContext;
    std::function<ScreenshotRecognitionResults()> cachedRecognitionResults;
};

class ScreenshotSelectionExportWorkflow final {
  public:
    explicit ScreenshotSelectionExportWorkflow(ScreenshotSelectionExportWorkflowContext context);

    using Completion = std::function<void(bool, QImage)>;
    using CopyCompletion = std::function<void(bool, QImage)>;
    using ResultValidator = std::function<bool()>;

    [[nodiscard]] bool copySelectionToClipboard(ResultValidator validator,
                                                CopyCompletion completion,
                                                quint64 publicationId = 0);
    [[nodiscard]] bool pinSelectionToScreen(ResultValidator validator, Completion completion);

  private:
    [[nodiscard]] ScreenshotSelectionParams currentSelectionParams() const;
    [[nodiscard]] QRect selectionBounds() const;
    void persistSelectionParams(const ScreenshotSelectionParams& params);

    ScreenshotSelectionExportWorkflowContext m_context;
};

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTSELECTIONEXPORTWORKFLOW_H
