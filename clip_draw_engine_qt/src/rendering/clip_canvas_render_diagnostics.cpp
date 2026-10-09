#include "clip_canvas_render_diagnostics.h"

#include <atomic>

namespace clip_canvas_render_diagnostics {
namespace {

std::atomic<bool> g_enabled{false};

} // namespace

void setEnabled(bool enabled) {
    g_enabled.store(enabled, std::memory_order_relaxed);
}

bool isEnabled() {
    return g_enabled.load(std::memory_order_relaxed);
}

} // namespace clip_canvas_render_diagnostics
