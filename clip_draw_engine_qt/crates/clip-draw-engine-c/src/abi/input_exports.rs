use crate::abi::convert::*;
use crate::abi::handles::*;
use crate::abi::types::*;

/// Returns the topmost element eligible for body selection with the given button.
/// Coordinates are in canvas space; tolerance comes from the queried viewport.
///
/// # Safety
/// Handles must be live and output pointers must be writable.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_hit_quick_selection(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    canvas_x: f64,
    canvas_y: f64,
    button: ClipPointerButton,
    out_id: *mut ClipElementId,
    out_hit: *mut u8,
) -> ClipError {
    ffi_error(|| {
        if out_id.is_null() || out_hit.is_null() || !canvas_x.is_finite() || !canvas_y.is_finite() {
            return ClipError::InvalidArgument;
        }
        let button = match button {
            ClipPointerButton::Primary => clip_draw_engine::PointerButton::Primary,
            ClipPointerButton::Secondary => clip_draw_engine::PointerButton::Secondary,
            _ => return ClipError::InvalidArgument,
        };
        ffi_status(with_runtime_viewport_ref(
            runtime,
            viewport,
            |runtime, id| {
                let hit = runtime
                    .hit_quick_selection_at(
                        id,
                        clip_draw_engine_core::Point::new(canvas_x, canvas_y),
                        button,
                    )
                    .map_err(ClipError::from)?;
                write_out(
                    out_id,
                    hit.map_or(ClipElementId::default(), clip_element_id_from_rust),
                );
                write_out(out_hit, u8::from(hit.is_some()));
                Ok(())
            },
        ))
    })
}

/// # Safety
/// If `runtime` and `viewport` are non-null, they must be live handles created by this library.
/// `event` must point to a readable `ClipInputEvent`.
/// `out_output` and `out_changed_viewports` must be writable.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_process_input_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    event: *const ClipInputEvent,
    out_output: *mut ClipInteractionOutput,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if event.is_null() || out_output.is_null() || out_changed_viewports.is_null() {
            return ClipError::InvalidArgument;
        }

        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let id = viewport_id(viewport)?;
            let event = clip_input_event_to_rust(unsafe { &*event })?;
            let update = state
                .runtime
                .process_input_with_viewport_changes(id, event)
                .map_err(ClipError::from)?;
            write_out(
                out_output,
                clip_interaction_output_from_rust(update.interaction),
            );
            write_changed_viewports(out_changed_viewports, update.changed_viewports);
            Ok(())
        }))
    })
}

/// Processes an ordered, non-empty batch containing only pointer-move events.
/// Viewport presentation is refreshed once after the final event.
///
/// # Safety
/// If `runtime` and `viewport` are non-null, they must be live handles created by this library.
/// `events` must point to `event_count` readable values. Output pointers must be writable.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_viewport_process_pointer_move_batch_ex(
    runtime: ClipRuntime,
    viewport: ClipViewport,
    events: *const ClipInputEvent,
    event_count: u32,
    out_output: *mut ClipInteractionOutput,
    out_changed_viewports: *mut ClipChangedViewportList,
) -> ClipError {
    ffi_error(|| {
        if events.is_null()
            || event_count == 0
            || out_output.is_null()
            || out_changed_viewports.is_null()
        {
            return ClipError::InvalidArgument;
        }
        let events = match unsafe { std::slice::from_raw_parts(events, event_count as usize) }
            .iter()
            .map(clip_input_event_to_rust)
            .collect::<Result<Vec<_>, _>>()
        {
            Ok(events) => events,
            Err(error) => return error,
        };
        ffi_status(with_runtime_impl_mut(runtime, |state| {
            let id = viewport_id(viewport)?;
            let update = state
                .runtime
                .process_pointer_move_batch_with_viewport_changes(id, &events)
                .map_err(ClipError::from)?;
            write_out(
                out_output,
                clip_interaction_output_from_rust(update.interaction),
            );
            write_changed_viewports(out_changed_viewports, update.changed_viewports);
            Ok(())
        }))
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use clip_draw_engine::{
        ActiveTool, InputEvent, Modifiers, Point, PointerButton, PointerButtons, PointerDevice,
        PointerEvent, PointerEventType, Runtime, ViewportConfig,
    };

    #[test]
    fn right_quick_selection_query_reports_hits_misses_and_invalid_arguments() {
        let mut state = ClipRuntimeImpl {
            runtime: Runtime::default(),
        };
        let id = state
            .runtime
            .create_viewport(ViewportConfig::default())
            .unwrap();
        state
            .runtime
            .set_viewport_surface_size(id, 200, 200)
            .unwrap();
        state
            .runtime
            .set_viewport_active_tool(id, ActiveTool::Shape)
            .unwrap();
        for (kind, x, y, button) in [
            (PointerEventType::Down, 80.0, 80.0, PointerButton::Primary),
            (PointerEventType::Move, 120.0, 120.0, PointerButton::Primary),
            (PointerEventType::Up, 120.0, 120.0, PointerButton::Primary),
            (
                PointerEventType::Down,
                100.0,
                80.0,
                PointerButton::Secondary,
            ),
            (PointerEventType::Up, 100.0, 80.0, PointerButton::Secondary),
        ] {
            state
                .runtime
                .process_input(
                    id,
                    InputEvent::Pointer(PointerEvent {
                        pointer_id: 1,
                        event_type: kind,
                        device: PointerDevice::Mouse,
                        position: Point::new(x, y),
                        button: Some(button),
                        buttons: PointerButtons(if kind == PointerEventType::Up {
                            0
                        } else if button == PointerButton::Primary {
                            PointerButtons::PRIMARY
                        } else {
                            PointerButtons::SECONDARY
                        }),
                        modifiers: Modifiers::default(),
                    }),
                )
                .unwrap();
        }
        state
            .runtime
            .set_quick_selection_disabled_tools(u64::MAX)
            .unwrap();
        let selected = state.runtime.selected_ids();
        let mut viewport = ClipViewportImpl { id };
        let mut out_id = ClipElementId::default();
        let mut hit = 0;
        unsafe {
            assert_eq!(
                clip_viewport_hit_quick_selection(
                    &mut state,
                    &mut viewport,
                    0.0,
                    -20.0,
                    ClipPointerButton::Secondary,
                    &mut out_id,
                    &mut hit
                ),
                ClipError::Ok
            );
            assert_eq!(hit, 1);
            assert_eq!(out_id, clip_element_id_from_rust(selected[0]));
            assert_eq!(
                clip_viewport_hit_quick_selection(
                    &mut state,
                    &mut viewport,
                    0.0,
                    -20.0,
                    ClipPointerButton::Primary,
                    &mut out_id,
                    &mut hit
                ),
                ClipError::Ok
            );
            assert_eq!(hit, 0);
            assert_eq!(out_id, ClipElementId::default());
            assert_eq!(
                clip_viewport_hit_quick_selection(
                    &mut state,
                    &mut viewport,
                    f64::NAN,
                    0.0,
                    ClipPointerButton::Secondary,
                    &mut out_id,
                    &mut hit
                ),
                ClipError::InvalidArgument
            );
            assert_eq!(
                clip_viewport_hit_quick_selection(
                    &mut state,
                    &mut viewport,
                    0.0,
                    0.0,
                    ClipPointerButton::Middle,
                    &mut out_id,
                    &mut hit
                ),
                ClipError::InvalidArgument
            );
            assert_eq!(
                clip_viewport_hit_quick_selection(
                    &mut state,
                    &mut viewport,
                    0.0,
                    0.0,
                    ClipPointerButton::Secondary,
                    &mut out_id,
                    std::ptr::null_mut()
                ),
                ClipError::InvalidArgument
            );
        }
        assert_eq!(state.runtime.selected_ids(), selected);
    }
}
