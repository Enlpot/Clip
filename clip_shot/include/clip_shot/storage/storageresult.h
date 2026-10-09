#ifndef CLIP_SHOT_STORAGE_STORAGERESULT_H
#define CLIP_SHOT_STORAGE_STORAGERESULT_H

#include <QString>

#include <utility>

namespace clip_shot::storage {
struct StorageResult {
    bool success = false;
    QString error;

    [[nodiscard]] static StorageResult ok() {
        return {true, {}};
    }
    [[nodiscard]] static StorageResult failure(QString message) {
        return {false, std::move(message)};
    }
};
} // namespace clip_shot::storage

#endif // CLIP_SHOT_STORAGE_STORAGERESULT_H
