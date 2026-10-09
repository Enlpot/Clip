#ifndef CLIP_SHOT_PRESENTATION_COMPONENTS_GLOBALMOUSEROW_H
#define CLIP_SHOT_PRESENTATION_COMPONENTS_GLOBALMOUSEROW_H

#include "clip_shot/presentation/components/shortcutconfigurationbutton.h"
#include "clip_shot/presentation/components/actionrow.h"
#include "clip_shot/presentation/settings/settingscatalog.h"
#include "clip_shot/presentation/styles/themecolorscheme.h"

#include <QPointer>
#include <QWidget>

class QLabel;
class QEvent;
class QMouseEvent;
namespace clip_shot::presentation::components::form_fields {
class FormField;
}
namespace adqt::widgets {
class AdFormItem;
class AdModal;
class AdSelect;
} // namespace adqt::widgets
namespace clip_shot::presentation::settings {
class SettingsRuntimeSession;
}

class GlobalMouseRow final : public ActionRow {
    Q_OBJECT

  public:
    GlobalMouseRow(const QString& title,
                   clip_shot::presentation::settings::SettingsGlobalMouseAction action,
                   clip_shot::presentation::settings::SettingsRuntimeSession& runtimeSession,
                   const clip_shot::presentation::styles::ThemeColorScheme& colorScheme,
                   QWidget* parent = nullptr);

    void setTitle(const QString& title);
    void setCombination(
        const clip_shot::presentation::settings::SettingsGlobalMouseCombination& combination);
    [[nodiscard]] ShortcutConfigurationButton* configurationButton() const;
    void applyTheme(const clip_shot::presentation::styles::ThemeColorScheme& scheme) override;
    void retranslateUi();

  signals:
    void dragRequested(clip_shot::presentation::settings::SettingsGlobalMouseAction action);

  protected:
    void changeEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

  private:
    void openConfigurationDialog();
    void syncButton();
    void syncModalText();
    void syncModalValidation();
    [[nodiscard]] clip_shot::presentation::settings::SettingsGlobalMouseCombination
    modalCombination() const;
    [[nodiscard]] QString activationKeyLabel(const QString& value) const;
    [[nodiscard]] QString mouseButtonLabel(const QString& value) const;

    QString m_title;
    clip_shot::presentation::settings::SettingsGlobalMouseAction m_action;
    clip_shot::presentation::settings::SettingsRuntimeSession& m_runtimeSession;
    clip_shot::presentation::settings::SettingsGlobalMouseCombination m_combination;
    ShortcutConfigurationButton* m_button = nullptr;
    QPointer<adqt::widgets::AdModal> m_modal;
    QPointer<adqt::widgets::AdFormItem> m_activationField;
    QPointer<clip_shot::presentation::components::form_fields::FormField> m_activationController;
    QPointer<adqt::widgets::AdFormItem> m_mouseButtonField;
    QPointer<clip_shot::presentation::components::form_fields::FormField> m_mouseButtonController;
    QPointer<QLabel> m_validationLabel;
    QPointer<adqt::widgets::AdSelect> m_activationSelect;
    QPointer<adqt::widgets::AdSelect> m_mouseButtonSelect;
};

#endif // CLIP_SHOT_PRESENTATION_COMPONENTS_GLOBALMOUSEROW_H
