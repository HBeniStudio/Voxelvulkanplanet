# Voxel (working title)

C++20 / Vulkan 1.2 voxel survival — **P1 Half A**: CMake + vcpkg toolchain, volk + vk-bootstrap + VMA, headless offscreen triangle render, core registry/logging/config.

## Build (Linux)

Prerequisites: CMake 3.25+, Ninja, a C++20 compiler, [vcpkg](https://vcpkg.io/), and the Vulkan SDK `glslc` on `PATH`.

```bash
export VCPKG_ROOT=/path/to/vcpkg
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build
```

## Headless render (CI-style)

```bash
./build/voxel --headless --frames 1 --out frame.png
```

Uses Lavapipe or a discrete GPU; no window or swapchain.

## Architecture

- Systems register with `REGISTER_SYSTEM(Name, Phase::Update)` in their own translation unit; `src/main.cpp` is not edited for new features.
- Simulation code must not depend on Vulkan (future phases).

## Contracts v1 (frozen)

Declared in code where P1 files allow; the rest are specified here until later headers land.

| Contract | Summary |
|----------|---------|
| `BlockId` | `uint16_t`; block defs in `data/blocks.json` schema v1 (Half B) |
| Textures | 16×16 PNG per face name under `assets/textures/`, array texture at build time |
| `Chunk` | 32³ cubic, `ChunkPos {int32 x,y,z}`, palette + bit-packed indices (`ChunkStorage.h`, P3) |
| `IWorldGen` | `void generate(ChunkPos, ChunkStorage&, uint64 seed) const` |
| `ISystem` | `virtual void update(World&, float dt)` — see `src/core/Registry.h` |
| `Entity` | `{ id, pos (dvec3), vel, aabb, health, hunger, inventory }` |
| Recipes | `data/recipes.json`: inputs/output/station |
| Save | Region files `saves/<world>/r.X.Y.Z.bin`, header `{ magic "VXSV", version 1 }` |

Block ids 0–12: air, stone, dirt, grass, sand, water, log, leaves, coal_ore, iron_ore, bedrock, planks, crafting_table.

**Note (Half A):** FastNoise2 is planned for P4 but has no upstream vcpkg port yet; it will be added via overlay or FetchContent in a follow-up (see PR wishlist).

## License

Dependencies are MIT/BSD/zlib only. No code from Veloren (GPL) or Luanti (LGPL).
