#ifndef CLIP_SHOT_WEBDAVSYNCENGINE_H
#define CLIP_SHOT_WEBDAVSYNCENGINE_H

#include "clip_shot/network/webdavclient.h"

#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QString>

#include <functional>

namespace clip_shot::webdav_sync {

// A single synced file descriptor used by snapshots and the state store.
struct SyncEntry {
    qint64 size = 0;
    QDateTime modified;
};

// Local snapshot of one sync scope (config / pinned / capture-history).
struct SyncScopeSnapshot {
    // Relative path (forward slashes, no leading slash) -> entry.
    QMap<QString, SyncEntry> files;
};

struct SyncPlanEntry {
    QString path;
    bool directory = false;
};

struct WebSyncPlan {
    QVector<SyncPlanEntry> uploads;       // Local new/changed files pushed to the server.
    QVector<SyncPlanEntry> downloads;     // Remote files pulled to the local side.
    QVector<SyncPlanEntry> remoteDeletes; // Files removed locally are removed remotely.
    // Local new/changed files that also changed remotely. Local copy is kept,
    // the remote variant is downloaded as "<path>.conflict-<timestamp>".
    QVector<SyncPlanEntry> conflicts;
    int skipped = 0;                      // Identical on both sides.
};

// Pure diff logic: combines the local snapshot, the remote snapshot and the
// previously recorded sync state into an executable plan. Deterministic and
// unit-testable without a server.
//
// Rule summary (two-way mirror with conflict preservation):
//  - present locally only              -> upload
//  - present remotely only             -> download (includes first sync merge)
//  - present in both, same size+mtime  -> skip
//  - present in both, local changed    -> upload (remote older or equal to state)
//  - present in both, remote changed   -> download
//  - present in both, both changed     -> conflict (upload local under the
//                                          original name, download remote as a
//                                          .conflict-<timestamp> sibling)
//  - present in state store only       -> remote delete (local file was removed)
// mtime comparison uses a 2-second tolerance to absorb WebDAV second precision.
WebSyncPlan buildSyncPlan(const SyncScopeSnapshot& local,
                          const SyncScopeSnapshot& remote,
                          const SyncScopeSnapshot& previousState);

// Directory plan helper: deduplicates the directories that must exist on the
// remote side for a set of file paths.
QVector<QString> remoteDirectoriesFor(const QVector<SyncPlanEntry>& uploads);

// Local snapshot collector for a sync scope rooted at rootPath. Only regular
// files are included; directory entries are implied by their children.
// excludedSubPaths are relative directory names (e.g. ".clip-webdav-sync")
// skipped entirely.
SyncScopeSnapshot collectLocalSnapshot(const QString& rootPath,
                                       const QStringList& excludedSubPaths);

// State store persisted under <rootPath>/.clip-webdav-sync/state.json. The
// state file itself is never part of any snapshot.
class WebSyncStateStore {
public:
    explicit WebSyncStateStore(QString rootPath);
    SyncScopeSnapshot load() const;
    // Merges the executed plan into the store: files uploaded/downloaded keep
    // the local entry; remote-only deletes remove the stored entry; conflicts
    // record the local file under its original path.
    void commit(const WebSyncPlan& plan, const SyncScopeSnapshot& local,
                const SyncScopeSnapshot& remote);
    bool save() const;
    QString stateFilePath() const;

private:
    QString m_statePath;
    SyncScopeSnapshot m_state;
};

// Result of one sync run over one scope.
struct WebSyncResult {
    bool success = false;
    QString message;
    int uploaded = 0;
    int downloaded = 0;
    int deleted = 0;
    int conflicted = 0;
    int skipped = 0;
};

} // namespace clip_shot::webdav_sync

#endif // CLIP_SHOT_WEBDAVSYNCENGINE_H
