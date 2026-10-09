#pragma once

#include "clip_draw_engine.h"

namespace clip_canvas_state {

struct Snapshot {
    ClipActiveTool activeTool = CLIP_ACTIVE_TOOL_SHAPE;
    ClipStyleToolbarState styleToolbarState{};
    ClipWatermarkConfig watermarkConfig{};
    ClipSpotlightConfig spotlightConfig{};
    ClipHistoryState historyState{};
    ClipSnapConfig snapConfig{};
    ClipGridConfig gridConfig{};
};

struct Changes {
    bool activeToolChanged = false;
    bool styleToolbarChanged = false;
    bool historyChanged = false;
    bool snapConfigChanged = false;
    bool gridConfigChanged = false;

    bool any() const;
};

bool colorsEqual(const ClipColorRgba8& lhs, const ClipColorRgba8& rhs);
bool cornerRadiiEqual(const ClipCornerRadii& lhs, const ClipCornerRadii& rhs);
bool shapeStylesEqual(const ClipShapeStyle& lhs, const ClipShapeStyle& rhs);
bool textStylesEqual(const ClipTextStyle& lhs, const ClipTextStyle& rhs);
bool serialNumberStylesEqual(const ClipSerialNumberStyle& lhs, const ClipSerialNumberStyle& rhs);
bool filterStylesEqual(const ClipFilterStyle& lhs, const ClipFilterStyle& rhs);
bool styleToolbarStatesEqual(const ClipStyleToolbarState& lhs, const ClipStyleToolbarState& rhs);
bool watermarkConfigsEqual(const ClipWatermarkConfig& lhs, const ClipWatermarkConfig& rhs);
bool spotlightConfigsEqual(const ClipSpotlightConfig& lhs, const ClipSpotlightConfig& rhs);
bool historyStatesEqual(const ClipHistoryState& lhs, const ClipHistoryState& rhs);
bool snapConfigsEqual(const ClipSnapConfig& lhs, const ClipSnapConfig& rhs);
bool gridConfigsEqual(const ClipGridConfig& lhs, const ClipGridConfig& rhs);

Changes diffSnapshots(const Snapshot& previous, const Snapshot& next);
bool readSnapshot(ClipRuntime runtime, ClipViewport viewport, Snapshot* outSnapshot);

class Store final {
  public:
    const Snapshot& snapshot() const;
    void reset();
    void applyEngineConfigDefaults(const ClipEngineConfig& config);
    bool refresh(ClipRuntime runtime, ClipViewport viewport, Changes* outChanges = nullptr);

  private:
    Snapshot m_snapshot;
};

} // namespace clip_canvas_state
