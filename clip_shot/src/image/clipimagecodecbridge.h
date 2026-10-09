#pragma once

#include <stdint.h>

#if defined(_WIN32) && !defined(CLIP_SHOT_IMAGE_CODEC_BACKEND_STATIC)
#if defined(CLIP_SHOT_IMAGE_CODEC_BACKEND_BUILD)
#define CLIP_SHOT_IMAGE_CODEC_API __declspec(dllexport)
#else
#define CLIP_SHOT_IMAGE_CODEC_API __declspec(dllimport)
#endif
#define CLIP_SHOT_IMAGE_CODEC_CALL __cdecl
#elif defined(_WIN32)
#define CLIP_SHOT_IMAGE_CODEC_API
#define CLIP_SHOT_IMAGE_CODEC_CALL __cdecl
#elif defined(__GNUC__) || defined(__clang__)
#define CLIP_SHOT_IMAGE_CODEC_API __attribute__((visibility("default")))
#define CLIP_SHOT_IMAGE_CODEC_CALL
#else
#define CLIP_SHOT_IMAGE_CODEC_API
#define CLIP_SHOT_IMAGE_CODEC_CALL
#endif

#define CLIP_SHOT_IMAGE_CODEC_ABI_VERSION 3U

#ifdef __cplusplus
extern "C" {
#endif

enum ClipShotImageCodecFormat {
    CLIP_SHOT_IMAGE_CODEC_FORMAT_UNKNOWN = 0,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_BMP = 1,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_CUR = 2,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_GIF = 3,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_ICO = 4,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_JPEG = 5,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_PBM = 6,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_PGM = 7,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_PNG = 8,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_PPM = 9,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_SVG = 10,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_SVGZ = 11,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_XBM = 12,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_XPM = 13,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_HEIF = 14,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_AVIF = 15,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_JXL = 16,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_EXR = 17,
    CLIP_SHOT_IMAGE_CODEC_FORMAT_WEBP = 18,
};

enum ClipShotImageCodecChromaSubsampling {
    CLIP_SHOT_IMAGE_CODEC_CHROMA_NONE = 0,
    CLIP_SHOT_IMAGE_CODEC_CHROMA_YUV444 = 1,
    CLIP_SHOT_IMAGE_CODEC_CHROMA_YUV422 = 2,
    CLIP_SHOT_IMAGE_CODEC_CHROMA_YUV420 = 3,
    CLIP_SHOT_IMAGE_CODEC_CHROMA_YUV440 = 4,
    CLIP_SHOT_IMAGE_CODEC_CHROMA_YUV411 = 5,
    CLIP_SHOT_IMAGE_CODEC_CHROMA_YUV441 = 6,
};

enum ClipShotImageCodecAlphaContent {
    CLIP_SHOT_IMAGE_CODEC_ALPHA_OPAQUE = 0,
    CLIP_SHOT_IMAGE_CODEC_ALPHA_NON_OPAQUE = 1,
};

enum ClipShotImageCodecPixelRoundTrip {
    CLIP_SHOT_IMAGE_CODEC_PIXEL_ROUND_TRIP_EXACT = 0,
    CLIP_SHOT_IMAGE_CODEC_PIXEL_ROUND_TRIP_CODEC_ARTIFACT = 1,
};

enum ClipShotImageCodecColorPrimaries {
    CLIP_SHOT_IMAGE_CODEC_PRIMARIES_UNKNOWN = 0,
    CLIP_SHOT_IMAGE_CODEC_PRIMARIES_SRGB = 1,
    CLIP_SHOT_IMAGE_CODEC_PRIMARIES_DISPLAY_P3 = 2,
    CLIP_SHOT_IMAGE_CODEC_PRIMARIES_ADOBE_RGB = 3,
    CLIP_SHOT_IMAGE_CODEC_PRIMARIES_REC2020 = 4,
    CLIP_SHOT_IMAGE_CODEC_PRIMARIES_CUSTOM = 5,
};

enum ClipShotImageCodecTransferFunction {
    CLIP_SHOT_IMAGE_CODEC_TRANSFER_UNKNOWN = 0,
    CLIP_SHOT_IMAGE_CODEC_TRANSFER_LINEAR = 1,
    CLIP_SHOT_IMAGE_CODEC_TRANSFER_SRGB = 2,
    CLIP_SHOT_IMAGE_CODEC_TRANSFER_GAMMA = 3,
    CLIP_SHOT_IMAGE_CODEC_TRANSFER_PQ = 4,
    CLIP_SHOT_IMAGE_CODEC_TRANSFER_HLG = 5,
};

typedef struct ClipShotImageCodecColorEncoding {
    uint8_t* icc_profile;
    uint64_t icc_profile_size;
    uint32_t primaries;
    uint32_t transfer;
} ClipShotImageCodecColorEncoding;

typedef struct ClipShotImageCodecEncodeOptions {
    uint32_t struct_size;
    uint32_t abi_version;
    uint32_t format;
    int32_t quality;
    int32_t effort;
    int32_t lossless_effort;
    int32_t compression_level;
    uint8_t lossless;
    uint8_t preserve_metadata;
    uint8_t progressive;
    uint8_t interlaced;
    uint8_t has_chroma_subsampling;
    uint8_t chroma_subsampling;
    uint8_t has_verified_alpha_content;
    uint8_t verified_alpha_content;
} ClipShotImageCodecEncodeOptions;

typedef struct ClipShotImageCodecBuffer {
    uint8_t* data;
    uint64_t size;
    uint32_t width;
    uint32_t height;
    uint64_t row_stride;
    // Decoders preserve the selected frame's color declaration without converting pixels.
    // The profile and pixels are both owned by this buffer and released together.
    ClipShotImageCodecColorEncoding color;
} ClipShotImageCodecBuffer;

typedef struct ClipShotImageCodecImageInfo {
    uint32_t width;
    uint32_t height;
} ClipShotImageCodecImageInfo;

enum ClipShotImageCodecSkinError {
    CLIP_SHOT_IMAGE_CODEC_SKIN_ERROR_NONE = 0,
    CLIP_SHOT_IMAGE_CODEC_SKIN_ERROR_UNSUPPORTED_FORMAT = 1,
    CLIP_SHOT_IMAGE_CODEC_SKIN_ERROR_INPUT_TOO_LARGE = 2,
    CLIP_SHOT_IMAGE_CODEC_SKIN_ERROR_INVALID_IMAGE = 3,
    CLIP_SHOT_IMAGE_CODEC_SKIN_ERROR_RESOURCE_LIMIT = 4,
};

typedef struct ClipShotImageCodecSkinInfo {
    uint32_t canvas_width;
    uint32_t canvas_height;
    uint32_t frame_x;
    uint32_t frame_y;
    uint32_t orientation;
} ClipShotImageCodecSkinInfo;

typedef int32_t(CLIP_SHOT_IMAGE_CODEC_CALL* ClipShotImageCodecReadRowsCallback)(
    void* context, uint32_t first_row, uint32_t row_count, uint64_t destination_stride,
    uint8_t* destination, uint64_t destination_size);
typedef int32_t(CLIP_SHOT_IMAGE_CODEC_CALL* ClipShotImageCodecWriteCallback)(void* context,
                                                                             const uint8_t* source,
                                                                             uint64_t source_size);
typedef int32_t(CLIP_SHOT_IMAGE_CODEC_CALL* ClipShotImageCodecPositionCallback)(void* context,
                                                                                uint64_t* position);
typedef int32_t(CLIP_SHOT_IMAGE_CODEC_CALL* ClipShotImageCodecSeekCallback)(void* context,
                                                                            uint64_t position);
typedef int32_t(CLIP_SHOT_IMAGE_CODEC_CALL* ClipShotImageCodecFlushCallback)(void* context);
typedef int32_t(CLIP_SHOT_IMAGE_CODEC_CALL* ClipShotImageCodecCancelCallback)(void* context);

typedef struct ClipShotImageCodecRgba8Source {
    uint32_t struct_size;
    uint32_t abi_version;
    void* context;
    uint32_t width;
    uint32_t height;
    ClipShotImageCodecReadRowsCallback read_rows;
    ClipShotImageCodecCancelCallback is_cancelled;
} ClipShotImageCodecRgba8Source;

typedef struct ClipShotImageCodecByteSink {
    uint32_t struct_size;
    uint32_t abi_version;
    void* context;
    ClipShotImageCodecWriteCallback write;
    ClipShotImageCodecPositionCallback position;
    ClipShotImageCodecSeekCallback seek;
    ClipShotImageCodecFlushCallback flush;
    ClipShotImageCodecCancelCallback is_cancelled;
    uint8_t seekable;
    uint8_t reserved[7];
} ClipShotImageCodecByteSink;

typedef struct ClipShotImageCodecEncodeResult {
    uint32_t struct_size;
    uint32_t abi_version;
    uint64_t bytes_written;
    uint32_t encoded_format;
    uint32_t canvas_width;
    uint32_t canvas_height;
    uint32_t emitted_frame_count;
    uint8_t pixel_round_trip;
    uint8_t encoder_finalized_and_sink_flushed;
    uint8_t reserved[6];
} ClipShotImageCodecEncodeResult;

typedef struct ClipShotImageCodecEncoderOptionRange {
    int32_t minimum;
    int32_t maximum;
    int32_t default_value;
} ClipShotImageCodecEncoderOptionRange;

typedef struct ClipShotImageCodecEncoderInfo {
    uint32_t struct_size;
    uint32_t abi_version;
    uint32_t format;
    uint32_t features;
    ClipShotImageCodecEncoderOptionRange quality;
    ClipShotImageCodecEncoderOptionRange effort;
    ClipShotImageCodecEncoderOptionRange lossless_effort;
    ClipShotImageCodecEncoderOptionRange compression_level;
} ClipShotImageCodecEncoderInfo;

CLIP_SHOT_IMAGE_CODEC_API uint32_t CLIP_SHOT_IMAGE_CODEC_CALL
clip_shot_image_codec_abi_version(void);
CLIP_SHOT_IMAGE_CODEC_API int32_t CLIP_SHOT_IMAGE_CODEC_CALL
clip_shot_image_codec_encoder_info(uint32_t format, ClipShotImageCodecEncoderInfo* output);

// Both encoding entry points consume straight-alpha, sRGB RGBA8 pixels.
// Output buffers must be zero-initialized and released before being reused.
CLIP_SHOT_IMAGE_CODEC_API int32_t CLIP_SHOT_IMAGE_CODEC_CALL clip_shot_image_codec_encode_rgba8(
    const uint8_t* pixels, uint64_t pixels_size, uint32_t width, uint32_t height,
    uint64_t row_stride, const ClipShotImageCodecEncodeOptions* options,
    ClipShotImageCodecBuffer* output, char* error, uint64_t error_capacity);

CLIP_SHOT_IMAGE_CODEC_API int32_t CLIP_SHOT_IMAGE_CODEC_CALL
clip_shot_image_codec_encode_rgba8_stream(const ClipShotImageCodecRgba8Source* source,
                                          const ClipShotImageCodecByteSink* sink,
                                          const ClipShotImageCodecEncodeOptions* options,
                                          ClipShotImageCodecEncodeResult* result, char* error,
                                          uint64_t error_capacity);

CLIP_SHOT_IMAGE_CODEC_API int32_t CLIP_SHOT_IMAGE_CODEC_CALL clip_shot_image_codec_decode_rgba8(
    const uint8_t* encoded, uint64_t encoded_size, uint32_t expected_format,
    ClipShotImageCodecBuffer* output, char* error, uint64_t error_capacity);

// Decode only the ICO frame nearest the requested extent (first entry wins ties).
CLIP_SHOT_IMAGE_CODEC_API int32_t CLIP_SHOT_IMAGE_CODEC_CALL
clip_shot_image_codec_decode_icon_rgba8(const uint8_t* encoded, uint64_t encoded_size,
                                        uint32_t preferred_extent, ClipShotImageCodecBuffer* output,
                                        char* error, uint64_t error_capacity);

// Bounded PNG/JPEG/WebP preview decoding. The encoded storage is borrowed only
// for this synchronous call. Orientation is applied after preview reduction by
// the Qt adapter; canvas coordinates describe an APNG first-frame subrectangle.
CLIP_SHOT_IMAGE_CODEC_API int32_t CLIP_SHOT_IMAGE_CODEC_CALL
clip_shot_image_codec_decode_skin_rgba8(const uint8_t* encoded, uint64_t encoded_size,
                                        ClipShotImageCodecBuffer* output,
                                        ClipShotImageCodecSkinInfo* info, uint32_t* failure);

CLIP_SHOT_IMAGE_CODEC_API int32_t CLIP_SHOT_IMAGE_CODEC_CALL clip_shot_image_codec_decode_bgra8(
    const uint8_t* encoded, uint64_t encoded_size, uint32_t expected_format,
    ClipShotImageCodecBuffer* output, char* error, uint64_t error_capacity);

CLIP_SHOT_IMAGE_CODEC_API int32_t CLIP_SHOT_IMAGE_CODEC_CALL clip_shot_image_codec_inspect(
    const uint8_t* encoded, uint64_t encoded_size, uint32_t expected_format,
    ClipShotImageCodecImageInfo* output, char* error, uint64_t error_capacity);

CLIP_SHOT_IMAGE_CODEC_API void CLIP_SHOT_IMAGE_CODEC_CALL
clip_shot_image_codec_release_buffer(ClipShotImageCodecBuffer* buffer);

// Export-only operations use caller-owned, optionally file-mapped RGBA buffers.
CLIP_SHOT_IMAGE_CODEC_API int32_t CLIP_SHOT_IMAGE_CODEC_CALL clip_shot_image_codec_resize_rgba8(
    const ClipShotImageCodecRgba8Source* source, uint8_t* destination, uint64_t destination_stride,
    uint64_t destination_size, uint32_t output_width, uint32_t output_height, char* error,
    uint64_t error_capacity);
CLIP_SHOT_IMAGE_CODEC_API int32_t CLIP_SHOT_IMAGE_CODEC_CALL clip_shot_image_codec_decode_file_into(
    const char* utf8_path, uint8_t* destination, uint32_t width, uint32_t height,
    void* cancel_context, ClipShotImageCodecCancelCallback cancelled, char* error,
    uint64_t error_capacity);
CLIP_SHOT_IMAGE_CODEC_API int32_t CLIP_SHOT_IMAGE_CODEC_CALL
clip_shot_image_codec_export_limits(uint32_t format, uint32_t* width, uint32_t* height);

#ifdef __cplusplus
}
#endif
