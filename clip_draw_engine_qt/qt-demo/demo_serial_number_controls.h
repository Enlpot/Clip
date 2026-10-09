#pragma once

#include "clip_draw_engine_qt/clip_canvas_types.h"

#include <QtGlobal>

#include <algorithm>

namespace demo_serial_number_controls {

inline bool isSerialNumberSource(ClipCanvasStyleToolbarSource source) {
    return source == ClipCanvasStyleToolbarSource::DefaultSerialNumber ||
           source == ClipCanvasStyleToolbarSource::SelectedSerialNumber;
}

inline bool hasMixedSelectedSerialNumber(const ClipCanvasStyleToolbarState& state) {
    return (state.serialNumberStyleMixed & ClipCanvasSerialNumberStyleMixedNumber) != 0;
}

inline bool selectedSerialNumberCanDecrease(const ClipCanvasStyleToolbarState& state) {
    if (state.source != ClipCanvasStyleToolbarSource::SelectedSerialNumber) {
        return false;
    }
    return hasMixedSelectedSerialNumber(state) || state.serialNumberStyle.number > 0;
}

inline bool serialNumberControlsCanDecrease(const ClipCanvasStyleToolbarState& state) {
    if (state.source == ClipCanvasStyleToolbarSource::SelectedSerialNumber) {
        return selectedSerialNumberCanDecrease(state);
    }
    if (state.source == ClipCanvasStyleToolbarSource::DefaultSerialNumber) {
        return state.serialNumberStyle.number > 0;
    }
    return false;
}

inline qint64 defaultSerialNumberAfterStep(qint64 number, qint64 delta) {
    if (delta < 0) {
        return std::max<qint64>(0, number + delta);
    }
    return number + delta;
}

} // namespace demo_serial_number_controls
