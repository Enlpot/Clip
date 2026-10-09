use super::{convert::*, exports::*, handles::*, style_exports::*, text_exports::*, types::*};
use clip_draw_engine::{
    ActiveTool, InputEvent, Modifiers, Point, PointerButton, PointerButtons, PointerDevice,
    PointerEvent, PointerEventType, Runtime, ViewportConfig,
};

#[test]
fn presentation_scope_ffi_validation_keeps_scopes_balanced() {
    let mut state = ClipRuntimeImpl {
        runtime: Runtime::default(),
    };
    let mut changed = std::ptr::null_mut();
    unsafe {
        assert_eq!(
            clip_runtime_begin_presentation_update(std::ptr::null_mut()),
            ClipError::InvalidArgument
        );
        assert_eq!(
            clip_runtime_end_presentation_update_ex(&mut state, &mut changed),
            ClipError::InvalidState
        );
        assert!(changed.is_null());
        assert_eq!(
            clip_runtime_begin_presentation_update(&mut state),
            ClipError::Ok
        );
        assert_eq!(
            clip_runtime_end_presentation_update_ex(&mut state, std::ptr::null_mut()),
            ClipError::InvalidArgument
        );
        assert_eq!(
            clip_runtime_end_presentation_update_ex(&mut state, &mut changed),
            ClipError::Ok
        );
        clip_changed_viewports_destroy(changed);
        assert_eq!(
            clip_runtime_end_presentation_update_ex(&mut state, &mut changed),
            ClipError::InvalidState
        );
        assert!(changed.is_null());
    }
}

#[test]
fn distance_abi_values_layout_and_round_trip() {
    assert_eq!(ClipDistanceStyle::default().unit, ClipDistanceUnit::Cm);
    assert_eq!(ClipDistanceUnit::Px as u32, 0);
    assert_eq!(ClipDistanceUnit::Cm as u32, 1);
    assert_eq!(ClipDistanceUnit::M as u32, 2);
    assert_eq!(ClipDistanceUnit::Km as u32, 3);
    assert_eq!(ClipDistanceUnit::Mm as u32, 4);
    assert_eq!(ClipActiveTool::Distance as u32, 17);
    assert_eq!(ClipStyleToolbarSource::DefaultDistance as u32, 26);
    assert_eq!(ClipStyleToolbarSource::SelectedDistance as u32, 27);
    assert_eq!(std::mem::size_of::<ClipDistanceStyle>(), 48);
    assert_eq!(std::mem::offset_of!(ClipDistanceStyle, stroke_width), 8);
    assert_eq!(std::mem::offset_of!(ClipDistanceStyle, unit), 24);
    assert_eq!(std::mem::offset_of!(ClipDistanceStyle, endpoint_scale), 32);
    assert_eq!(std::mem::offset_of!(ClipDistanceStyle, endpoint_style), 40);
    let style = ClipDistanceStyle {
        factor: 2.31,
        unit: ClipDistanceUnit::Km,
        decimal_places: 3,
        endpoint_scale: 2.5,
        endpoint_style: ClipArrowhead::IndentedTriangle,
        ..ClipDistanceStyle::default()
    };
    let rust: clip_draw_engine::DistanceStyle = style.into();
    assert_eq!(ClipDistanceStyle::from(rust), style);
    let millimeters = ClipDistanceStyle {
        unit: ClipDistanceUnit::Mm,
        ..style
    };
    let rust: clip_draw_engine::DistanceStyle = millimeters.into();
    assert_eq!(rust.unit, clip_draw_engine::DistanceUnit::Mm);
    assert_eq!(ClipDistanceStyle::from(rust), millimeters);
    assert_eq!(
        clip_active_tool_to_rust(ClipActiveTool::Distance),
        ActiveTool::Distance
    );
    assert_eq!(
        clip_active_tool_from_rust(ActiveTool::Distance),
        ClipActiveTool::Distance
    );
    assert_eq!(
        clip_active_tool_mask_to_rust(1 << 17),
        ActiveTool::Distance.policy_bit()
    );
}

#[test]
fn distance_ffi_validates_and_supplies_complete_provisional_label() {
    let mut state = ClipRuntimeImpl {
        runtime: Runtime::default(),
    };
    let id = state
        .runtime
        .create_viewport(ViewportConfig::default())
        .unwrap();
    state
        .runtime
        .set_viewport_surface_size(id, 600, 360)
        .unwrap();
    state
        .runtime
        .set_viewport_active_tool(id, ActiveTool::Distance)
        .unwrap();
    let mut viewport = ClipViewportImpl { id };
    let mut changed = std::ptr::null_mut();
    unsafe {
        assert_eq!(
            clip_viewport_set_distance_pixel_scale_ex(
                &mut state,
                &mut viewport,
                2.0,
                3.0,
                &mut changed
            ),
            ClipError::Ok
        );
        clip_changed_viewports_destroy(changed);
        changed = std::ptr::null_mut();
        assert_eq!(
            clip_viewport_set_distance_pixel_scale_ex(
                &mut state,
                &mut viewport,
                0.0,
                1.0,
                &mut changed
            ),
            ClipError::InvalidArgument
        );
        assert!(changed.is_null());
        let invalid = ClipDistanceStyle {
            factor: 1001.0,
            ..ClipDistanceStyle::default()
        };
        assert_eq!(
            clip_viewport_set_distance_style_patch_ex(
                &mut state,
                &mut viewport,
                &invalid,
                CLIP_DISTANCE_STYLE_PROPERTY_ALL,
                &mut changed
            ),
            ClipError::InvalidArgument
        );
        let invalid = ClipDistanceStyle {
            decimal_places: 256,
            ..ClipDistanceStyle::default()
        };
        assert_eq!(
            clip_viewport_set_distance_style_patch_ex(
                &mut state,
                &mut viewport,
                &invalid,
                CLIP_DISTANCE_STYLE_PROPERTY_ALL,
                &mut changed
            ),
            ClipError::InvalidArgument
        );
        let invalid = ClipDistanceStyle {
            endpoint_scale: 0.4,
            ..ClipDistanceStyle::default()
        };
        assert_eq!(
            clip_viewport_set_distance_style_patch_ex(
                &mut state,
                &mut viewport,
                &invalid,
                CLIP_DISTANCE_STYLE_PROPERTY_ENDPOINT_SCALE,
                &mut changed
            ),
            ClipError::InvalidArgument
        );
        assert_eq!(
            clip_viewport_set_distance_style_patch_ex(
                &mut state,
                &mut viewport,
                std::ptr::null(),
                1,
                &mut changed
            ),
            ClipError::InvalidArgument
        );
    }
    for (event_type, x) in [
        (PointerEventType::Down, 100.0),
        (PointerEventType::Move, 300.0),
    ] {
        state
            .runtime
            .process_input(
                id,
                InputEvent::Pointer(PointerEvent {
                    pointer_id: 1,
                    event_type,
                    device: PointerDevice::Mouse,
                    position: Point::new(x, 180.0),
                    button: Some(PointerButton::Primary),
                    buttons: PointerButtons(PointerButtons::PRIMARY),
                    modifiers: Modifiers::default(),
                }),
            )
            .unwrap();
    }
    assert!(!state.runtime.has_document_content());
    let mut count = 0;
    let mut request = ClipArrowTextLayoutRequest::default();
    unsafe {
        assert_eq!(
            clip_viewport_get_arrow_text_layout_requests(
                &mut state,
                &mut viewport,
                &mut request,
                1,
                &mut count
            ),
            ClipError::Ok
        );
    }
    assert_eq!(count, 1);
    let text = String::from_utf8(
        request.info.text_utf8[..request.info.text_utf8_len as usize]
            .iter()
            .map(|c| *c as u8)
            .collect(),
    )
    .unwrap();
    assert_eq!(text, "400 cm");
    assert_eq!(request.info.font_size, 20.0);
    assert_eq!(request.style.font_size, 20.0);
    assert_eq!(request.info.center_x, -100.0);
    assert_eq!(request.info.rotation, 0.0);
    let old_key = request.key;
    state
        .runtime
        .process_input(
            id,
            InputEvent::Pointer(PointerEvent {
                pointer_id: 1,
                event_type: PointerEventType::Move,
                device: PointerDevice::Mouse,
                position: Point::new(400.0, 180.0),
                button: Some(PointerButton::Primary),
                buttons: PointerButtons(PointerButtons::PRIMARY),
                modifiers: Modifiers::default(),
            }),
        )
        .unwrap();
    unsafe {
        assert_eq!(
            clip_viewport_get_arrow_text_layout_requests(
                &mut state,
                &mut viewport,
                &mut request,
                1,
                &mut count
            ),
            ClipError::Ok
        );
    }
    assert_ne!(request.key, old_key);
    let text = String::from_utf8(
        request.info.text_utf8[..request.info.text_utf8_len as usize]
            .iter()
            .map(|c| *c as u8)
            .collect(),
    )
    .unwrap();
    assert_eq!(text, "600 cm");
}
