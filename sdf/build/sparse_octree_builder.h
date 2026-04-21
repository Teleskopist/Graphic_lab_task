#pragma once
#include <vector>
#include "utils/mesh/mesh.h"
#include "utils/common/primitive_list_octree.h"
#include "utils/mesh/triangle_list_octree.h"
#include "sdf/common/global_octree.h"
#include "utils/mesh/augmented_mesh.h"
#include "utils/vtk/vtk_structures.h"

namespace sdf_converter
{
  void sdf_to_global_octree(SparseOctreeSettings settings, MultithreadedDistanceFunction sdf, 
                            unsigned max_threads, GlobalOctree &octree);
  void mesh_octree_to_global_octree(SparseOctreeSettings settings, const cmesh4::SimpleMesh &mesh, 
                                    const PrimitiveListOctree &pl_octree, GlobalOctree &out_octree,
                                    GlobalOctreeBuildStat *build_stat = nullptr, bool multithreaded = true);
  
  void augmented_mesh_to_global_octree(SparseOctreeSettings settings, const cmesh4::AugmentedMesh &model, 
                                       const PrimitiveListOctree &pl_octree, GlobalOctree &out_octree,
                                       GlobalOctreeBuildStat *build_stat = nullptr, bool multithreaded = true);

  void unstructured_grid_to_global_octree(SparseOctreeSettings settings, const vtk::UnstructuredGrid &model,
                                          const PrimitiveListOctree &pl_octree, GlobalOctree &out_octree,
                                          GlobalOctreeBuildStat *build_stat = nullptr, bool multithreaded = true);
  //Merging a set of octrees, where each octree represent some subset of the space that will become a branch of
  //merged octree. This subset of space is defined by code (pos.x, pos.y, pos.z, size) (e.g. (0,0,0,1) is the whole space)
  //It is useful when working with a lot of small octrees.
  //Warning: this function will modify input octrees, do not use them afterwards
  void merge_global_octrees(GlobalOctree &out_octree, std::vector<GlobalOctree> &octrees, 
                            const std::vector<uint4> &octree_codes, bool is_vox_merge);
}