// Standalone model smoke test: compile with C++17.
// g++ -std=c++17 -Wall -Wextra -pedantic workspace_model_smoke.cpp -o workspace_smoke
#include "../workspace_model.h"
#include <cassert>

int main()
{
    auto registry = robotdolly::makeDefaultRegistry();
    robotdolly::Workspace space(registry);
    const auto world = space.root().area.id;
    assert(space.root().area.editor_type == "sl.world");

    const auto nearby = space.split(world, robotdolly::SplitAxis::Horizontal, "sl.chat");
    assert(nearby != 0);
    assert(space.setEditor(nearby, "sl.chat", "nearby"));
    assert(space.resize(world, 0.7f));

    const auto inventory = space.split(nearby, robotdolly::SplitAxis::Vertical, "sl.inventory");
    assert(inventory != 0);
    const auto group = space.split(inventory, robotdolly::SplitAxis::Horizontal, "sl.chat");
    assert(space.setEditor(group, "sl.chat", "group:example"));
    const auto places = space.split(group, robotdolly::SplitAxis::Vertical, "sl.landmarks");
    assert(places != 0);
    assert(!space.setEditor(places, "not.registered"));
    assert(space.closeArea(places));
    assert(!space.closeArea(world) || space.root().is_split);
}
