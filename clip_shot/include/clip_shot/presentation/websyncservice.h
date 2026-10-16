#ifndef CLIP_SHOT_PRESENTATION_WEBSYNCSERVICE_H
#define CLIP_SHOT_PRESENTATION_WEBSYNCSERVICE_H

#include "clip_shot/webdavsyncsettings.h"

#include <QObject>

#include <memory>

namespace clip_shot::presentation {

// Runs the WebDAV sync pipeline (settings + pinned windows + optional capture
// history) against the configured server. Used by both the manual "Sync now"
// button and the automatic sync after a screenshot is stored.
class WebSyncService final : public QObject {
    Q_OBJECT

  public:
    static WebSyncService& instance();

    [[nodiscard]] bool isRunning() const;

    // force=true runs even when the sync is disabled in settings (manual
    // button); force=false requires webdav_sync/enabled (screenshot trigger).
    void runSync(bool force);

  signals:
    void syncStatusChanged(const QString& status);
    void syncFinished(bool success, const QString& message);

  private:
    WebSyncService();
    ~WebSyncService() override;
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace clip_shot::presentation

#endif // CLIP_SHOT_PRESENTATION_WEBSYNCSERVICE_H
