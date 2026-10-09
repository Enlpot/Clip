#ifndef CLIP_SHOT_PRESENTATION_COMPONENTS_TOOLBAREDITSETTINGSWIDGET_H
#define CLIP_SHOT_PRESENTATION_COMPONENTS_TOOLBAREDITSETTINGSWIDGET_H

#include "clip_shot/presentation/components/settingscustomwidget.h"
#include "clip_shot/storage/settingsadapters.h"

#include <memory>

class QEvent;

namespace clip_shot::presentation::settings {
class SettingsRuntimeSession;
}

class ToolbarEditorSettingsWidget final : public SettingsCustomWidget {
  public:
    ToolbarEditorSettingsWidget(
        clip_shot::presentation::settings::SettingsCustomRenderer renderer,
        clip_shot::storage::ScreenshotToolbarLayoutKind layoutKind,
        clip_shot::presentation::settings::SettingsRuntimeSession& runtimeSession,
        QWidget* parent = nullptr);
    ~ToolbarEditorSettingsWidget() override;

    void applyTheme(const clip_shot::presentation::styles::ThemeColorScheme& scheme) override;
    void retranslateUi() override;

  protected:
    void changeEvent(QEvent* event) override;

  private:
    struct Private;
    std::unique_ptr<Private> m_private;
};

#endif // CLIP_SHOT_PRESENTATION_COMPONENTS_TOOLBAREDITSETTINGSWIDGET_H
