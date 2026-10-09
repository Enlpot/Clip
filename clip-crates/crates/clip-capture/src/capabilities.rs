//! Feature discovery is separate from permission requests and target acquisition.
use crate::backend::CaptureBackendKind;
use clip_media::{PixelFormat, geometry::DesktopSpace};
#[derive(Clone, Debug)]
pub struct CaptureCapabilities {
    pub backends: Vec<CaptureBackendKind>,
    pub desktop_space: DesktopSpace,
    pub cpu_formats: Vec<PixelFormat>,
    pub native_frames: bool,
    pub hdr_capture: bool,
    pub window_enumeration: bool,
}
impl CaptureCapabilities {
    pub fn current() -> Self {
        {
            let supported = cfg!(windows);
            Self {
                backends: if supported {
                    vec![
                        CaptureBackendKind::DxgiDuplication,
                        CaptureBackendKind::WindowsGraphicsCapture,
                        CaptureBackendKind::Gdi,
                    ]
                } else {
                    vec![]
                },
                desktop_space: DesktopSpace::PhysicalPixels,
                cpu_formats: if supported {
                    vec![PixelFormat::Rgba8, PixelFormat::Bgra8]
                } else {
                    vec![]
                },
                native_frames: supported,
                hdr_capture: false,
                window_enumeration: false,
            }
        }
    }
}
