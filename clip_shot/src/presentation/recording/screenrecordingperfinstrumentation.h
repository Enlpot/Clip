#ifndef CLIP_SHOT_SCREENRECORDINGPERFINSTRUMENTATION_H
#define CLIP_SHOT_SCREENRECORDINGPERFINSTRUMENTATION_H

#include <QtGlobal>
#include <QString>

#include <chrono>

namespace clip_shot::presentation::recording_perf {
class Sink {
  public:
    virtual ~Sink() = default;
    virtual void recordScope(const char* name, qint64 nanoseconds) = 0;
    virtual void recordMilestone(const char* name, qint64 nanoseconds) = 0;
    virtual void recordCounter(const char* name, qint64 value) = 0;
    virtual void finish(bool success) = 0;
};

// JSON-lines trace sink; used for end-to-end runs of instrumented binaries.
void configureTrace(const QString& path);
// Installs an in-process sink for benchmark-driven samples. Pass nullptr to
// fall back to the trace sink configured through configureTrace().
void installSink(Sink* sink);
void beginSample(const char* scenario, qint64 width, qint64 height);
void setSampleDescriptor(const char* scenario, qint64 width, qint64 height);
// Records the elapsed time since beginSample() the first time the name occurs
// within the active sample; later occurrences of the same name are ignored so
// call sites such as paintEvent can report unconditionally.
void milestone(const char* name);
void counter(const char* name, qint64 value = 1);

class Scope final {
  public:
    explicit Scope(const char* name) : m_name(name), m_started(std::chrono::steady_clock::now()) {}
    ~Scope();

  private:
    const char* m_name;
    std::chrono::steady_clock::time_point m_started;
};

void finish(bool success);
} // namespace clip_shot::presentation::recording_perf

#define CLIP_SHOT_RECORDING_PERF_CONCAT_IMPL(a, b) a##b
#define CLIP_SHOT_RECORDING_PERF_CONCAT(a, b) CLIP_SHOT_RECORDING_PERF_CONCAT_IMPL(a, b)
#if defined(CLIP_SHOT_RECORDING_PERF_INSTRUMENTATION)
#define CLIP_SHOT_RECORDING_PERF_SCOPE(name)                                                       \
    ::clip_shot::presentation::recording_perf::Scope CLIP_SHOT_RECORDING_PERF_CONCAT(              \
        clipShotRecordingPerfScope, __LINE__)(name)
#define CLIP_SHOT_RECORDING_PERF_BEGIN(scenario, width, height)                                    \
    ::clip_shot::presentation::recording_perf::beginSample(scenario, width, height)
#define CLIP_SHOT_RECORDING_PERF_DESCRIPTOR(scenario, width, height)                               \
    ::clip_shot::presentation::recording_perf::setSampleDescriptor(scenario, width, height)
#define CLIP_SHOT_RECORDING_PERF_MILESTONE(name)                                                   \
    ::clip_shot::presentation::recording_perf::milestone(name)
#define CLIP_SHOT_RECORDING_PERF_COUNTER(name, value)                                              \
    ::clip_shot::presentation::recording_perf::counter(name, value)
#define CLIP_SHOT_RECORDING_PERF_FINISH(success)                                                   \
    ::clip_shot::presentation::recording_perf::finish(success)
#else
// The inert forms still consume their value arguments so call sites can pass
// local variables without triggering unused-parameter diagnostics when the
// instrumentation is compiled out.
#define CLIP_SHOT_RECORDING_PERF_SCOPE(name) ((void)(name))
#define CLIP_SHOT_RECORDING_PERF_BEGIN(scenario, width, height) ((void)(scenario))
#define CLIP_SHOT_RECORDING_PERF_DESCRIPTOR(scenario, width, height) ((void)(scenario))
#define CLIP_SHOT_RECORDING_PERF_MILESTONE(name) ((void)(name))
#define CLIP_SHOT_RECORDING_PERF_COUNTER(name, value) ((void)(value))
#define CLIP_SHOT_RECORDING_PERF_FINISH(success) ((void)(success))
#endif

#endif
