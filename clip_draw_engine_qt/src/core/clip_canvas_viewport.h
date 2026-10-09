#pragma once

#include <cstdint>

#include "clip_draw_engine.h"

class ClipCanvasViewport final {
  public:
    ClipCanvasViewport() = default;
    ~ClipCanvasViewport();

    ClipCanvasViewport(const ClipCanvasViewport&) = delete;
    ClipCanvasViewport& operator=(const ClipCanvasViewport&) = delete;

    ClipCanvasViewport(ClipCanvasViewport&& other) noexcept;
    ClipCanvasViewport& operator=(ClipCanvasViewport&& other) noexcept;

    bool create(ClipRuntime runtime, const ClipEngineConfig& config);
    void reset();

    ClipViewport get() const;
    ClipRuntime runtime() const;
    bool isValid() const;
    bool id(std::uint64_t* outId) const;

  private:
    ClipRuntime m_runtime = nullptr;
    ClipViewport m_viewport = nullptr;
};

namespace clip_canvas_viewport {

ClipEngineConfig defaultEngineConfig();

} // namespace clip_canvas_viewport
