use super::*;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ClipArrowTextLayoutRequest {
    pub info: ClipTextElementInfo,
    pub style: ClipTextStyle,
    pub key: u64,
    pub max_width: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ClipArrowTextLayoutResult {
    pub text_id: ClipElementId,
    pub key: u64,
    pub size: ClipTextLayoutSize,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ClipArrowTextLayoutMetrics {
    pub text_id: ClipElementId,
    pub key: u64,
    pub size: ClipTextLayoutSize,
    pub natural_width: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipSerialLabelLayoutRequest {
    pub text_id: ClipElementId,
    pub font_size: f64,
    pub font_family_utf8_len: u32,
    pub font_family_truncated: u8,
    pub reserved0: [u8; 3],
    pub font_family_utf8: [std::ffi::c_char; CLIP_FONT_FAMILY_UTF8_CAPACITY],
}

impl Default for ClipSerialLabelLayoutRequest {
    fn default() -> Self {
        Self {
            text_id: ClipElementId::default(),
            font_size: 0.0,
            font_family_utf8_len: 0,
            font_family_truncated: 0,
            reserved0: [0; 3],
            font_family_utf8: [0; CLIP_FONT_FAMILY_UTF8_CAPACITY],
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct ClipElementId {
    pub index: u32,
    pub generation: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ClipTextLayoutSize {
    /// Exact host-renderer measured text width.
    pub width: f64,
    /// Exact host-renderer measured text height.
    pub height: f64,
    /// Measured painted ink width (widest line); 0 means "not measured" and
    /// consumers fall back to `width`.
    pub content_width: f64,
    /// Measured painted ink height; 0 means "not measured" and consumers fall
    /// back to `height`.
    pub content_height: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ClipTextLayoutOverride {
    /// Text element that this exact host-renderer layout applies to.
    pub id: ClipElementId,
    pub size: ClipTextLayoutSize,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipTextCommitDraft {
    pub element_id: ClipElementId,
    pub arrow_id: ClipElementId,
    pub has_existing_element: u8,
    pub auto_resize: u8,
    pub update_default_style: u8,
    pub reserved0: [u8; 5],
    pub center_x: f64,
    pub center_y: f64,
    pub text_utf8: *const std::ffi::c_char,
    pub text_utf8_len: u32,
    pub reserved1: u32,
    pub measured_layout: ClipTextLayoutSize,
    pub style: ClipTextStyle,
}

impl Default for ClipTextCommitDraft {
    fn default() -> Self {
        Self {
            element_id: ClipElementId::default(),
            arrow_id: ClipElementId::default(),
            has_existing_element: 0,
            auto_resize: 0,
            update_default_style: 0,
            reserved0: [0; 5],
            center_x: 0.0,
            center_y: 0.0,
            text_utf8: std::ptr::null(),
            text_utf8_len: 0,
            reserved1: 0,
            measured_layout: ClipTextLayoutSize::default(),
            style: ClipTextStyle::default(),
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipActiveTextDraftPresentation {
    pub element_id: ClipElementId,
    pub arrow_id: ClipElementId,
    pub has_existing_element: u8,
    pub auto_resize: u8,
    pub reserved0: [u8; 6],
    pub center_x: f64,
    pub center_y: f64,
    pub width: f64,
    pub height: f64,
    pub rotation: f64,
    /* Painted ink box of the draft preview; 0 means "not measured". */
    pub content_width: f64,
    pub content_height: f64,
    pub text_utf8: *const std::ffi::c_char,
    pub text_utf8_len: u32,
    pub reserved1: u32,
    pub style: ClipTextStyle,
}

impl Default for ClipActiveTextDraftPresentation {
    fn default() -> Self {
        Self {
            element_id: ClipElementId::default(),
            arrow_id: ClipElementId::default(),
            has_existing_element: 0,
            auto_resize: 0,
            reserved0: [0; 6],
            center_x: 0.0,
            center_y: 0.0,
            width: 0.0,
            height: 0.0,
            rotation: 0.0,
            content_width: 0.0,
            content_height: 0.0,
            text_utf8: std::ptr::null(),
            text_utf8_len: 0,
            reserved1: 0,
            style: ClipTextStyle::default(),
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipTextElementInfo {
    pub id: ClipElementId,
    pub arrow_id: ClipElementId,
    pub arrow_width: f64,
    pub center_x: f64,
    pub center_y: f64,
    pub width: f64,
    pub height: f64,
    pub content_width: f64,
    pub content_height: f64,
    pub rotation: f64,
    pub font_size: f64,
    pub text_utf8_len: u32,
    pub text_truncated: u8,
    pub auto_resize: u8,
    /// Nonzero asks the host to measure natural unwrapped width; zero asks for
    /// wrapped height using `width`.
    pub measure_natural_width: u8,
    pub reserved0: [u8; 1],
    pub text_utf8: [std::ffi::c_char; CLIP_TEXT_UTF8_CAPACITY],
    pub font_family_utf8_len: u32,
    pub font_family_truncated: u8,
    pub reserved1: [u8; 3],
    pub font_family_utf8: [std::ffi::c_char; CLIP_FONT_FAMILY_UTF8_CAPACITY],
}

impl Default for ClipTextElementInfo {
    fn default() -> Self {
        Self {
            id: ClipElementId::default(),
            arrow_id: ClipElementId::default(),
            arrow_width: 0.0,
            center_x: 0.0,
            center_y: 0.0,
            width: 0.0,
            height: 0.0,
            content_width: 0.0,
            content_height: 0.0,
            rotation: 0.0,
            font_size: 0.0,
            text_utf8_len: 0,
            text_truncated: 0,
            auto_resize: 0,
            measure_natural_width: 0,
            reserved0: [0; 1],
            text_utf8: [0; CLIP_TEXT_UTF8_CAPACITY],
            font_family_utf8_len: 0,
            font_family_truncated: 0,
            reserved1: [0; 3],
            font_family_utf8: [0; CLIP_FONT_FAMILY_UTF8_CAPACITY],
        }
    }
}
