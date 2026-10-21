#include "core/Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>

namespace voxel::core {

void Log::init(const std::string& level) {
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto log = std::make_shared<spdlog::logger>("voxel", sink);
    log->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    if (level == "trace") {
        log->set_level(spdlog::level::trace);
    } else if (level == "debug") {
        log->set_level(spdlog::level::debug);
    } else if (level == "warn") {
        log->set_level(spdlog::level::warn);
    } else if (level == "error") {
        log->set_level(spdlog::level::err);
    } else {
        log->set_level(spdlog::level::info);
    }
    spdlog::set_default_logger(log);
}

std::shared_ptr<spdlog::logger> Log::logger() {
    return spdlog::default_logger();
}

} // namespace voxel::core
