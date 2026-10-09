#ifndef CLIP_SHOT_SERVERCONFIGURATION_H
#define CLIP_SHOT_SERVERCONFIGURATION_H

#include <QString>
#include <QUrl>
#include <optional>

namespace clip_shot {
// Empty means the configured environment/build default.
[[nodiscard]] inline std::optional<QString> normalizedServerUrl(const QString& value) {
    QString normalized = value.trimmed();
    if (normalized.isEmpty())
        return normalized;
    const QUrl url(normalized, QUrl::StrictMode);
    if (!url.isValid() || url.isRelative() || url.host().isEmpty() ||
        (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https")) ||
        url.authority().contains(u'@') || url.hasQuery() || url.hasFragment())
        return std::nullopt;
    normalized = url.toString(QUrl::FullyEncoded);
    while (normalized.endsWith(u'/'))
        normalized.chop(1);
    return normalized;
}
} // namespace clip_shot

#endif // CLIP_SHOT_SERVERCONFIGURATION_H
