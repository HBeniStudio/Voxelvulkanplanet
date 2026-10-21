#include "world/ChunkManager.h"

#include "world/Mesher.h"

#include <spdlog/spdlog.h>

#include <chrono>
#include <cmath>
#include <thread>

namespace voxel::world {

ChunkManager::ChunkManager() = default;

ChunkManager::~ChunkManager() {
    waitForPendingMeshes(300'000);
    pool_.waitIdle(std::chrono::milliseconds(300'000));
}

void ChunkManager::setWorldGen(const IWorldGen* generator, uint64_t seed) {
    std::lock_guard lock(mutex_);
    generator_ = generator;
    seed_ = seed;
}

void ChunkManager::setBlocks(const voxel::core::BlocksFile* blocks) {
    std::lock_guard lock(mutex_);
    blocks_ = blocks;
}

void ChunkManager::ensureLoaded(ChunkPos pos) {
    {
        std::lock_guard lock(mutex_);
        if (chunks_.contains(pos)) {
            return;
        }
    }

    ChunkStorage storage;
    if (generator_ != nullptr) {
        generator_->generate(pos, storage, seed_);
    } else {
        storage.fill(0);
    }

    bool needsRemesh = false;
    {
        std::lock_guard lock(mutex_);
        if (chunks_.contains(pos)) {
            return;
        }
        ChunkEntry entry;
        entry.storage = std::move(storage);
        chunks_.emplace(pos, std::move(entry));
        needsRemesh = true;
    }
    if (needsRemesh) {
        requestRemesh(pos);
    }
}

void ChunkManager::unloadOutside(const ChunkPos& center, int horizontalDistance, int verticalDistance) {
    std::lock_guard lock(mutex_);
    for (auto it = chunks_.begin(); it != chunks_.end();) {
        const int dx = std::abs(it->first.x - center.x);
        const int dy = std::abs(it->first.y - center.y);
        const int dz = std::abs(it->first.z - center.z);
        if (dx > horizontalDistance || dz > horizontalDistance || dy > verticalDistance) {
            it = chunks_.erase(it);
        } else {
            ++it;
        }
    }
}

void ChunkManager::updateAround(const ChunkPos& center, int horizontalDistance, int verticalDistance) {
    for (int dz = -horizontalDistance; dz <= horizontalDistance; ++dz) {
        for (int dy = -verticalDistance; dy <= verticalDistance; ++dy) {
            for (int dx = -horizontalDistance; dx <= horizontalDistance; ++dx) {
                ensureLoaded({center.x + dx, center.y + dy, center.z + dz});
            }
        }
    }
    unloadOutside(center, horizontalDistance, verticalDistance);
    pumpCompletedMeshes();
}

voxel::core::BlockId ChunkManager::queryWorld(int wx, int wy, int wz) const {
    const int cx = wx >= 0 ? wx / kChunkSize : (wx - kChunkSize + 1) / kChunkSize;
    const int cy = wy >= 0 ? wy / kChunkSize : (wy - kChunkSize + 1) / kChunkSize;
    const int cz = wz >= 0 ? wz / kChunkSize : (wz - kChunkSize + 1) / kChunkSize;
    const ChunkPos key{cx, cy, cz};
    std::lock_guard lock(mutex_);
    const auto it = chunks_.find(key);
    if (it == chunks_.end()) {
        return 0;
    }
    const int lx = wx - cx * kChunkSize;
    const int ly = wy - cy * kChunkSize;
    const int lz = wz - cz * kChunkSize;
    return it->second.storage.get(lx, ly, lz);
}

void ChunkManager::requestRemesh(ChunkPos pos) {
    if (blocks_ == nullptr) {
        std::lock_guard lock(mutex_);
        const auto it = chunks_.find(pos);
        if (it != chunks_.end()) {
            it->second.meshReady = true;
            it->second.meshing = false;
        }
        return;
    }
    ChunkStorage storageCopy;
    {
        std::lock_guard lock(mutex_);
        const auto it = chunks_.find(pos);
        if (it == chunks_.end()) {
            return;
        }
        if (it->second.meshing) {
            return;
        }
        it->second.meshing = true;
        storageCopy = it->second.storage;
    }

    const voxel::core::BlocksFile* blocks = blocks_;
    pool_.enqueue([this, pos, storageCopy, blocks]() {
        BlockQueryFn query = [this](int wx, int wy, int wz) { return queryWorld(wx, wy, wz); };
        const MesherResult meshed = meshChunkGreedy(pos, storageCopy, query, *blocks);

        PendingMesh pending{pos, meshed.quads};
        std::lock_guard lock(pendingMutex_);
        pendingMeshes_.push_back(std::move(pending));
    });
}

void ChunkManager::pumpCompletedMeshes() {
    std::vector<PendingMesh> pending;
    {
        std::lock_guard lock(pendingMutex_);
        pending.swap(pendingMeshes_);
    }
    std::lock_guard lock(mutex_);
    for (PendingMesh& mesh : pending) {
        auto it = chunks_.find(mesh.pos);
        if (it == chunks_.end()) {
            continue;
        }
        it->second.quads = std::move(mesh.quads);
        it->second.meshReady = true;
        it->second.meshing = false;
    }
}

voxel::core::BlockId ChunkManager::getBlock(int wx, int wy, int wz) const {
    return queryWorld(wx, wy, wz);
}

void ChunkManager::requestRemeshWithNeighbors(ChunkPos pos, int localX, int localY, int localZ) {
    requestRemesh(pos);
    if (localX == 0) {
        requestRemesh({pos.x - 1, pos.y, pos.z});
    }
    if (localX == kChunkSize - 1) {
        requestRemesh({pos.x + 1, pos.y, pos.z});
    }
    if (localY == 0) {
        requestRemesh({pos.x, pos.y - 1, pos.z});
    }
    if (localY == kChunkSize - 1) {
        requestRemesh({pos.x, pos.y + 1, pos.z});
    }
    if (localZ == 0) {
        requestRemesh({pos.x, pos.y, pos.z - 1});
    }
    if (localZ == kChunkSize - 1) {
        requestRemesh({pos.x, pos.y, pos.z + 1});
    }
}

void ChunkManager::setBlock(int wx, int wy, int wz, voxel::core::BlockId id) {
    const int cx = wx >= 0 ? wx / kChunkSize : (wx - kChunkSize + 1) / kChunkSize;
    const int cy = wy >= 0 ? wy / kChunkSize : (wy - kChunkSize + 1) / kChunkSize;
    const int cz = wz >= 0 ? wz / kChunkSize : (wz - kChunkSize + 1) / kChunkSize;
    const ChunkPos pos{cx, cy, cz};
    ensureLoaded(pos);
    const int lx = wx - cx * kChunkSize;
    const int ly = wy - cy * kChunkSize;
    const int lz = wz - cz * kChunkSize;
    {
        std::lock_guard lock(mutex_);
        chunks_[pos].storage.set(lx, ly, lz, id);
    }
    requestRemeshWithNeighbors(pos, lx, ly, lz);
}

void ChunkManager::setChunkStorage(ChunkPos pos, ChunkStorage storage) {
    {
        std::lock_guard lock(mutex_);
        ChunkEntry& entry = chunks_[pos];
        entry.storage = std::move(storage);
        entry.meshReady = false;
        entry.meshing = false;
    }
    requestRemesh(pos);
}

std::vector<ChunkPos> ChunkManager::loadedChunkPositions() const {
    std::lock_guard lock(mutex_);
    std::vector<ChunkPos> out;
    out.reserve(chunks_.size());
    for (const auto& [pos, _] : chunks_) {
        out.push_back(pos);
    }
    return out;
}

const std::vector<MeshQuad>& ChunkManager::meshQuads(ChunkPos pos) const {
    std::lock_guard lock(mutex_);
    const auto it = chunks_.find(pos);
    if (it == chunks_.end()) {
        static const std::vector<MeshQuad> kEmpty;
        return kEmpty;
    }
    return it->second.quads;
}

bool ChunkManager::hasMeshingChunks() const {
    std::lock_guard lock(mutex_);
    for (const auto& [_, entry] : chunks_) {
        if (entry.meshing) {
            return true;
        }
    }
    return false;
}

void ChunkManager::logMeshStatus() const {
    size_t loaded = 0;
    size_t meshed = 0;
    size_t meshing = 0;
    {
        std::lock_guard lock(mutex_);
        loaded = chunks_.size();
        for (const auto& [_, entry] : chunks_) {
            if (entry.meshReady) {
                ++meshed;
            }
            if (entry.meshing) {
                ++meshing;
            }
        }
    }
    spdlog::info("ChunkManager: {} loaded, {} meshed, {} meshing", loaded, meshed, meshing);
}

void ChunkManager::waitForPendingMeshes(int maxWaitMs) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(maxWaitMs);
    while (std::chrono::steady_clock::now() < deadline) {
        pumpCompletedMeshes();
        while (pool_.tryExecuteOne()) {
        }
        if (!hasMeshingChunks()) {
            pool_.waitIdle(std::chrono::milliseconds(50));
            pumpCompletedMeshes();
            if (!hasMeshingChunks()) {
                return;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    spdlog::warn("ChunkManager: mesh wait timed out after {} ms; clearing stuck meshing flags", maxWaitMs);
    {
        std::lock_guard lock(mutex_);
        for (auto& [_, entry] : chunks_) {
            if (entry.meshing) {
                entry.meshing = false;
                entry.meshReady = true;
            }
        }
    }
    while (pool_.tryExecuteOne()) {
    }
    pumpCompletedMeshes();
    pool_.waitIdle(std::chrono::milliseconds(60'000));
}

const ChunkStorage* ChunkManager::chunkStorage(ChunkPos pos) const {
    std::lock_guard lock(mutex_);
    const auto it = chunks_.find(pos);
    if (it == chunks_.end()) {
        return nullptr;
    }
    return &it->second.storage;
}

} // namespace voxel::world
