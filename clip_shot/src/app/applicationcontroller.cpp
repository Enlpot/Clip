#include "clip_draw_engine_qt/clip_canvas_image.h"
#include "clip_shot/app/edition.h"
#include "clip_shot/app/applicationcontroller.h"
#include "clip_shot/app/applicationrestart.h"
#include "clip_shot/app/updateconfirmationdialog.h"
#include "clip_shot/app/featureavailability.h"
#include "clip_shot/app/applicationinputguard.h"
#include "clip_shot/presentation/apppermissionservice.h"
#ifdef Q_OS_MACOS
#include "clip_shot/platform/macos/applicationactivation.h"
#include "clip_shot/presentation/permissionguidecontroller.h"
#endif
#include "clip_shot/platform/windows/administratorlaunch.h"
#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION
#include "clip_shot/translation/translationservice.h"
#endif
#include "clip_shot/presentation/languagemanager.h"
#include "clip_shot/presentation/styles/thememanager.h"
#include "theme/theme_manager.h"
#include "clip_shot/update/updateservice.h"
#include "clip_shot/update/startupupdate.h"
#include "clip_shot/presentation/screenshotexportcoordinator.h"
#include "clip_shot/presentation/screenshotexportartifact.h"

#include "clip_shot/presentation/globalshortcutmanager.h"
#include "clip_shot/presentation/globalcanvascontroller.h"
#include "clip_shot/presentation/globalmousemanager.h"
#include "clip_shot/presentation/mainwindow.h"
#include "clip_shot/presentation/pinnedwindowgroupmanager.h"
#include "clip_shot/presentation/windowgroupswitchercontroller.h"
#include "clip_shot/presentation/screenshotcontroller.h"
#include "clip_shot/presentation/screenshotautofiltercontroller.h"
#include "clip_shot/presentation/directcapturecontroller.h"
#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION
#include "clip_shot/presentation/selectedtexttranslationcoordinator.h"
#endif
#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION
#include "clip_shot/presentation/selectedtexttranslationcontroller.h"
#endif
#include "clip_shot/presentation/screenshotocrrecognitionservice.h"
#include "clip_shot/presentation/screenshotpinnedwindow.h"
#include "clip_shot/presentation/screenrecordingfolder.h"
#include "clip_shot/presentation/systemtraycontroller.h"
#include "clip_shot/presentation/systemnotificationfeedback.h"
#include "clip_shot/presentation/floatingtoolbarcontroller.h"
#include "clip_shot/presentation/screenshotqrrecognitionservice.h"
#include "clip_shot/presentation/screenshotclipboardcontent.h"
#include "clip_shot/presentation/screenshothistorytypes.h"
#include "clip_shot/storage/capturehistoryrepository.h"
#include "clip_shot/storage/pinnedwindowrepository.h"
#include "clip_shot/presentation/settings/settingsbackend.h"
#include "clip_shot/presentation/settings/settingsregistry.h"
#include "clip_shot/presentation/settings/settingsruntimesession.h"
#include "clip_shot/platform/selectedfiles.h"
#include "clip_shot/platform/screenshotnative.h"
#include "clip_shot/storage/applicationstorage.h"
#include "clip_shot/storage/settingsadapters.h"
#include "widgets/message.h"

#include <QApplication>
#include <QEvent>
#include <QClipboard>
#include <QFutureWatcher>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonValue>
#include <QPointer>
#include <QScreen>
#include <QTimer>
#include <QtMath>
#include <QUuid>

#include <memory>
#include <atomic>
#include <cmath>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace clip_shot::app {
namespace {
const QString kPinBorderColorKey = QStringLiteral("pin_to_screen/border_color");
const QString kPinBorderActiveColorKey = QStringLiteral("pin_to_screen/border_active_color");
const QString kPinLockedBorderColorKey = QStringLiteral("pin_to_screen/locked_border_color");
const QString kTrayEnabledKey = QStringLiteral("tray/enabled");
const QString kTrayIconKey = QStringLiteral("tray/icon");
const QString kTrayCustomIconKey = QStringLiteral("tray/custom_icon");
const QString kTrayLeftClickActionKey = QStringLiteral("tray/left_click_action");
const QString kTrayMiddleClickActionKey = QStringLiteral("tray/middle_click_action");
const QString kTrayMenuOptionsKey = QStringLiteral("tray/menu_options");
const QString kScreenshotDelaySecondsKey = QStringLiteral("screenshot/delay_seconds");
const QString kFullscreenSuppressionKey =
    QStringLiteral("global_shortcuts/disable_on_focused_fullscreen_window");
const QString kOcrModelTypeKey = QStringLiteral("text_recognition/model_type");
const QString kOcrDetectorResizePolicyKey =
    QStringLiteral("text_recognition/detector_resize_policy");
const QString kOcrTextDetectionProcessingKey =
    QStringLiteral("text_recognition/text_detection_processing");
const QString kOcrDirectMlKey = QStringLiteral("text_recognition/direct_ml_acceleration");

QStringList stringList(const QJsonValue& value) {
    QStringList result;
    for (const QJsonValue& item : value.toArray()) {
        result.push_back(item.toString());
    }
    return result;
}

storage::PinnedWindowRepository* initializedPinnedWindowRepository() {
    auto& applicationStorage = storage::ApplicationStorage::instance();
    if (!applicationStorage.isInitialized()) {
        static_cast<void>(applicationStorage.initialize());
    }
    return applicationStorage.isInitialized() ? &applicationStorage.pinnedWindows() : nullptr;
}
} // namespace

class ApplicationController::Impl {
  public:
    enum class QuickActionOrigin { Other, GlobalHotkey };

    Impl(ApplicationController& owner, QApplication& application)
        : q(owner), app(application), restartCoordinator(application),
          groupManager(initializedPinnedWindowRepository()),
          systemTray(presentation::settings::builtInTrayCommandManifest(), &groupManager),
          featureRouter([this](FeatureFamily feature) { showUnavailableFeature(feature); }) {
        QObject::connect(&systemTray,
                         &presentation::SystemTrayController::notificationDeliveryFinished, &q,
                         [this](const platform::SystemNotificationRequest& request,
                                const platform::SystemNotificationResult& result) {
                             presentation::presentSystemNotificationResult(
                                 request, result, [this]() -> QWidget* {
                                     MainWindow& window = ensureMainWindow();
                                     window.showAndActivate();
                                     return &window;
                                 });
                         });
        QObject::connect(&floatingToolbar,
                         &presentation::FloatingToolbarController::actionRequested, &q,
                         [this](const QString& action) { dispatchFloatingAction(action); });
        QObject::connect(&floatingToolbar,
                         &presentation::FloatingToolbarController::customizeRequested, &q, [this] {
                             ensureMainWindow().showSettingsLocation(
                                 QStringLiteral("desktop-tools"),
                                 QStringLiteral("floating-toolbar"));
                         });
        QObject::connect(
            &floatingToolbar, &presentation::FloatingToolbarController::contentDropped, &q,
            [this](ScreenshotClipboardContentSnapshot snapshot, QStringList paths) {
                if (storage::ApplicationStorage::instance().directoryChanging())
                    return;
                static_cast<void>(featureRouter.dispatch(
                    FeatureFamily::PinToScreen,
                    [this, snapshot = std::move(snapshot), paths = std::move(paths)]() mutable {
                        if (auto* controller = ensureScreenshotController())
                            controller->pinDroppedContent(std::move(snapshot), std::move(paths));
                    }));
            });
        QObject::connect(&app, &QCoreApplication::aboutToQuit, &floatingToolbar,
                         &presentation::FloatingToolbarController::shutdown);
        QObject::connect(
            &systemTray, &presentation::SystemTrayController::screenshotRequested, &q, [this]() {
                if (storage::ApplicationStorage::instance().directoryChanging())
                    return;
                if (!allowPermissions(presentation::requiredPermissions(
                        presentation::GlobalShortcutAction::Screenshot,
                        permissions.microphoneEnabled())))
                    return;
                static_cast<void>(featureRouter.dispatch(FeatureFamily::Screenshot, [this]() {
                    if (ScreenshotController* controller = ensureScreenshotController()) {
                        controller->startCapture();
                    }
                }));
            });
        QObject::connect(&systemTray, &presentation::SystemTrayController::showMainWindowRequested,
                         &q, [this]() { showMainWindow(); });
        QObject::connect(
            &systemTray, &presentation::SystemTrayController::restartRequested, &q, [this]() {
                const auto result = restartCoordinator.restart(
                    [this]() { return restartAllowed(); },
                    []() { return storage::ApplicationStorage::instance().flushNow().success; });
                if (!result.success) {
                    systemTray.showWarningMessage(ApplicationController::tr("Restart failed"),
                                                  result.error);
                }
            });
        QObject::connect(&systemTray,
                         &presentation::SystemTrayController::openFunctionSettingsRequested, &q,
                         [this]() { ensureMainWindow().showScreenshotSettings(); });
        QObject::connect(&systemTray, &presentation::SystemTrayController::openAboutRequested, &q,
                         [this]() { ensureMainWindow().showAbout(); });
        QObject::connect(
            &systemTray, &presentation::SystemTrayController::openRecordingFileRequested, &q,
            [](const QString& path) {
                static_cast<void>(presentation::recording::revealScreenRecordingFile(path));
            });
        QObject::connect(&systemTray, &presentation::SystemTrayController::exitRequested, &q,
                         [this]() {
                             if (storage::ApplicationStorage::instance().directoryChanging())
                                 return;
                             systemTray.hide();
                             QApplication::quit();
                         });
        QObject::connect(
            &systemTray, &presentation::SystemTrayController::quickActionRequested, &q,
            [this](presentation::GlobalShortcutAction action) { dispatchQuickAction(action); });
        auto& pinnedStorage = storage::ApplicationStorage::instance();
        QObject::connect(&pinnedStorage, &storage::ApplicationStorage::pinnedWindowShowRequested,
                         &q, [this](const QString& id) {
                             if (auto* controller = ensureScreenshotController())
                                 controller->showPinnedRecord(id);
                         });
        QObject::connect(&pinnedStorage, &storage::ApplicationStorage::pinnedWindowDeleteRequested,
                         &q, [this](const QVector<QString>& ids) {
                             if (auto* controller = ensureScreenshotController())
                                 controller->destroyPinnedRecords(ids);
                         });
        QObject::connect(&pinnedStorage, &storage::ApplicationStorage::pinnedWindowsChanged,
                         &groupManager,
                         &presentation::PinnedWindowGroupManager::onPinnedRecordsChanged);
        QObject::connect(
            &groupManager,
            &presentation::PinnedWindowGroupManager::restoreActiveGroupWindowsRequested, &q,
            [this]() {
                static_cast<void>(featureRouter.dispatch(
                    FeatureFamily::PinToScreen,
                    [this]() {
                        if (ScreenshotController* controller = ensureScreenshotController()) {
                            controller->restoreActivePinnedGroupWindows();
                        }
                    },
                    started));
            });
        QObject::connect(&globalShortcutManager, &presentation::GlobalShortcutManager::activated,
                         &q, [this](presentation::GlobalShortcutAction action) {
                             if (action != presentation::GlobalShortcutAction::SwitchWindowGroup)
                                 dispatchQuickAction(action, QuickActionOrigin::GlobalHotkey);
                         });
        QObject::connect(&globalShortcutManager,
                         &presentation::GlobalShortcutManager::bindingActivated, &q,
                         [this](presentation::GlobalShortcutAction action, int registrationId) {
                             if (action == presentation::GlobalShortcutAction::SwitchWindowGroup)
                                 ensureWindowGroupSwitcher().activateShortcut(registrationId);
                         });
        QObject::connect(&globalShortcutManager, &presentation::GlobalShortcutManager::stateChanged,
                         &q,
                         [this](presentation::GlobalShortcutAction action,
                                const presentation::GlobalShortcutRegistrationState& state) {
                             systemTray.setGlobalShortcuts(action, state.shortcuts);
                         });
        QObject::connect(&globalShortcutManager,
                         &presentation::GlobalShortcutManager::globalHotkeysEnabledChanged, &q,
                         [this](bool enabled) {
                             systemTray.setQuickActionChecked(
                                 presentation::GlobalShortcutAction::ToggleGlobalHotkeys, !enabled);
                         });
        QObject::connect(&app, &QCoreApplication::aboutToQuit, &systemTray,
                         &presentation::SystemTrayController::hide);
        QObject::connect(&app, &QCoreApplication::aboutToQuit, &globalMouseManager,
                         &presentation::GlobalMouseManager::shutdown);
        QObject::connect(
            &globalMouseManager, &presentation::GlobalMouseManager::operationFailed, &q,
            [this](const QString& message) {
#ifdef Q_OS_MACOS
                const auto state = globalMouseManager.permissionState().status;
                if (state == presentation::GlobalMousePermissionState::Status::ListenRequired ||
                    state ==
                        presentation::GlobalMousePermissionState::Status::AccessibilityRequired)
                    return;
#endif
                systemTray.showCaptureMessage(message, true);
            });
        QObject::connect(
            &globalMouseManager, &presentation::GlobalMouseManager::dragEvent, &q,
            [this](const presentation::GlobalMouseDragEvent& event) {
                using Kind = presentation::GlobalMouseDragEvent::Kind;
                const FeatureFamily feature = featureFamilyFor(event.action);
                if (event.kind == Kind::Begin) {
                    if (!allowPermissions(presentation::requiredPermissions(
                            event.action, permissions.microphoneEnabled()))) {
                        globalMouseManager.cancelGesture(event.id);
                        return;
                    }
                    static_cast<void>(featureRouter.beginGesture(
                        feature, [this, id = event.id]() { globalMouseManager.cancelGesture(id); },
                        [this, event]() { dispatchGlobalMouseEvent(event); }));
                    return;
                }
                static_cast<void>(featureRouter.dispatch(
                    feature, [this, event]() { dispatchGlobalMouseEvent(event); }, false));
            });
        auto& applicationStorage = storage::ApplicationStorage::instance();
        if (!applicationStorage.isInitialized()) {
            static_cast<void>(applicationStorage.initialize());
        }
#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION
        translationClient =
            std::make_unique<ClipShotApiClient>(ClipShotApiClient::configuredBaseUrl(
                applicationStorage.configuration()
                    .value(QStringLiteral("api_configuration/server_url"))
                    .toString()));
        translationService = &translation::TranslationService::forClient(
            *translationClient, applicationStorage.configuration(),
            presentation::LanguageManager::instance().currentLocale());
        QObject::connect(&presentation::LanguageManager::instance(),
                         &presentation::LanguageManager::languageChanged, translationService,
                         [this](const QString&, const QLocale& locale) {
                             translationService->setLocale(locale);
                         });
#endif
        // OCR process ownership is application-scoped. ScreenshotController
        // instances receive a consumer of this service instead of creating a
        // second child process for each controller.
        ScreenshotOcrRecognitionService::Options ocrOptions;
        ocrOptions.offlineRoot =
            QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("assets/ocr"));
        if (applicationStorage.isInitialized() &&
            !applicationStorage.configurationDirectory().trimmed().isEmpty()) {
            ocrOptions.cacheRoot = QDir(applicationStorage.configurationDirectory())
                                       .filePath(QStringLiteral("assets/ocr"));
        }
        ocrOptions.modelType = screenshotOcrModelTypeFromValue(
            applicationStorage.configuration()
                .value(QStringLiteral("text_recognition/model_type"))
                .toString());
        ocrOptions.detectorResizePolicy = screenshotOcrDetectorResizePolicyFromValue(
            applicationStorage.configuration().value(kOcrDetectorResizePolicyKey).toString());
        ocrOptions.textDetectionProcessing = screenshotOcrTextDetectionProcessingFromValue(
            applicationStorage.configuration().value(kOcrTextDetectionProcessingKey).toString());
        const auto backendPreference =
            applicationStorage.configuration()
                    .value(QStringLiteral("text_recognition/direct_ml_acceleration"))
                    .toBool()
                ? ScreenshotOcrBackendPreference::DirectMl
                : ScreenshotOcrBackendPreference::Cpu;
        ocrRecognition =
            std::make_unique<ScreenshotOcrRecognitionService>(ocrOptions, backendPreference, &q);
        auto& configuration = applicationStorage.configuration();
        updates = new update::UpdateService(update::defaultUpdateServiceOptions(), &app);
        updates->setProgressAppearance(
            presentation::styles::ThemeManager::instance().updateProgressAppearance());
        QObject::connect(
            &adqt::theme::ThemeManager::instance(), &adqt::theme::ThemeManager::themeChanged,
            updates, [updateService = updates] {
                updateService->setProgressAppearance(
                    presentation::styles::ThemeManager::instance().updateProgressAppearance());
            });
        updates->setMode(configuration.value(QStringLiteral("updates/mode")).toString());
        updates->setSystemProxy(configuration.value(QStringLiteral("network/proxy")).toString() ==
                                u"system");
        QObject::connect(&presentation::LanguageManager::instance(),
                         &presentation::LanguageManager::languageChanged, updates,
                         [updateService = updates] {
                             QEvent event(QEvent::LanguageChange);
                             QCoreApplication::sendEvent(updateService, &event);
                         });
        QObject::connect(
            updates, &update::UpdateService::automaticUpdateAvailable, &q,
            [this](const QString& version) {
                systemTray.showUpdateMessage(
                    (edition::isMini
                         ? ApplicationController::tr(
                               "%1 %2 is available. Open About for update options.")
                               .arg(edition::productName(), version)
                         : ApplicationController::tr(
                               "Clip %1 is available. Open About for update options.")
                               .arg(version)));
            });
#ifndef Q_OS_MACOS
        QObject::connect(updates, &update::UpdateService::updateReady, &q, [this] {
            systemTray.showUpdateMessage(
                (edition::isMini
                     ? ApplicationController::tr(
                           "An update is ready. Open About to restart and update %1.")
                           .arg(edition::productName())
                     : ApplicationController::tr(
                           "An update is ready. Open About to restart and update Clip.")));
        });
#endif
        platform::windows::setAdministratorRestartGuard([this] { return restartAllowed(); });
#ifdef Q_OS_WIN
        migrationInput = new ApplicationInputGuard(&q);
        app.installEventFilter(migrationInput);
        applicationStorage.setDirectoryChangeHooks(
            [this] {
                if (!restartAllowed() || (ocrRecognition && ocrRecognition->storageBusy()) ||
                    ScreenshotExportCoordinator::shared().pendingJobCount() != 0 ||
                    diagnostics::DiagnosticsService::instance().status().exporting)
                    return storage::StorageResult::failure(ApplicationController::tr(
                        "Finish capturing, recording, exporting, recognizing text, or updating "
                        "before changing storage."));
                migrationSource = storage::ApplicationStorage::instance().configurationDirectory();
                migrationInput->active = true;
                systemTray.setEnabled(false);
                for (auto* window : QApplication::topLevelWidgets()) {
                    if (auto* pin = qobject_cast<ScreenshotPinnedWindow*>(window))
                        pin->suspendStorageWrites();
                }
                migrationShortcuts = globalShortcutManager.suspendRegistrations();
                globalMouseManager.setCaptureAvailable(false);
                if (ocrRecognition)
                    ocrRecognition->suspendStorage();
                return storage::StorageResult::ok();
            },
            [this](const QString& root) {
                if (ocrRecognition)
                    ocrRecognition->resumeStorage(
                        QDir(root).filePath(QStringLiteral("assets/ocr")));
                for (auto* window : QApplication::topLevelWidgets()) {
                    if (auto* pin = qobject_cast<ScreenshotPinnedWindow*>(window))
                        pin->resumeStorageWrites(migrationSource, root);
                }
                migrationInput->active = false;
                systemTray.setEnabled(storage::ApplicationStorage::instance()
                                          .configuration()
                                          .value(kTrayEnabledKey)
                                          .toBool());
                globalShortcutManager.resumeRegistrations(migrationShortcuts);
                globalMouseManager.setCaptureAvailable(true);
            },
            [this] {
                if (ocrRecognition)
                    ocrRecognition->drainStorage();
            });
#endif
        QObject::connect(updates, &update::UpdateService::restartRequested, &q, [this] {
            if (platform::windows::administratorOperationPending())
                return;
            if ((screenshotController != nullptr &&
                 screenshotController->blocksApplicationUpdate()) ||
                (directCaptureController != nullptr &&
                 directCaptureController->blocksApplicationUpdate())) {
                updates->reportBlocked(ApplicationController::tr(
                    "Finish capturing, recording, or exporting before updating."));
                return;
            }
            if (!confirmRestartAndUpdate(mainWindow)) {
                return;
            }
            if (!storage::ApplicationStorage::instance().flushNow().success) {
                updates->reportBlocked(ApplicationController::tr(
                    "Your settings could not be saved. Please retry before updating."));
                return;
            }
            updates->beginApply();
        });
        handoffInput = new ApplicationInputGuard(&q);
        app.installEventFilter(handoffInput);
        QObject::connect(updates, &update::UpdateService::handoffPendingChanged, &q,
                         [this](bool pending) { setUpdateHandoffPending(pending); });
        QObject::connect(updates, &update::UpdateService::handoffReady, &q, [this] {
            if ((screenshotController != nullptr &&
                 screenshotController->blocksApplicationUpdate()) ||
                (directCaptureController != nullptr &&
                 directCaptureController->blocksApplicationUpdate()) ||
                !storage::ApplicationStorage::instance().flushNow().success) {
                updates->reportBlocked(ApplicationController::tr(
                    "Finish capturing, recording, or exporting before updating."));
                return;
            }
        });
        QObject::connect(updates, &update::UpdateService::handoffCommitted, &q, [this] {
            handoffInput->active = false;
            globalShortcutManager.setGlobalHotkeysEnabled(false);
            globalMouseManager.shutdown();
            QApplication::quit();
        });
        applyRuntimeConfiguration(configuration.value(kPinBorderColorKey), kPinBorderColorKey);
        applyRuntimeConfiguration(configuration.value(kPinBorderActiveColorKey),
                                  kPinBorderActiveColorKey);
        applyRuntimeConfiguration(configuration.value(kPinLockedBorderColorKey),
                                  kPinLockedBorderColorKey);
        applyRuntimeConfiguration(configuration.value(kTrayEnabledKey), kTrayEnabledKey);
        applyRuntimeConfiguration(configuration.value(kTrayIconKey), kTrayIconKey);
        applyRuntimeConfiguration(configuration.value(kTrayCustomIconKey), kTrayCustomIconKey);
        applyRuntimeConfiguration(configuration.value(kTrayLeftClickActionKey),
                                  kTrayLeftClickActionKey);
        applyRuntimeConfiguration(configuration.value(kTrayMiddleClickActionKey),
                                  kTrayMiddleClickActionKey);
        applyRuntimeConfiguration(configuration.value(kTrayMenuOptionsKey), kTrayMenuOptionsKey);
        applyRuntimeConfiguration(configuration.value(kScreenshotDelaySecondsKey),
                                  kScreenshotDelaySecondsKey);
        applyRuntimeConfiguration(configuration.value(kFullscreenSuppressionKey),
                                  kFullscreenSuppressionKey);
        QObject::connect(&configuration, &storage::ConfigurationStore::valueChanged, &q,
                         [this](const QString& key, const QJsonValue& value) {
                             applyRuntimeConfiguration(value, key);
                         });
#ifdef Q_OS_MACOS
        reopenHandler = std::make_unique<platform::macos::ApplicationReopenHandler>(
            [this]() { showMainWindow(); });
#endif
    }

    ~Impl() {
        floatingToolbar.shutdown();
        platform::windows::setAdministratorRestartGuard({});
        storage::ApplicationStorage::instance().setDirectoryChangeHooks({}, {});
        if (mainWindow != nullptr) {
            mainWindow->setAttribute(Qt::WA_DeleteOnClose, false);
            delete mainWindow;
        }
    }

    [[nodiscard]] bool restartAllowed() const {
        return !storage::ApplicationStorage::instance().directoryChanging() && updates != nullptr &&
               updates->status().state != update::UpdateState::Applying &&
               !(screenshotController && screenshotController->blocksApplicationUpdate()) &&
               !(directCaptureController && directCaptureController->blocksApplicationUpdate());
    }

    void setUpdateHandoffPending(bool pending) {
        handoffInput->active = pending;
        featureRouter.setSuspended(pending);
        if (screenshotController)
            screenshotController->setCaptureSuspended(pending);
        if (pending) {
            handoffShortcuts = globalShortcutManager.suspendRegistrations();
            globalMouseManager.setCaptureAvailable(false);
            systemTray.setEnabled(false);
        } else {
            globalShortcutManager.resumeRegistrations(handoffShortcuts);
            handoffShortcuts = 0;
            globalMouseManager.setCaptureAvailable(!screenshotController ||
                                                   screenshotController->captureAvailable());
            systemTray.setEnabled(storage::ApplicationStorage::instance()
                                      .configuration()
                                      .value(kTrayEnabledKey)
                                      .toBool());
        }
    }

    void applyOcrConfiguration() {
        if (ocrRecognition == nullptr)
            return;
        const auto& configuration = storage::ApplicationStorage::instance().configuration();
        ocrRecognition->setRuntimeConfiguration(
            {screenshotOcrModelTypeFromValue(configuration.value(kOcrModelTypeKey).toString()),
             configuration.value(kOcrDirectMlKey).toBool()
                 ? ScreenshotOcrBackendPreference::DirectMl
                 : ScreenshotOcrBackendPreference::Cpu,
             screenshotOcrDetectorResizePolicyFromValue(
                 configuration.value(kOcrDetectorResizePolicyKey).toString()),
             configuration.value(QStringLiteral("text_recognition/resident_process")).toBool(),
             configuration.value(QStringLiteral("text_recognition/model_hot_start")).toBool(),
             screenshotOcrTextDetectionProcessingFromValue(
                 configuration.value(kOcrTextDetectionProcessingKey).toString())});
    }

    void start() {
        if (started) {
            return;
        }
        started = true;
        updates->start();

#ifdef Q_OS_MACOS
        permissions.setMicrophoneEnabled(storage::RecordingSettings().microphoneEnabled());
        globalMouseManager.usePermissionSnapshot(false, false);
        QObject::connect(&permissions, &presentation::AppPermissionService::refreshed, &q, [this] {
            const auto& snapshot = permissions.snapshot();
            globalMouseManager.usePermissionSnapshot(
                snapshot.granted(presentation::AppPermission::InputMonitoring),
                snapshot.granted(presentation::AppPermission::Accessibility));
            const auto missing = permissions.takeStartupMissing();
            if (!missing.isEmpty())
                ensureMainWindow().showAppPermissions(
                    presentation::appPermissionId(missing.first()));
        });
        QObject::connect(&globalMouseManager,
                         &presentation::GlobalMouseManager::permissionRefreshRequested,
                         &permissions, &presentation::AppPermissionService::refresh);
        permissions.refresh();
#endif
        systemTray.show();
        globalShortcutManager.initialize();
        globalMouseManager.setCaptureAvailable(ensureScreenshotController()->captureAvailable());
        globalMouseManager.initialize();
        QObject::connect(&app, &QGuiApplication::applicationStateChanged, &globalMouseManager,
                         [this](Qt::ApplicationState state) {
                             if (state == Qt::ApplicationActive) {
#ifdef Q_OS_MACOS
                                 permissions.refresh();
#else
                                 globalMouseManager.refreshPermission();
#endif
                             }
                         });
        static_cast<void>(featureRouter.dispatch(
            FeatureFamily::Screenshot,
            [this]() {
                QTimer::singleShot(0, &q, [this]() {
                    if (ScreenshotController* controller = ensureScreenshotController()) {
                        controller->prewarmResources();
                    }
                });
            },
            false));
        static_cast<void>(featureRouter.dispatch(
            FeatureFamily::PinToScreen,
            [this]() { QTimer::singleShot(0, &q, [this]() { restorePinnedWindows(); }); }, false));
        QTimer::singleShot(0, &q, [this]() { applyOcrConfiguration(); });
    }

    ScreenshotController* ensureScreenshotController() {
        if (screenshotController == nullptr) {
            screenshotController = std::make_unique<ScreenshotController>(
                &q, &groupManager, ocrRecognition.get(), apiClient());
            screenshotController->setCaptureSuspended(updates && updates->handoffPending());
            QObject::connect(screenshotController.get(),
                             &ScreenshotController::recordingExportNotificationRequested,
                             &systemTray,
                             &presentation::SystemTrayController::showRecordingExportMessage);
            QObject::connect(screenshotController.get(),
                             &ScreenshotController::captureActivityChanged, &floatingToolbar,
                             &presentation::FloatingToolbarController::setCaptureActive);
#ifdef Q_OS_MACOS
            screenshotController->setRecordingPermissionCheck([this](bool microphone, bool input,
                                                                     bool notify) {
                presentation::AppPermissions required{presentation::AppPermission::ScreenRecording};
                if (microphone)
                    required.append(presentation::AppPermission::Microphone);
                if (input)
                    required.append(presentation::AppPermission::InputMonitoring);
                return notify ? allowPermissions(required) : permissions.allow(required);
            });
#else
            screenshotController->setRecordingPermissionCheck(
                [](bool, bool, bool) { return true; });
#endif
            QObject::connect(
                screenshotController.get(), &ScreenshotController::accessibilityPermissionRequested,
                &q, [this] {
                    permissions.refresh();
                    ensureMainWindow().showAppPermissions(
                        presentation::appPermissionId(presentation::AppPermission::Accessibility));
                });
            QObject::connect(
                screenshotController.get(), &ScreenshotController::selectedFilePinFailed, &q,
                [this](const QString& message) {
                    if (!mainWindow || !mainWindow->isVisible() || mainWindow->isMinimized()) {
                        systemTray.showWarningMessage(
                            ApplicationController::tr("Could not pin selected files"), message);
                        return;
                    }
                    MainWindow& window = ensureMainWindow();
                    window.showAndActivate();
                    adqt::widgets::AdMessage::Request request;
                    request.key = QStringLiteral("pin-selected-files-error");
                    request.content = message;
                    adqt::widgets::AdMessageService::warning(std::move(request), &window);
                });
            QObject::connect(screenshotController.get(),
                             &ScreenshotController::showMainWindowRequested, &q,
                             [this]() { showMainWindow(); });
#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION
            QObject::connect(screenshotController.get(),
                             &ScreenshotController::translationPageRequested, &q,
                             [this](const QString& text) {
                                 ensureSelectedTextTranslationCoordinator().presentText(text);
                             });
#endif
            if (isFeatureAvailable(FeatureFamily::Screenshot) ||
                isFeatureAvailable(FeatureFamily::PinToScreen) ||
                isFeatureAvailable(FeatureFamily::ScreenRecording)) {
                QObject::connect(
                    screenshotController.get(), &ScreenshotController::captureAvailabilityChanged,
                    &globalMouseManager, &presentation::GlobalMouseManager::setCaptureAvailable);
            }
            QObject::connect(screenshotController.get(),
                             &ScreenshotController::globalMouseCaptureEnded, &globalMouseManager,
                             &presentation::GlobalMouseManager::cancelGesture);
        }
        return screenshotController.get();
    }

    void applyRuntimeConfiguration(const QJsonValue& value, const QString& key) {
        if (key == QStringLiteral("screen_recording/enable_microphone"))
            permissions.setMicrophoneEnabled(value.toBool());
#if CLIP_SHOT_ENABLE_EXTENDED_FEATURES
        if (key == QStringLiteral("extended_features/translation_page_enabled")) {
            systemTray.setMenuOptions(
                stringList(storage::ApplicationStorage::instance().configuration().value(
                    kTrayMenuOptionsKey)));
        }
#endif
        if (key == u"updates/mode" && updates != nullptr) {
            updates->setMode(value.toString());
        } else if (key == u"network/proxy" && updates != nullptr) {
            updates->setSystemProxy(value.toString() == u"system");
        } else if (key == kPinBorderColorKey) {
            QColor color = storage::colorFromRgbaString(value.toString());
            if (!color.isValid()) {
                color = QColor(219, 219, 219, 255);
            }
            ScreenshotPinnedWindow::setRuntimeBorderColor(color);
        } else if (key == kPinLockedBorderColorKey) {
            ScreenshotPinnedWindow::setRuntimeLockedBorderColor(
                storage::colorFromRgbaString(value.toString()));
        } else if (key == kPinBorderActiveColorKey) {
            QColor color = storage::colorFromRgbaString(value.toString());
            if (!color.isValid()) {
                color = QColor(105, 177, 255, 255);
            }
            ScreenshotPinnedWindow::setRuntimeBorderActiveColor(color);
        } else if (key == kTrayEnabledKey) {
            const bool enabled = value.isBool() ? value.toBool() : true;
            systemTray.setEnabled(enabled);
            ScreenshotPinnedWindow::setRuntimeTrayEnabled(enabled);
        } else if (key == kTrayIconKey) {
            systemTray.setIconSelection(value.toString(QStringLiteral("default")));
        } else if (key == kTrayCustomIconKey) {
            systemTray.setCustomIconPath(value.toString());
        } else if (key == kTrayLeftClickActionKey) {
            systemTray.setLeftClickAction(value.toString(QStringLiteral("screenshot")));
        } else if (key == kTrayMiddleClickActionKey) {
            systemTray.setMiddleClickAction(value.toString(QStringLiteral("screenshot_fixed")));
        } else if (key == kTrayMenuOptionsKey) {
            systemTray.setMenuOptions(stringList(value));
        } else if (key == kScreenshotDelaySecondsKey) {
            systemTray.setScreenshotDelaySeconds(value.toInt(3));
        } else if (key == kFullscreenSuppressionKey) {
            systemTray.setQuickActionChecked(
                presentation::GlobalShortcutAction::ToggleDisableOnFocusedFullscreenWindow,
                value.toBool());
        } else if (key == kOcrModelTypeKey || key == kOcrDirectMlKey ||
                   key == kOcrDetectorResizePolicyKey || key == kOcrTextDetectionProcessingKey ||
                   key == QStringLiteral("text_recognition/resident_process") ||
                   key == QStringLiteral("text_recognition/model_hot_start")) {
            if (started)
                applyOcrConfiguration();
        }
    }

    MainWindow& ensureMainWindow() {
        if (mainWindow == nullptr) {
            ensureSettingsRuntime();
            mainWindow = new MainWindow(*settingsRegistry, *runtimeSession, nullptr, apiClient());
            QObject::connect(mainWindow, &QObject::destroyed, &q,
                             [this]() { mainWindow = nullptr; });
            QObject::connect(mainWindow, &MainWindow::screenshotRequested, &q, [this]() {
                if (!allowPermissions(presentation::requiredPermissions(
                        presentation::GlobalShortcutAction::Screenshot,
                        permissions.microphoneEnabled())))
                    return;
                static_cast<void>(featureRouter.dispatch(FeatureFamily::Screenshot, [this]() {
                    if (ScreenshotController* controller = ensureScreenshotController()) {
                        controller->startCapture();
                    }
                }));
            });
            QObject::connect(
                mainWindow, &MainWindow::quickActionRequested, &q,
                [this](presentation::GlobalShortcutAction action) { dispatchQuickAction(action); });
            QObject::connect(mainWindow, &MainWindow::globalMouseDragRequested, &q,
                             [this](presentation::settings::SettingsGlobalMouseAction action) {
                                 if (!allowPermissions(presentation::requiredPermissions(
                                         action, permissions.microphoneEnabled())))
                                     return;
                                 static_cast<void>(featureRouter.dispatch(
                                     featureFamilyFor(action), [this, action]() {
                                         globalMouseManager.beginButtonDrag(action);
                                     }));
                             });
            QObject::connect(
                mainWindow, &MainWindow::screenshotHistoryEditRequested, &q,
                [this](const QString& recordId) {
                    static_cast<void>(
                        featureRouter.dispatch(FeatureFamily::Screenshot, [this, recordId]() {
                            if (ScreenshotController* controller = ensureScreenshotController()) {
                                controller->editHistoryRecord(recordId);
                            }
                        }));
                });
            QObject::connect(
                mainWindow, &MainWindow::screenshotHistoryPinRequested, &q,
                [this](const QString& recordId) {
                    static_cast<void>(
                        featureRouter.dispatch(FeatureFamily::PinToScreen, [this, recordId]() {
                            if (ScreenshotController* controller = ensureScreenshotController()) {
                                controller->pinHistoryRecord(recordId);
                            }
                        }));
                });
        }
        return *mainWindow;
    }

    void ensureSettingsRuntime() {
        if (settingsRegistry == nullptr) {
            settingsRegistry = std::make_unique<presentation::settings::SettingsRegistry>(
                presentation::settings::buildBuiltInSettingsRegistry());
        }
        if (settingsBackend == nullptr) {
            settingsBackend = std::make_unique<presentation::settings::BuiltInSettingsBackend>(
                globalShortcutManager, nullptr, &globalMouseManager, &permissions);
#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION
            settingsBackend->setTranslationService(translationService);
#endif
        }
        if (runtimeSession == nullptr) {
            runtimeSession = std::make_unique<presentation::settings::SettingsRuntimeSession>(
                *settingsRegistry, *settingsBackend);
        }
    }

    bool allowPermissions(const presentation::AppPermissions& requirements) {
#ifdef Q_OS_MACOS
        return permissions.allow(requirements, [this](const presentation::AppPermissions& missing) {
            ensureMainWindow().showAppPermissions(presentation::appPermissionId(missing.first()));
        });
#else
        Q_UNUSED(requirements);
#endif
        return true;
    }

    presentation::WindowGroupSwitcherController& ensureWindowGroupSwitcher() {
        if (!windowGroupSwitcher) {
            windowGroupSwitcher = std::make_unique<presentation::WindowGroupSwitcherController>(
                globalShortcutManager, groupManager);
            QObject::connect(
                windowGroupSwitcher.get(),
                &presentation::WindowGroupSwitcherController::errorOccurred, &q,
                [](const QString& message) { adqt::widgets::AdMessageService::error(message); });
        }
        return *windowGroupSwitcher;
    }

    void dispatchFloatingAction(const QString& id) {
        using Quick = presentation::GlobalShortcutAction;
        static const QHash<QString, Quick> quickActions{
            {QStringLiteral("screenshot"), Quick::Screenshot},
            {QStringLiteral("screenshot-delay"), Quick::ScreenshotDelay},
            {QStringLiteral("pin-to-screen"), Quick::ScreenshotFixed},
            {QStringLiteral("text-recognition"), Quick::ScreenshotOcr},
            {QStringLiteral("text-translation"), Quick::ScreenshotTranslation},
            {QStringLiteral("record-screen"), Quick::ScreenRecord},
        };
        if (const auto action = quickActions.constFind(id); action != quickActions.cend()) {
            dispatchQuickAction(*action);
            return;
        }
        using Action = ScreenshotController::CaptureAction;
        static const QHash<QString, Action> selectionActions{
            {QStringLiteral("scrolling-screenshot"), Action::StartScrolling},
            {QStringLiteral("table-recognition"), Action::RecognizeTable},
            {QStringLiteral("barcode-recognition"), Action::RecognizeQr},
            {QStringLiteral("latex-recognition"), Action::RecognizeFormula},
            {QStringLiteral("convert-to-markdown"), Action::ConvertMarkdown},
            {QStringLiteral("convert-to-html"), Action::ConvertHtml},
            {QStringLiteral("save-as-file"), Action::Save},
        };
        const auto action = selectionActions.constFind(id);
        if (action == selectionActions.cend() ||
            storage::ApplicationStorage::instance().directoryChanging() ||
            !allowPermissions(presentation::requiredPermissions(Quick::Screenshot, false)))
            return;
        static_cast<void>(
            featureRouter.dispatch(FeatureFamily::Screenshot, [this, action = *action] {
                if (auto* controller = ensureScreenshotController())
                    static_cast<void>(controller->captureForAction(action));
            }));
    }

    void dispatchQuickAction(presentation::GlobalShortcutAction action,
                             QuickActionOrigin origin = QuickActionOrigin::Other) {
        if (storage::ApplicationStorage::instance().directoryChanging() ||
            (updates && updates->handoffPending()))
            return;
        if (!allowPermissions(
                presentation::requiredPermissions(action, permissions.microphoneEnabled())))
            return;
        const std::optional<FeatureFamily> feature = featureFamilyFor(action);
        if (feature && !featureRouter.dispatch(*feature, [this, action, origin]() {
                dispatchAvailableQuickAction(action, origin);
            })) {
            return;
        }
        if (!feature) {
            dispatchAvailableQuickAction(action, origin);
        }
    }

    void dispatchAvailableQuickAction(presentation::GlobalShortcutAction action,
                                      QuickActionOrigin origin) {
        if (origin == QuickActionOrigin::GlobalHotkey && screenshotController &&
            screenshotController->handleActiveScreenshotShortcut(action))
            return;
        switch (action) {
        case presentation::GlobalShortcutAction::Screenshot:
            if (ScreenshotController* controller = ensureScreenshotController()) {
                controller->startCapture();
            }
            break;
        case presentation::GlobalShortcutAction::ScreenshotDelay:
            if (ScreenshotController* controller = ensureScreenshotController()) {
                controller->startDelayedCapture(storage::ScreenshotSettings().delaySeconds());
            }
            break;
        case presentation::GlobalShortcutAction::ScreenshotFixed:
            if (ScreenshotController* controller = ensureScreenshotController()) {
                controller->captureAndPinSelection();
            }
            break;
        case presentation::GlobalShortcutAction::ScreenshotOcr:
            if (ScreenshotController* controller = ensureScreenshotController()) {
                controller->captureAndRecognizeText();
            }
            break;
        case presentation::GlobalShortcutAction::ScreenshotTranslation:
#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION
            if (ScreenshotController* controller = ensureScreenshotController()) {
                controller->captureAndTranslateText();
            }
#endif
            break;
        case presentation::GlobalShortcutAction::ScreenshotCopy:
            if (ScreenshotController* controller = ensureScreenshotController()) {
                controller->captureAndCopySelection();
            }
            break;
        case presentation::GlobalShortcutAction::ScreenshotSave:
            if (ScreenshotController* controller = ensureScreenshotController()) {
                static_cast<void>(
                    controller->captureForAction(ScreenshotController::CaptureAction::Save));
            }
            break;
        case presentation::GlobalShortcutAction::ScreenshotQuickSave:
            if (ScreenshotController* controller = ensureScreenshotController()) {
                static_cast<void>(
                    controller->captureForAction(ScreenshotController::CaptureAction::QuickSave));
            }
            break;
        case presentation::GlobalShortcutAction::ScreenshotFullScreen:
            ensureDirectCaptureController().captureCurrentMonitor();
            break;
        case presentation::GlobalShortcutAction::ScreenshotFocusedWindow:
            ensureDirectCaptureController().captureFocusedWindow();
            break;
        case presentation::GlobalShortcutAction::ScreenRecord:
            if (ScreenshotController* controller = ensureScreenshotController()) {
                controller->captureAndStartScreenRecording();
            }
            break;
        case presentation::GlobalShortcutAction::ScreenRecordCopy:
            if (ScreenshotController* controller = ensureScreenshotController()) {
                controller->startOrStopScreenRecordingAndCopy();
            }
            break;
        case presentation::GlobalShortcutAction::OpenScreenRecordingFolder:
            static_cast<void>(presentation::recording::openScreenRecordingFolder());
            break;
        case presentation::GlobalShortcutAction::OpenCaptureHistory:
            ensureMainWindow().showScreenshotHistory();
            break;
        case presentation::GlobalShortcutAction::GlobalCanvas:
            if (!globalCanvasController) {
                globalCanvasController = std::make_unique<presentation::GlobalCanvasController>();
                QObject::connect(globalCanvasController.get(),
                                 &presentation::GlobalCanvasController::activeChanged,
                                 &globalShortcutManager,
                                 &presentation::GlobalShortcutManager::setGlobalCanvasActive);
                QObject::connect(globalCanvasController.get(),
                                 &presentation::GlobalCanvasController::errorOccurred, &q,
                                 [](const QString& message) {
                                     adqt::widgets::AdMessageService::error(message);
                                 });
            }
            globalCanvasController->activate();
            break;
        case presentation::GlobalShortcutAction::SwitchWindowGroup:
            ensureWindowGroupSwitcher().openPicker();
            break;
        case presentation::GlobalShortcutAction::OpenPinToScreenManagement:
            ensureMainWindow().showPinToScreenManagement();
            break;
        case presentation::GlobalShortcutAction::OpenSettings:
            showGeneralSettings();
            break;
        case presentation::GlobalShortcutAction::TranslateSelectedText:
#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION
            if (storage::ExtendedFeaturesSettings().translationPageEnabled()) {
                ensureSelectedTextTranslationCoordinator().capture();
            }
#endif
            break;
        case presentation::GlobalShortcutAction::PinSelectedFiles: {
            const auto target = platform::createSelectedFileBackend()->captureTarget();
            if (ScreenshotController* controller = ensureScreenshotController()) {
                controller->pinSelectedFilesToScreen(target);
            }
            break;
        }
        case presentation::GlobalShortcutAction::RestoreLastClosedWindows:
            if (ScreenshotController* controller = ensureScreenshotController())
                controller->restoreLastClosedPinnedWindow();
            break;
        case presentation::GlobalShortcutAction::PinClipboardContent:
            if (ScreenshotController* controller = ensureScreenshotController()) {
                controller->pinClipboardContentToScreen();
            }
            break;
        case presentation::GlobalShortcutAction::ToggleGlobalHotkeys:
            globalShortcutManager.setGlobalHotkeysEnabled(
                !globalShortcutManager.globalHotkeysEnabled());
            break;
        case presentation::GlobalShortcutAction::ToggleDisableOnFocusedFullscreenWindow: {
            storage::GlobalShortcutSettings shortcutSettings;
            shortcutSettings.setDisableOnFocusedFullscreenWindow(
                !shortcutSettings.disableOnFocusedFullscreenWindow());
            break;
        }
        }
    }

#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION
    presentation::SelectedTextTranslationCoordinator& ensureSelectedTextTranslationCoordinator() {
        if (!selectedTextTranslationCoordinator) {
            selectedTextTranslationCoordinator =
                std::make_unique<presentation::SelectedTextTranslationCoordinator>(
                    storage::ApplicationStorage::instance().configuration(), apiClient());
            QObject::connect(
                selectedTextTranslationCoordinator.get(),
                &presentation::SelectedTextTranslationCoordinator::mainTranslationRequested, &q,
                [this](const QString& text) {
                    if (storage::ExtendedFeaturesSettings().translationPageEnabled()) {
                        ensureMainWindow().showTranslation(text);
                    }
                });
            QObject::connect(&app, &QCoreApplication::aboutToQuit,
                             selectedTextTranslationCoordinator.get(),
                             &presentation::SelectedTextTranslationCoordinator::shutdown);
        }
        return *selectedTextTranslationCoordinator;
    }

#endif
    presentation::DirectCaptureController& ensureDirectCaptureController() {
        if (!directCaptureController) {
            directCaptureController = std::make_unique<presentation::DirectCaptureController>(&q);
            QObject::connect(directCaptureController.get(),
                             &presentation::DirectCaptureController::captureActivityChanged,
                             &floatingToolbar,
                             &presentation::FloatingToolbarController::setCaptureActive);
            QObject::connect(directCaptureController.get(),
                             &presentation::DirectCaptureController::operationFailed, &q,
                             [this](const QString& message, bool warning) {
                                 systemTray.showCaptureMessage(message, warning);
                             });
            QObject::connect(&app, &QCoreApplication::aboutToQuit, directCaptureController.get(),
                             &presentation::DirectCaptureController::shutdown);
        }
        return *directCaptureController;
    }

    void showMainWindow() {
        if (storage::ApplicationStorage::instance().directoryChanging())
            return;
        ensureMainWindow().showAndActivate();
    }

    void restorePinnedWindows() {
        static_cast<void>(featureRouter.dispatch(FeatureFamily::PinToScreen, [this]() {
            if (ScreenshotController* controller = ensureScreenshotController()) {
                controller->restorePinnedWindows();
            }
        }));
    }

    void dispatchGlobalMouseEvent(const presentation::GlobalMouseDragEvent& event) {
        auto* controller = ensureScreenshotController();
        using Kind = presentation::GlobalMouseDragEvent::Kind;
        switch (event.kind) {
        case Kind::Begin:
            if (!controller->beginGlobalMouseCapture(event.action, event.id, event.position,
                                                     event.coordinateSpace)) {
                globalMouseManager.cancelGesture(event.id);
            }
            break;
        case Kind::Update:
            controller->updateGlobalMouseCapture(event.id, event.position);
            break;
        case Kind::Finish:
            controller->finishGlobalMouseCapture(event.id, event.position);
            break;
        case Kind::Cancel:
            controller->cancelGlobalMouseCapture(event.id);
            break;
        }
    }

    QString unavailableFeatureMessage(FeatureFamily feature) const {
        Q_UNUSED(feature);
        return {};
    }

    QString unavailableFeatureKey(FeatureFamily feature) const {
        Q_UNUSED(feature);
        return {};
    }

    void showUnavailableFeatureInWindow(MainWindow& window, FeatureFamily feature) {
        adqt::widgets::AdMessage::Request request;
        request.key = unavailableFeatureKey(feature);
        request.content = unavailableFeatureMessage(feature);
        adqt::widgets::AdMessageService::warning(std::move(request), &window);
    }

    void showUnavailableFeature(FeatureFamily feature) {
        if (mainWindow != nullptr && mainWindow->isVisible() && !mainWindow->isMinimized()) {
            showUnavailableFeatureInWindow(*mainWindow, feature);
            return;
        }
        systemTray.showWarningMessage(ApplicationController::tr("Feature unavailable"),
                                      unavailableFeatureMessage(feature));
    }

    void showGeneralSettings() {
        ensureMainWindow().showGeneralSettings();
    }

    ApplicationController& q;
    QApplication& app;
    ApplicationRestartCoordinator restartCoordinator;
    ApplicationInputGuard* migrationInput = nullptr;
    ApplicationInputGuard* handoffInput = nullptr;
    quint64 handoffShortcuts = 0;
    quint64 migrationShortcuts = 0;
    QString migrationSource;
    // These services outlive the disposable configuration window.
    presentation::PinnedWindowGroupManager groupManager;
    presentation::SystemTrayController systemTray;
    presentation::FloatingToolbarController floatingToolbar;
    FeatureActionRouter featureRouter;
    presentation::GlobalShortcutManager globalShortcutManager;
    std::unique_ptr<presentation::WindowGroupSwitcherController> windowGroupSwitcher;
    presentation::AppPermissionService permissions;
#ifdef Q_OS_MACOS
    presentation::PermissionGuideController permissionGuide{permissions};
#endif
    presentation::GlobalMouseManager globalMouseManager;
    // Settings are intentionally constructed on first window access.  The
    // tray and shortcut manager use only their compact bootstrap data.
    std::unique_ptr<presentation::settings::SettingsRegistry> settingsRegistry;
    std::unique_ptr<presentation::settings::BuiltInSettingsBackend> settingsBackend;
    std::unique_ptr<presentation::settings::SettingsRuntimeSession> runtimeSession;
    ClipShotApiClient* apiClient() const {
#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION
        return translationClient.get();
#else
        return nullptr;
#endif
    }
#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION
    std::unique_ptr<ClipShotApiClient> translationClient;
#endif
#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION
    translation::TranslationService* translationService = nullptr;
#endif
    std::unique_ptr<ScreenshotOcrRecognitionService> ocrRecognition;
    std::unique_ptr<ScreenshotController> screenshotController;
    std::unique_ptr<presentation::GlobalCanvasController> globalCanvasController;
    std::unique_ptr<presentation::DirectCaptureController> directCaptureController;
#if CLIP_SHOT_ENABLE_TEXT_TRANSLATION
    std::unique_ptr<presentation::SelectedTextTranslationCoordinator>
        selectedTextTranslationCoordinator;
#endif
    QPointer<MainWindow> mainWindow;
#ifdef Q_OS_MACOS
    std::unique_ptr<platform::macos::ApplicationReopenHandler> reopenHandler;
#endif
    bool started = false;
    update::UpdateService* updates = nullptr;
};

ApplicationController::ApplicationController(QApplication& application, QObject* parent)
    : QObject(parent), m_impl(std::make_unique<Impl>(*this, application)) {}

ApplicationController::~ApplicationController() = default;

void ApplicationController::start() {
    m_impl->start();
}

void ApplicationController::showMainWindow() {
    m_impl->showMainWindow();
}

void ApplicationController::handleLaunchRequest(const QStringList& arguments) {
    if (storage::ApplicationStorage::instance().directoryChanging())
        return;
    if (arguments.contains(QStringLiteral("--autostart"))) {
        return;
    }
    m_impl->showMainWindow();
}

void ApplicationController::restorePinnedWindows() {
    m_impl->restorePinnedWindows();
}
} // namespace clip_shot::app


