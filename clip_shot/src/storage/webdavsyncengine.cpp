#include "clip_shot/storage/webdavsyncengine.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>

namespace clip_shot::webdav_sync {
namespace {

constexpr qint64 kMtimeToleranceSeconds = 2;
const QString kStateDirectory = QStringLiteral(".clip-webdav-sync");
const QString kStateFileName = QStringLiteral("state.json");

bool sameModified(const QDateTime& left, const QDateTime& right) {
    return left.secsTo(right) <= kMtimeToleranceSeconds &&
           right.secsTo(left) <= kMtimeToleranceSeconds;
}

bool newerThan(const QDateTime& left, const QDateTime& right) {
    // left is considered newer when it exceeds right beyond the tolerance.
    return left.secsTo(right) < -kMtimeToleranceSeconds;
}

SyncEntry toSyncEntry(const QFileInfo& info) {
    return SyncEntry{info.size(), info.lastModified()};
}

QJsonObject entryJson(const SyncEntry& entry) {
    return {{QStringLiteral("size"), entry.size},
            {QStringLiteral("modified"),
             entry.modified.toString(Qt::ISODateWithMs)}};
}

bool entryFromJson(const QJsonObject& object, SyncEntry* entry) {
    if (!object.value(QStringLiteral("size")).isDouble()) {
        return false;
    }
    const QDateTime modified =
        QDateTime::fromString(object.value(QStringLiteral("modified")).toString(),
                              Qt::ISODateWithMs);
    if (!modified.isValid()) {
        return false;
    }
    *entry = SyncEntry{object.value(QStringLiteral("size")).toInteger(), modified};
    return true;
}

} // namespace

WebSyncPlan buildSyncPlan(const SyncScopeSnapshot& local,
                          const SyncScopeSnapshot& remote,
                          const SyncScopeSnapshot& previousState) {
    WebSyncPlan plan;
    QSet<QString> seen;
    const auto consider = [&](const QString& path) {
        if (seen.contains(path)) {
            return;
        }
        seen.insert(path);
        const auto localIt = local.files.constFind(path);
        const auto remoteIt = remote.files.constFind(path);
        const auto stateIt = previousState.files.constFind(path);
        const bool hasLocal = localIt != local.files.cend();
        const bool hasRemote = remoteIt != remote.files.cend();
        const bool hasState = stateIt != previousState.files.cend();

        if (hasLocal && !hasRemote) {
            // New or restored locally; push to the server.
            plan.uploads.append({path, false});
            return;
        }
        if (!hasLocal && hasRemote) {
            if (hasState) {
                // The local file was removed after it had been synced: mirror
                // the removal to the remote side.
                plan.remoteDeletes.append({path, false});
            } else {
                plan.downloads.append({path, false});
            }
            return;
        }
        if (!hasLocal || !hasRemote) {
            return;
        }
        const SyncEntry& localEntry = localIt.value();
        const SyncEntry& remoteEntry = remoteIt.value();
        if (localEntry.size == remoteEntry.size &&
            sameModified(localEntry.modified, remoteEntry.modified)) {
            plan.skipped++;
            return;
        }
        const bool localChanged = !hasState ||
                                  !sameModified(localEntry.modified,
                                                stateIt.value().modified) ||
                                  localEntry.size != stateIt.value().size;
        const bool remoteChanged = !hasState ||
                                   !sameModified(remoteEntry.modified,
                                                 stateIt.value().modified) ||
                                   remoteEntry.size != stateIt.value().size;
        if (localChanged && remoteChanged) {
            // Both sides changed since the last sync: keep the local copy under
            // its name and pull the remote variant as a conflict sibling.
            plan.uploads.append({path, false});
            plan.conflicts.append({path, false});
            return;
        }
        if (localChanged) {
            plan.uploads.append({path, false});
            return;
        }
        if (remoteChanged) {
            plan.downloads.append({path, false});
            return;
        }
        plan.skipped++;
    };

    for (auto it = local.files.cbegin(); it != local.files.cend(); ++it) {
        consider(it.key());
    }
    for (auto it = remote.files.cbegin(); it != remote.files.cend(); ++it) {
        consider(it.key());
    }
    for (auto it = previousState.files.cbegin(); it != previousState.files.cend(); ++it) {
        consider(it.key());
    }
    return plan;
}

QVector<QString> remoteDirectoriesFor(const QVector<SyncPlanEntry>& uploads) {
    QSet<QString> directories;
    for (const auto& entry : uploads) {
        const int slash = entry.path.lastIndexOf(u'/');
        if (slash <= 0) {
            continue;
        }
        const QString parent = entry.path.left(slash);
        QString current = parent;
        for (;;) {
            directories.insert(current);
            const int next = current.lastIndexOf(u'/');
            if (next <= 0) {
                break;
            }
            current = current.left(next);
        }
    }
    QVector<QString> result;
    for (const QString& directory : directories) {
        result.append(directory);
    }
    std::sort(result.begin(), result.end(),
              [](const QString& left, const QString& right) {
                  return left.size() < right.size();
              });
    return result;
}

SyncScopeSnapshot collectLocalSnapshot(const QString& rootPath,
                                       const QStringList& excludedSubPaths) {
    SyncScopeSnapshot snapshot;
    QDir root(rootPath);
    if (!root.exists()) {
        return snapshot;
    }
    const QStringList excluded = [&] {
        QStringList result;
        for (const QString& path : excludedSubPaths) {
            result.append(QDir::cleanPath(path));
        }
        return result;
    }();
    QDirIterator iterator(rootPath, QDir::Files | QDir::NoDotAndDotDot,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        const QString relative = root.relativeFilePath(iterator.filePath());
        bool excludedMatch = false;
        for (const QString& excludedPath : excluded) {
            if (relative == excludedPath ||
                relative.startsWith(excludedPath + u'/')) {
                excludedMatch = true;
                break;
            }
        }
        if (excludedMatch) {
            continue;
        }
        const QFileInfo info = iterator.fileInfo();
        snapshot.files.insert(QDir::fromNativeSeparators(relative),
                              toSyncEntry(info));
    }
    return snapshot;
}

WebSyncStateStore::WebSyncStateStore(QString rootPath) {
    m_statePath = QDir(QDir::cleanPath(rootPath))
                      .filePath(kStateDirectory + u'/' + kStateFileName);
}

SyncScopeSnapshot WebSyncStateStore::load() const {
    SyncScopeSnapshot snapshot;
    QFile file(m_statePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return snapshot;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return snapshot;
    }
    const QJsonObject root = document.object();
    const QJsonObject files = root.value(QStringLiteral("files")).toObject();
    for (auto it = files.constBegin(); it != files.constEnd(); ++it) {
        SyncEntry entry;
        if (entryFromJson(it.value().toObject(), &entry)) {
            snapshot.files.insert(it.key(), entry);
        }
    }
    return snapshot;
}

void WebSyncStateStore::commit(const WebSyncPlan& plan,
                               const SyncScopeSnapshot& local,
                               const SyncScopeSnapshot& remote) {
    // Files kept locally keep their local entry; files pulled from the remote
    // side also become local files after the download.
    for (const auto& entry : plan.uploads) {
        const auto it = local.files.constFind(entry.path);
        if (it != local.files.cend()) {
            m_state.files.insert(entry.path, it.value());
        }
    }
    for (const auto& entry : plan.downloads) {
        const auto it = remote.files.constFind(entry.path);
        if (it != remote.files.cend()) {
            m_state.files.insert(entry.path, it.value());
        }
    }
    for (const auto& entry : plan.conflicts) {
        // The local copy remains under the original path and becomes the
        // authoritative entry; the .conflict sibling is a fresh remote file.
        const auto it = local.files.constFind(entry.path);
        if (it != local.files.cend()) {
            m_state.files.insert(entry.path, it.value());
        }
    }
    for (const auto& entry : plan.remoteDeletes) {
        m_state.files.remove(entry.path);
    }
}

bool WebSyncStateStore::save() const {
    QJsonObject files;
    for (auto it = m_state.files.cbegin(); it != m_state.files.cend(); ++it) {
        files.insert(it.key(), entryJson(it.value()));
    }
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("files"), files);
    const QDir directory(QFileInfo(m_statePath).absolutePath());
    if (!directory.mkpath(QStringLiteral("."))) {
        return false;
    }
    QSaveFile file(m_statePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return file.commit();
}

QString WebSyncStateStore::stateFilePath() const {
    return m_statePath;
}

} // namespace clip_shot::webdav_sync
