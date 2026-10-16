#include "clip_shot/webdavsyncsettings.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace clip_shot {
namespace {
const QString kStateDirectory = QStringLiteral(".clip-webdav-sync");
const QString kCredentialsFileName = QStringLiteral("credentials.json");
} // namespace

WebDavCredentialsStore::WebDavCredentialsStore(QString storageRoot) {
    m_path = QDir(QDir::cleanPath(std::move(storageRoot)))
                 .filePath(kStateDirectory + u'/' + kCredentialsFileName);
}

QString WebDavCredentialsStore::loadPassword() const {
    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return {};
    }
    return document.object().value(QStringLiteral("password")).toString();
}

bool WebDavCredentialsStore::savePassword(const QString& password) const {
    const QDir directory(QFileInfo(m_path).absolutePath());
    if (!directory.mkpath(QStringLiteral("."))) {
        return false;
    }
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(QJsonDocument(QJsonObject{{QStringLiteral("password"), password}})
                   .toJson(QJsonDocument::Indented));
    return file.commit();
}

QString WebDavCredentialsStore::credentialsFilePath() const {
    return m_path;
}

} // namespace clip_shot
