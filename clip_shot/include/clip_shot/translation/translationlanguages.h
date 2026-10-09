#ifndef CLIP_SHOT_TRANSLATION_TRANSLATIONLANGUAGES_H
#define CLIP_SHOT_TRANSLATION_TRANSLATIONLANGUAGES_H

#include "clip_shot/network/clipshotapiclient.h"
#include "clip_shot/translation/translationlanguagecatalog.h"

#include <QLocale>

namespace clip_shot::translation {
[[nodiscard]] QString translationLanguageName(const QString& code);
[[nodiscard]] QString defaultTranslationTargetLanguage(const QLocale& locale);
// Returns an index into the original catalog. Custom vision models also support translation.
[[nodiscard]] int translationModelIndex(const QVector<ClipShotChatModel>& models,
                                        const QString& preferredId);
[[nodiscard]] QString translationModelGroup(const ClipShotChatModel& model);
} // namespace clip_shot::translation

#endif
