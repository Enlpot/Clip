#include "clip_draw_engine_qt/clip_canvas_style_edit.h"
#include "clip_canvas_text_measurement.h"

#include "clip_canvas_text.h"
#include "clip_canvas_text_layout.h"
#include "clip_canvas_utf8.h"

#include <QSizeF>
#include <QString>
#include <QDataStream>
#include <QIODevice>

#include <algorithm>
#include <cstdint>

namespace clip_canvas_text_measurement {
namespace {

ClipCanvasSceneItem previewItemForStyle(const ClipTextElementInfo& info,
                                        const ClipTextStyle& style) {
    ClipCanvasSceneItem item = clip_canvas_text::defaultPreviewItem(info);
    clip_canvas_text::applyTextStyleToSceneItem(item, style);
    return item;
}

} // namespace

NaturalTextLayoutCache::NaturalTextLayoutCache(int maximumBytes) : m_layouts(maximumBytes) {}

clip_canvas_text_layout::TextMeasuredLayout
NaturalTextLayoutCache::measure(const QString& text, const QFont& baseFont,
                                const ClipSceneDisplayItem& item) {
    const auto resolution = clip_canvas_text_layout::resolveFont(baseFont, item, 1.0);
    QByteArray key;
    QDataStream stream(&key, QIODevice::WriteOnly);
    stream << text << resolution.font << resolution.scale
           << static_cast<quint32>(item.text_horizontal_align);
    if (const auto* cached = m_layouts.object(key)) {
        return *cached;
    }
    const auto measured = clip_canvas_text_layout::measureNaturalTextLayout(text, baseFont, item);
    ++m_measurementCount;
    const qsizetype cost = key.size() + static_cast<qsizetype>(sizeof(measured));
    if (cost <= m_layouts.maxCost()) {
        m_layouts.insert(key, new clip_canvas_text_layout::TextMeasuredLayout(measured), cost);
    }
    return measured;
}

void NaturalTextLayoutCache::clear() {
    m_layouts.clear();
}

qsizetype NaturalTextLayoutCache::retainedBytes() const {
    return m_layouts.totalCost();
}

std::uint64_t NaturalTextLayoutCache::measurementCount() const {
    return m_measurementCount;
}

TextLayoutOverrideMeasurement
measureSelectedAutoResizeLayoutOverrides(const SelectedTextLayoutMeasurementRequest& request) {
    TextLayoutOverrideMeasurement result;
    constexpr std::uint32_t fontProperties =
        CLIP_TEXT_STYLE_MIXED_FONT_SIZE | CLIP_TEXT_STYLE_MIXED_FONT_FAMILY;
    if ((request.properties & fontProperties) == 0) {
        return result;
    }
    if (request.runtime == nullptr || request.viewport == nullptr) {
        result.success = false;
        return result;
    }

    std::uint32_t count = 0;
    if (clip_viewport_get_selected_text_elements(request.runtime, request.viewport, nullptr, 0,
                                                 &count) != CLIP_OK) {
        result.success = false;
        return result;
    }
    if (count == 0) {
        return result;
    }

    std::vector<ClipTextElementInfo> infos(count);
    std::uint32_t writtenCount = 0;
    if (clip_viewport_get_selected_text_elements(request.runtime, request.viewport, infos.data(),
                                                 count, &writtenCount) != CLIP_OK) {
        result.success = false;
        return result;
    }

    return measureAutoResizeLayoutOverrides(infos.data(), qMin(count, writtenCount), request.style,
                                            request.baseFont, request.properties);
}

TextLayoutOverrideMeasurement measureAutoResizeLayoutOverrides(const ClipTextElementInfo* infos,
                                                               std::uint32_t infoCount,
                                                               const ClipTextStyle& style,
                                                               const QFont& baseFont,
                                                               std::uint32_t properties) {
    TextLayoutOverrideMeasurement result;
    if (infos == nullptr && infoCount != 0) {
        result.success = false;
        return result;
    }

    result.layouts.reserve(infoCount);
    for (std::uint32_t index = 0; index < infoCount; ++index) {
        const ClipTextElementInfo& info = infos[index];

        ClipCanvasSceneItem item = clip_canvas_text::defaultPreviewItem(info);
        if ((properties & CLIP_TEXT_STYLE_MIXED_FONT_SIZE) != 0) {
            item.font_size = clip_canvas_text::resolvedTextFontSize(style.font_size);
        }
        if ((properties & CLIP_TEXT_STYLE_MIXED_FONT_FAMILY) != 0) {
            item.setFontFamilyUtf8(clip_canvas_utf8::stringFromField(style.font_family_utf8,
                                                                     style.font_family_utf8_len,
                                                                     CLIP_FONT_FAMILY_UTF8_CAPACITY)
                                       .trimmed()
                                       .toUtf8());
        }
        const QString text = clip_canvas_text::textFromSceneItem(item);
        const clip_canvas_text_layout::TextMeasuredLayout measured =
            info.auto_resize != 0
                ? clip_canvas_text_layout::measureNaturalTextLayout(text, baseFont, item)
                : clip_canvas_text_layout::measureWrappedTextLayout(text, baseFont, item,
                                                                    item.width);
        result.layouts.push_back(ClipTextLayoutOverride{
            info.id,
            ClipTextLayoutSize{
                info.auto_resize != 0 ? measured.layout.width() : qMax(1.0, info.width),
                measured.layout.height(),
                measured.content.width(),
                measured.content.height(),
            },
        });
    }
    return result;
}

ClipTextLayoutSize measureEmptyDraftLayout(const ClipTextStyle& style, const QFont& baseFont) {
    ClipTextElementInfo info{};
    info.font_size = clip_canvas_text::resolvedTextFontSize(style.font_size);
    info.auto_resize = 1;
    ClipCanvasSceneItem item = previewItemForStyle(info, style);
    const clip_canvas_text_layout::TextMeasuredLayout measured =
        clip_canvas_text_layout::measureNaturalTextLayout(QString(), baseFont, item);
    return ClipTextLayoutSize{
        measured.layout.width(),
        measured.layout.height(),
        measured.content.width(),
        measured.content.height(),
    };
}

ClipTextLayoutSize measureSerialLabelLayout(const ClipSerialLabelLayoutRequest& request,
                                            const QFont& baseFont) {
    ClipTextStyle style{};
    style.font_size = request.font_size;
    const QString fontFamily = clip_canvas_utf8::stringFromField(
        request.font_family_utf8, request.font_family_utf8_len, CLIP_FONT_FAMILY_UTF8_CAPACITY);
    clip_canvas_utf8::copyStringToField(fontFamily.trimmed(), style.font_family_utf8,
                                        style.font_family_utf8_len, style.font_family_truncated,
                                        CLIP_FONT_FAMILY_UTF8_CAPACITY);
    return measureEmptyDraftLayout(style, baseFont);
}

ClipTextLayoutSize measureSerialNumberBoundTextLayout(const ClipTextStyle& textStyle,
                                                      const QFont& baseFont) {
    return measureEmptyDraftLayout(textStyle, baseFont);
}

ClipTextLayoutSize measureResizeLayout(const ResizeLayoutMeasurementRequest& request) {
    ClipCanvasSceneItem item = clip_canvas_text::defaultPreviewItem(request.info);
    const QString text = clip_canvas_text::textFromSceneItem(item);
    const double zoom = qMax(0.0001, request.zoom);
    if (request.info.measure_natural_width != 0) {
        const clip_canvas_text_layout::TextMeasuredLayout measured =
            clip_canvas_text_layout::measureNaturalTextLayout(text, request.baseFont, item, zoom);
        return ClipTextLayoutSize{
            qMax(1.0, measured.layout.width()),
            qMax(1.0, measured.layout.height()),
            qMax(1.0, measured.content.width()),
            qMax(1.0, measured.content.height()),
        };
    }

    const double measuredWidth =
        qMax(request.info.width,
             clip_canvas_text_layout::measureMinimumWrappedWidth(request.baseFont, item, zoom));
    const clip_canvas_text_layout::TextMeasuredLayout measured =
        clip_canvas_text_layout::measureWrappedTextLayout(text, request.baseFont, item,
                                                          measuredWidth, zoom);
    return ClipTextLayoutSize{
        qMax(1.0, measuredWidth),
        qMax(1.0, measured.layout.height()),
        qMax(1.0, measured.content.width()),
        qMax(1.0, measured.content.height()),
    };
}

double steppedFontSize(double current, bool increase) {
    const double resolvedCurrent = clip_canvas_text::resolvedTextFontSize(current);
    return std::clamp(resolvedCurrent + (increase ? 1.0 : -1.0),
                      clip_canvas_style_limits::minimumFontSize,
                      clip_canvas_style_limits::maximumTextFontSize);
}

} // namespace clip_canvas_text_measurement
