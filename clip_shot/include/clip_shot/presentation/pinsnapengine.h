#ifndef CLIP_SHOT_PRESENTATION_PINSNAPENGINE_H
#define CLIP_SHOT_PRESENTATION_PINSNAPENGINE_H

#include <QPoint>
#include <QRect>
#include <QString>
#include <QVector>

#include <optional>

namespace clip_shot::presentation {

// Snap decision engine - pure logic, no UI dependency, unit-testable.
//
// Rules (geometry semantics ported from PixJoin SnapEngine to C++):
//   1. Contact snap: gap between the dragged pinned window and a candidate
//      on the primary axis within [0, snapDistance] while the orthogonal
//      axes overlap; secondary-axis "edge/center align" deviation within
//      alignTolerance snaps along (not forced). Among multiple candidates
//      pick the smallest total displacement; ties go to the smaller gap.
//   2. Edge align: same-side edges (top-to-top / bottom-to-bottom /
//      left-to-left / right-to-right) plus close orthogonal projection
//      cause a whole-window translation (no grouping).
//
// Coordinates are physical pixels (origin at the virtual screen top-left),
// matching the pinned window native geometry.
struct PinsnapOptions {
    double snapDistance = 10.0;
    double alignTolerance = 5.0;
};

// One alignment guide line (physical pixel coordinates).
// vertical = true means a vertical guide (position is x, startPos/endPos are the y range);
// vertical = false means a horizontal guide (position is y, startPos/endPos are the x range).
struct PinsnapGuide {
    bool vertical = true;
    int position = 0;
    int startPos = 0;
    int endPos = 0;
};

// One candidate snap target.
struct PinsnapCandidate {
    QRect bounds;
    QString id;
};

// The result of one snap decision.
struct PinsnapResult {
    QRect snapped; // snapped bounding box of the dragged pinned window
    int dx = 0;    // horizontal displacement to apply (physical px)
    int dy = 0;    // vertical displacement to apply
    QString targetId;
    QVector<PinsnapGuide> guides;
    int gap = 0; // gap on the primary snap axis
    int cost = 0; // total displacement (sum of abs(dx) + abs(dy))
};

class PinsnapEngine {
  public:
    // Contact snap (includes secondary-axis alignment). When preferredTargetId
    // is non-empty only that target is considered (drag hysteresis).
    static std::optional<PinsnapResult> resolveContact(
        const QRect& moving, const QVector<PinsnapCandidate>& candidates,
        const PinsnapOptions& opt = {}, const QString& preferredTargetId = {});

    // Edge-align magnet (same-side alignment, no grouping). Same preferredTargetId semantics.
    static std::optional<PinsnapResult> resolveAlign(
        const QRect& moving, const QVector<PinsnapCandidate>& candidates,
        const PinsnapOptions& opt = {}, const QString& preferredTargetId = {});

    // Convenience entry: try contact snap first, fall back to edge align.
    static std::optional<PinsnapResult> resolve(
        const QRect& moving, const QVector<PinsnapCandidate>& candidates,
        const PinsnapOptions& opt = {}, const QString& preferredTargetId = {});
};

} // namespace clip_shot::presentation

#endif // CLIP_SHOT_PRESENTATION_PINSNAPENGINE_H
