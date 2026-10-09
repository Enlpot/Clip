#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTHISTORYSERVICE_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTHISTORYSERVICE_H

#include "clip_shot/presentation/screenshothistorytypes.h"
#include "clip_shot/storage/capturehistoryrepository.h"

#include <QObject>
#include <QSet>
#include <QString>
#include <QVector>

#include <functional>
#include <future>
#include <optional>

class ScreenshotDisplaySession;
class ScreenshotInteractionState;
class ScreenshotHistoryValidationQueue;
class ScreenshotSelectionModel;
class ClipCanvasRuntime;

struct ScreenshotHistoryServiceContext {
    ScreenshotDisplaySession& displays;
    ClipCanvasRuntime& runtime;
    ScreenshotSelectionModel& selection;
    ScreenshotInteractionState& interaction;
    ScreenshotIntelligentSelectionModel& intelligentSelection;
    std::function<void()> presentationChanged = []() {};
    std::function<void(bool)> loadingStateChanged = [](bool) {};
    std::function<void()> intelligentSelectionRequested = []() {};
    std::function<void(const ScreenshotSelectionParams&)> selectionCommitted =
        [](const ScreenshotSelectionParams&) {};
};

class ScreenshotHistoryService final : public QObject {
  public:
    using Clock = std::function<QDateTime()>;

    explicit ScreenshotHistoryService(ScreenshotHistoryServiceContext context,
                                      QString storageRoot = {}, Clock clock = {});
    ScreenshotHistoryService(ScreenshotHistoryServiceContext context,
                             clip_shot::storage::CaptureHistoryRepository& repository,
                             Clock clock = {});
    ~ScreenshotHistoryService();

    [[nodiscard]] std::optional<ScreenshotHistoryEntry> snapshotCurrent(bool persistent) const;
    void commit(ScreenshotHistoryEntry entry);
    [[nodiscard]] bool navigateToRecord(const QString& recordId);
    [[nodiscard]] bool navigatePrevious();
    [[nodiscard]] bool navigateNext();
    [[nodiscard]] bool returnToCurrentScreenshot();
    [[nodiscard]] bool navigationInProgress() const;
    void resetCaptureNavigation();
    void drainPendingWrites();
    void refreshMetadata();
    [[nodiscard]] bool presentTransientEntry(const ScreenshotHistoryEntry& entry);

  private:
    friend class ScreenshotHistoryServiceTestAccess;

    struct PendingWrite {
        QString entryId;
        std::shared_future<clip_shot::storage::CaptureHistoryPublishResult> result;
    };

    [[nodiscard]] QRect currentCanvasBounds() const;
    [[nodiscard]] bool ensureLiveEndpoint();
    [[nodiscard]] bool navigateTo(int index);
    [[nodiscard]] bool applyEntry(const ScreenshotHistoryEntry& entry);
    void finishPersistentNavigation(quint64 generation, int index, const QString& entryId,
                                    std::optional<ScreenshotHistoryEntry> entry);
    void reapCompletedLoads();
    void drainPendingLoads();
    void initializeRepository();
    void reapCompletedWrites();
    void scheduleWrite(ScreenshotHistoryEntry entry);
    [[nodiscard]] std::shared_future<clip_shot::storage::CaptureHistoryPublishResult>
    pendingWriteFor(const QString& entryId) const;

    ScreenshotHistoryServiceContext m_context;
    std::unique_ptr<clip_shot::storage::CaptureHistoryRepository> m_ownedRepository;
    clip_shot::storage::CaptureHistoryRepository* m_repository = nullptr;
    std::unique_ptr<ScreenshotHistoryValidationQueue> m_validationQueue;
    Clock m_clock;
    QVector<clip_shot::storage::CaptureHistoryRecord> m_entries;
    QSet<QString> m_unreadableEntries;
    std::optional<ScreenshotHistoryEntry> m_liveEndpoint;
    int m_navigationIndex = -1;
    quint64 m_navigationGeneration = 0;
    bool m_navigationInProgress = false;
    std::vector<std::future<void>> m_pendingLoads;
    std::vector<PendingWrite> m_pendingWrites;
};

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTHISTORYSERVICE_H
