use crate::abi::convert::*;
use crate::abi::handles::*;
use crate::abi::raw_enum::ClipRawEnum;
use crate::abi::types::*;

/// # Safety
/// Handles must be live and pointers must be readable/writable as indicated.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_distance_style_patch_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    style: *const ClipDistanceStyle,
    properties: u32,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if style.is_null() || out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }
        let valid = unsafe {
            raw_c_enum_is_valid(std::ptr::addr_of!((*style).unit))
                && raw_c_enum_is_valid(std::ptr::addr_of!((*style).endpoint_style))
                && (*style).decimal_places <= 3
        };
        if !valid {
            return ClipError::InvalidArgument;
        }
        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let id = viewport_id(viewport)?;
            let result = state
                .runtime
                .set_viewport_distance_style_patch(id, unsafe { (*style).into() }, properties)
                .map_err(ClipError::from)?;
            write_changed_viewports(out_changed_viewports, result.changed_viewports);
            Ok(())
        }))
    })
}

/// # Safety
/// Handles must be live and output must be writable.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_distance_pixel_scale_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    scale_x: f64,
    scale_y: f64,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }
        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let id = viewport_id(viewport)?;
            let result = state
                .runtime
                .set_viewport_distance_pixel_scale(
                    id,
                    clip_draw_engine::Point::new(scale_x, scale_y),
                )
                .map_err(ClipError::from)?;
            write_changed_viewports(out_changed_viewports, result.changed_viewports);
            Ok(())
        }))
    })
}

unsafe fn filter_style_type_is_valid(style: *const ClipFilterStyle) -> bool {
    let raw =
        unsafe { std::ptr::read_unaligned(std::ptr::addr_of!((*style).filter_type).cast::<i32>()) };
    ClipFilterType::from_raw(raw).is_some()
}

unsafe fn serial_number_style_type_is_valid(style: *const ClipSerialNumberStyle) -> bool {
    let raw = unsafe {
        std::ptr::read_unaligned(std::ptr::addr_of!((*style).serial_number_type).cast::<i32>())
    };
    let numeric_raw = unsafe {
        std::ptr::read_unaligned(std::ptr::addr_of!((*style).numeric_type).cast::<i32>())
    };
    ClipSerialNumberType::from_raw(raw).is_some()
        && ClipSerialNumberNumericType::from_raw(numeric_raw).is_some()
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_get_watermark_config(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    out_config: *mut ClipWatermarkConfig,
) -> ClipError {
    ffi_error(|| {
        if out_config.is_null() {
            return ClipError::InvalidArgument;
        }
        ffi_status(with_runtime_viewport_ref(
            runtime,
            viewport,
            |runtime, _| {
                write_out(out_config, runtime.watermark_config().clone().into());
                Ok(())
            },
        ))
    })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_watermark_config_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    config: *const ClipWatermarkConfig,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if config.is_null() || out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }
        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let id = viewport_id(viewport)?;
            let result = state
                .runtime
                .set_viewport_watermark_config(id, unsafe { (*config).into() })
                .map_err(ClipError::from)?;
            write_changed_viewports(out_changed_viewports, result.changed_viewports);
            Ok(())
        }))
    })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_get_spotlight_config(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    out_config: *mut ClipSpotlightConfig,
) -> ClipError {
    ffi_error(|| {
        if out_config.is_null() {
            return ClipError::InvalidArgument;
        }
        ffi_status(with_runtime_viewport_ref(
            runtime,
            viewport,
            |runtime, _| {
                write_out(out_config, runtime.spotlight_config().into());
                Ok(())
            },
        ))
    })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_spotlight_config_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    config: *const ClipSpotlightConfig,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if config.is_null() || out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }
        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let id = viewport_id(viewport)?;
            let result = state
                .runtime
                .set_viewport_spotlight_config(id, unsafe { (*config).into() })
                .map_err(ClipError::from)?;
            write_changed_viewports(out_changed_viewports, result.changed_viewports);
            Ok(())
        }))
    })
}

#[cfg(test)]
#[allow(clippy::items_after_test_module)]
mod spotlight_export_tests {
    use super::*;

    #[test]
    fn spotlight_exports_reject_null_output_and_config_pointers() {
        unsafe {
            assert_eq!(
                clip_viewport_get_spotlight_config(
                    std::ptr::null_mut(),
                    std::ptr::null_mut(),
                    std::ptr::null_mut(),
                ),
                ClipError::InvalidArgument
            );
            assert_eq!(
                clip_viewport_set_spotlight_config_ex(
                    std::ptr::null_mut(),
                    std::ptr::null_mut(),
                    std::ptr::null(),
                    std::ptr::null_mut(),
                ),
                ClipError::InvalidArgument
            );
            let config = ClipSpotlightConfig::default();
            assert_eq!(
                clip_viewport_set_spotlight_config_ex(
                    std::ptr::null_mut(),
                    std::ptr::null_mut(),
                    &config,
                    std::ptr::null_mut(),
                ),
                ClipError::InvalidArgument
            );
        }
    }
}

/// # Safety
/// If `runtime` and `viewport` are non-null, they must be live handles created by this library.
/// `out_state` must be valid for writes of one `ClipStyleToolbarState` value.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_get_style_toolbar_state(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    out_state: *mut ClipStyleToolbarState,
) -> ClipError {
    ffi_error(|| {
        if out_state.is_null() {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_viewport_ref(
            runtime,
            viewport,
            |runtime, id| {
                let state = runtime
                    .viewport_style_toolbar_state(id)
                    .map_err(ClipError::from)?;
                write_out(
                    out_state,
                    ClipStyleToolbarState {
                        source: clip_style_toolbar_source_from_rust(state.source),
                        selected_element_count: state.selected_element_count,
                        shape_style: state.shape_style.into(),
                        text_style: state.text_style.into(),
                        serial_number_style: state.serial_number_style.into(),
                        text_style_mixed: state.text_style_mixed,
                        serial_number_style_mixed: state.serial_number_style_mixed,
                        shape_style_mixed: state.shape_style_mixed,
                        filter_style: state.filter_style.into(),
                        filter_style_mixed: state.filter_style_mixed,
                        brush_eraser_style: state.brush_eraser_style.into(),
                        distance_style: state.distance_style.into(),
                        distance_style_mixed: state.distance_style_mixed,
                    },
                );
                Ok(())
            },
        ))
    })
}

/// # Safety
/// Handles and output pointers must be live and writable; `style` must be readable.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_filter_style_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    style: *const ClipFilterStyle,
    properties: u32,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if style.is_null()
            || out_changed_viewports.is_null()
            || !unsafe { filter_style_type_is_valid(style) }
        {
            return ClipError::InvalidArgument;
        }
        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let id = viewport_id(viewport)?;
            let result = state
                .runtime
                .set_viewport_filter_style(id, unsafe { (*style).into() }, properties)
                .map_err(ClipError::from)?;
            write_changed_viewports(out_changed_viewports, result.changed_viewports);
            Ok(())
        }))
    })
}

/// # Safety
/// Handles and output pointers must be live and writable; `style` must be readable.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_filter_creation_style_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    style: *const ClipFilterStyle,
    properties: u32,
    // Decode the C enum as an integer so invalid discriminants can be rejected safely.
    tool: i32,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if style.is_null()
            || out_changed_viewports.is_null()
            || !unsafe { filter_style_type_is_valid(style) }
        {
            return ClipError::InvalidArgument;
        }
        let tool = match tool {
            value if value == ClipActiveTool::RectangleFilter as i32 => {
                clip_draw_engine::ActiveTool::RectangleFilter
            }
            value if value == ClipActiveTool::PenFilter as i32 => {
                clip_draw_engine::ActiveTool::PenFilter
            }
            _ => return ClipError::InvalidArgument,
        };
        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let id = viewport_id(viewport)?;
            let result = state
                .runtime
                .set_viewport_filter_creation_style(
                    id,
                    unsafe { (*style).into() },
                    properties,
                    tool,
                )
                .map_err(ClipError::from)?;
            write_changed_viewports(out_changed_viewports, result.changed_viewports);
            Ok(())
        }))
    })
}

#[cfg(test)]
mod filter_creation_style_tests {
    use super::*;

    #[test]
    fn filter_creation_style_export_preserves_state_and_output_on_rejection() {
        let mut state = ClipRuntimeImpl {
            runtime: clip_draw_engine::Runtime::new(clip_draw_engine::RuntimeConfig::default()),
        };
        let id = state
            .runtime
            .create_viewport(clip_draw_engine::ViewportConfig::default())
            .unwrap();
        let mut viewport = ClipViewportImpl { id };
        state
            .runtime
            .set_viewport_active_tool(id, clip_draw_engine::ActiveTool::Select)
            .unwrap();
        let before = state.runtime.serialize_document_session().unwrap();
        let style = ClipFilterStyle {
            filter_type: ClipFilterType::GaussianBlur,
            strength: 0.3,
            opacity: 0.6,
            stroke_width: 20.0,
        };
        let sentinel = std::ptr::dangling_mut::<ClipChangedViewportListImpl>();
        let mut changed = sentinel;
        unsafe {
            let mut invalid = Box::<ClipFilterStyle>::new_uninit();
            let invalid_ptr = invalid.as_mut_ptr();
            invalid_ptr.write(style);
            std::ptr::addr_of_mut!((*invalid_ptr).filter_type)
                .cast::<i32>()
                .write_unaligned(99);
            for (patch, properties, tool) in [
                (
                    &style as *const _,
                    u32::MAX,
                    ClipActiveTool::PenFilter as i32,
                ),
                (
                    &style as *const _,
                    clip_draw_engine::FILTER_STYLE_PROPERTY_ALL,
                    ClipActiveTool::Shape as i32,
                ),
                (
                    std::ptr::null(),
                    clip_draw_engine::FILTER_STYLE_PROPERTY_ALL,
                    ClipActiveTool::PenFilter as i32,
                ),
                (
                    invalid_ptr.cast_const(),
                    clip_draw_engine::FILTER_STYLE_PROPERTY_ALL,
                    ClipActiveTool::PenFilter as i32,
                ),
                (
                    &style as *const _,
                    clip_draw_engine::FILTER_STYLE_PROPERTY_ALL,
                    99,
                ),
            ] {
                assert_eq!(
                    clip_viewport_set_filter_creation_style_ex(
                        &mut state,
                        &mut viewport,
                        patch,
                        properties,
                        tool,
                        &mut changed
                    ),
                    ClipError::InvalidArgument
                );
                assert_eq!(changed, sentinel);
                assert_eq!(state.runtime.serialize_document_session().unwrap(), before);
            }
            assert_eq!(
                clip_viewport_set_filter_style_ex(
                    &mut state,
                    &mut viewport,
                    invalid_ptr,
                    clip_draw_engine::FILTER_STYLE_PROPERTY_ALL,
                    &mut changed
                ),
                ClipError::InvalidArgument
            );
            assert_eq!(changed, sentinel);
            assert_eq!(state.runtime.serialize_document_session().unwrap(), before);
            changed = std::ptr::null_mut();
            assert_eq!(
                clip_viewport_set_filter_creation_style_ex(
                    &mut state,
                    &mut viewport,
                    &style,
                    clip_draw_engine::FILTER_STYLE_PROPERTY_ALL,
                    ClipActiveTool::PenFilter as i32,
                    &mut changed
                ),
                ClipError::Ok
            );
            assert!(!changed.is_null());
            assert_eq!(
                state.runtime.viewport_active_tool(id).unwrap(),
                clip_draw_engine::ActiveTool::Select
            );
            assert!(!state.runtime.history_state().can_undo);
            crate::abi::exports::clip_changed_viewports_destroy(changed);
        }
    }
}

#[cfg(test)]
mod serial_number_style_type_tests {
    use super::*;

    #[test]
    fn invalid_raw_type_is_rejected_before_style_conversion() {
        unsafe {
            let valid = ClipSerialNumberStyle::default();
            assert!(serial_number_style_type_is_valid(&valid));

            let mut invalid = Box::<ClipSerialNumberStyle>::new_uninit();
            let invalid_ptr = invalid.as_mut_ptr();
            invalid_ptr.write(ClipSerialNumberStyle::default());
            std::ptr::addr_of_mut!((*invalid_ptr).serial_number_type)
                .cast::<i32>()
                .write_unaligned(99);
            assert!(!serial_number_style_type_is_valid(invalid_ptr));
            invalid_ptr.write(ClipSerialNumberStyle::default());
            std::ptr::addr_of_mut!((*invalid_ptr).numeric_type)
                .cast::<i32>()
                .write_unaligned(99);
            assert!(!serial_number_style_type_is_valid(invalid_ptr));
        }
    }
}

/// # Safety
/// If `runtime` and `viewport` are non-null, they must be live handles created by this library.
/// `out_state` must be valid for writes of one `ClipSerialNumberToolbarState` value.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_get_serial_number_toolbar_state(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    out_state: *mut ClipSerialNumberToolbarState,
) -> ClipError {
    ffi_error(|| {
        if out_state.is_null() {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_viewport_ref(
            runtime,
            viewport,
            |runtime, id| {
                let state = runtime
                    .viewport_serial_number_toolbar_state(id)
                    .map_err(ClipError::from)?;
                write_out(
                    out_state,
                    ClipSerialNumberToolbarState {
                        visible: u8::from(state.visible),
                        can_decrease: u8::from(state.can_decrease),
                        can_increase: u8::from(state.can_increase),
                        can_create_text: u8::from(state.can_create_text),
                        reserved0: [0; 4],
                        left: state.left,
                        top: state.top,
                        width: state.width,
                        height: state.height,
                    },
                );
                Ok(())
            },
        ))
    })
}

/// # Safety
/// If `runtime` and `viewport` are non-null, they must be live handles created by this library.
/// `style` must point to a readable `ClipShapeStyle` value.
/// `out_changed_viewports` must be valid for writes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_shape_style_patch_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    style: *const ClipShapeStyle,
    properties: u32,
    kind: ClipShapeKind,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if style.is_null() || out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let id = viewport_id(viewport)?;
            let result = state
                .runtime
                .set_viewport_shape_style_patch(
                    id,
                    clip_draw_engine::ShapeStylePatch {
                        kind: clip_shape_kind_to_rust(kind),
                        style: unsafe { (*style).into() },
                        properties,
                    },
                )
                .map_err(ClipError::from)?;
            write_changed_viewports(out_changed_viewports, result.changed_viewports);
            Ok(())
        }))
    })
}

/// # Safety
/// If `runtime` and `viewport` are non-null, they must be live handles created by this library.
/// `style` must point to a readable `ClipRectangleShapeStyle` value.
/// `out_changed_viewports` must be valid for writes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_rectangle_shape_style_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    style: *const ClipRectangleShapeStyle,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if style.is_null() || out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let id = viewport_id(viewport)?;
            let result = state
                .runtime
                .set_viewport_rectangle_shape_style(id, unsafe { (*style).into() })
                .map_err(ClipError::from)?;
            write_changed_viewports(out_changed_viewports, result.changed_viewports);
            Ok(())
        }))
    })
}

/// # Safety
/// If `runtime` and `viewport` are non-null, they must be live handles created by this library.
/// `style` must point to a readable `ClipTextStyle` value.
/// `out_changed_viewports` must be valid for writes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_text_style_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    style: *const ClipTextStyle,
    layouts: *const ClipTextLayoutOverride,
    layout_count: u32,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    unsafe {
        clip_viewport_patch_text_style_ex(
            runtime,
            viewport,
            style,
            clip_draw_engine::TEXT_STYLE_ALL_PROPERTIES,
            layouts,
            layout_count,
            out_changed_viewports,
        )
    }
}

/// # Safety
/// If `runtime` and `viewport` are non-null, they must be live handles created by this library.
/// `style` must point to a readable `ClipTextStyle` value.
/// `out_changed_viewports` must be valid for writes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_patch_text_style_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    style: *const ClipTextStyle,
    properties: u32,
    layouts: *const ClipTextLayoutOverride,
    layout_count: u32,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if style.is_null()
            || out_changed_viewports.is_null()
            || (layouts.is_null() && layout_count != 0)
        {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let id = viewport_id(viewport)?;
            let layout_slice = if layout_count == 0 {
                &[][..]
            } else {
                unsafe { std::slice::from_raw_parts(layouts, layout_count as usize) }
            };
            let layout_overrides = layout_slice
                .iter()
                .copied()
                .map(Into::into)
                .collect::<Vec<_>>();
            let result = state
                .runtime
                .set_viewport_text_style_patch(
                    id,
                    unsafe { (*style).into() },
                    properties,
                    &layout_overrides,
                )
                .map_err(ClipError::from)?;
            write_changed_viewports(out_changed_viewports, result.changed_viewports);
            Ok(())
        }))
    })
}

/// # Safety
/// If `runtime` and `viewport` are non-null, they must be live handles created by this library.
/// `style` must point to a readable `ClipSerialNumberStyle` value.
/// `out_changed_viewports` must be valid for writes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_serial_number_style_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    style: *const ClipSerialNumberStyle,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if style.is_null()
            || out_changed_viewports.is_null()
            || !unsafe { serial_number_style_type_is_valid(style) }
        {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let id = viewport_id(viewport)?;
            let result = state
                .runtime
                .set_viewport_serial_number_style(id, unsafe { (*style).into() })
                .map_err(ClipError::from)?;
            write_changed_viewports(out_changed_viewports, result.changed_viewports);
            Ok(())
        }))
    })
}

/// Apply only the specified serial-number properties.
///
/// # Safety
/// Handles and pointers must satisfy the same requirements as the full style setter.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_serial_number_style_patch_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    style: *const ClipSerialNumberStyle,
    properties: u32,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if style.is_null()
            || out_changed_viewports.is_null()
            || !unsafe { serial_number_style_type_is_valid(style) }
        {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let id = viewport_id(viewport)?;
            let result = state
                .runtime
                .set_viewport_serial_number_style_patch(id, unsafe { (*style).into() }, properties)
                .map_err(ClipError::from)?;
            write_changed_viewports(out_changed_viewports, result.changed_viewports);
            Ok(())
        }))
    })
}

/// Update future text defaults without touching selected objects or an active draft.
///
/// # Safety
/// Handles must be live, style readable and out_changed_viewports writable.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_text_creation_style_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    style: *const ClipTextStyle,
    properties: u32,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if style.is_null() || out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }
        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let id = viewport_id(viewport)?;
            let result = state
                .runtime
                .set_viewport_text_creation_style(id, unsafe { (*style).into() }, properties)
                .map_err(ClipError::from)?;
            write_changed_viewports(out_changed_viewports, result.changed_viewports);
            Ok(())
        }))
    })
}

/// # Safety
/// Handles must be live; style must be readable and output writable.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_brush_eraser_creation_style_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    style: *const ClipBrushEraserStyle,
    properties: u32,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if style.is_null() || out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }
        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let id = viewport_id(viewport)?;
            let result = state
                .runtime
                .set_viewport_brush_eraser_creation_style(
                    id,
                    unsafe { (*style).into() },
                    properties,
                )
                .map_err(ClipError::from)?;
            write_changed_viewports(out_changed_viewports, result.changed_viewports);
            Ok(())
        }))
    })
}

#[cfg(test)]
mod brush_eraser_tests {
    use super::*;

    #[test]
    fn eraser_filters_ffi_style_is_creation_only_and_rejection_preserves_output() {
        let mut state = ClipRuntimeImpl {
            runtime: clip_draw_engine::Runtime::default(),
        };
        let id = state
            .runtime
            .create_viewport(clip_draw_engine::ViewportConfig::default())
            .unwrap();
        let mut viewport = ClipViewportImpl { id };
        state
            .runtime
            .set_viewport_active_tool(id, clip_draw_engine::ActiveTool::BrushEraser)
            .unwrap();
        let before = state.runtime.serialize_document_session().unwrap();
        let sentinel = std::ptr::dangling_mut::<ClipChangedViewportListImpl>();
        let mut changed = sentinel;
        for (style, properties) in [
            (
                ClipBrushEraserStyle { stroke_width: 0.0 },
                CLIP_BRUSH_ERASER_STYLE_PROPERTY_STROKE_WIDTH,
            ),
            (
                ClipBrushEraserStyle {
                    stroke_width: f64::NAN,
                },
                CLIP_BRUSH_ERASER_STYLE_PROPERTY_STROKE_WIDTH,
            ),
            (ClipBrushEraserStyle::default(), u32::MAX),
        ] {
            assert_eq!(
                unsafe {
                    clip_viewport_set_brush_eraser_creation_style_ex(
                        &mut state,
                        &mut viewport,
                        &style,
                        properties,
                        &mut changed,
                    )
                },
                ClipError::InvalidArgument
            );
            assert_eq!(changed, sentinel);
            assert_eq!(state.runtime.serialize_document_session().unwrap(), before);
        }
        let style = ClipBrushEraserStyle { stroke_width: 42.0 };
        assert_eq!(
            unsafe {
                clip_viewport_set_brush_eraser_creation_style_ex(
                    &mut state,
                    &mut viewport,
                    &style,
                    CLIP_BRUSH_ERASER_STYLE_PROPERTY_STROKE_WIDTH,
                    &mut changed,
                )
            },
            ClipError::Ok
        );
        let toolbar = state.runtime.viewport_style_toolbar_state(id).unwrap();
        assert_eq!(toolbar.brush_eraser_style.stroke_width, 42.0);
        assert_eq!(
            clip_style_toolbar_source_from_rust(toolbar.source),
            ClipStyleToolbarSource::DefaultBrushEraser
        );
        assert_eq!(
            state.runtime.viewport_active_tool(id).unwrap(),
            clip_draw_engine::ActiveTool::BrushEraser
        );
        assert!(!state.runtime.history_state().can_undo);
        unsafe {
            crate::abi::exports::clip_changed_viewports_destroy(changed);
        }
        for tool in [ClipActiveTool::RectangleEraser, ClipActiveTool::BrushEraser] {
            assert_eq!(
                clip_active_tool_from_rust(clip_active_tool_to_rust(tool)),
                tool
            );
            assert_eq!(
                clip_active_tool_mask_to_rust(1 << tool as u32),
                clip_active_tool_to_rust(tool).policy_bit()
            );
        }
        assert_eq!(std::mem::size_of::<ClipBrushEraserStyle>(), 8);
        assert_eq!(ClipFilterType::RestoreBackground as u32, 7);
        assert_eq!(ClipActiveTool::RectangleEraser as u32, 15);
        assert_eq!(ClipActiveTool::BrushEraser as u32, 16);
    }
}
