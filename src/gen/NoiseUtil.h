#pragma once

#include "gen/Biomes.h"

#include <FastNoise/FastNoise.h>

#include <algorithm>

namespace voxel::gen {

inline FastNoise::SmartNode<> makeFractalNoise(const NoiseParams& params) {
    auto simplex = FastNoise::New<FastNoise::Simplex>();
    simplex->SetScale(1.0f / std::max(params.frequency, 1.0e-6f));
    auto fractal = FastNoise::New<FastNoise::FractalFBm>();
    fractal->SetSource(simplex);
    fractal->SetOctaveCount(static_cast<int>(std::max(1, params.octaves)));
    return fractal;
}

} // namespace voxel::gen
