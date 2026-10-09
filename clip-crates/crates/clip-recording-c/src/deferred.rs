//! Versioned, explicitly owned deferred sources and cancellable render tasks.
use super::*;
use clip_screen_recorder::{
    DeferredRecordingOptions, DeferredRecordingSession, DeferredRecordingSource,
    DeferredRenderState, DeferredRenderTask, ExportStage, PlaybackOverlay,
};
use std::sync::Arc;

const OPTIONS_VERSION: u32 = 1;
const PROGRESS_VERSION: u32 = 1;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ClipRecordingDeferredOptions {
    version: u32,
    struct_size: u32,
    overlay: u32,
    progress_bar_rgba: u32,
    working_directory_utf8: *const c_char,
}

#[repr(C)]
pub struct ClipRecordingRenderProgress {
    version: u32,
    struct_size: u32,
    state: u32,
    stage: u32,
    percent: f32,
    completed_pts: u64,
    total_pts: u64,
    duration_ms: u64,
}

pub struct ClipRecordingSourceImpl {
    source: DeferredRecordingSource,
    active_tasks: Arc<AtomicUsize>,
}

pub struct ClipRecordingRenderTaskImpl {
    task: DeferredRenderTask,
    active_tasks: Arc<AtomicUsize>,
}

impl Drop for ClipRecordingRenderTaskImpl {
    fn drop(&mut self) {
        // Task teardown joins before decrementing the source's lease count.
        self.task.cancel();
    }
}

unsafe fn parse_options(
    options: *const ClipRecordingDeferredOptions,
) -> Result<DeferredRecordingOptions, String> {
    if options.is_null() {
        return Err("deferred recording options are null".into());
    }
    let header =
        unsafe { ptr::read_unaligned(options.cast::<ClipCaptureDirectRecordingConfigHeader>()) };
    if header.version != OPTIONS_VERSION {
        return Err("unsupported deferred recording options version".into());
    }
    if (header.struct_size as usize) < std::mem::size_of::<ClipRecordingDeferredOptions>() {
        return Err("deferred recording options are too small".into());
    }
    let options = unsafe { ptr::read_unaligned(options) };
    let playback_overlay = match options.overlay {
        0 => PlaybackOverlay::None,
        1 => PlaybackOverlay::ProgressBar {
            rgba: packed_rgba(options.progress_bar_rgba),
        },
        2 => PlaybackOverlay::PlaybackTime {
            rgba: [255, 255, 255, 255],
        },
        _ => return Err("invalid playback overlay".into()),
    };
    let working_directory = if options.working_directory_utf8.is_null() {
        None
    } else {
        let value = unsafe { CStr::from_ptr(options.working_directory_utf8) }
            .to_str()
            .map_err(|_| "deferred working directory must be UTF-8")?;
        if value.is_empty() {
            return Err("deferred working directory must not be empty".into());
        }
        Some(PathBuf::from(value))
    };
    Ok(DeferredRecordingOptions {
        working_directory,
        playback_overlay,
    })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_recording_session_create_deferred(
    config: *const ClipCaptureDirectRecordingConfig,
    options: *const ClipRecordingDeferredOptions,
    out_session: *mut *mut ClipRecordingSessionImpl,
) -> ClipRecordingResult {
    if out_session.is_null() {
        set_last_error("deferred recording out_session is null");
        return ClipRecordingResult::InvalidArgument;
    }
    unsafe { *out_session = ptr::null_mut() };
    let parsed = unsafe { read_direct_recording_config(config) }.and_then(|raw| {
        let mut config = parse_direct_recording_config(&raw)?;
        let options = unsafe { parse_options(options) }?;
        // The time badge uses the application font even when no keycaps are enabled.
        // Retaining its style must not enable keyboard observation or keycap rendering.
        if matches!(
            options.playback_overlay,
            PlaybackOverlay::PlaybackTime { .. }
        ) && config.keyboard.is_none()
        {
            let font_config = ClipCaptureDirectRecordingConfig {
                show_keyboard: 1,
                ..raw
            };
            config.keyboard = parse_keyboard_config(&font_config)?;
        }
        Ok((config, options))
    });
    let (config, options) = match parsed {
        Ok(parsed) => parsed,
        Err(error) => {
            set_last_error(error);
            return ClipRecordingResult::InvalidArgument;
        }
    };
    match DeferredRecordingSession::create(config, options) {
        Ok(recording) => {
            let session = ClipRecordingSessionImpl {
                recording: Some(RecordingSessionKind::Deferred(Box::new(recording))),
                state: RecordingState::Created,
            };
            LIVE_RECORDING_SESSIONS.fetch_add(1, Ordering::Relaxed);
            unsafe { *out_session = Box::into_raw(Box::new(session)) };
            clear_last_error();
            ClipRecordingResult::Ok
        }
        Err(error) => {
            let result = direct_result_for_error(&error);
            set_last_error(error);
            result
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_recording_session_finalize_deferred(
    session: *mut ClipRecordingSessionImpl,
    out_source: *mut *mut ClipRecordingSourceImpl,
) -> ClipRecordingResult {
    if out_source.is_null() {
        set_last_error("deferred out_source is null");
        return ClipRecordingResult::InvalidArgument;
    }
    unsafe { *out_source = ptr::null_mut() };
    let Some(session) = recording_session_mut(session) else {
        return ClipRecordingResult::InvalidArgument;
    };
    if !matches!(
        session.state,
        RecordingState::Running | RecordingState::Paused
    ) || !matches!(session.recording, Some(RecordingSessionKind::Deferred(_)))
    {
        set_last_error("finalize_deferred requires a running or paused deferred session");
        return ClipRecordingResult::InvalidState;
    }
    let Some(RecordingSessionKind::Deferred(recording)) = session.recording.take() else {
        return ClipRecordingResult::InternalError;
    };
    session.state = RecordingState::Stopped;
    match (*recording).stop() {
        Ok(source) => {
            let source = ClipRecordingSourceImpl {
                source,
                active_tasks: Arc::new(AtomicUsize::new(0)),
            };
            unsafe { *out_source = Box::into_raw(Box::new(source)) };
            clear_last_error();
            ClipRecordingResult::Ok
        }
        Err(error) => {
            let result = direct_result_for_error(&error);
            set_last_error(error);
            result
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_recording_source_render_start(
    source: *mut ClipRecordingSourceImpl,
    out_task: *mut *mut ClipRecordingRenderTaskImpl,
) -> ClipRecordingResult {
    if out_task.is_null() {
        set_last_error("render out_task is null");
        return ClipRecordingResult::InvalidArgument;
    }
    unsafe { *out_task = ptr::null_mut() };
    let Some(source) = (unsafe { source.as_ref() }) else {
        set_last_error("render source is null");
        return ClipRecordingResult::InvalidArgument;
    };
    if source
        .active_tasks
        .compare_exchange(0, 1, Ordering::AcqRel, Ordering::Acquire)
        .is_err()
    {
        set_last_error("source already has a render task");
        return ClipRecordingResult::InvalidState;
    }
    match source.source.render_async() {
        Ok(task) => {
            let task = ClipRecordingRenderTaskImpl {
                task,
                active_tasks: Arc::clone(&source.active_tasks),
            };
            unsafe { *out_task = Box::into_raw(Box::new(task)) };
            clear_last_error();
            ClipRecordingResult::Ok
        }
        Err(error) => {
            source.active_tasks.store(0, Ordering::Release);
            let result = direct_result_for_error(&error);
            set_last_error(error);
            result
        }
    }
}

unsafe fn copy_text(value: &str, buffer: *mut c_char, capacity: usize) -> usize {
    let value = sanitize_cstring(value);
    let bytes = value.as_bytes();
    if !buffer.is_null() && capacity > 0 {
        let count = bytes.len().min(capacity - 1);
        unsafe {
            ptr::copy_nonoverlapping(bytes.as_ptr(), buffer.cast(), count);
            *buffer.add(count) = 0;
        }
    }
    bytes.len() + 1
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_recording_source_path(
    source: *const ClipRecordingSourceImpl,
    buffer: *mut c_char,
    capacity: usize,
) -> usize {
    let Some(source) = (unsafe { source.as_ref() }) else {
        return 0;
    };
    unsafe { copy_text(&source.source.path().to_string_lossy(), buffer, capacity) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_recording_source_discard(
    source: *mut ClipRecordingSourceImpl,
) -> ClipRecordingResult {
    let Some(source) = (unsafe { source.as_ref() }) else {
        set_last_error("discard source is null");
        return ClipRecordingResult::InvalidArgument;
    };
    if source
        .active_tasks
        .compare_exchange(0, usize::MAX, Ordering::AcqRel, Ordering::Acquire)
        .is_err()
    {
        set_last_error("cannot discard a source with an active render task");
        return ClipRecordingResult::InvalidState;
    }
    match source.source.discard() {
        Ok(()) => {
            clear_last_error();
            ClipRecordingResult::Ok
        }
        Err(error) => {
            source.active_tasks.store(0, Ordering::Release);
            let result = direct_result_for_error(&error);
            set_last_error(error);
            result
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_recording_source_destroy(source: *mut ClipRecordingSourceImpl) {
    if !source.is_null() {
        unsafe { drop(Box::from_raw(source)) };
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_recording_render_task_poll(
    task: *const ClipRecordingRenderTaskImpl,
    progress: *mut ClipRecordingRenderProgress,
) -> ClipRecordingResult {
    let Some(task) = (unsafe { task.as_ref() }) else {
        return ClipRecordingResult::InvalidArgument;
    };
    if progress.is_null() {
        return ClipRecordingResult::InvalidArgument;
    }
    let header =
        unsafe { ptr::read_unaligned(progress.cast::<ClipCaptureDirectRecordingConfigHeader>()) };
    if header.version != PROGRESS_VERSION
        || (header.struct_size as usize) < std::mem::size_of::<ClipRecordingRenderProgress>()
    {
        return ClipRecordingResult::InvalidArgument;
    }
    let snapshot = task.task.snapshot();
    let state = match snapshot.state {
        DeferredRenderState::Running => 0,
        DeferredRenderState::Succeeded => 1,
        DeferredRenderState::Canceled => 2,
        DeferredRenderState::Failed => 3,
    };
    let stage = match snapshot.stage {
        ExportStage::Plan => 0,
        ExportStage::Mux | ExportStage::Finalize => 2,
        _ => 1,
    };
    let value = ClipRecordingRenderProgress {
        version: PROGRESS_VERSION,
        struct_size: std::mem::size_of::<ClipRecordingRenderProgress>() as u32,
        state,
        stage,
        percent: snapshot.percent.clamp(0.0, 100.0),
        completed_pts: snapshot.completed_frames,
        total_pts: snapshot.total_frames,
        duration_ms: snapshot.duration_ms,
    };
    unsafe { ptr::write_unaligned(progress, value) };
    ClipRecordingResult::Ok
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_recording_render_task_cancel(
    task: *mut ClipRecordingRenderTaskImpl,
) -> ClipRecordingResult {
    let Some(task) = (unsafe { task.as_ref() }) else {
        return ClipRecordingResult::InvalidArgument;
    };
    task.task.cancel();
    ClipRecordingResult::Ok
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_recording_render_task_error(
    task: *const ClipRecordingRenderTaskImpl,
    buffer: *mut c_char,
    capacity: usize,
) -> usize {
    let Some(task) = (unsafe { task.as_ref() }) else {
        return 0;
    };
    let snapshot = task.task.snapshot();
    unsafe { copy_text(snapshot.error.as_deref().unwrap_or(""), buffer, capacity) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_recording_render_task_output_path(
    task: *const ClipRecordingRenderTaskImpl,
    buffer: *mut c_char,
    capacity: usize,
) -> usize {
    let Some(task) = (unsafe { task.as_ref() }) else {
        return 0;
    };
    let snapshot = task.task.snapshot();
    let output = snapshot
        .output_path
        .as_deref()
        .map(|path| path.to_string_lossy());
    unsafe { copy_text(output.as_deref().unwrap_or(""), buffer, capacity) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_recording_render_task_destroy(
    task: *mut ClipRecordingRenderTaskImpl,
) {
    if task.is_null() {
        return;
    }
    let task = unsafe { Box::from_raw(task) };
    let active_tasks = Arc::clone(&task.active_tasks);
    drop(task);
    active_tasks.store(0, Ordering::Release);
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn string_getter_queries_size_terminates_and_preserves_guard() {
        let mut bytes = [0x7fu8; 5];
        let required = unsafe { copy_text("abcdef", bytes.as_mut_ptr().cast(), 4) };
        assert_eq!(required, 7);
        assert_eq!(&bytes, b"abc\0\x7f");
        assert_eq!(unsafe { copy_text("abcdef", ptr::null_mut(), 0) }, 7);
        assert_eq!(unsafe { copy_text("", ptr::null_mut(), 0) }, 1);
    }

    #[test]
    fn options_validate_header_without_reading_later_fields() {
        let header = ClipCaptureDirectRecordingConfigHeader {
            version: 1,
            struct_size: 8,
        };
        assert!(unsafe { parse_options((&raw const header).cast()) }.is_err());
        assert!(unsafe { parse_options(ptr::null()) }.is_err());
    }

    #[test]
    fn deferred_creation_accepts_every_supported_c_prefix_and_owns_strings() {
        let _guard = SESSION_TEST_LOCK
            .lock()
            .unwrap_or_else(|error| error.into_inner());
        for version in 1..=DIRECT_RECORDING_CONFIG_VERSION {
            let output = CString::new("retained.mp4").unwrap();
            let mut raw = crate::tests::direct_config(&output);
            raw.version = version;
            raw.struct_size = direct_config_size(version).unwrap();
            raw.system_audio_gain_db = -9;
            raw.microphone_gain_db = 6;
            if version == 2 {
                raw.mouse_trail_duration_ms = 0;
            }
            let prefix = unsafe {
                std::slice::from_raw_parts((&raw const raw).cast::<u8>(), raw.struct_size as usize)
            }
            .to_vec();
            let options = ClipRecordingDeferredOptions {
                version: OPTIONS_VERSION,
                struct_size: std::mem::size_of::<ClipRecordingDeferredOptions>() as u32,
                overlay: 2,
                progress_bar_rgba: 0x1677ffff,
                working_directory_utf8: ptr::null(),
            };
            let mut session = ptr::null_mut();
            assert_eq!(
                unsafe {
                    clip_recording_session_create_deferred(
                        prefix.as_ptr().cast(),
                        &options,
                        &mut session,
                    )
                },
                ClipRecordingResult::Ok,
                "version={version}"
            );
            drop(output);
            drop(prefix);
            assert!(!session.is_null());
            let control = recording_audio_control(session).unwrap();
            assert_eq!(
                control.gain_db(AudioSourceKind::System),
                if version >= 11 { -9 } else { 0 }
            );
            assert_eq!(
                control.gain_db(AudioSourceKind::Microphone),
                if version >= 11 { 6 } else { 0 }
            );
            assert_eq!(clip_recording_session_set_audio_gain(session, 0, 12), 1);
            assert_eq!(clip_recording_session_set_audio_gain(session, 1, -12), 1);
            assert_eq!(clip_recording_session_set_audio_gain(session, 1, 25), 0);
            assert_eq!(control.gain_db(AudioSourceKind::System), 12);
            assert_eq!(control.gain_db(AudioSourceKind::Microphone), -12);
            assert_eq!(clip_recording_session_set_audio_metering(session, 3), 1);
            assert_eq!(clip_recording_session_set_audio_metering(session, 4), 0);
            let mut levels = ClipRecordingAudioLevels::default();
            assert_eq!(
                unsafe { clip_recording_session_take_audio_levels(session, &mut levels) },
                1
            );
            assert_eq!(levels.system_audio.status, 0);
            assert_eq!(levels.microphone.peak, 0.0);
            let windows = [10u32, 20];
            let exclusions = ClipCaptureExclusions {
                windows: windows.as_ptr(),
                window_count: windows.len(),
                processes: ptr::null(),
                process_count: 0,
            };
            let mut generation = 0;
            assert_eq!(
                unsafe {
                    clip_recording_session_request_exclusions(
                        session,
                        &exclusions,
                        windows.as_ptr(),
                        1,
                        &mut generation,
                    )
                },
                1
            );
            let mut status = ClipRecordingExclusionStatus::default();
            assert_eq!(
                unsafe { clip_recording_session_exclusion_status(session, &mut status) },
                1
            );
            assert_eq!(status.requested_generation, generation);
            #[cfg(not(target_os = "macos"))]
            assert_eq!(status.applied_generation, generation);
            assert_eq!(
                clip_recording_session_request_stop(session),
                ClipRecordingResult::InvalidState
            );
            let mut source = ptr::null_mut();
            assert_eq!(
                unsafe { clip_recording_session_finalize_deferred(session, &mut source) },
                ClipRecordingResult::InvalidState
            );
            assert!(source.is_null());
            unsafe { clip_recording_session_destroy(session) };
        }
    }

    #[test]
    fn null_opaque_handles_fail_without_accessing_buffers() {
        let mut source = ptr::null_mut();
        let mut task = ptr::null_mut();
        assert_eq!(
            unsafe { clip_recording_session_finalize_deferred(ptr::null_mut(), &mut source) },
            ClipRecordingResult::InvalidArgument
        );
        assert_eq!(
            unsafe { clip_recording_source_render_start(ptr::null_mut(), &mut task) },
            ClipRecordingResult::InvalidArgument
        );
        assert_eq!(
            unsafe { clip_recording_source_discard(ptr::null_mut()) },
            ClipRecordingResult::InvalidArgument
        );
        assert_eq!(
            unsafe { clip_recording_render_task_cancel(ptr::null_mut()) },
            ClipRecordingResult::InvalidArgument
        );
        assert_eq!(
            unsafe { clip_recording_render_task_poll(ptr::null_mut(), ptr::null_mut()) },
            ClipRecordingResult::InvalidArgument
        );
        assert_eq!(
            unsafe { clip_recording_render_task_output_path(ptr::null_mut(), ptr::null_mut(), 0) },
            0
        );
        unsafe {
            clip_recording_source_destroy(ptr::null_mut());
            clip_recording_render_task_destroy(ptr::null_mut());
        }
    }

    #[test]
    fn deferred_options_own_working_directory_and_overlay() {
        let directory = CString::new("working directory").unwrap();
        let raw = ClipRecordingDeferredOptions {
            version: OPTIONS_VERSION,
            struct_size: std::mem::size_of::<ClipRecordingDeferredOptions>() as u32,
            overlay: 1,
            progress_bar_rgba: 0x1677ff80,
            working_directory_utf8: directory.as_ptr(),
        };
        let parsed = unsafe { parse_options(&raw) }.unwrap();
        drop(directory);
        assert_eq!(
            parsed.working_directory,
            Some(PathBuf::from("working directory"))
        );
        assert!(matches!(
            parsed.playback_overlay,
            PlaybackOverlay::ProgressBar {
                rgba: [22, 119, 255, 128]
            }
        ));
        let invalid = ClipRecordingDeferredOptions {
            overlay: 99,
            working_directory_utf8: ptr::null(),
            ..raw
        };
        assert!(unsafe { parse_options(&invalid) }.is_err());
    }
}
