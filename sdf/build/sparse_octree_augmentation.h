#pragma once
#include "sdf/common/global_octree.h"
#include "utils/mesh/augmented_mesh.h"
#include "utils/common/primitive_list_octree.h"

namespace sdf_converter
{
  struct AugTriangleInBBoxFunc
  {
    static inline bool in_bbox(const cmesh4::AugmentedMesh &mesh, unsigned t_i, const float3 &bbox_min, const float3 &bbox_max)
    {
      float3 a = mesh.vertices[mesh.indices[3*t_i+0]];
      float3 b = mesh.vertices[mesh.indices[3*t_i+1]];
      float3 c = mesh.vertices[mesh.indices[3*t_i+2]];

      return contains(a, bbox_min, bbox_max) || contains(b, bbox_min, bbox_max) || contains(c, bbox_min, bbox_max) ||
            cmesh4::triangle_aabb_intersect(a, b, c, 0.5f*(bbox_min + bbox_max), 0.5f*(bbox_max - bbox_min));
    }
  };

  struct AugmentedMeshDistFunc
  {
    PointAttributes calculate(const cmesh4::AugmentedMesh &mesh, const uint32_t *p_ids, uint32_t p_ids_count, float3 pos)
    {
      float sign = 1, min_dist_sq = 1000000;
      constexpr float sqrt_dist_equal_diff = 1e-8;//I don't know why it's so big

      float3 surface_pos = float3(0, 0, 0), min_a = float3(0, 0, 0), min_b = float3(0, 0, 0), min_c = float3(0, 0, 0);
      int min_ti = -1;
      float min_dot_norm = 0;

      for (int tri=0; tri<p_ids_count; tri++)
      {
        uint32_t t_i = p_ids[tri];
        float3 a = mesh.vertices[mesh.indices[3*t_i+0]];
        float3 b = mesh.vertices[mesh.indices[3*t_i+1]];
        float3 c = mesh.vertices[mesh.indices[3*t_i+2]];
        float3 vt = pos - cmesh4::closest_point_triangle(pos, a, b, c);
        float dst_sq = LiteMath::dot(vt, vt);

        if ((min_dist_sq > dst_sq && min_dist_sq - dst_sq > sqrt_dist_equal_diff) || 
            (std::abs(dst_sq - min_dist_sq) < sqrt_dist_equal_diff && 
             std::abs(min_dot_norm) < std::abs(LiteMath::dot(normalize(LiteMath::cross(a - b, a - c)), normalize(vt)))))
        {
          min_dist_sq = dst_sq;
          min_ti = t_i;
          min_a = a;
          min_b = b;
          min_c = c;
          surface_pos = cmesh4::closest_point_triangle(pos, a, b, c);
          min_dot_norm = LiteMath::dot(normalize(LiteMath::cross(a - b, a - c)), normalize(vt));

          if (LiteMath::dot(LiteMath::cross(a - b, a - c), vt) < 0)
            sign = -1;
          else
            sign = 1;
        }
      }

      PointAttributes pa;
      if (min_ti != -1)
      {
        pa.distance = sqrtf(min_dist_sq) * sign;
        pa.tex_coord = float2(0,0);
        pa.mat = 0u;
        pa.closest_point = surface_pos;
        pa.closest_primitive_id = min_ti;
      }
      return pa;
    }
  };
  void fill_data_channels_node(const cmesh4::AugmentedMesh &a_mesh, GlobalOctree &octree, unsigned v_size, unsigned size,
                               const std::vector<sdf_converter::PointAttributes> &attributes, unsigned node_idx);

  void init_data_channels(const std::vector<DataChannel> &point_channels, const std::vector<DataChannel> &primitive_channels, 
                          GlobalOctree &octree);
  void copy_data_channels(const GlobalOctree &from, GlobalOctree &to);
  void resize_data_channels(GlobalOctree &octree, int prim_count);
  void reserve_data_channels(GlobalOctree &octree, int prim_count);
  void eliminate_empty_nodes_from_data_channels(const GlobalOctree &in_octree, GlobalOctree &out_octree,
                                                const std::vector<unsigned> &idx_remap);
  void recursive_voxel_channels_merging(GlobalOctree &octree, uint32_t idx = 0);

  float parent_child_distance_channels(const GlobalOctree &octree, uint32_t node_id, 
                                       uint32_t ch_node_id, uint32_t child_n);
}