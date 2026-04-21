#pragma once
#include "sdf/common/sdf_common_shared.h"

// enum SdfMultiOctreeFlags
static constexpr unsigned MULTI_OCTREE_FLAG_SURFACE = 1;

//node for MultiOctree, does not contain values
struct SdfMultiOctreeNode
{
  uint32_t data_offset;    // offset in data vector for all distance values, offset is in uint32_t, not bytes 
  uint32_t children_offset;// offset for children (they are stored together). 0 offset means it's a leaf
  uint32_t voxel_count;    // number of voxels in this node, 0 mean it does not contain surface
  uint32_t flags;          // enum SdfMultiOctreeFlags
};