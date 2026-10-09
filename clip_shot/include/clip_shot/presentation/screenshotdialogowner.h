#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTDIALOGOWNER_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTDIALOGOWNER_H

#include "clip_shot/presentation/screenshotdisplaysession.h"
#include "clip_shot/presentation/screenshotgeometry.h"

[[nodiscard]] inline ScreenshotOverlayWindow*
screenshotSelectionDialogOwner(const ScreenshotDisplaySession& displays,
                               const ScreenshotGeometryMapper& geometry, const QRectF& selection,
                               ScreenshotOverlayWindow* keyboardOwner) {
    if (!selection.isEmpty()) {
        const auto* display = geometry.displayForCanvasRect(displays, selection);
        if (auto* overlay = displays.overlayForDisplay(display)) {
            return overlay;
        }
    }
    return keyboardOwner;
}

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTDIALOGOWNER_H
