#ifndef CLIP_SHOT_PLATFORM_WINDOWS_SCROLLINPUT_H
#define CLIP_SHOT_PLATFORM_WINDOWS_SCROLLINPUT_H

#include <QPoint>
#include <QRect>

namespace clip_shot::platform::windows {
struct ScrollInputResult {
    enum class Status {
        Posted,
        InvalidRequest,
        TargetNotFound,
        CoordinateFailure,
        PostFailed,
        Unsupported
    };
    Status status = Status::Unsupported;
    quint32 error = 0;
};
ScrollInputResult sendScrollingWheelStep(const QRect& physicalSelection, const QPoint& wheelDelta);
} // namespace clip_shot::platform::windows

#endif // CLIP_SHOT_PLATFORM_WINDOWS_SCROLLINPUT_H
