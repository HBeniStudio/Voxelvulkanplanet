#include "world/RegionFile.h"

#include "world/Chunk.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace voxel::world {

namespace {

constexpr char kMagic[4] = {'V', 'X', 'S', 'V'};
constexpr uint32_t kVersion = 1;

void writeU32(std::ostream& out, uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        out.put(static_cast<char>((v >> (8 * i)) & 0xFF));
    }
}

uint32_t readU32(std::istream& in) {
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) {
        v |= static_cast<uint32_t>(static_cast<unsigned char>(in.get())) << (8 * i);
    }
    return v;
}

[[nodiscard]] int localIndexInRegion(int32_t chunkCoord, int32_t regionCoord) {
    return static_cast<int>(chunkCoord - regionCoord * kRegionSize);
}

} // namespace

RegionCoord regionOfChunk(ChunkPos chunk) {
    const auto floorDiv = [](int32_t a, int32_t b) {
        int32_t q = a / b;
        int32_t r = a % b;
        if ((r != 0) && ((r > 0) != (b > 0))) {
            --q;
        }
        return q;
    };
    return RegionCoord{floorDiv(chunk.x, kRegionSize), floorDiv(chunk.y, kRegionSize),
                       floorDiv(chunk.z, kRegionSize)};
}

void RegionFile::saveRegion(const std::filesystem::path& worldDir, RegionCoord region,
                            const std::unordered_map<ChunkPos, ChunkStorage, ChunkPosHash>& chunks) {
    std::filesystem::create_directories(worldDir);
    const std::filesystem::path path =
        worldDir / ("r." + std::to_string(region.x) + "." + std::to_string(region.y) + "." +
                    std::to_string(region.z) + ".bin");
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("RegionFile save: cannot open " + path.string());
    }
    out.write(kMagic, 4);
    writeU32(out, kVersion);
    writeU32(out, static_cast<uint32_t>(chunks.size()));
    for (const auto& [pos, storage] : chunks) {
        writeU32(out, pos.x);
        writeU32(out, pos.y);
        writeU32(out, pos.z);
        const std::vector<uint8_t> bytes = storage.serialize();
        writeU32(out, static_cast<uint32_t>(bytes.size()));
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }
}

std::unordered_map<ChunkPos, ChunkStorage, ChunkPosHash>
RegionFile::loadRegion(const std::filesystem::path& worldDir, RegionCoord region) {
    const std::filesystem::path path =
        worldDir / ("r." + std::to_string(region.x) + "." + std::to_string(region.y) + "." +
                    std::to_string(region.z) + ".bin");
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("RegionFile load: cannot open " + path.string());
    }
    char magic[4];
    in.read(magic, 4);
    if (std::string(magic, 4) != std::string(kMagic, 4)) {
        throw std::runtime_error("RegionFile load: bad magic");
    }
    const uint32_t version = readU32(in);
    if (version != kVersion) {
        throw std::runtime_error("RegionFile load: unsupported version");
    }
    const uint32_t count = readU32(in);
    std::unordered_map<ChunkPos, ChunkStorage, ChunkPosHash> out;
    for (uint32_t i = 0; i < count; ++i) {
        ChunkPos pos{};
        pos.x = static_cast<int32_t>(readU32(in));
        pos.y = static_cast<int32_t>(readU32(in));
        pos.z = static_cast<int32_t>(readU32(in));
        const uint32_t byteCount = readU32(in);
        std::vector<uint8_t> bytes(byteCount);
        in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(byteCount));
        out.emplace(pos, ChunkStorage::deserialize(bytes));
    }
    return out;
}

} // namespace voxel::world
