#pragma once

#include "clip_draw_engine.h"

#include <QColor>

#include <cstddef>

class QPainter;
class QPainterPath;

namespace clip_canvas_fill_render {

void drawStyledFill(QPainter& painter, const QPainterPath& path, const ClipColorRgba8& fill,
                    ClipFillStyle style, double referenceStrokeWidth, double coordinateScale = 1.0);

void drawTextBackgroundFill(QPainter& painter, const QPainterPath& path, const ClipColorRgba8& fill,
                            ClipFillStyle style, double fontSize, double coordinateScale = 1.0);

std::size_t hatchTextureCacheEntryCountForCurrentThread();
void resetHatchTextureCacheForCurrentThread();

} // namespace clip_canvas_fill_render
