#ifndef CLIP_SHOT_GLOBALMOUSEACTIVATIONKEYS_H
#define CLIP_SHOT_GLOBALMOUSEACTIVATIONKEYS_H

#include <QStringList>

namespace clip_shot {
inline QStringList globalMouseActivationKeys() {
#ifdef Q_OS_MACOS
    return {QStringLiteral("command"), QStringLiteral("control"), QStringLiteral("option"),
            QStringLiteral("shift")};
#else
    return {QStringLiteral("windows"), QStringLiteral("ctrl"), QStringLiteral("alt"),
            QStringLiteral("shift")};
#endif
}
} // namespace clip_shot

#endif
