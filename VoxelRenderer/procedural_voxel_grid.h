#pragma once
#include "LiteMath.h"
#include "Image2d.h"
#include <vector>

using LiteMath::int2;
using LiteMath::int3;
using LiteMath::int4;
using LiteMath::uint2;
using LiteMath::uint3;
using LiteMath::uint4;
using LiteMath::float2;
using LiteMath::float3;
using LiteMath::float4;

struct Tree
{
	enum class Type
	{
		Pine,
		Spruce
	};

	int height = 0;
	int size = 0; //half size of the box
	Type type = Type::Pine;
};

struct TerrainSettings
{
  float hmap_scale = 1.0f;
  float hmap_min = 0.2f;
  float hmap_max = 0.25f;
  float hmap_base_freq = 0.5f;
  int   hmap_bands = 6;

  int   mountains_count = 6;
  int   mountains_base_radius = 200;
  float mountains_min_h = 0.5f;
  float mountains_max_h = 0.7f;
  int   mountains_deform_vector_count = 4;
  
  float tree_spawn_chance = 0.33f;
  int   tree_region_size = 4;
};

struct Terrain
{
  float water_level = 0.5f;
  float hmap_scale = 1.0f;
  float bmap_scale = 0.25f;
  LiteImage::Image2D<float> heightmap; //normalized (0-1) values
  LiteImage::Image2D<uint4> biome_map;

  std::vector<Tree> trees;
  std::vector<float2> tree_positions;//normalized x,z positions
};

void create_demo_terrain(TerrainSettings settings, Terrain &terrain, uint2 size);
void create_tree(std::vector<uint32_t> &grid, const uint3 &size, uint3 p, const Tree &tree);
std::vector<uint32_t> convert_region_to_voxel_grid(const Terrain &terrian, uint3 p0, uint3 size, uint32_t max_height = 256);

std::vector<uint32_t> create_voxel_grid_simple_terrain(const LiteMath::uint3 &size, float min_h, float max_h, float freq);
std::vector<uint32_t> create_voxel_grid_terrain(const LiteMath::uint3 &size, float min_h, float max_h, float base_freq,
                                                int bands, LiteMath::float2 box_min, LiteMath::float2 box_max);