#pragma once

#include <QFont>
#include <QPointF>
#include <QString>

#include "clip_canvas_display_item.h"
#include "clip_draw_engine.h"

namespace clip_canvas_text {

double defaultTextFontSize();
double minimumTextFontSize();
double resolvedTextFontSize(double fontSize);
QString textFromSceneItem(const ClipSceneDisplayItem& item);
QString textFromElementInfo(const ClipTextElementInfo& info);
QString fontFamilyFromSceneItem(const ClipSceneDisplayItem& item);
void copyTextToSceneItem(ClipCanvasSceneItem& item, const QString& text);
void applyTextStyleToSceneItem(ClipCanvasSceneItem& item, const ClipTextStyle& style);
ClipTextStyle textStyleFromSceneItem(const ClipSceneDisplayItem& item);
ClipTextStyle patchedTextStyle(ClipTextStyle current, const ClipTextStyle& requested,
                               std::uint32_t properties);

ClipTextElementInfo newTextInfoAt(const QPointF& canvasPoint, const QFont& baseFont,
                                  const ClipTextStyle& style);
ClipCanvasSceneItem defaultPreviewItem(const ClipTextElementInfo& info);
void updatePreviewFromEditorText(ClipCanvasSceneItem& item, const QString& text, bool autoResize,
                                 const QFont& baseFont);

} // namespace clip_canvas_text
