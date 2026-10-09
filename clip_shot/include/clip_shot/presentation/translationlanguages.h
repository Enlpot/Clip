#ifndef CLIP_SHOT_PRESENTATION_TRANSLATIONLANGUAGES_H
#define CLIP_SHOT_PRESENTATION_TRANSLATIONLANGUAGES_H
#include "clip_shot/translation/translationlanguages.h"

// Compatibility names for presentation consumers; all policy lives in the translation core.
namespace clip_shot::presentation {
using translation::defaultTranslationTargetLanguage;
using translation::TranslationLanguage;
using translation::translationLanguageName;
using translation::translationLanguages;
using translation::translationModelIndex;
} // namespace clip_shot::presentation
#endif
