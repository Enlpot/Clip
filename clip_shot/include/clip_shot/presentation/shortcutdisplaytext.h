#pragma once

#include "clip_shot/shortcuts/shortcutdisplayservice.h"

namespace clip_shot::presentation {

inline QString formatShortcutDisplayText(const shortcuts::ShortcutBinding& shortcut) {
    return shortcuts::formatShortcutDisplayText(shortcut);
}

inline QString formatShortcutListDisplayText(const shortcuts::ShortcutBindingList& shortcuts) {
    return shortcuts::formatShortcutListDisplayText(shortcuts);
}

} // namespace clip_shot::presentation
