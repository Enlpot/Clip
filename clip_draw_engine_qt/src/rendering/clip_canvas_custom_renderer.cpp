#include "clip_draw_engine_qt/clip_canvas_custom_renderer.h"

std::uint64_t ClipCanvasCustomRenderer::contentRevision() const {
    return 0;
}

std::uint64_t ClipCanvasCustomRenderer::originalBackgroundRevision() const {
    return contentRevision();
}

void ClipCanvasCustomRenderer::clearRenderState() {}

std::optional<ClipCanvasFilterRenderReference>
ClipCanvasCustomRenderer::filterRenderReference() const {
    return std::nullopt;
}

void ClipCanvasCustomRenderer::renderBeforeCanvas(QPainter& painter,
                                                  const ClipCanvasRenderContext& context) {
    Q_UNUSED(painter);
    Q_UNUSED(context);
}

void ClipCanvasCustomRenderer::renderAfterCanvas(QPainter& painter,
                                                 const ClipCanvasRenderContext& context) {
    Q_UNUSED(painter);
    Q_UNUSED(context);
}

void ClipCanvasCustomRenderer::renderOriginalBackground(QPainter& painter,
                                                        const ClipCanvasRenderContext& context) {
    renderBeforeCanvas(painter, context);
}
