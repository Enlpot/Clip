#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTFILEPINBATCH_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTFILEPINBATCH_H

#include "clip_shot/presentation/screenshotclipboardcontent.h"
#include "clip_shot/presentation/screenshotexportcoordinator.h"
#include "clip_shot/platform/selectedfiles.h"
#include <QHash>
#include <QSet>
#include <QObject>
#include <QStringList>

#include <functional>
#include <memory>

namespace clip_shot::platform {
class SelectedFileBackend;
struct SelectedFileTarget;
} // namespace clip_shot::platform

class ScreenshotFilePinBatch final : public QObject {
  public:
    using Present = std::function<bool(ScreenshotClipboardContent)>;
    using Source =
        std::function<clip_shot::platform::SelectedFileResult(const ScreenshotExportCancellation&)>;
    using Failure = std::function<void(clip_shot::platform::SelectedFileError)>;

    struct DuplicateFilter {
        QSet<QString> identities;
        // Runs on the GUI thread; false means the match disappeared and must be decoded.
        std::function<bool(const clip_shot::storage::PinnedSourceIdentity&)> consume;
    };
    explicit ScreenshotFilePinBatch(QObject* parent = nullptr);
    ~ScreenshotFilePinBatch() override;
    void start(QStringList paths, Present present, DuplicateFilter filter = {});
    void startSelection(std::shared_ptr<clip_shot::platform::SelectedFileBackend> backend,
                        clip_shot::platform::SelectedFileTarget target, Present present,
                        Failure failure = {}, DuplicateFilter filter = {});
    void cancel();
    [[nodiscard]] bool active() const {
        return m_active;
    }

  private:
    void startSource(Source source, Present present, Failure failure = {},
                     DuplicateFilter filter = {});
    void submitDecode(quint64 generation, qsizetype index);
    void submitPendingDecodes(quint64 generation);
    void dispatch(quint64 generation);

    ScreenshotExportJobHandle m_snapshotJob;
    QHash<qsizetype, ScreenshotExportJobHandle> m_decodeJobs;
    quint64 m_generation = 0;
    QList<ScreenshotClipboardLocalImage> m_files;
    // Completed decodes keyed by file index; out-of-order completions wait in
    // their slot so presentation always follows source order.
    QHash<qsizetype, std::optional<ScreenshotClipboardContent>> m_ready;
    qsizetype m_nextSubmit = 0;
    qsizetype m_nextPresent = 0;
    Present m_present;
    DuplicateFilter m_duplicateFilter;
    QSet<qsizetype> m_skippedDuplicates;
    bool m_active = false;
    bool m_dispatching = false;
    bool m_redispatch = false;
};
#endif
