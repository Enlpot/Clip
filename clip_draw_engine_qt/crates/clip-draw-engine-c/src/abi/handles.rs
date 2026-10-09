use std::panic::{AssertUnwindSafe, catch_unwind};

use clip_draw_engine::{Runtime, ViewportId};

use crate::abi::patch::ClipPatchPayload;
use crate::abi::types::ClipError;

pub type ClipRuntime = *mut ClipRuntimeImpl;
pub type ClipViewport = *mut ClipViewportImpl;
pub type ClipPatchHandle = *mut ClipPatchHandleImpl;
pub type ClipChangedViewportList = *mut ClipChangedViewportListImpl;
pub type ClipSerializedBytes = *mut ClipSerializedBytesImpl;

pub struct ClipSerializedBytesImpl {
    pub(crate) bytes: Vec<u8>,
}

pub struct ClipRuntimeImpl {
    pub(crate) runtime: Runtime,
}

pub struct ClipViewportImpl {
    pub(crate) id: ViewportId,
}

pub struct ClipPatchHandleImpl {
    pub(crate) payload: ClipPatchPayload,
}

pub struct ClipChangedViewportListImpl {
    pub(crate) viewport_ids: Box<[u64]>,
}

pub(crate) fn ffi_status(result: Result<(), ClipError>) -> ClipError {
    match result {
        Ok(()) => ClipError::Ok,
        Err(error) => error,
    }
}

pub(crate) fn ffi_error(f: impl FnOnce() -> ClipError) -> ClipError {
    match catch_unwind(AssertUnwindSafe(f)) {
        Ok(error) => error,
        Err(_) => ClipError::Internal,
    }
}

pub(crate) fn ffi_value<T>(fallback: T, f: impl FnOnce() -> T) -> T {
    match catch_unwind(AssertUnwindSafe(f)) {
        Ok(value) => value,
        Err(_) => fallback,
    }
}

pub(crate) fn ffi_void(f: impl FnOnce()) {
    let _ = catch_unwind(AssertUnwindSafe(f));
}

fn slice_ptr_or_null<T>(slice: &[T]) -> *const T {
    if slice.is_empty() {
        std::ptr::null()
    } else {
        slice.as_ptr()
    }
}

pub(crate) fn write_slice_out<T>(out_items: *mut *const T, out_count: *mut u32, items: &[T]) {
    write_out(out_items, slice_ptr_or_null(items));
    write_out(out_count, items.len() as u32);
}

pub(crate) fn write_changed_viewports(
    out_changed_viewports: *mut ClipChangedViewportList,
    changed_viewports: Vec<ViewportId>,
) {
    let viewport_ids = changed_viewports
        .into_iter()
        .map(|viewport_id| viewport_id.0)
        .collect::<Vec<_>>()
        .into_boxed_slice();
    write_out(
        out_changed_viewports,
        Box::into_raw(Box::new(ClipChangedViewportListImpl { viewport_ids })),
    );
}

pub(crate) fn write_out<T>(ptr: *mut T, value: T) {
    unsafe {
        *ptr = value;
    }
}

pub(crate) fn with_runtime_impl_mut<T>(
    runtime: ClipRuntime,
    f: impl FnOnce(&mut ClipRuntimeImpl) -> Result<T, ClipError>,
) -> Result<T, ClipError> {
    if runtime.is_null() {
        return Err(ClipError::InvalidArgument);
    }
    let runtime = unsafe { &mut *runtime };
    f(runtime)
}

pub(crate) fn with_runtime_ref<T>(
    runtime: ClipRuntime,
    f: impl FnOnce(&Runtime) -> Result<T, ClipError>,
) -> Result<T, ClipError> {
    if runtime.is_null() {
        return Err(ClipError::InvalidArgument);
    }
    let runtime = unsafe { &*runtime };
    f(&runtime.runtime)
}

pub(crate) fn viewport_id(viewport: ClipViewport) -> Result<ViewportId, ClipError> {
    if viewport.is_null() {
        return Err(ClipError::InvalidArgument);
    }
    Ok(unsafe { (*viewport).id })
}

pub(crate) fn with_runtime_viewport_ref<T>(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    f: impl FnOnce(&Runtime, ViewportId) -> Result<T, ClipError>,
) -> Result<T, ClipError> {
    let id = viewport_id(viewport)?;
    with_runtime_ref(runtime, |runtime| f(runtime, id))
}

pub(crate) fn with_runtime_viewport_mut<T>(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    f: impl FnOnce(&mut Runtime, ViewportId) -> Result<T, ClipError>,
) -> Result<T, ClipError> {
    let id = viewport_id(viewport)?;
    with_runtime_impl_mut(runtime, |runtime| f(&mut runtime.runtime, id))
}

pub(crate) fn with_patch_ref<T>(
    patch: ClipPatchHandle,
    f: impl FnOnce(&ClipPatchHandleImpl) -> Result<T, ClipError>,
) -> Result<T, ClipError> {
    if patch.is_null() {
        return Err(ClipError::InvalidArgument);
    }
    let patch = unsafe { &*patch };
    f(patch)
}
