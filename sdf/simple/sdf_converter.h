#pragma once
#include "utils/mesh/mesh.h"
#include "utils/common/data_channel.h"
#include "sdf/common/global_octree.h"
#include "sdf/simple/sdf_simple.h"

#include <vector>
#include <functional>
#include <LiteMath/Image2d.h>

namespace sdf_converter
{
  SdfGrid create_sdf_grid(GridSettings settings, MultithreadedDistanceFunction sdf, unsigned max_threads);
  SdfGrid create_sdf_grid(GridSettings settings, const cmesh4::SimpleMesh &mesh);

  std::vector<SdfFrameOctreeNode> create_sdf_frame_octree(SparseOctreeSettings settings, MultithreadedDistanceFunction sdf, unsigned max_threads);
  std::vector<SdfFrameOctreeNode> create_sdf_frame_octree(SparseOctreeSettings settings, const cmesh4::SimpleMesh &mesh);

  std::vector<SdfSVSNode> create_sdf_SVS(SparseOctreeSettings settings, MultithreadedDistanceFunction sdf, unsigned max_threads);
  std::vector<SdfSVSNode> create_sdf_SVS(SparseOctreeSettings settings, const cmesh4::SimpleMesh &mesh);  

  SdfSBS create_sdf_SBS(SparseOctreeSettings settings, SdfSBSHeader header, MultithreadedDistanceFunction sdf, unsigned max_threads);
  SdfSBS create_sdf_SBS(SparseOctreeSettings settings, SdfSBSHeader header, const cmesh4::SimpleMesh &mesh);

  std::vector<SdfFrameOctreeTexNode> create_sdf_frame_octree_tex(SparseOctreeSettings settings, const cmesh4::SimpleMesh &mesh);
  SdfSBS create_sdf_SBS_tex(SparseOctreeSettings settings, SdfSBSHeader header, const cmesh4::SimpleMesh &mesh, bool noisy = false);

  //-------------------------------------------------------------------------------------------------
  //builders for specific SDF representations
  SdfSBS create_sdf_SBS_col(SparseOctreeSettings settings, SdfSBSHeader header, const cmesh4::SimpleMesh &mesh, unsigned mat_id,
                            const std::vector<MultiRendererMaterial> &materials_lib, 
                            const std::vector<std::shared_ptr<LiteImage::ICombinedImageSampler>> &textures_lib, bool noisy = false);

  //creates SBS with layout SDF_SBS_NODE_LAYOUT_ID32F_IRGB32F
  SdfSBS create_sdf_SBS_indexed(SparseOctreeSettings settings, SdfSBSHeader header, const cmesh4::SimpleMesh &mesh, unsigned mat_id,
                                const std::vector<MultiRendererMaterial> &materials_lib, 
                                const std::vector<std::shared_ptr<LiteImage::ICombinedImageSampler>> &textures_lib, bool noisy = false);

  //creates SBS with layout SDF_SBS_NODE_LAYOUT_ID32F_IRGB32F_IN
  SdfSBS create_sdf_SBS_indexed_with_neighbors(SparseOctreeSettings settings, SdfSBSHeader header, const cmesh4::SimpleMesh &mesh, unsigned mat_id,
                                               const std::vector<MultiRendererMaterial> &materials_lib, 
                                               const std::vector<std::shared_ptr<LiteImage::ICombinedImageSampler>> &textures_lib);

  
  //-------------------------------------------------------------------------------------------------
  //experimental and weird builders
  SdfSBS SBS_ind_to_SBS_ind_with_neighbors(const SdfSBS &sbs);
  std::vector<SdfFrameOctreeNode> create_sdf_frame_octree(SparseOctreeSettings settings, MultithreadedDistanceFunction sdf, float eps, bool is_smooth, bool fix_artefacts);
  std::vector<SdfFrameOctreeNode> create_psdf_frame_octree(SparseOctreeSettings settings, const cmesh4::SimpleMesh &mesh);
  std::vector<SdfFrameOctreeNode> create_vmpdf_frame_octree(SparseOctreeSettings settings, const cmesh4::SimpleMesh &mesh);

  //-------------------------------------------------------------------------------------------------
  //create desired representation directly from Global Octree. Be aware of possible dangers.
  //some settings of global octree will lead to incorrect transformation or even crush.
  //e.g. trying to create framed octree from global octree with brick size > 1
  void global_octree_to_frame_octree(const GlobalOctree &octree, std::vector<SdfFrameOctreeNode> &out_frame); 
  void global_octree_to_frame_octree_tex(const GlobalOctree &octree, std::vector<SdfFrameOctreeTexNode> &out_frame);
  void global_octree_to_SVS(const GlobalOctree &octree, std::vector<SdfSVSNode> &svs);
  void global_octree_to_SBS(const GlobalOctree &octree, SdfSBS &sbs);
}