#pragma once

#include "clip/image/export.h"
#include "clip/image/raster.h"

#include <cstdint>
#include <stop_token>

namespace clip::image {

struct RasterConversionOptions final {
    AlphaMode output_alpha = AlphaMode::straight;
};

// Reads a bounded source region into an RGBA8 destination. Native grayscale
// and planar YCbCr layouts are converted without materializing the full frame.
[[nodiscard]] CLIP_IMAGE_API Result<void>
read_rgba8_region(const RasterSource& source, std::uint32_t frame_index, RasterRect region,
                  MutablePlaneView destination, std::stop_token stop = {});

[[nodiscard]] CLIP_IMAGE_API Result<void>
read_rgba8_region(const RasterSource& source, std::uint32_t frame_index, RasterRect region,
                  MutablePlaneView destination, const RasterConversionOptions& options,
                  std::stop_token stop = {});

} // namespace clip::image
