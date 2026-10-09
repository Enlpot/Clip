use std::io;

use thiserror::Error;

#[derive(Debug, Error)]
pub enum RecordingExportError {
    #[error("invalid config: {0}")]
    InvalidConfig(String),

    #[error("encode error: {0}")]
    Encode(String),

    #[error("io error: {0}")]
    Io(#[from] io::Error),

    #[error("decode error: {0}")]
    Decode(String),

    #[error("export error: {0}")]
    Export(String),

    #[error("export canceled")]
    ExportCanceled,
}

pub type Result<T> = std::result::Result<T, RecordingExportError>;

impl From<clip_recording_model::RecordingModelError> for RecordingExportError {
    fn from(value: clip_recording_model::RecordingModelError) -> Self {
        match value {
            clip_recording_model::RecordingModelError::Io(err) => Self::Io(err),
            clip_recording_model::RecordingModelError::Decode(err) => Self::Decode(err),
        }
    }
}
