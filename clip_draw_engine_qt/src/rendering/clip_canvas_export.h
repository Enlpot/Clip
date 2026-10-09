#pragma once

#include "clip_draw_engine.h"
#include "clip_draw_engine_qt/clip_canvas_export_types.h"
#include "clip_draw_engine_qt/clip_canvas_smart_erase.h"

#include <QImage>
#include <QList>
#include <QRectF>
#include <QSize>

#include <cstddef>

namespace clip_canvas_export {

struct RenderDiagnostics {
    std::size_t directSourceFastPathCount = 0;
    std::size_t fullCompositorPathCount = 0;
    std::size_t unsynchronizedFallbackCount = 0;
};

RenderDiagnostics diagnosticsForCurrentThread();
void resetDiagnosticsForCurrentThread();

QImage renderToImage(ClipRuntime runtime, const QRectF& virtualSelectionRect,
                     const QSize& outputSize, const QList<CanvasExportSource>& sources,
                     const ClipCanvasSmartEraseSnapshot& smartErase = {});

} // namespace clip_canvas_export
