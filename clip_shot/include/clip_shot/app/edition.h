#ifndef CLIP_SHOT_APP_EDITION_H
#define CLIP_SHOT_APP_EDITION_H

#include <QCoreApplication>
#include <QString>

#ifndef CLIP_SHOT_EDITION_MINI
#define CLIP_SHOT_EDITION_MINI 0
#endif
#ifndef CLIP_SHOT_ENABLE_QR_RECOGNITION
#define CLIP_SHOT_ENABLE_QR_RECOGNITION (!CLIP_SHOT_EDITION_MINI)
#endif
#ifndef CLIP_SHOT_ENABLE_TABLE_RECOGNITION
#define CLIP_SHOT_ENABLE_TABLE_RECOGNITION (!CLIP_SHOT_EDITION_MINI)
#endif
#ifndef CLIP_SHOT_ENABLE_IMAGE_CONVERSION
#define CLIP_SHOT_ENABLE_IMAGE_CONVERSION (!CLIP_SHOT_EDITION_MINI)
#endif
#ifndef CLIP_SHOT_ENABLE_LATEX_RECOGNITION
#define CLIP_SHOT_ENABLE_LATEX_RECOGNITION (!CLIP_SHOT_EDITION_MINI)
#endif
#ifndef CLIP_SHOT_ENABLE_TEXT_TRANSLATION
#define CLIP_SHOT_ENABLE_TEXT_TRANSLATION (!CLIP_SHOT_EDITION_MINI)
#endif
#ifndef CLIP_SHOT_ENABLE_API_CONFIGURATION
#define CLIP_SHOT_ENABLE_API_CONFIGURATION (!CLIP_SHOT_EDITION_MINI)
#endif
#ifndef CLIP_SHOT_ENABLE_EXTENDED_FEATURES
#define CLIP_SHOT_ENABLE_EXTENDED_FEATURES (!CLIP_SHOT_EDITION_MINI)
#endif

namespace clip_shot::app::edition {
inline constexpr bool isMini = CLIP_SHOT_EDITION_MINI != 0;
inline constexpr bool qrRecognition = CLIP_SHOT_ENABLE_QR_RECOGNITION != 0;
inline constexpr bool tableRecognition = CLIP_SHOT_ENABLE_TABLE_RECOGNITION != 0;
inline constexpr bool imageConversion = CLIP_SHOT_ENABLE_IMAGE_CONVERSION != 0;
inline constexpr bool latexRecognition = CLIP_SHOT_ENABLE_LATEX_RECOGNITION != 0;
inline constexpr bool textTranslation = CLIP_SHOT_ENABLE_TEXT_TRANSLATION != 0;
inline constexpr bool apiConfiguration = CLIP_SHOT_ENABLE_API_CONFIGURATION != 0;
inline constexpr bool extendedFeatures = CLIP_SHOT_ENABLE_EXTENDED_FEATURES != 0;

[[nodiscard]] inline QString productName() {
    return isMini ? QCoreApplication::translate("EditionMetadata", "Clip Mini")
                  : QCoreApplication::translate("EditionMetadata", "Clip");
}
[[nodiscard]] inline QString applicationName() {
    return isMini ? QStringLiteral("clip_shot_mini") : QStringLiteral("clip_shot");
}
[[nodiscard]] inline QString registryName() {
    return isMini ? QStringLiteral("ClipShotMini") : QStringLiteral("ClipShot");
}
[[nodiscard]] inline QString executableName() {
#ifdef Q_OS_WIN
    return applicationName() + QStringLiteral(".exe");
#else
    return applicationName();
#endif
}
[[nodiscard]] inline QString productId() {
    return isMini ? QStringLiteral("clip-shot-mini") : QStringLiteral("clip-shot");
}
[[nodiscard]] inline QString portableMarkerName() {
    return isMini ? QStringLiteral("__mini_data_directory") : QStringLiteral("__data_directory");
}
[[nodiscard]] inline QString updaterName() {
#ifdef Q_OS_WIN
    return productId() + QStringLiteral("-updater.exe");
#else
    return productId() + QStringLiteral("-updater");
#endif
}
[[nodiscard]] inline QString mcpName() {
#ifdef Q_OS_WIN
    return productId() + QStringLiteral("-mcp.exe");
#else
    return productId() + QStringLiteral("-mcp");
#endif
}
[[nodiscard]] inline QString bundleId() {
    return QStringLiteral("com.clipshot.") + applicationName();
}
} // namespace clip_shot::app::edition

#endif // CLIP_SHOT_APP_EDITION_H
