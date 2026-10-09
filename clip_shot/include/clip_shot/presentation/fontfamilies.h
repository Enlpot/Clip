#ifndef CLIP_SHOT_PRESENTATION_FONTFAMILIES_H
#define CLIP_SHOT_PRESENTATION_FONTFAMILIES_H

#include <QFontDatabase>
#include <QStringList>

namespace clip_shot::presentation {
// Enumerate once: font selectors are rebuilt when pages and toolbar editors change.
inline const QStringList& applicationFontFamilies() {
    static const QStringList families = [] {
        QStringList result;
        for (const QString& family : QFontDatabase::families()) {
            const QString trimmed = family.trimmed();
            if (!trimmed.isEmpty()) {
                result.append(trimmed);
            }
        }
        result.removeDuplicates();
        result.sort(Qt::CaseInsensitive);
        return result;
    }();
    return families;
}
} // namespace clip_shot::presentation

#endif // CLIP_SHOT_PRESENTATION_FONTFAMILIES_H
