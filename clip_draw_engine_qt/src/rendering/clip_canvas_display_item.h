#pragma once

#include "clip_draw_engine.h"

#include <QByteArray>
#include <QPainterPath>
#include <QRectF>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

class ClipCanvasSceneItem : public ClipSceneDisplayItem {
  public:
    ClipCanvasSceneItem();
    explicit ClipCanvasSceneItem(const ClipSceneDisplayItem& item);
    ClipCanvasSceneItem(const ClipCanvasSceneItem& other);
    ClipCanvasSceneItem(ClipCanvasSceneItem&& other) noexcept;
    ClipCanvasSceneItem& operator=(const ClipCanvasSceneItem& other);
    ClipCanvasSceneItem& operator=(ClipCanvasSceneItem&& other) noexcept;
    ClipCanvasSceneItem& operator=(const ClipSceneDisplayItem& item);

    void setArrowPoints(const ClipArrowPoint* points, std::uint32_t count);
    bool applyPenFilterGeometryPatch(std::uint64_t expectedRevision, std::uint64_t revision,
                                     std::uint32_t retainPrefixCount,
                                     const ClipArrowPoint* appendedPoints,
                                     std::uint32_t appendCount, bool fullReset);
    bool applyPathGeometryPatch(std::uint64_t expectedRevision, std::uint64_t revision,
                                const ClipPathChunkRange* ranges, std::uint32_t rangeCount,
                                const ClipPathChunk* chunks, std::uint32_t chunkCount,
                                const ClipArrowPathCommand* commands, std::uint32_t commandCount,
                                bool closed, bool fullReset);
    void setTextUtf8(const QByteArray& text);
    void setFontFamilyUtf8(const QByteArray& family);
    void rebuildLocalRectangleGeometry();

    const QRectF& localRect() const;
    const QPainterPath& localRectanglePath() const;
    bool hasRoundedCorners() const;
    bool hasUniformCorners() const;
    struct PenSegmentChunk {
        std::uint32_t firstPoint = 0;
        std::uint32_t pointCount = 0;
        QRectF canvasBounds;
        std::uint64_t revision = 0;
        std::vector<std::int64_t> spatialCells;
    };
    const std::vector<PenSegmentChunk>& penSegmentChunks() const;
    const std::vector<ClipArrowPoint>& penFilterPoints() const;
    void queryPenSegmentChunks(const QRectF& canvasBounds,
                               std::vector<std::uint32_t>* outChunkIndices,
                               std::size_t* candidateCount = nullptr) const;
    void takePenFilterGeometryDiagnostics(std::size_t* chunkBuildCount,
                                          std::size_t* chunkReuseCount) const;
    std::uint64_t penFilterGeometryRevision() const;
    struct PathChunk {
        std::uint64_t stableId = 0;
        std::uint32_t commandStart = 0;
        QRectF canvasBounds;
        double cumulativeStartLength = 0.0;
        QPainterPath canvasPath;
        std::vector<std::int64_t> spatialCells;
    };
    const std::vector<PathChunk>& pathChunks() const;
    const QPainterPath& aggregateClosedPath() const;
    bool pathIsClosed() const;
    void queryPathChunks(const QRectF& canvasBounds,
                         std::vector<std::uint32_t>* outChunkIndices) const;
    std::uint64_t pathGeometryRevision() const;

    QRectF viewBounds;

  private:
    void assign(const ClipSceneDisplayItem& item);
    void refreshPointers();
    void rebuildPenFilterGeometry(std::size_t firstChangedPoint);
    void rebuildPathSpatialIndex();
    void rebuildAggregateClosedPath();

    std::vector<ClipArrowPoint> m_arrowPoints;
    std::vector<ClipArrowPathCommand> m_arrowPathCommands;
    std::vector<ClipArrowheadPrimitive> m_arrowheadPrimitives;
    QByteArray m_textUtf8;
    QByteArray m_fontFamilyUtf8;
    QRectF m_localRect;
    QPainterPath m_localRectanglePath;
    bool m_hasRoundedCorners = false;
    bool m_hasUniformCorners = false;
    std::vector<PenSegmentChunk> m_penGeometryChunks;
    std::unordered_map<std::int64_t, std::vector<std::uint32_t>> m_penSpatialCells;
    std::uint64_t m_penGeometryRevision = 0;
    std::vector<PathChunk> m_pathChunks;
    std::unordered_map<std::int64_t, std::vector<std::uint32_t>> m_pathSpatialCells;
    QPainterPath m_aggregateClosedPath;
    std::uint64_t m_pathGeometryRevision = 0;
    bool m_pathClosed = false;
    mutable std::size_t m_pendingPenGeometryChunkBuildCount = 0;
    mutable std::size_t m_pendingPenGeometryChunkReuseCount = 0;
};
class ClipCanvasOverlayItem : public ClipOverlayDisplayItem {
  public:
    ClipCanvasOverlayItem();
    explicit ClipCanvasOverlayItem(const ClipOverlayDisplayItem& item);
    ClipCanvasOverlayItem(const ClipCanvasOverlayItem& other);
    ClipCanvasOverlayItem(ClipCanvasOverlayItem&& other) noexcept;
    ClipCanvasOverlayItem& operator=(const ClipCanvasOverlayItem& other);
    ClipCanvasOverlayItem& operator=(ClipCanvasOverlayItem&& other) noexcept;
    ClipCanvasOverlayItem& operator=(const ClipOverlayDisplayItem& item);

    void rebuildLocalRectangleGeometry();

    const QRectF& localRect() const;
    const QPainterPath& localRectanglePath() const;
    bool hasRoundedCorners() const;
    bool hasUniformCorners() const;

    QRectF viewBounds;

  private:
    void assign(const ClipOverlayDisplayItem& item);
    void refreshPointers();

    std::vector<ClipArrowPoint> m_arrowPoints;
    std::vector<ClipArrowPathCommand> m_arrowPathCommands;
    std::vector<ClipArrowheadPrimitive> m_arrowheadPrimitives;
    QRectF m_localRect;
    QPainterPath m_localRectanglePath;
    bool m_hasRoundedCorners = false;
    bool m_hasUniformCorners = false;
};
