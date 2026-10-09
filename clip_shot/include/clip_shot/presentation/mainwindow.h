#ifndef CLIP_SHOT_PRESENTATION_MAINWINDOW_H
#define CLIP_SHOT_PRESENTATION_MAINWINDOW_H

#include <QByteArray>
#include <QMainWindow>

#include "clip_shot/presentation/globalshortcuttypes.h"
#include "clip_shot/presentation/globalmousetypes.h"
#include "clip_shot/presentation/windowgeometrymemory.h"

class ClipShotApiClient;
class QCloseEvent;
class QEvent;
class QResizeEvent;
class QWidget;
class SidebarWidget;
class ContentCardWidget;
class MainContentHeaderWidget;
class TitleBarWidget;
namespace clip_shot::presentation::styles {
struct ThemeColorScheme;
}
namespace clip_shot::presentation {
class MainWindowSkinWidget;
}
namespace clip_shot::presentation::settings {
class SettingsRegistry;
class SettingsRuntimeSession;
} // namespace clip_shot::presentation::settings

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    MainWindow(const clip_shot::presentation::settings::SettingsRegistry& registry,
               clip_shot::presentation::settings::SettingsRuntimeSession& runtimeSession,
               QWidget* parent = nullptr, ClipShotApiClient* translationClient = nullptr);

    void showAndActivate();
    void showGeneralSettings();
    void showScreenshotSettings();
    void showSettingsLocation(const QString& pageId, const QString& sectionId = {});
    void showAbout();
    void showAppPermissions(const QString& permissionId = {});
    void showScreenshotHistory();
    void showPinToScreenManagement();
    void showTranslation(const QString& text);

  signals:
    void screenshotRequested();
    void quickActionRequested(clip_shot::presentation::GlobalShortcutAction action);
    void
    globalMouseDragRequested(clip_shot::presentation::settings::SettingsGlobalMouseAction action);
    void screenshotHistoryEditRequested(const QString& recordId);
    void screenshotHistoryPinRequested(const QString& recordId);

  protected:
    bool event(QEvent* event) override;
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
#endif

  private:
    void buildUi();
    void applyTheme(const clip_shot::presentation::styles::ThemeColorScheme& scheme);
    void syncSkinBackground();
    void applySkinAppearance();
    void syncTitleBarBottomShadowGeometry();
    void setupDwmShadow();
#ifdef Q_OS_MACOS
    void setupNativeTitleBar();
#endif
    ClipShotApiClient* m_translationClient = nullptr;
    TitleBarWidget* m_titleBar = nullptr;
    SidebarWidget* m_sidebar = nullptr;
    MainContentHeaderWidget* m_contentHeader = nullptr;
    ContentCardWidget* m_contentCard = nullptr;
    const clip_shot::presentation::settings::SettingsRegistry& m_settingsRegistry;
    clip_shot::presentation::settings::SettingsRuntimeSession& m_runtimeSession;
    QWidget* m_titleBarBottomShadow = nullptr;
    clip_shot::presentation::MainWindowSkinWidget* m_skinBackground = nullptr;
    QWidget* m_body = nullptr;
    QWidget* m_contentShell = nullptr;
    QWidget* m_contentArea = nullptr;
    // Member (not a QObject child) so the destructor still runs while QWidget is
    // alive and can persist visible-space geometry on shutdown delete.
    clip_shot::presentation::WindowGeometryMemory m_geometryMemory;
    bool m_isApplyingTheme = false;
};

#endif // CLIP_SHOT_PRESENTATION_MAINWINDOW_H
