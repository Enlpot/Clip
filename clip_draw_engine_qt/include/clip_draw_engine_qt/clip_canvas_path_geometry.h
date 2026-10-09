#pragma once

#include <QPainterPath>
#include <QPointF>
#include <QVector>

// Stateless access to the engine's free-draw curve algorithm.
QPainterPath clipCanvasCatmullRomPath(const QVector<QPointF>& vertices, bool closed);

struct ClipStrokeFilter;

// Queues pointer events locally; crosses FFI once per preview batch, never resends history.
class ClipCanvasStrokeFilter {
  public:
    ClipCanvasStrokeFilter() = default;
    ~ClipCanvasStrokeFilter();
    ClipCanvasStrokeFilter(const ClipCanvasStrokeFilter&) = delete;
    ClipCanvasStrokeFilter& operator=(const ClipCanvasStrokeFilter&) = delete;

    void reset(const QPointF& start, qreal scale = 1.0);
    void append(const QPointF& point);
    QVector<QPointF> takePoints(bool finish = false);

  private:
    ClipStrokeFilter* m_filter = nullptr;
    QVector<QPointF> m_pending;
};
