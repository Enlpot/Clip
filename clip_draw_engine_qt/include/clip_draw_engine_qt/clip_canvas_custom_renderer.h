#pragma once

#include <QRect>
#include <QRegion>
#include <QTransform>
#include <QtGlobal>

#include <cstdint>
#include <optional>

class QPainter;

// A fixed source-pixel grid for scenes whose filters must scale with the image.
struct ClipCanvasFilterRenderReference {
    QRectF canvasRect;
    qreal pixelsPerCanvasUnit = 1.0;
};

struct ClipCanvasRenderContext {
    QRect viewportRect;
    QRegion exposedRegion;
    QTransform canvasToViewTransform;
    qreal devicePixelRatio = 1.0;
};

class ClipCanvasCustomRenderer {
  public:
    virtual ~ClipCanvasCustomRenderer() = default;

    // Increment when renderBeforeCanvas() or renderOriginalBackground() would produce different
    // pixels. The canvas uses this revision to invalidate its cached background tiles.
    [[nodiscard]] virtual std::uint64_t contentRevision() const;

    // Increment only when pristine pixels change. Hosts with temporary backdrop previews
    // can keep eraser source tiles stable; the default follows contentRevision().
    [[nodiscard]] virtual std::uint64_t originalBackgroundRevision() const;

    // Retain the background and scene at this resolution when filters are present.
    // Zoom, viewport size, and display DPR only transform the retained output.
    // The rectangle must cover the complete scene presented by the host.
    [[nodiscard]] virtual std::optional<ClipCanvasFilterRenderReference>
    filterRenderReference() const;

    // Releases derived data while preserving the sources and rendered content.
    virtual void clearRenderState();

    virtual void renderBeforeCanvas(QPainter& painter, const ClipCanvasRenderContext& context);
    // Paint the pristine host backdrop, excluding annotation and temporary effect previews.
    // The default preserves existing custom renderers' background behavior.
    virtual void renderOriginalBackground(QPainter& painter,
                                          const ClipCanvasRenderContext& context);
    virtual void renderAfterCanvas(QPainter& painter, const ClipCanvasRenderContext& context);
};
