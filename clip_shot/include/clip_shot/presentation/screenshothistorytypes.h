#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTHISTORYTYPES_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTHISTORYTYPES_H

#include "clip_shot/presentation/screenshotintelligentselectionmodel.h"
#include "clip_shot/presentation/screenshotselectionparams.h"
#include "clip_shot/storage/capturehistorytypes.h"
#include "clip_shot/storage/preparedpngimage.h"
#include "clip_shot/presentation/screenshotclipboardcontent.h"
#include "clip_shot/presentation/screenshotrecognitionresults.h"

#include <QByteArray>
#include <QDateTime>
#include <QImage>
#include <QRect>
#include <QString>
#include <QVector>

#include <optional>

struct ScreenshotHistoryDisplay {
    QString stableId;
    QString name;
    QImage image;
    std::optional<QPoint> sourceCanvasOrigin{};
    std::optional<QRect> sourceCanvasRect{};
    bool canvasUsesPoints = false;
    qreal backingScale = 0.0;
    quint32 nativeDisplayId = 0;
    QImage cursorPatch{};
    QRect cursorPixelRect{};
};

struct ScreenshotHistoryEntry {
    clip_shot::storage::CaptureHistoryContentKind contentKind =
        clip_shot::storage::CaptureHistoryContentKind::ScreenshotSession;
    QString id;
    QDateTime createdUtc;
    QRect recordedCanvasBounds;
    ScreenshotSelectionParams selection;
    QByteArray canvasHistory;
    QVector<ScreenshotHistoryDisplay> displays;
    std::optional<QImage> resultImage;
    std::optional<clip_shot::storage::PreparedPngImage> preparedResultImage;
    clip_shot::storage::CaptureHistorySource source =
        clip_shot::storage::CaptureHistorySource::CopiedToClipboard;
    // Absent on entries restored from records persisted before the scrolling marker existed.
    std::optional<bool> scrolling{};
    std::optional<clip_shot::storage::CaptureHistoryDesktopGeometry> desktopGeometry{};
    bool cursorVisible = false;
    bool cursorAvailable = false;
    bool intelligentSelectionMode = false;
    bool persistent = true;
    std::optional<ScreenshotIntelligentSelectionModel> liveIntelligentSelection;
    // Transient session handoff state; historical on-disk records remain unchanged.
    QByteArray documentSession;
    ScreenshotClipboardOriginalContent originalContent;
    ScreenshotRecognitionResults recognitionResults;
    QString tool;
};

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTHISTORYTYPES_H
