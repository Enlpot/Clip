#ifndef CLIP_SHOT_PRESENTATION_COMPONENTS_THUMBNAILCACHE_H
#define CLIP_SHOT_PRESENTATION_COMPONENTS_THUMBNAILCACHE_H

#include <QImage>
#include <QSize>
#include <QString>

namespace clip_shot::presentation::components::thumbnail_cache {

struct CachedThumbnail final {
    QImage image;
    QSize naturalSize;
};

[[nodiscard]] QString pathForKey(const QString& key, const QString& directory = {});
[[nodiscard]] CachedThumbnail load(const QString& path);
void persist(const QString& path, QImage image, QSize naturalSize = {});

} // namespace clip_shot::presentation::components::thumbnail_cache

#endif // CLIP_SHOT_PRESENTATION_COMPONENTS_THUMBNAILCACHE_H
