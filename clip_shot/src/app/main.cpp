#include "clip_shot/app/edition.h"
#include "clip_shot/platform/applicationqos.h"
#include "clip_shot/app/applicationcontroller.h"
#include "clip_shot/app/applicationrestart.h"
#include <QTemporaryDir>
#include <QProcess>
#include <QLocalServer>
#include <QLocalSocket>
#include <QUuid>
#include "clip_shot/app/singleinstancecoordinator.h"
#include "clip_shot/presentation/languagemanager.h"
#include "clip_shot/presentation/components/icons/clipshoticons.h"
#include "clip_shot/presentation/styles/thememanager.h"
#include "clip_shot/presentation/settings/applicationpriority.h"
#include "clip_shot/platform/windows/autostartregistration.h"
#include "clip_shot/platform/windows/administratorlaunch.h"
#include "widgets/message.h"
#include "widgets/platform_compatibility.h"
#include "clip_shot/presentation/components/screenshothistorypagewidget.h"
#include "clip_shot/storage/applicationstorage.h"
#include "clip_shot/update/startupupdate.h"
#include "clip_shot/diagnostics/diagnostics.h"
#include "diagnosticsbridge.h"
#include "startupregistrationreport.h"
#include "clip_shot/storage/settingsadapters.h"
#include "clip_shot/presentation/capture/screenshotcapturepolicy.h"
#include "../presentation/capture/screenshotcaptureperfinstrumentation.h"
#include "../presentation/pinned/screenshotpintoperfinstrumentation.h"

#include "icon_renderer.h"
#include "locale/locale.h"
#include "widgets/tooltip.h"
#include "theme/theme_manager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QRegularExpression>
#include <QString>
#include <QDir>
#include <QStandardPaths>
#include <QTimer>
#include <QSysInfo>
#include <optional>
#include <utility>
#include "clip_capture.h"
#include "clip_recording.h"
#ifdef CLIP_SHOT_MCP_TEST_FIXTURE
#include "clip_shot/storage/capturehistoryrepository.h"
#include "clip_draw_engine_qt/clip_canvas_runtime.h"
#endif
#ifdef Q_OS_MACOS
#include "clip_shot/platform/macos/loginitemservice.h"
#include "clip_shot/platform/macos/rastercolorspace.h"
#include <QScopeGuard>
#include <future>
#include <thread>
#include <QTimer>
#include <QScreen>
#endif

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

extern "C" void clip_diagnostics_install_panic_hook(void (*callback)(const unsigned char*, size_t));

namespace {
#ifndef Q_OS_MACOS
QString updateInstallationRoot(const QString& executableDirectory) {
    return QFileInfo(executableDirectory).dir().absolutePath();
}

std::optional<bool> updateTransactionPending(const QString& helperPath, const QString& root) {
    QProcess helper;
    helper.start(helperPath,
                 {QStringLiteral("--transaction-state"), QStringLiteral("--target"), root},
                 QIODevice::ReadOnly);
    if (!helper.waitForStarted(10000) || !helper.waitForFinished(10000)) {
        return std::nullopt;
    }
    const QByteArray output = helper.readAllStandardOutput().trimmed();
    if (helper.exitStatus() != QProcess::NormalExit || helper.exitCode() != 0 ||
        (output != "pending" && output != "clean")) {
        return std::nullopt;
    }
    return output == "pending";
}
#endif
} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication::setOrganizationName(clip_shot::app::edition::registryName());
    QString applicationName = clip_shot::app::edition::applicationName();
    QString e2eInstanceId;
    bool e2eCaptureEnabled = false;
    for (int index = 1; index < argc; ++index) {
        const QString argument = QString::fromLocal8Bit(argv[index]);
        e2eCaptureEnabled =
            e2eCaptureEnabled || argument == QStringLiteral("--e2e-allow-overlay-capture");
        constexpr auto e2eInstancePrefix = "--e2e-instance-id=";
        if (argument.startsWith(QString::fromLatin1(e2eInstancePrefix))) {
            e2eInstanceId =
                argument.mid(static_cast<int>(std::char_traits<char>::length(e2eInstancePrefix)));
        }
    }
    if (QRegularExpression(QStringLiteral("^[A-Za-z0-9_-]{1,64}$"))
            .match(e2eInstanceId)
            .hasMatch()) {
        applicationName += QStringLiteral("-e2e-") + e2eInstanceId;
    }
    QCoreApplication::setApplicationName(applicationName);
    QCoreApplication::setApplicationVersion(QStringLiteral(CLIP_DIAGNOSTICS_VERSION));
#ifdef CLIP_SHOT_MCP_TEST_FIXTURE
    // Only test builds expose this isolated production-router fixture. It bypasses
    // singleton acquisition, startup registration, and the user's configuration.
    if (argc >= 3 && QString::fromLocal8Bit(argv[1]) == u"--mcp-fixture") {
        const QFileInfo rootInfo(QString::fromLocal8Bit(argv[2]));
        if (!rootInfo.isAbsolute() || !rootInfo.isDir() || rootInfo.isSymLink())
            return 2;
        const QString root = rootInfo.canonicalFilePath();
        qputenv("CLIP_SHOT_MCP_DESCRIPTOR",
                QDir(root).filePath(QStringLiteral("descriptor.json")).toUtf8());
        QCoreApplication::setApplicationName(QStringLiteral("clip-shot-mcp-fixture-") +
                                             QUuid::createUuid().toString(QUuid::Id128));
        QStandardPaths::setTestModeEnabled(true);
        QApplication fixture(argc, argv);
        fixture.setQuitOnLastWindowClosed(false);
        adqt::widgets::initializePlatformCompatibility(fixture);
        const QString executable = QDir(root).filePath(QStringLiteral("bin"));
        if (!QDir().mkpath(executable))
            return 3;
        auto& storage = clip_shot::storage::ApplicationStorage::instance();
        if (!storage.initialize({executable, root, 60000}).success)
            return 4;
        storage.configuration().setValues(
            {{QStringLiteral("mcp/enabled"), true},
             {QStringLiteral("tray/enabled"), false},
             {QStringLiteral("pin_to_screen/automatic_text_recognition"), false}});
        {
            // Publish through the real repository so router tests can exercise
            // history payloads and artifacts without capturing the user's desktop.
            ClipCanvasRuntime canvas;
            clip_shot::storage::CaptureHistoryDraft draft;
            draft.id = QStringLiteral("fd97d4e3-311c-48de-bbfb-3b4f43b0fef6");
            draft.createdUtc = QDateTime::currentDateTimeUtc();
            draft.canvasBounds = QRect(0, 0, 80, 60);
            draft.selection.rectangle = draft.canvasBounds;
            draft.selection.shadowColor = QColor(Qt::black);
            draft.canvasHistory = canvas.serializeDocumentHistory();
            QImage image(draft.canvasBounds.size(), QImage::Format_RGBA8888);
            image.fill(QColor(40, 150, 210));
            draft.displays.append({QStringLiteral("fixture"), QStringLiteral("Fixture"), image});
            draft.resultImage = image;
            if (!storage.captureHistory().publish(std::move(draft)).get().storage.success)
                return 5;
        }
        clip_shot::presentation::LanguageManager::instance().initialize();
        clip_shot::presentation::styles::ThemeManager::instance().initialize(fixture);
        int result = 0;
        {
            clip_shot::app::ApplicationController controller(fixture);
            QTimer::singleShot(180000, &fixture, &QCoreApplication::quit);
            result = fixture.exec();
        }
        storage.shutdown();
        return result;
    }
#endif
    bool applicationRestart = false;
    if (argc > 1 && QString::fromLocal8Bit(argv[1]) == u"--restart-helper") {
        QCoreApplication helper(argc, argv);
        const int result = clip_shot::app::dispatchApplicationRestartHelper(helper.arguments());
        if (result != -1)
            return result;
        applicationRestart = true;
    }
    bool administratorRestart = false;
    if (argc > 1 && QString::fromLocal8Bit(argv[1]) == u"--administrator-helper") {
        QCoreApplication helper(argc, argv);
        const int result =
            clip_shot::platform::windows::dispatchAdministratorHelper(helper.arguments());
        if (result != -1)
            return result;
        administratorRestart = true;
    }
#ifdef Q_OS_MACOS
    // Runs the shipped recording bridge with the Cocoa event loop, without user storage.
    if (argc >= 3 && QString::fromLocal8Bit(argv[1]) == u"--recording-macos-probe") {
        QApplication probe(argc, argv);
        adqt::widgets::initializePlatformCompatibility(probe);
        const QStringList arguments = probe.arguments();
        const QByteArray path = QFileInfo(arguments[2]).absoluteFilePath().toUtf8();
        const QRect screen = probe.primaryScreen()->geometry();
        const QPoint origin = screen.topLeft() + QPoint(40, 40);
        auto result = std::async(std::launch::async, [path, arguments, origin] {
            clip_shot::platform::applyApplicationQoSToCurrentThread();
            ClipCaptureDirectRecordingConfig config{};
            config.version = CLIP_CAPTURE_DIRECT_RECORDING_CONFIG_VERSION;
            config.struct_size = sizeof(config);
            config.x = origin.x();
            config.y = origin.y();
            config.width = 321;
            config.height = 239;
            config.capture_backend = CLIP_CAPTURE_BACKEND_AUTO;
            config.output_file_utf8 = path.constData();
            const auto suffix = QFileInfo(QString::fromUtf8(path)).suffix();
            config.output_format = suffix == u"gif"    ? CLIP_RECORDING_OUTPUT_FORMAT_GIF
                                   : suffix == u"apng" ? CLIP_RECORDING_OUTPUT_FORMAT_APNG
                                   : suffix == u"webp" ? CLIP_RECORDING_OUTPUT_FORMAT_WEBP
                                                       : CLIP_RECORDING_OUTPUT_FORMAT_MP4;
            config.capture_fps = 30;
            config.output_fps = 15;
            config.maximum_width = 1920;
            config.maximum_height = 1080;
            config.quality = 80;
            config.codec = arguments.contains(u"hevc") ? CLIP_CAPTURE_VIDEO_CODEC_H265
                                                       : CLIP_CAPTURE_VIDEO_CODEC_H264;
            config.preset = CLIP_CAPTURE_VIDEO_ENCODING_PRESET_VERYFAST;
            config.encoder_preference = arguments.contains(u"software")
                                            ? CLIP_CAPTURE_ENCODER_PREFERENCE_SOFTWARE
                                            : CLIP_CAPTURE_ENCODER_PREFERENCE_H264_HARDWARE;
            config.enable_system_audio = static_cast<uint8_t>(arguments.contains(u"system-audio"));
            config.enable_microphone = static_cast<uint8_t>(arguments.contains(u"microphone"));
            config.show_cursor = 1;
            config.keyboard_size = 64;
            config.mouse_trail_duration_ms = 500;
            config.loop_animated_images = 1;
            if (arguments.contains(u"effects")) {
                config.show_keyboard = 1;
                config.record_mouse_clicks = 1;
                config.mouse_trail_rgba = 0xff4080e0;
                config.mouse_click_rgba = 0x40a0ffe0;
                config.keyboard_background_rgba = 0x000000cc;
                config.keyboard_text_rgba = 0xffffffff;
                config.keyboard_border_rgba = 0x808080ff;
                config.mouse_highlight_rgba = 0xffff0080;
            }
            uint32_t width = 0, height = 0;
            if (!clip_recording_region_output_dimensions(
                    config.x, config.y, config.width, config.height, config.maximum_width,
                    config.maximum_height, config.output_format, &width, &height)) {
                qWarning("%s", clip_recording_last_error_message());
                return 1;
            }
            ClipRecordingSession* session = nullptr;
            if (clip_recording_session_create_direct(&config, &session) !=
                CLIP_RECORDING_RESULT_OK) {
                qWarning("%s", clip_recording_last_error_message());
                return 2;
            }
            const auto destroy =
                qScopeGuard([session] { clip_recording_session_destroy(session); });
            if (!clip_recording_session_start(session)) {
                qWarning("%s", clip_recording_last_error_message());
                return 3;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
            if (!clip_recording_session_pause(session))
                return 4;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            if (!clip_recording_session_resume(session))
                return 5;
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
            if (clip_recording_session_stop(session) != CLIP_RECORDING_RESULT_OK) {
                qWarning("%s", clip_recording_last_error_message());
                return 6;
            }
            qInfo("macos recording probe: logical=321x239 output=%ux%u", width, height);
            return 0;
        });
        QTimer poll;
        QObject::connect(&poll, &QTimer::timeout, &probe, [&] {
            if (result.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                probe.exit(result.get());
        });
        poll.start(20);
        return probe.exec();
    }
#endif
    // Package QA uses the ordinary FFI and linked vendor encoders, without
    // opening UI, taking the singleton, or modifying the user's settings.
    if ((argc == 4 || argc == 5) && QString::fromLocal8Bit(argv[1]) == u"--recording-gpu-probe") {
        QCoreApplication probe(argc, argv);
        const QByteArray path = QString::fromLocal8Bit(argv[2]).toUtf8();
        const QString backend = QString::fromLocal8Bit(argv[3]);
        if (backend != u"dxgi" && backend != u"wgc") {
            return 2;
        }
        const bool recover = argc == 5;
        if (recover && QString::fromLocal8Bit(argv[4]) != u"recover") {
            return 2;
        }
        ClipCaptureDirectRecordingConfig config{};
        config.version = CLIP_CAPTURE_DIRECT_RECORDING_CONFIG_VERSION;
        config.struct_size = sizeof(config);
        config.width = 640;
        config.height = 480;
        config.capture_backend = backend == u"dxgi" ? 1 : 2;
        config.output_file_utf8 = path.constData();
        config.capture_fps = 30;
        config.output_fps = 30;
        config.preset = 1;
        config.quality = 80;
        config.encoder_preference = 1;
        config.enable_system_audio = 1;
        config.show_cursor = 1;
        config.mouse_trail_duration_ms = 500;
        config.keyboard_size = 64;
        const auto result = clip_recording_gpu_probe(&config, recover ? 1 : 0);
        if (result != CLIP_RECORDING_RESULT_OK) {
            qWarning().noquote() << clip_recording_last_error_message();
        }
        return result == CLIP_RECORDING_RESULT_OK ? 0 : 1;
    }
    // A probe runs before diagnostics, singleton acquisition, or any live user-state access.
    if (argc == 3 && QString::fromLocal8Bit(argv[1]) == u"--update-probe") {
        if (QString::fromLocal8Bit(argv[2]) != QCoreApplication::applicationVersion()) {
            return 3;
        }
        qputenv("QT_QPA_PLATFORM", "offscreen");
        QApplication probe(argc, argv);
        adqt::widgets::initializePlatformCompatibility(probe);
        QTemporaryDir directory;
        if (!directory.isValid()) {
            return 4;
        }
        auto& storage = clip_shot::storage::ApplicationStorage::instance();
        if (!storage.initialize({directory.path(), directory.path(), 8000}).success) {
            return 5;
        }
        clip_shot::presentation::LanguageManager::instance().initialize();
        const auto startupSettings = clip_shot::storage::SystemSettings();
        const auto startupResult = clip_shot::platform::windows::reconcileStartupMode(
            !startupSettings.autoStartAtBoot() ? clip_shot::platform::windows::StartupMode::Off
            : startupSettings.launchAsAdministrator()
                ? clip_shot::platform::windows::StartupMode::ElevatedTask
                : clip_shot::platform::windows::StartupMode::Registry);

        clip_shot::presentation::styles::ThemeManager::instance().initialize(probe);
        storage.shutdown();
        return 0;
    }
#ifndef Q_OS_MACOS
    // macOS updates use a downloaded DMG, not the standalone transaction helper.
    QString executablePath = QString::fromLocal8Bit(argv[0]);
#ifdef Q_OS_WIN
    wchar_t modulePath[32768]{};
    const DWORD moduleLength = GetModuleFileNameW(nullptr, modulePath, 32768);
    if (moduleLength > 0 && moduleLength < 32768) {
        executablePath = QString::fromWCharArray(modulePath, static_cast<int>(moduleLength));
    }
#endif
    const QString updateRoot = updateInstallationRoot(QFileInfo(executablePath).absolutePath());
    const QString updateHelper = QDir(QFileInfo(executablePath).absolutePath())
                                     .filePath(clip_shot::app::edition::updaterName());
    const auto pendingUpdate = updateTransactionPending(updateHelper, updateRoot);
    if (!pendingUpdate.has_value()) {
        return 6;
    }
    if (*pendingUpdate) {
        QCoreApplication recovery(argc, argv);
        const QString root = updateInstallationRoot(QCoreApplication::applicationDirPath());
        const QString pipe =
            QStringLiteral("clip-shot-recover-") + QUuid::createUuid().toString(QUuid::Id128);
        QLocalServer server;
        server.setSocketOptions(QLocalServer::UserAccessOption);
        if (!server.listen(pipe)) {
            return 6;
        }
        const QString result =
            QDir(QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                     .canonicalPath())
                .filePath(pipe + QStringLiteral(".txt"));
        const QStringList args{
            QStringLiteral("--launch"), QStringLiteral("--recovery"),
            QStringLiteral("--target"), root,
            QStringLiteral("--parent"), QString::number(QCoreApplication::applicationPid()),
            QStringLiteral("--pipe"),   pipe,
            QStringLiteral("--result"), result};
        if (!QProcess::startDetached(updateHelper, args)) {
            return 6;
        }
        if (!server.waitForNewConnection(180000)) {
            return 6;
        }
        auto* socket = server.nextPendingConnection();
        if (!socket->canReadLine() && !socket->waitForReadyRead(10000)) {
            return 6;
        }
        if (socket->readLine().trimmed() != "ready") {
            return 6;
        }
        socket->write("go\n");
        socket->waitForBytesWritten(5000);
        return 0;
    }
#endif
#ifdef Q_OS_MACOS
    // Exercise normal pre-application startup without touching live user state.
    if (argc == 2 && QString::fromLocal8Bit(argv[1]) == u"--startup-probe") {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        QApplication probe(argc, argv);
        adqt::widgets::initializePlatformCompatibility(probe);
        return 0;
    }
#endif
    auto& diagnostics = clip_shot::diagnostics::DiagnosticsService::instance();
    struct DiagnosticsLifetime {
        ~DiagnosticsLifetime() {
            clip_shot::diagnostics::DiagnosticsService::instance().shutdown();
        }
    } diagnosticsLifetime;
    clip_shot::storage::StorageInitializationOptions storageOptions;
    storageOptions.resolvedDirectory =
        std::make_shared<clip_shot::storage::StorageDirectorySelection>(
            clip_shot::storage::ApplicationStorage::resolveDirectory());
    clip_shot::diagnostics::DiagnosticsOptions diagnosticsOptions;
    const auto& selectedStorage = *storageOptions.resolvedDirectory;
    if (!selectedStorage.effectiveDirectory.isEmpty()) {
        diagnosticsOptions.directories.append(
            QDir(selectedStorage.effectiveDirectory).filePath(QStringLiteral("logs")));
    }
    diagnosticsOptions.directories.append(
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
            .filePath(QStringLiteral("logs")));
    // Crashpad registers process-wide exception resources once. Its live database must
    // stay at a stable path while the user-selected data/log directory is migrated.
    diagnosticsOptions.crashCaptureDirectory =
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
            .filePath(QStringLiteral("logs/crashes"));
    diagnosticsOptions.directories.append(
        QDir(QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).canonicalPath())
            .filePath(QStringLiteral("ClipShot/%1/logs").arg(applicationName)));
#ifdef Q_OS_MACOS
    diagnosticsOptions.handlerPath =
        QDir(selectedStorage.executableDirectory).filePath(QStringLiteral("crashpad_handler"));
#else
    diagnosticsOptions.handlerPath =
        QDir(selectedStorage.executableDirectory).filePath(QStringLiteral("crashpad_handler.exe"));
#endif
    diagnosticsOptions.version = QStringLiteral(CLIP_DIAGNOSTICS_VERSION);
    diagnosticsOptions.revision = QStringLiteral(CLIP_DIAGNOSTICS_REVISION);
    diagnosticsOptions.buildConfiguration = QStringLiteral(CLIP_DIAGNOSTICS_BUILD);
    static_cast<void>(diagnostics.initialize(std::move(diagnosticsOptions)));
    clip_diagnostics_install_panic_hook(clip_diag_panic);

    // Capture overlays embed native window-type children (the floating tool
    // palette) alongside alien, click-through children (selection toolbar,
    // shortcut hints). Qt would otherwise force every sibling of an embedded
    // native child native, which silently turns those click-through surfaces
    // into OS-level input interceptors.
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

#ifdef Q_OS_MACOS
    clip_shot::platform::macos::configureRasterColorSpace();
    clip_shot::platform::macos::observeNativeLoginItemLaunch();
#endif
    // The internal application name also owns settings and single-instance keys.
    // Keep it stable while giving Qt windows and the macOS menu the product name.
    QGuiApplication::setApplicationDisplayName(clip_shot::app::edition::productName());
    QApplication app(argc, argv);
    adqt::widgets::initializePlatformCompatibility(app);
    clip_shot::diagnostics::logEvent(QStringLiteral("clip_shot.app"),
                                     QStringLiteral("application.platform"),
                                     {{QStringLiteral("backend"), QGuiApplication::platformName()},
                                      {QStringLiteral("os"), QSysInfo::kernelVersion()}});
#ifdef Q_OS_MACOS
    const auto logDisplay = [](QScreen* screen) {
        const QRect geometry = screen->geometry();
        clip_shot::diagnostics::logEvent(
            QStringLiteral("clip_shot.platform"), QStringLiteral("display.configuration"),
            {{QStringLiteral("width"), geometry.width()},
             {QStringLiteral("height"), geometry.height()},
             {QStringLiteral("x"), geometry.x()},
             {QStringLiteral("y"), geometry.y()},
             {QStringLiteral("scale"), screen->devicePixelRatio()},
             {QStringLiteral("refresh_rate"), screen->refreshRate()},
             {QStringLiteral("count"), QGuiApplication::screens().size()}});
    };
    const auto observeDisplay = [&app, logDisplay](QScreen* screen) {
        logDisplay(screen);
        QObject::connect(screen, &QScreen::geometryChanged, &app,
                         [screen, logDisplay] { logDisplay(screen); });
        QObject::connect(screen, &QScreen::logicalDotsPerInchChanged, &app,
                         [screen, logDisplay] { logDisplay(screen); });
        QObject::connect(screen, &QScreen::refreshRateChanged, &app,
                         [screen, logDisplay] { logDisplay(screen); });
    };
    for (auto* screen : QGuiApplication::screens())
        observeDisplay(screen);
    QObject::connect(&app, &QGuiApplication::screenAdded, &app, observeDisplay);
    QObject::connect(&app, &QGuiApplication::screenRemoved, &app, [] {
        clip_shot::diagnostics::logEvent(
            QStringLiteral("clip_shot.platform"), QStringLiteral("display.removed"),
            {{QStringLiteral("count"), QGuiApplication::screens().size()}});
    });
#endif
    static_cast<void>(clip_shot::presentation::capture::resolveAutoScreenshotApiMode());
#if defined(CLIP_SHOT_PIN_PERF_INSTRUMENTATION)
    clip_shot::presentation::pin_perf::configureTrace(
        qEnvironmentVariable("CLIP_SHOT_PIN_PERF_TRACE"));
#endif
#if defined(CLIP_SHOT_CAPTURE_PERF_INSTRUMENTATION)
    clip_shot::presentation::capture_perf::configureTrace(
        qEnvironmentVariable("CLIP_SHOT_CAPTURE_PERF_TRACE"));
#endif
    auto launchArguments =
        applicationRestart ? clip_shot::app::normalApplicationArguments(QApplication::arguments())
                           : QApplication::arguments();
    const bool updaterRelaunch = clip_shot::update::isStartupUpdateRelaunch(launchArguments);
    launchArguments = clip_shot::update::normalStartupArguments(std::move(launchArguments));
#ifdef Q_OS_MACOS
    launchArguments = clip_shot::platform::macos::loginItemLaunchArguments(
        launchArguments, clip_shot::platform::macos::initialNativeLoginItemLaunch());
#endif
    clip_shot::app::SingleInstanceCoordinator singleInstance;
    const clip_shot::app::SingleInstanceResult instanceResult =
        singleInstance.acquireOrForward(launchArguments);
    if (instanceResult.outcome == clip_shot::app::SingleInstanceOutcome::Forwarded) {
        return 0;
    }
    if (instanceResult.outcome == clip_shot::app::SingleInstanceOutcome::Failed) {
        qWarning().noquote() << instanceResult.error;
        return 2;
    }

    static_cast<void>(
        clip_shot::storage::ApplicationStorage::instance().initialize(storageOptions));
    struct StorageLifetime {
        ~StorageLifetime() {
            clip_shot::storage::ApplicationStorage::instance().shutdown();
        }
    } storageLifetime;
    // Declared after storageLifetime so reverse-order destruction drains the
    // history executor while the storage singleton it reads is still alive and
    // rejects straggler submissions; no history task runs after main returns.
    struct HistoryTaskDrain {
        ~HistoryTaskDrain() {
            shutdownScreenshotHistoryTasks();
        }
    } historyTaskDrain;
#ifndef Q_OS_MACOS
    static_cast<void>(clip_shot::presentation::settings::applyConfiguredApplicationPriority());
#endif
    QApplication::setQuitOnLastWindowClosed(false);
#ifndef Q_OS_MACOS
    QApplication::setWindowIcon(
        adqt::icons::makeIcon(clip_shot::presentation::icons::custom::app::ApplicationIcon()));
#endif
    adqt::locale::LocaleManager::instance().applyTo(app);
    clip_shot::presentation::LanguageManager::instance().initialize();
    clip_shot::presentation::styles::ThemeManager::instance().initialize(app);
#ifdef Q_OS_WIN
    auto& applicationStorage = clip_shot::storage::ApplicationStorage::instance();
    if (!updaterRelaunch &&
        applicationStorage.configuration().value(QStringLiteral("updates/mode")).toString() ==
            u"next_launch") {
        clip_shot::update::UpdateService startupUpdate(
            clip_shot::update::defaultUpdateServiceOptions());
        startupUpdate.setMode(QStringLiteral("next_launch"));
        startupUpdate.setProgressAppearance(
            clip_shot::presentation::styles::ThemeManager::instance().updateProgressAppearance());
        QObject::connect(&adqt::theme::ThemeManager::instance(),
                         &adqt::theme::ThemeManager::themeChanged, &startupUpdate,
                         [&startupUpdate] {
                             startupUpdate.setProgressAppearance(
                                 clip_shot::presentation::styles::ThemeManager::instance()
                                     .updateProgressAppearance());
                         });
        startupUpdate.setSystemProxy(
            applicationStorage.configuration().value(QStringLiteral("network/proxy")).toString() ==
            u"system");
        bool foregroundRequested = administratorRestart;
        const auto forwardedLaunch = QObject::connect(
            &singleInstance, &clip_shot::app::SingleInstanceCoordinator::launchRequestReceived,
            &app, [&](const QStringList& arguments) {
                foregroundRequested =
                    foregroundRequested || !arguments.contains(QStringLiteral("--autostart"));
                startupUpdate.setRelaunchArguments(
                    clip_shot::update::startupUpdateRelaunchArguments(foregroundRequested));
            });
        const auto startupUpdateResult = clip_shot::update::runStartupUpdate(
            startupUpdate, [&applicationStorage] { return applicationStorage.flushNow().success; },
            [&] { return foregroundRequested; });
        QObject::disconnect(forwardedLaunch);
        if (startupUpdateResult == clip_shot::update::StartupUpdateResult::ExitForUpdate)
            return 0;
    }
#else
    static_cast<void>(updaterRelaunch);
#endif
    const auto startupSettings = clip_shot::storage::SystemSettings();
#ifdef Q_OS_MACOS
    const auto startupResult =
        clip_shot::platform::macos::loginItemAutomaticRegistrationAllowed(launchArguments)
            ? clip_shot::platform::macos::loginItemService().initialize(
                  startupSettings.autoStartAtBoot())
            : clip_shot::platform::macos::LoginItemResult{};
#else
    const auto startupResult = clip_shot::platform::windows::reconcileStartupMode(
        !startupSettings.autoStartAtBoot() ? clip_shot::platform::windows::StartupMode::Off
        : startupSettings.launchAsAdministrator()
            ? clip_shot::platform::windows::StartupMode::ElevatedTask
            : clip_shot::platform::windows::StartupMode::Registry);
#endif

    adqt::widgets::AdTooltip::installApplicationTooltips();

    clip_shot::app::ApplicationController applicationController(app);
    singleInstance.setLaunchRequestHandler([&applicationController](const QStringList& arguments) {
        applicationController.handleLaunchRequest(
            clip_shot::update::normalStartupArguments(arguments));
    });
    applicationController.start();
    if (!startupResult.success) {
        clip_shot::app::reportStartupRegistrationFailure(startupResult.error);
#ifdef Q_OS_MACOS
        QTimer::singleShot(0, &app, [&applicationController, error = startupResult.error] {
            // Startup can otherwise have only a tiny floating toolbar visible.
            // Give the error a readable application surface before opening it.
            applicationController.showMainWindow();
            adqt::widgets::AdMessage::Request request;
            request.content = error;
            adqt::widgets::AdMessageService::error(std::move(request));
        });
#endif
    }
    if (administratorRestart)
        applicationController.showMainWindow();
    clip_shot::diagnostics::logEvent(QStringLiteral("clip_shot.app"),
                                     QStringLiteral("application.ready"));
    if (!launchArguments.contains(QStringLiteral("--autostart")) &&
        launchArguments.contains(QStringLiteral("--show-main-window"))) {
        applicationController.showMainWindow();
    }
    return QApplication::exec();
}
