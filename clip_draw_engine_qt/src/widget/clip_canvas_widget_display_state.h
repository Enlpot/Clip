#pragma once

#include "clip_canvas_display_cache.h"
#include "clip_canvas_state.h"

#include <QFont>
#include <QRect>
#include <QRegion>

class ClipCanvasTextEditorSession;
class ClipCanvasViewport;

class ClipCanvasWidgetDisplayState final {
  public:
    ClipCanvasWidgetDisplayState();

    ClipCanvasDisplayCache& displayCache();
    const ClipCanvasDisplayCache& displayCache() const;
    const clip_canvas_state::Snapshot& snapshot() const;

    void resetRetainedState();
    void resetDocumentRetainedState();
    std::uint64_t initializeEngine(ClipRuntime runtime, ClipCanvasViewport& viewport);
    bool refreshState(ClipRuntime runtime, ClipViewport viewport,
                      clip_canvas_state::Changes* outChanges = nullptr);
    QRegion syncDisplayCache(ClipRuntime runtime, ClipViewport viewport, const QRect& widgetRect,
                             const QFont& font, ClipCanvasTextEditorSession& textEditorSession,
                             bool showDirtyRects);

    const QRegion& dirtyVisualizationRegion() const;

  private:
    ClipCanvasDisplayCache m_displayCache;
    clip_canvas_state::Store m_engineState;
    QRegion m_dirtyVisualizationRegion;
};
