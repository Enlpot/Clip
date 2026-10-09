#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTSELECTORPOLICY_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTSELECTORPOLICY_H

#include "clip_shot/presentation/screenshotselectorworkflowports.h"

#include "clip_ui_selector.h"

#include <QByteArray>

struct ScreenshotSelectorLookupPolicy {
#ifdef Q_OS_MACOS
    ClipUiSelectorBackend backend = CLIP_UI_SELECTOR_BACKEND_ACCESSIBILITY;
#else
    ClipUiSelectorBackend backend = CLIP_UI_SELECTOR_BACKEND_UIA;
#endif
    ClipUiSelectorHitTestMode mode = CLIP_UI_SELECTOR_HIT_TEST_MODE_UI_ELEMENT;
};

inline ScreenshotSelectorLookupPolicy
screenshotSelectorLookupPolicy(bool smartSelectionEnabled, const QByteArray& configuredBackend) {
#ifdef Q_OS_MACOS
    Q_UNUSED(configuredBackend);
    return {CLIP_UI_SELECTOR_BACKEND_ACCESSIBILITY, smartSelectionEnabled
                                                        ? CLIP_UI_SELECTOR_HIT_TEST_MODE_UI_ELEMENT
                                                        : CLIP_UI_SELECTOR_HIT_TEST_MODE_WINDOW};
#else
    const QByteArray backend = configuredBackend.trimmed().toLower();
    ClipUiSelectorBackend selectedBackend = CLIP_UI_SELECTOR_BACKEND_UIA;
    if (backend == "msaa") {
        selectedBackend = CLIP_UI_SELECTOR_BACKEND_MSAA;
    }
    return {selectedBackend, smartSelectionEnabled ? CLIP_UI_SELECTOR_HIT_TEST_MODE_UI_ELEMENT
                                                   : CLIP_UI_SELECTOR_HIT_TEST_MODE_WINDOW};
#endif
}

inline ClipUiSelectorHitTestMode
screenshotSelectorHitTestMode(bool smartSelectionEnabled,
                              ScreenshotSelectorHitTestMode requestedMode) {
    return !smartSelectionEnabled || requestedMode == ScreenshotSelectorHitTestMode::Window
               ? CLIP_UI_SELECTOR_HIT_TEST_MODE_WINDOW
               : CLIP_UI_SELECTOR_HIT_TEST_MODE_UI_ELEMENT;
}

#endif // CLIP_SHOT_PRESENTATION_SCREENSHOTSELECTORPOLICY_H
