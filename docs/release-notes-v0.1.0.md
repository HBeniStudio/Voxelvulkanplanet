# Voxel v0.1.0

First public preview of the C++20 / Vulkan 1.2 voxel engine. Development is **paused after P4**; survival gameplay (P5) has not started.

## What shipped by phase

| Phase | Deliverables |
|-------|----------------|
| **P1** | CMake + vcpkg toolchain, volk / vk-bootstrap / VMA, core registry / logging / config, headless offscreen rendering, block registry from JSON, GitHub Actions CI on Linux (Lavapipe). |
| **P2** | Swapchain desktop app, chunk renderer, texture array, frustum culling, directional light and fog, `voxel_desktop`, headless `test_cube` golden scene. |
| **P3** | 32³ palette chunks, greedy mesher, chunk streaming, region save files (`VXSV` v1), storage and mesher tests. |
| **P4** | Procedural terrain: FastNoise2 height/biome noise, caves, ores, trees, world border; `data/biomes.json` **schema v1.1**; terrain headless golden (`--scene terrain --seed 42`); desktop terrain streaming. |

## Known gaps

- Textures are simple procedural placeholders (16×16 PNGs).
- **Windows desktop build has not been verified on physical GPU hardware** in this release pipeline; only Linux CI (Lavapipe) runs automated render tests.
- Region files are **uncompressed** on disk.
- **P5 (survival)** — inventory, crafting gameplay, entities, combat — not started.

## Windows: how to run

1. Install or update your GPU driver so **Vulkan 1.2+** is available (the engine requests Vulkan 1.2 via vk-bootstrap).
2. Unzip `voxel-v0.1.0-windows-x64.zip` to a folder. Keep `voxel_desktop.exe`, `voxel.exe`, `data/`, `shaders/`, and `assets/` together.
3. Open a terminal in that folder.

**Desktop (windowed):**

```text
voxel_desktop.exe --scene terrain --width 1280 --height 720
```

Supported scenes: `terrain` (procedural world with streaming) or `test_cube` (small fixed test layout).

**Controls** (desktop, both scenes):

- **W / A / S / D** — move horizontally along view axes
- **Space** — move up
- **Left Shift** — move down
- **Mouse** — look (cursor captured)
- Close the window to exit (no in-game menu yet)

**Common CLI flags** (shared by `voxel` and `voxel_desktop` unless noted):

| Flag | Meaning |
|------|---------|
| `--scene <name>` | `triangle` (headless only), `test_cube`, or `terrain` |
| `--seed <N>` | World seed for terrain generation (default **42**) |
| `--stream-distance <N>` | Horizontal chunk stream radius for terrain (default **8**; vertical is half, minimum 1) |
| `--width <N>` / `--height <N>` | Window or offscreen size (default **256×256**) |
| `--log-level <level>` | spdlog level (default `info`) |
| `--planet-curvature` | Enable planet curvature shader path (experimental) |
| `--headless` | Headless mode (`voxel` only; `voxel_desktop` rejects this) |
| `--frames <N>` | Headless frame count (default 1) |
| `--out <path.png>` | Headless output image |
| `--help` / `-h` | Print usage and exit |

Example headless terrain capture (same as CI):

```text
voxel.exe --headless --scene terrain --seed 42 --stream-distance 4 --frames 1 --width 256 --height 256 --out frame.png
```

## Linux

CI builds on Ubuntu 24.04 with Lavapipe. A Linux tarball may be attached to this release when the optional packaging job succeeds; build from source using `README.md` otherwise.

## Assets in the Windows zip

- `voxel.exe`, `voxel_desktop.exe`
- Runtime DLLs from vcpkg (not `vulkan-1.dll` — use the loader from your GPU driver)
- `data/` — `blocks.json`, `biomes.json`
- `shaders/*.spv` — compiled SPIR-V
- `assets/textures/` — block face textures
- `README-windows.txt`
