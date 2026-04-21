#pragma once
#include "LiteMath.h"
#include <vector>
#include <string>

// simple structure that contains a density field
// it can be saved and loaded and used in Hydra scene
struct DensityGrid
{
  std::vector<float> data;
  LiteMath::uint3 size;
};

// simple structure that contains a color and a density
struct ColorDensityGrid
{
  std::vector<LiteMath::float4> data;
  LiteMath::uint3 size;
};

// simple structure that contains a set of density grids
// each with a timestamp, so it can be animated
struct DensityMultiGrid
{
  uint32_t frames; // number of frames (grids in data)
  std::vector<float> timestamps; // time in seconds
  std::vector<float> data;
  LiteMath::uint3 size;
};

// simple structure that contains a set of color and density grids
// each with a timestamp, so it can be animated
struct ColorDensityMultiGrid
{
  uint32_t frames; // number of frames (grids in data)
  std::vector<float> timestamps; // time in seconds
  std::vector<LiteMath::float4> data;
  LiteMath::uint3 size;
};

void save_density_grid(const DensityGrid &scene, const std::string &path);
void load_density_grid(DensityGrid &scene, const std::string &path);

void save_density_grid(const ColorDensityGrid &scene, const std::string &path);
void load_density_grid(ColorDensityGrid &scene, const std::string &path);

void save_density_grid(const DensityMultiGrid &scene, const std::string &path);
void load_density_grid(DensityMultiGrid &scene, const std::string &path);

void save_density_grid(const ColorDensityMultiGrid &scene, const std::string &path);
void load_density_grid(ColorDensityMultiGrid &scene, const std::string &path);

struct ModelInfo;
ModelInfo get_info_density_grid(const DensityGrid &scene);
ModelInfo get_info_density_grid(const ColorDensityGrid &scene);
ModelInfo get_info_density_grid(const ColorDensityMultiGrid &scene);
ModelInfo get_info_density_grid(const DensityMultiGrid &scene);