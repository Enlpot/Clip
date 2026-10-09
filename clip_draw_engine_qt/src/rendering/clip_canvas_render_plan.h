#pragma once

#include "clip_canvas_display_item.h"
#include "clip_canvas_filter_render.h"
#include <QPainterPath>
#include <cstdint>
#include <vector>

struct SceneDisplayInfo;

namespace clip_canvas_renderer {

struct FilterFrameInfo {
    bool effective = false;
    int logicalSamplingRadius = 0;
    bool axisAlignedRect = false;
    bool devicePixelAlignedRect = false;
    QPainterPath clipPath;
    QRectF logicalBounds;
};

struct EffectGroup {
    std::uint32_t type = 0;
    int blockPixels = 0;
    int samplingRadiusPixels = 0;
    double strength = 0.0;
    clip_canvas_filter_render::GaussianBlurPlan gaussianPlan;
    std::vector<std::uint32_t> indices;
};

struct SourcePass {
    ClipElementId id{};
    std::uint32_t start = 0;
    std::uint32_t end = 0;
    std::vector<EffectGroup> groups;
};

struct SceneExecutionPlan {
    std::vector<SourcePass> passes;
    std::vector<int> passForItem;
    std::vector<int> filterForItem;
    std::vector<FilterFrameInfo> filters;
    std::vector<std::uint32_t> filterIndices;
    std::vector<std::uint32_t> restorationIndices;
    std::vector<int> restorationForItem;
    std::vector<FilterFrameInfo> restorations;
    std::uint64_t revision = 0;
    double dpr = 0;
    std::size_t buildCount = 0;

    const FilterFrameInfo& filter(std::uint32_t index) const {
        return filters[static_cast<std::size_t>(filterForItem[index])];
    }
    const FilterFrameInfo& restoration(std::uint32_t index) const {
        return restorations[static_cast<std::size_t>(restorationForItem[index])];
    }
};

// Used only for direct callers supplying a complete, uncropped ordered scene.
std::vector<ClipSceneRenderRun> buildUnculledRenderPlan(const ClipCanvasSceneItem* items,
                                                        std::uint32_t count,
                                                        const SceneDisplayInfo& info, double dpr);
bool validateRenderPlan(const std::vector<ClipSceneRenderRun>& runs,
                        const ClipCanvasSceneItem* items, std::uint32_t count);
void prepareExecutionPlan(SceneExecutionPlan& plan, const ClipCanvasSceneItem* items,
                          std::uint32_t count, const SceneDisplayInfo& info, double dpr,
                          const std::vector<ClipSceneRenderRun>& runs, std::uint64_t revision);

} // namespace clip_canvas_renderer
