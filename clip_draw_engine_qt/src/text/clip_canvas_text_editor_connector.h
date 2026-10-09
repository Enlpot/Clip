#pragma once

#include "clip_canvas_display_cache.h"
#include "clip_draw_engine.h"

#include <QRegion>

#include <cstdint>

namespace clip_canvas_text_editor_connector {

bool itemBindsPreview(const ClipSceneDisplayItem& item, const ClipSceneDisplayItem* preview);
bool connectorBelongsToPreview(const ClipSceneDisplayItem& connector,
                               const ClipCanvasSceneItem* sceneItems, std::uint32_t sceneItemCount,
                               const ClipSceneDisplayItem* preview);
bool connectorItemForPreview(const ClipSceneDisplayItem& serial,
                             const ClipSceneDisplayItem& preview,
                             ClipCanvasSceneItem* outConnector);
QRegion connectorRegion(const SceneDisplayInfo& displayInfo, const ClipCanvasSceneItem* sceneItems,
                        std::uint32_t sceneItemCount, const ClipSceneDisplayItem* preview);

} // namespace clip_canvas_text_editor_connector
