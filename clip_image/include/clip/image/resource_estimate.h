#pragma once

#include "clip/image/codec.h"
#include "clip/image/processing.h"

#include <cstdint>

namespace clip::image {

struct ResourceEstimate final {
    std::uint64_t private_memory_bytes = 0;
    std::uint64_t mapped_bytes = 0;
    std::uint64_t output_bytes = 0;

    [[nodiscard]] Result<std::uint64_t> total_bytes() const;
};

[[nodiscard]] CLIP_IMAGE_API Result<ResourceEstimate>
estimate_decode_resources(const DocumentInfo& document, const DecodeOptions& options = {});
[[nodiscard]] CLIP_IMAGE_API Result<ResourceEstimate>
estimate_transform_resources(const DocumentInfo& document, const TransformOptions& options);
[[nodiscard]] CLIP_IMAGE_API Result<ResourceEstimate>
estimate_export_resources(const DocumentInfo& document, Format format, bool reduce_palette);
[[nodiscard]] CLIP_IMAGE_API Result<ResourceEstimate>
estimate_preview_decode_resources(const DocumentInfo& document);
[[nodiscard]] CLIP_IMAGE_API Result<ResourceEstimate>
estimate_encode_resources(const Document& document, const EncodeOptions& options);

} // namespace clip::image
