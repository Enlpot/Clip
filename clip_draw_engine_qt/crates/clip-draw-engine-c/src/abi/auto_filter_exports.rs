use crate::abi::{handles::*, types::*};
use clip_draw_engine_core::DrawRect;
use clip_draw_engine_document::{AutoFilterRegion, AutoFilterRegionRecord};

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct ClipAutoFilterBounds {
    pub left: f64,
    pub top: f64,
    pub right: f64,
    pub bottom: f64,
}
impl From<DrawRect> for ClipAutoFilterBounds {
    fn from(r: DrawRect) -> Self {
        Self {
            left: r.min_x,
            top: r.min_y,
            right: r.max_x,
            bottom: r.max_y,
        }
    }
}
impl From<ClipAutoFilterBounds> for DrawRect {
    fn from(r: ClipAutoFilterBounds) -> Self {
        Self::new(r.left, r.top, r.right, r.bottom)
    }
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct ClipAutoFilterRegion {
    pub id: u64,
    pub bounds: ClipAutoFilterBounds,
    pub category: [u8; 128],
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_get_auto_filter_regions(
    runtime: ClipRuntime,
    generation: *mut u64,
    identified: *mut u8,
    bounds: *mut ClipAutoFilterBounds,
    regions: *mut ClipAutoFilterRegion,
    capacity: usize,
    count: *mut usize,
) -> ClipError {
    ffi_error(|| {
        if generation.is_null() || identified.is_null() || bounds.is_null() || count.is_null() {
            return ClipError::InvalidArgument;
        }
        ffi_status(with_runtime_ref(runtime, |engine| {
            write_out(generation, engine.auto_filter_generation());
            let record = engine.auto_filter_regions();
            write_out(identified, u8::from(record.is_some()));
            write_out(
                bounds,
                record.map(|r| r.source_bounds.into()).unwrap_or_default(),
            );
            write_out(count, record.map_or(0, |r| r.regions.len()));
            if let Some(record) = record
                && !regions.is_null()
            {
                if capacity < record.regions.len() {
                    return Err(ClipError::InvalidArgument);
                }
                for (i, region) in record.regions.iter().enumerate() {
                    let mut category = [0; 128];
                    let bytes = region.category.as_bytes();
                    category[..bytes.len()].copy_from_slice(bytes);
                    unsafe {
                        regions.add(i).write(ClipAutoFilterRegion {
                            id: region.id,
                            bounds: region.bounds.into(),
                            category,
                        });
                    }
                }
            }
            Ok(())
        }))
    })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_auto_filter_regions(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    bounds: *const ClipAutoFilterBounds,
    regions: *const ClipAutoFilterRegion,
    count: usize,
    changed: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if changed.is_null() || count > 100_000 || (count != 0 && regions.is_null()) {
            return ClipError::InvalidArgument;
        }
        let record = if bounds.is_null() {
            None
        } else {
            let input = if count == 0 {
                &[]
            } else {
                unsafe { std::slice::from_raw_parts(regions, count) }
            };
            let mut output = Vec::with_capacity(count);
            for region in input {
                let length = region.category.iter().position(|b| *b == 0).unwrap_or(128);
                let Ok(category) = std::str::from_utf8(&region.category[..length]) else {
                    return ClipError::InvalidArgument;
                };
                output.push(AutoFilterRegion {
                    id: region.id,
                    bounds: region.bounds.into(),
                    category: category.into(),
                });
            }
            Some(AutoFilterRegionRecord {
                source_bounds: unsafe { (*bounds).into() },
                regions: output,
            })
        };
        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let result = state
                .runtime
                .set_auto_filter_regions(viewport_id(viewport)?, record)
                .map_err(ClipError::from)?;
            write_changed_viewports(changed, result.changed_viewports);
            Ok(())
        }))
    })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_fill_auto_filter_category(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    category: *const u8,
    length: usize,
    changed: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if changed.is_null() || category.is_null() || length > 128 {
            return ClipError::InvalidArgument;
        }
        let Ok(category) =
            std::str::from_utf8(unsafe { std::slice::from_raw_parts(category, length) })
        else {
            return ClipError::InvalidArgument;
        };
        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let result = state
                .runtime
                .fill_auto_filter_category(viewport_id(viewport)?, category)
                .map_err(ClipError::from)?;
            write_changed_viewports(changed, result.changed_viewports);
            Ok(())
        }))
    })
}
