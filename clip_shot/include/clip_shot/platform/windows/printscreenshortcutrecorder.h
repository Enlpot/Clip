#ifndef CLIP_SHOT_PLATFORM_WINDOWS_PRINTSCREENSHORTCUTRECORDER_H
#define CLIP_SHOT_PLATFORM_WINDOWS_PRINTSCREENSHORTCUTRECORDER_H

#include <Qt>

#include <functional>
#include <memory>

class QKeyEvent;
class QWidget;

namespace clip_shot::platform::windows {

class PrintScreenShortcutRecorder final {
  public:
    using Handler = std::function<void(Qt::KeyboardModifiers)>;

    PrintScreenShortcutRecorder(QWidget& target, Handler handler);
    ~PrintScreenShortcutRecorder();

    bool handleKeyEvent(const QKeyEvent& event);
    void cancelPendingCapture();

  private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace clip_shot::platform::windows

#endif // CLIP_SHOT_PLATFORM_WINDOWS_PRINTSCREENSHORTCUTRECORDER_H
