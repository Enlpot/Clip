#include "clip_shot/presentation/pinsnapgroupregistry.h"

#include <QCoreApplication>

#include <iostream>
#include <stdexcept>

namespace {
using clip_shot::presentation::PinsnapGroupRegistry;

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// Merging two ungrouped windows creates a fresh group with both members.
void testMergeCreatesGroup() {
    PinsnapGroupRegistry registry;
    const QString key = registry.merge(QStringLiteral("a"), QStringLiteral("b"));
    require(registry.groupOf(QStringLiteral("a")) == key,
            "a should belong to the merged group");
    require(registry.groupOf(QStringLiteral("b")) == key,
            "b should belong to the merged group");
    require(registry.membersOf(QStringLiteral("a")).size() == 1,
            "a should see exactly one other member");
    require(registry.membersOf(QStringLiteral("a")).contains(QStringLiteral("b")),
            "a's member list should contain b");
    require(registry.membersOf(QStringLiteral("b")).contains(QStringLiteral("a")),
            "b's member list should contain a");
}

// MembersOf never includes the queried window itself.
void testMembersOfExcludesSelf() {
    PinsnapGroupRegistry registry;
    static_cast<void>(registry.merge(QStringLiteral("x"), QStringLiteral("y")));
    const QStringList members = registry.membersOf(QStringLiteral("x"));
    require(!members.contains(QStringLiteral("x")),
            "membersOf should not include the window itself");
}

// Merging two existing groups absorbs one into the other and keeps both keys
// equal afterwards.
void testMergeAbsorbsGroup() {
    PinsnapGroupRegistry registry;
    static_cast<void>(registry.merge(QStringLiteral("a"), QStringLiteral("b")));
    static_cast<void>(registry.merge(QStringLiteral("c"), QStringLiteral("d")));
    const QString key = registry.merge(QStringLiteral("a"), QStringLiteral("c"));
    require(registry.groupOf(QStringLiteral("b")) == key,
            "b should follow a into the merged group");
    require(registry.groupOf(QStringLiteral("d")) == key,
            "d should follow c into the merged group");
    require(registry.membersOf(QStringLiteral("a")).size() == 3,
            "a should now see b, c and d");
    require(registry.groupCount() == 1, "the two groups should collapse into one");
}

// Merging windows already in the same group is a no-op.
void testMergeSameGroupNoOp() {
    PinsnapGroupRegistry registry;
    const QString key = registry.merge(QStringLiteral("a"), QStringLiteral("b"));
    const QString again = registry.merge(QStringLiteral("a"), QStringLiteral("b"));
    require(again == key, "re-merging the same pair keeps the same key");
    require(registry.groupCount() == 1, "no extra group should appear");
}

// Empty ids are rejected without creating state.
void testMergeEmptyIds() {
    PinsnapGroupRegistry registry;
    static_cast<void>(registry.merge(QString(), QStringLiteral("b")));
    require(registry.isEmpty(), "empty window ids should not create a group");
    static_cast<void>(registry.merge(QStringLiteral("a"), QString()));
    require(registry.isEmpty(), "empty other id should not create a group");
}

// Removing a member drops it from the group; the remaining member becomes
// ungrouped when the group falls below two members.
void testRemoveMemberDisbandsSparseGroup() {
    PinsnapGroupRegistry registry;
    static_cast<void>(registry.merge(QStringLiteral("a"), QStringLiteral("b")));
    registry.removeMember(QStringLiteral("a"));
    require(!registry.groupOf(QStringLiteral("a")).has_value(),
            "removed window should be ungrouped");
    require(!registry.groupOf(QStringLiteral("b")).has_value(),
            "a two-member group should disband after one leaves");
    require(registry.isEmpty(), "registry should be empty after disband");
}

// A three-member group keeps the remaining pair after one member leaves.
void testRemoveMemberKeepsLargerGroup() {
    PinsnapGroupRegistry registry;
    static_cast<void>(registry.merge(QStringLiteral("a"), QStringLiteral("b")));
    static_cast<void>(registry.merge(QStringLiteral("a"), QStringLiteral("c")));
    registry.removeMember(QStringLiteral("c"));
    require(registry.membersOf(QStringLiteral("a")).size() == 1,
            "a should still see b after c leaves");
    require(registry.membersOf(QStringLiteral("a")).contains(QStringLiteral("b")),
            "b should remain in a's group");
    require(!registry.groupOf(QStringLiteral("c")).has_value(),
            "c should be ungrouped");
}

// Removing a window that is not grouped is harmless.
void testRemoveUngroupedMember() {
    PinsnapGroupRegistry registry;
    registry.removeMember(QStringLiteral("solo"));
    require(registry.isEmpty(), "removing an ungrouped window changes nothing");
}

// clearGroup empties the group and leaves every member ungrouped.
void testClearGroup() {
    PinsnapGroupRegistry registry;
    static_cast<void>(registry.merge(QStringLiteral("a"), QStringLiteral("b")));
    static_cast<void>(registry.merge(QStringLiteral("a"), QStringLiteral("c")));
    const QString key = *registry.groupOf(QStringLiteral("a"));
    registry.clearGroup(key);
    require(registry.isEmpty(), "clearGroup should remove every member");
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    try {
        testMergeCreatesGroup();
        testMembersOfExcludesSelf();
        testMergeAbsorbsGroup();
        testMergeSameGroupNoOp();
        testMergeEmptyIds();
        testRemoveMemberDisbandsSparseGroup();
        testRemoveMemberKeepsLargerGroup();
        testRemoveUngroupedMember();
        testClearGroup();
    } catch (const std::exception& error) {
        std::cerr << "pinned snap group tests failed: " << error.what() << std::endl;
        return 1;
    }

    std::cout << "pinned snap group tests passed" << std::endl;
    return 0;
}
