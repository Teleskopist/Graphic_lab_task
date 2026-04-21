#pragma once
#include "sdf/multi/sdf_multi_shared.h"
#include "utils/misc/scene_common.h"
#include "utils/common/data_channel.h"
#include "blk/blk.h"
#include "sdf/common/global_octree.h"
#include "core/scene_extension.h"

struct SdfMultiOctree
{
  std::vector<SdfMultiOctreeNode> nodes;
  std::vector<float> values;
};

struct SdfMultiOctreeView
{
  SdfMultiOctreeView() = default;
  
  SdfMultiOctreeView(const SdfMultiOctree &a_octree)
  {
    nodes = a_octree.nodes.data();
    nodes_size = a_octree.nodes.size();
    values = a_octree.values.data();
    values_size = a_octree.values.size();
  }

  SdfMultiOctreeView(const std::vector<SdfMultiOctreeNode> &a_nodes,
                     const std::vector<float> &a_values)
  {
    nodes = a_nodes.data();
    nodes_size = a_nodes.size();
    values = a_values.data();
    values_size = a_values.size();
  }

  const SdfMultiOctreeNode *nodes = nullptr;
  uint32_t nodes_size = 0;
  const float *values = nullptr;
  uint32_t values_size = 0;
};

struct SdfMultiOctreeAugmented
{
  SdfMultiOctree octree;
  
  std::vector<DataChannel> point_channels;
  std::vector<DataChannel> voxel_channels;
};

void save_sdf_multi_octree(const SdfMultiOctreeView &scene, const std::string &path);
void load_sdf_multi_octree(SdfMultiOctree &scene, const std::string &path);
ModelInfo get_info_sdf_multi_octree(const SdfMultiOctreeView &scene);

void save_sdf_augmented_MO(const SdfMultiOctreeAugmented &scene, const std::string &path);
void load_sdf_augmented_MO(SdfMultiOctreeAugmented &scene, const std::string &path);
ModelInfo get_info_sdf_augmented_MO(const SdfMultiOctreeAugmented &scene);

namespace LiteScene
{
  REGISTER_TYPE(TYPE_SDF_MULTI_OCTREE, SDFMultiOctreeGeometry, SdfMultiOctree,                   "sdf_multi_octree", save_sdf_multi_octree, load_sdf_multi_octree, get_info_sdf_multi_octree)
  REGISTER_TYPE(TYPE_SDF_MULTI_OCTREE, SDFAugmentedMultiOctreeGeometry, SdfMultiOctreeAugmented, "sdf_augmented_multi_octree", save_sdf_augmented_MO, load_sdf_augmented_MO, get_info_sdf_augmented_MO)
}

namespace sdf_converter
{
  SdfMultiOctree create_sdf_multi_octree(SparseOctreeSettings settings, MultithreadedDistanceFunction sdf, unsigned max_threads);
  SdfMultiOctree create_sdf_multi_octree(SparseOctreeSettings settings, const cmesh4::SimpleMesh &mesh);

  SdfMultiOctree create_sdf_multi_octree(SparseOctreeSettings settings, const std::vector<cmesh4::SimpleMesh> &mesh, bool is_vox_merge = true);

  SdfMultiOctreeAugmented create_sdf_augmented_multi_octree(SparseOctreeSettings settings, MultithreadedDistanceFunction sdf, unsigned max_threads);
  SdfMultiOctreeAugmented create_sdf_augmented_multi_octree(SparseOctreeSettings settings, const cmesh4::SimpleMesh &mesh);

  SdfMultiOctreeAugmented create_sdf_augmented_multi_octree(SparseOctreeSettings settings, const std::vector<cmesh4::SimpleMesh> &mesh, bool is_vox_merge = true);

  void global_octree_to_multi_octree(const GlobalOctree &octree, SdfMultiOctree &multi_octree);
  void global_octree_to_augmented_multi_octree(const GlobalOctree &octree, SdfMultiOctreeAugmented &multi_octree);
}