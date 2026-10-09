use super::*;

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ClipPointerEventType {
    Down = 0,
    Move = 1,
    Up = 2,
    Cancel = 3,
    Enter = 4,
    Leave = 5,
    DoubleClick = 6,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ClipPointerDevice {
    Mouse = 0,
    Touch = 1,
    Pen = 2,
    Unknown = 3,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ClipPointerButton {
    None = 0,
    Primary = 1,
    Secondary = 2,
    Middle = 3,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ClipInputEventKind {
    Pointer = 0,
    Wheel = 1,
    Key = 2,
    FocusLost = 3,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ClipWheelDeltaKind {
    Pixel = 0,
    Angle = 1,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ClipKeyEventType {
    KeyDown = 0,
    KeyUp = 1,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ClipKeyCode {
    Unknown = 0,
    Space = 1,
    Escape = 2,
    ArrowUp = 3,
    ArrowDown = 4,
    ArrowLeft = 5,
    ArrowRight = 6,
    Character = 7,
    Backspace = 8,
    Delete = 9,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipPointerEvent {
    pub pointer_id: u32,
    pub event_type: ClipPointerEventType,
    pub device: ClipPointerDevice,
    pub position_x: f64,
    pub position_y: f64,
    pub button: ClipPointerButton,
    pub buttons: u8,
    pub reserved0: [u8; 3],
    pub modifiers: ClipModifiers,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipWheelEvent {
    pub position_x: f64,
    pub position_y: f64,
    pub delta_x: f64,
    pub delta_y: f64,
    pub delta_kind: ClipWheelDeltaKind,
    pub modifiers: ClipModifiers,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipKeyEvent {
    pub event_type: ClipKeyEventType,
    pub key_code: ClipKeyCode,
    pub codepoint: u32,
    pub modifiers: ClipModifiers,
    pub repeat: u8,
    pub reserved0: [u8; 3],
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipInputEvent {
    pub kind: ClipInputEventKind,
    pub pointer: ClipPointerEvent,
    pub wheel: ClipWheelEvent,
    pub key: ClipKeyEvent,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ClipPointerCaptureCommandKind {
    NoChange = 0,
    Capture = 1,
    Release = 2,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ClipCursorCommandKind {
    NoChange = 0,
    Set = 1,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ClipCursorStyle {
    Default = 0,
    Crosshair = 1,
    Grab = 2,
    Grabbing = 3,
    Move = 4,
    ResizeHorizontal = 5,
    ResizeVertical = 6,
    ResizeNwSe = 7,
    ResizeNeSw = 8,
    NotAllowed = 9,
    Text = 10,
    CornerRadius = 11,
    Hidden = 12,
    Stroke = 13,
    Eraser = 14,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ClipInteractionOutput {
    pub consumed: u8,
    pub reserved0: [u8; 3],
    pub capture_kind: ClipPointerCaptureCommandKind,
    pub capture_pointer_id: u32,
    pub cursor_kind: ClipCursorCommandKind,
    pub cursor_style: ClipCursorStyle,
}
