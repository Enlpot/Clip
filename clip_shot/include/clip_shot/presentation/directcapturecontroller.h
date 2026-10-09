#ifndef CLIP_SHOT_PRESENTATION_DIRECTCAPTURECONTROLLER_H
#define CLIP_SHOT_PRESENTATION_DIRECTCAPTURECONTROLLER_H

#include <QObject>
#include <QImage>
#include <QJsonObject>
#include <functional>
#include <memory>

namespace clip_shot::presentation {
class DirectCaptureController final : public QObject {
    Q_OBJECT
  public:
    explicit DirectCaptureController(QObject* parent = nullptr);
    ~DirectCaptureController() override;
    void captureFocusedWindow();
    void captureCurrentMonitor();
    void shutdown();
    [[nodiscard]] bool blocksApplicationUpdate() const;

  signals:
    void captureActivityChanged(const QString& source, bool active);
    void operationFailed(const QString& message, bool warning);

  private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace clip_shot::presentation
#endif
