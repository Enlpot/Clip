#include "clip_canvas_text_editor_connector.h"

#include "clip_canvas_element_id.h"
#include "clip_canvas_render_geometry.h"

#include <cstdint>

namespace clip_canvas_text_editor_connector {
namespace {

ClipElementId boundTextElementId(const ClipSceneDisplayItem& item) {
    return item.has_bound_text_element != 0
        ? ClipElementId {
            item.bound_text_element_index,
            item.bound_text_element_generation,
        }
        : ClipElementId {};
}

const ClipSceneDisplayItem* findSerialNumberItem(const ClipCanvasSceneItem* sceneItems,
                                                 std::uint32_t sceneItemCount,
                                                 const ClipElementId& id) {
    if (sceneItems == nullptr || !clip_canvas_element_id::hasElementId(id)) {
        return nullptr;
    }
    for (std::uint32_t index = 0; index < sceneItemCount; ++index) {
        const ClipSceneDisplayItem& item = sceneItems[index];
        if (item.kind == CLIP_SCENE_DISPLAY_ITEM_SERIAL_NUMBER &&
            clip_canvas_element_id::sameElementId(item.element_id, id)) {
            return &item;
        }
    }
    return nullptr;
}

} // namespace

bool itemBindsPreview(const ClipSceneDisplayItem& item, const ClipSceneDisplayItem* preview) {
    if (preview == nullptr || preview->kind != CLIP_SCENE_DISPLAY_ITEM_TEXT ||
        !clip_canvas_element_id::hasElementId(preview->element_id)) {
        return false;
    }
    return clip_canvas_element_id::sameElementId(boundTextElementId(item), preview->element_id);
}

bool connectorBelongsToPreview(const ClipSceneDisplayItem& connector,
                               const ClipCanvasSceneItem* sceneItems, std::uint32_t sceneItemCount,
                               const ClipSceneDisplayItem* preview) {
    if (connector.kind != CLIP_SCENE_DISPLAY_ITEM_SERIAL_NUMBER_CONNECTOR) {
        return false;
    }
    const ClipSceneDisplayItem* serial =
        findSerialNumberItem(sceneItems, sceneItemCount, connector.element_id);
    return serial != nullptr && itemBindsPreview(*serial, preview);
}

bool connectorItemForPreview(const ClipSceneDisplayItem& serial,
                             const ClipSceneDisplayItem& preview,
                             ClipCanvasSceneItem* outConnector) {
    if (outConnector == nullptr || serial.kind != CLIP_SCENE_DISPLAY_ITEM_SERIAL_NUMBER ||
        !itemBindsPreview(serial, &preview)) {
        return false;
    }

    ClipSerialTextConnection connection{};
    if (clip_scene_resolve_serial_text_connection(&serial, &preview, &connection) == 0) {
        return false;
    }

    ClipCanvasSceneItem connector;
    connector.kind = CLIP_SCENE_DISPLAY_ITEM_SERIAL_NUMBER_CONNECTOR;
    connector.element_id = serial.element_id;
    connector.center_x = connection.start_x;
    connector.center_y = connection.start_y;
    connector.width = connection.end_x;
    connector.height = connection.end_y;
    connector.stroke = serial.stroke;
    connector.stroke_width = serial.stroke_width;
    connector.opacity = serial.opacity;
    if (connection.has_baseline != 0) {
        const ClipArrowPoint points[] = {
            {connection.baseline_start_x, connection.baseline_start_y},
            {connection.baseline_end_x, connection.baseline_end_y},
        };
        connector.setArrowPoints(points, 2);
    }
    *outConnector = connector;
    return true;
}

QRegion connectorRegion(const SceneDisplayInfo& displayInfo, const ClipCanvasSceneItem* sceneItems,
                        std::uint32_t sceneItemCount, const ClipSceneDisplayItem* preview) {
    QRegion region;
    if (sceneItems == nullptr || preview == nullptr ||
        preview->kind != CLIP_SCENE_DISPLAY_ITEM_TEXT ||
        !clip_canvas_element_id::hasElementId(preview->element_id)) {
        return region;
    }

    for (std::uint32_t index = 0; index < sceneItemCount; ++index) {
        const ClipSceneDisplayItem& item = sceneItems[index];
        if (connectorBelongsToPreview(item, sceneItems, sceneItemCount, preview)) {
            region += clip_canvas_render_geometry::alignedRectForBounds(
                clip_canvas_render_geometry::sceneItemBounds(displayInfo, item));
        }
        if (item.kind != CLIP_SCENE_DISPLAY_ITEM_SERIAL_NUMBER ||
            !itemBindsPreview(item, preview)) {
            continue;
        }

        ClipCanvasSceneItem connector;
        if (!connectorItemForPreview(item, *preview, &connector)) {
            continue;
        }
        region += clip_canvas_render_geometry::alignedRectForBounds(
            clip_canvas_render_geometry::sceneItemBounds(displayInfo, connector));
    }

    return region;
}

} // namespace clip_canvas_text_editor_connector
