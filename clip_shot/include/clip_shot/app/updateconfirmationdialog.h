#ifndef CLIP_SHOT_APP_UPDATECONFIRMATIONDIALOG_H
#define CLIP_SHOT_APP_UPDATECONFIRMATIONDIALOG_H

class QWidget;

namespace clip_shot::app {
[[nodiscard]] bool confirmRestartAndUpdate(QWidget* owner);
}

#endif // CLIP_SHOT_APP_UPDATECONFIRMATIONDIALOG_H
