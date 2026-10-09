#pragma once

#include <QRect>
#include <QRegion>

#include <cstdint>
#include <array>
#include <unordered_map>
#include <vector>

#include "clip_canvas_display_item.h"
#include "clip_canvas_render_plan.h"
#include "clip_draw_engine.h"
#include "clip_draw_engine_qt/clip_canvas_types.h"

struct SceneDisplayInfo {
    std::uint32_t item_count = 0;
    std::uint32_t dirty_rect_count = 0;
    double surface_width = 0.0;
    double surface_height = 0.0;
    double camera_center_x = 0.0;
    double camera_center_y = 0.0;
    double camera_zoom = 1.0;
    ClipColorRgba8 clear_color{};
};

struct WatermarkDisplayInfo {
    double surface_width = 0.0;
    double surface_height = 0.0;
    double camera_center_x = 0.0;
    double camera_center_y = 0.0;
    double camera_zoom = 1.0;
    ClipColorRgba8 watermark_color{};
    std::array<char, CLIP_WATERMARK_TEXT_CAPACITY> watermark_text{};
    std::uint16_t watermark_text_len = 0;
    double watermark_font_size = 16.0;
    std::array<char, CLIP_WATERMARK_FONT_FAMILY_CAPACITY> watermark_font_family{};
    std::uint16_t watermark_font_family_len = 0;
    double watermark_angle = 30.0;
    double watermark_gap = 56.0;
    double watermark_opacity = 0.16;
};

struct SpotlightDisplayInfo {
    ClipColorRgba8 color{0, 0, 0, 255};
    double opacity = 0.64;
    bool active = false;
};

struct OverlayDisplayInfo {
    std::uint32_t item_count = 0;
    std::uint32_t dirty_rect_count = 0;
    double surface_width = 0.0;
    double surface_height = 0.0;
    double camera_center_x = 0.0;
    double camera_center_y = 0.0;
    double camera_zoom = 1.0;
};

struct AppliedPenFilterGeometryDelta {
    ClipElementId elementId{};
    std::uint64_t expectedRevision = 0;
    std::uint64_t resultingRevision = 0;
    QRectF oldChangedCanvasBounds;
    QRectF newChangedCanvasBounds;
    bool fullReset = false;
    bool elementRemoved = false;
};

class ClipCanvasDisplayCache {
  public:
    ClipCanvasDisplayCache();

    void reset(const ClipColorRgba8& clearColor);
    // Releases the derived execution plan while preserving the current scene snapshot.
    void clearRenderState();
    void setClearColor(const ClipColorRgba8& clearColor);
    bool sync(ClipRuntime runtime, ClipViewport viewport);

    const SceneDisplayInfo& sceneInfo() const;
    const std::vector<ClipSceneRenderRun>& renderPlan() const {
        return m_renderPlan;
    }
    const clip_canvas_renderer::SceneExecutionPlan& executionPlan(double dpr) const;
    std::size_t executionPlanBuildCount() const {
        return m_executionPlan.buildCount;
    }
    // Owned container capacity, excluding payloads inside display items and the execution plan.
    std::size_t retainedStorageBytes() const;
    const WatermarkDisplayInfo& watermarkInfo() const;
    const SpotlightDisplayInfo& spotlightInfo() const;
    const OverlayDisplayInfo& overlayInfo() const;
    const ClipPatchCursor& patchCursor() const;

    const ClipCanvasSceneItem* sceneItems() const;
    const ClipCanvasOverlayItem* overlayItems() const;
    const ClipSpotlightCutout* spotlightCutouts() const;
    std::uint32_t sceneItemCount() const;
    std::uint32_t overlayItemCount() const;
    std::uint32_t spotlightCutoutCount() const;
    std::uint32_t lastPenFilterGeometryPointCount() const;
    const std::vector<AppliedPenFilterGeometryDelta>& appliedPenFilterGeometryDeltas() const;
    const std::vector<std::uint32_t>& filterIndices() const;

    const ClipDirtyRect* sceneDirtyRects() const;
    const ClipDirtyRect* overlayDirtyRects() const;
    const ClipDirtyRect* decorationDirtyRects() const;
    std::uint32_t sceneDirtyRectCount() const;
    std::uint32_t overlayDirtyRectCount() const;
    std::uint32_t decorationDirtyRectCount() const;
    void sceneCandidateIndices(const QRegion& exposedRegion,
                               std::vector<std::uint32_t>* outIndices) const;

  private:
    void refreshViews();
    void applyPatchInfo(const ClipPatchInfo& patchInfo);
    void rebuildViewBoundsAndSpatialIndex();
    void rebuildSceneSpatialIndex();
    void updateSceneSpatialItem(std::uint32_t index);
    void removeSceneSpatialItem(std::uint32_t index);
    void rebuildFilterIndices();
    void updateFilterIndices(const std::vector<std::uint32_t>& changedIndices);

    std::vector<ClipSceneRenderRun> m_renderPlan;
    mutable clip_canvas_renderer::SceneExecutionPlan m_executionPlan;
    ClipPatchCursor m_patchCursor{};
    SceneDisplayInfo m_sceneDisplayInfo{};
    WatermarkDisplayInfo m_watermarkDisplayInfo{};
    SpotlightDisplayInfo m_spotlightDisplayInfo{};
    OverlayDisplayInfo m_overlayDisplayInfo{};
    std::vector<ClipCanvasSceneItem> m_sceneStorage;
    std::vector<ClipCanvasOverlayItem> m_overlayStorage;
    std::vector<ClipSpotlightCutout> m_spotlightStorage;
    std::vector<ClipDirtyRect> m_sceneDirtyStorage;
    std::vector<ClipDirtyRect> m_overlayDirtyStorage;
    std::vector<ClipDirtyRect> m_decorationDirtyStorage;
    const ClipCanvasSceneItem* m_sceneItems = nullptr;
    const ClipCanvasOverlayItem* m_overlayItems = nullptr;
    const ClipSpotlightCutout* m_spotlightCutouts = nullptr;
    const ClipDirtyRect* m_sceneDirtyRects = nullptr;
    const ClipDirtyRect* m_overlayDirtyRects = nullptr;
    const ClipDirtyRect* m_decorationDirtyRects = nullptr;
    std::uint32_t m_sceneItemCount = 0;
    std::uint32_t m_overlayItemCount = 0;
    std::uint32_t m_spotlightCutoutCount = 0;
    std::uint32_t m_lastPenFilterGeometryPointCount = 0;
    std::vector<AppliedPenFilterGeometryDelta> m_appliedPenFilterGeometryDeltas;
    std::uint32_t m_sceneDirtyRectCount = 0;
    std::uint32_t m_overlayDirtyRectCount = 0;
    std::uint32_t m_decorationDirtyRectCount = 0;
    std::unordered_map<std::int64_t, std::vector<std::uint32_t>> m_sceneSpatialCells;
    std::vector<std::vector<std::int64_t>> m_sceneItemSpatialCells;
    std::vector<std::uint32_t> m_sceneGlobalItems;
    std::vector<std::uint32_t> m_filterIndices;
    mutable std::vector<std::uint32_t> m_sceneQueryMarks;
    mutable std::uint32_t m_sceneQueryStamp = 0;
};

namespace clip_canvas_display {

QRect dirtyRectToUpdateRect(const ClipDirtyRect& dirtyRect, int paddingPx = 1);

QRegion dirtyRectsToRegion(const ClipDirtyRect* dirtyRects, std::uint32_t dirtyRectCount,
                           const QRect& clip, int paddingPx = 1);

QRegion dirtyVisualizationRegion(const ClipCanvasDisplayCache& cache, const QRect& clip,
                                 int paddingPx = 1);

} // namespace clip_canvas_display
