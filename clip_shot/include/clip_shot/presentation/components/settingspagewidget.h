#ifndef CLIP_SHOT_PRESENTATION_COMPONENTS_SETTINGSPAGEWIDGET_H
#define CLIP_SHOT_PRESENTATION_COMPONENTS_SETTINGSPAGEWIDGET_H

#include "clip_shot/presentation/settings/settingsregistry.h"
#include "clip_shot/presentation/styles/themecolorscheme.h"

#include <QWidget>

#include <memory>

class QEvent;
class QHideEvent;
class QShowEvent;

namespace clip_shot::presentation::settings {
class SettingsRuntimeSession;
}

class SettingsPageWidget final : public QWidget {
    Q_OBJECT

  public:
    SettingsPageWidget(const clip_shot::presentation::settings::SettingsRegistry& registry,
                       const QString& pageId,
                       clip_shot::presentation::settings::SettingsRuntimeSession& runtimeSession,
                       QWidget* parent = nullptr);
    ~SettingsPageWidget() override;

    [[nodiscard]] QString pageId() const;
    void reveal(const clip_shot::presentation::settings::SettingsLocation& location);
    void applyTheme(const clip_shot::presentation::styles::ThemeColorScheme& scheme);
    void retranslateUi();

  signals:
    void commandRequested(const clip_shot::presentation::settings::SettingsCommand& command);
    void visibleSectionChanged(const QString& sectionId);

  protected:
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif // CLIP_SHOT_PRESENTATION_COMPONENTS_SETTINGSPAGEWIDGET_H
