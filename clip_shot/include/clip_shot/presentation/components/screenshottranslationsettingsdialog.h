#ifndef CLIP_SHOT_PRESENTATION_SCREENSHOTTRANSLATIONSETTINGSDIALOG_H
#define CLIP_SHOT_PRESENTATION_SCREENSHOTTRANSLATIONSETTINGSDIALOG_H
#include <functional>
class QObject;
class QWidget;
namespace adqt::widgets {
class AdModal;
}
namespace clip_shot::translation {
class TranslationService;
}
namespace clip_shot::presentation {
adqt::widgets::AdModal*
createScreenshotTranslationSettingsDialog(translation::TranslationService& service, QWidget* owner,
                                          QObject* parent,
                                          std::function<void(bool before)> displayModeChanged);
}
#endif
