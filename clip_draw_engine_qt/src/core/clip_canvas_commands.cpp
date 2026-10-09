#include "clip_canvas_commands.h"

namespace clip_canvas_commands {
namespace {

bool hasViewport(ClipRuntime runtime, ClipViewport viewport) {
    return runtime != nullptr && viewport != nullptr;
}

bool setSnapConfigRaw(ClipRuntime runtime, ClipViewport viewport, ClipSnapConfig* config,
                      ClipChangedViewportList* changedViewports) {
    return clip_viewport_set_snap_config_ex(runtime, viewport, config, changedViewports) == CLIP_OK;
}

bool getSnapConfigRaw(ClipRuntime runtime, ClipViewport viewport, ClipSnapConfig* config) {
    return clip_viewport_get_snap_config(runtime, viewport, config) == CLIP_OK;
}

bool setGridConfigRaw(ClipRuntime runtime, ClipViewport viewport, ClipGridConfig* config,
                      ClipChangedViewportList* changedViewports) {
    return clip_viewport_set_grid_config_ex(runtime, viewport, config, changedViewports) == CLIP_OK;
}

bool getGridConfigRaw(ClipRuntime runtime, ClipViewport viewport, ClipGridConfig* config) {
    return clip_viewport_get_grid_config(runtime, viewport, config) == CLIP_OK;
}

} // namespace

ClipTextCommitDraft CommitTextRequest::toAbi() const {
    ClipTextCommitDraft draft{};
    draft.element_id = elementId;
    draft.arrow_id = arrowId;
    draft.has_existing_element = hasExistingElement ? 1 : 0;
    draft.auto_resize = autoResize ? 1 : 0;
    draft.update_default_style = updateDefaultStyle ? 1 : 0;
    draft.center_x = centerX;
    draft.center_y = centerY;
    draft.text_utf8 = utf8Data;
    draft.text_utf8_len = utf8Len;
    draft.measured_layout = measuredLayout;
    draft.style = style;
    return draft;
}

ClipActiveTextDraftPresentation ActiveTextDraftPresentationRequest::toAbi() const {
    ClipActiveTextDraftPresentation draft{};
    draft.element_id = elementId;
    draft.arrow_id = arrowId;
    draft.has_existing_element = hasExistingElement ? 1 : 0;
    draft.auto_resize = autoResize ? 1 : 0;
    draft.center_x = centerX;
    draft.center_y = centerY;
    draft.width = width;
    draft.height = height;
    draft.rotation = rotation;
    draft.content_width = contentWidth;
    draft.content_height = contentHeight;
    draft.text_utf8 = utf8Data;
    draft.text_utf8_len = utf8Len;
    draft.style = style;
    return draft;
}

MutationResult setActiveTool(ClipRuntime runtime, ClipViewport viewport, ClipActiveTool tool) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success = clip_viewport_set_active_tool_ex(
                         runtime, viewport, tool, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult undo(ClipRuntime runtime) {
    MutationResult result;
    if (runtime == nullptr) {
        return result;
    }
    result.success = clip_runtime_undo_ex(runtime, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult redo(ClipRuntime runtime) {
    MutationResult result;
    if (runtime == nullptr) {
        return result;
    }
    result.success = clip_runtime_redo_ex(runtime, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult resetEditingState(ClipRuntime runtime, ClipViewport viewport) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success = clip_viewport_reset_editing_state_ex(
                         runtime, viewport, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult selectElement(ClipRuntime runtime, ClipViewport viewport, ClipElementId id) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success = clip_viewport_select_element_ex(runtime, viewport, id,
                                                     result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult deleteSelected(ClipRuntime runtime, ClipViewport viewport) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success = clip_viewport_delete_selected_ex(
                         runtime, viewport, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult deleteAllElements(ClipRuntime runtime, ClipViewport viewport) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success = clip_viewport_delete_all_elements_ex(
                         runtime, viewport, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult duplicateSelected(ClipRuntime runtime, ClipViewport viewport, double offsetX,
                                 double offsetY) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success =
        clip_viewport_duplicate_selected_ex(runtime, viewport, offsetX, offsetY,
                                            result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult insertDrawTemplate(ClipRuntime runtime, ClipViewport viewport,
                                  const QByteArray& payload, double centerX, double centerY) {
    MutationResult result;
    if (!hasViewport(runtime, viewport) || payload.isEmpty()) {
        return result;
    }
    result.success =
        clip_viewport_insert_draw_template_ex(
            runtime, viewport, reinterpret_cast<const std::uint8_t*>(payload.constData()),
            static_cast<std::size_t>(payload.size()), centerX, centerY,
            result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult reorderSelected(ClipRuntime runtime, ClipViewport viewport, std::uint32_t action) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success = clip_viewport_reorder_selected_ex(
                         runtime, viewport, action, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult alignSelected(ClipRuntime runtime, ClipViewport viewport, std::uint32_t alignment) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success = clip_viewport_align_selected_ex(runtime, viewport, alignment,
                                                     result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult setSelectedOpacity(ClipRuntime runtime, ClipViewport viewport, double opacity) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success = clip_viewport_set_selected_opacity_ex(
                         runtime, viewport, opacity, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult adjustSelectedSerialNumbers(ClipRuntime runtime, ClipViewport viewport,
                                           std::int64_t delta) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success = clip_viewport_adjust_selected_serial_numbers_ex(
                         runtime, viewport, delta, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

CreateSerialNumberTextResult createSerialNumberText(ClipRuntime runtime, ClipViewport viewport,
                                                    const ClipTextLayoutSize& layout) {
    CreateSerialNumberTextResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }

    std::uint8_t hasTextId = 0;
    result.success = clip_viewport_create_serial_number_text_ex(
                         runtime, viewport, layout.width, layout.height, &result.textId, &hasTextId,
                         result.changedViewports.outParam()) == CLIP_OK;
    result.hasTextId = hasTextId != 0;
    return result;
}

MutationResult commitText(ClipRuntime runtime, ClipViewport viewport,
                          const CommitTextRequest& request) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    if (request.utf8Data == nullptr && request.utf8Len > 0) {
        return result;
    }

    const ClipTextCommitDraft draft = request.toAbi();
    result.success = clip_viewport_commit_text_draft_payload_ex(
                         runtime, viewport, &draft, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

ProcessInputResult processInput(ClipRuntime runtime, ClipViewport viewport,
                                const ClipInputEvent& event) {
    ProcessInputResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }

    result.success = clip_viewport_process_input_ex(runtime, viewport, &event, &result.output,
                                                    result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

ProcessInputResult processPointerMoveBatch(ClipRuntime runtime, ClipViewport viewport,
                                           const std::vector<ClipInputEvent>& events) {
    ProcessInputResult result;
    if (!hasViewport(runtime, viewport) || events.empty()) {
        return result;
    }
    result.success =
        clip_viewport_process_pointer_move_batch_ex(
            runtime, viewport, events.data(), static_cast<std::uint32_t>(events.size()),
            &result.output, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

TextResizeMeasurementResult activeTextResizeMeasurement(ClipRuntime runtime,
                                                        ClipViewport viewport) {
    TextResizeMeasurementResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    std::uint8_t active = 0;
    result.success = clip_viewport_get_active_text_resize_measurement(
                         runtime, viewport, &result.info, &active) == CLIP_OK;
    result.active = active != 0;
    return result;
}

MutationResult applyActiveTextResizeMeasurement(ClipRuntime runtime, ClipViewport viewport,
                                                const ClipTextLayoutSize& layout) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success = clip_viewport_apply_active_text_resize_measurement_ex(
                         runtime, viewport, &layout, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult setActiveTextDraftPresentation(ClipRuntime runtime, ClipViewport viewport,
                                              const ActiveTextDraftPresentationRequest& request) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    if (request.utf8Data == nullptr && request.utf8Len > 0) {
        return result;
    }

    const ClipActiveTextDraftPresentation draft = request.toAbi();
    result.success = clip_viewport_set_active_text_draft_presentation_ex(
                         runtime, viewport, &draft, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult clearActiveTextDraftPresentation(ClipRuntime runtime, ClipViewport viewport) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success = clip_viewport_clear_active_text_draft_presentation_ex(
                         runtime, viewport, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult setShapeStylePatch(ClipRuntime runtime, ClipViewport viewport,
                                  const ClipShapeStyle& style, std::uint32_t properties,
                                  ClipShapeKind kind) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success =
        clip_viewport_set_shape_style_patch_ex(runtime, viewport, &style, properties, kind,
                                               result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult setDistanceStylePatch(ClipRuntime runtime, ClipViewport viewport,
                                     const ClipDistanceStyle& style, std::uint32_t properties) {
    MutationResult result;
    if (!hasViewport(runtime, viewport))
        return result;
    result.success =
        clip_viewport_set_distance_style_patch_ex(runtime, viewport, &style, properties,
                                                  result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult setDistancePixelScale(ClipRuntime runtime, ClipViewport viewport, double scaleX,
                                     double scaleY) {
    MutationResult result;
    if (!hasViewport(runtime, viewport))
        return result;
    result.success =
        clip_viewport_set_distance_pixel_scale_ex(runtime, viewport, scaleX, scaleY,
                                                  result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult setWatermarkConfig(ClipRuntime runtime, ClipViewport viewport,
                                  const ClipWatermarkConfig& config) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success = clip_viewport_set_watermark_config_ex(
                         runtime, viewport, &config, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult setSpotlightConfig(ClipRuntime runtime, ClipViewport viewport,
                                  const ClipSpotlightConfig& config) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success = clip_viewport_set_spotlight_config_ex(
                         runtime, viewport, &config, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult setFilterStyle(ClipRuntime runtime, ClipViewport viewport,
                              const ClipFilterStyle& style, std::uint32_t properties) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success =
        clip_viewport_set_filter_style_ex(runtime, viewport, &style, properties,
                                          result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult setFilterCreationStyle(ClipRuntime runtime, ClipViewport viewport,
                                      const ClipFilterStyle& style, std::uint32_t properties,
                                      ClipActiveTool tool) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success =
        clip_viewport_set_filter_creation_style_ex(runtime, viewport, &style, properties, tool,
                                                   result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult setBrushEraserCreationStyle(ClipRuntime runtime, ClipViewport viewport,
                                           const ClipBrushEraserStyle& style,
                                           std::uint32_t properties) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success =
        clip_viewport_set_brush_eraser_creation_style_ex(
            runtime, viewport, &style, properties, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult setTextStyle(ClipRuntime runtime, ClipViewport viewport, const ClipTextStyle& style,
                            std::uint32_t properties,
                            const std::vector<ClipTextLayoutOverride>& layouts) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success =
        clip_viewport_patch_text_style_ex(runtime, viewport, &style, properties,
                                          layouts.empty() ? nullptr : layouts.data(),
                                          static_cast<std::uint32_t>(layouts.size()),
                                          result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult setTextCreationStyle(ClipRuntime runtime, ClipViewport viewport,
                                    const ClipTextStyle& style, std::uint32_t properties) {
    MutationResult result;
    if (!hasViewport(runtime, viewport))
        return result;
    result.success =
        clip_viewport_set_text_creation_style_ex(runtime, viewport, &style, properties,
                                                 result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult setSerialNumberStylePatch(ClipRuntime runtime, ClipViewport viewport,
                                         const ClipSerialNumberStyle& style,
                                         std::uint32_t properties) {
    MutationResult result;
    if (!hasViewport(runtime, viewport))
        return result;
    result.success =
        clip_viewport_set_serial_number_style_patch_ex(
            runtime, viewport, &style, properties, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

MutationResult setSerialNumberStyle(ClipRuntime runtime, ClipViewport viewport,
                                    const ClipSerialNumberStyle& style) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success = clip_viewport_set_serial_number_style_ex(
                         runtime, viewport, &style, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

PairedMutationResult setSnapConfig(ClipRuntime runtime, ClipViewport viewport,
                                   ClipSnapConfig config) {
    PairedMutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }

    if (!setSnapConfigRaw(runtime, viewport, &config, result.firstChangedViewports.outParam())) {
        return result;
    }

    if (config.enabled == 0) {
        result.success = true;
        return result;
    }

    ClipGridConfig gridConfig{};
    if (!getGridConfigRaw(runtime, viewport, &gridConfig)) {
        return result;
    }
    if (gridConfig.enabled != 0) {
        gridConfig.enabled = 0;
        if (!setGridConfigRaw(runtime, viewport, &gridConfig,
                              result.secondChangedViewports.outParam())) {
            return result;
        }
    }

    result.success = true;
    return result;
}

MutationResult setSnapGuideTargets(ClipRuntime runtime, ClipViewport viewport,
                                   const double* verticalXs, size_t verticalCount,
                                   const double* horizontalYs, size_t horizontalCount) {
    MutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }
    result.success = clip_viewport_set_snap_guide_targets_ex(
                         runtime, viewport, verticalXs, verticalCount, horizontalYs,
                         horizontalCount, result.changedViewports.outParam()) == CLIP_OK;
    return result;
}

PairedMutationResult setGridConfig(ClipRuntime runtime, ClipViewport viewport,
                                   ClipGridConfig config) {
    PairedMutationResult result;
    if (!hasViewport(runtime, viewport)) {
        return result;
    }

    if (!setGridConfigRaw(runtime, viewport, &config, result.firstChangedViewports.outParam())) {
        return result;
    }

    if (config.enabled == 0) {
        result.success = true;
        return result;
    }

    ClipSnapConfig snapConfig{};
    if (!getSnapConfigRaw(runtime, viewport, &snapConfig)) {
        return result;
    }
    if (snapConfig.enabled != 0) {
        snapConfig.enabled = 0;
        if (!setSnapConfigRaw(runtime, viewport, &snapConfig,
                              result.secondChangedViewports.outParam())) {
            return result;
        }
    }

    result.success = true;
    return result;
}

} // namespace clip_canvas_commands
