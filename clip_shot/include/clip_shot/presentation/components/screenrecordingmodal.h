#ifndef CLIP_SHOT_PRESENTATION_COMPONENTS_SCREENRECORDINGMODAL_H
#define CLIP_SHOT_PRESENTATION_COMPONENTS_SCREENRECORDINGMODAL_H

#include "widgets/modal.h"

namespace clip_shot::presentation {

// The recording area owns placement and native stacking while it exists.
// A render job can outlive that area and then use a detached application modal.
inline void configureScreenRecordingModal(adqt::widgets::AdModal& modal, QWidget* owner) {
    modal.setWindowModeDetached(owner == nullptr);
    modal.setOwnerWindow(owner);
    modal.setMode(adqt::widgets::AdModal::Mode::Window);
    modal.setWindowModality(Qt::ApplicationModal);
    modal.setWindowAlwaysOnTop(owner == nullptr);
    modal.setCentered(true);
    modal.setMaskVisible(false);
    modal.setCloseOnMaskClick(false);
}

} // namespace clip_shot::presentation

#endif
