#include "clip_shot/presentation/pinsnapengine.h"

#include <QCoreApplication>

#include <iostream>
#include <stdexcept>

namespace {
using clip_shot::presentation::PinsnapCandidate;
using clip_shot::presentation::PinsnapEngine;
using clip_shot::presentation::PinsnapOptions;
using clip_shot::presentation::PinsnapResult;

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

PinsnapCandidate candidate(const QString& id, const QRect& bounds) {
    PinsnapCandidate result;
    result.id = id;
    result.bounds = bounds;
    return result;
}

const PinsnapOptions kDefaultOptions;

// Horizontal contact: A is 6px left of B with vertical overlap -> snap right by 6.
void testHorizontalContactLeft() {
    const QRect moving(100, 100, 50, 50);
    // All three vertical alignment candidates (top/bottom/center) deviate by
    // 6px, beyond tolerance -> primary axis only.
    const QVector<PinsnapCandidate> candidates{
        candidate(QStringLiteral("b"), QRect(155, 106, 50, 50)),
    };
    const auto result = PinsnapEngine::resolveContact(moving, candidates, kDefaultOptions);
    require(result.has_value(), "a 6px horizontal gap inside snap distance should snap");
    require(result->dx == 6, "A right edge should move to B left edge");
    require(result->dy == 0, "alignment deviation beyond tolerance should not align");
    require(result->targetId == QStringLiteral("b"), "target id should be reported");
    require(result->snapped == QRect(106, 100, 50, 50), "snapped bounds should be translated");
    require(!result->guides.isEmpty(), "contact snap should emit guides");
}

// Horizontal contact: A is right of B (B right edge 4px from A left edge) -> move left by 4.
void testHorizontalContactRight() {
    const QRect moving(300, 100, 50, 50);
    const QVector<PinsnapCandidate> candidates{
        candidate(QStringLiteral("b"), QRect(100, 100, 197, 60)),
    };
    const auto result = PinsnapEngine::resolveContact(moving, candidates, kDefaultOptions);
    require(result.has_value(), "right-side contact within snap distance should snap");
    require(result->dx == -4, "A should move left to touch B right edge");
    require(result->snapped.left() == 296, "snapped left edge should equal B right edge");
}

// Vertical contact: A is 9px above B with horizontal overlap -> move down by 9.
void testVerticalContactTop() {
    const QRect moving(100, 10, 50, 50);
    const QVector<PinsnapCandidate> candidates{
        candidate(QStringLiteral("b"), QRect(80, 68, 90, 60)),
    };
    const auto result = PinsnapEngine::resolveContact(moving, candidates, kDefaultOptions);
    require(result.has_value(), "vertical gap inside snap distance should snap");
    require(result->dy == 9, "A bottom should move to B top");
    require(result->dx == 0, "no horizontal alignment expected");
}

// Threshold boundary: gap == 10 snaps; gap == 11 does not.
void testSnapDistanceBoundary() {
    const QRect moving(100, 100, 50, 50);
    const auto within = PinsnapEngine::resolveContact(
        moving, {candidate(QStringLiteral("b"), QRect(159, 100, 50, 50))}, kDefaultOptions);
    require(within.has_value() && within->dx == 10, "gap equal to snap distance should snap");

    const auto outside = PinsnapEngine::resolveContact(
        moving, {candidate(QStringLiteral("b"), QRect(160, 100, 50, 50))}, kDefaultOptions);
    require(!outside.has_value(), "gap beyond snap distance should not snap");
}

// No orthogonal overlap -> no snap (avoid overlap).
void testNoOverlapRejects() {
    const QRect moving(100, 100, 50, 50);
    const QVector<PinsnapCandidate> candidates{
        candidate(QStringLiteral("b"), QRect(155, 300, 80, 80)),
    };
    const auto result = PinsnapEngine::resolveContact(moving, candidates, kDefaultOptions);
    require(!result.has_value(), "no orthogonal overlap should reject contact snap");
}

// Secondary-axis alignment: vertical overlap + top-edge deviation 3px (<= tolerance) -> snap along.
void testSecondaryAlign() {
    const QRect moving(100, 103, 50, 50);
    const QVector<PinsnapCandidate> candidates{
        candidate(QStringLiteral("b"), QRect(155, 100, 80, 80)),
    };
    const auto result = PinsnapEngine::resolveContact(moving, candidates, kDefaultOptions);
    require(result.has_value(), "contact snap should still occur");
    require(result->dx == 6, "primary axis dx should be 6");
    require(result->dy == -3, "secondary axis should align top edges within tolerance");
}

// Multiple candidates: pick the smallest total displacement.
void testBestCandidate() {
    const QRect moving(100, 100, 50, 50);
    // Far candidate (dx=10) vs near candidate (dx=3).
    const QVector<PinsnapCandidate> candidates{
        candidate(QStringLiteral("far"), QRect(159, 100, 50, 50)),
        candidate(QStringLiteral("near"), QRect(152, 100, 50, 50)),
    };
    const auto result = PinsnapEngine::resolveContact(moving, candidates, kDefaultOptions);
    require(result.has_value(), "at least one candidate should snap");
    require(result->targetId == QStringLiteral("near"), "nearer candidate should win");
    require(result->dx == 3, "nearer candidate displacement should win");
}

// preferredTargetId: only snap against the requested target.
void testPreferredTarget() {
    const QRect moving(100, 100, 50, 50);
    const QVector<PinsnapCandidate> candidates{
        candidate(QStringLiteral("near"), QRect(152, 100, 50, 50)),
        candidate(QStringLiteral("far"), QRect(159, 100, 50, 50)),
    };
    const auto result = PinsnapEngine::resolveContact(
        moving, candidates, kDefaultOptions, QStringLiteral("far"));
    require(result.has_value() && result->targetId == QStringLiteral("far"),
            "preferred target should be honored");
    require(result->dx == 10, "preferred target displacement should apply");
}

// Empty candidates -> no result.
void testEmptyCandidates() {
    const QRect moving(100, 100, 50, 50);
    const auto result = PinsnapEngine::resolve(moving, {}, kDefaultOptions);
    require(!result.has_value(), "empty candidates should not snap");
}

// Edge align: same-side top alignment (horizontal projection overlap + top-edge deviation 3px).
void testAlignTopEdges() {
    const QRect moving(100, 103, 50, 50);
    const QVector<PinsnapCandidate> candidates{
        candidate(QStringLiteral("b"), QRect(140, 100, 50, 60)),
    };
    const auto result = PinsnapEngine::resolveAlign(moving, candidates, kDefaultOptions);
    require(result.has_value(), "top-edge alignment within tolerance should snap");
    require(result->dy == -3, "top edges should align");
    require(result->dx == 0, "no horizontal shift for top-edge align");
}

// Edge align: same-side left alignment (vertical projection overlap + left-edge deviation 3px).
void testAlignLeftEdges() {
    const QRect moving(103, 100, 50, 50);
    // B sits above A: top/bottom deviations both exceed tolerance, so no extra vertical shift.
    const QVector<PinsnapCandidate> candidates{
        candidate(QStringLiteral("b"), QRect(100, 88, 80, 80)),
    };
    const auto result = PinsnapEngine::resolveAlign(moving, candidates, kDefaultOptions);
    require(result.has_value(), "left-edge alignment within tolerance should snap");
    require(result->dx == -3, "left edges should align");
    require(result->dy == 0, "no vertical shift for left-edge align");
}

// Edge align: vertical projection far away -> no snap.
void testAlignProjectionFar() {
    const QRect moving(100, 100, 50, 50);
    const QVector<PinsnapCandidate> candidates{
        candidate(QStringLiteral("b"), QRect(100, 500, 80, 60)),
    };
    const auto result = PinsnapEngine::resolveAlign(moving, candidates, kDefaultOptions);
    require(!result.has_value(), "far projection should reject alignment");
}

// Convenience entry: contact first, fall back to align (contact fails here,
// falls back to top-edge alignment).
void testResolveFallbackToAlign() {
    const QRect moving(100, 103, 50, 50);
    const QVector<PinsnapCandidate> candidates{
        candidate(QStringLiteral("b"), QRect(140, 100, 50, 60)),
    };
    const auto result = PinsnapEngine::resolve(moving, candidates, kDefaultOptions);
    require(result.has_value(), "fallback alignment should snap");
    require(result->dy == -3, "fallback should apply top-edge alignment");
    require(result->dx == 0, "fallback should not shift horizontally");
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    try {
        testHorizontalContactLeft();
        testHorizontalContactRight();
        testVerticalContactTop();
        testSnapDistanceBoundary();
        testNoOverlapRejects();
        testSecondaryAlign();
        testBestCandidate();
        testPreferredTarget();
        testEmptyCandidates();
        testAlignTopEdges();
        testAlignLeftEdges();
        testAlignProjectionFar();
        testResolveFallbackToAlign();
    } catch (const std::exception& error) {
        std::cerr << "pinned snap engine tests failed: " << error.what() << std::endl;
        return 1;
    }

    std::cout << "pinned snap engine tests passed" << std::endl;
    return 0;
}
