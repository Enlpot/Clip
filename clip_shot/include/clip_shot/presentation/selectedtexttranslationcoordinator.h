#ifndef CLIP_SHOT_PRESENTATION_SELECTEDTEXTTRANSLATIONCOORDINATOR_H
#define CLIP_SHOT_PRESENTATION_SELECTEDTEXTTRANSLATIONCOORDINATOR_H

#include "clip_shot/presentation/selectedtexttranslationcontroller.h"

#include <QObject>
#include <QPointer>
#include <functional>
#include <memory>

class QScreen;
class ClipShotApiClient;
namespace clip_shot::storage {
class ConfigurationStore;
}
namespace clip_shot::presentation {
class StandaloneTranslationWindow;

class SelectedTextTranslationCoordinator final : public QObject {
    Q_OBJECT
  public:
    using ScreenProvider = std::function<QScreen*()>;
    SelectedTextTranslationCoordinator(storage::ConfigurationStore& configuration,
                                       ClipShotApiClient* client, QObject* parent = nullptr,
                                       std::unique_ptr<SelectedTextCaptureBackend> backend = {},
                                       ScreenProvider screenProvider = {});
    ~SelectedTextTranslationCoordinator() override;
    void capture();
    void presentText(const QString& text);
    void shutdown();

  signals:
    void mainTranslationRequested(const QString& text);

  private:
    void routeText(const QString& text, bool standalone, QScreen* screen);

    storage::ConfigurationStore& m_configuration;
    SelectedTextTranslationController* m_capture;
    StandaloneTranslationWindow* m_window;
    ScreenProvider m_screenProvider;
    QPointer<QScreen> m_screen;
    bool m_standalone = false;
    bool m_shutdown = false;
};
} // namespace clip_shot::presentation
#endif
