#pragma once

#include <QImage>
#include <QByteArray>
#include <QList>
#include <QJsonArray>
#include <QRectF>
#include <QSize>
#include <QSizeF>

#include <memory>
#include <functional>

#include "clip_draw_engine_qt/clip_canvas_export_types.h"
#include "clip_draw_engine_qt/clip_canvas_smart_erase.h"
#include "clip_draw_engine_qt/clip_canvas_types.h"

namespace clip_canvas_runtime {
struct Access;
}

class ClipCanvasRuntime {
  public:
    ClipCanvasRuntime();
    explicit ClipCanvasRuntime(const ClipCanvasRuntimeConfig& config);
    ~ClipCanvasRuntime();

    ClipCanvasRuntime(const ClipCanvasRuntime&) = delete;
    ClipCanvasRuntime& operator=(const ClipCanvasRuntime&) = delete;

    // Thread-affine: construct, use, and destroy a runtime on the same thread.
    // Cross-thread calls that can fail return false or an empty image.
    bool isOwnerThread() const;
    bool isValid() const;
    bool reset();
    bool cloneDocumentSessionFrom(const ClipCanvasRuntime& source);
    QByteArray serializeDocumentSession() const;
    QByteArray serializeSelectedDrawTemplate() const;
    QJsonArray selectedElementIds() const;
    bool restoreDocumentSession(const QByteArray& payload);
    QByteArray serializeDocumentHistory() const;
    bool restoreDocumentHistory(const QByteArray& payload);
    bool restoreDocumentHistoryPreservingEditorStyles(const QByteArray& payload);
    bool clearDocumentPreservingViewports();
    // Releases derived drawing caches without changing the document, history, or viewports.
    void clearRenderState();
    bool setQuickSelectionDisabledTools(const QSet<ClipCanvasTool>& tools);
    QByteArray applyAnnotationTransaction(const QByteArray& payload);
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;
    quint64 documentRevision() const;
    // Includes hidden elements and watermark content, but excludes creation styles and history.
    bool hasDocumentContent() const;
    void setDocumentChangedHandler(std::function<void()> handler);
    // Detaches clients immediately. Engine storage is released by the process cleanup worker,
    // without waiting in the runtime owner's destructor.
    void destroyAsync();
    void setBaseImageSources(const QList<ClipCanvasBaseImageSource>& sources);
    ClipCanvasSmartEraseSnapshot smartEraseSnapshot() const;
    void restoreSmartEraseSnapshot(const ClipCanvasSmartEraseSnapshot& snapshot);
    QImage renderToImage(const QRectF& virtualSelectionRect, const QSize& outputSize,
                         const QList<CanvasExportSource>& sources);

  private:
    friend struct clip_canvas_runtime::Access;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// A short-lived editor viewport without QWidget, rendering surfaces, or event processing.
// It must be destroyed before its runtime is reset/restored and used on the runtime owner thread.
class ClipCanvasRuntimeEditor final {
  public:
    explicit ClipCanvasRuntimeEditor(ClipCanvasRuntime& runtime,
                                     ClipCanvasTool tool = ClipCanvasTool::Select);
    ~ClipCanvasRuntimeEditor();
    ClipCanvasRuntimeEditor(const ClipCanvasRuntimeEditor&) = delete;
    ClipCanvasRuntimeEditor& operator=(const ClipCanvasRuntimeEditor&) = delete;
    bool isValid() const;
    bool setActiveTool(ClipCanvasTool tool);
    ClipCanvasStyleToolbarState canvasStyleToolbarState() const;
    ClipCanvasWatermarkConfig canvasWatermarkConfig() const;
    ClipCanvasSpotlightConfig canvasSpotlightConfig() const;
    bool setDistanceStyleFromToolbar(const ClipCanvasDistanceStyle&, quint32);
    bool setDistanceCreationPixelScale(const QSizeF&);
    bool setShapeStyleFromToolbar(const ClipCanvasShapeStyle&, quint32, ClipCanvasShapeKind);
    bool setTextStyleFromToolbar(const ClipCanvasTextStyle&,
                                 quint32 properties = ClipCanvasTextStyleAllProperties);
    bool setSerialNumberStyleFromToolbar(const ClipCanvasSerialNumberStyle&);
    bool setSerialNumberStyleFromToolbar(const ClipCanvasSerialNumberStyle&, quint32 properties);
    bool setFilterStyleFromToolbar(const ClipCanvasFilterStyle&, quint32);
    bool
    setBrushEraserCreationStyle(const ClipCanvasBrushEraserStyle&,
                                quint32 properties = ClipCanvasBrushEraserStylePropertyStrokeWidth);
    bool setWatermarkConfigFromToolbar(const ClipCanvasWatermarkConfig&);
    bool setSpotlightConfigFromToolbar(const ClipCanvasSpotlightConfig&);
    bool select(quint32 index, quint32 generation);
    bool deleteSelected();
    bool deleteAllElements();
    // Rectangle Eraser uses two corners; Brush Eraser accepts a stroke or a single dot.
    bool erasePath(const QList<QPointF>& points,
                   ClipCanvasTool eraserTool = ClipCanvasTool::Eraser);
    bool duplicateSelected(QPointF offset = QPointF(20, 20));
    bool reorderSelected(ClipCanvasSelectionOrder);
    bool alignSelected(ClipCanvasSelectionAlignment);
    bool setSelectedOpacity(double);
    bool adjustSelectedSerialNumbers(qint64);
    bool createSerialNumberText();
    bool insertDrawTemplate(const QByteArray&, QPointF center);
    bool setAutoFilterRegions(const ClipCanvasAutoFilterRecord&);
    bool fillAutoFilterCategory(const QString& category);
    bool succeeded() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
