#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTRECOGNITIONIMAGE_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTRECOGNITIONIMAGE_H

#include "clip_shot/presentation/screenshotocrpresentation.h"
#include "clip_shot/presentation/screenshotresultcompositor.h"
#include <QFont>
#include <QImage>
#include <functional>

// Value snapshot: images and line containers use Qt copy-on-write; no session or widget references.
struct ScreenshotRecognitionImageSnapshot final {
    QImage image;
    QRectF canvasRect;
    QImage filteredImage;
    QRectF filteredCanvasRect;
    QVector<ScreenshotOcrLine> lines;
    QFont font;
    QColor textColor;
    ScreenshotResultStyle resultStyle;
    qreal outputOpacity = 1.0;
    QPainterPath bakedSelectionPath;
};

[[nodiscard]] QImage
renderScreenshotRecognitionImage(const ScreenshotRecognitionImageSnapshot& snapshot,
                                 const std::function<bool()>& cancelled = {});

#endif
