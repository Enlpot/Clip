use crate::abi::handles::*;
use crate::abi::types::*;

/// # Safety
/// If `runtime` is non-null, it must be a live handle returned by `clip_runtime_create`.
/// `out_state` must be valid for writes of one `ClipHistoryState` value.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_get_history_state(
    runtime: ClipRuntime,
    out_state: *mut ClipHistoryState,
) -> ClipError {
    ffi_error(|| {
        if out_state.is_null() {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_ref(runtime, |runtime| {
            write_out(out_state, runtime.history_state().into());
            Ok(())
        }))
    })
}

/// # Safety
/// If `runtime` is non-null, it must be a live handle returned by `clip_runtime_create`.
/// `out_changed_viewports` must be valid for writes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_clear_document_preserving_viewports(
    runtime: ClipRuntime,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let result = state
                .runtime
                .clear_document_preserving_viewports()
                .map_err(ClipError::from)?;
            write_changed_viewports(out_changed_viewports, result.changed_viewports);
            Ok(())
        }))
    })
}

/// # Safety
/// If `runtime` is non-null, it must be a live handle returned by `clip_runtime_create`.
/// `bytes` must point to `size` readable bytes containing a serialized document history.
/// `out_changed_viewports` must be valid for writes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_runtime_restore_document_history_preserving_editor_styles(
    runtime: ClipRuntime,
    bytes: *const u8,
    size: usize,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if bytes.is_null() || size == 0 || out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }
        let bytes = unsafe { std::slice::from_raw_parts(bytes, size) };

        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let result = state
                .runtime
                .restore_document_history_preserving_editor_styles(bytes)
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
pub unsafe extern "C" fn clip_runtime_undo_ex(
    runtime: ClipRuntime,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let result = state
                .runtime
                .undo_with_viewport_changes()
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
pub unsafe extern "C" fn clip_runtime_redo_ex(
    runtime: ClipRuntime,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let result = state
                .runtime
                .redo_with_viewport_changes()
                .map_err(ClipError::from)?;
            write_changed_viewports(out_changed_viewports, result.changed_viewports);
            Ok(())
        }))
    })
}
