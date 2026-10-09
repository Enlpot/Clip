#ifndef CLIP_SHOT_PRESENTATION_TRANSLATABLETEXT_H
#define CLIP_SHOT_PRESENTATION_TRANSLATABLETEXT_H

#include <QString>

namespace clip_shot::presentation::settings {

struct TranslatableText {
    const char* context = nullptr;
    const char* source = nullptr;

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] QString translated() const;
};

} // namespace clip_shot::presentation::settings

#endif // CLIP_SHOT_PRESENTATION_TRANSLATABLETEXT_H
