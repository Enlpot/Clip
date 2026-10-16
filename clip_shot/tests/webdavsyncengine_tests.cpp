#include "clip_shot/storage/webdavsyncengine.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <iostream>
#include <stdexcept>

namespace {
using clip_shot::webdav_sync::buildSyncPlan;
using clip_shot::webdav_sync::collectLocalSnapshot;
using clip_shot::webdav_sync::SyncEntry;
using clip_shot::webdav_sync::SyncScopeSnapshot;
using clip_shot::webdav_sync::WebSyncStateStore;

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

SyncEntry entry(qint64 size, const QDateTime& modified) {
    return SyncEntry{size, modified};
}

QDateTime mtime(int dayOffset, int secondOffset) {
    // Deterministic timestamps independent of the current clock.
    return QDateTime(QDate(2026, 10, 1).addDays(dayOffset),
                     QTime(12, 0).addSecs(secondOffset));
}

bool paths(const QVector<clip_shot::webdav_sync::SyncPlanEntry>& entries,
           const QString& path) {
    for (const auto& entry : entries) {
        if (entry.path == path) {
            return true;
        }
    }
    return false;
}

void testUploadsNewLocalFiles() {
    SyncScopeSnapshot local;
    local.files.insert(QStringLiteral("config.json"), entry(100, mtime(1, 0)));
    const SyncScopeSnapshot remote;
    const SyncScopeSnapshot previous;
    const auto plan = buildSyncPlan(local, remote, previous);
    require(plan.uploads.size() == 1, "a new local file should upload");
    require(paths(plan.uploads, QStringLiteral("config.json")),
            "config.json should be in the upload plan");
    require(plan.downloads.isEmpty() && plan.remoteDeletes.isEmpty(),
            "no downloads or deletes expected");
}

void testDownloadsRemoteOnlyOnFirstSync() {
    const SyncScopeSnapshot local;
    SyncScopeSnapshot remote;
    remote.files.insert(QStringLiteral("capture_history/index.json"),
                        entry(50, mtime(2, 5)));
    const SyncScopeSnapshot previous;
    const auto plan = buildSyncPlan(local, remote, previous);
    require(plan.downloads.size() == 1, "remote-only file without state should download");
    require(paths(plan.downloads, QStringLiteral("capture_history/index.json")),
            "index.json should be in the download plan");
}

void testRemoteDeleteWhenLocalRemoved() {
    const SyncScopeSnapshot local;
    SyncScopeSnapshot remote;
    remote.files.insert(QStringLiteral("pinned_windows_v2/index.json"),
                        entry(80, mtime(3, 0)));
    SyncScopeSnapshot previous;
    previous.files.insert(QStringLiteral("pinned_windows_v2/index.json"),
                          entry(80, mtime(3, 0)));
    const auto plan = buildSyncPlan(local, remote, previous);
    require(plan.remoteDeletes.size() == 1,
            "a synced file removed locally should delete remotely");
    require(paths(plan.remoteDeletes, QStringLiteral("pinned_windows_v2/index.json")),
            "pinned index should be in the delete plan");
}

void testSkipsIdenticalFiles() {
    SyncScopeSnapshot local;
    local.files.insert(QStringLiteral("config.json"), entry(100, mtime(1, 0)));
    SyncScopeSnapshot remote;
    remote.files.insert(QStringLiteral("config.json"), entry(100, mtime(1, 2)));
    SyncScopeSnapshot previous;
    previous.files.insert(QStringLiteral("config.json"), entry(100, mtime(1, 0)));
    const auto plan = buildSyncPlan(local, remote, previous);
    require(plan.uploads.isEmpty() && plan.downloads.isEmpty() &&
                plan.remoteDeletes.isEmpty() && plan.conflicts.isEmpty(),
            "identical files should be skipped");
    require(plan.skipped == 1, "one skip should be counted");
}

void testUploadsLocalChange() {
    SyncScopeSnapshot local;
    local.files.insert(QStringLiteral("config.json"), entry(120, mtime(2, 0)));
    SyncScopeSnapshot remote;
    remote.files.insert(QStringLiteral("config.json"), entry(100, mtime(1, 0)));
    SyncScopeSnapshot previous;
    previous.files.insert(QStringLiteral("config.json"), entry(100, mtime(1, 0)));
    const auto plan = buildSyncPlan(local, remote, previous);
    require(paths(plan.uploads, QStringLiteral("config.json")),
            "local change should upload");
    require(plan.downloads.isEmpty(), "no download when local changed");
}

void testDownloadsRemoteChange() {
    SyncScopeSnapshot local;
    local.files.insert(QStringLiteral("config.json"), entry(100, mtime(1, 0)));
    SyncScopeSnapshot remote;
    remote.files.insert(QStringLiteral("config.json"), entry(130, mtime(3, 0)));
    SyncScopeSnapshot previous;
    previous.files.insert(QStringLiteral("config.json"), entry(100, mtime(1, 0)));
    const auto plan = buildSyncPlan(local, remote, previous);
    require(paths(plan.downloads, QStringLiteral("config.json")),
            "remote change should download");
    require(plan.uploads.isEmpty(), "no upload when remote changed");
}

void testConflictWhenBothChanged() {
    SyncScopeSnapshot local;
    local.files.insert(QStringLiteral("config.json"), entry(120, mtime(4, 0)));
    SyncScopeSnapshot remote;
    remote.files.insert(QStringLiteral("config.json"), entry(130, mtime(5, 0)));
    SyncScopeSnapshot previous;
    previous.files.insert(QStringLiteral("config.json"), entry(100, mtime(1, 0)));
    const auto plan = buildSyncPlan(local, remote, previous);
    require(paths(plan.uploads, QStringLiteral("config.json")),
            "conflict keeps the local file uploaded");
    require(paths(plan.conflicts, QStringLiteral("config.json")),
            "conflict records the remote sibling download");
    require(plan.downloads.isEmpty(),
            "conflict downloads are tracked separately");
}

void testCollectLocalSnapshotExcludesStateDirectory() {
    QTemporaryDir directory;
    require(directory.isValid(), "temporary directory should exist");
    const QString root = directory.path();
    require(QDir().mkpath(root + u"/capture_history/records/abc"),
            "nested directory should be created");
    require(QDir().mkpath(root + u"/.clip-webdav-sync"),
            "state directory should be created");
    QFile file(root + u"/config.json");
    require(file.open(QIODevice::WriteOnly), "config file should open");
    file.write("{}");
    file.close();
    QFile nested(root + u"/capture_history/records/abc/display_0.png");
    require(nested.open(QIODevice::WriteOnly), "nested file should open");
    nested.write("png");
    nested.close();
    QFile state(root + u"/.clip-webdav-sync/state.json");
    require(state.open(QIODevice::WriteOnly), "state file should open");
    state.write("{}");
    state.close();

    const auto snapshot =
        collectLocalSnapshot(root, {QStringLiteral(".clip-webdav-sync")});
    require(snapshot.files.contains(QStringLiteral("config.json")),
            "config.json should be collected");
    require(snapshot.files.contains(
                QStringLiteral("capture_history/records/abc/display_0.png")),
            "nested history file should be collected");
    require(!snapshot.files.contains(QStringLiteral(".clip-webdav-sync/state.json")),
            "the state directory should be excluded");
}

void testStateStoreRoundTrip() {
    QTemporaryDir directory;
    require(directory.isValid(), "temporary directory should exist");
    WebSyncStateStore store(directory.path());
    SyncScopeSnapshot local;
    local.files.insert(QStringLiteral("config.json"), entry(100, mtime(1, 0)));
    WebSyncPlan plan;
    plan.uploads.append({QStringLiteral("config.json"), false});
    store.commit(plan, local, {});
    require(store.save(), "state should save");

    WebSyncStateStore reload(directory.path());
    const auto loaded = reload.load();
    require(loaded.files.contains(QStringLiteral("config.json")),
            "reloaded state should contain the committed file");
    require(loaded.files.value(QStringLiteral("config.json")).size == 100,
            "reloaded size should match");
}

void testStateStoreDeleteRemovesEntry() {
    QTemporaryDir directory;
    require(directory.isValid(), "temporary directory should exist");
    WebSyncStateStore store(directory.path());
    SyncScopeSnapshot local;
    local.files.insert(QStringLiteral("capture_history/index.json"),
                       entry(50, mtime(2, 0)));
    WebSyncPlan uploadPlan;
    uploadPlan.uploads.append({QStringLiteral("capture_history/index.json"), false});
    store.commit(uploadPlan, local, {});
    require(store.save(), "state should save after upload");

    WebSyncPlan deletePlan;
    deletePlan.remoteDeletes.append(
        {QStringLiteral("capture_history/index.json"), false});
    store.commit(deletePlan, {}, {});
    require(store.save(), "state should save after delete");
    const auto loaded = store.load();
    require(!loaded.files.contains(QStringLiteral("capture_history/index.json")),
            "deleted entry should be removed from state");
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        testUploadsNewLocalFiles();
        testDownloadsRemoteOnlyOnFirstSync();
        testRemoteDeleteWhenLocalRemoved();
        testSkipsIdenticalFiles();
        testUploadsLocalChange();
        testDownloadsRemoteChange();
        testConflictWhenBothChanged();
        testCollectLocalSnapshotExcludesStateDirectory();
        testStateStoreRoundTrip();
        testStateStoreDeleteRemovesEntry();
    } catch (const std::exception& error) {
        std::cerr << "webdav sync engine test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "webdav sync engine tests passed\n";
    return 0;
}
