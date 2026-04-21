#pragma once
#include "voxel_octree.h"
#include "core/scene_extension.h"

namespace LiteScene
{
  REGISTER_TYPE(TYPE_SPARSE_VOXEL_OCTREE, SparseVoxelOctreeGeometry, SparseVoxelOctree, "SVO", save_SVO, load_SVO, get_info_SVO)
}