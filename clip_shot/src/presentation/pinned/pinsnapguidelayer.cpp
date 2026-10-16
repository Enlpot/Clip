#include "clip_shot/presentation/pinsnapguidelayer.h"

#include <QPainter>
#include <QPainterPath>
#include <QScreen>

#include <cmath>

namespace clip_shot::presentation {

PinsnapGuideLayer::PinsnapGuideLayer(QWidget* parent) : QWidget(parent) {
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                   Qt::WindowDoesNotAcceptFocus | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_NoSystemBackground);
    hide();
}

void PinsnapGuideLayer::showGuides(const QVector<PinsnapGuide>& guides, QScreen* screen) {
    m_guides = guides;
    m_screen = screen;
    if (m_guides.isEmpty() || m_screen == nullptr) {
        clearGuides();
        return;
    }
    // Paint guides only within the screen where the snap happened
    // (cross-screen snapping is out of scope for v1).
    setGeometry(m_screen->geometry());
    show();
    raise();
    update();
}

void PinsnapGuideLayer::clearGuides() {
    m_guides.clear();
    m_screen = nullptr;
    hide();
}

void PinsnapGuideLayer::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    if (m_guides.isEmpty() || m_screen == nullptr) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    QPen pen(QColor(0x00, 0xE5, 0xC0, 0xCC));
    pen.setWidthF(1.0);
    painter.setPen(pen);

    // Physical px -> window logical coords: the window covers the whole screen
    // logical geometry, so window coords = screen-local logical offset
    // = (physical point - screen physical origin) / DPR.
    const qreal dpr = m_screen->devicePixelRatio();
    if (dpr <= 0.0) {
        return;
    }
    const QPointF screenPhysicalOrigin =
        QPointF(m_screen->geometry().topLeft()) * dpr;

    const auto toWindowPoint = [&](const QPointF& physical) {
        return (physical - screenPhysicalOrigin) / dpr;
    };

    for (const PinsnapGuide& guide : m_guides) {
        if (guide.vertical) {
            const QPointF top = toWindowPoint(QPointF(guide.position, guide.startPos));
            const QPointF bottom = toWindowPoint(QPointF(guide.position, guide.endPos));
            painter.drawLine(top, bottom);
        } else {
            const QPointF left = toWindowPoint(QPointF(guide.startPos, guide.position));
            const QPointF right = toWindowPoint(QPointF(guide.endPos, guide.position));
            painter.drawLine(left, right);
        }
    }
}

} // namespace clip_shot::presentation
