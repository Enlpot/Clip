#ifndef CLIP_SHOT_PRESENTATION_COMPONENTS_WEBDAVSYNCSETTINGSWIDGET_H
#define CLIP_SHOT_PRESENTATION_COMPONENTS_WEBDAVSYNCSETTINGSWIDGET_H

#include "clip_shot/presentation/components/settingscustomwidget.h"

#include <memory>

namespace adqt::widgets {
class AdAlert;
class AdButton;
class AdLineEdit;
class AdSwitch;
} // namespace adqt::widgets

namespace clip_shot::presentation {
class WebDavSyncSettingsWidget;
namespace settings {
class SettingsRuntimeSession;
}

// Settings page section for WebDAV sync: server URL, account, history toggle,
// and a manual "Sync now" action.
class WebDavSyncSettingsWidget final : public SettingsCustomWidget {
    Q_OBJECT
  public:
    WebDavSyncSettingsWidget(settings::SettingsRuntimeSession& session, QWidget* parent);
    ~WebDavSyncSettingsWidget() override;

    void applyTheme(const styles::ThemeColorScheme& scheme) override;
    void retranslateUi() override;

  protected:
    void changeEvent(QEvent* event) override;

  private:
    void rebuild();
    bool saveSettings();
    void savePassword(const QString& password);
    void updateStatus(const QString& status);
    void finishSync(bool success, const QString& message);
    void retranslate();
    void onEnabledToggled(bool checked);
    void onHistoryToggled(bool checked);
    void syncNow();

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace clip_shot::presentation

#endif // CLIP_SHOT_PRESENTATION_COMPONENTS_WEBDAVSYNCSETTINGSWIDGET_H
