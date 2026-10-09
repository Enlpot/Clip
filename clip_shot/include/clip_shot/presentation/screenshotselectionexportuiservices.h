#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTSELECTIONEXPORTUISERVICES_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTSELECTIONEXPORTUISERVICES_H

#include "clip_shot/storage/pinnedwindowtypes.h"
#include "clip_shot/storage/storageresult.h"

#include "clip_shot/presentation/screenshotclipboardcontent.h"
#include "clip_shot/presentation/screenshotclipboardservice.h"
#include "clip_shot/presentation/screenshotexportartifact.h"
#include "clip_shot/presentation/screenshotimagesource.h"
#include "clip_shot/presentation/screenshotselectionexportworkflowports.h"

#include <atomic>
#include <QSet>
#include <QHash>
#include <QPointer>
#include <functional>
#include <memory>

class QScreen;
class ScreenshotOcrRecognitionPort;
class ScreenshotQrRecognitionPort;
class ClipShotApiClient;
class ScreenshotPinnedWindowPool;
class ScreenshotPinnedWindow;
class ScreenshotPendingPinCoordinator;
class QTextDocument;
struct ScreenshotPinnedRecognitionProviders;
struct ScreenshotHistoryEntry;

namespace clip_shot::presentation {
class PinnedWindowGroupManager;
class PinnedWindowSelectionController;
} // namespace clip_shot::presentation

class ScreenshotSelectionExportUiServices final : public ScreenshotSelectionExportDestinationPort {
  public:
    explicit ScreenshotSelectionExportUiServices(
        ScreenshotOcrRecognitionPort* recognition = nullptr,
        ScreenshotQrRecognitionPort* qrRecognition = nullptr,
        ClipShotApiClient* tableRecognition = nullptr,
        std::function<void()> showMainWindowRequested = {},
        std::function<ScreenshotPinnedRecognitionProviders()> recognitionProvider = {},
        clip_shot::presentation::PinnedWindowGroupManager* groupManager = nullptr);
    ~ScreenshotSelectionExportUiServices() override;

    [[nodiscard]] bool publishClipboard(QObject* receiver, ScreenshotClipboardPayload payload,
                                        ClipboardCompletion completion) override;
    [[nodiscard]] bool publishClipboard(QObject* receiver, ScreenshotClipboardPayload payload,
                                        ClipboardCompletion completion,
                                        quint64 publicationId) override;
    void cancelClipboardPublication();
    // Prepares one hidden native shell for the next Pin to Screen presentation.
    void prewarmPinnedWindow(QScreen* screen = nullptr);
    // Fits decoded file or drop content on the target screen, retaining its text document.
    [[nodiscard]] bool
    presentDecodedContentOnScreen(ScreenshotClipboardContent content, QScreen* screen,
                                  bool autoResizeWindow,
                                  clip_shot::storage::PinnedWindowCreationSource source);
    // A null image is accepted when imageLoader is provided and
    // initialWindowSize supplies the known canvas dimensions.
    [[nodiscard]] bool presentPinnedImage(
        const QImage& image, QScreen* screen, const QRect& nativeGeometry,
        const QSize& initialWindowSize = {},
        std::shared_ptr<QTextDocument> formattedTextDocument = {},
        const QString& formattedPlainText = {}, qreal formattedTextDevicePixelRatio = 1.0,
        ScreenshotClipboardOriginalContent originalContent = {},
        ScreenshotImageLoader imageLoader = {}, PinnedCompletion completion = {},
        std::optional<clip_shot::storage::PinnedBorderAppearance> borderAppearance = {},
        std::optional<bool> checkerboardEnabled = {},
        clip_shot::storage::PinnedWindowCreationSource source =
            clip_shot::storage::PinnedWindowCreationSource::Other,
        clip_shot::storage::PinnedSourceIdentity sourceIdentity = {},
        std::optional<bool> initialBorderVisible = {});
    // An already composited selection bitmap placed by screenshotSelectionPinRequest.
    [[nodiscard]] bool
    presentCompositedSelectionImage(const QImage& image,
                                    const ScreenshotPinnedSelectionRequest& request,
                                    PinnedCompletion completion = {});
    [[nodiscard]] bool presentPinnedSelection(const ScreenshotPinnedSelectionRequest& request,
                                              ScreenshotPinnedSelectionResultHandle result,
                                              PinnedCompletion completion) override;
    [[nodiscard]] bool presentPinnedDocument(const QImage& background, QScreen* screen,
                                             const QRect& nativeGeometry,
                                             const ScreenshotHistoryEntry& entry,
                                             PinnedCompletion completion);
    [[nodiscard]] bool presentPinnedArtifact(const ScreenshotPinnedSelectionRequest& request,
                                             std::shared_ptr<ScreenshotExportArtifact> artifact,
                                             PinnedCompletion completion = {});
    [[nodiscard]] bool
    presentPinnedImageArtifact(std::shared_ptr<ScreenshotExportArtifact> artifact, QScreen* screen,
                               const QRect& nativeGeometry, const QSize& initialWindowSize,
                               PinnedCompletion completion = {});
    void restorePersistedWindows();
    // Returns whether restoration was queued; completion and failures are asynchronous.
    bool restoreRecord(const QString& id, bool activateGroup = true);
    void restoreLastClosedWindow();
    [[nodiscard]] ScreenshotPinnedWindow*
    findDuplicatePin(const clip_shot::storage::PinnedSourceIdentity& identity) const;
    [[nodiscard]] QSet<QString> duplicateSourceKeys() const;
    // A true result consumes the request. The caller owns one restore guard per action/batch.
    bool handleDuplicatePin(const clip_shot::storage::PinnedSourceIdentity& identity,
                            const QString& action, bool& restored);

    void setRestoreFailureHandler(std::function<void()> handler) {
        m_restoreFailure = std::move(handler);
    }
    void destroyRecords(const QVector<QString>& ids);
    [[nodiscard]] clip_shot::storage::StorageResult tryDestroyRecords(const QVector<QString>& ids);

  private:
    void trackSourceWindow(ScreenshotPinnedWindow* window,
                           const clip_shot::storage::PinnedSourceIdentity& identity);
    [[nodiscard]] bool presentRestoredRecord(clip_shot::storage::PinnedWindowRecord record);
    [[nodiscard]] bool presentPinnedImageOnCanvas(
        const QImage& image, QScreen* screen, const QRect& nativeGeometry,
        const QSize& initialWindowSize, const QRectF& canvasRect,
        std::shared_ptr<QTextDocument> formattedTextDocument, const QString& formattedPlainText,
        qreal formattedTextDevicePixelRatio, ScreenshotClipboardOriginalContent originalContent,
        ScreenshotImageLoader imageLoader, PinnedCompletion completion,
        std::optional<clip_shot::storage::PinnedBorderAppearance> borderAppearance = {},
        std::optional<bool> checkerboardEnabled = {},
        clip_shot::storage::PinnedWindowCreationSource source =
            clip_shot::storage::PinnedWindowCreationSource::Other,
        const ScreenshotHistoryEntry* document = nullptr,
        clip_shot::storage::PinnedSourceIdentity sourceIdentity = {},
        std::optional<bool> initialBorderVisible = {});

    QHash<QString, QList<QPointer<ScreenshotPinnedWindow>>> m_sourceWindows;
    std::function<void()> m_restoreFailure;
    struct RestoringPin {
        clip_shot::storage::PinnedSourceIdentity sourceIdentity;
        QString groupId;
        QDateTime createdUtc;
        bool attentionPending = false;
    };
    QHash<QString, RestoringPin> m_restoringIds;
    std::shared_ptr<std::atomic_bool> m_restoreAlive = std::make_shared<std::atomic_bool>(true);
    ScreenshotOcrRecognitionPort* m_recognition = nullptr;
    ScreenshotQrRecognitionPort* m_qrRecognition = nullptr;
    ClipShotApiClient* m_tableRecognition = nullptr;
    std::function<void()> m_showMainWindowRequested;
    std::function<ScreenshotPinnedRecognitionProviders()> m_recognitionProvider;
    clip_shot::presentation::PinnedWindowGroupManager* m_groupManager = nullptr;
    std::unique_ptr<clip_shot::presentation::PinnedWindowSelectionController> m_selectionController;
    std::unique_ptr<ScreenshotPinnedWindowPool> m_windowPool;
    std::unique_ptr<ScreenshotPendingPinCoordinator> m_pendingPinCoordinator;
    ScreenshotClipboardCommitScope m_clipboardScope;
};

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTSELECTIONEXPORTUISERVICES_H
