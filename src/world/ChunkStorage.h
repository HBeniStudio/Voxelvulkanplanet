#pragma once

#include "core/Blocks.h"

#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace voxel::world {

inline constexpr int kChunkSize = 32;
inline constexpr int kChunkVolume = kChunkSize * kChunkSize * kChunkSize;

[[nodiscard]] inline int chunkIndex(int x, int y, int z) noexcept {
    return x + y * kChunkSize + z * kChunkSize * kChunkSize;
}

// Palette + bit-packed voxel indices for a 32³ chunk (uniform shortcut when all voxels match).
class ChunkStorage {
public:
    ChunkStorage() = default;

    [[nodiscard]] bool isUniform() const noexcept { return uniform_; }
    [[nodiscard]] voxel::core::BlockId uniformBlock() const noexcept { return uniformId_; }
    [[nodiscard]] const std::vector<voxel::core::BlockId>& palette() const noexcept { return palette_; }
    [[nodiscard]] int bitsPerIndex() const noexcept { return bitsPerIndex_; }

    [[nodiscard]] voxel::core::BlockId get(int x, int y, int z) const;
    void set(int x, int y, int z, voxel::core::BlockId id);

    void fill(voxel::core::BlockId id);

    [[nodiscard]] std::vector<uint8_t> serialize() const;
    static ChunkStorage deserialize(std::span<const uint8_t> bytes);

    [[nodiscard]] bool operator==(const ChunkStorage& other) const;

private:
    bool uniform_ = true;
    voxel::core::BlockId uniformId_ = 0;
    std::vector<voxel::core::BlockId> palette_;
    std::vector<uint8_t> packedIndices_;
    int bitsPerIndex_ = 0;

    [[nodiscard]] static int bitsNeededForPaletteSize(size_t paletteSize) noexcept;
    void ensurePaletteMode();
    void repackIndices(int newBits);
    [[nodiscard]] uint32_t readIndex(int linearIndex) const;
    void writeIndex(int linearIndex, uint32_t paletteIndex);
    [[nodiscard]] uint32_t paletteIndexOf(voxel::core::BlockId id);
};

} // namespace voxel::world
