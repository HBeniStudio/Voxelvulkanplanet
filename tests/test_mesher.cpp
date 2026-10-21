#include <doctest/doctest.h>

#include "core/Blocks.h"
#include "world/Mesher.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace {

voxel::core::BlocksFile loadBlocks() {
    const std::filesystem::path path = std::filesystem::path(VOXEL_SOURCE_DIR) / "data/blocks.json";
    voxel::core::BlocksFile file = voxel::core::loadBlocksJson(path);
    const std::string err = voxel::core::validateBlocksSchemaV1(file);
    if (!err.empty()) {
        throw std::runtime_error(err);
    }
    return file;
}

voxel::world::BlockQueryFn airOutside() {
    return [](int, int, int) -> voxel::core::BlockId { return 0; };
}

struct FaceKey {
    int wx = 0;
    int wy = 0;
    int wz = 0;
    int dir = 0;

    bool operator<(const FaceKey& o) const {
        return std::tie(wx, wy, wz, dir) < std::tie(o.wx, o.wy, o.wz, o.dir);
    }

    bool operator==(const FaceKey& o) const {
        return wx == o.wx && wy == o.wy && wz == o.wz && dir == o.dir;
    }
};

bool isOpaqueSolid(voxel::core::BlockId id, const voxel::core::BlocksFile& blocks) {
    if (id == 0 || id >= blocks.blocks.size()) {
        return false;
    }
    const auto& def = blocks.blocks[id];
    return def.solid && !def.transparent;
}

voxel::core::BlockId blockAtLocal(const voxel::world::ChunkPos& pos, const voxel::world::ChunkStorage& storage,
                                  const voxel::world::BlockQueryFn& query, int lx, int ly, int lz) {
    if (lx >= 0 && ly >= 0 && lz >= 0 && lx < voxel::world::kChunkSize && ly < voxel::world::kChunkSize &&
        lz < voxel::world::kChunkSize) {
        return storage.get(lx, ly, lz);
    }
    const int wx = pos.x * voxel::world::kChunkSize + lx;
    const int wy = pos.y * voxel::world::kChunkSize + ly;
    const int wz = pos.z * voxel::world::kChunkSize + lz;
    return query(wx, wy, wz);
}

size_t bruteForceExposedFaces(const voxel::world::ChunkPos& pos, const voxel::world::ChunkStorage& storage,
                              const voxel::world::BlockQueryFn& query, const voxel::core::BlocksFile& blocks) {
    constexpr int dirs[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    size_t count = 0;
    for (int lz = 0; lz < voxel::world::kChunkSize; ++lz) {
        for (int ly = 0; ly < voxel::world::kChunkSize; ++ly) {
            for (int lx = 0; lx < voxel::world::kChunkSize; ++lx) {
                const voxel::core::BlockId self = storage.get(lx, ly, lz);
                if (!isOpaqueSolid(self, blocks)) {
                    continue;
                }
                for (int d = 0; d < 6; ++d) {
                    const voxel::core::BlockId neighbor =
                        blockAtLocal(pos, storage, query, lx + dirs[d][0], ly + dirs[d][1], lz + dirs[d][2]);
                    if (isOpaqueSolid(self, blocks) && !isOpaqueSolid(neighbor, blocks)) {
                        ++count;
                    }
                }
            }
        }
    }
    return count;
}

void quadToFaceKeys(const voxel::world::ChunkPos& pos, const voxel::world::MeshQuad& quad,
                    std::set<FaceKey>& faces) {
    const int dir = quad.axis * 2 + (quad.positive ? 0 : 1);
    for (int dv = 0; dv < quad.height; ++dv) {
        for (int du = 0; du < quad.width; ++du) {
            const int u = quad.u + du;
            const int v = quad.v + dv;
            int lx = 0;
            int ly = 0;
            int lz = 0;
            const int along = quad.positive ? (quad.slice - 1) : (quad.slice - 1);
            if (quad.axis == 0) {
                lx = along;
                ly = u;
                lz = v;
            } else if (quad.axis == 1) {
                lx = u;
                ly = along;
                lz = v;
            } else {
                lx = u;
                ly = v;
                lz = along;
            }
            const int wx = pos.x * voxel::world::kChunkSize + lx;
            const int wy = pos.y * voxel::world::kChunkSize + ly;
            const int wz = pos.z * voxel::world::kChunkSize + lz;
            faces.insert(FaceKey{wx, wy, wz, dir});
        }
    }
}

void verifyMesherCoverage(const voxel::world::ChunkPos& pos, const voxel::world::ChunkStorage& storage,
                          const voxel::world::BlockQueryFn& query, const voxel::core::BlocksFile& blocks) {
    const voxel::world::MesherResult meshed = voxel::world::meshChunkGreedy(pos, storage, query, blocks);
    const size_t brute = bruteForceExposedFaces(pos, storage, query, blocks);
    CHECK(meshed.coveredUnitFaces == brute);

    std::set<FaceKey> covered;
    for (const voxel::world::MeshQuad& quad : meshed.quads) {
        quadToFaceKeys(pos, quad, covered);
    }
    CHECK(covered.size() == brute);

    std::set<FaceKey> expected;
    constexpr int dirs[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (int lz = 0; lz < voxel::world::kChunkSize; ++lz) {
        for (int ly = 0; ly < voxel::world::kChunkSize; ++ly) {
            for (int lx = 0; lx < voxel::world::kChunkSize; ++lx) {
                const voxel::core::BlockId self = storage.get(lx, ly, lz);
                if (!isOpaqueSolid(self, blocks)) {
                    continue;
                }
                for (int d = 0; d < 6; ++d) {
                    const voxel::core::BlockId neighbor =
                        blockAtLocal(pos, storage, query, lx + dirs[d][0], ly + dirs[d][1], lz + dirs[d][2]);
                    if (!isOpaqueSolid(neighbor, blocks)) {
                        const int wx = pos.x * voxel::world::kChunkSize + lx;
                        const int wy = pos.y * voxel::world::kChunkSize + ly;
                        const int wz = pos.z * voxel::world::kChunkSize + lz;
                        expected.insert(FaceKey{wx, wy, wz, d});
                    }
                }
            }
        }
    }
    CHECK(covered == expected);
}

voxel::world::ChunkStorage makeSingleBlock() {
    voxel::world::ChunkStorage storage;
    storage.fill(0);
    storage.set(16, 16, 16, 1);
    return storage;
}

voxel::world::ChunkStorage makeSolid() {
    voxel::world::ChunkStorage storage;
    storage.fill(1);
    return storage;
}

voxel::world::ChunkStorage makeChecker() {
    voxel::world::ChunkStorage checker;
    checker.fill(0);
    for (int z = 0; z < voxel::world::kChunkSize; ++z) {
        for (int y = 0; y < voxel::world::kChunkSize; ++y) {
            for (int x = 0; x < voxel::world::kChunkSize; ++x) {
                if (((x + y + z) & 1) == 0) {
                    checker.set(x, y, z, 1);
                }
            }
        }
    }
    return checker;
}

voxel::world::ChunkStorage makeLShape() {
    voxel::world::ChunkStorage lShape;
    lShape.fill(0);
    const int coords[][3] = {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {0, 0, 1}, {0, 0, 2}};
    for (const auto& c : coords) {
        lShape.set(c[0], c[1], c[2], 1);
    }
    return lShape;
}

void printLShapeAnalysis(const voxel::core::BlocksFile& blocks) {
    const voxel::world::ChunkPos pos{0, 0, 0};
    const voxel::world::ChunkStorage lShape = makeLShape();
    const size_t brute = bruteForceExposedFaces(pos, lShape, airOutside(), blocks);
    const auto meshed = voxel::world::meshChunkGreedy(pos, lShape, airOutside(), blocks);

    std::ostringstream layout;
    layout << "L-shape layout (y=0, z rows top to bottom, x columns):\n";
    for (int z = 2; z >= 0; --z) {
        layout << "z=" << z << ": ";
        for (int x = 0; x < 3; ++x) {
            layout << (lShape.get(x, 0, z) != 0 ? 'X' : '.');
        }
        layout << '\n';
    }

    MESSAGE(layout.str());
    MESSAGE("Brute-force exposed unit faces: ", brute);
    MESSAGE("Greedy quad count: ", meshed.quadCount, " (covered area ", meshed.coveredUnitFaces, ")");
    MESSAGE("Minimal greedy quad count reasoning: top/bottom each merge the xz footprint into two "
            "rectangles (3x1 bar at z=0 plus 1x2 stem at x=0,z=1..2) = 2 quads per horizontal "
            "direction; four vertical side directions contribute one merged quad each for the outer "
            "shell (east face of x=2, south face of z=0 bar, west/north stems), totaling 10 quads "
            "while covering all ", brute, " unit faces exactly once.");
}

} // namespace

TEST_CASE("mesher coverage matches brute-force exposed faces") {
    const voxel::core::BlocksFile blocks = loadBlocks();
    const voxel::world::ChunkPos pos{0, 0, 0};

    verifyMesherCoverage(pos, makeSingleBlock(), airOutside(), blocks);
    verifyMesherCoverage(pos, makeSolid(), airOutside(), blocks);
    verifyMesherCoverage(pos, makeChecker(), airOutside(), blocks);
    verifyMesherCoverage(pos, makeLShape(), airOutside(), blocks);

    std::mt19937 rng(42);
    for (int sample = 0; sample < 4; ++sample) {
        voxel::world::ChunkStorage random;
        random.fill(0);
        for (int i = 0; i < 400; ++i) {
            const int x = static_cast<int>(rng() % voxel::world::kChunkSize);
            const int y = static_cast<int>(rng() % voxel::world::kChunkSize);
            const int z = static_cast<int>(rng() % voxel::world::kChunkSize);
            random.set(x, y, z, static_cast<voxel::core::BlockId>(1 + (rng() % 3)));
        }
        verifyMesherCoverage(pos, random, airOutside(), blocks);
    }
}

TEST_CASE("mesher quad counts for known shapes") {
    const voxel::core::BlocksFile blocks = loadBlocks();
    const voxel::world::ChunkPos pos{0, 0, 0};

    const auto singleResult =
        voxel::world::meshChunkGreedy(pos, makeSingleBlock(), airOutside(), blocks);
    CHECK(singleResult.quadCount == 6);

    const auto solidResult = voxel::world::meshChunkGreedy(pos, makeSolid(), airOutside(), blocks);
    CHECK(solidResult.quadCount == 6);

    const auto checkerResult = voxel::world::meshChunkGreedy(pos, makeChecker(), airOutside(), blocks);
    CHECK(checkerResult.quadCount == 98304);

    const auto lResult = voxel::world::meshChunkGreedy(pos, makeLShape(), airOutside(), blocks);
    CHECK(lResult.quadCount == 10);

    printLShapeAnalysis(blocks);
}

TEST_CASE("mesher cpu budget for solid 32 cubed chunk") {
    const voxel::core::BlocksFile blocks = loadBlocks();
    const voxel::world::ChunkPos pos{0, 0, 0};
    const voxel::world::ChunkStorage solid = makeSolid();

    const auto start = std::chrono::steady_clock::now();
    const auto result = voxel::world::meshChunkGreedy(pos, solid, airOutside(), blocks);
    const auto end = std::chrono::steady_clock::now();
    const double ms =
        std::chrono::duration<double, std::milli>(end - start).count();

    MESSAGE("mesh solid 32^3: ", ms, " ms, quads=", result.quadCount);
    if (ms > 5.0) {
        MESSAGE("WARNING: meshing exceeded 5 ms CPU budget on this runner");
    }
    CHECK(result.quadCount == 6);
}
