#ifndef CLIP_SHOT_PRESENTATION_PINSNAPGUIDELAYER_H
#define CLIP_SHOT_PRESENTATION_PINSNAPGUIDELAYER_H

#include "clip_shot/presentation/pinsnapengine.h"

#include <QVector>
#include <QWidget>

class QScreen;

namespace clip_shot::presentation {

// Snap guide layer: borderless topmost translucent window that paints the
// alignment guides produced by a snap decision. Guide coordinates are in
// physical pixels (virtual screen origin); they are converted to window
// logical coordinates using the DPI of the screen the layer sits on.
class PinsnapGuideLayer final : public QWidget {
    Q_OBJECT

  public:
    explicit PinsnapGuideLayer(QWidget* parent = nullptr);

    // Show guides on the given screen; an empty list is equivalent to clearGuides.
    void showGuides(const QVector<PinsnapGuide>& guides, QScreen* screen);
    void clearGuides();

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    QVector<PinsnapGuide> m_guides;
    QScreen* m_screen = nullptr;
};

} // namespace clip_shot::presentation

#endif // CLIP_SHOT_PRESENTATION_PINSNAPGUIDELAYER_H
