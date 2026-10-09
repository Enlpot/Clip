#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct ClipDetectedRegion {
    int32_t x, y, width, height;
    uint32_t category;
} ClipDetectedRegion;
typedef struct ClipDetectedRegions ClipDetectedRegions;
// BGR8 input with explicit stride. 0 = success, 1 = invalid input, 2 = detection failure.
int32_t clip_detect_visual_regions(const uint8_t* pixels, size_t length, uint32_t width,
                                   uint32_t height, size_t stride, ClipDetectedRegions** result);
const ClipDetectedRegion* clip_detected_regions_data(const ClipDetectedRegions* result,
                                                     size_t* count);
void clip_detected_regions_release(ClipDetectedRegions* result);
#ifdef __cplusplus
}
#endif
