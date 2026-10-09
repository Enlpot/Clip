#ifndef CLIP_SHOT_PRESENTATION_SCREENRECORDINGSHORTCUTCONTROLLER_H
#define CLIP_SHOT_PRESENTATION_SCREENRECORDINGSHORTCUTCONTROLLER_H

#include "clip_shot/presentation/windowshortcutmanager.h"

#include <QMap>
#include <QPointer>

class ScreenRecordingAreaWindow;
class ScreenRecordingToolbarWindow;

class ScreenRecordingShortcutController final : public QObject {
  public:
    ScreenRecordingShortcutController(ScreenRecordingAreaWindow& area,
                                      ScreenRecordingToolbarWindow& toolbar,
                                      QObject* parent = nullptr);

  private:
    using ShortcutManager = clip_shot::presentation::WindowShortcutManager;
    bool canActivate(const ShortcutManager::ActivationContext& context) const;
    void reloadConfiguredShortcuts();

    QPointer<ScreenRecordingAreaWindow> m_area;
    QPointer<ScreenRecordingToolbarWindow> m_toolbar;
    ShortcutManager m_shortcutManager;
    QMap<QString, ShortcutManager::BindingHandle> m_recordingBindings;
    QMap<QString, ShortcutManager::BindingHandle> m_drawingBindings;
};

#endif // CLIP_SHOT_PRESENTATION_SCREENRECORDINGSHORTCUTCONTROLLER_H
