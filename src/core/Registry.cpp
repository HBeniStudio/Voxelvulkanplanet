#include "core/Registry.h"

#include <algorithm>

namespace voxel::core {

Registry& Registry::instance() {
    static Registry reg;
    return reg;
}

void Registry::registerSystem(Phase phase, std::unique_ptr<ISystem> system, const char* name) {
    entries_.push_back(Entry{phase, name ? name : "unknown", std::move(system)});
}

void Registry::runPhase(Phase phase, World& world, float dt) {
    for (auto& entry : entries_) {
        if (entry.phase == phase) {
            entry.system->update(world, dt);
        }
    }
}

std::vector<std::string> Registry::systemNamesForPhase(Phase phase) const {
    std::vector<std::string> names;
    for (const auto& entry : entries_) {
        if (entry.phase == phase) {
            names.push_back(entry.name);
        }
    }
    return names;
}

} // namespace voxel::core
