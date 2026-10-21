#pragma once

#include <memory>
#include <string>

namespace spdlog {
class logger;
}

namespace voxel::core {

class Log {
public:
    static void init(const std::string& level = "info");
    static std::shared_ptr<spdlog::logger> logger();
};

} // namespace voxel::core
