#pragma once
#include <vector>
#include <cstdint>
#include "LiteMath/LiteMath.h"

using LiteMath::uint3;

struct BinaryVoxelGrid
{
  inline bool get(uint64_t pos) const 
  { 
    return (data_bricks[pos / 64] >> (pos % 64llu)) & 1llu; 
  }
  inline void set(uint64_t pos, bool value)
  {
    if (value)
      data_bricks[pos / 64] |= 1llu << (pos % 64llu);
    else
      data_bricks[pos / 64] &= ~(1llu << (pos % 64llu));
  }
  inline bool get(int x, int y, int z) const
  {
    uint64_t pos = z + y * size.z + x * uint64_t(size.z * size.y);
    return (data_bricks[pos / 64] >> (pos % 64llu)) & 1llu;
  }
  inline void set(int x, int y, int z, bool value)
  {
    uint64_t pos = z + y * size.z + x * uint64_t(size.z * size.y);
    if (value)
      data_bricks[pos / 64] |= 1llu << (pos % 64llu);
    else
      data_bricks[pos / 64] &= ~(1llu << (pos % 64llu));
  }

  inline bool get(uint3 pos) const { return get(pos.x, pos.y, pos.z); }
  inline void set(uint3 pos, bool value) { set(pos.x, pos.y, pos.z, value); }

  void resize(uint3 new_size)
  {
    data_bricks.resize((new_size.x * new_size.y * new_size.z + 63) / 64, 0u);
    size = new_size;
  }

  std::vector<uint64_t> data_bricks; // each brick is 64 bits, 4x4x4 voxels
  uint3 size = uint3(0,0,0);
};

enum class BinaryMipType
{
  ALL,     // mip is 1 if all voxels are 1
  ANY,     // mip is 1 if any voxel is 1
  MAJORITY // mip is 1 if >= than half of voxels are 1
};

/** creates binary voxel grid from float grid */
void  bvg_create_from_grid(const float *grid, BinaryVoxelGrid &bvg, float thr, uint3 grid_size, uint3 bvg_start = uint3(0,0,0));

/** creates mip level of binary voxel grid with half size 
 *    mip_type: ALL, ANY, MAJORITY, defines how aggregation is done
*/
void  bvg_create_mip(const BinaryVoxelGrid &original, BinaryVoxelGrid &mip, BinaryMipType mip_type);

/** recursive fill of binary voxel grid with 1s from seed position 
 *  filling stops on border voxels, border is defined by border_grid (must be same size)
 *  if border grid defines a closed shell, either inside or outside fill be filled (depends on seed_pos)
 *  any of the border voxels can be either 1 or 0, no guarantees here
*/
void  bvg_recursive_fill(const BinaryVoxelGrid &border_grid, BinaryVoxelGrid &grid, uint3 seed_pos);

/** returns fraction of 1s in binary voxel grid */
float bvg_calculate_occupancy(const BinaryVoxelGrid &bvg);