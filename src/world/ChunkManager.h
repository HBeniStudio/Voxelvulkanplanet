#pragma once

#include "core/Blocks.h"
#include "core/ThreadPool.h"
#include "world/Chunk.h"
#include "world/ChunkStorage.h"
#include "world/IWorldGen.h"
#include "world/Mesher.h"

#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace voxel::world {

class ChunkManager {
public:
    static constexpr int kDefaultHorizontalDistance = 8;
    static constexpr int kDefaultVerticalDistance = 4;

    ChunkManager();
    ~ChunkManager();

    void setWorldGen(const IWorldGen* generator, uint64_t seed);
    void setBlocks(const voxel::core::BlocksFile* blocks);

    void updateAround(const ChunkPos& center, int horizontalDistance = kDefaultHorizontalDistance,
                      int verticalDistance = kDefaultVerticalDistance);

    [[nodiscard]] voxel::core::BlockId getBlock(int wx, int wy, int wz) const;
    void setBlock(int wx, int wy, int wz, voxel::core::BlockId id);

    void setChunkStorage(ChunkPos pos, ChunkStorage storage);

    void pumpCompletedMeshes();
    /// Wait until no chunk has an in-flight mesh job (wall-clock cap in milliseconds).
    void waitForPendingMeshes(int maxWaitMs = 120'000);

    void logMeshStatus() const;

    [[nodiscard]] std::vector<ChunkPos> loadedChunkPositions() const;
    [[nodiscard]] const std::vector<MeshQuad>& meshQuads(ChunkPos pos) const;

    [[nodiscard]] const ChunkStorage* chunkStorage(ChunkPos pos) const;

private:
    struct ChunkEntry {
        ChunkStorage storage;
        std::vector<MeshQuad> quads;
        bool meshReady = false;
        bool meshing = false;
    };

    const IWorldGen* generator_ = nullptr;
    uint64_t seed_ = 0;
    const voxel::core::BlocksFile* blocks_ = nullptr;
    voxel::core::ThreadPool pool_{2};

    mutable std::mutex mutex_;
    std::unordered_map<ChunkPos, ChunkEntry, ChunkPosHash> chunks_;

    struct PendingMesh {
        ChunkPos pos;
        std::vector<MeshQuad> quads;
    };
    std::mutex pendingMutex_;
    std::vector<PendingMesh> pendingMeshes_;

    void ensureLoaded(ChunkPos pos);
    void unloadOutside(const ChunkPos& center, int horizontalDistance, int verticalDistance);
    void requestRemesh(ChunkPos pos);
    [[nodiscard]] bool hasMeshingChunks() const;
    void requestRemeshWithNeighbors(ChunkPos pos, int localX, int localY, int localZ);
    [[nodiscard]] voxel::core::BlockId queryWorld(int wx, int wy, int wz) const;
};

} // namespace voxel::world
