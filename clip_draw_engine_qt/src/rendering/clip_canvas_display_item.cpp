#include "clip_canvas_display_item.h"

#include "clip_canvas_render_geometry.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <limits>
#include <iterator>
#include <unordered_set>
#include <utility>

namespace {

template <typename T>
void assignBorrowed(std::vector<T>& destination, const T* source, std::uint32_t count) {
    if (source == nullptr || count == 0) {
        destination.clear();
        return;
    }
    destination.assign(source, source + count);
}

template <typename T> const T* dataOrNull(const std::vector<T>& values) {
    return values.empty() ? nullptr : values.data();
}

QByteArray borrowedBytes(const char* bytes, std::uint32_t length) {
    if (bytes == nullptr || length == 0) {
        return {};
    }
    return QByteArray(bytes, static_cast<qsizetype>(length));
}

bool hasAnyRadius(const ClipCornerRadii& radii) {
    return radii.top_left > 0.0 || radii.top_right > 0.0 || radii.bottom_right > 0.0 ||
           radii.bottom_left > 0.0;
}

bool fuzzyEqual(double left, double right) {
    return std::abs(left - right) <= 1e-12;
}

bool rectsOverlapInclusive(const QRectF& left, const QRectF& right) {
    return left.right() >= right.left() && right.right() >= left.left() &&
           left.bottom() >= right.top() && right.bottom() >= left.top();
}

constexpr double kPenSpatialCellSize = 256.0;

std::int64_t penSpatialCellKey(int x, int y) {
    return static_cast<std::int64_t>(
        (static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) << 32) |
        static_cast<std::uint32_t>(y));
}

int penSpatialCell(double coordinate) {
    return static_cast<int>(std::floor(coordinate / kPenSpatialCellSize));
}

QPainterPath canvasPathFromCommands(double startX, double startY,
                                    const ClipArrowPathCommand* commands, std::uint32_t count) {
    QPainterPath path;
    path.moveTo(startX, startY);
    if (commands == nullptr) {
        return path;
    }
    for (std::uint32_t index = 0; index < count; ++index) {
        const ClipArrowPathCommand& command = commands[index];
        switch (command.kind) {
        case CLIP_ARROW_PATH_COMMAND_MOVE_TO:
            path.moveTo(command.point.x, command.point.y);
            break;
        case CLIP_ARROW_PATH_COMMAND_LINE_TO:
            path.lineTo(command.point.x, command.point.y);
            break;
        case CLIP_ARROW_PATH_COMMAND_QUAD_TO:
            path.quadTo(command.control1.x, command.control1.y, command.point.x, command.point.y);
            break;
        case CLIP_ARROW_PATH_COMMAND_CUBIC_TO:
            path.cubicTo(command.control1.x, command.control1.y, command.control2.x,
                         command.control2.y, command.point.x, command.point.y);
            break;
        default:
            break;
        }
    }
    return path;
}

bool hasUniformRadii(const ClipCornerRadii& radii) {
    return fuzzyEqual(radii.top_left, radii.top_right) &&
           fuzzyEqual(radii.top_left, radii.bottom_right) &&
           fuzzyEqual(radii.top_left, radii.bottom_left);
}

void rebuildRectangleGeometry(double width, double height, const ClipCornerRadii& radii,
                              QRectF* localRect, QPainterPath* path, bool* rounded, bool* uniform) {
    *localRect = QRectF(-width / 2.0, -height / 2.0, width, height);
    *rounded = hasAnyRadius(radii);
    *uniform = *rounded && hasUniformRadii(radii);
    *path = {};
    if (*rounded && !*uniform && localRect->isValid() && !localRect->isEmpty()) {
        *path = clip_canvas_render_geometry::roundedRectPath(
            *localRect, clip_canvas_render_geometry::toViewCornerRadii(radii, 1.0, *localRect));
    }
}

} // namespace

ClipCanvasSceneItem::ClipCanvasSceneItem() : ClipSceneDisplayItem{} {
    opacity = 1.0;
    refreshPointers();
    rebuildLocalRectangleGeometry();
}

ClipCanvasSceneItem::ClipCanvasSceneItem(const ClipSceneDisplayItem& item) : ClipCanvasSceneItem() {
    assign(item);
}

ClipCanvasSceneItem::ClipCanvasSceneItem(const ClipCanvasSceneItem& other) : ClipCanvasSceneItem() {
    assign(other);
    viewBounds = other.viewBounds;
    m_penGeometryRevision = other.m_penGeometryRevision;
    m_pathChunks = other.m_pathChunks;
    m_pathSpatialCells = other.m_pathSpatialCells;
    m_aggregateClosedPath = other.m_aggregateClosedPath;
    m_pathGeometryRevision = other.m_pathGeometryRevision;
    m_pathClosed = other.m_pathClosed;
}

ClipCanvasSceneItem::ClipCanvasSceneItem(ClipCanvasSceneItem&& other) noexcept
    : ClipSceneDisplayItem(static_cast<const ClipSceneDisplayItem&>(other)),
      viewBounds(other.viewBounds), m_arrowPoints(std::move(other.m_arrowPoints)),
      m_arrowPathCommands(std::move(other.m_arrowPathCommands)),
      m_arrowheadPrimitives(std::move(other.m_arrowheadPrimitives)),
      m_textUtf8(std::move(other.m_textUtf8)), m_fontFamilyUtf8(std::move(other.m_fontFamilyUtf8)),
      m_localRect(other.m_localRect), m_localRectanglePath(std::move(other.m_localRectanglePath)),
      m_hasRoundedCorners(other.m_hasRoundedCorners),
      m_hasUniformCorners(other.m_hasUniformCorners),
      m_penGeometryChunks(std::move(other.m_penGeometryChunks)),
      m_penSpatialCells(std::move(other.m_penSpatialCells)),
      m_penGeometryRevision(other.m_penGeometryRevision),
      m_pathChunks(std::move(other.m_pathChunks)),
      m_pathSpatialCells(std::move(other.m_pathSpatialCells)),
      m_aggregateClosedPath(std::move(other.m_aggregateClosedPath)),
      m_pathGeometryRevision(other.m_pathGeometryRevision), m_pathClosed(other.m_pathClosed),
      m_pendingPenGeometryChunkBuildCount(other.m_pendingPenGeometryChunkBuildCount),
      m_pendingPenGeometryChunkReuseCount(other.m_pendingPenGeometryChunkReuseCount) {
    refreshPointers();
    other.refreshPointers();
    other.m_pendingPenGeometryChunkBuildCount = 0;
    other.m_pendingPenGeometryChunkReuseCount = 0;
}

ClipCanvasSceneItem& ClipCanvasSceneItem::operator=(const ClipCanvasSceneItem& other) {
    if (this != &other) {
        assign(other);
        viewBounds = other.viewBounds;
        m_penGeometryRevision = other.m_penGeometryRevision;
        m_pathChunks = other.m_pathChunks;
        m_pathSpatialCells = other.m_pathSpatialCells;
        m_aggregateClosedPath = other.m_aggregateClosedPath;
        m_pathGeometryRevision = other.m_pathGeometryRevision;
        m_pathClosed = other.m_pathClosed;
    }
    return *this;
}

ClipCanvasSceneItem& ClipCanvasSceneItem::operator=(ClipCanvasSceneItem&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    static_cast<ClipSceneDisplayItem&>(*this) = other;
    viewBounds = other.viewBounds;
    m_arrowPoints = std::move(other.m_arrowPoints);
    m_arrowPathCommands = std::move(other.m_arrowPathCommands);
    m_arrowheadPrimitives = std::move(other.m_arrowheadPrimitives);
    m_textUtf8 = std::move(other.m_textUtf8);
    m_fontFamilyUtf8 = std::move(other.m_fontFamilyUtf8);
    m_localRect = other.m_localRect;
    m_localRectanglePath = std::move(other.m_localRectanglePath);
    m_hasRoundedCorners = other.m_hasRoundedCorners;
    m_hasUniformCorners = other.m_hasUniformCorners;
    m_penGeometryChunks = std::move(other.m_penGeometryChunks);
    m_penSpatialCells = std::move(other.m_penSpatialCells);
    m_penGeometryRevision = other.m_penGeometryRevision;
    m_pathChunks = std::move(other.m_pathChunks);
    m_pathSpatialCells = std::move(other.m_pathSpatialCells);
    m_aggregateClosedPath = std::move(other.m_aggregateClosedPath);
    m_pathGeometryRevision = other.m_pathGeometryRevision;
    m_pathClosed = other.m_pathClosed;
    m_pendingPenGeometryChunkBuildCount = other.m_pendingPenGeometryChunkBuildCount;
    m_pendingPenGeometryChunkReuseCount = other.m_pendingPenGeometryChunkReuseCount;
    refreshPointers();
    other.refreshPointers();
    other.m_pendingPenGeometryChunkBuildCount = 0;
    other.m_pendingPenGeometryChunkReuseCount = 0;
    return *this;
}

ClipCanvasSceneItem& ClipCanvasSceneItem::operator=(const ClipSceneDisplayItem& item) {
    assign(item);
    return *this;
}

void ClipCanvasSceneItem::assign(const ClipSceneDisplayItem& item) {
    const bool preservePenGeometry = kind == CLIP_SCENE_DISPLAY_ITEM_FILTER && is_free_draw != 0 &&
                                     item.kind == CLIP_SCENE_DISPLAY_ITEM_FILTER &&
                                     item.is_free_draw != 0 &&
                                     element_id.index == item.element_id.index &&
                                     element_id.generation == item.element_id.generation &&
                                     item.arrow_points == nullptr && item.arrow_point_count == 0;
    const bool geometryStyleChanged =
        preservePenGeometry &&
        (!fuzzyEqual(stroke_width, item.stroke_width) || !fuzzyEqual(rotation, item.rotation));
    const bool preservePathGeometry = kind == CLIP_SCENE_DISPLAY_ITEM_ARROW &&
                                      item.kind == CLIP_SCENE_DISPLAY_ITEM_ARROW &&
                                      element_id.index == item.element_id.index &&
                                      element_id.generation == item.element_id.generation;
    static_cast<ClipSceneDisplayItem&>(*this) = item;
    if (!preservePenGeometry) {
        assignBorrowed(m_arrowPoints, item.arrow_points, item.arrow_point_count);
        m_penGeometryRevision = 0;
    }
    assignBorrowed(m_arrowPathCommands, item.arrow_path_commands, item.arrow_path_command_count);
    if (!preservePathGeometry) {
        m_pathChunks.clear();
        m_pathSpatialCells.clear();
        m_aggregateClosedPath = {};
        m_pathGeometryRevision = 0;
        m_pathClosed = false;
    }
    assignBorrowed(m_arrowheadPrimitives, item.arrowhead_primitives,
                   item.arrowhead_primitive_count);
    m_textUtf8 = borrowedBytes(item.text_utf8, item.text_utf8_len);
    m_fontFamilyUtf8 = borrowedBytes(item.font_family_utf8, item.font_family_utf8_len);
    refreshPointers();
    rebuildLocalRectangleGeometry();
    if (!preservePenGeometry || geometryStyleChanged) {
        rebuildPenFilterGeometry(0);
    }
}

void ClipCanvasSceneItem::rebuildPenFilterGeometry(std::size_t firstChangedPoint) {
    constexpr std::uint32_t kSegmentsPerChunk = 32;
    const bool dot = filter.filter_type == 7 && m_arrowPoints.size() == 1;
    if (kind != CLIP_SCENE_DISPLAY_ITEM_FILTER || is_free_draw == 0 ||
        (m_arrowPoints.size() < 2 && !dot) || !std::isfinite(stroke_width) || stroke_width <= 0.0) {
        m_penGeometryChunks.clear();
        m_penSpatialCells.clear();
        return;
    }

    std::uint32_t firstSegment = 0;
    std::size_t reused = 0;
    if (firstChangedPoint == 0 || m_penGeometryChunks.empty()) {
        m_penGeometryChunks.clear();
        m_penSpatialCells.clear();
    } else {
        const std::uint32_t changedSegment = static_cast<std::uint32_t>(firstChangedPoint - 1);
        while (!m_penGeometryChunks.empty()) {
            const std::uint32_t chunkIndex =
                static_cast<std::uint32_t>(m_penGeometryChunks.size() - 1);
            const PenSegmentChunk& chunk = m_penGeometryChunks.back();
            const std::uint32_t chunkFirstSegment = chunk.firstPoint;
            const std::uint32_t segmentCount = chunk.pointCount - 1;
            if (chunk.pointCount > 1 && chunkFirstSegment + segmentCount <= changedSegment) {
                break;
            }
            for (std::int64_t cell : chunk.spatialCells) {
                auto found = m_penSpatialCells.find(cell);
                if (found == m_penSpatialCells.end()) {
                    continue;
                }
                auto& indices = found->second;
                indices.erase(std::remove(indices.begin(), indices.end(), chunkIndex),
                              indices.end());
                if (indices.empty()) {
                    m_penSpatialCells.erase(found);
                }
            }
            m_penGeometryChunks.pop_back();
        }
        reused = m_penGeometryChunks.size();
        if (!m_penGeometryChunks.empty()) {
            const PenSegmentChunk& chunk = m_penGeometryChunks.back();
            firstSegment = chunk.firstPoint + chunk.pointCount - 1;
        }
    }

    const std::uint32_t segmentCount =
        dot ? 1 : static_cast<std::uint32_t>(m_arrowPoints.size() - 1);
    std::size_t built = 0;
    while (firstSegment < segmentCount) {
        const std::uint32_t count =
            dot ? 0 : std::min(kSegmentsPerChunk, segmentCount - firstSegment);
        double minX = std::numeric_limits<double>::infinity();
        double minY = std::numeric_limits<double>::infinity();
        double maxX = -std::numeric_limits<double>::infinity();
        double maxY = -std::numeric_limits<double>::infinity();
        for (std::uint32_t offset = 0; offset <= count; ++offset) {
            const ClipArrowPoint& point = m_arrowPoints[firstSegment + offset];
            minX = std::min(minX, point.x);
            minY = std::min(minY, point.y);
            maxX = std::max(maxX, point.x);
            maxY = std::max(maxY, point.y);
        }
        const double radius = stroke_width * 0.5;
        PenSegmentChunk chunk;
        chunk.firstPoint = firstSegment;
        chunk.pointCount = count + 1;
        chunk.canvasBounds =
            QRectF(QPointF(minX - radius, minY - radius), QPointF(maxX + radius, maxY + radius));
        chunk.revision = m_penGeometryRevision;
        const int firstCellX = penSpatialCell(chunk.canvasBounds.left());
        const int lastCellX = penSpatialCell(chunk.canvasBounds.right());
        const int firstCellY = penSpatialCell(chunk.canvasBounds.top());
        const int lastCellY = penSpatialCell(chunk.canvasBounds.bottom());
        for (int y = firstCellY; y <= lastCellY; ++y) {
            for (int x = firstCellX; x <= lastCellX; ++x) {
                chunk.spatialCells.push_back(penSpatialCellKey(x, y));
            }
        }
        const std::uint32_t chunkIndex = static_cast<std::uint32_t>(m_penGeometryChunks.size());
        for (std::int64_t cell : chunk.spatialCells) {
            m_penSpatialCells[cell].push_back(chunkIndex);
        }
        m_penGeometryChunks.push_back(std::move(chunk));
        ++built;
        firstSegment += std::max(1u, count);
    }
    m_pendingPenGeometryChunkBuildCount += built;
    m_pendingPenGeometryChunkReuseCount += reused;
}

const std::vector<ClipCanvasSceneItem::PenSegmentChunk>&
ClipCanvasSceneItem::penSegmentChunks() const {
    return m_penGeometryChunks;
}

const std::vector<ClipArrowPoint>& ClipCanvasSceneItem::penFilterPoints() const {
    return m_arrowPoints;
}

void ClipCanvasSceneItem::queryPenSegmentChunks(const QRectF& canvasBounds,
                                                std::vector<std::uint32_t>* outChunkIndices,
                                                std::size_t* candidateCount) const {
    if (outChunkIndices == nullptr || canvasBounds.isEmpty()) {
        return;
    }
    outChunkIndices->clear();
    std::unordered_set<std::uint32_t> seen;
    const int firstCellX = penSpatialCell(canvasBounds.left());
    const int lastCellX = penSpatialCell(canvasBounds.right());
    const int firstCellY = penSpatialCell(canvasBounds.top());
    const int lastCellY = penSpatialCell(canvasBounds.bottom());
    for (int y = firstCellY; y <= lastCellY; ++y) {
        for (int x = firstCellX; x <= lastCellX; ++x) {
            const auto found = m_penSpatialCells.find(penSpatialCellKey(x, y));
            if (found == m_penSpatialCells.end()) {
                continue;
            }
            if (candidateCount != nullptr) {
                *candidateCount += found->second.size();
            }
            for (std::uint32_t index : found->second) {
                if (seen.insert(index).second && index < m_penGeometryChunks.size() &&
                    m_penGeometryChunks[index].canvasBounds.intersects(canvasBounds)) {
                    outChunkIndices->push_back(index);
                }
            }
        }
    }
}

void ClipCanvasSceneItem::takePenFilterGeometryDiagnostics(std::size_t* chunkBuildCount,
                                                           std::size_t* chunkReuseCount) const {
    if (chunkBuildCount != nullptr) {
        *chunkBuildCount += m_pendingPenGeometryChunkBuildCount;
    }
    if (chunkReuseCount != nullptr) {
        *chunkReuseCount += m_pendingPenGeometryChunkReuseCount;
    }
    m_pendingPenGeometryChunkBuildCount = 0;
    m_pendingPenGeometryChunkReuseCount = 0;
}

std::uint64_t ClipCanvasSceneItem::penFilterGeometryRevision() const {
    return m_penGeometryRevision;
}

void ClipCanvasSceneItem::refreshPointers() {
    arrow_points = dataOrNull(m_arrowPoints);
    arrow_point_count = static_cast<std::uint32_t>(m_arrowPoints.size());
    arrow_path_commands = dataOrNull(m_arrowPathCommands);
    arrow_path_command_count = static_cast<std::uint32_t>(m_arrowPathCommands.size());
    arrowhead_primitives = dataOrNull(m_arrowheadPrimitives);
    arrowhead_primitive_count = static_cast<std::uint32_t>(m_arrowheadPrimitives.size());
    text_utf8 = m_textUtf8.isEmpty() ? nullptr : m_textUtf8.constData();
    text_utf8_len = static_cast<std::uint32_t>(m_textUtf8.size());
    font_family_utf8 = m_fontFamilyUtf8.isEmpty() ? nullptr : m_fontFamilyUtf8.constData();
    font_family_utf8_len = static_cast<std::uint32_t>(m_fontFamilyUtf8.size());
}

void ClipCanvasSceneItem::setArrowPoints(const ClipArrowPoint* points, std::uint32_t count) {
    assignBorrowed(m_arrowPoints, points, count);
    m_penGeometryRevision = 0;
    refreshPointers();
    rebuildPenFilterGeometry(0);
}

bool ClipCanvasSceneItem::applyPenFilterGeometryPatch(std::uint64_t expectedRevision,
                                                      std::uint64_t revision,
                                                      std::uint32_t retainPrefixCount,
                                                      const ClipArrowPoint* appendedPoints,
                                                      std::uint32_t appendCount, bool fullReset) {
    if (kind != CLIP_SCENE_DISPLAY_ITEM_FILTER || is_free_draw == 0 ||
        (!fullReset && m_penGeometryRevision != expectedRevision) ||
        (appendCount != 0 && appendedPoints == nullptr) ||
        (!fullReset && retainPrefixCount > m_arrowPoints.size())) {
        return false;
    }
    const std::size_t firstChangedPoint = fullReset ? 0 : retainPrefixCount;
    if (fullReset) {
        m_arrowPoints.clear();
    } else {
        m_arrowPoints.resize(firstChangedPoint);
    }
    if (appendCount != 0) {
        m_arrowPoints.insert(m_arrowPoints.end(), appendedPoints, appendedPoints + appendCount);
    }
    m_penGeometryRevision = revision;
    refreshPointers();
    rebuildPenFilterGeometry(firstChangedPoint);
    return true;
}

bool ClipCanvasSceneItem::applyPathGeometryPatch(
    std::uint64_t expectedRevision, std::uint64_t revision, const ClipPathChunkRange* ranges,
    std::uint32_t rangeCount, const ClipPathChunk* chunks, std::uint32_t chunkCount,
    const ClipArrowPathCommand* commands, std::uint32_t commandCount, bool closed, bool fullReset) {
    if (kind != CLIP_SCENE_DISPLAY_ITEM_ARROW ||
        (!fullReset && m_pathGeometryRevision != expectedRevision) ||
        (rangeCount != 0 && ranges == nullptr)) {
        return false;
    }
    if (fullReset) {
        m_pathChunks.clear();
    }
    using Difference = std::vector<PathChunk>::difference_type;
    for (std::uint32_t rangeIndex = 0; rangeIndex < rangeCount; ++rangeIndex) {
        const ClipPathChunkRange& range = ranges[rangeIndex];
        const std::size_t start = std::min<std::size_t>(m_pathChunks.size(), range.start);
        const std::size_t deleteCount =
            std::min<std::size_t>(m_pathChunks.size() - start, range.delete_count);
        if (range.insert_chunk_offset > chunkCount ||
            range.insert_chunk_count > chunkCount - range.insert_chunk_offset) {
            return false;
        }
        std::vector<PathChunk> inserted;
        inserted.reserve(range.insert_chunk_count);
        for (std::uint32_t offset = 0; offset < range.insert_chunk_count; ++offset) {
            const ClipPathChunk& source = chunks[range.insert_chunk_offset + offset];
            if (source.command_offset > commandCount ||
                source.command_count > commandCount - source.command_offset) {
                return false;
            }
            PathChunk chunk;
            chunk.stableId = source.stable_id;
            chunk.commandStart = source.command_start;
            chunk.canvasBounds =
                QRectF(QPointF(source.min_x, source.min_y), QPointF(source.max_x, source.max_y))
                    .normalized();
            chunk.cumulativeStartLength = source.cumulative_start_length;
            chunk.canvasPath = canvasPathFromCommands(
                source.start_x, source.start_y,
                source.command_count == 0 ? nullptr : commands + source.command_offset,
                source.command_count);
            inserted.push_back(std::move(chunk));
        }
        m_pathChunks.erase(m_pathChunks.begin() + static_cast<Difference>(start),
                           m_pathChunks.begin() + static_cast<Difference>(start + deleteCount));
        m_pathChunks.insert(m_pathChunks.begin() + static_cast<Difference>(start),
                            std::make_move_iterator(inserted.begin()),
                            std::make_move_iterator(inserted.end()));
    }
    m_pathGeometryRevision = revision;
    m_pathClosed = closed;
    rebuildPathSpatialIndex();
    rebuildAggregateClosedPath();
    return true;
}

void ClipCanvasSceneItem::rebuildPathSpatialIndex() {
    m_pathSpatialCells.clear();
    for (std::uint32_t index = 0; index < m_pathChunks.size(); ++index) {
        PathChunk& chunk = m_pathChunks[index];
        chunk.spatialCells.clear();
        const int firstCellX = penSpatialCell(chunk.canvasBounds.left());
        const int lastCellX = penSpatialCell(chunk.canvasBounds.right());
        const int firstCellY = penSpatialCell(chunk.canvasBounds.top());
        const int lastCellY = penSpatialCell(chunk.canvasBounds.bottom());
        for (int y = firstCellY; y <= lastCellY; ++y) {
            for (int x = firstCellX; x <= lastCellX; ++x) {
                const std::int64_t cell = penSpatialCellKey(x, y);
                chunk.spatialCells.push_back(cell);
                m_pathSpatialCells[cell].push_back(index);
            }
        }
    }
}

void ClipCanvasSceneItem::rebuildAggregateClosedPath() {
    m_aggregateClosedPath = {};
    if (!m_pathClosed || fill.a == 0) {
        return;
    }
    for (const PathChunk& chunk : m_pathChunks) {
        m_aggregateClosedPath.connectPath(chunk.canvasPath);
    }
    m_aggregateClosedPath.closeSubpath();
}

const std::vector<ClipCanvasSceneItem::PathChunk>& ClipCanvasSceneItem::pathChunks() const {
    return m_pathChunks;
}

const QPainterPath& ClipCanvasSceneItem::aggregateClosedPath() const {
    return m_aggregateClosedPath;
}

bool ClipCanvasSceneItem::pathIsClosed() const {
    return m_pathClosed;
}
std::uint64_t ClipCanvasSceneItem::pathGeometryRevision() const {
    return m_pathGeometryRevision;
}

void ClipCanvasSceneItem::queryPathChunks(const QRectF& canvasBounds,
                                          std::vector<std::uint32_t>* outChunkIndices) const {
    if (outChunkIndices == nullptr) {
        return;
    }
    outChunkIndices->clear();
    std::unordered_set<std::uint32_t> seen;
    const int firstCellX = penSpatialCell(canvasBounds.left());
    const int lastCellX = penSpatialCell(canvasBounds.right());
    const int firstCellY = penSpatialCell(canvasBounds.top());
    const int lastCellY = penSpatialCell(canvasBounds.bottom());
    for (int y = firstCellY; y <= lastCellY; ++y) {
        for (int x = firstCellX; x <= lastCellX; ++x) {
            const auto found = m_pathSpatialCells.find(penSpatialCellKey(x, y));
            if (found == m_pathSpatialCells.end()) {
                continue;
            }
            for (std::uint32_t index : found->second) {
                if (seen.insert(index).second && index < m_pathChunks.size() &&
                    rectsOverlapInclusive(m_pathChunks[index].canvasBounds, canvasBounds)) {
                    outChunkIndices->push_back(index);
                }
            }
        }
    }
    std::sort(outChunkIndices->begin(), outChunkIndices->end());
}

void ClipCanvasSceneItem::setTextUtf8(const QByteArray& text) {
    m_textUtf8 = text;
    refreshPointers();
}

void ClipCanvasSceneItem::setFontFamilyUtf8(const QByteArray& family) {
    m_fontFamilyUtf8 = family;
    refreshPointers();
}

void ClipCanvasSceneItem::rebuildLocalRectangleGeometry() {
    rebuildRectangleGeometry(width, height, corner_radii, &m_localRect, &m_localRectanglePath,
                             &m_hasRoundedCorners, &m_hasUniformCorners);
}

const QRectF& ClipCanvasSceneItem::localRect() const {
    return m_localRect;
}
const QPainterPath& ClipCanvasSceneItem::localRectanglePath() const {
    return m_localRectanglePath;
}
bool ClipCanvasSceneItem::hasRoundedCorners() const {
    return m_hasRoundedCorners;
}
bool ClipCanvasSceneItem::hasUniformCorners() const {
    return m_hasUniformCorners;
}

ClipCanvasOverlayItem::ClipCanvasOverlayItem() : ClipOverlayDisplayItem{} {
    refreshPointers();
    rebuildLocalRectangleGeometry();
}

ClipCanvasOverlayItem::ClipCanvasOverlayItem(const ClipOverlayDisplayItem& item) {
    assign(item);
}

ClipCanvasOverlayItem::ClipCanvasOverlayItem(const ClipCanvasOverlayItem& other)
    : ClipOverlayDisplayItem(static_cast<const ClipOverlayDisplayItem&>(other)),
      viewBounds(other.viewBounds), m_arrowPoints(other.m_arrowPoints),
      m_arrowPathCommands(other.m_arrowPathCommands),
      m_arrowheadPrimitives(other.m_arrowheadPrimitives), m_localRect(other.m_localRect),
      m_localRectanglePath(other.m_localRectanglePath),
      m_hasRoundedCorners(other.m_hasRoundedCorners),
      m_hasUniformCorners(other.m_hasUniformCorners) {
    refreshPointers();
}

ClipCanvasOverlayItem::ClipCanvasOverlayItem(ClipCanvasOverlayItem&& other) noexcept
    : ClipOverlayDisplayItem(static_cast<const ClipOverlayDisplayItem&>(other)),
      viewBounds(other.viewBounds), m_arrowPoints(std::move(other.m_arrowPoints)),
      m_arrowPathCommands(std::move(other.m_arrowPathCommands)),
      m_arrowheadPrimitives(std::move(other.m_arrowheadPrimitives)), m_localRect(other.m_localRect),
      m_localRectanglePath(std::move(other.m_localRectanglePath)),
      m_hasRoundedCorners(other.m_hasRoundedCorners),
      m_hasUniformCorners(other.m_hasUniformCorners) {
    refreshPointers();
    other.refreshPointers();
}

ClipCanvasOverlayItem& ClipCanvasOverlayItem::operator=(const ClipCanvasOverlayItem& other) {
    if (this != &other) {
        assign(other);
        viewBounds = other.viewBounds;
    }
    return *this;
}

ClipCanvasOverlayItem& ClipCanvasOverlayItem::operator=(ClipCanvasOverlayItem&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    static_cast<ClipOverlayDisplayItem&>(*this) = other;
    viewBounds = other.viewBounds;
    m_arrowPoints = std::move(other.m_arrowPoints);
    m_arrowPathCommands = std::move(other.m_arrowPathCommands);
    m_arrowheadPrimitives = std::move(other.m_arrowheadPrimitives);
    m_localRect = other.m_localRect;
    m_localRectanglePath = std::move(other.m_localRectanglePath);
    m_hasRoundedCorners = other.m_hasRoundedCorners;
    m_hasUniformCorners = other.m_hasUniformCorners;
    refreshPointers();
    other.refreshPointers();
    return *this;
}

ClipCanvasOverlayItem& ClipCanvasOverlayItem::operator=(const ClipOverlayDisplayItem& item) {
    assign(item);
    return *this;
}

void ClipCanvasOverlayItem::assign(const ClipOverlayDisplayItem& item) {
    static_cast<ClipOverlayDisplayItem&>(*this) = item;
    assignBorrowed(m_arrowPoints, item.arrow_points, item.arrow_point_count);
    assignBorrowed(m_arrowPathCommands, item.arrow_path_commands, item.arrow_path_command_count);
    assignBorrowed(m_arrowheadPrimitives, item.arrowhead_primitives,
                   item.arrowhead_primitive_count);
    refreshPointers();
    rebuildLocalRectangleGeometry();
}

void ClipCanvasOverlayItem::refreshPointers() {
    arrow_points = dataOrNull(m_arrowPoints);
    arrow_point_count = static_cast<std::uint32_t>(m_arrowPoints.size());
    arrow_path_commands = dataOrNull(m_arrowPathCommands);
    arrow_path_command_count = static_cast<std::uint32_t>(m_arrowPathCommands.size());
    arrowhead_primitives = dataOrNull(m_arrowheadPrimitives);
    arrowhead_primitive_count = static_cast<std::uint32_t>(m_arrowheadPrimitives.size());
}

void ClipCanvasOverlayItem::rebuildLocalRectangleGeometry() {
    rebuildRectangleGeometry(width, height, corner_radii, &m_localRect, &m_localRectanglePath,
                             &m_hasRoundedCorners, &m_hasUniformCorners);
}

const QRectF& ClipCanvasOverlayItem::localRect() const {
    return m_localRect;
}
const QPainterPath& ClipCanvasOverlayItem::localRectanglePath() const {
    return m_localRectanglePath;
}
bool ClipCanvasOverlayItem::hasRoundedCorners() const {
    return m_hasRoundedCorners;
}
bool ClipCanvasOverlayItem::hasUniformCorners() const {
    return m_hasUniformCorners;
}
