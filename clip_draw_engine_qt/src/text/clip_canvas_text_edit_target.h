#pragma once

#include "clip_draw_engine.h"

#include <QPointF>

#include <optional>

class QFont;

namespace clip_canvas_text_edit_target {

std::optional<ClipTextElementInfo>
resolveTextEditTarget(ClipRuntime runtime, ClipViewport viewport, const QPointF& canvasPoint,
                      const QFont& baseFont, const ClipTextStyle& newTextStyle, bool allowCreate);

std::optional<ClipTextElementInfo> resolveSelectedTextEditTarget(ClipRuntime runtime,
                                                                 ClipViewport viewport,
                                                                 const QPointF& canvasPoint,
                                                                 bool requireSerialBoundText);

} // namespace clip_canvas_text_edit_target
