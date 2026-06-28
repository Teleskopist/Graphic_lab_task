#pragma once
#include "../utils/mesh/mesh.h"
#include "voxel_octree.h"
#include "LiteMath.h"

namespace Voxelizer
{
  enum class SparseVoxelizationMode
  {
    SURFACE_ONLY,
    SOLID
  };

  struct SparseVoxelOctreeBuildSettings
  {
    uint32_t resolution = 256; // rounded up to a power of two
    SparseVoxelizationMode mode = SparseVoxelizationMode::SURFACE_ONLY;
    uint32_t voxel_value = 1;
    // Each top-tree leaf is rasterized into a leaf_grid_size^3 grid.
    // Rounded up to a power of two; 2 means one extra octree level per leaf.
    uint32_t leaf_grid_size = 2;
    // UINT32_MAX selects 1 voxel for surface mode and 0 for solid mode.
    uint32_t padding = 0xFFFFFFFFu;
    bool verbose = false;
  };

  bool mesh_supports_solid_voxelization(const cmesh4::SimpleMesh &mesh,
                                        bool verbose = false);

  SparseVoxelOctree build_sparse_voxel_octree_from_mesh(
      const cmesh4::SimpleMesh &mesh,
      const SparseVoxelOctreeBuildSettings &settings = SparseVoxelOctreeBuildSettings());
}
