#ifndef CLIP_SHOT_CLOUDUPLOADSETTINGSWIDGET_H
#define CLIP_SHOT_CLOUDUPLOADSETTINGSWIDGET_H
#include "clip_shot/presentation/components/settingscustomwidget.h"
#include "clip_shot/clouduploadconfiguration.h"
#include <QPointer>
#include <array>
class QLabel;
class QVBoxLayout;
namespace adqt::widgets {
class AdButton;
class AdAlert;
class AdModal;
class AdComboBox;
} // namespace adqt::widgets
namespace clip_shot::presentation::components::form_fields {
class FormField;
}
class CloudUploadSettingsWidget final : public SettingsCustomWidget {
    Q_OBJECT
  public:
    explicit CloudUploadSettingsWidget(
        clip_shot::presentation::settings::SettingsRuntimeSession& session,
        QWidget* parent = nullptr);
    void applyTheme(const clip_shot::presentation::styles::ThemeColorScheme& scheme) override;
    void retranslateUi() override;

  protected:
    void changeEvent(QEvent* event) override;

  private:
    void rebuild();
    void openEditor(const QString& id = {});
    void deleteConfiguration(const QString& id);
    void copyConfiguration(const QString& id);
    void submitEditor(bool validateOnly = false);
    void translateEditor();
    bool save(const clip_shot::CloudUploadSettings& values);
    clip_shot::presentation::settings::SettingsRuntimeSession& m_session;
    clip_shot::presentation::styles::ThemeColorScheme m_scheme;
    QVBoxLayout* m_rows = nullptr;
    QLabel* m_title = nullptr;
    adqt::widgets::AdButton* m_add = nullptr;
    adqt::widgets::AdAlert* m_error = nullptr;
    clip_shot::presentation::components::form_fields::FormField* m_default = nullptr;
    QPointer<adqt::widgets::AdModal> m_modal;
    QPointer<adqt::widgets::AdModal> m_deleteModal;
    adqt::widgets::AdAlert* m_modalError = nullptr;
    std::array<clip_shot::presentation::components::form_fields::FormField*, 9> m_fields{};
    clip_shot::presentation::components::form_fields::FormField* m_addressing = nullptr;
    clip_shot::presentation::components::form_fields::FormField* m_protocol = nullptr;
    QString m_editId;
    bool m_editing = false;
    bool m_editorValidationAttempted = false;
};
#endif
