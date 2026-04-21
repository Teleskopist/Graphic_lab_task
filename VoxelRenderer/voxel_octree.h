#pragma once
#include "LiteMath.h"
#include <vector>
#include <string>

static constexpr uint32_t MAX_CHILD_POINTER = 0x7FFF;
static constexpr uint32_t IS_FAR_BIT = 0x8000;

#ifndef KERNEL_SLICER
// simple structure that contains a voxel octree
// based on Efficient Sparse Voxel Octrees – Analysis, Extensions, and Implementation
// https://research.nvidia.com/sites/default/files/pubs/2010-02_Efficient-Sparse-Voxel/laine2010tr1_paper.pdf
// node - 32 bits: 
// | child pointer | is_far | child_is_leaf | child_has_data |
// |       15 bits | 1 bit  | 8 bits        | 8 bits         |
// |0            14|   15   |16           23| 24           31|
// leaf - 32 bits (if required): unique type of leaf (e.g. material id)
// child pointer is relative to the node, points to the list of child nodes 
// (only those where child_has_data bit is set)
// if is_far is set, the child pointer point to far pointer (32-bit absolute index)
struct SparseVoxelOctreeHeader
{
  uint32_t max_level_size = 1; // 2^max_level
};

struct SparseVoxelOctree
{
  SparseVoxelOctreeHeader header;
  std::vector<uint32_t> data;
};

SparseVoxelOctree voxel_grid_to_octree(const std::vector<uint32_t> &voxel_grid, const LiteMath::uint3 &size,
                                       bool merge_same_type = false);

// Merge a set of octrees, each must be placed so that it has octree code and all octrees
// must not overlap. Returns the merged octree, destroying the input
SparseVoxelOctree merge_voxel_octrees(std::vector<SparseVoxelOctree> &octrees, std::vector<LiteMath::uint4> &codes);

struct ModelInfo;
ModelInfo get_info_SVO(const SparseVoxelOctree &scene);
void save_SVO(const SparseVoxelOctree &scene, const std::string &path);
void load_SVO(SparseVoxelOctree &scene, const std::string &path);
#endif