#ifndef CLIP_SHOT_WEBDAVSYNCSETTINGS_H
#define CLIP_SHOT_WEBDAVSYNCSETTINGS_H

#include <QDateTime>
#include <QJsonObject>
#include <QMetaType>
#include <QString>
#include <QUrl>

namespace clip_shot {

// WebDAV sync settings persisted inside config.json. The password is NOT part
// of this structure: it is stored separately (see WebDavCredentialsStore) so
// that credentials never travel with the synced configuration file.
struct WebDavSyncSettings {
    bool enabled = false;
    QString baseUrl;
    QString username;
    // Capture history is optional; settings and pinned windows always sync.
    bool syncHistory = false;
    QString lastSync;
    QString lastStatus;

    friend bool operator==(const WebDavSyncSettings&, const WebDavSyncSettings&) = default;

    bool usable() const {
        const QUrl url(baseUrl, QUrl::StrictMode);
        return !username.trimmed().isEmpty() && url.isValid() && !url.isRelative() &&
               !url.host().isEmpty() &&
               (url.scheme() == QStringLiteral("http") ||
                url.scheme() == QStringLiteral("https")) &&
               url.userInfo().isEmpty() && !url.hasQuery() && !url.hasFragment();
    }
};

inline QJsonObject webDavSyncSettingsToJson(const WebDavSyncSettings& settings) {
    return {{QStringLiteral("enabled"), settings.enabled},
            {QStringLiteral("base_url"), settings.baseUrl},
            {QStringLiteral("username"), settings.username},
            {QStringLiteral("sync_history"), settings.syncHistory},
            {QStringLiteral("last_sync"), settings.lastSync},
            {QStringLiteral("last_status"), settings.lastStatus}};
}

inline WebDavSyncSettings webDavSyncSettingsFromJson(const QJsonValue& json,
                                                     bool* valid = nullptr) {
    WebDavSyncSettings settings;
    const auto object = json.toObject();
    bool ok = json.isObject();
    if (object.contains(QStringLiteral("enabled"))) {
        if (!object.value(QStringLiteral("enabled")).isBool()) {
            ok = false;
        } else {
            settings.enabled = object.value(QStringLiteral("enabled")).toBool();
        }
    }
    if (object.contains(QStringLiteral("base_url"))) {
        if (!object.value(QStringLiteral("base_url")).isString()) {
            ok = false;
        } else {
            settings.baseUrl = object.value(QStringLiteral("base_url")).toString();
        }
    }
    if (object.contains(QStringLiteral("username"))) {
        if (!object.value(QStringLiteral("username")).isString()) {
            ok = false;
        } else {
            settings.username = object.value(QStringLiteral("username")).toString();
        }
    }
    if (object.contains(QStringLiteral("sync_history"))) {
        if (!object.value(QStringLiteral("sync_history")).isBool()) {
            ok = false;
        } else {
            settings.syncHistory = object.value(QStringLiteral("sync_history")).toBool();
        }
    }
    if (object.contains(QStringLiteral("last_sync"))) {
        if (!object.value(QStringLiteral("last_sync")).isString()) {
            ok = false;
        } else {
            settings.lastSync = object.value(QStringLiteral("last_sync")).toString();
        }
    }
    if (object.contains(QStringLiteral("last_status"))) {
        if (!object.value(QStringLiteral("last_status")).isString()) {
            ok = false;
        } else {
            settings.lastStatus = object.value(QStringLiteral("last_status")).toString();
        }
    }
    if (valid != nullptr) {
        *valid = ok;
    }
    return settings;
}

// Password-only store kept at <storageRoot>/.clip-webdav-sync/credentials.json.
// The credentials file lives inside the state directory that the sync engine
// excludes from every snapshot, so secrets never leave the local machine.
class WebDavCredentialsStore {
public:
    explicit WebDavCredentialsStore(QString storageRoot);

    QString loadPassword() const;
    bool savePassword(const QString& password) const;
    QString credentialsFilePath() const;

private:
    QString m_path;
};

} // namespace clip_shot

Q_DECLARE_METATYPE(clip_shot::WebDavSyncSettings)

#endif // CLIP_SHOT_WEBDAVSYNCSETTINGS_H
