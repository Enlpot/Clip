#ifndef CLIP_SHOT_PRESENTATION_PINSNAPGROUPREGISTRY_H
#define CLIP_SHOT_PRESENTATION_PINSNAPGROUPREGISTRY_H

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>

#include <optional>

namespace clip_shot::presentation {

// Tracks ad-hoc runtime snap groups between pinned screenshot windows.
// A snap group is created whenever one pinned window magnetically snaps to
// another (contact snap or edge alignment). Windows in the same group move and
// scale together; pressing Ctrl while dragging detaches the dragged window.
//
// This is intentionally independent from the persisted window "groups"
// managed by PinnedWindowGroupManager (which are user-created classifications).
//
// Threading: this class is used from the GUI thread only.
class PinsnapGroupRegistry final {
  public:
    // Merges the snap groups of both windows into a single group. If neither
    // window belongs to a group yet, a fresh group is created. Returns the
    // group key both windows now share.
    QString merge(const QString& windowId, const QString& otherWindowId);

    // Returns the group key of the window, or nullopt when it is not grouped.
    [[nodiscard]] std::optional<QString> groupOf(const QString& windowId) const;

    // Returns the other members of the window's snap group. The window itself
    // is never included. Empty when the window is not grouped.
    [[nodiscard]] QStringList membersOf(const QString& windowId) const;

    // Removes a window from its snap group. Groups that drop to one member are
    // disbanded (the remaining window becomes ungrouped).
    void removeMember(const QString& windowId);

    // Removes every window belonging to the given group.
    void clearGroup(const QString& groupKey);

    [[nodiscard]] bool isEmpty() const {
        return m_groups.isEmpty();
    }

    [[nodiscard]] int groupCount() const {
        return m_groups.size();
    }

    void clear() {
        m_groups.clear();
        m_windowToGroup.clear();
    }

  private:
    [[nodiscard]] QString freshGroupKey();
    [[nodiscard]] void disbandIfSparse(const QString& groupKey);

    QHash<QString, QSet<QString>> m_groups;
    QHash<QString, QString> m_windowToGroup;
    quint32 m_groupCounter = 0;
};

} // namespace clip_shot::presentation

#endif // CLIP_SHOT_PRESENTATION_PINSNAPGROUPREGISTRY_H
