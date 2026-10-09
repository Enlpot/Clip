#pragma once

#if defined(_WIN32) && defined(CLIP_IMAGE_SHARED)
#if defined(CLIP_IMAGE_BUILDING_LIBRARY)
#define CLIP_IMAGE_API __declspec(dllexport)
#else
#define CLIP_IMAGE_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define CLIP_IMAGE_API __attribute__((visibility("default")))
#else
#define CLIP_IMAGE_API
#endif
