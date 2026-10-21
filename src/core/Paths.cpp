#include "core/Paths.h"

#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace voxel::core {

namespace {

std::filesystem::path executableDirectory() {
#if defined(_WIN32)
    std::vector<wchar_t> buffer(MAX_PATH, L'\0');
    for (;;) {
        const DWORD len = GetModuleFileNameW(nullptr, buffer.data(),
                                            static_cast<DWORD>(buffer.size()));
        if (len == 0) {
            return {};
        }
        if (len < buffer.size() - 1) {
            return std::filesystem::path(buffer.data(), buffer.data() + len).parent_path();
        }
        buffer.resize(buffer.size() * 2);
    }
#else
    std::vector<char> buffer(4096);
    while (true) {
        const ssize_t len = readlink("/proc/self/exe", buffer.data(), buffer.size());
        if (len < 0) {
            return {};
        }
        if (static_cast<size_t>(len) < buffer.size()) {
            return std::filesystem::path(std::string(buffer.data(), static_cast<size_t>(len)))
                .parent_path();
        }
        buffer.resize(buffer.size() * 2);
    }
#endif
}

bool hasContentRoot(const std::filesystem::path& root) {
    return std::filesystem::is_regular_file(root / "data" / "blocks.json");
}

bool hasShaderDir(const std::filesystem::path& dir) {
    return std::filesystem::is_regular_file(dir / "chunk.vert.spv") ||
           std::filesystem::is_regular_file(dir / "tri.vert.spv");
}

std::filesystem::path firstExistingContentRoot() {
    const std::filesystem::path cwd = std::filesystem::current_path();
    if (hasContentRoot(cwd)) {
        return cwd;
    }
    const std::filesystem::path exeDir = executableDirectory();
    if (!exeDir.empty() && hasContentRoot(exeDir)) {
        return exeDir;
    }
    return std::filesystem::path(VOXEL_SOURCE_DIR);
}

std::filesystem::path firstExistingShaderDir() {
    const std::vector<std::filesystem::path> candidates = {
        executableDirectory() / "shaders",
        std::filesystem::current_path() / "shaders",
        std::filesystem::current_path() / "build" / "shaders",
        executableDirectory() / ".." / "shaders",
    };
    for (const auto& candidate : candidates) {
        if (!candidate.empty() && hasShaderDir(candidate)) {
            return std::filesystem::weakly_canonical(candidate);
        }
    }
    return std::filesystem::path(VOXEL_SHADER_DIR);
}

} // namespace

std::filesystem::path contentRoot() {
    return firstExistingContentRoot();
}

std::filesystem::path shaderDir() {
    return firstExistingShaderDir();
}

} // namespace voxel::core
