#pragma once
#include "voxel_octree.h"
#include "sdf/scom2/scom.h"

void sparse_voxel_octree_to_DAG(const SparseVoxelOctree &octree, SdfDAG &dag);