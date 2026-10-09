#pragma once

#include "clip_draw_engine.h"
#include "clip_canvas_text_layout.h"

#include <QFont>
#include <QByteArray>
#include <QCache>

#include <cstdint>
#include <vector>

namespace clip_canvas_text_measurement {

// Natural metrics depend on typography, not on the arrow's current wrap width.
// Costs include the key's text/font bytes so very large labels cannot grow the
// cache without bound. Wrapped layouts remain measured at the exact live width.
class NaturalTextLayoutCache {
  public:
    explicit NaturalTextLayoutCache(int maximumBytes = 1024 * 1024);
    clip_canvas_text_layout::TextMeasuredLayout measure(const QString& text, const QFont& baseFont,
                                                        const ClipSceneDisplayItem& item);
    void clear();
    qsizetype retainedBytes() const;
    std::uint64_t measurementCount() const;

  private:
    QCache<QByteArray, clip_canvas_text_layout::TextMeasuredLayout> m_layouts;
    std::uint64_t m_measurementCount = 0;
};

struct TextLayoutOverrideMeasurement {
    bool success = true;
    std::vector<ClipTextLayoutOverride> layouts;
};

struct SelectedTextLayoutMeasurementRequest {
    ClipRuntime runtime = nullptr;
    ClipViewport viewport = nullptr;
    ClipTextStyle style{};
    QFont baseFont;
    std::uint32_t properties = CLIP_TEXT_STYLE_ALL_PROPERTIES;
};

struct ResizeLayoutMeasurementRequest {
    ClipTextElementInfo info{};
    QFont baseFont;
    double zoom = 1.0;
};

TextLayoutOverrideMeasurement
measureAutoResizeLayoutOverrides(const ClipTextElementInfo* infos, std::uint32_t infoCount,
                                 const ClipTextStyle& style, const QFont& baseFont,
                                 std::uint32_t properties = CLIP_TEXT_STYLE_ALL_PROPERTIES);
TextLayoutOverrideMeasurement
measureSelectedAutoResizeLayoutOverrides(const SelectedTextLayoutMeasurementRequest& request);
ClipTextLayoutSize measureEmptyDraftLayout(const ClipTextStyle& style, const QFont& baseFont);
ClipTextLayoutSize measureSerialLabelLayout(const ClipSerialLabelLayoutRequest& request,
                                            const QFont& baseFont);
ClipTextLayoutSize measureSerialNumberBoundTextLayout(const ClipTextStyle& textStyle,
                                                      const QFont& baseFont);
ClipTextLayoutSize measureResizeLayout(const ResizeLayoutMeasurementRequest& request);
double steppedFontSize(double current, bool increase);

} // namespace clip_canvas_text_measurement
