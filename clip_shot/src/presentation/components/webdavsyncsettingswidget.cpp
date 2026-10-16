#include "clip_shot/presentation/components/webdavsyncsettingswidget.h"

#include "clip_shot/presentation/components/settingscustomwidget.h"
#include "clip_shot/presentation/settings/settingsruntimesession.h"
#include "clip_shot/presentation/styles/mainwindowcomponenttoken.h"
#include "clip_shot/presentation/styles/thememanager.h"
#include "clip_shot/presentation/websyncservice.h"
#include "clip_shot/storage/applicationstorage.h"
#include "clip_shot/webdavsyncsettings.h"
#include "theme/theme_manager.h"
#include "widgets/alert.h"
#include "widgets/button.h"
#include "widgets/input_line_edit.h"
#include "widgets/switch.h"

#include <QEvent>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>

using namespace adqt::widgets;
using namespace clip_shot;
namespace settings = clip_shot::presentation::settings;

namespace clip_shot::presentation {

struct WebDavSyncSettingsWidget::Impl {
    Impl(WebDavSyncSettingsWidget* owner, settings::SettingsRuntimeSession& session)
        : self(owner), session(session) {}

    settings::SettingsRuntimeSession& session;
    WebDavSyncSettingsWidget* self = nullptr;

    QLabel* title = nullptr;
    AdSwitch* enabledSwitch = nullptr;
    AdLineEdit* urlField = nullptr;
    AdLineEdit* usernameField = nullptr;
    AdLineEdit* passwordField = nullptr;
    AdSwitch* historySwitch = nullptr;
    QLabel* statusLabel = nullptr;
    QLabel* passwordHint = nullptr;
    AdButton* syncButton = nullptr;
    AdAlert* error = nullptr;

    WebDavSyncSettings settings;
    bool loading = false;
    bool syncing = false;
};

WebDavSyncSettingsWidget::WebDavSyncSettingsWidget(settings::SettingsRuntimeSession& session,
                                                   QWidget* parent)
    : SettingsCustomWidget(parent), m_impl(std::make_unique<Impl>(this, session)) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_impl->title = new QLabel(this);
    m_impl->title->setObjectName(QStringLiteral("webDavSyncTitle"));
    layout->addWidget(m_impl->title);

    m_impl->error = new AdAlert(this);
    m_impl->error->setSeverity(AdAlert::Severity::Error);
    m_impl->error->hide();
    layout->addWidget(m_impl->error);

    auto* form = new QFormLayout;
    form->setContentsMargins(0, 0, 0, 0);
    form->setHorizontalSpacing(12);

    m_impl->enabledSwitch = new AdSwitch(this);
    m_impl->enabledSwitch->setObjectName(QStringLiteral("webDavSyncEnabled"));
    m_impl->enabledSwitch->setControlSize(AdSwitch::ControlSize::Medium);
    form->addRow(tr("Enable sync"), m_impl->enabledSwitch);

    m_impl->urlField = new AdLineEdit(this);
    m_impl->urlField->setObjectName(QStringLiteral("webDavSyncUrl"));
    m_impl->urlField->setPlaceholderText(tr("https://dav.example.com/dav/"));
    m_impl->urlField->setAllowClear(true);
    form->addRow(tr("Server URL"), m_impl->urlField);

    m_impl->usernameField = new AdLineEdit(this);
    m_impl->usernameField->setObjectName(QStringLiteral("webDavSyncUsername"));
    m_impl->usernameField->setAllowClear(true);
    form->addRow(tr("Username"), m_impl->usernameField);

    m_impl->passwordField = new AdLineEdit(this);
    m_impl->passwordField->setObjectName(QStringLiteral("webDavSyncPassword"));
    m_impl->passwordField->setEchoMode(QLineEdit::Password);
    m_impl->passwordField->setAllowClear(true);
    form->addRow(tr("Password"), m_impl->passwordField);

    m_impl->historySwitch = new AdSwitch(this);
    m_impl->historySwitch->setObjectName(QStringLiteral("webDavSyncHistory"));
    m_impl->historySwitch->setControlSize(AdSwitch::ControlSize::Medium);
    form->addRow(tr("Sync capture history"), m_impl->historySwitch);

    layout->addLayout(form);

    m_impl->passwordHint = new QLabel(this);
    m_impl->passwordHint->setObjectName(QStringLiteral("webDavSyncPasswordHint"));
    m_impl->passwordHint->setWordWrap(true);
    layout->addWidget(m_impl->passwordHint);

    m_impl->statusLabel = new QLabel(this);
    m_impl->statusLabel->setObjectName(QStringLiteral("webDavSyncStatus"));
    m_impl->statusLabel->setWordWrap(true);
    layout->addWidget(m_impl->statusLabel);

    m_impl->syncButton = new AdButton(this);
    m_impl->syncButton->setObjectName(QStringLiteral("webDavSyncNow"));
    m_impl->syncButton->setAccentRole(AdButton::AccentRole::Primary);
    m_impl->syncButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addWidget(m_impl->syncButton);

    connect(m_impl->enabledSwitch, &QAbstractButton::toggled, this,
            [this](bool checked) { onEnabledToggled(checked); });
    connect(m_impl->historySwitch, &QAbstractButton::toggled, this,
            [this](bool checked) { onHistoryToggled(checked); });
    connect(m_impl->syncButton, &AdButton::clicked, this, [this] { syncNow(); });

    auto& syncService = WebSyncService::instance();
    connect(&syncService, &WebSyncService::syncStatusChanged, this,
            [this](const QString& status) { updateStatus(status); });
    connect(&syncService, &WebSyncService::syncFinished, this,
            [this](bool success, const QString& message) { finishSync(success, message); });

    rebuild();
    retranslateUi();
}

WebDavSyncSettingsWidget::~WebDavSyncSettingsWidget() = default;

void WebDavSyncSettingsWidget::applyTheme(const styles::ThemeColorScheme& scheme) {
    layout()->setSpacing(scheme.metricAlias.margin);
    QFont contentFont = font();
    contentFont.setPixelSize(scheme.metricAlias.fontSize);
    setFont(contentFont);
    QFont titleFont = contentFont;
    titleFont.setWeight(QFont::DemiBold);
    m_impl->title->setFont(titleFont);
    QPalette colors = palette();
    colors.setColor(QPalette::WindowText, scheme.map.colorText);
    setPalette(colors);
}

void WebDavSyncSettingsWidget::retranslateUi() {
    retranslate();
}

void WebDavSyncSettingsWidget::changeEvent(QEvent* event) {
    SettingsCustomWidget::changeEvent(event);
    if (event != nullptr && event->type() == QEvent::LanguageChange) {
        retranslate();
    }
}

void WebDavSyncSettingsWidget::retranslate() {
    m_impl->title->setText(tr("WebDAV Sync"));
    m_impl->passwordHint->setText(
        tr("Settings and pinned windows always sync. The password is stored only on this "
           "device and never uploaded."));
    m_impl->syncButton->setText(tr("Sync now"));
    m_impl->urlField->setAccessibleName(tr("WebDAV server URL"));
    m_impl->usernameField->setAccessibleName(tr("WebDAV username"));
    m_impl->passwordField->setAccessibleName(tr("WebDAV password"));
    if (!m_impl->settings.lastSync.isEmpty()) {
        m_impl->statusLabel->setText(
            tr("Last sync: %1").arg(m_impl->settings.lastSync));
    }
    if (m_impl->syncing) {
        m_impl->syncButton->setText(tr("Syncing..."));
    }
}

void WebDavSyncSettingsWidget::rebuild() {
    m_impl->settings = m_impl->session.webDavSyncSettings();
    m_impl->loading = true;
    m_impl->enabledSwitch->setChecked(m_impl->settings.enabled);
    m_impl->urlField->setText(m_impl->settings.baseUrl);
    m_impl->usernameField->setText(m_impl->settings.username);
    m_impl->historySwitch->setChecked(m_impl->settings.syncHistory);
    m_impl->loading = false;
    const QString root = storage::ApplicationStorage::instance().status().effectiveDirectory;
    if (!root.isEmpty()) {
        m_impl->passwordField->setText(WebDavCredentialsStore(root).loadPassword());
    }
    updateStatus(m_impl->settings.lastStatus.isEmpty()
                     ? QString()
                     : m_impl->settings.lastStatus == QStringLiteral("ok")
                           ? tr("Last sync succeeded")
                           : m_impl->settings.lastStatus);
    retranslate();
}

void WebDavSyncSettingsWidget::updateStatus(const QString& status) {
    if (!status.isEmpty()) {
        m_impl->statusLabel->setText(status);
    }
}

void WebDavSyncSettingsWidget::finishSync(bool success, const QString& message) {
    m_impl->syncing = false;
    m_impl->syncButton->setText(tr("Sync now"));
    m_impl->error->setVisible(!success);
    if (!success) {
        m_impl->error->setText(message);
    } else {
        m_impl->error->hide();
        m_impl->settings = m_impl->session.webDavSyncSettings();
        updateStatus(message);
    }
}

bool WebDavSyncSettingsWidget::saveSettings() {
    m_impl->settings.enabled = m_impl->enabledSwitch->isChecked();
    m_impl->settings.baseUrl = m_impl->urlField->text().trimmed();
    m_impl->settings.username = m_impl->usernameField->text().trimmed();
    m_impl->settings.syncHistory = m_impl->historySwitch->isChecked();
    const bool success = m_impl->session.applyWebDavSyncSettings(m_impl->settings);
    m_impl->error->setVisible(!success);
    if (!success) {
        m_impl->error->setText(tr("Unable to save WebDAV sync settings. Check that "
                                  "configuration storage is writable and try again."));
    }
    return success;
}

void WebDavSyncSettingsWidget::savePassword(const QString& password) {
    const QString root = storage::ApplicationStorage::instance().status().effectiveDirectory;
    if (!root.isEmpty() && !password.isEmpty()) {
        WebDavCredentialsStore(root).savePassword(password);
    }
}

void WebDavSyncSettingsWidget::onEnabledToggled(bool checked) {
    if (m_impl->loading) {
        return;
    }
    m_impl->settings.enabled = checked;
    saveSettings();
}

void WebDavSyncSettingsWidget::onHistoryToggled(bool checked) {
    if (m_impl->loading) {
        return;
    }
    m_impl->settings.syncHistory = checked;
    saveSettings();
}

void WebDavSyncSettingsWidget::syncNow() {
    if (m_impl->syncing) {
        return;
    }
    savePassword(m_impl->passwordField->text());
    if (!saveSettings()) {
        return;
    }
    m_impl->syncing = true;
    m_impl->syncButton->setText(tr("Syncing..."));
    WebSyncService::instance().runSync(true);
}

} // namespace clip_shot::presentation
