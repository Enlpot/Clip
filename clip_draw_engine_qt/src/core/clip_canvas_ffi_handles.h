#pragma once

#include "clip_draw_engine.h"

class ScopedRuntimeHandle final {
  public:
    ScopedRuntimeHandle() = default;
    explicit ScopedRuntimeHandle(ClipRuntime handle) : m_handle(handle) {}
    ~ScopedRuntimeHandle() {
        reset();
    }

    ScopedRuntimeHandle(const ScopedRuntimeHandle&) = delete;
    ScopedRuntimeHandle& operator=(const ScopedRuntimeHandle&) = delete;

    ScopedRuntimeHandle(ScopedRuntimeHandle&& other) noexcept : m_handle(other.release()) {}

    ScopedRuntimeHandle& operator=(ScopedRuntimeHandle&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    ClipRuntime get() const {
        return m_handle;
    }

    ClipRuntime* outParam() {
        reset();
        return &m_handle;
    }

    ClipRuntime release() {
        ClipRuntime handle = m_handle;
        m_handle = nullptr;
        return handle;
    }

    void reset(ClipRuntime handle = nullptr) {
        if (m_handle != nullptr) {
            clip_runtime_destroy(m_handle);
        }
        m_handle = handle;
    }

  private:
    ClipRuntime m_handle = nullptr;
};

class ScopedChangedViewportList final {
  public:
    ScopedChangedViewportList() = default;
    explicit ScopedChangedViewportList(ClipChangedViewportList handle) : m_handle(handle) {}
    ~ScopedChangedViewportList() {
        reset();
    }

    ScopedChangedViewportList(const ScopedChangedViewportList&) = delete;
    ScopedChangedViewportList& operator=(const ScopedChangedViewportList&) = delete;

    ScopedChangedViewportList(ScopedChangedViewportList&& other) noexcept
        : m_handle(other.release()) {}

    ScopedChangedViewportList& operator=(ScopedChangedViewportList&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    ClipChangedViewportList get() const {
        return m_handle;
    }

    ClipChangedViewportList* outParam() {
        reset();
        return &m_handle;
    }

    ClipChangedViewportList release() {
        ClipChangedViewportList handle = m_handle;
        m_handle = nullptr;
        return handle;
    }

    void reset(ClipChangedViewportList handle = nullptr) {
        if (m_handle != nullptr) {
            clip_changed_viewports_destroy(m_handle);
        }
        m_handle = handle;
    }

  private:
    ClipChangedViewportList m_handle = nullptr;
};

// Balances a presentation scope even when a caller exits after a failed command.
class ScopedPresentationUpdate final {
  public:
    explicit ScopedPresentationUpdate(ClipRuntime runtime)
        : m_runtime(clip_runtime_begin_presentation_update(runtime) == CLIP_OK ? runtime
                                                                               : nullptr) {}
    ~ScopedPresentationUpdate() {
        ScopedChangedViewportList ignored;
        finish(ignored);
    }

    ScopedPresentationUpdate(const ScopedPresentationUpdate&) = delete;
    ScopedPresentationUpdate& operator=(const ScopedPresentationUpdate&) = delete;

    bool isActive() const {
        return m_runtime != nullptr;
    }

    bool finish(ScopedChangedViewportList& changedViewports) {
        if (m_runtime == nullptr) {
            return false;
        }
        ClipRuntime runtime = m_runtime;
        m_runtime = nullptr;
        return clip_runtime_end_presentation_update_ex(runtime, changedViewports.outParam()) ==
               CLIP_OK;
    }

  private:
    ClipRuntime m_runtime = nullptr;
};

class ScopedPatchHandle final {
  public:
    ScopedPatchHandle() = default;
    explicit ScopedPatchHandle(ClipPatchHandle handle) : m_handle(handle) {}
    ~ScopedPatchHandle() {
        reset();
    }

    ScopedPatchHandle(const ScopedPatchHandle&) = delete;
    ScopedPatchHandle& operator=(const ScopedPatchHandle&) = delete;

    ScopedPatchHandle(ScopedPatchHandle&& other) noexcept : m_handle(other.release()) {}

    ScopedPatchHandle& operator=(ScopedPatchHandle&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    ClipPatchHandle get() const {
        return m_handle;
    }

    ClipPatchHandle* outParam() {
        reset();
        return &m_handle;
    }

    ClipPatchHandle release() {
        ClipPatchHandle handle = m_handle;
        m_handle = nullptr;
        return handle;
    }

    void reset(ClipPatchHandle handle = nullptr) {
        if (m_handle != nullptr) {
            clip_patch_destroy(m_handle);
        }
        m_handle = handle;
    }

  private:
    ClipPatchHandle m_handle = nullptr;
};
