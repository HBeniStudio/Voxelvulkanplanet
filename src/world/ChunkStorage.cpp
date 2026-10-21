#include "world/ChunkStorage.h"

#include <algorithm>
#include <bit>
#include <stdexcept>

namespace voxel::world {

namespace {

constexpr uint8_t kSerializeMagic = 0x56; // 'V'
constexpr uint8_t kSerializeVersion = 1;

void appendU16(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

void appendU32(std::vector<uint8_t>& out, uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        out.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
    }
}

[[nodiscard]] uint16_t readU16(std::span<const uint8_t> bytes, size_t& offset) {
    if (offset + 2 > bytes.size()) {
        throw std::runtime_error("ChunkStorage deserialize: truncated u16");
    }
    const uint16_t v = static_cast<uint16_t>(bytes[offset]) |
                       (static_cast<uint16_t>(bytes[offset + 1]) << 8);
    offset += 2;
    return v;
}

[[nodiscard]] uint32_t readU32(std::span<const uint8_t> bytes, size_t& offset) {
    if (offset + 4 > bytes.size()) {
        throw std::runtime_error("ChunkStorage deserialize: truncated u32");
    }
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) {
        v |= static_cast<uint32_t>(bytes[offset + i]) << (8 * i);
    }
    offset += 4;
    return v;
}

} // namespace

int ChunkStorage::bitsNeededForPaletteSize(size_t paletteSize) noexcept {
    if (paletteSize <= 1) {
        return 0;
    }
    return static_cast<int>(std::bit_width(static_cast<unsigned>(paletteSize - 1)));
}

voxel::core::BlockId ChunkStorage::get(int x, int y, int z) const {
    if (x < 0 || y < 0 || z < 0 || x >= kChunkSize || y >= kChunkSize || z >= kChunkSize) {
        return 0;
    }
    if (uniform_) {
        return uniformId_;
    }
    const int idx = chunkIndex(x, y, z);
    const uint32_t palIdx = readIndex(idx);
    if (palIdx >= palette_.size()) {
        return 0;
    }
    return palette_[palIdx];
}

void ChunkStorage::fill(voxel::core::BlockId id) {
    uniform_ = true;
    uniformId_ = id;
    palette_.clear();
    packedIndices_.clear();
    bitsPerIndex_ = 0;
}

void ChunkStorage::ensurePaletteMode() {
    if (!uniform_) {
        return;
    }
    palette_.clear();
    palette_.push_back(uniformId_);
    bitsPerIndex_ = bitsNeededForPaletteSize(palette_.size());
    const size_t totalBits = static_cast<size_t>(kChunkVolume) * static_cast<size_t>(bitsPerIndex_);
    packedIndices_.assign((totalBits + 7) / 8, 0);
    for (int i = 0; i < kChunkVolume; ++i) {
        writeIndex(i, 0);
    }
    uniform_ = false;
}

void ChunkStorage::repackIndices(int newBits) {
    if (newBits == bitsPerIndex_) {
        return;
    }
    std::vector<uint32_t> decoded(static_cast<size_t>(kChunkVolume));
    for (int i = 0; i < kChunkVolume; ++i) {
        decoded[static_cast<size_t>(i)] = readIndex(i);
    }
    bitsPerIndex_ = newBits;
    const size_t totalBits = static_cast<size_t>(kChunkVolume) * static_cast<size_t>(bitsPerIndex_);
    packedIndices_.assign((totalBits + 7) / 8, 0);
    for (int i = 0; i < kChunkVolume; ++i) {
        writeIndex(i, decoded[static_cast<size_t>(i)]);
    }
}

uint32_t ChunkStorage::readIndex(int linearIndex) const {
    if (bitsPerIndex_ == 0) {
        return 0;
    }
    const size_t bitOffset =
        static_cast<size_t>(linearIndex) * static_cast<size_t>(bitsPerIndex_);
    size_t pos = bitOffset / 8;
    int shiftWithinByte = static_cast<int>(bitOffset % 8);
    uint32_t value = 0;
    int bitsRead = 0;
    while (bitsRead < bitsPerIndex_) {
        const uint8_t byte = pos < packedIndices_.size() ? packedIndices_[pos] : 0;
        const int take = std::min(bitsPerIndex_ - bitsRead, 8 - shiftWithinByte);
        const uint8_t mask = static_cast<uint8_t>((1u << take) - 1u);
        const uint8_t chunk = static_cast<uint8_t>((byte >> shiftWithinByte) & mask);
        value |= static_cast<uint32_t>(chunk) << bitsRead;
        bitsRead += take;
        ++pos;
        shiftWithinByte = 0;
    }
    return value;
}

void ChunkStorage::writeIndex(int linearIndex, uint32_t paletteIndex) {
    if (bitsPerIndex_ == 0) {
        return;
    }
    const size_t bitOffset =
        static_cast<size_t>(linearIndex) * static_cast<size_t>(bitsPerIndex_);
    size_t pos = bitOffset / 8;
    int shiftWithinByte = static_cast<int>(bitOffset % 8);
    int bitsWritten = 0;
    uint32_t remaining = paletteIndex;
    while (bitsWritten < bitsPerIndex_) {
        const int take = std::min(bitsPerIndex_ - bitsWritten, 8 - shiftWithinByte);
        const uint8_t mask = static_cast<uint8_t>((1u << take) - 1u);
        const uint8_t chunk = static_cast<uint8_t>(remaining & mask);
        if (pos >= packedIndices_.size()) {
            packedIndices_.resize(pos + 1, 0);
        }
        packedIndices_[pos] &= static_cast<uint8_t>(~(mask << shiftWithinByte));
        packedIndices_[pos] |= static_cast<uint8_t>(chunk << shiftWithinByte);
        remaining >>= take;
        bitsWritten += take;
        ++pos;
        shiftWithinByte = 0;
    }
}

uint32_t ChunkStorage::paletteIndexOf(voxel::core::BlockId id) {
    for (size_t i = 0; i < palette_.size(); ++i) {
        if (palette_[i] == id) {
            return static_cast<uint32_t>(i);
        }
    }
    palette_.push_back(id);
    const int neededBits = bitsNeededForPaletteSize(palette_.size());
    if (!uniform_ && neededBits > bitsPerIndex_) {
        repackIndices(neededBits);
    } else if (uniform_) {
        bitsPerIndex_ = neededBits;
    }
    return static_cast<uint32_t>(palette_.size() - 1);
}

void ChunkStorage::set(int x, int y, int z, voxel::core::BlockId id) {
    if (x < 0 || y < 0 || z < 0 || x >= kChunkSize || y >= kChunkSize || z >= kChunkSize) {
        return;
    }
    if (uniform_ && id == uniformId_) {
        return;
    }
    if (uniform_) {
        ensurePaletteMode();
    }
    const int idx = chunkIndex(x, y, z);
    const uint32_t palIdx = paletteIndexOf(id);
    writeIndex(idx, palIdx);
}

std::vector<uint8_t> ChunkStorage::serialize() const {
    std::vector<uint8_t> out;
    out.push_back(kSerializeMagic);
    out.push_back(kSerializeVersion);
    out.push_back(uniform_ ? 1 : 0);
    appendU16(out, uniformId_);
    appendU16(out, static_cast<uint16_t>(palette_.size()));
    for (voxel::core::BlockId id : palette_) {
        appendU16(out, id);
    }
    out.push_back(static_cast<uint8_t>(bitsPerIndex_));
    appendU32(out, static_cast<uint32_t>(packedIndices_.size()));
    out.insert(out.end(), packedIndices_.begin(), packedIndices_.end());
    return out;
}

ChunkStorage ChunkStorage::deserialize(std::span<const uint8_t> bytes) {
    size_t offset = 0;
    if (bytes.size() < 4) {
        throw std::runtime_error("ChunkStorage deserialize: too short");
    }
    if (bytes[offset++] != kSerializeMagic || bytes[offset++] != kSerializeVersion) {
        throw std::runtime_error("ChunkStorage deserialize: bad header");
    }
    ChunkStorage storage;
    storage.uniform_ = bytes[offset++] != 0;
    storage.uniformId_ = readU16(bytes, offset);
    const uint16_t paletteCount = readU16(bytes, offset);
    storage.palette_.resize(paletteCount);
    for (uint16_t i = 0; i < paletteCount; ++i) {
        storage.palette_[i] = readU16(bytes, offset);
    }
    storage.bitsPerIndex_ = static_cast<int>(bytes[offset++]);
    const uint32_t packedSize = readU32(bytes, offset);
    if (offset + packedSize > bytes.size()) {
        throw std::runtime_error("ChunkStorage deserialize: packed indices truncated");
    }
    storage.packedIndices_.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                                  bytes.begin() + static_cast<std::ptrdiff_t>(offset + packedSize));
    return storage;
}

bool ChunkStorage::operator==(const ChunkStorage& other) const {
    for (int z = 0; z < kChunkSize; ++z) {
        for (int y = 0; y < kChunkSize; ++y) {
            for (int x = 0; x < kChunkSize; ++x) {
                if (get(x, y, z) != other.get(x, y, z)) {
                    return false;
                }
            }
        }
    }
    return true;
}

} // namespace voxel::world
