#ifndef CLIP_SHOT_STORAGE_STORAGEDIRECTORYUTILS_P_H
#define CLIP_SHOT_STORAGE_STORAGEDIRECTORYUTILS_P_H

#include <QDir>
#include <QFileInfo>

namespace clip_shot::storage {

inline bool storageLink(const QFileInfo& info) {
    return info.isSymLink() || info.isJunction();
}

// Recursive on-disk size of a file or directory in bytes.  Symlinks are
// skipped so relocated stores are not double-counted.
inline qint64 directoryBytes(const QString& path) {
    const QFileInfo root(path);
    if (!root.exists() || storageLink(root)) {
        return 0;
    }
    if (root.isFile() && !root.isSymLink()) {
        return root.size();
    }
    qint64 total = 0;
    const QFileInfoList entries = QDir(path).entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    for (const QFileInfo& entry : entries) {
        if (storageLink(entry)) {
            continue;
        }
        total += entry.isDir() ? directoryBytes(entry.absoluteFilePath()) : entry.size();
    }
    return total;
}

} // namespace clip_shot::storage

#endif // CLIP_SHOT_STORAGE_STORAGEDIRECTORYUTILS_P_H
