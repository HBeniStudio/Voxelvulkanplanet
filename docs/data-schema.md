# Data schema v1.1 (world generation)

Schema revision **v1.1** extends v1 without renaming existing fields. The JSON `schema` integer remains **`1`**. Validators reject unknown `schema` values, missing required fields, and block ids that are not defined in `data/blocks.json`.

### Changes in v1.1

1. **Counts:** At least **four** biomes, including the required names `plains`, `forest`, `desert`, and `mountains` (additional biomes allowed). At least **one** ore and **one** tree definition.
2. **Per-biome `trees` (optional):** Array of tree archetype ids. When present, only those archetypes may spawn in that biome; when omitted, any top-level tree may spawn (subject to `tree_density`).
3. **Snow on peaks (optional):** Per-biome `snow_line` (world Y) and `snow_block` (block id). Both must be set together. When the column surface height is **≥ `snow_line`**, the top surface voxel uses `snow_block` instead of `surface_block`.
4. **Trees and blocks:** `log_block` may reference any block id (e.g. `pine_log`, `cactus`). `leaves_block` is **optional**; when omitted or set to **`0` (air)**, the engine places no leaf canopy (cactus-style).

Block ids **13–16** in `data/blocks.json`: `pine_log`, `pine_leaves`, `cactus`, `snow` (append-only).

## `data/biomes.json` (world generation root)

Single file for biome layout, ores, trees, and shared generation parameters.

### Top level

| Field | Type | Description |
|-------|------|-------------|
| `schema` | int | Must be `1`. |
| `sea_level` | int | Y coordinate of water surface (inclusive). |
| `bedrock_layers` | int | Solid bedrock from `y = 0` through `y = bedrock_layers - 1`. |
| `region` | object | `size_x`, `size_y`, `size_z` (positive integers). |
| `height_noise` | object | 2D fractal height sampler (see Noise). |
| `biome_noise` | object | 2D noise used to pick a biome column. |
| `cave_noise` | object | 3D noise parameters for caves. |
| `biomes` | array | ≥ 4 entries; must include the four required names. |
| `ores` | array | ≥ 1 ore definition. |
| `trees` | array | ≥ 1 tree definition. |

### Noise object

| Field | Type | Description |
|-------|------|-------------|
| `frequency` | number | Base frequency for FastNoise2. |
| `octaves` | int | Fractal octave count (≥ 1). |
| `seed_offset` | int | Added to world seed. |

### Biome object

| Field | Type | Description |
|-------|------|-------------|
| `id` | int | Contiguous from 0. |
| `name` | string | Must include `plains`, `forest`, `desert`, `mountains`. |
| `surface_block` | int | Top solid block at surface. |
| `subsurface_block` | int | 1–3 blocks below surface. |
| `filler_block` | int | Deeper fill to bedrock. |
| `height_base` | int | Mean surface height before noise. |
| `height_amplitude` | number | Height noise scale (blocks). |
| `tree_density` | number | 0..1 spawn weight for trees. |
| `noise_frequency` | number | Per-biome height noise multiplier. |
| `trees` | int[] | **Optional.** Allowed tree archetype ids for this biome. |
| `snow_line` | int | **Optional.** With `snow_block`, surface Y threshold for snow cap. |
| `snow_block` | int | **Optional.** Block id for snow-capped surface (requires `snow_line`). |

### Ore object

| Field | Type | Description |
|-------|------|-------------|
| `block` | int | Ore block id. |
| `name` | string | Label. |
| `min_y` | int | Minimum world Y (inclusive). |
| `max_y` | int | Maximum world Y (inclusive). |
| `vein_frequency` | number | 3D noise threshold scale. |
| `vein_size` | number | Vein scatter scale. |

### Tree object

| Field | Type | Description |
|-------|------|-------------|
| `id` | int | Tree archetype id. |
| `log_block` | int | Trunk block (non-air). |
| `leaves_block` | int | **Optional.** Canopy block; omit or `0` for no leaves. |
| `min_height` | int | Minimum trunk height. |
| `max_height` | int | Maximum trunk height. |
| `canopy_radius` | int | Horizontal leaf radius (ignored when no leaves). |

### Wishlist

- Biome-specific ore tables, structure prefabs, and art-final textures for pine/cactus/snow.
