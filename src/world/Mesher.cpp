#include "world/Mesher.h"

#include <array>
#include <bit>

namespace voxel::world {

namespace {

bool isOpaqueSolid(voxel::core::BlockId id, const voxel::core::BlocksFile& blocks) {
    if (id == 0 || id >= blocks.blocks.size()) {
        return false;
    }
    const voxel::core::BlockDef& def = blocks.blocks[id];
    return def.solid && !def.transparent;
}

bool faceVisible(voxel::core::BlockId self, voxel::core::BlockId neighbor,
                 const voxel::core::BlocksFile& blocks) {
    return isOpaqueSolid(self, blocks) && !isOpaqueSolid(neighbor, blocks);
}

voxel::core::BlockId blockAt(const ChunkPos& pos, const ChunkStorage& storage, BlockQueryFn& query, int lx,
                             int ly, int lz) {
    if (lx >= 0 && ly >= 0 && lz >= 0 && lx < kChunkSize && ly < kChunkSize && lz < kChunkSize) {
        return storage.get(lx, ly, lz);
    }
    const int wx = pos.x * kChunkSize + lx;
    const int wy = pos.y * kChunkSize + ly;
    const int wz = pos.z * kChunkSize + lz;
    return query(wx, wy, wz);
}

size_t greedyMergeQuads(std::array<std::array<uint16_t, kChunkSize>, kChunkSize>& grid) {
    size_t quads = 0;
    for (int v = 0; v < kChunkSize; ++v) {
        for (int u = 0; u < kChunkSize; ++u) {
            const uint16_t type = grid[static_cast<size_t>(v)][static_cast<size_t>(u)];
            if (type == 0) {
                continue;
            }
            int width = 1;
            while (u + width < kChunkSize &&
                   grid[static_cast<size_t>(v)][static_cast<size_t>(u + width)] == type) {
                ++width;
            }
            int height = 1;
            bool grow = true;
            while (v + height < kChunkSize && grow) {
                for (int du = 0; du < width; ++du) {
                    if (grid[static_cast<size_t>(v + height)][static_cast<size_t>(u + du)] != type) {
                        grow = false;
                        break;
                    }
                }
                if (grow) {
                    ++height;
                }
            }
            ++quads;
            for (int dv = 0; dv < height; ++dv) {
                for (int du = 0; du < width; ++du) {
                    grid[static_cast<size_t>(v + dv)][static_cast<size_t>(u + du)] = 0;
                }
            }
        }
    }
    return quads;
}

// axis: 0=X, 1=Y, 2=Z. positive=true means face normal points along +axis.
size_t meshAxis(const ChunkPos& pos, const ChunkStorage& storage, BlockQueryFn& query,
                const voxel::core::BlocksFile& blocks, int axis, bool positive) {
    // sliceMask[slice][row] has one bit per column (column index 0..31).
    std::array<std::array<uint32_t, kChunkSize>, kChunkSize + 1> sliceMask{};
    for (int slice = 0; slice <= kChunkSize; ++slice) {
        sliceMask[static_cast<size_t>(slice)].fill(0);
    }

    for (int lx = 0; lx < kChunkSize; ++lx) {
        for (int ly = 0; ly < kChunkSize; ++ly) {
            for (int lz = 0; lz < kChunkSize; ++lz) {
                if (!isOpaqueSolid(blockAt(pos, storage, query, lx, ly, lz), blocks)) {
                    continue;
                }
                int slice = 0;
                int row = 0;
                int col = 0;
                if (axis == 0) {
                    slice = lx + 1;
                    row = ly;
                    col = lz;
                } else if (axis == 1) {
                    slice = ly + 1;
                    row = lx;
                    col = lz;
                } else {
                    slice = lz + 1;
                    row = ly;
                    col = lx;
                }
                sliceMask[static_cast<size_t>(slice)][static_cast<size_t>(row)] |= (1u << col);
            }
        }
    }

    size_t quads = 0;
    std::array<std::array<uint16_t, kChunkSize>, kChunkSize> grid{};

    for (int slice = 0; slice <= kChunkSize; ++slice) {
        for (auto& row : grid) {
            row.fill(0);
        }

        for (int row = 0; row < kChunkSize; ++row) {
            const uint32_t here = sliceMask[static_cast<size_t>(slice)][static_cast<size_t>(row)];
            uint32_t facing = 0;
            if (positive) {
                const uint32_t ahead =
                    slice + 1 <= kChunkSize ? sliceMask[static_cast<size_t>(slice + 1)][static_cast<size_t>(row)]
                                            : 0;
                facing = here & ~ahead;
            } else {
                const uint32_t behind =
                    slice > 0 ? sliceMask[static_cast<size_t>(slice - 1)][static_cast<size_t>(row)] : 0;
                facing = here & ~behind;
            }

            uint32_t bits = facing;
            while (bits != 0) {
                const int col = std::countr_zero(bits);
                bits &= bits - 1;

                int lx = 0;
                int ly = 0;
                int lz = 0;
                if (axis == 0) {
                    lx = slice - 1;
                    ly = row;
                    lz = col;
                } else if (axis == 1) {
                    lx = row;
                    ly = slice - 1;
                    lz = col;
                } else {
                    lx = col;
                    ly = row;
                    lz = slice - 1;
                }

                if (lx < 0 || ly < 0 || lz < 0 || lx >= kChunkSize || ly >= kChunkSize || lz >= kChunkSize) {
                    continue;
                }

                const voxel::core::BlockId self = blockAt(pos, storage, query, lx, ly, lz);
                int nlx = lx;
                int nly = ly;
                int nlz = lz;
                if (axis == 0) {
                    nlx += positive ? 1 : -1;
                } else if (axis == 1) {
                    nly += positive ? 1 : -1;
                } else {
                    nlz += positive ? 1 : -1;
                }
                const voxel::core::BlockId neighbor = blockAt(pos, storage, query, nlx, nly, nlz);
                if (!faceVisible(self, neighbor, blocks)) {
                    continue;
                }

                int u = 0;
                int v = 0;
                if (axis == 0) {
                    u = ly;
                    v = lz;
                } else if (axis == 1) {
                    u = lx;
                    v = lz;
                } else {
                    u = lx;
                    v = ly;
                }
                grid[static_cast<size_t>(v)][static_cast<size_t>(u)] = self;
            }
        }

        quads += greedyMergeQuads(grid);
    }
    return quads;
}

} // namespace

MesherResult meshChunkGreedy(const ChunkPos& pos, const ChunkStorage& storage, BlockQueryFn blockAtWorld,
                              const voxel::core::BlocksFile& blocks) {
    MesherResult result;
    for (int axis = 0; axis < 3; ++axis) {
        result.quadCount += meshAxis(pos, storage, blockAtWorld, blocks, axis, true);
        result.quadCount += meshAxis(pos, storage, blockAtWorld, blocks, axis, false);
    }
    return result;
}

} // namespace voxel::world
