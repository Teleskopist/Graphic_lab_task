#include "procedural_voxel_grid.h"
#include "minecraft_blocks.h"
#include "SimplexNoise.h"
#include "Image2d.h"
#include "utils/common/rand.h"
#include <random>

using LiteMath::smoothstep;

void create_heightmap_perlin(Terrain &terrain, uint2 size, float min_h, float max_h, float base_freq, int bands)
{
  float h_c = (min_h + max_h) / 2.0f;
  float h_a = (max_h - min_h) / 2.0f;
  SimplexNoise noiseGen = SimplexNoise();
  terrain.water_level = h_c;
  terrain.heightmap.resize(size.x, size.y);
  for (uint32_t j = 0; j < size.y; j++)
  {
    for (uint32_t i = 0; i < size.x; i++)
    {
      LiteMath::float2 p = LiteMath::float2(i, j)/LiteMath::float2(size.x - 1, size.y - 1);
      float h0 = 0;
      float freq = base_freq;
      for (int b = 0; b < bands; b++)
      {
        h0 += noiseGen.noise(freq*p.x, freq*p.y);
        freq *= 2.0f;
      }
      terrain.heightmap.data()[j*size.x+i] = std::clamp(h_c + h_a*h0, 0.0f, 1.0f);
    }
  }
}

void add_mountain(LiteImage::Image2D<float> &heightmap, uint2 pos, int base_radius_pixels, float base_h, float max_h, int deform_vector_count)
{
  std::mt19937 gen(0);
  std::normal_distribution<float> normGen(1.0f, 0.5f);

  float max_q = 0.0f;
  std::vector<float> deform_qs(deform_vector_count, 1.0f);
  for (float &q : deform_qs)
  {
    q = std::max(0.1f, normGen(gen));
    max_q = std::max(max_q, q);
  }
  int2 sz = int2(base_radius_pixels*max_q);
  int2 p0 = clamp(int2(pos)-sz, int2(0,0), int2(heightmap.width()-1, heightmap.height()-1));
  int2 p1 = clamp(int2(pos)+sz, int2(0,0), int2(heightmap.width()-1, heightmap.height()-1));
  for (int y=p0.y; y<=p1.y; y++)
  {
    for (int x=p0.x; x<=p1.x; x++)
    {
      float2 d = float2(x,y)-float2(pos.x, pos.y);
      float dist = length(d);
      float phi = dist > 0.5f ? std::acos(d.x/dist) : 0;
      float qf = deform_vector_count*(phi/(2*LiteMath::M_PI));
      int q0 = qf;
      int q1 = (q0+1)%deform_vector_count;
      float dq = qf-q0;
      float radius = base_radius_pixels*LiteMath::lerp(deform_qs[q0], deform_qs[q1], dq);
      float rel_r = dist/radius;
      if (rel_r < 1)
      {
        //printf("rel_r %f\n", rel_r);
        float h = base_h + (max_h-base_h)*LiteMath::smoothstep(0, 1, 1-rel_r);
        float old_rel_h = heightmap.data()[y*heightmap.width()+x] - base_h;
        heightmap.data()[y*heightmap.width()+x] = std::clamp(h+old_rel_h, 0.0f, 1.0f);
      }
    }
  }
}

void add_mountains(Terrain &terrain, int count, int base_radius, float min_h, float max_h, int deform_vector_count)
{
  int base_radius_pixels = std::max<int>(2, base_radius*terrain.hmap_scale);
  for (int i=0;i<count;i++)
  {
    uint2 pos = uint2(urand(1, terrain.heightmap.width()), urand(1, terrain.heightmap.height()));
    add_mountain(terrain.heightmap, pos, urand(0.5f, 2.0f)*base_radius_pixels, terrain.water_level, urand(min_h, max_h), deform_vector_count);
  }
}

void add_trees(Terrain &terrain, int region_size, float region_pick_chance)
{
  LiteImage::Sampler sampler;
  sampler.filter = LiteImage::Sampler::Filter::LINEAR;

  for (int x=0; x<terrain.heightmap.width(); x+=region_size)
  {
    for (int z=0; z<terrain.heightmap.height(); z+=region_size)
    {
      if (urand(0.0f, 1.0f) > region_pick_chance)
        continue;
      
      float2 p = float2(x + urand(0,region_size), z + urand(0, region_size));
      p /= float2(terrain.heightmap.width(), terrain.heightmap.height());
      float h = terrain.heightmap.sample(sampler, p).x;

      if (h < terrain.water_level)
        continue;

      const float dx = 0.001f;
      float hx = terrain.heightmap.sample(sampler, p+float2(dx,0)).x;
      float hy = terrain.heightmap.sample(sampler, p+float2(0,dx)).x;
      float2 grad = float2(hx-h,hy-h)/dx;
      float slope = length(grad);

      float height_penalty = smoothstep(0.45f, 0.7f, h);
      float slope_penalty = smoothstep(1.5f, 4.5f, slope);
      if (urand() < height_penalty + slope_penalty)
        continue;

      Tree t;
      t.type = urand() > 0.5f ? Tree::Type::Pine : Tree::Type::Spruce;
      t.height = urand(7,15);
      t.size = t.height*urand(0.25f, 0.45f) + 1;
      terrain.trees.push_back(t);
      terrain.tree_positions.push_back(p);
    }
  }
}

void create_demo_terrain(TerrainSettings settings, Terrain &terrain, uint2 size)
{
  create_heightmap_perlin(terrain, size, settings.hmap_min, settings.hmap_max, settings.hmap_base_freq, settings.hmap_bands);
  add_mountains(terrain, settings.mountains_count, settings.mountains_base_radius, settings.mountains_min_h, 
                settings.mountains_max_h, settings.mountains_deform_vector_count);
  add_trees(terrain, settings.tree_region_size, settings.tree_spawn_chance);
  printf("created %d trees\n", (int)terrain.trees.size());

  LiteImage::SaveImage<float>("saves/demo_hmap.png", terrain.heightmap, 1.0f);
}

BlockType depth_to_block_type(int depth)
{
  BlockType block_id = BlockType::Air;
  if (depth < 0)
    block_id = BlockType::Air;
  else if (depth == 0)
    block_id = BlockType::Grass;
  else if (depth < 3)
    block_id = BlockType::Dirt;
  else
    block_id = BlockType::Stone;
  return block_id;
}

BlockType height_based_block_type(int height, int terrain_height, int max_height, int water_level, float slope)
{
  if (height > terrain_height)
    return height <= water_level ? BlockType::Water : BlockType::Air;

  int depth = terrain_height - height;
  float rh = float(height)/max_height;
  if (depth >= 3)
    return BlockType::Stone;

  if (terrain_height < water_level)
  {
    int water_depth = water_level - terrain_height;
    if (depth + water_depth < 3)
      return BlockType::Dirt;
    else 
      return BlockType::Stone;
  }

  float snow_chance = depth == 0 ? smoothstep(0.7, 0.9f, rh) : 0;
  float gravel_chance = rh < 0.7 ? smoothstep(0.5, 0.7f, rh) : 1 - smoothstep(0.7, 0.9f, rh);
  gravel_chance += std::max(0.0f, slope-3.5f)*std::max(0.0f, slope-3.5f);
  float dirt_chance = 0.5 * (rh < 0.6 ? smoothstep(0.6, 0.7f, rh) : 1 - smoothstep(0.7, 0.8f, rh));
  float grass_chance = depth == 0 ? 1 - smoothstep(0.5, 0.7, rh) : 0;
  float b_total = snow_chance + gravel_chance + dirt_chance + grass_chance;
  float rnd = urand(0, b_total);
  if (rnd < snow_chance)
    return BlockType::Snow;
  else if (rnd < snow_chance + gravel_chance)
    return BlockType::Gravel;
  else if (rnd < snow_chance + gravel_chance + dirt_chance)
    return BlockType::Dirt;
  return BlockType::Grass;

  return depth_to_block_type(depth);
}

std::vector<uint32_t> convert_region_to_voxel_grid(const Terrain &terrain, uint3 p0, uint3 size, uint32_t max_height)
{
  LiteImage::Sampler sampler;
  sampler.filter = LiteImage::Sampler::Filter::LINEAR;

  std::vector<uint32_t> grid(size.x * size.y * size.z);

  float2 box_min  = terrain.hmap_scale*float2(p0.x, p0.z) / float2(terrain.heightmap.width(), terrain.heightmap.height());
  float2 box_size = terrain.hmap_scale*float2(size.x, size.z) / float2(terrain.heightmap.width(), terrain.heightmap.height());

  for (uint32_t i = 0; i < size.z; i++)
  {
    for (uint32_t k = 0; k < size.x; k++)
    {
      float2 p = box_min + box_size*float2(k+0.5f, i+0.5f)/float2(size.x, size.z);
      float h = terrain.heightmap.sample(sampler, p).x;
      const float dx = 0.001f;
      float hx = terrain.heightmap.sample(sampler, p+float2(dx,0)).x;
      float hy = terrain.heightmap.sample(sampler, p+float2(0,dx)).x;
      float2 grad = float2(hx-h,hy-h)/dx;
      float slope = length(grad);
      for (uint32_t j = 0; j < size.y; j++)
      {
        BlockType block_id = height_based_block_type(p0.y+j, max_height*h, max_height, max_height*terrain.water_level, slope);
        grid[i*size.y*size.x + j*size.x + k] = (int)block_id;
      }
    }
  }

  for (int i=0;i<terrain.trees.size();i++)
  {
    float2 rel_p = terrain.tree_positions[i];
    float h = terrain.heightmap.sample(sampler, rel_p).x;
    int3 tree_p0 = int3(rel_p.x*terrain.heightmap.width(), 
                        h*max_height,
                        rel_p.y*terrain.heightmap.height());
    int3 tree_box_center = tree_p0 - int3(p0);
    int3 tree_box_max = tree_box_center + int3(terrain.trees[i].size, terrain.trees[i].height, terrain.trees[i].size);
    int3 tree_box_min = tree_box_center - int3(terrain.trees[i].size, 1, terrain.trees[i].size);
    if (tree_box_max.x < 0 || tree_box_min.x >= size.x ||
        tree_box_max.y < 0 || tree_box_min.y >= size.y ||
        tree_box_max.z < 0 || tree_box_min.z >= size.z)
    {
      continue;
    }

    create_tree(grid, size, uint3(tree_box_min), terrain.trees[i]);  
  }

  return grid;
}

void create_tree_pine(std::vector<uint32_t> &grid, const uint3 &size, uint3 p, const Tree &tree, 
                      float leaves_start, float leaves_max_width, float rnd_range)
{
  //trunk
  for (int y=p.y;y<std::min(size.y, p.y+tree.height-1);y++)
    grid[p.z*size.y*size.x + y*size.x + p.x] = uint32_t(BlockType::Wood);

  //crown
  int crown_start = p.y + round(tree.height*leaves_start);
  for (int y=crown_start;y<std::min(size.y, p.y+tree.height);y++)
  {
    float rel_p = float(y-p.y)/tree.height;
    float rel_r = 0;
    if (rel_p < leaves_start)
      rel_r = 0;
    else if (rel_p < leaves_max_width)
      rel_r = sqrtf((rel_p-leaves_start)/(leaves_max_width-leaves_start));
    else 
      rel_r = 1 - sqrtf((rel_p-leaves_max_width)/(1-leaves_max_width));
    rel_r = 0.2f + 0.8f*rel_r;
    int r = round(tree.size * rel_r);
    int max_r = r*(1+rnd_range);
    for (int z=std::max<int>(0,p.z-max_r);z<=std::min(size.z-1,p.z+max_r);z++)
    {
      for (int x=std::max<int>(0,p.x-max_r);x<=std::min(size.x-1,p.x+max_r);x++)
      {
        float dist = length(float2(x-int(p.x),z-int(p.z)));
        if (dist*urand(1-rnd_range,1+rnd_range) > r)
          continue;
        uint32_t idx = z*size.y*size.x + y*size.x + x;
        if (grid[idx] == uint32_t(BlockType::Air))
          grid[idx] = uint32_t(BlockType::Leaves);
      }      
    }
  }
}

void create_tree(std::vector<uint32_t> &grid, const uint3 &size, uint3 p, const Tree &tree)
{
  switch (tree.type)
  {
  case Tree::Type::Pine :
    create_tree_pine(grid, size, p, tree, 0.6, 0.7, 0.1);
    break;
  case Tree::Type::Spruce :
    create_tree_pine(grid, size, p, tree, 0.25, 0.3, 0.15);
    break;  
  default:
    break;
  }
}

std::vector<uint32_t> create_voxel_grid_simple_terrain(const LiteMath::uint3 &size, float min_h, float max_h, float freq)
{
  std::vector<uint32_t> grid(size.x * size.y * size.z);

  float h_c = (min_h + max_h) / 2.0f;
  float h_a = (max_h - min_h) / 2.0f;

  for (uint32_t i = 0; i < size.z; i++)
  {
    for (uint32_t j = 0; j < size.y; j++)
    {
      for (uint32_t k = 0; k < size.x; k++)
      {
        float h0 = std::sin((float(i) / size.x) * LiteMath::M_PI * freq) * 
                   std::sin((float(k) / size.z) * LiteMath::M_PI * freq);
        int h = size.y*(h_c + h_a*h0);
        int depth = h - j;
        BlockType block_id = depth_to_block_type(depth);
        grid[i*size.y*size.x + j*size.x + k] = (int)block_id;
      }
    }
  }

  return grid;
}

std::vector<uint32_t> create_voxel_grid_terrain(const LiteMath::uint3 &size, float min_h, float max_h, float base_freq,
                                                int bands, LiteMath::float2 box_min, LiteMath::float2 box_max)
{
  std::vector<uint32_t> grid(size.x * size.y * size.z);

  float h_c = (min_h + max_h) / 2.0f;
  float h_a = (max_h - min_h) / 2.0f;
  SimplexNoise noiseGen = SimplexNoise();

  for (uint32_t i = 0; i < size.z; i++)
  {
    for (uint32_t k = 0; k < size.x; k++)
    {
      LiteMath::float2 p = box_min + (box_max - box_min)*LiteMath::float2(k, i)/LiteMath::float2(size.x - 1, size.z - 1);
      float h0 = 0;
      float freq = base_freq;
      for (int b = 0; b < bands; b++)
      {
        h0 += noiseGen.noise(freq*p.x, freq*p.y);
        freq *= 2.0f;
      }
      int h = size.y*(h_c + h_a*h0);
      for (uint32_t j = 0; j < size.y; j++)
      {
        BlockType block_id = depth_to_block_type(h - j);
        grid[i*size.y*size.x + j*size.x + k] = (int)block_id;
      }
    }
  }

  return grid;
}