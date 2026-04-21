#include "binary_voxel_grid.h"
#include <cstdio>

void bvg_create_from_grid(const float *grid, BinaryVoxelGrid &bvg, float thr, uint3 grid_size, uint3 bvg_start)
{
  for (int x = 0; x < std::min(grid_size.x, bvg.size.x - bvg_start.x); x++)
  {
    for (int y = 0; y < std::min(grid_size.y, bvg.size.y - bvg_start.y); y++)
    {
      for (int z = 0; z < std::min(grid_size.z, bvg.size.z - bvg_start.z); z++)
      {
        bvg.set(bvg_start + uint3(x, y, z), grid[x * grid_size.y * grid_size.z + y * grid_size.z + z] > thr);
      }
    }
  }
}

void bvg_create_mip(const BinaryVoxelGrid &original, BinaryVoxelGrid &mip, BinaryMipType mip_type)
{
  mip.resize((original.size + 1) / 2);
  for (int x = 0; x < mip.size.x; x++)
  {
    for (int y = 0; y < mip.size.y; y++)
    {
      for (int z = 0; z < mip.size.z; z++)
      {
        int cnt = 0;
        cnt += original.get(2 * x, 2 * y, 2 * z);
        cnt += original.get(2 * x + 1, 2 * y, 2 * z);
        cnt += original.get(2 * x, 2 * y + 1, 2 * z);
        cnt += original.get(2 * x + 1, 2 * y + 1, 2 * z);
        cnt += original.get(2 * x, 2 * y, 2 * z + 1);
        cnt += original.get(2 * x + 1, 2 * y, 2 * z + 1);
        cnt += original.get(2 * x, 2 * y + 1, 2 * z + 1);
        cnt += original.get(2 * x + 1, 2 * y + 1, 2 * z + 1);
        if (mip_type == BinaryMipType::MAJORITY)
          mip.set(x, y, z, cnt >= 4);
        else if (mip_type == BinaryMipType::ALL)
          mip.set(x, y, z, cnt == 8);
        else // if (mip_type == BinaryMipType::ANY)
          mip.set(x, y, z, cnt > 0);
      }
    }
  }
}

float bvg_calculate_occupancy(const BinaryVoxelGrid &bvg)
{
  uint64_t cnt = 0;
  uint64_t total = bvg.size.x * bvg.size.y * bvg.size.z;
  for (auto &b : bvg.data_bricks)
    cnt += LiteMath::bitCount64(b);

  return float(cnt) / float(total);
}

void bvg_recursive_fill(const BinaryVoxelGrid &border_grid, BinaryVoxelGrid &grid, uint3 seed_pos)
{
  std::vector<uint3> stack;
  stack.resize(1024);
  int top = 0;
  stack[top] = seed_pos;
  while (top >= 0)
  {
    if (top + 6 > stack.size())
      stack.resize(stack.size() + 1024);
    uint3 p = stack[top];
    grid.set(p, true);

    if (border_grid.get(p.x, p.y, p.z) == false)
    {
      if (p.x > 0 && grid.get(p.x - 1, p.y, p.z) == false)
        stack[top++] = uint3(p.x - 1, p.y, p.z);
      if (p.x < grid.size.x - 1 && grid.get(p.x + 1, p.y, p.z) == false)
        stack[top++] = uint3(p.x + 1, p.y, p.z);
      if (p.y > 0 && grid.get(p.x, p.y - 1, p.z) == false)
        stack[top++] = uint3(p.x, p.y - 1, p.z);
      if (p.y < grid.size.y - 1 && grid.get(p.x, p.y + 1, p.z) == false)
        stack[top++] = uint3(p.x, p.y + 1, p.z);
      if (p.z > 0 && grid.get(p.x, p.y, p.z - 1) == false)
        stack[top++] = uint3(p.x, p.y, p.z - 1);
      if (p.z < grid.size.z - 1 && grid.get(p.x, p.y, p.z + 1) == false)
        stack[top++] = uint3(p.x, p.y, p.z + 1);
    }
    top--;
  }
}