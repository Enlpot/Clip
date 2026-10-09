#include "clip_draw_engine_qt/clip_canvas_region_filter.h"

#include "clip_canvas_filter_render.h"

#include <cmath>
#include <cstdint>

namespace {
clip_canvas_filter_render::Parameters
internalParameters(const ClipCanvasRegionFilterParameters& parameters) {
    clip_canvas_filter_render::Parameters internal;
    internal.type = static_cast<std::uint32_t>(parameters.type);
    internal.strength = parameters.strength;
    internal.logicalSigma = parameters.logicalSigma;
    internal.logicalSamplingRadius = parameters.logicalSamplingRadius;
    internal.devicePixelRatio = parameters.devicePixelRatio;
    internal.gridOriginInImage = parameters.gridOriginInImage;
    return internal;
}
} // namespace

struct ClipCanvasRegionFilterScratch::Impl {
    explicit Impl(std::size_t retainedByteLimit) : workspace(retainedByteLimit) {}

    clip_canvas_filter_render::RenderWorkspace workspace;
};

struct ClipCanvasRegionFilterScratchAccess {
    static clip_canvas_filter_render::RenderWorkspace&
    workspace(ClipCanvasRegionFilterScratch& scratch) {
        return scratch.m_impl->workspace;
    }
};

ClipCanvasRegionFilterScratch::ClipCanvasRegionFilterScratch(std::size_t retainedByteLimit)
    : m_impl(std::make_unique<Impl>(retainedByteLimit)) {}

ClipCanvasRegionFilterScratch::~ClipCanvasRegionFilterScratch() = default;

void ClipCanvasRegionFilterScratch::finishFrame() {
    m_impl->workspace.finishFrame(false);
}

std::size_t ClipCanvasRegionFilterScratch::retainedBytes() const {
    return m_impl->workspace.retainedBytes();
}

int clipCanvasRegionFilterSupportPixels(const ClipCanvasRegionFilterParameters& parameters) {
    return clip_canvas_filter_render::samplingRadiusPixels(internalParameters(parameters));
}

bool applyClipCanvasRegionFilter(const QImage& source, QImage& destination,
                                 const QRegion& destinationPixels,
                                 const ClipCanvasRegionFilterParameters& parameters,
                                 ClipCanvasRegionFilterScratch* scratch, bool singleThreaded) {
    if (parameters.type == ClipCanvasFilterType::SmartErase ||
        !std::isfinite(parameters.strength) || !std::isfinite(parameters.logicalSigma) ||
        !std::isfinite(parameters.logicalSamplingRadius) ||
        !std::isfinite(parameters.devicePixelRatio) || parameters.devicePixelRatio <= 0.0) {
        return false;
    }
    clip_canvas_filter_render::RenderWorkspace* workspace =
        scratch != nullptr ? &ClipCanvasRegionFilterScratchAccess::workspace(*scratch) : nullptr;
    clip_canvas_filter_render::ExecutionOptions options;
    options.singleThreaded = singleThreaded;
    return clip_canvas_filter_render::applyRegion(
        source, destination, destinationPixels, internalParameters(parameters), workspace, options);
}
