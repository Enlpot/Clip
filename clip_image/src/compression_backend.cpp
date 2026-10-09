#include "clip/image/codec.h"

#if defined(CLIP_IMAGE_HAS_PNG)
#include <zlib.h>

#if !defined(ZLIBNG_VERSION)
#error "clip_image PNG support requires the repository zlib-ng compatibility overlay"
#endif
#endif

namespace clip::image {

std::string_view compression_backend_version(Format format) noexcept {
#if defined(CLIP_IMAGE_HAS_PNG)
    if (format == Format::png)
        return zlibVersion();
#else
    (void)format;
#endif
    return {};
}

} // namespace clip::image
