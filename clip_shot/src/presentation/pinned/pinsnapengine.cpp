#include "clip_shot/presentation/pinsnapengine.h"

#include <algorithm>
#include <cmath>

namespace clip_shot::presentation {
namespace {

constexpr double kNearlyEqualTolerance = 0.5;

bool nearlyEqual(double p, double q) {
    return std::abs(p - q) <= kNearlyEqualTolerance;
}

// Orthogonal projection gap: > 0 means separated, <= 0 means overlapping.
double projectedGap(double min1, double max1, double min2, double max2) {
    return std::max(min1, min2) - std::min(max1, max2);
}

// Vertical alignment snap: top / bottom / center, returns displacement when
// within tolerance, otherwise 0.
double alignVertical(const QRect& a, const QRect& b, double tolerance) {
    const double candidates[] = {
        static_cast<double>(b.top()) - a.top(),
        static_cast<double>(b.bottom()) - a.bottom(),
        (b.top() + b.height() / 2.0) - (a.top() + a.height() / 2.0),
    };
    for (double d : candidates) {
        if (std::abs(d) <= tolerance) {
            return d;
        }
    }
    return 0.0;
}

// Horizontal alignment snap: left / right / center.
double alignHorizontal(const QRect& a, const QRect& b, double tolerance) {
    const double candidates[] = {
        static_cast<double>(b.left()) - a.left(),
        static_cast<double>(b.right()) - a.right(),
        (b.left() + b.width() / 2.0) - (a.left() + a.width() / 2.0),
    };
    for (double d : candidates) {
        if (std::abs(d) <= tolerance) {
            return d;
        }
    }
    return 0.0;
}

// Pick the candidate with the smaller absolute value among two displacements
// when both are within tolerance; otherwise return d1 (caller filters again).
double pickSmaller(double d1, double d2, double tolerance, bool* first) {
    const bool in1 = std::abs(d1) <= tolerance;
    const bool in2 = std::abs(d2) <= tolerance;
    if (in1 && (!in2 || std::abs(d1) <= std::abs(d2))) {
        *first = true;
        return d1;
    }
    if (in2) {
        *first = false;
        return d2;
    }
    *first = true;
    return d1;
}

bool betterCandidate(const PinsnapResult& a, const PinsnapResult& b) {
    if (a.cost != b.cost) {
        return a.cost < b.cost;
    }
    return a.gap < b.gap;
}

void finalizeResult(PinsnapResult& result, const QRect& moving, const QRect& target) {
    result.snapped = moving.translated(result.dx, result.dy);
    result.cost = std::abs(result.dx) + std::abs(result.dy);

    const QRect& snapped = result.snapped;
    const auto addGuide = [&result](bool vertical, int position, int start, int end) {
        PinsnapGuide guide;
        guide.vertical = vertical;
        guide.position = position;
        guide.startPos = start;
        guide.endPos = end;
        result.guides.append(guide);
    };

    // Primary-axis guide: the contact edge.
    if (std::abs(result.dx) > 0 && std::abs(result.gap) <= std::abs(result.dx)) {
        const int x = snapped.right() <= target.left() ? snapped.right() : snapped.left();
        addGuide(true, x, std::min(snapped.top(), target.top()),
                 std::max(snapped.bottom(), target.bottom()));
    } else if (std::abs(result.dy) > 0) {
        const int y = snapped.bottom() <= target.top() ? snapped.bottom() : snapped.top();
        addGuide(false, y, std::min(snapped.left(), target.left()),
                 std::max(snapped.right(), target.right()));
    }

    // Secondary-axis guide: edge / center alignment.
    if (std::abs(result.dx) > 0 && std::abs(result.dy) <= 1) {
        const int y = nearlyEqual(snapped.top(), target.top())
                          ? snapped.top()
                          : nearlyEqual(snapped.bottom(), target.bottom())
                                ? snapped.bottom()
                                : snapped.top() + snapped.height() / 2;
        addGuide(false, y, std::min(snapped.left(), target.left()),
                 std::max(snapped.right(), target.right()));
    } else if (std::abs(result.dy) > 0 && std::abs(result.dx) <= 1) {
        const int x = nearlyEqual(snapped.left(), target.left())
                          ? snapped.left()
                          : nearlyEqual(snapped.right(), target.right())
                                ? snapped.right()
                                : snapped.left() + snapped.width() / 2;
        addGuide(true, x, std::min(snapped.top(), target.top()),
                 std::max(snapped.bottom(), target.bottom()));
    }
}

// Contact snap, single-target evaluation: horizontal contact (A left/right of B)
// plus vertical contact (A above/below B).
std::optional<PinsnapResult> evaluateContact(const QRect& a, const QRect& b,
                                             const QString& targetId,
                                             const PinsnapOptions& opt) {
    const double aLeft = a.left(), aTop = a.top(), aRight = a.right(), aBottom = a.bottom();
    const double bLeft = b.left(), bTop = b.top(), bRight = b.right(), bBottom = b.bottom();

    // Horizontal contact: A left of B / right of B.
    std::optional<PinsnapResult> horizontal;
    {
        const double gapLeft = bLeft - aRight; // A left of B: A moves right by gapLeft
        const double gapRight = aLeft - bRight; // A right of B: A moves left by gapRight
        const double vOverlap = std::min(aBottom, bBottom) - std::max(aTop, bTop);
        if (vOverlap >= 0) {
            if (gapLeft >= 0 && gapLeft <= opt.snapDistance) {
                horizontal = PinsnapResult();
                horizontal->dx = qRound(gapLeft);
                horizontal->dy = qRound(alignVertical(a, b, opt.alignTolerance));
                horizontal->targetId = targetId;
                horizontal->gap = qRound(gapLeft);
            } else if (gapRight >= 0 && gapRight <= opt.snapDistance) {
                horizontal = PinsnapResult();
                horizontal->dx = -qRound(gapRight);
                horizontal->dy = qRound(alignVertical(a, b, opt.alignTolerance));
                horizontal->targetId = targetId;
                horizontal->gap = qRound(gapRight);
            }
        }
    }

    // Vertical contact: A above B / below B.
    std::optional<PinsnapResult> vertical;
    {
        const double gapTop = bTop - aBottom; // A above B: A moves down by gapTop
        const double gapBottom = aTop - bBottom; // A below B: A moves up by gapBottom
        const double hOverlap = std::min(aRight, bRight) - std::max(aLeft, bLeft);
        if (hOverlap >= 0) {
            if (gapTop >= 0 && gapTop <= opt.snapDistance) {
                vertical = PinsnapResult();
                vertical->dx = qRound(alignHorizontal(a, b, opt.alignTolerance));
                vertical->dy = qRound(gapTop);
                vertical->targetId = targetId;
                vertical->gap = qRound(gapTop);
            } else if (gapBottom >= 0 && gapBottom <= opt.snapDistance) {
                vertical = PinsnapResult();
                vertical->dx = qRound(alignHorizontal(a, b, opt.alignTolerance));
                vertical->dy = -qRound(gapBottom);
                vertical->targetId = targetId;
                vertical->gap = qRound(gapBottom);
            }
        }
    }

    std::optional<PinsnapResult> selected;
    if (horizontal.has_value() && vertical.has_value()) {
        selected = betterCandidate(*horizontal, *vertical) ? horizontal : vertical;
    } else if (horizontal.has_value()) {
        selected = horizontal;
    } else if (vertical.has_value()) {
        selected = vertical;
    }
    if (!selected.has_value()) {
        return std::nullopt;
    }
    finalizeResult(*selected, a, b);
    return selected;
}

// Edge-align single-target evaluation.
std::optional<PinsnapResult> evaluateAlign(const QRect& a, const QRect& b,
                                           const QString& targetId,
                                           const PinsnapOptions& opt) {
    const double hGap = projectedGap(a.left(), a.right(), b.left(), b.right());
    const double vGap = projectedGap(a.top(), a.bottom(), b.top(), b.bottom());

    double dx = 0.0;
    double dy = 0.0;
    bool hasDx = false;
    bool hasDy = false;
    PinsnapResult result;
    result.targetId = targetId;

    // Vertical edge alignment (left-to-left / right-to-right): requires close vertical projection.
    if (vGap <= opt.snapDistance) {
        const double dLeft = b.left() - a.left();
        const double dRight = b.right() - a.right();
        bool toLeft = true;
        const double d = pickSmaller(dLeft, dRight, opt.snapDistance, &toLeft);
        if (std::abs(d) <= opt.snapDistance) {
            dx = d;
            hasDx = true;
            PinsnapGuide guide;
            guide.vertical = true;
            guide.position = toLeft ? b.left() : b.right();
            guide.startPos = std::min(a.top(), b.top());
            guide.endPos = std::max(a.bottom(), b.bottom());
            result.guides.append(guide);
        }
    }

    // Horizontal edge alignment (top-to-top / bottom-to-bottom): requires close horizontal projection.
    if (hGap <= opt.snapDistance) {
        const double dTop = b.top() - a.top();
        const double dBottom = b.bottom() - a.bottom();
        bool toTop = true;
        const double d = pickSmaller(dTop, dBottom, opt.snapDistance, &toTop);
        if (std::abs(d) <= opt.snapDistance) {
            dy = d;
            hasDy = true;
            PinsnapGuide guide;
            guide.vertical = false;
            guide.position = toTop ? b.top() : b.bottom();
            guide.startPos = std::min(a.left(), b.left());
            guide.endPos = std::max(a.right(), b.right());
            result.guides.append(guide);
        }
    }

    if (!hasDx && !hasDy) {
        return std::nullopt;
    }

    result.dx = qRound(dx);
    result.dy = qRound(dy);
    finalizeResult(result, a, b);
    result.gap = result.cost;
    return result;
}

} // namespace

std::optional<PinsnapResult> PinsnapEngine::resolveContact(
    const QRect& moving, const QVector<PinsnapCandidate>& candidates,
    const PinsnapOptions& opt, const QString& preferredTargetId) {
    std::optional<PinsnapResult> best;
    for (const PinsnapCandidate& candidate : candidates) {
        if (!preferredTargetId.isEmpty() && candidate.id != preferredTargetId) {
            continue;
        }
        std::optional<PinsnapResult> candidateResult =
            evaluateContact(moving, candidate.bounds, candidate.id, opt);
        if (!candidateResult.has_value()) {
            continue;
        }
        if (!best.has_value() || betterCandidate(*candidateResult, *best)) {
            best = candidateResult;
        }
    }
    if (!best.has_value()) {
        return std::nullopt;
    }
    best->gap = std::abs(best->gap);
    return best;
}

std::optional<PinsnapResult> PinsnapEngine::resolveAlign(
    const QRect& moving, const QVector<PinsnapCandidate>& candidates,
    const PinsnapOptions& opt, const QString& preferredTargetId) {
    std::optional<PinsnapResult> best;
    for (const PinsnapCandidate& candidate : candidates) {
        if (!preferredTargetId.isEmpty() && candidate.id != preferredTargetId) {
            continue;
        }
        std::optional<PinsnapResult> candidateResult =
            evaluateAlign(moving, candidate.bounds, candidate.id, opt);
        if (!candidateResult.has_value()) {
            continue;
        }
        if (!best.has_value() || betterCandidate(*candidateResult, *best)) {
            best = candidateResult;
        }
    }
    return best;
}

std::optional<PinsnapResult> PinsnapEngine::resolve(
    const QRect& moving, const QVector<PinsnapCandidate>& candidates,
    const PinsnapOptions& opt, const QString& preferredTargetId) {
    std::optional<PinsnapResult> contact =
        resolveContact(moving, candidates, opt, preferredTargetId);
    if (contact.has_value()) {
        return contact;
    }
    return resolveAlign(moving, candidates, opt, preferredTargetId);
}

} // namespace clip_shot::presentation
