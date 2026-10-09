pub(crate) const CLIP_ARROWHEAD_PRIMITIVE_POINT_CAPACITY: usize = 8;
pub(crate) const CLIP_TEXT_UTF8_CAPACITY: usize = 1024;
pub(crate) const CLIP_FONT_FAMILY_UTF8_CAPACITY: usize = 128;
pub(crate) const CLIP_WATERMARK_TEXT_UTF8_CAPACITY: usize = 256;
pub(crate) const CLIP_WATERMARK_TEMPLATE_UTF8_CAPACITY: usize = 1024;

mod arrow;
mod config;
mod display;
mod document;
mod input;
mod patch;
mod status;

pub use arrow::*;
pub use config::*;
pub use display::*;
pub use document::*;
pub use input::*;
pub use patch::*;
pub use status::*;

pub(crate) use crate::abi::raw_enum::clip_c_enum;
