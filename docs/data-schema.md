# Data schema v1 (world generation)

Schema version **1** is frozen for P4. Validators reject unknown `schema` values, missing required fields, and block ids that are not defined in `data/blocks.json`.

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
| `ores` | array | Ore vein definitions (see Ore). |
| `trees` | array | Tree shape definitions (see Tree). |

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
