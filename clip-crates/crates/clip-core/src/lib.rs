//! Shared abstractions for the clip screen recording workspace.
//!
//! `clip-core` provides common traits, timestamp types, and error foundations
//! consumed by all leaf crates (`clip-capture`, `clip-audio-recorder`,
//! `clip-cursor`) and the compositor crate (`clip-screen-recorder`).

pub mod error;
pub mod event;
pub mod recording_clock;
pub mod stream_queue;
pub mod streaming;
pub mod timestamp;

pub mod cancellation;

pub mod keycap_layout;

pub mod qos;
