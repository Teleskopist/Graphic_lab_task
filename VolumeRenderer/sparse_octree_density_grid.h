#pragma once
#include "sdf/common/global_octree.h"
#include "utils/common/primitive_list_octree.h"
#include "utils/misc/density_grid.h"

namespace sdf_converter
{
  //3D grid to octree
  void density_grid_to_global_octree(SparseOctreeSettings settings, const DensityGrid &grid, 
                                     GlobalOctree &out_octree, GlobalOctreeBuildStat *build_stat);

  //4D grid to 2^4-tree
  void density_grid_to_global_octree(SparseOctreeSettings settings, const DensityMultiGrid &grid,
                                     GlobalOctree &out_octree, GlobalOctreeBuildStat *build_stat);

  //slice of 4D grid to octree
  void density_grid_slice_to_global_octree(SparseOctreeSettings settings, const DensityMultiGrid &grid, uint32_t time_step,
                                           GlobalOctree &out_octree, GlobalOctreeBuildStat *build_stat);
}