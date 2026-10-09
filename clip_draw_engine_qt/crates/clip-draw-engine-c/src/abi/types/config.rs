use super::*;

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ClipZoomFocus {
    Pointer = 0,
    Center = 1,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ClipActiveTool {
    Select = 0,
    Shape = 1,
    Arrow = 2,
    Text = 3,
    SerialNumber = 4,
    Line = 5,
    FreeDraw = 6,
    RectangleHighlight = 7,
    Eraser = 8,
    RectangleFilter = 9,
    Watermark = 10,
    PenHighlight = 11,
    PenFilter = 12,
    Spotlight = 13,
    AutoFilter = 14,
    RectangleEraser = 15,
    BrushEraser = 16,
    Distance = 17,
}

impl ClipActiveTool {
    #[allow(non_upper_case_globals)]
    pub const Filter: Self = Self::RectangleFilter;
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ClipStyleToolbarSource {
    DefaultRectangle = 0,
    SelectedRectangle = 1,
    DefaultArrow = 2,
    SelectedArrow = 3,
    DefaultText = 4,
    SelectedText = 5,
    DefaultSerialNumber = 6,
    SelectedSerialNumber = 7,
    DefaultLine = 8,
    SelectedLine = 9,
    DefaultFreeDraw = 10,
    SelectedFreeDraw = 11,
    DefaultRectangleHighlight = 12,
    SelectedRectangleHighlight = 13,
    Eraser = 14,
    DefaultRectangleFilter = 15,
    SelectedRectangleFilter = 16,
    Watermark = 17,
    DefaultPenHighlight = 18,
    SelectedPenHighlight = 19,
    DefaultPenFilter = 20,
    SelectedPenFilter = 21,
    DefaultSpotlight = 22,
    SelectedSpotlight = 23,
    DefaultRectangleEraser = 24,
    DefaultBrushEraser = 25,
    DefaultDistance = 26,
    SelectedDistance = 27,
}

impl ClipStyleToolbarSource {
    #[allow(non_upper_case_globals)]
    pub const DefaultFilter: Self = Self::DefaultRectangleFilter;
    #[allow(non_upper_case_globals)]
    pub const SelectedFilter: Self = Self::SelectedRectangleFilter;
}

clip_c_enum! {
    #[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
    pub enum ClipFilterType {
        #[default]
        Mosaic = 0,
        GaussianBlur = 1,
        Grayscale = 2,
        Inversion = 3,
        Emboss = 4,
        SmartErase = 5,
        Brightness = 6,
        RestoreBackground = 7,
    }
}

clip_c_enum! {
    #[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
    pub enum ClipSerialNumberType {
        #[default]
        OutlinedCircle = 0,
        SolidCircle = 1,
        OutlinedSquare = 2,
        SolidSquare = 3,
    Circle = 4,
    }
}

clip_c_enum! {
    #[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
    pub enum ClipSerialNumberNumericType {
        #[default]
        Arabic = 0,
        Roman = 1,
        LowercaseLetters = 2,
        UppercaseLetters = 3,
        Chinese = 4,
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ClipFilterStyle {
    pub filter_type: ClipFilterType,
    pub strength: f64,
    pub opacity: f64,
    pub stroke_width: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct ClipColorRgba8 {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub a: u8,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ClipCornerRadii {
    pub top_left: f64,
    pub top_right: f64,
    pub bottom_right: f64,
    pub bottom_left: f64,
}

clip_c_enum! {
    #[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
    pub enum ClipDistanceUnit {
        #[default]
        Px = 0,
        Cm = 1,
        M = 2,
        Km = 3,
        Mm = 4,
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipDistanceStyle {
    pub stroke: ClipColorRgba8,
    pub stroke_width: f64,
    pub factor: f64,
    pub unit: ClipDistanceUnit,
    pub decimal_places: u32,
    pub endpoint_scale: f64,
    pub endpoint_style: ClipArrowhead,
}

pub const CLIP_DISTANCE_STYLE_PROPERTY_STROKE: u32 = 1 << 0;
pub const CLIP_DISTANCE_STYLE_PROPERTY_STROKE_WIDTH: u32 = 1 << 1;
pub const CLIP_DISTANCE_STYLE_PROPERTY_FACTOR: u32 = 1 << 2;
pub const CLIP_DISTANCE_STYLE_PROPERTY_UNIT: u32 = 1 << 3;
pub const CLIP_DISTANCE_STYLE_PROPERTY_DECIMAL_PLACES: u32 = 1 << 4;
pub const CLIP_DISTANCE_STYLE_PROPERTY_ENDPOINT_SCALE: u32 = 1 << 5;
pub const CLIP_DISTANCE_STYLE_PROPERTY_ENDPOINT_STYLE: u32 = 1 << 6;
pub const CLIP_DISTANCE_STYLE_PROPERTY_ALL: u32 = (1 << 7) - 1;

pub const CLIP_DISTANCE_STYLE_MIXED_STROKE: u32 = CLIP_DISTANCE_STYLE_PROPERTY_STROKE;
pub const CLIP_DISTANCE_STYLE_MIXED_STROKE_WIDTH: u32 = CLIP_DISTANCE_STYLE_PROPERTY_STROKE_WIDTH;
pub const CLIP_DISTANCE_STYLE_MIXED_FACTOR: u32 = CLIP_DISTANCE_STYLE_PROPERTY_FACTOR;
pub const CLIP_DISTANCE_STYLE_MIXED_UNIT: u32 = CLIP_DISTANCE_STYLE_PROPERTY_UNIT;
pub const CLIP_DISTANCE_STYLE_MIXED_DECIMAL_PLACES: u32 =
    CLIP_DISTANCE_STYLE_PROPERTY_DECIMAL_PLACES;
pub const CLIP_DISTANCE_STYLE_MIXED_ENDPOINT_SCALE: u32 =
    CLIP_DISTANCE_STYLE_PROPERTY_ENDPOINT_SCALE;
pub const CLIP_DISTANCE_STYLE_MIXED_ENDPOINT_STYLE: u32 =
    CLIP_DISTANCE_STYLE_PROPERTY_ENDPOINT_STYLE;

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipShapeStyle {
    pub fill: ClipColorRgba8,
    pub stroke: ClipColorRgba8,
    pub stroke_width: f64,
    pub corner_radii: ClipCornerRadii,
    pub start_arrowhead: ClipArrowhead,
    pub end_arrowhead: ClipArrowhead,
    pub stroke_style: ClipStrokeStyle,
    pub arrow_type: ClipArrowType,
    pub arrow_shaft_type: ClipArrowShaftType,
    pub arrow_ratio: f64,
    pub fill_style: ClipFillStyle,
    pub opacity: f64,
    pub highlight_shape: ClipHighlightShape,
    pub shape: ClipRectangleShape,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct ClipWatermarkTemplateApplicationTime {
    pub year: i32,
    pub month: u8,
    pub day: u8,
    pub hour: u8,
    pub minute: u8,
    pub second: u8,
    pub reserved0: [u8; 3],
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipWatermarkConfig {
    pub color: ClipColorRgba8,
    pub text_utf8_len: u32,
    pub text_utf8: [std::ffi::c_char; CLIP_WATERMARK_TEXT_UTF8_CAPACITY],
    pub font_size: f64,
    pub font_family_utf8_len: u32,
    pub font_family_utf8: [std::ffi::c_char; CLIP_FONT_FAMILY_UTF8_CAPACITY],
    pub angle: f64,
    pub gap: f64,
    pub opacity: f64,
    pub template_value_utf8_len: u32,
    pub template_value_utf8: [std::ffi::c_char; CLIP_WATERMARK_TEMPLATE_UTF8_CAPACITY],
    pub has_template_application_time: u8,
    pub reserved0: [u8; 3],
    pub template_application_time: ClipWatermarkTemplateApplicationTime,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ClipSpotlightConfig {
    pub color: ClipColorRgba8,
    pub opacity: f64,
}

impl Default for ClipWatermarkConfig {
    fn default() -> Self {
        Self {
            color: ClipColorRgba8::default(),
            text_utf8_len: 0,
            text_utf8: [0; CLIP_WATERMARK_TEXT_UTF8_CAPACITY],
            font_size: 16.0,
            font_family_utf8_len: 0,
            font_family_utf8: [0; CLIP_FONT_FAMILY_UTF8_CAPACITY],
            angle: 30.0,
            gap: 56.0,
            opacity: 0.16,
            template_value_utf8_len: 0,
            template_value_utf8: [0; CLIP_WATERMARK_TEMPLATE_UTF8_CAPACITY],
            has_template_application_time: 0,
            reserved0: [0; 3],
            template_application_time: ClipWatermarkTemplateApplicationTime::default(),
        }
    }
}

clip_c_enum! {
    #[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
    pub enum ClipHighlightShape {
        #[default]
        Rectangle = 0,
        Ellipse = 1,
    }
}

clip_c_enum! {
    #[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
    pub enum ClipRectangleShape {
        #[default]
        Rectangle = 0,
        Ellipse = 1,
        Diamond = 2,
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipRectangleShapeStyle {
    pub fill: ClipColorRgba8,
    pub fill_style: ClipFillStyle,
    pub stroke: ClipColorRgba8,
    pub stroke_width: f64,
    pub stroke_style: ClipStrokeStyle,
    pub corner_radii: ClipCornerRadii,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ClipShapeKind {
    Rectangle = 0,
    Arrow = 1,
    Line = 2,
    FreeDraw = 3,
    RectangleHighlight = 4,
    PenHighlight = 5,
    Spotlight = 6,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipArrowStyle {
    pub stroke: ClipColorRgba8,
    pub stroke_width: f64,
    pub start_arrowhead: ClipArrowhead,
    pub end_arrowhead: ClipArrowhead,
    pub stroke_style: ClipStrokeStyle,
    pub arrow_type: ClipArrowType,
    pub arrow_shaft_type: ClipArrowShaftType,
    pub arrow_ratio: f64,
    pub reserved0: [u8; 4],
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipTextStyle {
    pub color: ClipColorRgba8,
    pub font_size: f64,
    pub fill: ClipColorRgba8,
    pub fill_style: ClipFillStyle,
    pub stroke: ClipColorRgba8,
    pub stroke_width: f64,
    pub corner_radii: ClipCornerRadii,
    pub horizontal_align: ClipTextHorizontalAlign,
    pub vertical_align: ClipTextVerticalAlign,
    pub opacity: f64,
    pub reserved0: [u8; 4],
    pub font_family_utf8_len: u32,
    pub font_family_truncated: u8,
    pub reserved1: [u8; 3],
    pub font_family_utf8: [std::ffi::c_char; CLIP_FONT_FAMILY_UTF8_CAPACITY],
}

#[cfg(test)]
mod spotlight_abi_tests {
    use super::*;

    #[test]
    fn spotlight_values_are_append_only_and_config_layout_is_stable() {
        assert_eq!(ClipActiveTool::PenFilter as i32, 12);
        assert_eq!(ClipActiveTool::Spotlight as i32, 13);
        assert_eq!(ClipStyleToolbarSource::SelectedPenFilter as i32, 21);
        assert_eq!(ClipStyleToolbarSource::DefaultSpotlight as i32, 22);
        assert_eq!(ClipStyleToolbarSource::SelectedSpotlight as i32, 23);
        assert_eq!(ClipShapeKind::PenHighlight as i32, 5);
        assert_eq!(ClipShapeKind::Spotlight as i32, 6);
        assert_eq!(std::mem::size_of::<ClipSpotlightConfig>(), 16);
        assert_eq!(std::mem::offset_of!(ClipSpotlightConfig, color), 0);
        assert_eq!(std::mem::offset_of!(ClipSpotlightConfig, opacity), 8);
        assert_eq!(std::mem::size_of::<ClipSpotlightCutout>(), 48);
        assert_eq!(std::mem::offset_of!(ClipSpotlightCutout, rotation), 32);
        assert_eq!(std::mem::offset_of!(ClipSpotlightCutout, shape), 40);
        assert_eq!(ClipSerialNumberType::OutlinedCircle as i32, 0);
        assert_eq!(ClipSerialNumberType::SolidCircle as i32, 1);
        assert_eq!(ClipSerialNumberType::OutlinedSquare as i32, 2);
        assert_eq!(ClipSerialNumberType::SolidSquare as i32, 3);
        assert_eq!(ClipSerialNumberType::Circle as i32, 4);
        assert_eq!(std::mem::size_of::<ClipSerialNumberStyle>(), 200);
        assert_eq!(std::mem::align_of::<ClipSerialNumberStyle>(), 8);
        assert_eq!(
            std::mem::offset_of!(ClipSerialNumberStyle, numeric_type),
            196
        );
        assert_eq!(ClipSerialNumberNumericType::Arabic as i32, 0);
        assert_eq!(ClipSerialNumberNumericType::Chinese as i32, 4);
        assert_eq!(
            std::mem::offset_of!(ClipSerialNumberStyle, serial_number_type),
            56
        );
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipSerialNumberStyle {
    pub number: i64,
    pub color: ClipColorRgba8,
    pub fill: ClipColorRgba8,
    pub fill_style: ClipFillStyle,
    pub font_size: f64,
    pub stroke_width: f64,
    pub stroke_style: ClipStrokeStyle,
    pub opacity: f64,
    pub serial_number_type: ClipSerialNumberType,
    pub font_family_utf8_len: u32,
    pub font_family_truncated: u8,
    pub reserved1: [u8; 3],
    pub font_family_utf8: [std::ffi::c_char; CLIP_FONT_FAMILY_UTF8_CAPACITY],
    pub numeric_type: ClipSerialNumberNumericType,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipStyleToolbarState {
    pub source: ClipStyleToolbarSource,
    pub selected_element_count: u32,
    pub shape_style: ClipShapeStyle,
    pub text_style: ClipTextStyle,
    pub serial_number_style: ClipSerialNumberStyle,
    pub text_style_mixed: u32,
    pub serial_number_style_mixed: u32,
    pub shape_style_mixed: u32,
    pub filter_style: ClipFilterStyle,
    pub filter_style_mixed: u32,
    pub brush_eraser_style: ClipBrushEraserStyle,
    pub distance_style: ClipDistanceStyle,
    pub distance_style_mixed: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipStyleDefaults {
    pub rectangle: ClipShapeStyle,
    pub arrow: ClipShapeStyle,
    pub line: ClipShapeStyle,
    pub free_draw: ClipShapeStyle,
    pub rectangle_highlight: ClipShapeStyle,
    pub pen_highlight: ClipShapeStyle,
    pub rectangle_filter: ClipFilterStyle,
    pub pen_filter: ClipFilterStyle,
    pub text: ClipTextStyle,
    pub serial_number: ClipSerialNumberStyle,
    pub watermark: ClipWatermarkConfig,
    pub spotlight: ClipSpotlightConfig,
    pub brush_eraser: ClipBrushEraserStyle,
    pub spotlight_shape: ClipRectangleShape,
    pub distance: ClipDistanceStyle,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipRuntimeConfig {
    pub style_defaults: *const ClipStyleDefaults,
}

impl Default for ClipRuntimeConfig {
    fn default() -> Self {
        Self {
            style_defaults: std::ptr::null(),
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ClipSerialNumberToolbarState {
    pub visible: u8,
    pub can_decrease: u8,
    pub can_increase: u8,
    pub can_create_text: u8,
    pub reserved0: [u8; 4],
    pub left: f64,
    pub top: f64,
    pub width: f64,
    pub height: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct ClipHistoryState {
    pub can_undo: u8,
    pub can_redo: u8,
    pub reserved0: [u8; 6],
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct ClipModifiers {
    pub ctrl: u8,
    pub shift: u8,
    pub alt: u8,
    pub meta: u8,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipSnapConfig {
    pub enabled: u8,
    pub enable_point_snaps: u8,
    pub enable_gap_snaps: u8,
    pub show_guides: u8,
    pub show_gap_size: u8,
    pub reserved0: [u8; 3],
    pub distance: f64,
    pub line_color: ClipColorRgba8,
    pub line_width: f64,
    pub marker_size: f64,
    pub gap_dash_length: f64,
    pub gap_dash_gap: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipGridConfig {
    pub enabled: u8,
    pub reserved0: [u8; 7],
    pub size: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipEngineConfig {
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub zoom_focus: ClipZoomFocus,
    pub wheel_zoom_sensitivity: f64,
    pub clear_color: ClipColorRgba8,
    pub snap: ClipSnapConfig,
    pub grid: ClipGridConfig,
    pub enable_pointer_capture: u8,
    pub reserved: [u8; 7],
}

impl Default for ClipSnapConfig {
    fn default() -> Self {
        Self {
            enabled: 0,
            enable_point_snaps: 1,
            enable_gap_snaps: 1,
            show_guides: 1,
            show_gap_size: 0,
            reserved0: [0; 3],
            distance: 8.0,
            line_color: ClipColorRgba8 {
                r: 255,
                g: 107,
                b: 107,
                a: 255,
            },
            line_width: 1.0,
            marker_size: 8.0,
            gap_dash_length: 4.0,
            gap_dash_gap: 4.0,
        }
    }
}

impl Default for ClipShapeStyle {
    fn default() -> Self {
        Self {
            fill: ClipColorRgba8::default(),
            stroke: ClipColorRgba8::default(),
            stroke_width: 0.0,
            corner_radii: ClipCornerRadii::default(),
            start_arrowhead: ClipArrowhead::None,
            end_arrowhead: ClipArrowhead::None,
            stroke_style: ClipStrokeStyle::Solid,
            arrow_type: ClipArrowType::Straight,
            arrow_shaft_type: ClipArrowShaftType::Plain,
            arrow_ratio: 1.0,
            fill_style: ClipFillStyle::Solid,
            opacity: 1.0,
            highlight_shape: ClipHighlightShape::Rectangle,
            shape: ClipRectangleShape::Rectangle,
        }
    }
}

impl Default for ClipRectangleShapeStyle {
    fn default() -> Self {
        Self {
            fill: ClipColorRgba8::default(),
            fill_style: ClipFillStyle::Solid,
            stroke: ClipColorRgba8::default(),
            stroke_width: 0.0,
            stroke_style: ClipStrokeStyle::Solid,
            corner_radii: ClipCornerRadii::default(),
        }
    }
}

impl Default for ClipArrowStyle {
    fn default() -> Self {
        Self {
            stroke: ClipColorRgba8::default(),
            stroke_width: 0.0,
            start_arrowhead: ClipArrowhead::None,
            end_arrowhead: ClipArrowhead::None,
            stroke_style: ClipStrokeStyle::Solid,
            arrow_type: ClipArrowType::Straight,
            arrow_shaft_type: ClipArrowShaftType::Plain,
            arrow_ratio: 1.0,
            reserved0: [0; 4],
        }
    }
}

impl Default for ClipTextStyle {
    fn default() -> Self {
        Self {
            color: ClipColorRgba8::default(),
            font_size: 0.0,
            fill: ClipColorRgba8::default(),
            fill_style: ClipFillStyle::Solid,
            stroke: ClipColorRgba8::default(),
            stroke_width: 0.0,
            corner_radii: ClipCornerRadii::default(),
            horizontal_align: ClipTextHorizontalAlign::Left,
            vertical_align: ClipTextVerticalAlign::Center,
            opacity: 1.0,
            reserved0: [0; 4],
            font_family_utf8_len: 0,
            font_family_truncated: 0,
            reserved1: [0; 3],
            font_family_utf8: [0; CLIP_FONT_FAMILY_UTF8_CAPACITY],
        }
    }
}

impl Default for ClipSerialNumberStyle {
    fn default() -> Self {
        Self {
            number: 0,
            color: ClipColorRgba8::default(),
            fill: ClipColorRgba8::default(),
            fill_style: ClipFillStyle::Solid,
            font_size: 0.0,
            stroke_width: 0.0,
            stroke_style: ClipStrokeStyle::Solid,
            opacity: 1.0,
            serial_number_type: ClipSerialNumberType::OutlinedCircle,
            numeric_type: ClipSerialNumberNumericType::Arabic,
            font_family_utf8_len: 0,
            font_family_truncated: 0,
            reserved1: [0; 3],
            font_family_utf8: [0; CLIP_FONT_FAMILY_UTF8_CAPACITY],
        }
    }
}

impl Default for ClipGridConfig {
    fn default() -> Self {
        Self {
            enabled: 0,
            reserved0: [0; 7],
            size: 20.0,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipBrushEraserStyle {
    pub stroke_width: f64,
}

impl Default for ClipBrushEraserStyle {
    fn default() -> Self {
        Self { stroke_width: 30.0 }
    }
}

pub const CLIP_BRUSH_ERASER_STYLE_PROPERTY_STROKE_WIDTH: u32 = 1 << 0;
