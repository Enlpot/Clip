#ifndef CLIP_SHOT_PLATFORM_MACOS_CAPTUREWINDOWANIMATION_P_H
#define CLIP_SHOT_PLATFORM_MACOS_CAPTUREWINDOWANIMATION_P_H

#include "capturewindowlayers_p.h"
#import <AppKit/AppKit.h>

namespace clip_shot::platform::detail {
inline NSWindowAnimationBehavior captureWindowAnimation(CaptureLayer role,
                                                        NSWindowAnimationBehavior requested,
                                                        NSWindowAnimationBehavior defaultAnimation,
                                                        bool immediatePresentation = false) {
    // Capture overlays and magnifiers must appear and disappear immediately,
    // including when Qt rewrites native settings during a pooled window's lifetime.
    if (immediatePresentation ||
        (role.family == CaptureFamily::Screenshot && role.layer == kOverlayLayer))
        return NSWindowAnimationBehaviorNone;
    return requested == NSWindowAnimationBehaviorDefault ? defaultAnimation : requested;
}
} // namespace clip_shot::platform::detail
#endif
