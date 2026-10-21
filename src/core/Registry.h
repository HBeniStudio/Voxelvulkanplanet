#pragma once

#include "core/World.h"

#include <memory>
#include <string>
#include <vector>

namespace voxel::core {

enum class Phase { Startup, Update, Render, Shutdown };

class ISystem {
public:
    virtual ~ISystem() = default;
    virtual void update(World& world, float dt) = 0;
};

class Registry {
public:
    static Registry& instance();

    void registerSystem(Phase phase, std::unique_ptr<ISystem> system, const char* name);
    void runPhase(Phase phase, World& world, float dt);
    std::vector<std::string> systemNamesForPhase(Phase phase) const;

private:
    struct Entry {
        Phase phase;
        std::string name;
        std::unique_ptr<ISystem> system;
    };
    std::vector<Entry> entries_;
};

#define REGISTER_SYSTEM(SystemType, phase)                                           \
    namespace {                                                                      \
        struct SystemType##_Registrar {                                              \
            SystemType##_Registrar() {                                               \
                voxel::core::Registry::instance().registerSystem(                    \
                    phase, std::make_unique<SystemType>(), #SystemType);             \
            }                                                                        \
        };                                                                           \
        const SystemType##_Registrar g_##SystemType##_registrar{};                   \
    }

} // namespace voxel::core
