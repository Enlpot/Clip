use super::*;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct ClipPatchCursor {
    pub scene_revision: u64,
    pub decoration_revision: u64,
    pub overlay_revision: u64,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct ClipPatchOp {
    pub start: u32,
    pub delete_count: u32,
    pub insert_offset: u32,
    pub insert_count: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ClipPenFilterGeometryPatch {
    pub element_id: ClipElementId,
    pub expected_geometry_revision: u64,
    pub resulting_geometry_revision: u64,
    pub retain_prefix_count: u32,
    pub append_offset: u32,
    pub append_count: u32,
    pub old_changed_min_x: f64,
    pub old_changed_min_y: f64,
    pub old_changed_max_x: f64,
    pub old_changed_max_y: f64,
    pub new_changed_min_x: f64,
    pub new_changed_min_y: f64,
    pub new_changed_max_x: f64,
    pub new_changed_max_y: f64,
    pub full_reset: u8,
    pub element_removed: u8,
    pub reserved0: [u8; 6],
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ClipPathGeometryPatch {
    pub element_id: ClipElementId,
    pub expected_geometry_revision: u64,
    pub resulting_geometry_revision: u64,
    pub range_offset: u32,
    pub range_count: u32,
    pub old_changed_min_x: f64,
    pub old_changed_min_y: f64,
    pub old_changed_max_x: f64,
    pub old_changed_max_y: f64,
    pub new_changed_min_x: f64,
    pub new_changed_min_y: f64,
    pub new_changed_max_x: f64,
    pub new_changed_max_y: f64,
    pub closed: u8,
    pub full_reset: u8,
    pub element_removed: u8,
    pub reserved0: [u8; 5],
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct ClipPathChunkRange {
    pub start: u32,
    pub delete_count: u32,
    pub insert_chunk_offset: u32,
    pub insert_chunk_count: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ClipPathChunk {
    pub stable_id: u64,
    pub command_start: u32,
    pub command_offset: u32,
    pub command_count: u32,
    pub reserved0: u32,
    pub start_x: f64,
    pub start_y: f64,
    pub min_x: f64,
    pub min_y: f64,
    pub max_x: f64,
    pub max_y: f64,
    pub cumulative_start_length: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipPatchInfo {
    pub scene_base_revision: u64,
    pub scene_revision: u64,
    pub decoration_base_revision: u64,
    pub decoration_revision: u64,
    pub overlay_base_revision: u64,
    pub overlay_revision: u64,
    pub scene_reset: u8,
    pub decoration_reset: u8,
    pub overlay_reset: u8,
    pub reserved0: [u8; 5],
    pub scene_op_count: u32,
    pub overlay_op_count: u32,
    pub spotlight_op_count: u32,
    pub scene_item_count: u32,
    pub overlay_item_count: u32,
    pub spotlight_item_count: u32,
    pub scene_dirty_rect_count: u32,
    pub decoration_dirty_rect_count: u32,
    pub overlay_dirty_rect_count: u32,
    pub surface_width: u32,
    pub surface_height: u32,
    pub camera_center_x: f64,
    pub camera_center_y: f64,
    pub camera_zoom: f64,
    pub clear_color: ClipColorRgba8,
    pub watermark_color: ClipColorRgba8,
    pub watermark_text_len: u16,
    pub watermark_font_family_len: u16,
    pub watermark_text: [u8; clip_draw_engine::WATERMARK_TEXT_CAPACITY],
    pub watermark_font_size: f64,
    pub watermark_font_family: [u8; clip_draw_engine::WATERMARK_FONT_FAMILY_CAPACITY],
    pub watermark_angle: f64,
    pub watermark_gap: f64,
    pub watermark_opacity: f64,
    pub spotlight_color: ClipColorRgba8,
    pub spotlight_opacity: f64,
    pub spotlight_active: u8,
    pub reserved1: [u8; 7],
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ClipDirtyRect {
    pub min_x: f64,
    pub min_y: f64,
    pub max_x: f64,
    pub max_y: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ClipSpotlightCutout {
    pub center_x: f64,
    pub center_y: f64,
    pub width: f64,
    pub height: f64,
    pub rotation: f64,
    pub shape: u8,
    pub reserved: [u8; 7],
}
