use crate::abi::{convert::clip_arrow_path_commands_from_rust, handles::*, types::*};
use clip_draw_engine_core::{PathSegmentMode, Point, catmull_rom_path_commands};

/// Builds the same curve used by closed free-draw elements. The caller owns both
/// buffers; a null output queries the required command count.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_build_catmull_rom_path(
    vertices: *const ClipArrowPoint,
    vertex_count: usize,
    closed: u8,
    commands: *mut ClipArrowPathCommand,
    capacity: usize,
    count: *mut usize,
) -> ClipError {
    ffi_error(|| {
        if count.is_null() || (vertices.is_null() && vertex_count != 0) || vertex_count > 65536 {
            return ClipError::InvalidArgument;
        }
        let points = if vertex_count == 0 {
            &[][..]
        } else {
            unsafe { std::slice::from_raw_parts(vertices, vertex_count) }
        };
        if points.iter().any(|p| !p.x.is_finite() || !p.y.is_finite()) {
            return ClipError::InvalidArgument;
        }
        let points: Vec<_> = points.iter().map(|p| Point::new(p.x, p.y)).collect();
        let modes = vec![PathSegmentMode::Curve; points.len()];
        let path = catmull_rom_path_commands(&points, &modes, closed != 0);
        let output = clip_arrow_path_commands_from_rust(&path);
        write_out(count, output.len());
        if !commands.is_null() {
            if capacity < output.len() {
                return ClipError::BufferTooSmall;
            }
            unsafe { std::ptr::copy_nonoverlapping(output.as_ptr(), commands, output.len()) };
        }
        ClipError::Ok
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn curve_bridge_matches_core_and_rejects_invalid_input() {
        let points = [
            ClipArrowPoint { x: 0.0, y: 0.0 },
            ClipArrowPoint { x: 80.0, y: 10.0 },
            ClipArrowPoint { x: 30.0, y: 70.0 },
        ];
        let mut output = [ClipArrowPathCommand::default(); 4];
        let mut count = 0;
        unsafe {
            assert_eq!(
                clip_build_catmull_rom_path(
                    points.as_ptr(),
                    3,
                    1,
                    output.as_mut_ptr(),
                    4,
                    &mut count
                ),
                ClipError::Ok
            );
        }
        let expected = catmull_rom_path_commands(
            &points.map(|p| Point::new(p.x, p.y)),
            &[PathSegmentMode::Curve; 3],
            true,
        );
        assert_eq!(count, 4);
        assert_eq!(
            output.as_slice(),
            clip_arrow_path_commands_from_rust(&expected)
        );
        unsafe {
            assert_eq!(
                clip_build_catmull_rom_path(
                    points.as_ptr(),
                    3,
                    1,
                    output.as_mut_ptr(),
                    1,
                    &mut count
                ),
                ClipError::BufferTooSmall
            );
            assert_eq!(
                clip_build_catmull_rom_path(
                    std::ptr::null(),
                    3,
                    1,
                    output.as_mut_ptr(),
                    4,
                    &mut count
                ),
                ClipError::InvalidArgument
            );
        }
    }
    #[test]
    fn curve_bridge_handles_open_closed_and_repeated_vertices() {
        let cases = [
            vec![],
            vec![ClipArrowPoint { x: 3.0, y: 4.0 }],
            vec![
                ClipArrowPoint { x: 0.0, y: 0.0 },
                ClipArrowPoint { x: 0.0, y: 0.0 },
                ClipArrowPoint { x: 70.0, y: 5.0 },
                ClipArrowPoint { x: 25.0, y: 90.0 },
            ],
        ];
        for points in cases {
            for closed in [0, 1] {
                let mut count = 0;
                unsafe {
                    assert_eq!(
                        clip_build_catmull_rom_path(
                            points.as_ptr(),
                            points.len(),
                            closed,
                            std::ptr::null_mut(),
                            0,
                            &mut count
                        ),
                        ClipError::Ok
                    );
                }
                let mut commands = vec![ClipArrowPathCommand::default(); count];
                unsafe {
                    assert_eq!(
                        clip_build_catmull_rom_path(
                            points.as_ptr(),
                            points.len(),
                            closed,
                            commands.as_mut_ptr(),
                            commands.len(),
                            &mut count
                        ),
                        ClipError::Ok
                    );
                }
                let vertices: Vec<_> = points.iter().map(|p| Point::new(p.x, p.y)).collect();
                let expected = catmull_rom_path_commands(
                    &vertices,
                    &vec![PathSegmentMode::Curve; vertices.len()],
                    closed != 0,
                );
                assert_eq!(commands, clip_arrow_path_commands_from_rust(&expected));
            }
        }
    }
}
