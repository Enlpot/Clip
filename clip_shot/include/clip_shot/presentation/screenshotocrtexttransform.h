#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTOCRTEXTTRANSFORM_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTOCRTEXTTRANSFORM_H

#include <QString>

class ScreenshotOcrPresentation;

namespace clip_shot::presentation {

QString originalOcrText(const ScreenshotOcrPresentation& presentation);
QString smartOcrText(const ScreenshotOcrPresentation& presentation, bool selectedOnly = false);
QString removeOcrLineBreaks(const QString& text);
QString convertOcrPunctuation(const QString& text, bool fullWidth);
QString applyOcrTextTransforms(const QString& text, const QString& formatting,
                               const QString& punctuation);
QString applyOcrTextTransforms(const ScreenshotOcrPresentation& presentation,
                               const QString& formatting, const QString& punctuation);

} // namespace clip_shot::presentation

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTOCRTEXTTRANSFORM_H
