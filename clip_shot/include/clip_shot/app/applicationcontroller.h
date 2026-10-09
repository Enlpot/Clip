#ifndef CLIP_SHOT_APP_APPLICATIONCONTROLLER_H
#define CLIP_SHOT_APP_APPLICATIONCONTROLLER_H

#include <QObject>
#include <QStringList>

#include <memory>

class QApplication;

namespace clip_shot::app {
class ApplicationController final : public QObject {
    Q_OBJECT

  public:
    explicit ApplicationController(QApplication& application, QObject* parent = nullptr);
    ~ApplicationController() override;

    void start();
    void showMainWindow();
    void handleLaunchRequest(const QStringList& arguments);
    void restorePinnedWindows();

  private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace clip_shot::app

#endif // CLIP_SHOT_APP_APPLICATIONCONTROLLER_H
