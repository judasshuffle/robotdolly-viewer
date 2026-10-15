#include "../layout_io.h"
#include <array>
#include <iostream>
#include <stdexcept>

struct Node {
    bool used = false, split = false, vertical = true;
    int parent = -1, first = -1, second = -1, editor = 3;
    float fraction = .5f;
};
using Nodes = std::array<Node, 63>;
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
const std::string names[] = {"world", "im_container:", "mini_map:", "empty", "camera:"};
std::string key(int id) { return id >= 0 && id < 5 ? names[id] : ""; }
int id(const std::string& key) {
    for (int i = 0; i < 5; ++i) if (names[i] == key) return i;
    return -1;
}
int main() {
    try {
        Nodes tree{};
        tree[0] = {true, true, true, -1, 7, 9, 3, .61234567f};
        tree[7] = {true, false, true, 0, -1, -1, 0};
        tree[9] = {true, true, false, 0, 11, 14, 3, .32123456f};
        tree[11] = {true, false, true, 9, -1, -1, 1};
        tree[14] = {true, false, true, 9, -1, -1, 2};
        std::string saved;
        check(robotdolly::encodeLayout(tree, key, saved), "serialize nested layout with non-contiguous indices");
        Nodes restored{};
        check(robotdolly::decodeLayout(saved, id, restored), "reload layout");
        std::string resaved;
        check(robotdolly::encodeLayout(restored, key, resaved) && saved == resaved, "round trip preserves tree, orientations, panels and exact ratios");
        check(restored[0].fraction == tree[0].fraction && restored[2].fraction == tree[9].fraction,
              "float ratios round trip exactly");
        auto reordered = [](const std::string& name) { int found = id(name); return found < 0 ? -1 : 4 - found; };
        Nodes remapped{};
        check(robotdolly::decodeLayout(saved, reordered, remapped) && remapped[1].editor == 4,
              "stable keys survive editor enum reorder");
        for (const std::string bad : {
            "", "ROBOTDOLLY_LAYOUT 99\nleaf world", "ROBOTDOLLY_LAYOUT 1\nleaf unknown",
            "ROBOTDOLLY_LAYOUT 1\nsplit vertical 0 leaf world leaf empty",
            "ROBOTDOLLY_LAYOUT 1\nsplit vertical 1 leaf world leaf empty",
            "ROBOTDOLLY_LAYOUT 1\nsplit vertical nan leaf world leaf empty",
            "ROBOTDOLLY_LAYOUT 1\nsplit sideways .5 leaf world leaf empty",
            "ROBOTDOLLY_LAYOUT 1\nsplit vertical .5 leaf world",
            "ROBOTDOLLY_LAYOUT 1\nleaf world trailing-data"}) {
            check(!robotdolly::decodeLayout(bad, id, restored), "reject corrupt or unsupported file");
            check(robotdolly::encodeLayout(restored, key, resaved) && saved == resaved, "failed load leaves destination unchanged");
        }
        std::string huge = "ROBOTDOLLY_LAYOUT 1\n";
        for (int i = 0; i < 64; ++i) huge += "split vertical .5\n";
        huge += "leaf world\n";
        check(!robotdolly::decodeLayout(huge, id, restored), "bound recursion and node count");
        check(!robotdolly::decodeLayout(std::string(65537, 'x'), id, restored), "bound file size");
        tree[9].first = 0;
        check(!robotdolly::encodeLayout(tree, key, resaved), "reject cyclic in-memory tree");
        check(robotdolly::decodeLayout("ROBOTDOLLY_LAYOUT 1\nleaf world\n", id, restored), "single unsplit area restores");
        check(!restored[1].used && restored[0].parent == -1, "old nodes cleared after load");
        std::cout << "Layout persistence tests passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
