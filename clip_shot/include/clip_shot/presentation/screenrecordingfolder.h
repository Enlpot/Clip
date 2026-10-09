#ifndef CLIP_SHOT_PRESENTATION_SCREENRECORDINGFOLDER_H
#define CLIP_SHOT_PRESENTATION_SCREENRECORDINGFOLDER_H

#include <QStringList>

namespace clip_shot::presentation::recording {
[[nodiscard]] QStringList screenRecordingDirectories();
[[nodiscard]] QString screenRecordingDirectory();
[[nodiscard]] bool openScreenRecordingFolder();
[[nodiscard]] bool revealScreenRecordingFile(const QString& path);
} // namespace clip_shot::presentation::recording

#endif // CLIP_SHOT_PRESENTATION_SCREENRECORDINGFOLDER_H
