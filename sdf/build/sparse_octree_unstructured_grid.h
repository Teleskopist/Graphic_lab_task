#pragma once
#include "sdf/common/global_octree.h"
#include "utils/common/primitive_list_octree.h"
#include "utils/vtk/vtk_structures.h"

namespace sdf_converter
{
  struct UGCellInBBoxFunc
  {
    static bool in_bbox(const vtk::UnstructuredGrid &grid, unsigned t_i, const float3 &bbox_min, const float3 &bbox_max);
  };

  struct UGCellDistFunc
  {
    PointAttributes calculate(const vtk::UnstructuredGrid &grid, const uint32_t *p_ids, uint32_t p_ids_count, float3 pos);
  };

  void init_data_channels(const vtk::UnstructuredGrid &grid, GlobalOctree &octree);
  void fill_data_channels_node(const vtk::UnstructuredGrid &grid, GlobalOctree &octree, unsigned v_size, unsigned size,
                               const std::vector<sdf_converter::PointAttributes> &attributes, unsigned node_idx);
}