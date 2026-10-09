#ifndef CLIP_SHOT_PRESENTATION_STYLES_BUTTONBORDER_H
#define CLIP_SHOT_PRESENTATION_STYLES_BUTTONBORDER_H

#include <QColor>
#include <QSize>

class QPainter;

namespace clip_shot::presentation::styles {
enum class BorderPattern {
    Solid,
    Dashed,
};

struct ButtonBorderSpec {
    QColor color;
    int width = 1;
    int radius = 0;
    BorderPattern pattern = BorderPattern::Solid;
};

void drawButtonBorder(QPainter* painter, const QSize& logicalSize, const ButtonBorderSpec& spec);
} // namespace clip_shot::presentation::styles

#endif // CLIP_SHOT_PRESENTATION_STYLES_BUTTONBORDER_H
