use crate::abi::{handles::*, types::*};
use clip_draw_engine_core::{Point, StrokePointFilter};

pub struct ClipStrokeFilter {
    filter: StrokePointFilter,
    points: Vec<Point<f64>>,
    output: Vec<ClipArrowPoint>,
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_stroke_filter_create(
    start: ClipArrowPoint,
    sample_spacing: f64,
    response_distance: f64,
    output: *mut *mut ClipStrokeFilter,
) -> ClipError {
    ffi_error(|| {
        if output.is_null()
            || !start.x.is_finite()
            || !start.y.is_finite()
            || !sample_spacing.is_finite()
            || sample_spacing <= 0.0
            || !response_distance.is_finite()
            || response_distance <= 0.0
        {
            return ClipError::InvalidArgument;
        }
        write_out(
            output,
            Box::into_raw(Box::new(ClipStrokeFilter {
                filter: StrokePointFilter::new(
                    Point::new(start.x, start.y),
                    sample_spacing,
                    response_distance,
                ),
                points: Vec::new(),
                output: Vec::new(),
            })),
        );
        ClipError::Ok
    })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_stroke_filter_free(filter: *mut ClipStrokeFilter) {
    ffi_void(|| {
        if !filter.is_null() {
            drop(unsafe { Box::from_raw(filter) });
        }
    });
}

/// Processes only new samples. Output is borrowed until the next append or free.
/// Finishing recovers the exact endpoint once, rather than bypassing stabilization
/// on each preview. Empty batches are valid, including a finish-only call.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn clip_stroke_filter_append(
    filter: *mut ClipStrokeFilter,
    points: *const ClipArrowPoint,
    count: usize,
    finish: u8,
    output: *mut *const ClipArrowPoint,
    output_count: *mut usize,
) -> ClipError {
    ffi_error(|| {
        if filter.is_null()
            || output.is_null()
            || output_count.is_null()
            || (points.is_null() && count != 0)
            || count > 65536
        {
            return ClipError::InvalidArgument;
        }
        let input = if count == 0 {
            &[][..]
        } else {
            unsafe { std::slice::from_raw_parts(points, count) }
        };
        // Validate the entire batch before changing the filter.
        if input.iter().any(|p| !p.x.is_finite() || !p.y.is_finite()) {
            return ClipError::InvalidArgument;
        }
        let state = unsafe { &mut *filter };
        // Bound work as well as input size: a finite but enormous jump (or tiny
        // spacing) must not expand into an unbounded resampling loop.
        let mut previous = state.filter.last_raw();
        let mut sample_budget = 1.0;
        for point in input {
            sample_budget +=
                (point.x - previous.x).hypot(point.y - previous.y) / state.filter.sample_spacing();
            if sample_budget > 65536.0 {
                return ClipError::InvalidArgument;
            }
            previous = Point::new(point.x, point.y);
        }
        state.points.clear();
        state.output.clear();
        for point in input {
            state
                .filter
                .ingest(Point::new(point.x, point.y), &mut state.points);
        }
        if finish != 0
            && let Some(point) = state.filter.settle_endpoint()
        {
            state.points.push(point);
        }
        state.output.extend(
            state
                .points
                .iter()
                .map(|p| ClipArrowPoint { x: p.x, y: p.y }),
        );
        write_out(output, state.output.as_ptr());
        write_out(output_count, state.output.len());
        ClipError::Ok
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn batches_match_core_and_invalid_batch_does_not_advance_state() {
        let input: Vec<_> = (1..=600)
            .map(|i| ClipArrowPoint {
                x: i as f64 * 0.1,
                y: if i % 2 == 0 { 1.0 } else { -1.0 },
            })
            .collect();
        let mut core = StrokePointFilter::new(Point::new(0.0, 0.0), 0.75, 6.0);
        let mut expected = Vec::new();
        for point in &input {
            core.ingest(Point::new(point.x, point.y), &mut expected);
        }
        expected.extend(core.settle_endpoint());
        for batch_size in [1, 16, 128, 600] {
            unsafe {
                let mut filter = std::ptr::null_mut();
                assert_eq!(
                    clip_stroke_filter_create(
                        ClipArrowPoint { x: 0.0, y: 0.0 },
                        0.75,
                        6.0,
                        &mut filter
                    ),
                    ClipError::Ok
                );
                let mut output = std::ptr::null();
                let mut count = 0;
                let invalid = [
                    ClipArrowPoint { x: 99.0, y: 99.0 },
                    ClipArrowPoint {
                        x: f64::NAN,
                        y: 0.0,
                    },
                ];
                assert_eq!(
                    clip_stroke_filter_append(
                        filter,
                        invalid.as_ptr(),
                        invalid.len(),
                        0,
                        &mut output,
                        &mut count
                    ),
                    ClipError::InvalidArgument
                );
                let oversized = [ClipArrowPoint { x: 1e100, y: 0.0 }];
                assert_eq!(
                    clip_stroke_filter_append(
                        filter,
                        oversized.as_ptr(),
                        1,
                        0,
                        &mut output,
                        &mut count
                    ),
                    ClipError::InvalidArgument
                );
                let mut actual = Vec::new();
                for batch in input.chunks(batch_size) {
                    assert_eq!(
                        clip_stroke_filter_append(
                            filter,
                            batch.as_ptr(),
                            batch.len(),
                            0,
                            &mut output,
                            &mut count
                        ),
                        ClipError::Ok
                    );
                    actual.extend(
                        std::slice::from_raw_parts(output, count)
                            .iter()
                            .map(|p| Point::new(p.x, p.y)),
                    );
                }
                assert_eq!(
                    clip_stroke_filter_append(
                        filter,
                        std::ptr::null(),
                        0,
                        1,
                        &mut output,
                        &mut count
                    ),
                    ClipError::Ok
                );
                actual.extend(
                    std::slice::from_raw_parts(output, count)
                        .iter()
                        .map(|p| Point::new(p.x, p.y)),
                );
                assert_eq!(actual, expected);
                assert_eq!(
                    clip_stroke_filter_append(
                        filter,
                        std::ptr::null(),
                        0,
                        1,
                        &mut output,
                        &mut count
                    ),
                    ClipError::Ok
                );
                assert_eq!(count, 0, "repeated finish must not add another endpoint");
                clip_stroke_filter_free(filter);
            }
        }
    }

    #[test]
    fn rejects_invalid_parameters_and_null_buffers() {
        unsafe {
            let start = ClipArrowPoint { x: 0.0, y: 0.0 };
            let mut filter = std::ptr::null_mut();
            for spacing in [0.0, -1.0, f64::NAN, f64::INFINITY] {
                assert_eq!(
                    clip_stroke_filter_create(start, spacing, 6.0, &mut filter),
                    ClipError::InvalidArgument
                );
                assert!(filter.is_null());
            }
            assert_eq!(
                clip_stroke_filter_create(start, 0.75, 6.0, std::ptr::null_mut()),
                ClipError::InvalidArgument
            );
            assert_eq!(
                clip_stroke_filter_append(
                    filter,
                    std::ptr::null(),
                    0,
                    0,
                    std::ptr::null_mut(),
                    std::ptr::null_mut()
                ),
                ClipError::InvalidArgument
            );
            clip_stroke_filter_free(filter);
        }
    }
}
