#ifndef CLIP_SHOT_PRESENTATION_COMPONENTS_SHORTCUTKEYROW_H
#define CLIP_SHOT_PRESENTATION_COMPONENTS_SHORTCUTKEYROW_H

#include "icon_core.h"
#include "clip_shot/presentation/globalshortcuttypes.h"
#include "clip_shot/presentation/components/actionrow.h"
#include "clip_shot/shortcuts/shortcutbinding.h"

#include <QString>
#include <QStringList>

#include <functional>

#include "clip_shot/presentation/styles/themecolorscheme.h"

class QWidget;
class QLabel;
class QColor;
class QEvent;
class QPaintEvent;
class QObject;
namespace clip_shot::presentation::styles {
struct MainWindowComponentMetricToken;
struct ThemeAliasMetricToken;
} // namespace clip_shot::presentation::styles
struct ShortcutKeyRowConfig {
    enum class Presentation {
        ActionCard,
        CompactFormField,
    };

    enum class ValidationScope {
        GlobalShortcut,
        ScreenshotShortcut,
        DrawingShortcut,
        PinnedWindowShortcut,
        RecordingShortcut,
    };

    QString title;
    adqt::icons::IconRef iconRef;
    clip_shot::shortcuts::ShortcutBindingList shortcuts;
    clip_shot::presentation::GlobalShortcutRegistrationState registrationState;
    QString rowState;
    bool useStableBorder = false;
    int maxShortcutCount = 2;
    std::function<clip_shot::presentation::GlobalShortcutValidationResult(
        const clip_shot::shortcuts::ShortcutBinding&)>
        shortcutValidator;
    std::function<quint64()> suspendGlobalShortcuts;
    std::function<void(quint64)> resumeGlobalShortcuts;
    bool adjustableDelay = false;
    int delaySeconds = 3;
    std::function<bool(int)> delaySetter;
    bool showRegistrationStatus = true;
    ValidationScope validationScope = ValidationScope::GlobalShortcut;
    bool allowModifierOnlyAlt = false;
    Presentation presentation = Presentation::ActionCard;
};

class ShortcutKeyRow : public ActionRow {
    Q_OBJECT

  public:
    explicit ShortcutKeyRow(
        const ShortcutKeyRowConfig& config,
        const clip_shot::presentation::styles::ThemeAliasMetricToken& metric,
        const clip_shot::presentation::styles::MainWindowComponentMetricToken& mainWindowMetric,
        QWidget* parent = nullptr);
    void applyTheme(const clip_shot::presentation::styles::ThemeColorScheme& scheme) override;
    void setTitle(const QString& title);
    void retranslateUi();
    void
    setRegistrationState(const clip_shot::presentation::GlobalShortcutRegistrationState& state);
    void setDelaySeconds(int seconds);
    [[nodiscard]] int delaySeconds() const;

  signals:
    void shortcutsChanged(const clip_shot::shortcuts::ShortcutBindingList& shortcuts);
    void delaySecondsChanged(int seconds);

  protected:
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    void openShortcutConfigDialog();
    [[nodiscard]] QString delayDisplayTitle() const;
    [[nodiscard]] QString titleLabelText() const;
    bool adjustDelayFromWheel(QEvent* event);
    void syncDelayUnderline();
    void syncRegistrationStatus();
    [[nodiscard]] QString registrationTooltipText() const;

    QWidget* m_delayUnderline = nullptr;
    adqt::widgets::AdButton* m_shortcutButton = nullptr;
    QString m_baseTitle;
    clip_shot::presentation::GlobalShortcutRegistrationState m_registrationState;
    bool m_showRegistrationStatus = true;
    ShortcutKeyRowConfig::ValidationScope m_validationScope =
        ShortcutKeyRowConfig::ValidationScope::GlobalShortcut;
    bool m_allowModifierOnlyAlt = false;
    int m_maxShortcutCount = 2;
    bool m_adjustableDelay = false;
    int m_delaySeconds = 3;
    std::function<bool(int)> m_delaySetter;
    std::function<clip_shot::presentation::GlobalShortcutValidationResult(
        const clip_shot::shortcuts::ShortcutBinding&)>
        m_shortcutValidator;
    std::function<quint64()> m_suspendGlobalShortcuts;
    std::function<void(quint64)> m_resumeGlobalShortcuts;
};

#endif // CLIP_SHOT_PRESENTATION_COMPONENTS_SHORTCUTKEYROW_H
