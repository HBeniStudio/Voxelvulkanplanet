# Data schema v1.1 (world generation)

Schema revision **v1.1** extends v1 without renaming existing fields. The JSON `schema` integer remains **`1`** for all v1.x files. Validators reject unknown `schema` values, missing required fields, and block ids that are not defined in `data/blocks.json`.

### Changes in v1.1

1. **Variable counts:** `biomes`, `ores`, and `trees` arrays may contain any length **≥ 1** (no fixed counts). The terrain generator still expects the four named biomes `plains`, `forest`, `desert`, and `mountains` to be present for correct biome_noise mapping.
2. **Per-biome tree list (optional):** Biome objects may include `trees`: an array of tree archetype ids from the top-level `trees` array. When present, only those archetypes may spawn in that biome. When omitted, the engine may use any top-level tree definition (subject to `tree_density`).

## `data/biomes.json` (world generation root)

Single file for biome layout, ores, trees, and shared generation parameters. Ore and tree definitions live **inside** this file (not separate files).

### Top level

| Field | Type | Description |
|-------|------|-------------|
| `schema` | int | Must be `1`. |
| `sea_level` | int | Y coordinate of water surface (inclusive). |
| `bedrock_layers` | int | Solid bedrock from `y = 0` through `y = bedrock_layers - 1`. |
| `region` | object | Finite world size in blocks: `size_x`, `size_y`, `size_z` (positive integers). |
| `height_noise` | object | 2D fractal height sampler (see Noise). |
| `biome_noise` | object | 2D noise used to pick a biome column (see Noise). |
| `cave_noise` | object | 3D noise parameters for caves (P4 engine; data may be placeholder). |
| `biomes` | array | At least one biome entry (see Biome). |
| `ores` | array | At least one ore vein definition (see Ore). |
| `trees` | array | At least one tree shape definition (see Tree). |

### Noise object

| Field | Type | Description |
|-------|------|-------------|
| `frequency` | number | Base frequency passed to FastNoise2. |
| `octaves` | int | Fractal octave count (≥ 1). |
| `seed_offset` | int | Added to world seed for this noise layer. |

### Biome object

| Field | Type | Description |
|-------|------|-------------|
| `id` | int | Unique biome id (0..n-1, contiguous). |
| `name` | string | Debug name (`plains`, `forest`, `desert`, `mountains` in shipped data). |
| `surface_block` | int | Block id for top solid voxel at or below local surface. |
| `subsurface_block` | int | Block id for 1–3 voxels below surface (dirt/sand). |
| `filler_block` | int | Block id from subsurface depth down to bedrock (usually stone). |
| `height_base` | int | Mean surface height before noise. |
| `height_amplitude` | number | Scales height noise (blocks). |
| `tree_density` | number | 0..1 probability weight for trees on surface (engine uses in forest/plains). |
| `noise_frequency` | number | Per-biome multiplier on `height_noise.frequency` for column height. |
| `trees` | int[] | **Optional (v1.1).** Tree archetype ids allowed in this biome. When omitted, any top-level tree may be used. |

### Ore object

| Field | Type | Description |
|-------|------|-------------|
| `block` | int | Ore block id (e.g. 8 coal, 9 iron). |
| `name` | string | Label for errors/logging. |
| `min_y` | int | Minimum world Y (inclusive). |
| `max_y` | int | Maximum world Y (inclusive). |
| `vein_frequency` | number | 3D noise threshold scale for placing ore. |
| `vein_size` | number | Vein scatter scale (engine-defined). |

### Tree object

| Field | Type | Description |
|-------|------|-------------|
| `id` | int | Tree archetype id. |
| `log_block` | int | Trunk block id (6). |
| `leaves_block` | int | Canopy block id (7). |
| `min_height` | int | Minimum trunk height (blocks). |
| `max_height` | int | Maximum trunk height (blocks). |
| `canopy_radius` | int | Horizontal leaf radius. |

Block ids must exist in `data/blocks.json` (schema v1, ids 0..12).

### Wishlist (out of scope for v1.1)

- New blocks and tree types: cactus, pine, snow-covered variants.
- Snow-on-peaks placement rule tied to elevation or biome.
