#ifndef CLIP_SHOT_PLATFORM_FOCUSEDFULLSCREENWINDOW_H
#define CLIP_SHOT_PLATFORM_FOCUSEDFULLSCREENWINDOW_H

#include <QRectF>
#include <QVector>

class QScreen;

namespace clip_shot::platform {

struct FocusedWindowSnapshot {
    qint64 ownerProcessId = 0;
    int layer = 0;
    qreal alpha = 0.0;
    QRectF bounds;
};

[[nodiscard]] bool focusedWindowCoversDisplay(qint64 frontmostProcessId,
                                              const QVector<FocusedWindowSnapshot>& orderedWindows,
                                              const QVector<QRectF>& displayBounds);

// Permission-free best-effort check used only to suppress configured global shortcuts.
[[nodiscard]] bool focusedFullscreenWindowExists();
[[nodiscard]] bool focusedFullscreenWindowOnScreen(QScreen* screen);

} // namespace clip_shot::platform

#endif // CLIP_SHOT_PLATFORM_FOCUSEDFULLSCREENWINDOW_H
