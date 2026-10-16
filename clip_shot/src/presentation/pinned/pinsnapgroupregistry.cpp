#include "clip_shot/presentation/pinsnapgroupregistry.h"

namespace clip_shot::presentation {

QString PinsnapGroupRegistry::freshGroupKey() {
    return QStringLiteral("snap-group-%1").arg(++m_groupCounter);
}

QString PinsnapGroupRegistry::merge(const QString& windowId,
                                    const QString& otherWindowId) {
    if (windowId.isEmpty() || otherWindowId.isEmpty() || windowId == otherWindowId) {
        return windowId.isEmpty() ? otherWindowId : windowId;
    }

    const std::optional<QString> first = groupOf(windowId);
    const std::optional<QString> second = groupOf(otherWindowId);
    QString targetKey;
    QString absorbedKey;

    if (first.has_value() && second.has_value()) {
        if (*first == *second) {
            return *first;
        }
        // Merge the smaller group into the larger one; ties keep the first key.
        if (m_groups.value(*first).size() >= m_groups.value(*second).size()) {
            targetKey = *first;
            absorbedKey = *second;
        } else {
            targetKey = *second;
            absorbedKey = *first;
        }
    } else if (first.has_value()) {
        targetKey = *first;
    } else if (second.has_value()) {
        targetKey = *second;
    } else {
        targetKey = freshGroupKey();
        m_groups.insert(targetKey, {});
    }

    const QSet<QString> absorbed = absorbedKey.isEmpty() ? QSet<QString>{}
                                                         : m_groups.take(absorbedKey);
    for (const QString& member : absorbed) {
        m_groups[targetKey].insert(member);
        m_windowToGroup.insert(member, targetKey);
    }
    m_groups[targetKey].insert(windowId);
    m_groups[targetKey].insert(otherWindowId);
    m_windowToGroup.insert(windowId, targetKey);
    m_windowToGroup.insert(otherWindowId, targetKey);
    return targetKey;
}

std::optional<QString> PinsnapGroupRegistry::groupOf(const QString& windowId) const {
    const auto it = m_windowToGroup.constFind(windowId);
    if (it == m_windowToGroup.constEnd()) {
        return std::nullopt;
    }
    return *it;
}

QStringList PinsnapGroupRegistry::membersOf(const QString& windowId) const {
    const std::optional<QString> key = groupOf(windowId);
    if (!key.has_value()) {
        return {};
    }
    QStringList members;
    const QSet<QString> all = m_groups.value(*key);
    for (const QString& member : all) {
        if (member != windowId) {
            members.append(member);
        }
    }
    return members;
}

void PinsnapGroupRegistry::removeMember(const QString& windowId) {
    const std::optional<QString> key = groupOf(windowId);
    if (!key.has_value()) {
        return;
    }
    m_windowToGroup.remove(windowId);
    m_groups[*key].remove(windowId);
    disbandIfSparse(*key);
}

void PinsnapGroupRegistry::clearGroup(const QString& groupKey) {
    const QSet<QString> members = m_groups.take(groupKey);
    for (const QString& member : members) {
        m_windowToGroup.remove(member);
    }
}

void PinsnapGroupRegistry::disbandIfSparse(const QString& groupKey) {
    if (m_groups.value(groupKey).size() <= 1) {
        clearGroup(groupKey);
    }
}

} // namespace clip_shot::presentation
