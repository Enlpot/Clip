#ifndef CLIP_SHOT_PRESENTATION_COMPONENTS_CONTENTCARDWIDGET_H
#define CLIP_SHOT_PRESENTATION_COMPONENTS_CONTENTCARDWIDGET_H

#include "clip_shot/presentation/settings/settingsregistry.h"
#include "clip_shot/presentation/styles/themecolorscheme.h"

#include <QFrame>
#include <QPointer>
#include <QBrush>

#include <memory>

class ClipShotApiClient;
class QEvent;
class QPaintEvent;
class QStackedWidget;
class SettingsPageWidget;
class ScreenshotHistoryPageWidget;
class QWidget;
namespace clip_shot::presentation::settings {
class SettingsRuntimeSession;
}

class ContentCardWidget final : public QFrame {
    Q_OBJECT

  public:
    ContentCardWidget(const clip_shot::presentation::settings::SettingsRegistry& registry,
                      clip_shot::presentation::settings::SettingsRuntimeSession& runtimeSession,
                      QWidget* parent = nullptr, ClipShotApiClient* translationClient = nullptr);
    ~ContentCardWidget() override;

    [[nodiscard]] QString currentRoute() const;
    [[nodiscard]] clip_shot::presentation::settings::SettingsLocation currentLocation() const;
    [[nodiscard]] QVector<clip_shot::presentation::settings::SettingsSectionSummary>
    currentSections() const;
    void setCurrentRoute(const QString& route);
    void activateSection(const QString& sectionId);
    void navigateTo(const clip_shot::presentation::settings::SettingsLocation& location);
    void showGeneralSettings();
    void showScreenshotSettings();
    void showTranslation(const QString& text);
    void applyTheme(const clip_shot::presentation::styles::ThemeColorScheme& scheme);
    void setSkinMaskOpacity(qreal opacity);
    void retranslateUi();

  signals:
    void closeWindowRequested();
    void routeChanged(const QString& route);
    void sectionListChanged();
    void locationChanged(const clip_shot::presentation::settings::SettingsLocation& location);
    void screenshotRequested();
    void quickActionRequested(clip_shot::presentation::GlobalShortcutAction action);
    void
    globalMouseDragRequested(clip_shot::presentation::settings::SettingsGlobalMouseAction action);
    void screenshotHistoryEditRequested(const QString& recordId);
    void screenshotHistoryPinRequested(const QString& recordId);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void changeEvent(QEvent* event) override;

  private:
    QWidget*
    createPage(const clip_shot::presentation::settings::SettingsPageDefinition& definition);
    void destroyActivePage();
    void handleCommand(const clip_shot::presentation::settings::SettingsCommand& command);
    void updateBackgroundBrush();

    const clip_shot::presentation::settings::SettingsRegistry& m_registry;
    clip_shot::presentation::settings::SettingsRuntimeSession& m_runtimeSession;
    ClipShotApiClient* m_translationClient = nullptr;
    QStackedWidget* m_stack = nullptr;
    QPointer<QWidget> m_activePage;
    QString m_activePageId;
    clip_shot::presentation::settings::SettingsLocation m_currentLocation;
    clip_shot::presentation::styles::ThemeColorScheme m_colorScheme;
    QBrush m_backgroundBrush;
    qreal m_skinMaskOpacity = 1.0;
};

#endif // CLIP_SHOT_PRESENTATION_COMPONENTS_CONTENTCARDWIDGET_H
