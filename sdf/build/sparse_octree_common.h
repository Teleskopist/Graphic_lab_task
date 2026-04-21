#pragma once
#include "utils/mesh/mesh.h"
#include "sdf/common/global_octree.h"
#include <vector>
#include <functional>
#include <fstream>

namespace LiteMath
{
  static inline int2 operator%(const int2 a, const int2 b) { return int2{a.x % b.x, a.y % b.y}; }
  static inline int3 operator%(const int3 a, const int3 b) { return int3{a.x % b.x, a.y % b.y, a.z % b.z}; }
  static inline int4 operator%(const int4 a, const int4 b) { return int4{a.x % b.x, a.y % b.y, a.z % b.z, a.w % b.w}; }

  static inline uint2 operator%(const uint2 a, const uint2 b) { return uint2{a.x % b.x, a.y % b.y}; }
  static inline uint3 operator%(const uint3 a, const uint3 b) { return uint3{a.x % b.x, a.y % b.y, a.z % b.z}; }
  static inline uint4 operator%(const uint4 a, const uint4 b) { return uint4{a.x % b.x, a.y % b.y, a.z % b.z, a.w % b.w}; }
}

namespace sdf_converter
{
  static constexpr unsigned MAX_DISTANCES_PER_NODE = 8*GlobalOctree::MAX_SURFACE_COUNT;

  // distance between nodes of different types, e.g. surface and volume
  static constexpr float INCOMPARABLE_NODES_DISTANCE = 1000.0f;

  struct NodeBuildData
  {
    unsigned prim_list_idx = 0;
    unsigned global_idx = 0;
    float d = 1.0f;
    float3 pos = float3(0, 0, 0);
    unsigned parent_global_idx = 0;
    unsigned child_n = 0; //0 to 7
    bool is_sign_working = true;
  };

  struct MeshDistFunc
  {
    PointAttributes calculate(const cmesh4::SimpleMesh &mesh, const uint32_t *p_ids, uint32_t p_ids_count, float3 pos)
    {
      float min_dist_sq = 1000000;
      float3 min_surface_pos = float3(0, 0, 0);
      int min_ti = -1;

      for (int tri=0; tri<p_ids_count; tri++)
      {
        uint32_t t_i = p_ids[tri];
        float3 a = to_float3(mesh.vPos4f[mesh.indices[3*t_i+0]]);
        float3 b = to_float3(mesh.vPos4f[mesh.indices[3*t_i+1]]);
        float3 c = to_float3(mesh.vPos4f[mesh.indices[3*t_i+2]]);
        float3 surface_pos = cmesh4::closest_point_triangle(pos, a, b, c);
        float3 vt = pos - surface_pos;
        float dst_sq = LiteMath::dot(vt, vt);

        if (min_dist_sq > dst_sq)
        {
          min_dist_sq = dst_sq;
          min_ti = t_i;
          min_surface_pos = surface_pos;
        }
      }

      PointAttributes pa;
      if (min_ti != -1)
      {
        float3 a = to_float3(mesh.vPos4f[mesh.indices[3*min_ti+0]]);
        float3 b = to_float3(mesh.vPos4f[mesh.indices[3*min_ti+1]]);
        float3 c = to_float3(mesh.vPos4f[mesh.indices[3*min_ti+2]]);
        float3 vt = pos - min_surface_pos;
        float sgn = LiteMath::sign(LiteMath::dot(LiteMath::cross(a - b, a - c), vt));
        float3 bc = cmesh4::barycentric(min_surface_pos, a, b, c);
        pa.distance = sqrtf(min_dist_sq) * sgn;
        pa.tex_coord = bc.x*mesh.vTexCoord2f[mesh.indices[3*min_ti+0]] + 
                       bc.y*mesh.vTexCoord2f[mesh.indices[3*min_ti+1]] + 
                       bc.z*mesh.vTexCoord2f[mesh.indices[3*min_ti+2]];
        pa.mat = mesh.matIndices[min_ti];
        pa.closest_point = min_surface_pos;
        pa.closest_primitive_id = min_ti;
      }
      return pa;
    }
  };

  static bool is_border(float distance, int level)
  {
    return level < 2  ? true : std::abs(distance) < sqrt(3)*pow(2, -level);
  }

  static bool is_leaf(unsigned offset)
  {
    return (offset == 0) || (offset & INVALID_IDX);
  }

  static float trilinear_interp(const float values[8], float3 dp)
  {
    return (1-dp.x)*(1-dp.y)*(1-dp.z)*values[0] + 
          (1-dp.x)*(1-dp.y)*(  dp.z)*values[1] + 
          (1-dp.x)*(  dp.y)*(1-dp.z)*values[2] + 
          (1-dp.x)*(  dp.y)*(  dp.z)*values[3] + 
          (  dp.x)*(1-dp.y)*(1-dp.z)*values[4] + 
          (  dp.x)*(1-dp.y)*(  dp.z)*values[5] + 
          (  dp.x)*(  dp.y)*(1-dp.z)*values[6] + 
          (  dp.x)*(  dp.y)*(  dp.z)*values[7];
  }

  static float3 eval_dist_trilinear_diff(const float values[8], float3 dp)
  {
    float ddist_dx = -(1-dp.y)*(1-dp.z)*values[0] + 
                     -(1-dp.y)*(  dp.z)*values[1] + 
                     -(  dp.y)*(1-dp.z)*values[2] + 
                     -(  dp.y)*(  dp.z)*values[3] + 
                      (1-dp.y)*(1-dp.z)*values[4] + 
                      (1-dp.y)*(  dp.z)*values[5] + 
                      (  dp.y)*(1-dp.z)*values[6] + 
                      (  dp.y)*(  dp.z)*values[7];
    
    float ddist_dy = -(1-dp.x)*(1-dp.z)*values[0] + 
                     -(1-dp.x)*(  dp.z)*values[1] + 
                      (1-dp.x)*(1-dp.z)*values[2] + 
                      (1-dp.x)*(  dp.z)*values[3] + 
                     -(  dp.x)*(1-dp.z)*values[4] + 
                     -(  dp.x)*(  dp.z)*values[5] + 
                      (  dp.x)*(1-dp.z)*values[6] + 
                      (  dp.x)*(  dp.z)*values[7];

    float ddist_dz = -(1-dp.x)*(1-dp.y)*values[0] + 
                      (1-dp.x)*(1-dp.y)*values[1] + 
                     -(1-dp.x)*(  dp.y)*values[2] + 
                      (1-dp.x)*(  dp.y)*values[3] + 
                     -(  dp.x)*(1-dp.y)*values[4] + 
                      (  dp.x)*(1-dp.y)*values[5] + 
                     -(  dp.x)*(  dp.y)*values[6] + 
                      (  dp.x)*(  dp.y)*values[7];
  
    return float3(ddist_dx, ddist_dy, ddist_dz);
  }

  static bool contains(const uint4 &a, const uint4 &b) 
  { 
    if (b.w < a.w) return false;
    uint4 a_min = a*(b.w/a.w);
    uint4 a_max = (a+1)*(b.w/a.w);
    return b.x >= a_min.x && b.y >= a_min.y && b.z >= a_min.z && 
           b.x <  a_max.x && b.y <  a_max.y && b.z <  a_max.z;
  }

  static bool equal(const uint4 &a, const uint4 &b)
  {
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
  }

  // only call from critical section named debug_write
  static GlobalOctreeDebugInfo::NodeInfo &get_node_info(GlobalOctreeDebugInfo *debug_info, unsigned node_id)
  {
    if (node_id >= debug_info->nodes_info.size())
      debug_info->nodes_info.resize(node_id + 1);
    debug_info->nodes_info[node_id].original_node_id = node_id;
    return debug_info->nodes_info[node_id];
  }

  unsigned remove_surfaces(float *values_f, unsigned surfaces_count);

  void take_brick_part_by_pos_code(GlobalOctreeHeader header, uint4 region_code, const float *values, 
                                   float *res, float &min_val, float &max_val);
  void print_grid(uint32_t brick_size, uint32_t brick_pad, const float *data);
  void resample_brick(const float* in, float* out, uint32_t v_size_in, uint32_t v_size_out);


  static constexpr int SIDE_X_NEG = 0;
  static constexpr int SIDE_X_POS = 1;
  static constexpr int SIDE_Y_NEG = 2;
  static constexpr int SIDE_Y_POS = 3;
  static constexpr int SIDE_Z_NEG = 4;
  static constexpr int SIDE_Z_POS = 5;
  static constexpr int SIDE_COUNT = 6;

  static constexpr int NONE = -1;
  static constexpr int SURFACE_INVALID = 0;
  static constexpr int SURFACE_UNUSED  = 1;
  static constexpr int SURFACE_USED    = 2;

  struct SdfSurface
  {
    const float *values = nullptr; // distance values for the surface
    bool side_intersect[SIDE_COUNT] = {false};
    uint32_t child_id = 0;
    uint32_t flags = 0; //temporary field, used during processing
  };

  struct MergeInData
  {
    const float **distances;  // distances for all surfaces in all child nodes, surface_counts is used to get surfaces for each node
    uint16_t *surface_counts; // for each child node, number of surfaces in it

    // optional, if child bricks should be rotated
    uint16_t *rotations;
    std::vector<std::vector<uint32_t>> transpositions;
  };
  struct MergeTmpData
  {
    // arrays used for merging single bricks
    // length of all arrays >= number of distance values in result brick
    uint32_t *counts;    
    float *distances;
    bool *frozen;
    bool *updated_values;
    bool *voxel_is_active;
    bool *dist_is_used;

    // arrays used for merging multi-bricks
    uint32_t *surface_offsets;
    SdfSurface *all_surfaces;
    float *all_surface_values;
    int *surface_ids;
    int2 *codes_stack;

    MergeInData single_merge_in_data;
  };

  void merge_child_bricks(uint32_t brick_size, uint32_t brick_pad, uint32_t node_grid_size, 
                          float merged_brick_length, float surface_match_threshold,
                          bool always_merge_single_bricks, MergeInData &in, MergeTmpData &tmp,
                          uint32_t *out_surface_count, std::vector<float> &out_distances);
  void upsample_global_octree(GlobalOctree &octree, GlobalOctree &octree_new);
  void upsample_global_octree_rec(GlobalOctree &octree, GlobalOctree &octree_new, MergeInData &in, MergeTmpData &tmp,
                                  uint32_t level, uint32_t node_index, uint32_t new_parent_node_index);
  void upsample_global_octree_node(GlobalOctree &octree, GlobalOctree &octree_new, MergeInData &in, MergeTmpData &tmp,
                                   uint32_t level, uint32_t node_index);

  // finds active voxels (voxels with both positive and negative distances)
  // and used distances (distances in active voxels)
  // distances and dist_is_used have size (brick_size+1)^3, voxel_is_active has size brick_size^3
  // min_add and max_add are added to min and max distances in voxel respectively, thus making more voxels active
  void find_active_voxels_and_distances(uint32_t brick_size, const float *distances, bool *voxel_is_active, bool *dist_is_used,
                                        float min_add = 0.0f, float max_add = 0.0f);
}