use clip_draw_engine::ErrorCode;

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ClipError {
    Ok = 0,
    InvalidArgument = 1,
    NotFound = 2,
    InvalidState = 3,
    BufferTooSmall = 4,
    StaleRevision = 5,
    Unsupported = 6,
    Internal = 7,
}

impl From<ErrorCode> for ClipError {
    fn from(value: ErrorCode) -> Self {
        match value {
            ErrorCode::InvalidArgument => ClipError::InvalidArgument,
            ErrorCode::NotFound => ClipError::NotFound,
            ErrorCode::InvalidState => ClipError::InvalidState,
            ErrorCode::BufferTooSmall => ClipError::BufferTooSmall,
            ErrorCode::StaleRevision => ClipError::StaleRevision,
            ErrorCode::Unsupported => ClipError::Unsupported,
            ErrorCode::Internal => ClipError::Internal,
        }
    }
}
