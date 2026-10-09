#include "clip_canvas_state.h"
#include "clip_canvas_ffi_handles.h"
#include "clip_canvas_runtime_access.h"
#include "clip_canvas_type_conversions.h"
#include "clip_canvas_viewport.h"
#include "clip_draw_engine_qt/clip_canvas_runtime.h"
#include "clip_draw_engine_qt/clip_canvas_types.h"
#include "clip_draw_engine_qt/clip_canvas_style_edit.h"

#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void filterStyleParticipatesInToolbarStateDiffs() {
    clip_canvas_state::Snapshot previous;
    clip_canvas_state::Snapshot next = previous;
    next.styleToolbarState.filter_style.strength = 0.73;

    const clip_canvas_state::Changes styleChanges =
        clip_canvas_state::diffSnapshots(previous, next);
    require(styleChanges.styleToolbarChanged,
            "filter style changes should invalidate the toolbar state");

    previous = next;
    next.styleToolbarState.filter_style_mixed = 1;
    const clip_canvas_state::Changes mixedChanges =
        clip_canvas_state::diffSnapshots(previous, next);
    require(mixedChanges.styleToolbarChanged,
            "mixed filter properties should invalidate the toolbar state");

    require(!clip_canvas_state::diffSnapshots(next, next).any(),
            "identical snapshots should not emit state synchronization");
    auto brushChanged = next;
    brushChanged.styleToolbarState.brush_eraser_style.stroke_width = 42.0;
    require(clip_canvas_state::diffSnapshots(next, brushChanged).styleToolbarChanged,
            "independent eraser creation width must trigger toolbar synchronization");
    require(clip_canvas_types::toCanvasStyleToolbarState(brushChanged.styleToolbarState)
                    .brushEraserStyle.strokeWidth == 42.0,
            "C/Qt state conversion must preserve independent brush width");
}

template <typename T> void requireEqualPair(const T& value, const char* message) {
    // The independent copy is the behavior under test here.
    const T copy = value; // NOLINT(performance-unnecessary-copy-initialization)
    require(value == copy && !(value != copy), message);
}

template <typename T> void requireUnequalPair(const T& lhs, const T& rhs, const char* message) {
    require(lhs != rhs && !(lhs == rhs), message);
}

void publicCanvasDtosUseExactCompleteEquality() {
    ClipCanvasCornerRadii radii{1.0, 2.0, 3.0, 4.0};
    requireEqualPair(radii, "identical corner radii should compare equal");
    for (int field = 0; field < 4; ++field) {
        ClipCanvasCornerRadii changed = radii;
        double* values[] = {&changed.topLeft, &changed.topRight, &changed.bottomRight,
                            &changed.bottomLeft};
        *values[field] += 0.25;
        requireUnequalPair(radii, changed, "every corner radius should participate");
    }

    ClipCanvasFilterStyle filter;
    requireEqualPair(filter, "identical filters should compare equal");
    auto changedFilter = filter;
    changedFilter.type = ClipCanvasFilterType::GaussianBlur;
    requireUnequalPair(filter, changedFilter, "filter type should participate");
    changedFilter = filter;
    changedFilter.strength += 0.1;
    requireUnequalPair(filter, changedFilter, "filter strength should participate");
    changedFilter = filter;
    changedFilter.opacity -= 0.1;
    requireUnequalPair(filter, changedFilter, "filter opacity should participate");

    ClipCanvasShapeStyle shape;
    shape.fill = QColor(1, 2, 3, 4);
    shape.stroke = QColor(5, 6, 7, 8);
    requireEqualPair(shape, "identical shape styles should compare equal");
#define REQUIRE_SHAPE_CHANGE(field, value)                                                         \
    do {                                                                                           \
        auto changed = shape;                                                                      \
        changed.field = value;                                                                     \
        requireUnequalPair(shape, changed, "shape field should participate");                      \
    } while (false)
    REQUIRE_SHAPE_CHANGE(fill, QColor(9, 2, 3, 4));
    REQUIRE_SHAPE_CHANGE(fillStyle, ClipCanvasFillStyle::CrossLine);
    REQUIRE_SHAPE_CHANGE(stroke, QColor(9, 6, 7, 8));
    REQUIRE_SHAPE_CHANGE(strokeWidth, 2.0);
    REQUIRE_SHAPE_CHANGE(cornerRadii, (ClipCanvasCornerRadii{1, 2, 3, 4}));
    REQUIRE_SHAPE_CHANGE(startArrowhead, ClipCanvasArrowhead::Bar);
    REQUIRE_SHAPE_CHANGE(endArrowhead, ClipCanvasArrowhead::Arrow);
    REQUIRE_SHAPE_CHANGE(strokeStyle, ClipCanvasStrokeStyle::Dashed);
    REQUIRE_SHAPE_CHANGE(arrowRatio, 2.0);
    REQUIRE_SHAPE_CHANGE(arrowType, ClipCanvasArrowType::Curve);
    REQUIRE_SHAPE_CHANGE(opacity, 0.5);
    REQUIRE_SHAPE_CHANGE(highlightShape, ClipCanvasHighlightShape::Ellipse);
    REQUIRE_SHAPE_CHANGE(shape, ClipCanvasRectangleShape::Diamond);
#undef REQUIRE_SHAPE_CHANGE

    ClipCanvasRectangleShapeStyle rectangle;
    requireEqualPair(rectangle, "identical rectangle styles should compare equal");
#define REQUIRE_RECTANGLE_CHANGE(field, value)                                                     \
    do {                                                                                           \
        auto changed = rectangle;                                                                  \
        changed.field = value;                                                                     \
        requireUnequalPair(rectangle, changed, "rectangle field should participate");              \
    } while (false)
    REQUIRE_RECTANGLE_CHANGE(fill, QColor(Qt::red));
    REQUIRE_RECTANGLE_CHANGE(fillStyle, ClipCanvasFillStyle::Line);
    REQUIRE_RECTANGLE_CHANGE(stroke, QColor(Qt::blue));
    REQUIRE_RECTANGLE_CHANGE(strokeWidth, 3.0);
    REQUIRE_RECTANGLE_CHANGE(strokeStyle, ClipCanvasStrokeStyle::Dotted);
    REQUIRE_RECTANGLE_CHANGE(cornerRadii, (ClipCanvasCornerRadii{2, 3, 4, 5}));
#undef REQUIRE_RECTANGLE_CHANGE

    ClipCanvasArrowStyle arrow;
    requireEqualPair(arrow, "identical arrow styles should compare equal");
#define REQUIRE_ARROW_CHANGE(field, value)                                                         \
    do {                                                                                           \
        auto changed = arrow;                                                                      \
        changed.field = value;                                                                     \
        requireUnequalPair(arrow, changed, "arrow field should participate");                      \
    } while (false)
    REQUIRE_ARROW_CHANGE(stroke, QColor(Qt::green));
    REQUIRE_ARROW_CHANGE(strokeWidth, 4.0);
    REQUIRE_ARROW_CHANGE(startArrowhead, ClipCanvasArrowhead::Dot);
    REQUIRE_ARROW_CHANGE(endArrowhead, ClipCanvasArrowhead::Triangle);
    REQUIRE_ARROW_CHANGE(strokeStyle, ClipCanvasStrokeStyle::Dotted);
    REQUIRE_ARROW_CHANGE(arrowRatio, 2.0);
    REQUIRE_ARROW_CHANGE(arrowType, ClipCanvasArrowType::Elbow);
#undef REQUIRE_ARROW_CHANGE

    ClipCanvasTextStyle text;
    requireEqualPair(text, "identical text styles should compare equal");
#define REQUIRE_TEXT_CHANGE(field, value)                                                          \
    do {                                                                                           \
        auto changed = text;                                                                       \
        changed.field = value;                                                                     \
        requireUnequalPair(text, changed, "text field should participate");                        \
    } while (false)
    REQUIRE_TEXT_CHANGE(color, QColor(Qt::blue));
    REQUIRE_TEXT_CHANGE(fontSize, 31.0);
    REQUIRE_TEXT_CHANGE(fontFamily, QStringLiteral("Exact Font"));
    REQUIRE_TEXT_CHANGE(fill, QColor(Qt::yellow));
    REQUIRE_TEXT_CHANGE(fillStyle, ClipCanvasFillStyle::Line);
    REQUIRE_TEXT_CHANGE(stroke, QColor(Qt::black));
    REQUIRE_TEXT_CHANGE(strokeWidth, 1.0);
    REQUIRE_TEXT_CHANGE(cornerRadii, (ClipCanvasCornerRadii{1, 2, 3, 4}));
    REQUIRE_TEXT_CHANGE(horizontalAlign, ClipCanvasTextHorizontalAlign::Right);
    REQUIRE_TEXT_CHANGE(verticalAlign, ClipCanvasTextVerticalAlign::Bottom);
    REQUIRE_TEXT_CHANGE(opacity, 0.75);
#undef REQUIRE_TEXT_CHANGE

    ClipCanvasSerialNumberStyle serial;
    requireEqualPair(serial, "identical serial styles should compare equal");
#define REQUIRE_SERIAL_CHANGE(field, value)                                                        \
    do {                                                                                           \
        auto changed = serial;                                                                     \
        changed.field = value;                                                                     \
        requireUnequalPair(serial, changed, "serial field should participate");                    \
    } while (false)
    REQUIRE_SERIAL_CHANGE(number, 2);
    REQUIRE_SERIAL_CHANGE(type, ClipCanvasSerialNumberType::SolidSquare);
    REQUIRE_SERIAL_CHANGE(numericType, ClipCanvasSerialNumberNumericType::Roman);
    REQUIRE_SERIAL_CHANGE(color, QColor(Qt::cyan));
    REQUIRE_SERIAL_CHANGE(fill, QColor(Qt::magenta));
    REQUIRE_SERIAL_CHANGE(fillStyle, ClipCanvasFillStyle::CrossLine);
    REQUIRE_SERIAL_CHANGE(fontSize, 25.0);
    REQUIRE_SERIAL_CHANGE(fontFamily, QStringLiteral("Exact Serial Font"));
    REQUIRE_SERIAL_CHANGE(strokeWidth, 3.0);
    REQUIRE_SERIAL_CHANGE(strokeStyle, ClipCanvasStrokeStyle::Dashed);
    REQUIRE_SERIAL_CHANGE(opacity, 0.5);
#undef REQUIRE_SERIAL_CHANGE

    ClipCanvasWatermarkConfig watermark;
    requireEqualPair(watermark, "identical watermarks should compare equal");
#define REQUIRE_WATERMARK_CHANGE(field, value)                                                     \
    do {                                                                                           \
        auto changed = watermark;                                                                  \
        changed.field = value;                                                                     \
        requireUnequalPair(watermark, changed, "watermark field should participate");              \
    } while (false)
    REQUIRE_WATERMARK_CHANGE(color, QColor(Qt::white));
    REQUIRE_WATERMARK_CHANGE(text, QStringLiteral("watermark"));
    REQUIRE_WATERMARK_CHANGE(templateValue, QStringLiteral("{text} {YYYY}"));
    REQUIRE_WATERMARK_CHANGE(templateApplicationTime,
                             (ClipCanvasWatermarkTemplateApplicationTime{2026, 9, 15, 12, 34, 56}));
    REQUIRE_WATERMARK_CHANGE(fontSize, 17.0);
    REQUIRE_WATERMARK_CHANGE(fontFamily, QStringLiteral("Exact Watermark Font"));
    REQUIRE_WATERMARK_CHANGE(angle, 31.0);
    REQUIRE_WATERMARK_CHANGE(gap, 57.0);
    REQUIRE_WATERMARK_CHANGE(opacity, 0.17);
#undef REQUIRE_WATERMARK_CHANGE

    ClipCanvasStyleToolbarState toolbar;
    requireEqualPair(toolbar, "identical toolbar states should compare equal");
#define REQUIRE_TOOLBAR_CHANGE(field, value)                                                       \
    do {                                                                                           \
        auto changed = toolbar;                                                                    \
        changed.field = value;                                                                     \
        requireUnequalPair(toolbar, changed, "toolbar field should participate");                  \
    } while (false)
    REQUIRE_TOOLBAR_CHANGE(source, ClipCanvasStyleToolbarSource::SelectedRectangle);
    auto changedToolbar = toolbar;
    changedToolbar.shapeStyle.arrowType = ClipCanvasArrowType::Curve;
    requireUnequalPair(toolbar, changedToolbar, "inactive shape fields should participate");
    changedToolbar = toolbar;
    changedToolbar.textStyle.fontFamily = QStringLiteral("text");
    requireUnequalPair(toolbar, changedToolbar, "text style should participate");
    changedToolbar = toolbar;
    changedToolbar.serialNumberStyle.number = 9;
    requireUnequalPair(toolbar, changedToolbar, "serial style should participate");
    REQUIRE_TOOLBAR_CHANGE(textStyleMixed, 1u);
    REQUIRE_TOOLBAR_CHANGE(canEditArrowText, true);
    REQUIRE_TOOLBAR_CHANGE(serialNumberStyleMixed, 2u);
    REQUIRE_TOOLBAR_CHANGE(shapeStyleMixed, 4u);
    changedToolbar = toolbar;
    changedToolbar.filterStyle.strength = 0.25;
    requireUnequalPair(toolbar, changedToolbar, "filter style should participate");
    REQUIRE_TOOLBAR_CHANGE(filterStyleMixed, 8u);
    REQUIRE_TOOLBAR_CHANGE(brushEraserStyle, (ClipCanvasBrushEraserStyle{42.0}));
#undef REQUIRE_TOOLBAR_CHANGE

    ClipCanvasSerialNumberToolbarState serialToolbar;
    requireEqualPair(serialToolbar, "identical serial-number toolbar states should compare equal");
#define REQUIRE_SERIAL_TOOLBAR_CHANGE(field, value)                                                \
    do {                                                                                           \
        auto changed = serialToolbar;                                                              \
        changed.field = value;                                                                     \
        requireUnequalPair(serialToolbar, changed,                                                 \
                           "serial-number toolbar field should participate");                      \
    } while (false)
    REQUIRE_SERIAL_TOOLBAR_CHANGE(visible, true);
    REQUIRE_SERIAL_TOOLBAR_CHANGE(geometry, QRectF(1, 2, 3, 4));
    REQUIRE_SERIAL_TOOLBAR_CHANGE(canDecrease, true);
    REQUIRE_SERIAL_TOOLBAR_CHANGE(canIncrease, true);
    REQUIRE_SERIAL_TOOLBAR_CHANGE(canCreateText, true);
#undef REQUIRE_SERIAL_TOOLBAR_CHANGE
}

ClipCanvasStyleDefaults customStyleDefaults() {
    ClipCanvasStyleDefaults defaults;
    const QColor transparent(1, 2, 3, 0);
    const QColor ink(17, 34, 51, 128);
    ClipCanvasShapeStyle* shapes[] = {
        &defaults.rectangle,          &defaults.arrow,        &defaults.line, &defaults.freeDraw,
        &defaults.rectangleHighlight, &defaults.penHighlight,
    };
    double width = 3.0;
    for (ClipCanvasShapeStyle* shape : shapes) {
        shape->fill = transparent;
        shape->stroke = ink;
        shape->strokeWidth = width;
        shape->opacity = 0.75;
        width += 1.0;
    }
    defaults.line.arrowType = ClipCanvasArrowType::Straight;
    defaults.rectangleFilter = {
        ClipCanvasFilterType::GaussianBlur,
        0.41,
        0.51,
        9.0,
    };
    defaults.penFilter = {
        ClipCanvasFilterType::Inversion,
        0.42,
        0.52,
        10.0,
    };
    defaults.text.color = ink;
    defaults.text.fill = transparent;
    defaults.text.stroke = QColor(68, 85, 102, 255);
    defaults.text.fontSize = 31.0;
    defaults.text.fontFamily = QStringLiteral("Qt Text Font");
    defaults.serialNumber.color = ink;
    defaults.serialNumber.fill = transparent;
    defaults.serialNumber.type = ClipCanvasSerialNumberType::Circle;
    defaults.serialNumber.fontSize = 25.0;
    defaults.serialNumber.fontFamily = QStringLiteral("Qt Serial Font");
    defaults.watermark.color = ink;
    defaults.watermark.text = QStringLiteral("Qt watermark");
    defaults.watermark.templateValue = QStringLiteral("{text} {YYYY-MM-DD_HH-mm-ss}");
    defaults.watermark.templateApplicationTime =
        ClipCanvasWatermarkTemplateApplicationTime{2026, 9, 15, 12, 34, 56};
    defaults.watermark.fontFamily = QStringLiteral("Qt Watermark Font");
    defaults.watermark.opacity = 0.24;
    defaults.spotlight.color = ink;
    defaults.spotlight.opacity = 0.62;
    defaults.spotlightShape = ClipCanvasRectangleShape::Diamond;
    return defaults;
}

ClipViewport createViewport(ClipCanvasRuntime& runtime) {
    ClipViewport viewport = nullptr;
    const ClipEngineConfig config = clip_canvas_viewport::defaultEngineConfig();
    require(clip_viewport_create(clip_canvas_runtime::Access::handle(runtime), &config,
                                 &viewport) == CLIP_OK &&
                viewport != nullptr,
            "runtime viewport creation should succeed");
    return viewport;
}

ClipStyleToolbarState toolbarState(ClipCanvasRuntime& runtime, ClipViewport viewport,
                                   ClipActiveTool tool) {
    ClipRuntime handle = clip_canvas_runtime::Access::handle(runtime);
    ScopedChangedViewportList changed;
    require(clip_viewport_set_active_tool_ex(handle, viewport, tool, changed.outParam()) == CLIP_OK,
            "configured tool selection should succeed");
    ClipStyleToolbarState state{};
    require(clip_viewport_get_style_toolbar_state(handle, viewport, &state) == CLIP_OK,
            "configured toolbar state query should succeed");
    return state;
}

void configuredRuntimeProfileFollowsRestoreAndResetLifecycle() {
    const ClipCanvasStyleDefaults defaults = customStyleDefaults();
    ClipCanvasRuntimeConfig config;
    config.styleDefaults = defaults;
    ClipCanvasRuntime runtime(config);
    require(runtime.isValid(), "a complete configured runtime should be valid");

    ClipViewport viewport = createViewport(runtime);
    ClipWatermarkConfig engineWatermark{};
    require(clip_viewport_get_watermark_config(clip_canvas_runtime::Access::handle(runtime),
                                               viewport, &engineWatermark) == CLIP_OK &&
                clip_canvas_types::toCanvasWatermarkConfig(engineWatermark) == defaults.watermark,
            "runtime watermark defaults must preserve the template snapshot and time");
    require(toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_SHAPE).shape_style.stroke_width ==
                defaults.rectangle.strokeWidth,
            "rectangle should expose its configured creation style");
    require(toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_SPOTLIGHT).shape_style.shape ==
                CLIP_RECTANGLE_SHAPE_DIAMOND,
            "spotlight should expose its independent configured shape");
    require(toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_ARROW).shape_style.stroke_width ==
                defaults.arrow.strokeWidth,
            "arrow should expose its configured creation style");
    require(toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_LINE).shape_style.stroke_width ==
                defaults.line.strokeWidth,
            "line should expose its configured creation style");
    require(toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_LINE).shape_style.arrow_type ==
                CLIP_ARROW_TYPE_STRAIGHT,
            "line should expose its configured straight type");
    require(toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_FREE_DRAW).shape_style.stroke_width ==
                defaults.freeDraw.strokeWidth,
            "free draw should expose its configured creation style");

    ClipShapeStyle editedLine = toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_LINE).shape_style;
    editedLine.arrow_type = CLIP_ARROW_TYPE_ELBOW;
    ClipChangedViewportList lineChanged = nullptr;
    require(clip_viewport_set_shape_style_patch_ex(clip_canvas_runtime::Access::handle(runtime),
                                                   viewport, &editedLine,
                                                   CLIP_SHAPE_STYLE_PROPERTY_ARROW_TYPE,
                                                   CLIP_SHAPE_KIND_LINE, &lineChanged) == CLIP_OK,
            "line arrow-type patches should be accepted");
    clip_changed_viewports_destroy(lineChanged);
    require(toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_LINE).shape_style.arrow_type ==
                CLIP_ARROW_TYPE_CURVE,
            "unsupported elbow line patches should normalize to curve");

    ClipShapeStyle editedFreeDraw =
        toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_FREE_DRAW).shape_style;
    editedFreeDraw.arrow_type = CLIP_ARROW_TYPE_STRAIGHT;
    require(clip_viewport_set_shape_style_patch_ex(
                clip_canvas_runtime::Access::handle(runtime), viewport, &editedFreeDraw,
                CLIP_SHAPE_STYLE_PROPERTY_ARROW_TYPE, CLIP_SHAPE_KIND_FREE_DRAW,
                nullptr) == CLIP_ERROR_INVALID_ARGUMENT,
            "free draw should reject the line-only arrow-type property");
    require(toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_RECTANGLE_HIGHLIGHT)
                    .shape_style.stroke_width == defaults.rectangleHighlight.strokeWidth,
            "rectangle highlight should expose its configured creation style");
    require(
        toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_PEN_HIGHLIGHT).shape_style.stroke_width ==
            defaults.penHighlight.strokeWidth,
        "pen highlight should expose its configured creation style");
    require(toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_RECTANGLE_FILTER)
                    .filter_style.stroke_width == defaults.rectangleFilter.strokeWidth,
            "rectangle filter should expose its configured creation style");
    require(
        toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_PEN_FILTER).filter_style.stroke_width ==
            defaults.penFilter.strokeWidth,
        "pen filter should expose its configured creation style");
    require(toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_TEXT).text_style.font_size ==
                defaults.text.fontSize,
            "text should expose its configured creation style");
    const ClipSerialNumberStyle serialNumberStyle =
        toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_SERIAL_NUMBER).serial_number_style;
    require(serialNumberStyle.font_size == defaults.serialNumber.fontSize,
            "sequence numbers should expose their configured creation style");
    require(serialNumberStyle.serial_number_type == CLIP_SERIAL_NUMBER_TYPE_CIRCLE,
            "sequence numbers should expose their configured creation type");

    ClipShapeStyle edited = toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_SHAPE).shape_style;
    edited.stroke_width = 41.0;
    ClipChangedViewportList changed = nullptr;
    require(clip_viewport_set_shape_style_patch_ex(clip_canvas_runtime::Access::handle(runtime),
                                                   viewport, &edited,
                                                   CLIP_SHAPE_STYLE_PROPERTY_STROKE_WIDTH,
                                                   CLIP_SHAPE_KIND_RECTANGLE, &changed) == CLIP_OK,
            "editing the configured rectangle style should succeed");
    clip_changed_viewports_destroy(changed);

    const QByteArray fullSession = runtime.serializeDocumentSession();
    const QByteArray documentHistory = runtime.serializeDocumentHistory();
    require(!fullSession.isEmpty() && !documentHistory.isEmpty(),
            "configured runtime serialization should succeed");

    require(runtime.restoreDocumentSession(fullSession),
            "full configured session restore should succeed");
    viewport = createViewport(runtime);
    require(toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_SHAPE).shape_style.stroke_width ==
                41.0,
            "full session restore should retain current editor styles");

    require(runtime.restoreDocumentHistory(documentHistory),
            "configured document-history restore should succeed");
    viewport = createViewport(runtime);
    require(toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_SHAPE).shape_style.stroke_width ==
                defaults.rectangle.strokeWidth,
            "history-only restore should use the target runtime profile");

    edited = toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_SHAPE).shape_style;
    edited.stroke_width = 42.0;
    changed = nullptr;
    require(clip_viewport_set_shape_style_patch_ex(clip_canvas_runtime::Access::handle(runtime),
                                                   viewport, &edited,
                                                   CLIP_SHAPE_STYLE_PROPERTY_STROKE_WIDTH,
                                                   CLIP_SHAPE_KIND_RECTANGLE, &changed) == CLIP_OK,
            "style mutation before clear should succeed");
    clip_changed_viewports_destroy(changed);
    require(runtime.clearDocumentPreservingViewports(),
            "configured clear should preserve attached viewports");
    require(toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_SHAPE).shape_style.stroke_width ==
                42.0,
            "configured clear should preserve the current creation style on the retained viewport");

    require(runtime.reset(), "configured runtime reset should succeed");
    viewport = createViewport(runtime);
    require(toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_SHAPE).shape_style.stroke_width ==
                defaults.rectangle.strokeWidth,
            "configured runtime reset should restore the immutable profile");

    ClipCanvasStyleDefaults elbowLineDefaults = defaults;
    elbowLineDefaults.line.arrowType = ClipCanvasArrowType::Elbow;
    ClipCanvasRuntimeConfig elbowLineConfig;
    elbowLineConfig.styleDefaults = elbowLineDefaults;
    ClipCanvasRuntime elbowLineRuntime(elbowLineConfig);
    require(elbowLineRuntime.isValid(),
            "a configured runtime should accept and normalize an elbow Line default");
    ClipViewport elbowLineViewport = createViewport(elbowLineRuntime);
    require(toolbarState(elbowLineRuntime, elbowLineViewport, CLIP_ACTIVE_TOOL_LINE)
                    .shape_style.arrow_type == CLIP_ARROW_TYPE_CURVE,
            "an unsupported elbow Line default should normalize to curve");

    ClipCanvasStyleDefaults invalid = defaults;
    invalid.text.fill = QColor();
    ClipCanvasRuntimeConfig invalidConfig;
    invalidConfig.styleDefaults = invalid;
    ClipCanvasRuntime rejected(invalidConfig);
    require(!rejected.isValid(), "an invalid configured profile should be rejected atomically");

    invalid = defaults;
    // Runtime configuration must reject values that arrive outside the C++ enum domain.
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    invalid.arrow.arrowType = static_cast<ClipCanvasArrowType>(99);
    invalidConfig.styleDefaults = invalid;
    ClipCanvasRuntime rejectedEnum(invalidConfig);
    require(!rejectedEnum.isValid(),
            "an invalid configured enum should be rejected before C ABI conversion");

    invalid = defaults;
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    invalid.serialNumber.type = static_cast<ClipCanvasSerialNumberType>(99);
    invalidConfig.styleDefaults = invalid;
    ClipCanvasRuntime rejectedSerialType(invalidConfig);
    require(!rejectedSerialType.isValid(),
            "an invalid sequence-number type should be rejected before C ABI conversion");

    invalid = defaults;
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    invalid.spotlightShape = static_cast<ClipCanvasRectangleShape>(99);
    invalidConfig.styleDefaults = invalid;
    ClipCanvasRuntime rejectedSpotlightShape(invalidConfig);
    require(!rejectedSpotlightShape.isValid(),
            "an invalid spotlight shape should be rejected before C ABI conversion");

    ClipCanvasStyleDefaults smartEraseDefaults = defaults;
    smartEraseDefaults.rectangleFilter.type = ClipCanvasFilterType::SmartErase;
    smartEraseDefaults.penFilter.type = ClipCanvasFilterType::SmartErase;
    ClipCanvasRuntimeConfig smartEraseConfig;
    smartEraseConfig.styleDefaults = smartEraseDefaults;
    ClipCanvasRuntime smartEraseRuntime(smartEraseConfig);
    require(smartEraseRuntime.isValid(),
            "a configured runtime with Smart Erase filter defaults should be valid");
    require(smartEraseRuntime.clearDocumentPreservingViewports(),
            "a Smart Erase configured runtime should clear its document");
}

void watermarkConfigurationConversionsPreserveSnapshotsAndUtf8Boundaries() {
    ClipCanvasWatermarkConfig config;
    config.color = QColor(12, 34, 56, 78);
    config.text = QStringLiteral("watermark");
    config.templateValue = QStringLiteral("  {text} @ {YYYY-MM-DD_HH-mm-ss}  ");
    config.templateApplicationTime =
        ClipCanvasWatermarkTemplateApplicationTime{2026, 9, 15, 12, 34, 56};
    config.fontSize = 21.0;
    config.fontFamily = QStringLiteral("Template Font");
    config.angle = -17.0;
    config.gap = 83.0;
    config.opacity = 0.42;

    const ClipWatermarkConfig engine = clip_canvas_types::toEngineWatermarkConfig(config);
    require(clip_canvas_types::toCanvasWatermarkConfig(engine) == config,
            "Qt/C watermark conversion must preserve the complete template snapshot");

    const QString emoji = QString::fromUcs4(U"\U0001F600");
    config.templateValue = emoji.repeated(300);
    const ClipWatermarkConfig truncated = clip_canvas_types::toEngineWatermarkConfig(config);
    const ClipCanvasWatermarkConfig roundTrip =
        clip_canvas_types::toCanvasWatermarkConfig(truncated);
    require(truncated.template_value_utf8_len == 1024 &&
                roundTrip.templateValue == emoji.repeated(256) &&
                !roundTrip.templateValue.contains(QChar::ReplacementCharacter),
            "template ABI truncation must stop on a valid UTF-8 boundary");

    clip_canvas_state::Snapshot before;
    clip_canvas_state::Snapshot after = before;
    after.watermarkConfig = engine;
    require(clip_canvas_state::diffSnapshots(before, after).styleToolbarChanged,
            "template snapshots must participate in C state diff detection");
    before = after;
    after.watermarkConfig.template_application_time.second = 57;
    require(clip_canvas_state::diffSnapshots(before, after).styleToolbarChanged,
            "template application time must participate in C state diff detection");
}

void serialNumberStrokePatchesPreserveOtherProperties() {
    const ClipCanvasSerialNumberStyle original;
    auto requested = original;
    requested.number = 27;
    requested.numericType = ClipCanvasSerialNumberNumericType::Roman;
    requested.strokeWidth = 4.0;
    requested.strokeStyle = ClipCanvasStrokeStyle::Dashed;
    auto actual = original;
    auto expected = original;
    expected.strokeWidth = requested.strokeWidth;
    clipCanvasMergeStyle(actual, requested, ClipCanvasSerialNumberStyleMixedStrokeWidth);
    require(actual == expected, "stroke width patches preserve all other serial properties");
    expected.strokeStyle = requested.strokeStyle;
    clipCanvasMergeStyle(actual, requested, ClipCanvasSerialNumberStyleMixedStrokeStyle);
    require(actual == expected, "stroke style patches preserve all other serial properties");
}

void defaultRuntimeUsesGenericEngineDefaults() {
    ClipStyleDefaults expected{};
    require(clip_runtime_style_defaults_default(&expected) == CLIP_OK,
            "generic C defaults should be available");
    require(expected.spotlight_shape == CLIP_RECTANGLE_SHAPE_RECTANGLE,
            "generic spotlight defaults should remain rectangular");
    ClipCanvasRuntime runtime;
    require(runtime.isValid(), "default runtime creation should be valid");
    ClipViewport viewport = createViewport(runtime);
    const ClipStyleToolbarState state = toolbarState(runtime, viewport, CLIP_ACTIVE_TOOL_SHAPE);
    require(state.shape_style.stroke.r == expected.rectangle.stroke.r &&
                state.shape_style.stroke.g == expected.rectangle.stroke.g &&
                state.shape_style.stroke.b == expected.rectangle.stroke.b &&
                state.shape_style.stroke_width == expected.rectangle.stroke_width,
            "default runtime creation should retain generic engine defaults");
}

void serializedSnapshotsOwnBytesAndPreserveUndoAndRedo() {
    ClipCanvasRuntime runtime;
    const QByteArray annotation =
        R"({"version":1,"operations":[{"type":"rectangle","bounds":[10,20,30,40]}]})";
    require(!runtime.applyAnnotationTransaction(annotation).isEmpty() &&
                !runtime.applyAnnotationTransaction(annotation).isEmpty() && runtime.undo(),
            "create a document with both undo and redo history");
    const QByteArray snapshot = runtime.serializeDocumentSession();
    const auto handle = clip_canvas_runtime::Access::handle(runtime);
    std::size_t size = 0;
    require(clip_runtime_serialize_document_session(handle, nullptr, 0, &size) == CLIP_OK,
            "query legacy snapshot size");
    QByteArray legacy(qsizetype(size), Qt::Uninitialized);
    require(clip_runtime_serialize_document_session(
                handle, reinterpret_cast<std::uint8_t*>(legacy.data()), size, &size) == CLIP_OK &&
                snapshot == legacy,
            "single-serialization snapshots preserve every legacy session byte");
    runtime.destroyAsync();
    ClipCanvasRuntime restored;
    require(restored.restoreDocumentSession(snapshot) && restored.canUndo() && restored.canRedo(),
            "snapshot owns independent bytes after its source runtime is destroyed");
    require(restored.redo() && !restored.canRedo() && restored.undo() && restored.undo() &&
                !restored.canUndo() && restored.redo() && restored.redo() && !restored.canRedo(),
            "restored snapshots retain complete undo and redo transactions");
}
} // namespace

int main() {
    serialNumberStrokePatchesPreserveOtherProperties();
    filterStyleParticipatesInToolbarStateDiffs();
    publicCanvasDtosUseExactCompleteEquality();
    watermarkConfigurationConversionsPreserveSnapshotsAndUtf8Boundaries();
    configuredRuntimeProfileFollowsRestoreAndResetLifecycle();
    defaultRuntimeUsesGenericEngineDefaults();
    serializedSnapshotsOwnBytesAndPreserveUndoAndRedo();
    return 0;
}
