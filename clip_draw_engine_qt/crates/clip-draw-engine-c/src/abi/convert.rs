mod patch_payloads;

pub(crate) use patch_payloads::*;

use clip_draw_engine::{
    ActiveTool, ArrowPathCommand, ArrowType, Arrowhead, ArrowheadDisplayDashMode,
    ArrowheadDisplayFillMode, ArrowheadDisplayPrimitive, ArrowheadDisplayPrimitiveKind,
    CanvasFilterType, ColorRgba8, CornerRadii, CursorCommand, CursorStyle, DisplayFillStyle,
    DisplayTextHorizontalAlign, DisplayTextVerticalAlign, DistanceStyle, DistanceUnit, ElementId,
    EngineConfig, FillStyle, FilterStyle, GridConfig, HistoryState, InputEvent, InteractionOutput,
    KeyCode, KeyEvent, KeyEventType, Modifiers, Point, PointerButton, PointerButtons,
    PointerCaptureCommand, PointerDevice, PointerEvent, PointerEventType, RectangleShapeStyle,
    RuntimeConfig, SerialNumberStyle, SerialNumberType, ShapeKind, ShapeStyle, SnapConfig,
    SpotlightConfig, StrokeStyle, StyleDefaults, StyleToolbarSource, TextElementInfo,
    TextHorizontalAlign, TextLayoutOverride, TextLayoutSize, TextStyle, TextVerticalAlign, Vector2,
    WatermarkConfig, WatermarkTemplateApplicationTime, WheelDeltaKind, WheelEvent, ZoomFocus,
    normalize_font_family,
};

use crate::abi::text::{
    copy_optional_str_to_c_char_field, copy_str_to_c_char_field, string_from_c_char_field,
};
use crate::abi::types::*;

impl From<ClipColorRgba8> for ColorRgba8 {
    fn from(value: ClipColorRgba8) -> Self {
        Self {
            r: value.r,
            g: value.g,
            b: value.b,
            a: value.a,
        }
    }
}

impl From<ClipWatermarkConfig> for WatermarkConfig {
    fn from(value: ClipWatermarkConfig) -> Self {
        let template_application_time = if value.has_template_application_time != 0 {
            let candidate: WatermarkTemplateApplicationTime =
                value.template_application_time.into();
            candidate.is_valid().then_some(candidate)
        } else {
            None
        };
        Self {
            color: value.color.into(),
            text: string_from_c_char_field(&value.text_utf8, value.text_utf8_len)
                .unwrap_or_default(),
            template_value: string_from_c_char_field(
                &value.template_value_utf8,
                value.template_value_utf8_len,
            )
            .unwrap_or_default(),
            template_application_time,
            font_size: value.font_size,
            font_family: string_from_c_char_field(
                &value.font_family_utf8,
                value.font_family_utf8_len,
            )
            .unwrap_or_default(),
            angle: value.angle,
            gap: value.gap,
            opacity: value.opacity,
        }
        .normalized()
    }
}

impl From<WatermarkConfig> for ClipWatermarkConfig {
    fn from(value: WatermarkConfig) -> Self {
        let template_application_time = value.template_application_time;
        let mut out = Self {
            color: value.color.into(),
            font_size: value.font_size,
            angle: value.angle,
            gap: value.gap,
            opacity: value.opacity,
            has_template_application_time: u8::from(template_application_time.is_some()),
            template_application_time: template_application_time.unwrap_or_default().into(),
            ..Self::default()
        };
        let mut truncated = 0;
        copy_str_to_c_char_field(
            &mut out.text_utf8,
            &mut out.text_utf8_len,
            &mut truncated,
            &value.text,
        );
        copy_str_to_c_char_field(
            &mut out.template_value_utf8,
            &mut out.template_value_utf8_len,
            &mut truncated,
            &value.template_value,
        );
        copy_str_to_c_char_field(
            &mut out.font_family_utf8,
            &mut out.font_family_utf8_len,
            &mut truncated,
            &value.font_family,
        );
        out
    }
}

impl From<ClipWatermarkTemplateApplicationTime> for WatermarkTemplateApplicationTime {
    fn from(value: ClipWatermarkTemplateApplicationTime) -> Self {
        Self {
            year: value.year,
            month: value.month,
            day: value.day,
            hour: value.hour,
            minute: value.minute,
            second: value.second,
        }
    }
}

impl From<WatermarkTemplateApplicationTime> for ClipWatermarkTemplateApplicationTime {
    fn from(value: WatermarkTemplateApplicationTime) -> Self {
        Self {
            year: value.year,
            month: value.month,
            day: value.day,
            hour: value.hour,
            minute: value.minute,
            second: value.second,
            reserved0: [0; 3],
        }
    }
}

impl From<ClipSpotlightConfig> for SpotlightConfig {
    fn from(value: ClipSpotlightConfig) -> Self {
        Self {
            color: value.color.into(),
            opacity: value.opacity,
        }
        .normalized()
    }
}

impl From<SpotlightConfig> for ClipSpotlightConfig {
    fn from(value: SpotlightConfig) -> Self {
        Self {
            color: value.color.into(),
            opacity: value.opacity,
        }
    }
}

impl From<ColorRgba8> for ClipColorRgba8 {
    fn from(value: ColorRgba8) -> Self {
        Self {
            r: value.r,
            g: value.g,
            b: value.b,
            a: value.a,
        }
    }
}

pub(crate) fn clip_arrowhead_from_rust(value: Option<Arrowhead>) -> ClipArrowhead {
    match value {
        None => ClipArrowhead::None,
        Some(Arrowhead::Arrow) => ClipArrowhead::Arrow,
        Some(Arrowhead::Bar) => ClipArrowhead::Bar,
        Some(Arrowhead::Dot) => ClipArrowhead::Dot,
        Some(Arrowhead::Circle) => ClipArrowhead::Circle,
        Some(Arrowhead::CircleOutline) => ClipArrowhead::CircleOutline,
        Some(Arrowhead::IndentedTriangle) => ClipArrowhead::IndentedTriangle,
        Some(Arrowhead::Triangle) => ClipArrowhead::Triangle,
        Some(Arrowhead::TriangleOutline) => ClipArrowhead::TriangleOutline,
        Some(Arrowhead::Diamond) => ClipArrowhead::Diamond,
        Some(Arrowhead::DiamondOutline) => ClipArrowhead::DiamondOutline,
        Some(Arrowhead::CrowfootOne) => ClipArrowhead::CrowfootOne,
        Some(Arrowhead::CrowfootMany) => ClipArrowhead::CrowfootMany,
        Some(Arrowhead::CrowfootOneOrMany) => ClipArrowhead::CrowfootOneOrMany,
        Some(Arrowhead::Square) => ClipArrowhead::Square,
        Some(Arrowhead::InvertedTriangle) => ClipArrowhead::InvertedTriangle,
    }
}

pub(crate) fn clip_arrowhead_to_rust(value: ClipArrowhead) -> Option<Arrowhead> {
    match value {
        ClipArrowhead::None => None,
        ClipArrowhead::Arrow => Some(Arrowhead::Arrow),
        ClipArrowhead::Bar => Some(Arrowhead::Bar),
        ClipArrowhead::Dot => Some(Arrowhead::Dot),
        ClipArrowhead::Circle => Some(Arrowhead::Circle),
        ClipArrowhead::CircleOutline => Some(Arrowhead::CircleOutline),
        ClipArrowhead::IndentedTriangle => Some(Arrowhead::IndentedTriangle),
        ClipArrowhead::Triangle => Some(Arrowhead::Triangle),
        ClipArrowhead::TriangleOutline => Some(Arrowhead::TriangleOutline),
        ClipArrowhead::Diamond => Some(Arrowhead::Diamond),
        ClipArrowhead::DiamondOutline => Some(Arrowhead::DiamondOutline),
        ClipArrowhead::CrowfootOne => Some(Arrowhead::CrowfootOne),
        ClipArrowhead::CrowfootMany => Some(Arrowhead::CrowfootMany),
        ClipArrowhead::CrowfootOneOrMany => Some(Arrowhead::CrowfootOneOrMany),
        ClipArrowhead::Square => Some(Arrowhead::Square),
        ClipArrowhead::InvertedTriangle => Some(Arrowhead::InvertedTriangle),
    }
}

pub(crate) fn clip_stroke_style_from_rust(value: StrokeStyle) -> ClipStrokeStyle {
    match value {
        StrokeStyle::Solid => ClipStrokeStyle::Solid,
        StrokeStyle::Dashed => ClipStrokeStyle::Dashed,
        StrokeStyle::Dotted => ClipStrokeStyle::Dotted,
    }
}

pub(crate) fn clip_stroke_style_to_rust(value: ClipStrokeStyle) -> StrokeStyle {
    match value {
        ClipStrokeStyle::Solid => StrokeStyle::Solid,
        ClipStrokeStyle::Dashed => StrokeStyle::Dashed,
        ClipStrokeStyle::Dotted => StrokeStyle::Dotted,
    }
}

pub(crate) fn clip_arrow_shaft_type_from_rust(
    value: clip_draw_engine_core::arrow::ArrowShaftType,
) -> ClipArrowShaftType {
    match value {
        clip_draw_engine_core::arrow::ArrowShaftType::Plain => ClipArrowShaftType::Plain,
        clip_draw_engine_core::arrow::ArrowShaftType::Tapered => ClipArrowShaftType::Tapered,
    }
}
pub(crate) fn clip_arrow_shaft_type_to_rust(
    value: ClipArrowShaftType,
) -> clip_draw_engine_core::arrow::ArrowShaftType {
    match value {
        ClipArrowShaftType::Plain => clip_draw_engine_core::arrow::ArrowShaftType::Plain,
        ClipArrowShaftType::Tapered => clip_draw_engine_core::arrow::ArrowShaftType::Tapered,
    }
}

pub(crate) fn clip_arrow_type_from_rust(value: ArrowType) -> ClipArrowType {
    match value {
        ArrowType::Straight => ClipArrowType::Straight,
        ArrowType::Curve => ClipArrowType::Curve,
        ArrowType::Elbow => ClipArrowType::Elbow,
    }
}

pub(crate) fn clip_arrow_type_to_rust(value: ClipArrowType) -> ArrowType {
    match value {
        ClipArrowType::Straight => ArrowType::Straight,
        ClipArrowType::Curve => ArrowType::Curve,
        ClipArrowType::Elbow => ArrowType::Elbow,
    }
}

pub(crate) fn clip_shape_kind_to_rust(value: ClipShapeKind) -> ShapeKind {
    match value {
        ClipShapeKind::Rectangle => ShapeKind::Rectangle,
        ClipShapeKind::Arrow => ShapeKind::Arrow,
        ClipShapeKind::Line => ShapeKind::Line,
        ClipShapeKind::FreeDraw => ShapeKind::FreeDraw,
        ClipShapeKind::RectangleHighlight => ShapeKind::RectangleHighlight,
        ClipShapeKind::PenHighlight => ShapeKind::PenHighlight,
        ClipShapeKind::Spotlight => ShapeKind::Spotlight,
    }
}

pub(crate) fn clip_arrow_points_from_rust(points: &[[f64; 2]]) -> Vec<ClipArrowPoint> {
    points
        .iter()
        .map(|point| ClipArrowPoint {
            x: point[0],
            y: point[1],
        })
        .collect()
}

fn clip_arrow_point_from_path_point(point: [f64; 2]) -> ClipArrowPoint {
    ClipArrowPoint {
        x: point[0],
        y: point[1],
    }
}

pub(crate) fn clip_arrow_path_commands_from_rust(
    commands: &[ArrowPathCommand],
) -> Vec<ClipArrowPathCommand> {
    commands
        .iter()
        .map(|command| match *command {
            ArrowPathCommand::MoveTo { point } => ClipArrowPathCommand {
                kind: ClipArrowPathCommandKind::MoveTo,
                point: clip_arrow_point_from_path_point(point),
                ..ClipArrowPathCommand::default()
            },
            ArrowPathCommand::LineTo { point } => ClipArrowPathCommand {
                kind: ClipArrowPathCommandKind::LineTo,
                point: clip_arrow_point_from_path_point(point),
                ..ClipArrowPathCommand::default()
            },
            ArrowPathCommand::QuadTo { control, end } => ClipArrowPathCommand {
                kind: ClipArrowPathCommandKind::QuadTo,
                point: clip_arrow_point_from_path_point(end),
                control1: clip_arrow_point_from_path_point(control),
                ..ClipArrowPathCommand::default()
            },
            ArrowPathCommand::CubicTo {
                control_1,
                control_2,
                end,
            } => ClipArrowPathCommand {
                kind: ClipArrowPathCommandKind::CubicTo,
                point: clip_arrow_point_from_path_point(end),
                control1: clip_arrow_point_from_path_point(control_1),
                control2: clip_arrow_point_from_path_point(control_2),
                ..ClipArrowPathCommand::default()
            },
        })
        .collect()
}

fn clip_arrowhead_primitive_kind_from_rust(
    value: ArrowheadDisplayPrimitiveKind,
) -> ClipArrowheadPrimitiveKind {
    match value {
        ArrowheadDisplayPrimitiveKind::Line => ClipArrowheadPrimitiveKind::Line,
        ArrowheadDisplayPrimitiveKind::Polygon => ClipArrowheadPrimitiveKind::Polygon,
        ArrowheadDisplayPrimitiveKind::Circle => ClipArrowheadPrimitiveKind::Circle,
    }
}

fn clip_arrowhead_fill_mode_from_rust(value: ArrowheadDisplayFillMode) -> ClipArrowheadFillMode {
    match value {
        ArrowheadDisplayFillMode::Stroke => ClipArrowheadFillMode::Stroke,
        ArrowheadDisplayFillMode::Background => ClipArrowheadFillMode::Background,
    }
}

fn clip_arrowhead_dash_mode_from_rust(value: ArrowheadDisplayDashMode) -> ClipArrowheadDashMode {
    match value {
        ArrowheadDisplayDashMode::Inherit => ClipArrowheadDashMode::Inherit,
        ArrowheadDisplayDashMode::Solid => ClipArrowheadDashMode::Solid,
        ArrowheadDisplayDashMode::DottedCap => ClipArrowheadDashMode::DottedCap,
    }
}

fn clip_arrowhead_primitives_from_rust(
    primitives: &[ArrowheadDisplayPrimitive],
) -> Vec<ClipArrowheadPrimitive> {
    primitives
        .iter()
        .map(|primitive| {
            let mut points = [ClipArrowPoint::default(); CLIP_ARROWHEAD_PRIMITIVE_POINT_CAPACITY];
            for (point_index, point) in primitive
                .points
                .iter()
                .take(CLIP_ARROWHEAD_PRIMITIVE_POINT_CAPACITY)
                .enumerate()
            {
                points[point_index] = ClipArrowPoint {
                    x: point[0],
                    y: point[1],
                };
            }
            ClipArrowheadPrimitive {
                kind: clip_arrowhead_primitive_kind_from_rust(primitive.kind),
                fill_mode: clip_arrowhead_fill_mode_from_rust(primitive.fill_mode),
                dash_mode: clip_arrowhead_dash_mode_from_rust(primitive.dash_mode),
                point_count: primitive
                    .points
                    .len()
                    .min(CLIP_ARROWHEAD_PRIMITIVE_POINT_CAPACITY)
                    as u32,
                points,
                center: ClipArrowPoint {
                    x: primitive.center[0],
                    y: primitive.center[1],
                },
                diameter: primitive.diameter,
            }
        })
        .collect()
}

pub(crate) fn clip_text_horizontal_align_from_rust(
    value: DisplayTextHorizontalAlign,
) -> ClipTextHorizontalAlign {
    match value {
        DisplayTextHorizontalAlign::Left => ClipTextHorizontalAlign::Left,
        DisplayTextHorizontalAlign::Center => ClipTextHorizontalAlign::Center,
        DisplayTextHorizontalAlign::Right => ClipTextHorizontalAlign::Right,
    }
}

pub(crate) fn clip_text_vertical_align_from_rust(
    value: DisplayTextVerticalAlign,
) -> ClipTextVerticalAlign {
    match value {
        DisplayTextVerticalAlign::Top => ClipTextVerticalAlign::Top,
        DisplayTextVerticalAlign::Center => ClipTextVerticalAlign::Center,
        DisplayTextVerticalAlign::Bottom => ClipTextVerticalAlign::Bottom,
    }
}

pub(crate) fn clip_fill_style_from_rust(value: DisplayFillStyle) -> ClipFillStyle {
    match value {
        DisplayFillStyle::Line => ClipFillStyle::Line,
        DisplayFillStyle::CrossLine => ClipFillStyle::CrossLine,
        DisplayFillStyle::Solid => ClipFillStyle::Solid,
    }
}

pub(crate) fn clip_fill_style_to_rust(value: ClipFillStyle) -> clip_draw_engine::FillStyle {
    match value {
        ClipFillStyle::Line => clip_draw_engine::FillStyle::Line,
        ClipFillStyle::CrossLine => clip_draw_engine::FillStyle::CrossLine,
        ClipFillStyle::Solid => clip_draw_engine::FillStyle::Solid,
    }
}

pub(crate) fn clip_document_fill_style_from_rust(value: FillStyle) -> ClipFillStyle {
    match value {
        FillStyle::Line => ClipFillStyle::Line,
        FillStyle::CrossLine => ClipFillStyle::CrossLine,
        FillStyle::Solid => ClipFillStyle::Solid,
    }
}

pub(crate) fn clip_text_horizontal_align_to_rust(
    value: ClipTextHorizontalAlign,
) -> TextHorizontalAlign {
    match value {
        ClipTextHorizontalAlign::Left => TextHorizontalAlign::Left,
        ClipTextHorizontalAlign::Center => TextHorizontalAlign::Center,
        ClipTextHorizontalAlign::Right => TextHorizontalAlign::Right,
    }
}

pub(crate) fn clip_document_text_horizontal_align_from_rust(
    value: TextHorizontalAlign,
) -> ClipTextHorizontalAlign {
    match value {
        TextHorizontalAlign::Left => ClipTextHorizontalAlign::Left,
        TextHorizontalAlign::Center => ClipTextHorizontalAlign::Center,
        TextHorizontalAlign::Right => ClipTextHorizontalAlign::Right,
    }
}

pub(crate) fn clip_text_vertical_align_to_rust(value: ClipTextVerticalAlign) -> TextVerticalAlign {
    match value {
        ClipTextVerticalAlign::Top => TextVerticalAlign::Top,
        ClipTextVerticalAlign::Center => TextVerticalAlign::Center,
        ClipTextVerticalAlign::Bottom => TextVerticalAlign::Bottom,
    }
}

pub(crate) fn clip_document_text_vertical_align_from_rust(
    value: TextVerticalAlign,
) -> ClipTextVerticalAlign {
    match value {
        TextVerticalAlign::Top => ClipTextVerticalAlign::Top,
        TextVerticalAlign::Center => ClipTextVerticalAlign::Center,
        TextVerticalAlign::Bottom => ClipTextVerticalAlign::Bottom,
    }
}

pub(crate) fn clip_element_id_from_rust(id: ElementId) -> ClipElementId {
    ClipElementId {
        index: id.index,
        generation: id.generation,
    }
}

pub(crate) fn clip_element_id_to_rust(id: ClipElementId) -> ElementId {
    ElementId {
        index: id.index,
        generation: id.generation,
    }
}

impl From<ClipTextLayoutSize> for TextLayoutSize {
    fn from(value: ClipTextLayoutSize) -> Self {
        Self::with_content(
            value.width,
            value.height,
            value.content_width,
            value.content_height,
        )
    }
}

impl From<ClipTextLayoutOverride> for TextLayoutOverride {
    fn from(value: ClipTextLayoutOverride) -> Self {
        Self {
            id: clip_element_id_to_rust(value.id),
            size: value.size.into(),
        }
    }
}

fn font_family_from_c<const N: usize>(bytes: &[std::ffi::c_char; N], len: u32) -> Option<String> {
    string_from_c_char_field(bytes, len).and_then(|value| normalize_font_family(Some(value)))
}

pub(crate) fn clip_text_element_info_from_rust(value: TextElementInfo) -> ClipTextElementInfo {
    let mut out = ClipTextElementInfo {
        id: clip_element_id_from_rust(value.id),
        arrow_id: value
            .arrow_id
            .map(clip_element_id_from_rust)
            .unwrap_or_default(),
        arrow_width: value.arrow_width,
        center_x: value.center.x,
        center_y: value.center.y,
        width: value.width,
        height: value.height,
        content_width: value.content_width,
        content_height: value.content_height,
        rotation: value.rotation,
        font_size: value.font_size,
        auto_resize: u8::from(value.auto_resize),
        measure_natural_width: u8::from(value.measure_natural_width),
        ..ClipTextElementInfo::default()
    };
    copy_str_to_c_char_field(
        &mut out.text_utf8,
        &mut out.text_utf8_len,
        &mut out.text_truncated,
        &value.text,
    );
    copy_optional_str_to_c_char_field(
        &mut out.font_family_utf8,
        &mut out.font_family_utf8_len,
        &mut out.font_family_truncated,
        value.font_family.as_deref(),
    );
    out
}

impl From<ClipCornerRadii> for CornerRadii {
    fn from(value: ClipCornerRadii) -> Self {
        Self {
            top_left: value.top_left,
            top_right: value.top_right,
            bottom_right: value.bottom_right,
            bottom_left: value.bottom_left,
        }
    }
}

impl From<ClipSnapConfig> for SnapConfig {
    fn from(value: ClipSnapConfig) -> Self {
        Self {
            enabled: value.enabled != 0,
            distance: value.distance,
            enable_point_snaps: value.enable_point_snaps != 0,
            enable_gap_snaps: value.enable_gap_snaps != 0,
            show_guides: value.show_guides != 0,
            show_gap_size: value.show_gap_size != 0,
            line_color: value.line_color.into(),
            line_width: value.line_width,
            marker_size: value.marker_size,
            gap_dash_length: value.gap_dash_length,
            gap_dash_gap: value.gap_dash_gap,
        }
    }
}

impl From<SnapConfig> for ClipSnapConfig {
    fn from(value: SnapConfig) -> Self {
        Self {
            enabled: u8::from(value.enabled),
            enable_point_snaps: u8::from(value.enable_point_snaps),
            enable_gap_snaps: u8::from(value.enable_gap_snaps),
            show_guides: u8::from(value.show_guides),
            show_gap_size: u8::from(value.show_gap_size),
            reserved0: [0; 3],
            distance: value.distance,
            line_color: value.line_color.into(),
            line_width: value.line_width,
            marker_size: value.marker_size,
            gap_dash_length: value.gap_dash_length,
            gap_dash_gap: value.gap_dash_gap,
        }
    }
}

impl From<ClipGridConfig> for GridConfig {
    fn from(value: ClipGridConfig) -> Self {
        Self {
            enabled: value.enabled != 0,
            size: value.size,
        }
    }
}

impl From<GridConfig> for ClipGridConfig {
    fn from(value: GridConfig) -> Self {
        Self {
            enabled: u8::from(value.enabled),
            reserved0: [0; 7],
            size: value.size,
        }
    }
}

impl From<CornerRadii> for ClipCornerRadii {
    fn from(value: CornerRadii) -> Self {
        Self {
            top_left: value.top_left,
            top_right: value.top_right,
            bottom_right: value.bottom_right,
            bottom_left: value.bottom_left,
        }
    }
}

impl From<ClipDistanceStyle> for DistanceStyle {
    fn from(value: ClipDistanceStyle) -> Self {
        Self {
            stroke: value.stroke.into(),
            stroke_width: value.stroke_width,
            factor: value.factor,
            unit: match value.unit {
                ClipDistanceUnit::Px => DistanceUnit::Px,
                ClipDistanceUnit::Cm => DistanceUnit::Cm,
                ClipDistanceUnit::M => DistanceUnit::M,
                ClipDistanceUnit::Km => DistanceUnit::Km,
                ClipDistanceUnit::Mm => DistanceUnit::Mm,
            },
            decimal_places: value.decimal_places.min(255) as u8,
            endpoint_ratio: value.endpoint_scale,
            endpoint_style: clip_arrowhead_to_rust(value.endpoint_style),
        }
    }
}

impl From<DistanceStyle> for ClipDistanceStyle {
    fn from(value: DistanceStyle) -> Self {
        Self {
            stroke: value.stroke.into(),
            stroke_width: value.stroke_width,
            factor: value.factor,
            unit: match value.unit {
                DistanceUnit::Px => ClipDistanceUnit::Px,
                DistanceUnit::Cm => ClipDistanceUnit::Cm,
                DistanceUnit::M => ClipDistanceUnit::M,
                DistanceUnit::Km => ClipDistanceUnit::Km,
                DistanceUnit::Mm => ClipDistanceUnit::Mm,
            },
            decimal_places: value.decimal_places as u32,
            endpoint_scale: value.endpoint_ratio,
            endpoint_style: clip_arrowhead_from_rust(value.endpoint_style),
        }
    }
}

impl Default for ClipDistanceStyle {
    fn default() -> Self {
        DistanceStyle::default().into()
    }
}

impl From<ClipShapeStyle> for ShapeStyle {
    fn from(value: ClipShapeStyle) -> Self {
        Self {
            fill: value.fill.into(),
            fill_style: clip_fill_style_to_rust(value.fill_style),
            stroke: value.stroke.into(),
            stroke_width: value.stroke_width,
            corner_radii: value.corner_radii.into(),
            start_arrowhead: clip_arrowhead_to_rust(value.start_arrowhead),
            end_arrowhead: clip_arrowhead_to_rust(value.end_arrowhead),
            stroke_style: clip_stroke_style_to_rust(value.stroke_style),
            arrow_type: clip_arrow_type_to_rust(value.arrow_type),
            arrow_shaft_type: clip_arrow_shaft_type_to_rust(value.arrow_shaft_type),
            arrow_ratio: clip_draw_engine_core::arrow::normalize_arrow_ratio(value.arrow_ratio),
            opacity: value.opacity,
            highlight_shape: match value.highlight_shape {
                ClipHighlightShape::Rectangle => clip_draw_engine::HighlightShape::Rectangle,
                ClipHighlightShape::Ellipse => clip_draw_engine::HighlightShape::Ellipse,
            },
            shape: match value.shape {
                ClipRectangleShape::Rectangle => clip_draw_engine::HighlightShape::Rectangle,
                ClipRectangleShape::Ellipse => clip_draw_engine::HighlightShape::Ellipse,
                ClipRectangleShape::Diamond => clip_draw_engine::HighlightShape::Diamond,
            },
        }
    }
}

impl From<ClipFilterStyle> for FilterStyle {
    fn from(value: ClipFilterStyle) -> Self {
        Self {
            filter_type: match value.filter_type {
                ClipFilterType::Mosaic => CanvasFilterType::Mosaic,
                ClipFilterType::GaussianBlur => CanvasFilterType::GaussianBlur,
                ClipFilterType::Grayscale => CanvasFilterType::Grayscale,
                ClipFilterType::Inversion => CanvasFilterType::Inversion,
                ClipFilterType::Emboss => CanvasFilterType::Emboss,
                ClipFilterType::Brightness => CanvasFilterType::Brightness,
                ClipFilterType::RestoreBackground => CanvasFilterType::RestoreBackground,
                ClipFilterType::SmartErase => CanvasFilterType::SmartErase,
            },
            strength: value.strength,
            opacity: value.opacity,
            stroke_width: value.stroke_width,
        }
    }
}

impl From<FilterStyle> for ClipFilterStyle {
    fn from(value: FilterStyle) -> Self {
        Self {
            filter_type: match value.filter_type {
                CanvasFilterType::Mosaic => ClipFilterType::Mosaic,
                CanvasFilterType::GaussianBlur => ClipFilterType::GaussianBlur,
                CanvasFilterType::Grayscale => ClipFilterType::Grayscale,
                CanvasFilterType::Inversion => ClipFilterType::Inversion,
                CanvasFilterType::Emboss => ClipFilterType::Emboss,
                CanvasFilterType::Brightness => ClipFilterType::Brightness,
                CanvasFilterType::RestoreBackground => ClipFilterType::RestoreBackground,
                CanvasFilterType::SmartErase => ClipFilterType::SmartErase,
            },
            strength: value.strength,
            opacity: value.opacity,
            stroke_width: value.stroke_width,
        }
    }
}

impl From<ClipRectangleShapeStyle> for RectangleShapeStyle {
    fn from(value: ClipRectangleShapeStyle) -> Self {
        Self {
            fill: value.fill.into(),
            fill_style: clip_fill_style_to_rust(value.fill_style),
            stroke: value.stroke.into(),
            stroke_width: value.stroke_width,
            stroke_style: clip_stroke_style_to_rust(value.stroke_style),
            corner_radii: value.corner_radii.into(),
            shape: clip_draw_engine::HighlightShape::Rectangle,
        }
    }
}

impl From<RectangleShapeStyle> for ClipRectangleShapeStyle {
    fn from(value: RectangleShapeStyle) -> Self {
        Self {
            fill: value.fill.into(),
            fill_style: clip_document_fill_style_from_rust(value.fill_style),
            stroke: value.stroke.into(),
            stroke_width: value.stroke_width,
            stroke_style: clip_stroke_style_from_rust(value.stroke_style),
            corner_radii: value.corner_radii.into(),
        }
    }
}

impl From<ClipArrowStyle> for clip_draw_engine::ArrowStyle {
    fn from(value: ClipArrowStyle) -> Self {
        Self {
            stroke: value.stroke.into(),
            stroke_width: value.stroke_width,
            start_arrowhead: clip_arrowhead_to_rust(value.start_arrowhead),
            end_arrowhead: clip_arrowhead_to_rust(value.end_arrowhead),
            stroke_style: clip_stroke_style_to_rust(value.stroke_style),
            arrow_type: clip_arrow_type_to_rust(value.arrow_type),
            arrow_shaft_type: clip_arrow_shaft_type_to_rust(value.arrow_shaft_type),
            arrow_ratio: clip_draw_engine_core::arrow::normalize_arrow_ratio(value.arrow_ratio),
        }
    }
}

impl From<clip_draw_engine::ArrowStyle> for ClipArrowStyle {
    fn from(value: clip_draw_engine::ArrowStyle) -> Self {
        Self {
            stroke: value.stroke.into(),
            stroke_width: value.stroke_width,
            start_arrowhead: clip_arrowhead_from_rust(value.start_arrowhead),
            end_arrowhead: clip_arrowhead_from_rust(value.end_arrowhead),
            stroke_style: clip_stroke_style_from_rust(value.stroke_style),
            arrow_type: clip_arrow_type_from_rust(value.arrow_type),
            arrow_shaft_type: clip_arrow_shaft_type_from_rust(value.arrow_shaft_type),
            arrow_ratio: clip_draw_engine_core::arrow::normalize_arrow_ratio(value.arrow_ratio),
            reserved0: [0; 4],
        }
    }
}

impl From<ClipTextStyle> for TextStyle {
    fn from(value: ClipTextStyle) -> Self {
        Self {
            color: value.color.into(),
            font_size: value.font_size,
            font_family: font_family_from_c(&value.font_family_utf8, value.font_family_utf8_len),
            fill: value.fill.into(),
            fill_style: clip_fill_style_to_rust(value.fill_style),
            stroke: value.stroke.into(),
            stroke_width: value.stroke_width,
            corner_radii: value.corner_radii.into(),
            horizontal_align: clip_text_horizontal_align_to_rust(value.horizontal_align),
            vertical_align: clip_text_vertical_align_to_rust(value.vertical_align),
            opacity: value.opacity,
        }
    }
}

impl From<TextStyle> for ClipTextStyle {
    fn from(value: TextStyle) -> Self {
        let mut out = Self {
            color: value.color.into(),
            font_size: value.font_size,
            fill: value.fill.into(),
            fill_style: match value.fill_style {
                clip_draw_engine::FillStyle::Line => ClipFillStyle::Line,
                clip_draw_engine::FillStyle::CrossLine => ClipFillStyle::CrossLine,
                clip_draw_engine::FillStyle::Solid => ClipFillStyle::Solid,
            },
            stroke: value.stroke.into(),
            stroke_width: value.stroke_width,
            corner_radii: value.corner_radii.into(),
            horizontal_align: clip_document_text_horizontal_align_from_rust(value.horizontal_align),
            vertical_align: clip_document_text_vertical_align_from_rust(value.vertical_align),
            opacity: value.opacity,
            reserved0: [0; 4],
            font_family_utf8_len: 0,
            font_family_truncated: 0,
            reserved1: [0; 3],
            font_family_utf8: [0; CLIP_FONT_FAMILY_UTF8_CAPACITY],
        };
        copy_optional_str_to_c_char_field(
            &mut out.font_family_utf8,
            &mut out.font_family_utf8_len,
            &mut out.font_family_truncated,
            value.font_family.as_deref(),
        );
        out
    }
}

pub(crate) fn clip_serial_number_numeric_type_to_rust(
    value: ClipSerialNumberNumericType,
) -> clip_draw_engine_document::SerialNumberNumericType {
    use clip_draw_engine_document::SerialNumberNumericType as Numeric;
    match value {
        ClipSerialNumberNumericType::Arabic => Numeric::Arabic,
        ClipSerialNumberNumericType::Roman => Numeric::Roman,
        ClipSerialNumberNumericType::LowercaseLetters => Numeric::LowercaseLetters,
        ClipSerialNumberNumericType::UppercaseLetters => Numeric::UppercaseLetters,
        ClipSerialNumberNumericType::Chinese => Numeric::Chinese,
    }
}

fn clip_serial_number_numeric_type_from_rust(
    value: clip_draw_engine_document::SerialNumberNumericType,
) -> ClipSerialNumberNumericType {
    use clip_draw_engine_document::SerialNumberNumericType as Numeric;
    match value {
        Numeric::Arabic => ClipSerialNumberNumericType::Arabic,
        Numeric::Roman => ClipSerialNumberNumericType::Roman,
        Numeric::LowercaseLetters => ClipSerialNumberNumericType::LowercaseLetters,
        Numeric::UppercaseLetters => ClipSerialNumberNumericType::UppercaseLetters,
        Numeric::Chinese => ClipSerialNumberNumericType::Chinese,
    }
}

pub(crate) fn clip_serial_number_type_to_rust(value: ClipSerialNumberType) -> SerialNumberType {
    match value {
        ClipSerialNumberType::OutlinedCircle => SerialNumberType::OutlinedCircle,
        ClipSerialNumberType::SolidCircle => SerialNumberType::SolidCircle,
        ClipSerialNumberType::OutlinedSquare => SerialNumberType::OutlinedSquare,
        ClipSerialNumberType::SolidSquare => SerialNumberType::SolidSquare,
        ClipSerialNumberType::Circle => SerialNumberType::Circle,
    }
}

impl From<ClipSerialNumberStyle> for SerialNumberStyle {
    fn from(value: ClipSerialNumberStyle) -> Self {
        Self {
            number: value.number.max(0),
            serial_number_type: clip_serial_number_type_to_rust(value.serial_number_type),
            numeric_type: clip_serial_number_numeric_type_to_rust(value.numeric_type),
            color: value.color.into(),
            fill: value.fill.into(),
            fill_style: clip_fill_style_to_rust(value.fill_style),
            font_size: value.font_size,
            font_family: font_family_from_c(&value.font_family_utf8, value.font_family_utf8_len),
            stroke_width: value.stroke_width,
            stroke_style: clip_stroke_style_to_rust(value.stroke_style),
            opacity: value.opacity,
        }
    }
}

impl From<SerialNumberStyle> for ClipSerialNumberStyle {
    fn from(value: SerialNumberStyle) -> Self {
        let mut out = Self {
            number: value.number.max(0),
            numeric_type: clip_serial_number_numeric_type_from_rust(value.numeric_type),
            serial_number_type: match value.serial_number_type {
                SerialNumberType::OutlinedCircle => ClipSerialNumberType::OutlinedCircle,
                SerialNumberType::SolidCircle => ClipSerialNumberType::SolidCircle,
                SerialNumberType::OutlinedSquare => ClipSerialNumberType::OutlinedSquare,
                SerialNumberType::SolidSquare => ClipSerialNumberType::SolidSquare,
                SerialNumberType::Circle => ClipSerialNumberType::Circle,
            },
            color: value.color.into(),
            fill: value.fill.into(),
            fill_style: match value.fill_style {
                clip_draw_engine::FillStyle::Line => ClipFillStyle::Line,
                clip_draw_engine::FillStyle::CrossLine => ClipFillStyle::CrossLine,
                clip_draw_engine::FillStyle::Solid => ClipFillStyle::Solid,
            },
            font_size: value.font_size,
            stroke_width: value.stroke_width,
            stroke_style: clip_stroke_style_from_rust(value.stroke_style),
            opacity: value.opacity,
            font_family_utf8_len: 0,
            font_family_truncated: 0,
            reserved1: [0; 3],
            font_family_utf8: [0; CLIP_FONT_FAMILY_UTF8_CAPACITY],
        };
        copy_optional_str_to_c_char_field(
            &mut out.font_family_utf8,
            &mut out.font_family_utf8_len,
            &mut out.font_family_truncated,
            value.font_family.as_deref(),
        );
        out
    }
}

impl From<ShapeStyle> for ClipShapeStyle {
    fn from(value: ShapeStyle) -> Self {
        Self {
            fill: value.fill.into(),
            fill_style: clip_document_fill_style_from_rust(value.fill_style),
            stroke: value.stroke.into(),
            stroke_width: value.stroke_width,
            corner_radii: value.corner_radii.into(),
            start_arrowhead: clip_arrowhead_from_rust(value.start_arrowhead),
            end_arrowhead: clip_arrowhead_from_rust(value.end_arrowhead),
            stroke_style: clip_stroke_style_from_rust(value.stroke_style),
            arrow_type: clip_arrow_type_from_rust(value.arrow_type),
            arrow_shaft_type: clip_arrow_shaft_type_from_rust(value.arrow_shaft_type),
            arrow_ratio: clip_draw_engine_core::arrow::normalize_arrow_ratio(value.arrow_ratio),
            opacity: value.opacity,
            highlight_shape: match value.highlight_shape {
                clip_draw_engine::HighlightShape::Rectangle => ClipHighlightShape::Rectangle,
                clip_draw_engine::HighlightShape::Ellipse => ClipHighlightShape::Ellipse,
                clip_draw_engine::HighlightShape::Diamond => ClipHighlightShape::Rectangle,
            },
            shape: match value.shape {
                clip_draw_engine::HighlightShape::Rectangle => ClipRectangleShape::Rectangle,
                clip_draw_engine::HighlightShape::Ellipse => ClipRectangleShape::Ellipse,
                clip_draw_engine::HighlightShape::Diamond => ClipRectangleShape::Diamond,
            },
        }
    }
}

fn strict_string_from_c_char_field<const N: usize>(
    bytes: &[std::ffi::c_char; N],
    len: u32,
) -> Result<String, ClipError> {
    if len as usize > N {
        return Err(ClipError::InvalidArgument);
    }
    string_from_c_char_field(bytes, len).ok_or(ClipError::InvalidArgument)
}

pub(crate) unsafe fn raw_c_enum_is_valid<E: crate::abi::raw_enum::ClipRawEnum>(
    value: *const E,
) -> bool {
    let raw = unsafe { std::ptr::read_unaligned(value.cast::<i32>()) };
    E::from_raw(raw).is_some()
}

unsafe fn runtime_style_default_enums_are_valid(defaults: *const ClipStyleDefaults) -> bool {
    let shapes = [
        unsafe { std::ptr::addr_of!((*defaults).rectangle) },
        unsafe { std::ptr::addr_of!((*defaults).arrow) },
        unsafe { std::ptr::addr_of!((*defaults).line) },
        unsafe { std::ptr::addr_of!((*defaults).free_draw) },
        unsafe { std::ptr::addr_of!((*defaults).rectangle_highlight) },
        unsafe { std::ptr::addr_of!((*defaults).pen_highlight) },
    ];
    for shape in shapes {
        if !unsafe {
            raw_c_enum_is_valid(std::ptr::addr_of!((*shape).fill_style))
                && raw_c_enum_is_valid(std::ptr::addr_of!((*shape).start_arrowhead))
                && raw_c_enum_is_valid(std::ptr::addr_of!((*shape).end_arrowhead))
                && raw_c_enum_is_valid(std::ptr::addr_of!((*shape).stroke_style))
                && raw_c_enum_is_valid(std::ptr::addr_of!((*shape).arrow_type))
                && raw_c_enum_is_valid(std::ptr::addr_of!((*shape).arrow_shaft_type))
                && raw_c_enum_is_valid(std::ptr::addr_of!((*shape).highlight_shape))
                && raw_c_enum_is_valid(std::ptr::addr_of!((*shape).shape))
        } {
            return false;
        }
    }

    unsafe {
        raw_c_enum_is_valid(std::ptr::addr_of!((*defaults).distance.unit))
            && raw_c_enum_is_valid(std::ptr::addr_of!((*defaults).distance.endpoint_style))
            && (*defaults).distance.decimal_places <= 3
            && raw_c_enum_is_valid(std::ptr::addr_of!((*defaults).spotlight_shape))
            && raw_c_enum_is_valid(std::ptr::addr_of!((*defaults).rectangle_filter.filter_type))
            && raw_c_enum_is_valid(std::ptr::addr_of!((*defaults).pen_filter.filter_type))
            && raw_c_enum_is_valid(std::ptr::addr_of!((*defaults).text.fill_style))
            && raw_c_enum_is_valid(std::ptr::addr_of!((*defaults).text.horizontal_align))
            && raw_c_enum_is_valid(std::ptr::addr_of!((*defaults).text.vertical_align))
            && raw_c_enum_is_valid(std::ptr::addr_of!((*defaults).serial_number.numeric_type))
            && raw_c_enum_is_valid(std::ptr::addr_of!((*defaults).serial_number.fill_style))
            && raw_c_enum_is_valid(std::ptr::addr_of!((*defaults).serial_number.stroke_style))
            && raw_c_enum_is_valid(std::ptr::addr_of!(
                (*defaults).serial_number.serial_number_type
            ))
    }
}

pub(crate) fn runtime_config_from_c(
    config: Option<&ClipRuntimeConfig>,
) -> Result<RuntimeConfig, ClipError> {
    let Some(config) = config else {
        return Ok(RuntimeConfig::default());
    };
    if config.style_defaults.is_null() {
        return Ok(RuntimeConfig::default());
    }
    if !unsafe { runtime_style_default_enums_are_valid(config.style_defaults) } {
        return Err(ClipError::InvalidArgument);
    }
    let defaults = unsafe { &*config.style_defaults };
    let rectangle: ShapeStyle = defaults.rectangle.into();
    let arrow: ShapeStyle = defaults.arrow.into();
    if defaults.text.font_family_truncated != 0 || defaults.serial_number.font_family_truncated != 0
    {
        return Err(ClipError::InvalidArgument);
    }
    let text_font_family = strict_string_from_c_char_field(
        &defaults.text.font_family_utf8,
        defaults.text.font_family_utf8_len,
    )?;
    let serial_font_family = strict_string_from_c_char_field(
        &defaults.serial_number.font_family_utf8,
        defaults.serial_number.font_family_utf8_len,
    )?;
    let watermark_text = strict_string_from_c_char_field(
        &defaults.watermark.text_utf8,
        defaults.watermark.text_utf8_len,
    )?;
    let watermark_template_value = strict_string_from_c_char_field(
        &defaults.watermark.template_value_utf8,
        defaults.watermark.template_value_utf8_len,
    )?;
    let watermark_font_family = strict_string_from_c_char_field(
        &defaults.watermark.font_family_utf8,
        defaults.watermark.font_family_utf8_len,
    )?;
    Ok(RuntimeConfig {
        style_defaults: StyleDefaults {
            editor: clip_draw_engine::EditorStyleDefaults {
                spotlight_shape: match defaults.spotlight_shape {
                    ClipRectangleShape::Rectangle => clip_draw_engine::HighlightShape::Rectangle,
                    ClipRectangleShape::Ellipse => clip_draw_engine::HighlightShape::Ellipse,
                    ClipRectangleShape::Diamond => clip_draw_engine::HighlightShape::Diamond,
                },
                rectangle: rectangle.rectangle_shape_style(),
                arrow: arrow.arrow_style(),
                line: defaults.line.into(),
                free_draw: defaults.free_draw.into(),
                rectangle_highlight: defaults.rectangle_highlight.into(),
                pen_highlight: defaults.pen_highlight.into(),
                rectangle_filter: defaults.rectangle_filter.into(),
                pen_filter: defaults.pen_filter.into(),
                brush_eraser: defaults.brush_eraser.into(),
                distance: defaults.distance.into(),
                text: TextStyle {
                    color: defaults.text.color.into(),
                    font_size: defaults.text.font_size,
                    font_family: normalize_font_family(Some(text_font_family)),
                    fill: defaults.text.fill.into(),
                    fill_style: clip_fill_style_to_rust(defaults.text.fill_style),
                    stroke: defaults.text.stroke.into(),
                    stroke_width: defaults.text.stroke_width,
                    corner_radii: defaults.text.corner_radii.into(),
                    horizontal_align: clip_text_horizontal_align_to_rust(
                        defaults.text.horizontal_align,
                    ),
                    vertical_align: clip_text_vertical_align_to_rust(defaults.text.vertical_align),
                    opacity: defaults.text.opacity,
                },
                serial_number: SerialNumberStyle {
                    number: defaults.serial_number.number,
                    numeric_type: clip_serial_number_numeric_type_to_rust(
                        defaults.serial_number.numeric_type,
                    ),
                    serial_number_type: match defaults.serial_number.serial_number_type {
                        ClipSerialNumberType::OutlinedCircle => SerialNumberType::OutlinedCircle,
                        ClipSerialNumberType::SolidCircle => SerialNumberType::SolidCircle,
                        ClipSerialNumberType::OutlinedSquare => SerialNumberType::OutlinedSquare,
                        ClipSerialNumberType::SolidSquare => SerialNumberType::SolidSquare,
                        ClipSerialNumberType::Circle => SerialNumberType::Circle,
                    },
                    color: defaults.serial_number.color.into(),
                    fill: defaults.serial_number.fill.into(),
                    fill_style: clip_fill_style_to_rust(defaults.serial_number.fill_style),
                    font_size: defaults.serial_number.font_size,
                    font_family: normalize_font_family(Some(serial_font_family)),
                    stroke_width: defaults.serial_number.stroke_width,
                    stroke_style: clip_stroke_style_to_rust(defaults.serial_number.stroke_style),
                    opacity: defaults.serial_number.opacity,
                },
            },
            watermark: WatermarkConfig {
                color: defaults.watermark.color.into(),
                text: watermark_text,
                template_value: watermark_template_value,
                template_application_time: if defaults.watermark.has_template_application_time != 0
                {
                    let candidate: WatermarkTemplateApplicationTime =
                        defaults.watermark.template_application_time.into();
                    candidate.is_valid().then_some(candidate)
                } else {
                    None
                },
                font_size: defaults.watermark.font_size,
                font_family: watermark_font_family,
                angle: defaults.watermark.angle,
                gap: defaults.watermark.gap,
                opacity: defaults.watermark.opacity,
            },
            spotlight: SpotlightConfig {
                color: defaults.spotlight.color.into(),
                opacity: defaults.spotlight.opacity,
            },
        },
    })
}

impl From<StyleDefaults> for ClipStyleDefaults {
    fn from(value: StyleDefaults) -> Self {
        let rectangle = value.editor.rectangle;
        let arrow = value.editor.arrow;
        let rectangle_shape = ClipShapeStyle {
            fill: rectangle.fill.into(),
            fill_style: clip_document_fill_style_from_rust(rectangle.fill_style),
            stroke: rectangle.stroke.into(),
            stroke_width: rectangle.stroke_width,
            stroke_style: clip_stroke_style_from_rust(match rectangle.stroke_style {
                StrokeStyle::Solid => StrokeStyle::Solid,
                StrokeStyle::Dashed => StrokeStyle::Dashed,
                StrokeStyle::Dotted => StrokeStyle::Dotted,
            }),
            corner_radii: rectangle.corner_radii.into(),
            shape: match rectangle.shape {
                clip_draw_engine::HighlightShape::Rectangle => ClipRectangleShape::Rectangle,
                clip_draw_engine::HighlightShape::Ellipse => ClipRectangleShape::Ellipse,
                clip_draw_engine::HighlightShape::Diamond => ClipRectangleShape::Diamond,
            },
            ..ClipShapeStyle::default()
        };
        let arrow_shape = ClipShapeStyle {
            stroke: arrow.stroke.into(),
            stroke_width: arrow.stroke_width,
            start_arrowhead: clip_arrowhead_from_rust(arrow.start_arrowhead),
            end_arrowhead: clip_arrowhead_from_rust(arrow.end_arrowhead),
            stroke_style: clip_stroke_style_from_rust(arrow.stroke_style),
            arrow_type: clip_arrow_type_from_rust(arrow.arrow_type),
            arrow_shaft_type: clip_arrow_shaft_type_from_rust(arrow.arrow_shaft_type),
            arrow_ratio: clip_draw_engine_core::arrow::normalize_arrow_ratio(arrow.arrow_ratio),
            ..ClipShapeStyle::default()
        };
        Self {
            rectangle: rectangle_shape,
            arrow: arrow_shape,
            line: value.editor.line.into(),
            free_draw: value.editor.free_draw.into(),
            rectangle_highlight: value.editor.rectangle_highlight.into(),
            pen_highlight: value.editor.pen_highlight.into(),
            rectangle_filter: value.editor.rectangle_filter.into(),
            pen_filter: value.editor.pen_filter.into(),
            text: value.editor.text.into(),
            serial_number: value.editor.serial_number.into(),
            watermark: value.watermark.into(),
            spotlight: value.spotlight.into(),
            brush_eraser: value.editor.brush_eraser.into(),
            distance: value.editor.distance.into(),
            spotlight_shape: match value.editor.spotlight_shape {
                clip_draw_engine::HighlightShape::Rectangle => ClipRectangleShape::Rectangle,
                clip_draw_engine::HighlightShape::Ellipse => ClipRectangleShape::Ellipse,
                clip_draw_engine::HighlightShape::Diamond => ClipRectangleShape::Diamond,
            },
        }
    }
}

impl Default for ClipStyleToolbarState {
    fn default() -> Self {
        Self {
            source: ClipStyleToolbarSource::DefaultRectangle,
            selected_element_count: 0,
            shape_style: ClipShapeStyle::default(),
            text_style: ClipTextStyle::default(),
            serial_number_style: ClipSerialNumberStyle::default(),
            text_style_mixed: 0,
            serial_number_style_mixed: 0,
            shape_style_mixed: 0,
            filter_style: ClipFilterStyle::default(),
            filter_style_mixed: 0,
            brush_eraser_style: ClipBrushEraserStyle::default(),
            distance_style: ClipDistanceStyle::default(),
            distance_style_mixed: 0,
        }
    }
}

impl From<HistoryState> for ClipHistoryState {
    fn from(value: HistoryState) -> Self {
        Self {
            can_undo: u8::from(value.can_undo),
            can_redo: u8::from(value.can_redo),
            reserved0: [0; 6],
        }
    }
}

pub(crate) fn clip_engine_config_to_rust(value: &ClipEngineConfig) -> EngineConfig {
    let zoom_focus = match value.zoom_focus {
        ClipZoomFocus::Pointer => ZoomFocus::Pointer,
        ClipZoomFocus::Center => ZoomFocus::Center,
    };
    EngineConfig {
        min_zoom: value.min_zoom,
        max_zoom: value.max_zoom,
        zoom_focus,
        wheel_zoom_sensitivity: value.wheel_zoom_sensitivity,
        clear_color: value.clear_color.into(),
        snap: value.snap.into(),
        grid: value.grid.into(),
        enable_pointer_capture: value.enable_pointer_capture != 0,
    }
}

pub(crate) fn clip_active_tool_to_rust(value: ClipActiveTool) -> ActiveTool {
    match value {
        ClipActiveTool::Select => ActiveTool::Select,
        ClipActiveTool::Shape => ActiveTool::Shape,
        ClipActiveTool::Arrow => ActiveTool::Arrow,
        ClipActiveTool::Line => ActiveTool::Line,
        ClipActiveTool::FreeDraw => ActiveTool::FreeDraw,
        ClipActiveTool::RectangleHighlight => ActiveTool::RectangleHighlight,
        ClipActiveTool::PenHighlight => ActiveTool::PenHighlight,
        ClipActiveTool::Eraser => ActiveTool::Eraser,
        ClipActiveTool::RectangleFilter => ActiveTool::RectangleFilter,
        ClipActiveTool::PenFilter => ActiveTool::PenFilter,
        ClipActiveTool::Spotlight => ActiveTool::Spotlight,
        ClipActiveTool::AutoFilter => ActiveTool::AutoFilter,
        ClipActiveTool::RectangleEraser => ActiveTool::RectangleEraser,
        ClipActiveTool::BrushEraser => ActiveTool::BrushEraser,
        ClipActiveTool::Distance => ActiveTool::Distance,
        ClipActiveTool::Watermark => ActiveTool::Watermark,
        ClipActiveTool::Text => ActiveTool::Text,
        ClipActiveTool::SerialNumber => ActiveTool::SerialNumber,
    }
}

pub(crate) fn clip_active_tool_from_rust(value: ActiveTool) -> ClipActiveTool {
    match value {
        ActiveTool::Select => ClipActiveTool::Select,
        ActiveTool::Shape => ClipActiveTool::Shape,
        ActiveTool::Arrow => ClipActiveTool::Arrow,
        ActiveTool::Line => ClipActiveTool::Line,
        ActiveTool::FreeDraw => ClipActiveTool::FreeDraw,
        ActiveTool::RectangleHighlight => ClipActiveTool::RectangleHighlight,
        ActiveTool::PenHighlight => ClipActiveTool::PenHighlight,
        ActiveTool::Eraser => ClipActiveTool::Eraser,
        ActiveTool::RectangleFilter => ClipActiveTool::RectangleFilter,
        ActiveTool::PenFilter => ClipActiveTool::PenFilter,
        ActiveTool::Spotlight => ClipActiveTool::Spotlight,
        ActiveTool::AutoFilter => ClipActiveTool::AutoFilter,
        ActiveTool::RectangleEraser => ClipActiveTool::RectangleEraser,
        ActiveTool::BrushEraser => ClipActiveTool::BrushEraser,
        ActiveTool::Distance => ClipActiveTool::Distance,
        ActiveTool::Watermark => ClipActiveTool::Watermark,
        ActiveTool::Text => ClipActiveTool::Text,
        ActiveTool::SerialNumber => ClipActiveTool::SerialNumber,
    }
}

pub(crate) fn clip_active_tool_mask_to_rust(value: u64) -> u64 {
    const TOOLS: [ClipActiveTool; 18] = [
        ClipActiveTool::Select,
        ClipActiveTool::Shape,
        ClipActiveTool::Arrow,
        ClipActiveTool::Text,
        ClipActiveTool::SerialNumber,
        ClipActiveTool::Line,
        ClipActiveTool::FreeDraw,
        ClipActiveTool::RectangleHighlight,
        ClipActiveTool::Eraser,
        ClipActiveTool::RectangleFilter,
        ClipActiveTool::Watermark,
        ClipActiveTool::PenHighlight,
        ClipActiveTool::PenFilter,
        ClipActiveTool::Spotlight,
        ClipActiveTool::AutoFilter,
        ClipActiveTool::RectangleEraser,
        ClipActiveTool::BrushEraser,
        ClipActiveTool::Distance,
    ];

    TOOLS.into_iter().fold(0, |mask, tool| {
        if value & (1_u64 << tool as u32) == 0 {
            mask
        } else {
            mask | clip_active_tool_to_rust(tool).policy_bit()
        }
    })
}

pub(crate) fn clip_style_toolbar_source_from_rust(
    value: StyleToolbarSource,
) -> ClipStyleToolbarSource {
    match value {
        StyleToolbarSource::DefaultRectangle => ClipStyleToolbarSource::DefaultRectangle,
        StyleToolbarSource::SelectedRectangle => ClipStyleToolbarSource::SelectedRectangle,
        StyleToolbarSource::DefaultArrow => ClipStyleToolbarSource::DefaultArrow,
        StyleToolbarSource::SelectedArrow => ClipStyleToolbarSource::SelectedArrow,
        StyleToolbarSource::DefaultLine => ClipStyleToolbarSource::DefaultLine,
        StyleToolbarSource::SelectedLine => ClipStyleToolbarSource::SelectedLine,
        StyleToolbarSource::DefaultFreeDraw => ClipStyleToolbarSource::DefaultFreeDraw,
        StyleToolbarSource::SelectedFreeDraw => ClipStyleToolbarSource::SelectedFreeDraw,
        StyleToolbarSource::DefaultRectangleHighlight => {
            ClipStyleToolbarSource::DefaultRectangleHighlight
        }
        StyleToolbarSource::SelectedRectangleHighlight => {
            ClipStyleToolbarSource::SelectedRectangleHighlight
        }
        StyleToolbarSource::DefaultPenHighlight => ClipStyleToolbarSource::DefaultPenHighlight,
        StyleToolbarSource::SelectedPenHighlight => ClipStyleToolbarSource::SelectedPenHighlight,
        StyleToolbarSource::Eraser => ClipStyleToolbarSource::Eraser,
        StyleToolbarSource::DefaultRectangleEraser => {
            ClipStyleToolbarSource::DefaultRectangleEraser
        }
        StyleToolbarSource::DefaultBrushEraser => ClipStyleToolbarSource::DefaultBrushEraser,
        StyleToolbarSource::DefaultDistance => ClipStyleToolbarSource::DefaultDistance,
        StyleToolbarSource::SelectedDistance => ClipStyleToolbarSource::SelectedDistance,
        StyleToolbarSource::DefaultRectangleFilter => {
            ClipStyleToolbarSource::DefaultRectangleFilter
        }
        StyleToolbarSource::SelectedRectangleFilter => {
            ClipStyleToolbarSource::SelectedRectangleFilter
        }
        StyleToolbarSource::DefaultPenFilter => ClipStyleToolbarSource::DefaultPenFilter,
        StyleToolbarSource::SelectedPenFilter => ClipStyleToolbarSource::SelectedPenFilter,
        StyleToolbarSource::DefaultSpotlight => ClipStyleToolbarSource::DefaultSpotlight,
        StyleToolbarSource::SelectedSpotlight => ClipStyleToolbarSource::SelectedSpotlight,
        StyleToolbarSource::Watermark => ClipStyleToolbarSource::Watermark,
        StyleToolbarSource::DefaultText => ClipStyleToolbarSource::DefaultText,
        StyleToolbarSource::SelectedText => ClipStyleToolbarSource::SelectedText,
        StyleToolbarSource::DefaultSerialNumber => ClipStyleToolbarSource::DefaultSerialNumber,
        StyleToolbarSource::SelectedSerialNumber => ClipStyleToolbarSource::SelectedSerialNumber,
    }
}

pub(crate) fn clip_pointer_event_to_rust(value: &ClipPointerEvent) -> PointerEvent {
    let event_type = match value.event_type {
        ClipPointerEventType::Down => PointerEventType::Down,
        ClipPointerEventType::Move => PointerEventType::Move,
        ClipPointerEventType::Up => PointerEventType::Up,
        ClipPointerEventType::Cancel => PointerEventType::Cancel,
        ClipPointerEventType::Enter => PointerEventType::Enter,
        ClipPointerEventType::Leave => PointerEventType::Leave,
        ClipPointerEventType::DoubleClick => PointerEventType::DoubleClick,
    };
    let device = match value.device {
        ClipPointerDevice::Mouse => PointerDevice::Mouse,
        ClipPointerDevice::Touch => PointerDevice::Touch,
        ClipPointerDevice::Pen => PointerDevice::Pen,
        ClipPointerDevice::Unknown => PointerDevice::Unknown,
    };
    let button = match value.button {
        ClipPointerButton::None => None,
        ClipPointerButton::Primary => Some(PointerButton::Primary),
        ClipPointerButton::Secondary => Some(PointerButton::Secondary),
        ClipPointerButton::Middle => Some(PointerButton::Middle),
    };

    PointerEvent {
        pointer_id: value.pointer_id,
        event_type,
        device,
        position: Point {
            x: value.position_x,
            y: value.position_y,
        },
        button,
        buttons: PointerButtons(value.buttons),
        modifiers: clip_modifiers_to_rust(value.modifiers),
    }
}

pub(crate) fn clip_input_event_to_rust(value: &ClipInputEvent) -> Result<InputEvent, ClipError> {
    match value.kind {
        ClipInputEventKind::Pointer => Ok(InputEvent::Pointer(clip_pointer_event_to_rust(
            &value.pointer,
        ))),
        ClipInputEventKind::Wheel => Ok(InputEvent::Wheel(clip_wheel_event_to_rust(&value.wheel))),
        ClipInputEventKind::Key => clip_key_event_to_rust(&value.key).map(InputEvent::Key),
        ClipInputEventKind::FocusLost => Ok(InputEvent::FocusLost),
    }
}

pub(crate) fn clip_wheel_event_to_rust(value: &ClipWheelEvent) -> WheelEvent {
    let delta_kind = match value.delta_kind {
        ClipWheelDeltaKind::Pixel => WheelDeltaKind::Pixel,
        ClipWheelDeltaKind::Angle => WheelDeltaKind::Angle,
    };

    WheelEvent {
        position: Point {
            x: value.position_x,
            y: value.position_y,
        },
        delta: Vector2 {
            x: value.delta_x,
            y: value.delta_y,
        },
        delta_kind,
        modifiers: clip_modifiers_to_rust(value.modifiers),
    }
}

pub(crate) fn clip_key_event_to_rust(value: &ClipKeyEvent) -> Result<KeyEvent, ClipError> {
    let event_type = match value.event_type {
        ClipKeyEventType::KeyDown => KeyEventType::KeyDown,
        ClipKeyEventType::KeyUp => KeyEventType::KeyUp,
    };
    let key_code = match value.key_code {
        ClipKeyCode::Unknown => KeyCode::Unknown,
        ClipKeyCode::Space => KeyCode::Space,
        ClipKeyCode::Escape => KeyCode::Escape,
        ClipKeyCode::ArrowUp => KeyCode::ArrowUp,
        ClipKeyCode::ArrowDown => KeyCode::ArrowDown,
        ClipKeyCode::ArrowLeft => KeyCode::ArrowLeft,
        ClipKeyCode::ArrowRight => KeyCode::ArrowRight,
        ClipKeyCode::Backspace => KeyCode::Backspace,
        ClipKeyCode::Delete => KeyCode::Delete,
        ClipKeyCode::Character => {
            let ch = char::from_u32(value.codepoint).ok_or(ClipError::InvalidArgument)?;
            KeyCode::Character(ch)
        }
    };

    Ok(KeyEvent {
        event_type,
        key_code,
        modifiers: clip_modifiers_to_rust(value.modifiers),
        repeat: value.repeat != 0,
    })
}

pub(crate) fn clip_modifiers_to_rust(value: ClipModifiers) -> Modifiers {
    Modifiers {
        ctrl: value.ctrl != 0,
        shift: value.shift != 0,
        alt: value.alt != 0,
        meta: value.meta != 0,
    }
}

pub(crate) fn clip_interaction_output_from_rust(value: InteractionOutput) -> ClipInteractionOutput {
    let (capture_kind, capture_pointer_id) = match value.capture {
        PointerCaptureCommand::NoChange => (ClipPointerCaptureCommandKind::NoChange, 0),
        PointerCaptureCommand::Capture(pointer_id) => {
            (ClipPointerCaptureCommandKind::Capture, pointer_id)
        }
        PointerCaptureCommand::Release => (ClipPointerCaptureCommandKind::Release, 0),
    };
    let (cursor_kind, cursor_style) = match value.cursor {
        CursorCommand::NoChange => (ClipCursorCommandKind::NoChange, ClipCursorStyle::Default),
        CursorCommand::Set(style) => (
            ClipCursorCommandKind::Set,
            clip_cursor_style_from_rust(style),
        ),
    };

    ClipInteractionOutput {
        consumed: u8::from(value.consumed),
        reserved0: [0; 3],
        capture_kind,
        capture_pointer_id,
        cursor_kind,
        cursor_style,
    }
}

pub(crate) fn clip_cursor_style_from_rust(value: CursorStyle) -> ClipCursorStyle {
    match value {
        CursorStyle::Default => ClipCursorStyle::Default,
        CursorStyle::Crosshair => ClipCursorStyle::Crosshair,
        CursorStyle::Grab => ClipCursorStyle::Grab,
        CursorStyle::Grabbing => ClipCursorStyle::Grabbing,
        CursorStyle::Move => ClipCursorStyle::Move,
        CursorStyle::ResizeHorizontal => ClipCursorStyle::ResizeHorizontal,
        CursorStyle::ResizeVertical => ClipCursorStyle::ResizeVertical,
        CursorStyle::ResizeNwSe => ClipCursorStyle::ResizeNwSe,
        CursorStyle::ResizeNeSw => ClipCursorStyle::ResizeNeSw,
        CursorStyle::Text => ClipCursorStyle::Text,
        CursorStyle::NotAllowed => ClipCursorStyle::NotAllowed,
        CursorStyle::CornerRadius => ClipCursorStyle::CornerRadius,
        CursorStyle::Hidden => ClipCursorStyle::Hidden,
        CursorStyle::Stroke => ClipCursorStyle::Stroke,
        CursorStyle::Eraser => ClipCursorStyle::Eraser,
    }
}

impl From<ClipBrushEraserStyle> for clip_draw_engine::BrushEraserStyle {
    fn from(value: ClipBrushEraserStyle) -> Self {
        Self {
            stroke_width: value.stroke_width,
        }
    }
}
impl From<clip_draw_engine::BrushEraserStyle> for ClipBrushEraserStyle {
    fn from(value: clip_draw_engine::BrushEraserStyle) -> Self {
        Self {
            stroke_width: value.stroke_width,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use clip_draw_engine::{FillStyle, StyleDefaults};

    fn boundary_test_string(prefix_len: usize) -> (String, String) {
        let prefix = "a".repeat(prefix_len);
        let value = format!("{prefix}😀");
        (prefix, value)
    }

    #[test]
    fn rectangle_style_fill_style_round_trips_through_c_abi() {
        let c_style = ClipShapeStyle {
            fill_style: ClipFillStyle::CrossLine,
            ..ClipShapeStyle::default()
        };

        let rust_style: ShapeStyle = c_style.into();
        assert_eq!(rust_style.fill_style, FillStyle::CrossLine);

        let round_trip: ClipShapeStyle = rust_style.into();
        assert_eq!(round_trip.fill_style, ClipFillStyle::CrossLine);
    }

    #[test]
    fn reference_arrowheads_round_trip_through_c_abi() {
        for (rust, c) in [
            (Arrowhead::IndentedTriangle, ClipArrowhead::IndentedTriangle),
            (Arrowhead::Triangle, ClipArrowhead::Triangle),
            (Arrowhead::Square, ClipArrowhead::Square),
            (Arrowhead::InvertedTriangle, ClipArrowhead::InvertedTriangle),
        ] {
            assert_eq!(clip_arrowhead_from_rust(Some(rust)), c);
            assert_eq!(clip_arrowhead_to_rust(c), Some(rust));
        }
    }

    #[test]
    fn corner_radius_cursor_is_exposed_through_the_c_abi() {
        assert_eq!(
            clip_cursor_style_from_rust(CursorStyle::CornerRadius),
            ClipCursorStyle::CornerRadius
        );
    }

    #[test]
    fn native_tool_cursors_are_exposed_through_the_c_abi() {
        for (style, expected) in [
            (CursorStyle::Stroke, ClipCursorStyle::Stroke),
            (CursorStyle::Eraser, ClipCursorStyle::Eraser),
        ] {
            let output = clip_interaction_output_from_rust(InteractionOutput {
                cursor: CursorCommand::Set(style),
                ..InteractionOutput::default()
            });
            assert_eq!(output.cursor_kind, ClipCursorCommandKind::Set);
            assert_eq!(output.cursor_style, expected);
        }
    }

    #[test]
    fn hidden_cursor_is_exposed_through_the_c_abi() {
        assert_eq!(
            clip_cursor_style_from_rust(CursorStyle::Hidden),
            ClipCursorStyle::Hidden
        );
    }

    #[test]
    fn text_element_info_conversion_truncates_text_and_font_family_at_utf8_boundaries() {
        let (text_prefix, text) = boundary_test_string(CLIP_TEXT_UTF8_CAPACITY - 1);
        let (family_prefix, family) = boundary_test_string(CLIP_FONT_FAMILY_UTF8_CAPACITY - 1);

        let info = clip_text_element_info_from_rust(TextElementInfo {
            arrow_id: None,
            arrow_width: 0.0,
            id: ElementId {
                index: 7,
                generation: 3,
            },
            center: Point { x: 1.0, y: 2.0 },
            width: 100.0,
            height: 30.0,
            content_width: 84.0,
            content_height: 27.0,
            rotation: 0.0,
            text,
            font_size: 21.0,
            font_family: Some(family),
            auto_resize: true,
            measure_natural_width: false,
        });

        assert_eq!(info.content_width, 84.0);
        assert_eq!(info.content_height, 27.0);
        assert_eq!(info.text_utf8_len, (CLIP_TEXT_UTF8_CAPACITY - 1) as u32);
        assert_eq!(info.text_truncated, 1);
        assert_eq!(
            string_from_c_char_field(&info.text_utf8, info.text_utf8_len),
            Some(text_prefix)
        );
        assert_eq!(
            info.font_family_utf8_len,
            (CLIP_FONT_FAMILY_UTF8_CAPACITY - 1) as u32
        );
        assert_eq!(info.font_family_truncated, 1);
        assert_eq!(
            string_from_c_char_field(&info.font_family_utf8, info.font_family_utf8_len),
            Some(family_prefix)
        );
    }

    #[test]
    fn style_conversion_truncates_font_family_at_utf8_boundaries() {
        let (family_prefix, family) = boundary_test_string(CLIP_FONT_FAMILY_UTF8_CAPACITY - 1);

        let text_style: ClipTextStyle = TextStyle {
            color: ColorRgba8::default(),
            font_size: 21.0,
            font_family: Some(family.clone()),
            fill: ColorRgba8::default(),
            fill_style: FillStyle::Solid,
            stroke: ColorRgba8::default(),
            stroke_width: 0.0,
            corner_radii: CornerRadii::default(),
            horizontal_align: TextHorizontalAlign::Left,
            vertical_align: TextVerticalAlign::Center,
            opacity: 1.0,
        }
        .into();

        assert_eq!(
            text_style.font_family_utf8_len,
            (CLIP_FONT_FAMILY_UTF8_CAPACITY - 1) as u32
        );
        assert_eq!(text_style.font_family_truncated, 1);
        assert_eq!(
            string_from_c_char_field(
                &text_style.font_family_utf8,
                text_style.font_family_utf8_len
            ),
            Some(family_prefix.clone())
        );

        let serial_style: ClipSerialNumberStyle = SerialNumberStyle {
            number: 1,
            serial_number_type: SerialNumberType::OutlinedCircle,
            numeric_type: clip_draw_engine_document::SerialNumberNumericType::Arabic,
            color: ColorRgba8::default(),
            fill: ColorRgba8::default(),
            fill_style: FillStyle::Solid,
            font_size: 16.0,
            font_family: Some(family),
            stroke_width: 2.0,
            stroke_style: StrokeStyle::Solid,
            opacity: 1.0,
        }
        .into();

        assert_eq!(
            serial_style.font_family_utf8_len,
            (CLIP_FONT_FAMILY_UTF8_CAPACITY - 1) as u32
        );
        assert_eq!(serial_style.font_family_truncated, 1);
        assert_eq!(
            string_from_c_char_field(
                &serial_style.font_family_utf8,
                serial_style.font_family_utf8_len
            ),
            Some(family_prefix)
        );
    }

    #[test]
    fn text_style_from_c_bounds_reported_font_family_length() {
        let mut style = ClipTextStyle::default();
        style.font_family_utf8.fill(b'a' as std::ffi::c_char);
        style.font_family_utf8_len = (CLIP_FONT_FAMILY_UTF8_CAPACITY + 10) as u32;

        let converted: TextStyle = style.into();

        assert_eq!(
            converted.font_family,
            Some("a".repeat(CLIP_FONT_FAMILY_UTF8_CAPACITY))
        );
    }

    #[test]
    fn complete_runtime_style_defaults_round_trip_through_c_abi() {
        let mut expected = StyleDefaults::default();
        expected.editor.rectangle.fill = ColorRgba8 {
            r: 1,
            g: 2,
            b: 3,
            a: 0,
        };
        expected.editor.rectangle.stroke_width = 3.0;
        expected.editor.arrow.stroke_width = 4.0;
        expected.editor.arrow.arrow_ratio = 2.3;
        expected.editor.line.stroke_width = 5.0;
        expected.editor.free_draw.stroke_width = 6.0;
        expected.editor.rectangle_highlight.stroke_width = 7.0;
        expected.editor.pen_highlight.stroke_width = 8.0;
        expected.editor.rectangle_filter.filter_type = CanvasFilterType::Emboss;
        expected.editor.rectangle_filter.strength = 0.41;
        expected.editor.rectangle_filter.stroke_width = 9.0;
        expected.editor.pen_filter.filter_type = CanvasFilterType::Emboss;
        expected.editor.pen_filter.strength = 0.42;
        expected.editor.pen_filter.stroke_width = 10.0;
        expected.editor.text.font_family = Some("C Text Font".to_owned());
        expected.editor.serial_number.font_family = Some("C Serial Font".to_owned());
        expected.editor.serial_number.serial_number_type = SerialNumberType::Circle;
        expected.watermark.text = "C watermark".to_owned();
        expected.watermark.template_value = "  {text} {YYYY-MM-DD_HH-mm-ss}  ".to_owned();
        expected.watermark.template_application_time = Some(WatermarkTemplateApplicationTime {
            year: 2026,
            month: 9,
            day: 15,
            hour: 12,
            minute: 34,
            second: 56,
        });
        expected.watermark.font_family = "C Watermark Font".to_owned();
        expected.watermark.opacity = 0.24;
        expected.spotlight.opacity = 0.62;
        expected.editor.spotlight_shape = clip_draw_engine::HighlightShape::Diamond;

        let c_defaults: ClipStyleDefaults = expected.clone().into();
        assert_eq!(c_defaults.rectangle.fill.a, 0);
        assert_eq!(c_defaults.spotlight_shape, ClipRectangleShape::Diamond);
        assert_eq!(
            c_defaults.rectangle_filter.filter_type,
            ClipFilterType::Emboss
        );
        assert_eq!(c_defaults.pen_filter.filter_type, ClipFilterType::Emboss);
        assert_eq!(c_defaults.text.font_family_truncated, 0);
        assert_eq!(c_defaults.serial_number.font_family_truncated, 0);
        assert_eq!(
            c_defaults.serial_number.serial_number_type,
            ClipSerialNumberType::Circle
        );
        let c_config = ClipRuntimeConfig {
            style_defaults: &c_defaults,
        };
        let converted = runtime_config_from_c(Some(&c_config)).unwrap();

        assert_eq!(converted.style_defaults, expected);
    }

    #[test]
    fn runtime_style_defaults_accept_smart_erase_filter_types() {
        let mut c_defaults: ClipStyleDefaults = StyleDefaults::default().into();
        c_defaults.rectangle_filter.filter_type = ClipFilterType::SmartErase;
        c_defaults.pen_filter.filter_type = ClipFilterType::SmartErase;
        let c_config = ClipRuntimeConfig {
            style_defaults: &c_defaults,
        };

        let converted = runtime_config_from_c(Some(&c_config)).unwrap();

        assert_eq!(
            converted.style_defaults.editor.rectangle_filter.filter_type,
            CanvasFilterType::SmartErase
        );
        assert_eq!(
            converted.style_defaults.editor.pen_filter.filter_type,
            CanvasFilterType::SmartErase
        );
    }

    #[test]
    fn watermark_template_round_trip_and_truncation_are_utf8_safe() {
        let application_time = WatermarkTemplateApplicationTime {
            year: 2026,
            month: 9,
            day: 15,
            hour: 12,
            minute: 34,
            second: 56,
        };
        let expected = WatermarkConfig {
            text: "watermark".to_owned(),
            template_value: "  {text} {YYYY}  ".to_owned(),
            template_application_time: Some(application_time),
            ..WatermarkConfig::default()
        };

        let c_config: ClipWatermarkConfig = expected.clone().into();
        assert_eq!(c_config.has_template_application_time, 1);
        assert_eq!(WatermarkConfig::from(c_config), expected);

        let prefix = "a".repeat(CLIP_WATERMARK_TEMPLATE_UTF8_CAPACITY - 1);
        let c_config: ClipWatermarkConfig = WatermarkConfig {
            template_value: format!("{prefix}😀"),
            ..WatermarkConfig::default()
        }
        .into();
        assert_eq!(
            c_config.template_value_utf8_len,
            (CLIP_WATERMARK_TEMPLATE_UTF8_CAPACITY - 1) as u32
        );
        assert_eq!(
            string_from_c_char_field(
                &c_config.template_value_utf8,
                c_config.template_value_utf8_len
            ),
            Some(prefix)
        );
    }

    #[test]
    fn invalid_c_watermark_template_time_is_normalized_to_unset() {
        let c_config = ClipWatermarkConfig {
            has_template_application_time: 1,
            template_application_time: ClipWatermarkTemplateApplicationTime {
                year: 2025,
                month: 2,
                day: 29,
                hour: 12,
                minute: 34,
                second: 56,
                reserved0: [0; 3],
            },
            ..ClipWatermarkConfig::default()
        };

        assert_eq!(
            WatermarkConfig::from(c_config).template_application_time,
            None
        );
    }

    #[test]
    fn runtime_config_rejects_truncated_profile_strings() {
        let mut defaults: ClipStyleDefaults = StyleDefaults::default().into();
        defaults.text.font_family_truncated = 1;
        let config = ClipRuntimeConfig {
            style_defaults: &defaults,
        };

        assert_eq!(
            runtime_config_from_c(Some(&config)),
            Err(ClipError::InvalidArgument)
        );
    }
}
