#![allow(clippy::missing_safety_doc)]

#[unsafe(no_mangle)]
pub extern "C" fn clip_diagnostics_install_panic_hook(callback: clip_diagnostics::PanicCallback) {
    clip_diagnostics::install_panic_hook(callback);
}

// Public module re-exports keep shared C-ABI entry points reachable while Cargo
// packages the FFI crates and the Rust runtime into one static archive. Full
// enables selected-text here; Mini keeps the default feature-free archive.
#[cfg(feature = "selected-text")]
pub mod selected_text {
    pub use clip_selected_text_c::*;
}

pub mod capture {
    pub use clip_capture_c::*;
}

pub mod draw_engine {
    pub use clip_draw_engine_c::*;
}

pub mod stitch_images {
    pub use clip_stitch_images_c::*;
}

#[cfg(any(windows, target_os = "macos"))]
pub mod ui_selector {
    pub use clip_ui_selector_c::*;
}

pub mod recording_effects {
    pub use clip_recording_effects_c::*;
}

pub mod visual_region_detector {
    pub use clip_visual_region_detector_c::*;
}

pub mod recording {
    pub use clip_recording_c::*;
}

// Launch-scoped policy shared by native Qt workers and Rust workers.
#[unsafe(no_mangle)]
pub extern "C" fn clip_application_qos_initialize(
    level: u32,
    report_error: Option<clip_core::qos::ErrorHandler>,
) -> i32 {
    let Some(qos) = clip_core::qos::ApplicationQos::from_code(level) else {
        return -1;
    };
    if clip_core::qos::initialize_application_qos(qos, report_error) {
        0
    } else {
        -2
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn clip_application_qos_apply_current_thread() -> i32 {
    clip_core::qos::apply_current_thread_result()
}

#[unsafe(no_mangle)]
pub extern "C" fn clip_application_qos_active() -> u32 {
    clip_core::qos::active_application_qos().map_or(u32::MAX, |qos| qos as u32)
}
