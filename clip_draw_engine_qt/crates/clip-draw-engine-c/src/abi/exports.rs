use clip_draw_engine::{Point, Runtime, SnapGuideTargets, ViewportConfig};

use crate::abi::convert::*;
use crate::abi::handles::*;
use crate::abi::types::*;

/// # Safety
/// If `out_runtime` is non-null, it must be valid for writes of one `ClipRuntime` value.
/// The returned handle must later be released with `clip_runtime_destroy`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_create(out_runtime: *mut ClipRuntime) -> ClipError {
    unsafe { clip_runtime_create_with_config(std::ptr::null(), out_runtime) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_create_with_config(
    config: *const ClipRuntimeConfig,
    out_runtime: *mut ClipRuntime,
) -> ClipError {
    ffi_error(|| {
        if out_runtime.is_null() {
            return ClipError::InvalidArgument;
        }
        let config = match runtime_config_from_c(unsafe { config.as_ref() }) {
            Ok(config) => config,
            Err(error) => return error,
        };
        match Runtime::try_new(config) {
            Ok(runtime) => {
                write_out(
                    out_runtime,
                    Box::into_raw(Box::new(ClipRuntimeImpl { runtime })),
                );
                ClipError::Ok
            }
            Err(error) => ClipError::from(error),
        }
    })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_style_defaults_default(
    out_defaults: *mut ClipStyleDefaults,
) -> ClipError {
    ffi_error(|| {
        if out_defaults.is_null() {
            return ClipError::InvalidArgument;
        }
        write_out(
            out_defaults,
            clip_draw_engine::StyleDefaults::default().into(),
        );
        ClipError::Ok
    })
}

/// # Safety
/// If `source` is non-null, it must be a live handle returned by `clip_runtime_create`.
/// If `out_runtime` is non-null, it must be valid for writes of one `ClipRuntime` value.
/// The returned handle must later be released with `clip_runtime_destroy`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_clone_document_session_with_config(
    source: ClipRuntime,
    config: *const ClipRuntimeConfig,
    out_runtime: *mut ClipRuntime,
) -> ClipError {
    ffi_error(|| {
        if source.is_null() || config.is_null() || out_runtime.is_null() {
            return ClipError::InvalidArgument;
        }
        let config = match runtime_config_from_c(unsafe { config.as_ref() }) {
            Ok(config) => config,
            Err(error) => return error,
        };
        let source = unsafe { &*source };
        match source.runtime.clone_document_session_with_config(config) {
            Ok(runtime) => {
                write_out(
                    out_runtime,
                    Box::into_raw(Box::new(ClipRuntimeImpl { runtime })),
                );
                ClipError::Ok
            }
            Err(error) => ClipError::from(error),
        }
    })
}

/// Serializes the document/editor/history session. Call with a null buffer and zero
/// capacity to query the required byte count.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_serialize_document_session(
    runtime: ClipRuntime,
    buffer: *mut u8,
    buffer_capacity: usize,
    out_size: *mut usize,
) -> ClipError {
    ffi_error(|| {
        if runtime.is_null() || out_size.is_null() || (buffer.is_null() && buffer_capacity != 0) {
            return ClipError::InvalidArgument;
        }
        let runtime = unsafe { &*runtime };
        let bytes = match runtime.runtime.serialize_document_session() {
            Ok(bytes) => bytes,
            Err(error) => return ClipError::from(error),
        };
        write_out(out_size, bytes.len());
        if buffer.is_null() {
            return ClipError::Ok;
        }
        if buffer_capacity < bytes.len() {
            return ClipError::BufferTooSmall;
        }
        unsafe {
            std::ptr::copy_nonoverlapping(bytes.as_ptr(), buffer, bytes.len());
        }
        ClipError::Ok
    })
}

/// Serializes once into an immutable snapshot independent of the runtime.
///
/// # Safety
/// `runtime` must be a live runtime and `out_bytes` must be writable. Release the
/// returned handle with `clip_serialized_bytes_destroy`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_serialize_document_session_bytes(
    runtime: ClipRuntime,
    out_bytes: *mut ClipSerializedBytes,
) -> ClipError {
    ffi_error(|| {
        if out_bytes.is_null() {
            return ClipError::InvalidArgument;
        }
        write_out(out_bytes, std::ptr::null_mut());
        let bytes = match with_runtime_ref(runtime, |runtime| {
            runtime
                .serialize_document_session()
                .map_err(ClipError::from)
        }) {
            Ok(bytes) => bytes,
            Err(error) => return error,
        };
        write_out(
            out_bytes,
            Box::into_raw(Box::new(ClipSerializedBytesImpl { bytes })),
        );
        ClipError::Ok
    })
}

/// # Safety
/// `bytes` must be a live snapshot handle. Output pointers must be writable.
/// The returned data is read-only and remains valid until the handle is destroyed.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_serialized_bytes_data(
    bytes: ClipSerializedBytes,
    out_data: *mut *const u8,
    out_size: *mut usize,
) -> ClipError {
    ffi_error(|| {
        if bytes.is_null() || out_data.is_null() || out_size.is_null() {
            return ClipError::InvalidArgument;
        }
        let bytes = unsafe { &(*bytes).bytes };
        write_out(out_data, bytes.as_ptr());
        write_out(out_size, bytes.len());
        ClipError::Ok
    })
}

/// # Safety
/// A non-null handle must be live and must not have been destroyed previously.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_serialized_bytes_destroy(bytes: ClipSerializedBytes) {
    ffi_void(|| {
        if !bytes.is_null() {
            drop(unsafe { Box::from_raw(bytes) });
        }
    });
}

/// Serializes the selected editable elements as a versioned draw template.
/// A null buffer with zero capacity queries the required byte count.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_serialize_selected_draw_template(
    runtime: ClipRuntime,
    buffer: *mut u8,
    buffer_capacity: usize,
    out_size: *mut usize,
) -> ClipError {
    ffi_error(|| {
        if runtime.is_null() || out_size.is_null() || (buffer.is_null() && buffer_capacity != 0) {
            return ClipError::InvalidArgument;
        }
        let runtime = unsafe { &*runtime };
        let bytes = match runtime.runtime.serialize_selected_draw_template() {
            Ok(bytes) => bytes,
            Err(error) => return ClipError::from(error),
        };
        write_out(out_size, bytes.len());
        if buffer.is_null() {
            return ClipError::Ok;
        }
        if buffer_capacity < bytes.len() {
            return ClipError::BufferTooSmall;
        }
        unsafe { std::ptr::copy_nonoverlapping(bytes.as_ptr(), buffer, bytes.len()) };
        ClipError::Ok
    })
}

/// Serializes only selected element identifiers, without document or image payloads.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_serialize_selected_element_ids(
    runtime: ClipRuntime,
    buffer: *mut u8,
    buffer_capacity: usize,
    out_size: *mut usize,
) -> ClipError {
    ffi_error(|| {
        if runtime.is_null() || out_size.is_null() || (buffer.is_null() && buffer_capacity != 0) {
            return ClipError::InvalidArgument;
        }
        let runtime = unsafe { &*runtime };
        let Ok(bytes) = runtime.runtime.serialize_selected_element_ids() else {
            return ClipError::InvalidArgument;
        };
        write_out(out_size, bytes.len());
        if buffer.is_null() {
            return ClipError::Ok;
        }
        if buffer_capacity < bytes.len() {
            return ClipError::BufferTooSmall;
        }
        unsafe { std::ptr::copy_nonoverlapping(bytes.as_ptr(), buffer, bytes.len()) };
        ClipError::Ok
    })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_create_from_document_session_with_config(
    bytes: *const u8,
    size: usize,
    config: *const ClipRuntimeConfig,
    out_runtime: *mut ClipRuntime,
) -> ClipError {
    ffi_error(|| {
        if bytes.is_null() || size == 0 || config.is_null() || out_runtime.is_null() {
            return ClipError::InvalidArgument;
        }
        let config = match runtime_config_from_c(unsafe { config.as_ref() }) {
            Ok(config) => config,
            Err(error) => return error,
        };
        let bytes = unsafe { std::slice::from_raw_parts(bytes, size) };
        match Runtime::from_serialized_document_session_with_config(bytes, config) {
            Ok(runtime) => {
                write_out(
                    out_runtime,
                    Box::into_raw(Box::new(ClipRuntimeImpl { runtime })),
                );
                ClipError::Ok
            }
            Err(error) => ClipError::from(error),
        }
    })
}

/// Serializes only the document elements and undo/redo history. Call with a
/// null buffer and zero capacity to query the required byte count.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_serialize_document_history(
    runtime: ClipRuntime,
    buffer: *mut u8,
    buffer_capacity: usize,
    out_size: *mut usize,
) -> ClipError {
    ffi_error(|| {
        if runtime.is_null() || out_size.is_null() || (buffer.is_null() && buffer_capacity != 0) {
            return ClipError::InvalidArgument;
        }
        let runtime = unsafe { &*runtime };
        let bytes = match runtime.runtime.serialize_document_history() {
            Ok(bytes) => bytes,
            Err(error) => return ClipError::from(error),
        };
        write_out(out_size, bytes.len());
        if buffer.is_null() {
            return ClipError::Ok;
        }
        if buffer_capacity < bytes.len() {
            return ClipError::BufferTooSmall;
        }
        unsafe {
            std::ptr::copy_nonoverlapping(bytes.as_ptr(), buffer, bytes.len());
        }
        ClipError::Ok
    })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_create_from_document_history_with_config(
    bytes: *const u8,
    size: usize,
    config: *const ClipRuntimeConfig,
    out_runtime: *mut ClipRuntime,
) -> ClipError {
    ffi_error(|| {
        if bytes.is_null() || size == 0 || config.is_null() || out_runtime.is_null() {
            return ClipError::InvalidArgument;
        }
        let config = match runtime_config_from_c(unsafe { config.as_ref() }) {
            Ok(config) => config,
            Err(error) => return error,
        };
        let bytes = unsafe { std::slice::from_raw_parts(bytes, size) };
        match Runtime::from_serialized_document_history_with_config(bytes, config) {
            Ok(runtime) => {
                write_out(
                    out_runtime,
                    Box::into_raw(Box::new(ClipRuntimeImpl { runtime })),
                );
                ClipError::Ok
            }
            Err(error) => ClipError::from(error),
        }
    })
}

/// # Safety
/// If `runtime` is non-null, it must be a live handle returned by `clip_runtime_create`.
/// The handle must not be used again after this call.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_destroy(runtime: ClipRuntime) {
    ffi_void(|| {
        if runtime.is_null() {
            return;
        }
        unsafe {
            drop(Box::from_raw(runtime));
        }
    })
}

/// # Safety
/// `runtime` must be a live runtime handle. Balance every successful begin
/// with an end, including when a mutation or measurement fails.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_begin_presentation_update(runtime: ClipRuntime) -> ClipError {
    ffi_error(|| {
        ffi_status(with_runtime_impl_mut(runtime, |state| {
            state
                .runtime
                .begin_presentation_update()
                .map_err(ClipError::from)
        }))
    })
}

/// # Safety
/// `runtime` must be a live runtime handle and `out_changed_viewports` writable.
/// Release the returned list with `clip_changed_viewports_destroy`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_end_presentation_update_ex(
    runtime: ClipRuntime,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }
        write_out(out_changed_viewports, std::ptr::null_mut());
        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let result = state
                .runtime
                .end_presentation_update()
                .map_err(ClipError::from)?;
            write_changed_viewports(out_changed_viewports, result.changed_viewports);
            Ok(())
        }))
    })
}

/// # Safety
/// If `runtime` is non-null, it must be a live handle returned by `clip_runtime_create`.
/// `out_changed_viewports` must be valid for writes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_set_quick_selection_disabled_tools_ex(
    runtime: ClipRuntime,
    tools: u64,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let result = state
                .runtime
                .set_quick_selection_disabled_tools(clip_active_tool_mask_to_rust(tools))
                .map_err(ClipError::from)?;
            write_changed_viewports(out_changed_viewports, result.changed_viewports);
            Ok(())
        }))
    })
}

/// # Safety
/// If `runtime` is non-null, it must be a live `ClipRuntime` handle.
/// `config` must point to a readable `ClipEngineConfig` and `out_viewport` must be writable.
/// The returned viewport handle must later be released with `clip_viewport_destroy`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_create(
    runtime: ClipRuntime,
    config: *const ClipEngineConfig,
    out_viewport: *mut ClipViewport,
) -> ClipError {
    ffi_error(|| {
        if config.is_null() || out_viewport.is_null() {
            return ClipError::InvalidArgument;
        }

        let engine = clip_engine_config_to_rust(unsafe { &*config });

        match with_runtime_impl_mut(runtime, |state| {
            let id = state
                .runtime
                .create_viewport(ViewportConfig { engine })
                .map_err(ClipError::from)?;
            write_out(
                out_viewport,
                Box::into_raw(Box::new(ClipViewportImpl { id })),
            );
            Ok(())
        }) {
            Ok(()) => ClipError::Ok,
            Err(error) => error,
        }
    })
}

/// # Safety
/// If `runtime` is non-null, it must be a live `ClipRuntime` handle.
/// If `viewport` is non-null, it must be a live handle created by `clip_viewport_create`.
/// The viewport handle must not be used again after this call.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_destroy(runtime: ClipRuntime, viewport: ClipViewport) {
    ffi_void(|| {
        if viewport.is_null() {
            return;
        }

        let id = unsafe { (*viewport).id };
        if !runtime.is_null() {
            let _ = with_runtime_impl_mut(runtime, |state| {
                state
                    .runtime
                    .destroy_viewport(id)
                    .map_err(ClipError::from)?;
                Ok(())
            });
        }

        unsafe {
            drop(Box::from_raw(viewport));
        }
    })
}

/// # Safety
/// If `runtime` and `viewport` are non-null, they must be live handles created by this library.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_surface_size(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    width: u32,
    height: u32,
) -> ClipError {
    ffi_error(|| {
        ffi_status(with_runtime_viewport_mut(
            runtime,
            viewport,
            |runtime, id| {
                runtime
                    .set_viewport_surface_size(id, width, height)
                    .map(|_| ())
                    .map_err(ClipError::from)
            },
        ))
    })
}

/// # Safety
/// If `runtime` and `viewport` are non-null, they must be live handles created by this library.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_camera(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    center_x: f64,
    center_y: f64,
    zoom: f64,
) -> ClipError {
    ffi_error(|| {
        ffi_status(with_runtime_viewport_mut(
            runtime,
            viewport,
            |runtime, id| {
                runtime
                    .set_viewport_camera(
                        id,
                        clip_draw_engine::Camera {
                            center: Point {
                                x: center_x,
                                y: center_y,
                            },
                            zoom,
                        },
                    )
                    .map(|_| ())
                    .map_err(ClipError::from)
            },
        ))
    })
}

/// # Safety
/// If `viewport` is non-null, it must be a live handle created by this library.
/// `out_id` must be valid for writes of one `uint64_t` value.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_get_id(
    viewport: ClipViewport,
    out_id: *mut u64,
) -> ClipError {
    ffi_error(|| {
        if out_id.is_null() {
            return ClipError::InvalidArgument;
        }
        match viewport_id(viewport) {
            Ok(id) => {
                write_out(out_id, id.0);
                ClipError::Ok
            }
            Err(error) => error,
        }
    })
}

/// # Safety
/// If `runtime` and `viewport` are non-null, they must be live handles created by this library.
/// `out_changed_viewports` must be valid for writes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_active_tool_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    tool: ClipActiveTool,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_viewport_mut(
            runtime,
            viewport,
            |runtime, id| {
                let result = runtime
                    .set_viewport_active_tool(id, clip_active_tool_to_rust(tool))
                    .map_err(ClipError::from)?;
                write_changed_viewports(out_changed_viewports, result.changed_viewports);
                Ok(())
            },
        ))
    })
}

/// # Safety
/// If `runtime` and `viewport` are non-null, they must be live handles created by this library.
/// `out_tool` must be valid for writes of one `ClipActiveTool` value.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_get_active_tool(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    out_tool: *mut ClipActiveTool,
) -> ClipError {
    ffi_error(|| {
        if out_tool.is_null() {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_viewport_ref(
            runtime,
            viewport,
            |runtime, id| {
                let tool = runtime.viewport_active_tool(id).map_err(ClipError::from)?;
                write_out(out_tool, clip_active_tool_from_rust(tool));
                Ok(())
            },
        ))
    })
}

/// # Safety
/// If `runtime` and `viewport` are non-null, they must be live handles created by this library.
/// `out_config` must be valid for writes of one `ClipSnapConfig` value.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_get_snap_config(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    out_config: *mut ClipSnapConfig,
) -> ClipError {
    ffi_error(|| {
        if out_config.is_null() {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_viewport_ref(
            runtime,
            viewport,
            |runtime, id| {
                write_out(
                    out_config,
                    runtime
                        .viewport_snap_config(id)
                        .map_err(ClipError::from)?
                        .into(),
                );
                Ok(())
            },
        ))
    })
}

/// # Safety
/// If `runtime` and `viewport` are non-null, they must be live handles created by this library.
/// `config` must point to a readable `ClipSnapConfig` value.
/// `out_changed_viewports` must be valid for writes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_snap_config_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    config: *const ClipSnapConfig,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if config.is_null() || out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_viewport_mut(
            runtime,
            viewport,
            |runtime, id| {
                let result = runtime
                    .set_viewport_snap_config(id, unsafe { (*config).into() })
                    .map_err(ClipError::from)?;
                write_changed_viewports(out_changed_viewports, result.changed_viewports);
                Ok(())
            },
        ))
    })
}

/// # Safety
/// Non-null arrays must be readable for their supplied lengths; the output must be writable.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_snap_guide_targets_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    vertical_xs: *const f64,
    vertical_count: usize,
    horizontal_ys: *const f64,
    horizontal_count: usize,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if out_changed_viewports.is_null()
            || vertical_count > 2
            || horizontal_count > 2
            || (vertical_count > 0 && vertical_xs.is_null())
            || (horizontal_count > 0 && horizontal_ys.is_null())
        {
            return ClipError::InvalidArgument;
        }
        let mut targets = SnapGuideTargets::default();
        for index in 0..vertical_count {
            targets.vertical_xs[index] = Some(unsafe { *vertical_xs.add(index) });
        }
        for index in 0..horizontal_count {
            targets.horizontal_ys[index] = Some(unsafe { *horizontal_ys.add(index) });
        }
        ffi_status(with_runtime_viewport_mut(
            runtime,
            viewport,
            |runtime, id| {
                let result = runtime
                    .set_viewport_snap_guide_targets(id, targets)
                    .map_err(ClipError::from)?;
                write_changed_viewports(out_changed_viewports, result.changed_viewports);
                Ok(())
            },
        ))
    })
}

/// # Safety
/// If `runtime` and `viewport` are non-null, they must be live handles created by this library.
/// `out_config` must be valid for writes of one `ClipGridConfig` value.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_get_grid_config(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    out_config: *mut ClipGridConfig,
) -> ClipError {
    ffi_error(|| {
        if out_config.is_null() {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_viewport_ref(
            runtime,
            viewport,
            |runtime, id| {
                write_out(
                    out_config,
                    runtime
                        .viewport_grid_config(id)
                        .map_err(ClipError::from)?
                        .into(),
                );
                Ok(())
            },
        ))
    })
}

/// # Safety
/// If `runtime` and `viewport` are non-null, they must be live handles created by this library.
/// `config` must point to a readable `ClipGridConfig` value.
/// `out_changed_viewports` must be valid for writes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_set_grid_config_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    config: *const ClipGridConfig,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if config.is_null() || out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_viewport_mut(
            runtime,
            viewport,
            |runtime, id| {
                let result = runtime
                    .set_viewport_grid_config(id, unsafe { (*config).into() })
                    .map_err(ClipError::from)?;
                write_changed_viewports(out_changed_viewports, result.changed_viewports);
                Ok(())
            },
        ))
    })
}

/// # Safety
/// If `changed_viewports` is non-null, it must be a live handle returned by this library.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_changed_viewports_destroy(
    changed_viewports: ClipChangedViewportList,
) {
    ffi_void(|| {
        if !changed_viewports.is_null() {
            drop(unsafe { Box::from_raw(changed_viewports) });
        }
    })
}

/// # Safety
/// If `changed_viewports` is non-null, it must be a live handle returned by this library.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_changed_viewports_count(
    changed_viewports: ClipChangedViewportList,
) -> u32 {
    ffi_value(0, || {
        if changed_viewports.is_null() {
            return 0;
        }
        let changed_viewports = unsafe { &*changed_viewports };
        changed_viewports.viewport_ids.len().min(u32::MAX as usize) as u32
    })
}

/// # Safety
/// If `changed_viewports` is non-null, it must be a live handle returned by this library.
/// `out_viewport_id` must be valid for writes of one `uint64_t` value.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_changed_viewports_get(
    changed_viewports: ClipChangedViewportList,
    index: u32,
    out_viewport_id: *mut u64,
) -> ClipError {
    ffi_error(|| {
        if changed_viewports.is_null() || out_viewport_id.is_null() {
            return ClipError::InvalidArgument;
        }
        let changed_viewports = unsafe { &*changed_viewports };
        let Some(viewport_id) = changed_viewports.viewport_ids.get(index as usize) else {
            return ClipError::NotFound;
        };
        write_out(out_viewport_id, *viewport_id);
        ClipError::Ok
    })
}

#[cfg(test)]
mod session_tests {
    use super::*;

    #[test]
    fn session_bytes_snapshot_outlives_runtime_and_rejects_bad_input() {
        unsafe {
            let mut runtime = std::ptr::null_mut();
            assert_eq!(clip_runtime_create(&mut runtime), ClipError::Ok);
            let expected = (*runtime).runtime.serialize_document_session().unwrap();
            let mut snapshot = std::ptr::null_mut();
            assert_eq!(
                clip_runtime_serialize_document_session_bytes(runtime, &mut snapshot),
                ClipError::Ok
            );
            assert!(!snapshot.is_null());
            assert_eq!(
                clip_runtime_serialize_document_session_bytes(runtime, std::ptr::null_mut()),
                ClipError::InvalidArgument
            );
            clip_runtime_destroy(runtime);
            let mut data = std::ptr::null();
            let mut size = 0;
            assert_eq!(
                clip_serialized_bytes_data(snapshot, &mut data, &mut size),
                ClipError::Ok
            );
            assert_eq!(std::slice::from_raw_parts(data, size), expected);
            assert_eq!(
                clip_serialized_bytes_data(snapshot, std::ptr::null_mut(), &mut size),
                ClipError::InvalidArgument
            );
            assert_eq!(
                clip_serialized_bytes_data(snapshot, &mut data, std::ptr::null_mut()),
                ClipError::InvalidArgument
            );
            assert_eq!(
                clip_serialized_bytes_data(std::ptr::null_mut(), &mut data, &mut size),
                ClipError::InvalidArgument
            );
            clip_serialized_bytes_destroy(snapshot);
            // Failure must clear an output slot rather than leave a stale handle.
            assert_eq!(
                clip_runtime_serialize_document_session_bytes(std::ptr::null_mut(), &mut snapshot),
                ClipError::InvalidArgument
            );
            assert!(snapshot.is_null());
            clip_serialized_bytes_destroy(std::ptr::null_mut());
        }
    }
    use clip_draw_engine::ActiveTool;

    #[test]
    fn selected_element_ids_abi_is_bounded_and_handles_empty_selection() {
        unsafe {
            let mut runtime = std::ptr::null_mut();
            assert_eq!(clip_runtime_create(&mut runtime), ClipError::Ok);
            let mut required = 0;
            assert_eq!(
                clip_runtime_serialize_selected_element_ids(
                    runtime,
                    std::ptr::null_mut(),
                    0,
                    &mut required
                ),
                ClipError::Ok
            );
            assert_eq!(required, 2);
            let mut bytes = [0u8; 2];
            assert_eq!(
                clip_runtime_serialize_selected_element_ids(
                    runtime,
                    bytes.as_mut_ptr(),
                    1,
                    &mut required
                ),
                ClipError::BufferTooSmall
            );
            assert_eq!(
                clip_runtime_serialize_selected_element_ids(
                    runtime,
                    bytes.as_mut_ptr(),
                    bytes.len(),
                    &mut required
                ),
                ClipError::Ok
            );
            assert_eq!(&bytes, b"[]");
            assert_eq!(
                clip_runtime_serialize_selected_element_ids(
                    std::ptr::null_mut(),
                    std::ptr::null_mut(),
                    0,
                    &mut required
                ),
                ClipError::InvalidArgument
            );
            clip_runtime_destroy(runtime);
        }
    }

    #[test]
    fn session_abi_is_two_pass_and_rejects_bad_input() {
        unsafe {
            let mut defaults = clip_draw_engine::StyleDefaults::default().into();
            assert_eq!(
                clip_runtime_style_defaults_default(&mut defaults),
                ClipError::Ok
            );
            let config = ClipRuntimeConfig {
                style_defaults: &defaults,
            };

            let mut runtime = std::ptr::null_mut();
            assert_eq!(clip_runtime_create(&mut runtime), ClipError::Ok);

            let mut required = 0usize;
            assert_eq!(
                clip_runtime_serialize_document_session(
                    runtime,
                    std::ptr::null_mut(),
                    0,
                    &mut required,
                ),
                ClipError::Ok
            );
            assert!(required > 0);

            let mut too_small = vec![0u8; required - 1];
            let mut reported = 0usize;
            assert_eq!(
                clip_runtime_serialize_document_session(
                    runtime,
                    too_small.as_mut_ptr(),
                    too_small.len(),
                    &mut reported,
                ),
                ClipError::BufferTooSmall
            );
            assert_eq!(reported, required);

            let mut payload = vec![0u8; required];
            assert_eq!(
                clip_runtime_serialize_document_session(
                    runtime,
                    payload.as_mut_ptr(),
                    payload.len(),
                    &mut reported,
                ),
                ClipError::Ok
            );
            let mut restored = std::ptr::null_mut();
            assert_eq!(
                clip_runtime_create_from_document_session_with_config(
                    payload.as_ptr(),
                    payload.len(),
                    &config,
                    &mut restored,
                ),
                ClipError::Ok
            );
            assert!(!restored.is_null());

            let mut history_required = 0usize;
            assert_eq!(
                clip_runtime_serialize_document_history(
                    runtime,
                    std::ptr::null_mut(),
                    0,
                    &mut history_required,
                ),
                ClipError::Ok
            );
            assert!(history_required > 0);
            let mut history_payload = vec![0u8; history_required];
            assert_eq!(
                clip_runtime_serialize_document_history(
                    runtime,
                    history_payload.as_mut_ptr(),
                    history_payload.len(),
                    &mut history_required,
                ),
                ClipError::Ok
            );
            let mut history_restored = std::ptr::null_mut();
            assert_eq!(
                clip_runtime_create_from_document_history_with_config(
                    history_payload.as_ptr(),
                    history_payload.len(),
                    &config,
                    &mut history_restored,
                ),
                ClipError::Ok
            );
            assert!(!history_restored.is_null());

            let malformed = br#"{"schemaVersion":999}"#;
            let mut rejected = std::ptr::null_mut();
            assert_ne!(
                clip_runtime_create_from_document_session_with_config(
                    malformed.as_ptr(),
                    malformed.len(),
                    &config,
                    &mut rejected,
                ),
                ClipError::Ok
            );
            assert!(rejected.is_null());

            clip_runtime_destroy(restored);
            clip_runtime_destroy(history_restored);
            clip_runtime_destroy(runtime);
        }
    }

    #[test]
    fn configured_runtime_creation_is_atomic() {
        unsafe {
            let mut defaults: ClipStyleDefaults = clip_draw_engine::StyleDefaults::default().into();
            assert_eq!(
                clip_runtime_style_defaults_default(&mut defaults),
                ClipError::Ok
            );
            defaults.text.font_size = 37.0;
            defaults.rectangle_filter.stroke_width = 0.0;
            defaults.pen_filter.stroke_width = 31.0;
            let config = ClipRuntimeConfig {
                style_defaults: &defaults,
            };
            let mut runtime = std::ptr::null_mut();
            assert_eq!(
                clip_runtime_create_with_config(&config, &mut runtime),
                ClipError::Ok
            );
            assert!(!runtime.is_null());
            clip_runtime_destroy(runtime);

            defaults.spotlight.opacity = 1.5;
            let invalid_config = ClipRuntimeConfig {
                style_defaults: &defaults,
            };
            let mut rejected = std::ptr::null_mut();
            assert_eq!(
                clip_runtime_create_with_config(&invalid_config, &mut rejected),
                ClipError::InvalidArgument
            );
            assert!(rejected.is_null());
        }
    }

    #[test]
    fn configured_runtime_creation_accepts_smart_erase_filter_defaults() {
        unsafe {
            let mut defaults: ClipStyleDefaults = clip_draw_engine::StyleDefaults::default().into();
            defaults.rectangle_filter.filter_type = ClipFilterType::SmartErase;
            defaults.pen_filter.filter_type = ClipFilterType::SmartErase;
            let config = ClipRuntimeConfig {
                style_defaults: &defaults,
            };
            let mut runtime = std::ptr::null_mut();
            assert_eq!(
                clip_runtime_create_with_config(&config, &mut runtime),
                ClipError::Ok
            );
            assert!(!runtime.is_null());
            clip_runtime_destroy(runtime);
        }
    }

    #[test]
    fn configured_runtime_creation_rejects_invalid_raw_enum() {
        unsafe {
            let mut defaults = Box::<ClipStyleDefaults>::new_uninit();
            let defaults_ptr = defaults.as_mut_ptr();
            defaults_ptr.write(clip_draw_engine::StyleDefaults::default().into());
            std::ptr::addr_of_mut!((*defaults_ptr).arrow.start_arrowhead)
                .cast::<i32>()
                .write_unaligned(99);

            let config = ClipRuntimeConfig {
                style_defaults: defaults_ptr,
            };
            let mut runtime = std::ptr::null_mut();
            assert_eq!(
                clip_runtime_create_with_config(&config, &mut runtime),
                ClipError::InvalidArgument
            );
            assert!(runtime.is_null());

            let mut serial_defaults = Box::<ClipStyleDefaults>::new_uninit();
            let serial_defaults_ptr = serial_defaults.as_mut_ptr();
            serial_defaults_ptr.write(clip_draw_engine::StyleDefaults::default().into());
            std::ptr::addr_of_mut!((*serial_defaults_ptr).serial_number.serial_number_type)
                .cast::<i32>()
                .write_unaligned(99);
            let serial_config = ClipRuntimeConfig {
                style_defaults: serial_defaults_ptr,
            };
            assert_eq!(
                clip_runtime_create_with_config(&serial_config, &mut runtime),
                ClipError::InvalidArgument
            );
            assert!(runtime.is_null());
        }
    }

    #[test]
    fn quick_selection_tool_mask_uses_the_stable_c_enum_layout() {
        let c_mask = (1_u64 << ClipActiveTool::FreeDraw as u32)
            | (1_u64 << ClipActiveTool::PenFilter as u32);
        let rust_mask = clip_active_tool_mask_to_rust(c_mask);

        assert_eq!(
            rust_mask,
            ActiveTool::FreeDraw.policy_bit() | ActiveTool::PenFilter.policy_bit()
        );
        assert_eq!(rust_mask & ActiveTool::Text.policy_bit(), 0);
        assert_eq!(rust_mask & ActiveTool::Spotlight.policy_bit(), 0);
    }
}
