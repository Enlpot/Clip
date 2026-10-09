#ifndef CLIP_SHOT_PRESENTATION_COMPONENTS_SETTINGSCUSTOMWIDGET_H
#define CLIP_SHOT_PRESENTATION_COMPONENTS_SETTINGSCUSTOMWIDGET_H

#include "clip_shot/presentation/settings/settingscatalog.h"
#include "clip_shot/presentation/styles/themecolorscheme.h"

#include <QWidget>

namespace clip_shot::presentation::settings {
class SettingsRegistry;
class SettingsRuntimeSession;
} // namespace clip_shot::presentation::settings

class SettingsCustomWidget : public QWidget {
  public:
    explicit SettingsCustomWidget(QWidget* parent = nullptr) : QWidget(parent) {}
    ~SettingsCustomWidget() override = default;

    virtual void applyTheme(const clip_shot::presentation::styles::ThemeColorScheme& scheme) = 0;
    virtual void retranslateUi() = 0;
};

[[nodiscard]] SettingsCustomWidget* createSettingsCustomWidget(
    clip_shot::presentation::settings::SettingsCustomRenderer renderer,
    const clip_shot::presentation::settings::SettingsRegistry& registry,
    const clip_shot::presentation::settings::SettingsItemDefinition& definition,
    clip_shot::presentation::settings::SettingsRuntimeSession& runtimeSession,
    QWidget* parent = nullptr);

#endif // CLIP_SHOT_PRESENTATION_COMPONENTS_SETTINGSCUSTOMWIDGET_H
