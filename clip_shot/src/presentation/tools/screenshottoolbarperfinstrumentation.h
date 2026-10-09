#ifndef CLIP_SHOT_SCREENSHOTTOOLBARPERFINSTRUMENTATION_H
#define CLIP_SHOT_SCREENSHOTTOOLBARPERFINSTRUMENTATION_H

#include <QtGlobal>

#include <chrono>

namespace clip_shot::presentation::toolbar_perf {
class Sink {
  public:
    virtual ~Sink() = default;
    virtual void recordScope(const char* name, qint64 elapsedNanoseconds) = 0;
    virtual void recordCounter(const char* name, qint64 value) = 0;
};

#if defined(CLIP_SHOT_TOOLBAR_PERF_INSTRUMENTATION)
inline Sink* activeSink = nullptr;

inline void setSink(Sink* sink) {
    activeSink = sink;
}

class Scope final {
  public:
    explicit Scope(const char* name) : m_name(name), m_started(std::chrono::steady_clock::now()) {}

    ~Scope() {
        if (activeSink == nullptr) {
            return;
        }
        const auto elapsed = std::chrono::steady_clock::now() - m_started;
        activeSink->recordScope(
            m_name, std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count());
    }

  private:
    const char* m_name;
    std::chrono::steady_clock::time_point m_started;
};

inline void counter(const char* name, qint64 value = 1) {
    if (activeSink != nullptr) {
        activeSink->recordCounter(name, value);
    }
}
#else
inline void setSink(Sink*) {}

class Scope final {
  public:
    explicit Scope(const char*) {}
};

inline void counter(const char*, qint64 = 1) {}
#endif
} // namespace clip_shot::presentation::toolbar_perf

#define CLIP_SHOT_TOOLBAR_PERF_CONCAT_IMPL(a, b) a##b
#define CLIP_SHOT_TOOLBAR_PERF_CONCAT(a, b) CLIP_SHOT_TOOLBAR_PERF_CONCAT_IMPL(a, b)
#define CLIP_SHOT_TOOLBAR_PERF_SCOPE(name)                                                         \
    ::clip_shot::presentation::toolbar_perf::Scope CLIP_SHOT_TOOLBAR_PERF_CONCAT(                  \
        clipShotToolbarPerfScope, __LINE__)(name)
#define CLIP_SHOT_TOOLBAR_PERF_COUNTER(name) ::clip_shot::presentation::toolbar_perf::counter(name)

#endif // CLIP_SHOT_SCREENSHOTTOOLBARPERFINSTRUMENTATION_H
