#ifndef CLIP_SHOT_NATIVEPRINTBACKEND_H
#define CLIP_SHOT_NATIVEPRINTBACKEND_H

#include "clip_shot/presentation/screenshotprintservice.h"

[[nodiscard]] ScreenshotPrintService::Backend screenshotNativePrintBackend(bool legacy);
#ifdef Q_OS_WIN
// Compatibility alias for the Windows legacy Photo Printing Wizard backend.
[[nodiscard]] ScreenshotPrintService::Backend screenshotClassicWindowsPrintBackend();
#endif

#endif
