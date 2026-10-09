#pragma once

#include "clip_canvas_ffi_handles.h"
#include "clip_draw_engine.h"

#include <QByteArray>

#include <cstdint>
#include <vector>

namespace clip_canvas_commands {

struct MutationResult {
    bool success = false;
    ScopedChangedViewportList changedViewports;
};

struct PairedMutationResult {
    bool success = false;
    ScopedChangedViewportList firstChangedViewports;
    ScopedChangedViewportList secondChangedViewports;
};

struct CreateSerialNumberTextResult {
    bool success = false;
    ScopedChangedViewportList changedViewports;
    ClipElementId textId{};
    bool hasTextId = false;
};

struct CommitTextRequest {
    ClipElementId elementId{};
    bool hasExistingElement = false;
    double centerX = 0.0;
    double centerY = 0.0;
    const char* utf8Data = nullptr;
    std::uint32_t utf8Len = 0;
    ClipTextLayoutSize measuredLayout{1.0, 1.0, 0.0, 0.0};
    ClipTextStyle style{};
    bool autoResize = false;
    bool updateDefaultStyle = false;
    ClipElementId arrowId{};

    ClipTextCommitDraft toAbi() const;
};

struct ActiveTextDraftPresentationRequest {
    ClipElementId elementId{};
    bool hasExistingElement = false;
    double centerX = 0.0;
    double centerY = 0.0;
    double width = 1.0;
    double height = 1.0;
    double rotation = 0.0;
    double contentWidth = 0.0;
    double contentHeight = 0.0;
    const char* utf8Data = nullptr;
    std::uint32_t utf8Len = 0;
    ClipTextStyle style{};
    bool autoResize = false;

    ClipElementId arrowId{};
    ClipActiveTextDraftPresentation toAbi() const;
};

struct ProcessInputResult {
    bool success = false;
    ScopedChangedViewportList changedViewports;
    ClipInteractionOutput output{};
};

struct TextResizeMeasurementResult {
    bool success = false;
    bool active = false;
    ClipTextElementInfo info{};
};

MutationResult setActiveTool(ClipRuntime runtime, ClipViewport viewport, ClipActiveTool tool);
MutationResult undo(ClipRuntime runtime);
MutationResult redo(ClipRuntime runtime);
MutationResult resetEditingState(ClipRuntime runtime, ClipViewport viewport);
MutationResult selectElement(ClipRuntime runtime, ClipViewport viewport, ClipElementId id);
MutationResult deleteSelected(ClipRuntime runtime, ClipViewport viewport);
MutationResult deleteAllElements(ClipRuntime runtime, ClipViewport viewport);
MutationResult duplicateSelected(ClipRuntime runtime, ClipViewport viewport, double offsetX,
                                 double offsetY);
MutationResult insertDrawTemplate(ClipRuntime runtime, ClipViewport viewport,
                                  const QByteArray& payload, double centerX, double centerY);
MutationResult reorderSelected(ClipRuntime runtime, ClipViewport viewport, std::uint32_t action);
MutationResult alignSelected(ClipRuntime runtime, ClipViewport viewport, std::uint32_t alignment);
MutationResult setSelectedOpacity(ClipRuntime runtime, ClipViewport viewport, double opacity);
MutationResult adjustSelectedSerialNumbers(ClipRuntime runtime, ClipViewport viewport,
                                           std::int64_t delta);
CreateSerialNumberTextResult createSerialNumberText(ClipRuntime runtime, ClipViewport viewport,
                                                    const ClipTextLayoutSize& layout);
MutationResult commitText(ClipRuntime runtime, ClipViewport viewport,
                          const CommitTextRequest& request);
ProcessInputResult processInput(ClipRuntime runtime, ClipViewport viewport,
                                const ClipInputEvent& event);
ProcessInputResult processPointerMoveBatch(ClipRuntime runtime, ClipViewport viewport,
                                           const std::vector<ClipInputEvent>& events);
TextResizeMeasurementResult activeTextResizeMeasurement(ClipRuntime runtime, ClipViewport viewport);
MutationResult applyActiveTextResizeMeasurement(ClipRuntime runtime, ClipViewport viewport,
                                                const ClipTextLayoutSize& layout);
MutationResult setActiveTextDraftPresentation(ClipRuntime runtime, ClipViewport viewport,
                                              const ActiveTextDraftPresentationRequest& request);
MutationResult clearActiveTextDraftPresentation(ClipRuntime runtime, ClipViewport viewport);
MutationResult setDistanceStylePatch(ClipRuntime runtime, ClipViewport viewport,
                                     const ClipDistanceStyle& style, std::uint32_t properties);
MutationResult setDistancePixelScale(ClipRuntime runtime, ClipViewport viewport, double scaleX,
                                     double scaleY);
MutationResult setShapeStylePatch(ClipRuntime runtime, ClipViewport viewport,
                                  const ClipShapeStyle& style, std::uint32_t properties,
                                  ClipShapeKind kind);
MutationResult setWatermarkConfig(ClipRuntime runtime, ClipViewport viewport,
                                  const ClipWatermarkConfig& config);
MutationResult setSpotlightConfig(ClipRuntime runtime, ClipViewport viewport,
                                  const ClipSpotlightConfig& config);
MutationResult setFilterStyle(ClipRuntime runtime, ClipViewport viewport,
                              const ClipFilterStyle& style, std::uint32_t properties);
MutationResult setFilterCreationStyle(ClipRuntime runtime, ClipViewport viewport,
                                      const ClipFilterStyle& style, std::uint32_t properties,
                                      ClipActiveTool tool);
MutationResult setBrushEraserCreationStyle(ClipRuntime runtime, ClipViewport viewport,
                                           const ClipBrushEraserStyle& style,
                                           std::uint32_t properties);
MutationResult setTextStyle(ClipRuntime runtime, ClipViewport viewport, const ClipTextStyle& style,
                            std::uint32_t properties,
                            const std::vector<ClipTextLayoutOverride>& layouts = {});
MutationResult setTextCreationStyle(ClipRuntime runtime, ClipViewport viewport,
                                    const ClipTextStyle& style, std::uint32_t properties);
MutationResult setSerialNumberStylePatch(ClipRuntime runtime, ClipViewport viewport,
                                         const ClipSerialNumberStyle& style,
                                         std::uint32_t properties);
MutationResult setSerialNumberStyle(ClipRuntime runtime, ClipViewport viewport,
                                    const ClipSerialNumberStyle& style);
PairedMutationResult setSnapConfig(ClipRuntime runtime, ClipViewport viewport,
                                   ClipSnapConfig config);
MutationResult setSnapGuideTargets(ClipRuntime runtime, ClipViewport viewport,
                                   const double* verticalXs, size_t verticalCount,
                                   const double* horizontalYs, size_t horizontalCount);
PairedMutationResult setGridConfig(ClipRuntime runtime, ClipViewport viewport,
                                   ClipGridConfig config);

} // namespace clip_canvas_commands
