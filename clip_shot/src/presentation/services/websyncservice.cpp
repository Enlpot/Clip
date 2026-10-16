#include "clip_shot/presentation/websyncservice.h"

#include "clip_shot/network/webdavclient.h"
#include "clip_shot/storage/applicationstorage.h"
#include "clip_shot/storage/settingsadapters.h"
#include "clip_shot/storage/webdavsyncengine.h"
#include "clip_shot/webdavsyncsettings.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QTimer>

namespace clip_shot::presentation {
namespace {

using clip_shot::webdav_sync::SyncEntry;
using clip_shot::webdav_sync::SyncScopeSnapshot;
using clip_shot::webdav_sync::WebSyncPlan;
using clip_shot::webdav_sync::WebSyncStateStore;
using clip_shot::webdav_sync::collectLocalSnapshot;

// Remote files live under a dedicated subdirectory so a shared WebDAV root
// (Nutstore dav root, Nextcloud base) is not polluted with sync internals.
const QString kRemoteRoot = QStringLiteral("clip-sync");
const QString kStateDirectoryName = QStringLiteral(".clip-webdav-sync");

QString remoteRelativePath(const QString& path) {
    return path.isEmpty() ? kRemoteRoot : kRemoteRoot + u'/' + path;
}

QString localConflictPath(const QString& path, qint64 timestamp) {
    return path + QStringLiteral(".conflict-") + QString::number(timestamp);
}

} // namespace

struct WebSyncService::Impl {
    explicit Impl(WebSyncService* owner) : service(owner) {}

    void start(bool force) {
        if (running) {
            return;
        }
        settings = storage::WebDavSyncConfigurationSettings().settings();
        const auto& storageStatus = storage::ApplicationStorage::instance().status();
        root = storageStatus.effectiveDirectory;
        if (root.isEmpty()) {
            finish(false, QObject::tr("WebDAV sync is unavailable because the storage "
                                      "directory could not be resolved"));
            return;
        }
        if (!force && !settings.enabled) {
            return;
        }
        if (!settings.usable()) {
            finish(false, QObject::tr("WebDAV sync is not configured. Provide a valid server "
                                      "URL and account before syncing."));
            return;
        }
        credentials = WebDavCredentialsStore(root).loadPassword();
        if (credentials.isEmpty()) {
            finish(false, QObject::tr("WebDAV password is not set. Save the account "
                                      "credentials before syncing."));
            return;
        }

        running = true;
        client = std::make_unique<webdav::WebDavClient>();
        QUrl base(settings.baseUrl);
        base.setPath(base.path() + u'/' + kRemoteRoot);
        client->setCredentials(base, settings.username, credentials);
        client->setIgnoreSslErrors(false);

        emit service->syncStatusChanged(
            QObject::tr("Scanning local files and the remote WebDAV server..."));

        QStringList excluded = {kStateDirectoryName};
        if (!settings.syncHistory) {
            excluded.append(QStringLiteral("capture_history"));
        }
        localSnapshot = collectLocalSnapshot(root, excluded);
        remoteSnapshot = {};
        stateStore = std::make_unique<WebSyncStateStore>(root);
        previous = stateStore->load();
        // Read the remote listing, then filter it by the history toggle so a
        // disabled history never gets pulled down.
        client->list(QString(), [this](const QVector<webdav::WebDavResource>& resources,
                                       const webdav::WebDavError& error) {
            if (error.failed) {
                finish(false,
                       QObject::tr("Could not list the WebDAV directory: %1").arg(error.message));
                return;
            }
            for (const auto& resource : resources) {
                if (resource.isDirectory) {
                    continue;
                }
                if (!settings.syncHistory &&
                    (resource.path.startsWith(QStringLiteral("capture_history/")) ||
                     resource.path == QStringLiteral("capture_history"))) {
                    continue;
                }
                remoteSnapshot.files.insert(resource.path,
                                            SyncEntry{resource.contentLength,
                                                      resource.lastModified});
            }
            plan = buildSyncPlan(localSnapshot, remoteSnapshot, previous);
            uploadIndex = 0;
            downloadIndex = 0;
            deleteIndex = 0;
            conflictIndex = 0;
            emit service->syncStatusChanged(
                QObject::tr("Syncing: %1 upload(s), %2 download(s), %3 deletion(s)")
                    .arg(plan.uploads.size())
                    .arg(plan.downloads.size() + plan.conflicts.size())
                    .arg(plan.remoteDeletes.size()));
            runUploads();
        });
    }

    void runUploads() {
        if (uploadIndex >= plan.uploads.size()) {
            runDownloads();
            return;
        }
        const auto& entry = plan.uploads.at(uploadIndex);
        ++uploadIndex;
        const QString localPath = QDir(root).filePath(entry.path);
        if (!QFileInfo::exists(localPath)) {
            runUploads();
            return;
        }
        const QString path = entry.path;
        QObject::connect(client.get(), &webdav::WebDavClient::uploadFinished, client.get(),
                         [this, path](const QString& finishedPath,
                                      const webdav::WebDavError& error) {
                             if (finishedPath != path) {
                                 return;
                             }
                             if (error.failed) {
                                 finish(false, QObject::tr("Upload failed for %1: %2")
                                                   .arg(path, error.message));
                                 return;
                             }
                             runUploads();
                         },
                         Qt::SingleShotConnection);
        client->upload(path, localPath);
    }

    void runDownloads() {
        if (downloadIndex >= plan.downloads.size()) {
            runConflicts();
            return;
        }
        const auto& entry = plan.downloads.at(downloadIndex);
        ++downloadIndex;
        downloadInto(entry.path, entry.path);
    }

    void runConflicts() {
        if (conflictIndex >= plan.conflicts.size()) {
            runDeletes();
            return;
        }
        const auto& entry = plan.conflicts.at(conflictIndex);
        ++conflictIndex;
        downloadInto(entry.path,
                     localConflictPath(entry.path, QDateTime::currentSecsSinceEpoch()));
    }

    void downloadInto(const QString& remotePath, const QString& localRelativePath) {
        const QString localPath = QDir(root).filePath(localRelativePath);
        if (!QDir().mkpath(QFileInfo(localPath).absolutePath())) {
            finish(false, QObject::tr("Could not create the local directory for %1")
                              .arg(localRelativePath));
            return;
        }
        QObject::connect(client.get(), &webdav::WebDavClient::downloadFinished, client.get(),
                         [this, remotePath, localRelativePath](
                             const QString& finishedPath, const QString& localFilePath,
                             const webdav::WebDavError& error) {
                             Q_UNUSED(localFilePath);
                             if (finishedPath != remotePath) {
                                 return;
                             }
                             if (error.failed) {
                                 finish(false, QObject::tr("Download failed for %1: %2")
                                                   .arg(remotePath, error.message));
                                 return;
                             }
                             runDownloadsOrConflicts();
                         },
                         Qt::SingleShotConnection);
        client->download(remotePath, localPath);
    }

    // Downloads and conflicts share one request pipeline; a single finished
    // callback resumes whichever phase is active.
    void runDownloadsOrConflicts() {
        if (downloadIndex < plan.downloads.size()) {
            runDownloads();
        } else {
            runConflicts();
        }
    }

    void runDeletes() {
        if (deleteIndex >= plan.remoteDeletes.size()) {
            commitAndFinish();
            return;
        }
        const auto& entry = plan.remoteDeletes.at(deleteIndex);
        ++deleteIndex;
        const QString path = entry.path;
        QObject::connect(client.get(), &webdav::WebDavClient::removeFinished, client.get(),
                         [this, path](const QString& finishedPath,
                                      const webdav::WebDavError& error) {
                             if (finishedPath != path) {
                                 return;
                             }
                             if (error.failed) {
                                 finish(false, QObject::tr("Remote deletion failed for %1: %2")
                                                   .arg(path, error.message));
                                 return;
                             }
                             runDeletes();
                         },
                         Qt::SingleShotConnection);
        client->remove(path);
    }

    void commitAndFinish() {
        stateStore->commit(plan, localSnapshot, remoteSnapshot);
        if (!stateStore->save()) {
            finish(false, QObject::tr("Sync completed but the local sync state could not be "
                                      "saved. The next sync will re-verify all files."));
            return;
        }
        settings.lastSync =
            QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
        settings.lastStatus = QStringLiteral("ok");
        storage::WebDavSyncConfigurationSettings().setSettings(settings);
        finish(true, QObject::tr("WebDAV sync finished successfully"));
    }

    void finish(bool success, const QString& message) {
        running = false;
        client.reset();
        emit service->syncFinished(success, message);
    }

    WebSyncService* service = nullptr;
    bool running = false;
    WebDavSyncSettings settings;
    QString root;
    QString credentials;
    std::unique_ptr<webdav::WebDavClient> client;
    std::unique_ptr<WebSyncStateStore> stateStore;
    SyncScopeSnapshot localSnapshot;
    SyncScopeSnapshot remoteSnapshot;
    SyncScopeSnapshot previous;
    WebSyncPlan plan;
    int uploadIndex = 0;
    int downloadIndex = 0;
    int conflictIndex = 0;
    int deleteIndex = 0;
};

WebSyncService& WebSyncService::instance() {
    static WebSyncService service;
    return service;
}

WebSyncService::WebSyncService() : m_impl(std::make_unique<Impl>(this)) {}

WebSyncService::~WebSyncService() = default;

bool WebSyncService::isRunning() const {
    return m_impl->running;
}

void WebSyncService::runSync(bool force) {
    m_impl->start(force);
}

} // namespace clip_shot::presentation
