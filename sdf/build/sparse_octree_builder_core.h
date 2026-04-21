#pragma once
#include "sparse_octree_builder.h"
#include "utils/common/position_hash.h"
#include "utils/common/stat_box.h"  
#include "sparse_octree_augmentation.h"
#include "sparse_octree_unstructured_grid.h"
#include "sparse_octree_flood.h"
#include "sparse_octree_flood_codes.h"
#include "sparse_octree_common.h"
#include "sdf_lod_maker.h"
#include "point_cloud_process.h"
#include "omp.h"

#include <set>
#include <chrono>
#include <unordered_map>
#include <map>
#include <atomic>
#include <functional>


namespace sdf_converter
{
  using cmesh4::SimpleMesh;
  struct SubpatchBoxSet;

  void surface_by_N_points(const std::vector<float3> &points, float &A, float &B, float &C, float &D, 
                           int iter = 5, bool verbose = false);
  unsigned create_missing_data_voxel_list(float vox_data[8], 
    const std::pair<float, float> edges_data[64],//for voxel edge contains min/max intersection offset
    std::vector<float> &values_f, bool verbose = false);//add at the end of vector n voxels and returns n
  void refill_node(GlobalOctree &out_octree, std::vector<std::vector<NodeBuildData>> &checks, 
                   int layer, int i, unsigned int v_size, const PrimitiveListOctree &pl_octree, SparseOctreeSettings &settings, 
                   std::pair<float, float> edges_data[64], const cmesh4::SimpleMesh &mesh);
  unsigned tri_connection_code(const cmesh4::SimpleMesh &model, unsigned tri_id_A, unsigned tri_id_B,
                               float3 bbox_min_pos, float3 bbox_max_pos);
  uint32_t count_connected_components(const PrimitiveListOctree &pl_octree, 
                                      const cmesh4::SimpleMesh &model,
                                      std::vector<std::vector<NodeBuildData>> &checks, 
                                      int layer, int idx, float thr);
  void fill_node_multi(GlobalOctree &out_octree, std::vector<std::vector<NodeBuildData>> &checks, 
                       int layer, int idx, uint32_t values_layer_offset, unsigned int v_size, const std::vector<uint32_t> &codes,
                       const PrimitiveListOctree &pl_octree, SparseOctreeSettings &settings, int pad, int size, 
                       std::vector<std::vector<PointAttributes>> &attributes, 
                       MeshDistFunc &dist_func, const cmesh4::SimpleMesh &model);
  float parent_child_distance(SparseOctreeSettings::VoxelMetric metric,
                              const GlobalOctree &octree, const GlobalOctreeNode &parent,
                              const GlobalOctreeNode &child, uint32_t child_n /*0 to 7 - number of child*/);
  void init_material_texture_data(GlobalOctree &octree, const SparseOctreeSettings &settings);
  void prim_octree_materials(const SimpleMesh &model, const PrimitiveListOctree &pl_octree, unsigned idx, float &err);
  void prim_octree_node_squares(const SimpleMesh &model, const PrimitiveListOctree &pl_octree, unsigned idx, 
                                float &mx, float &mn, float &average);

  void prune_octree(GlobalOctree &out_octree, SparseOctreeSettings &settings, 
                    std::vector<uint32_t> &node_offsets_by_layer);
  void generate_LODs(SparseOctreeSettings &settings, std::vector<uint32_t> &node_offsets_by_layer, 
                     GlobalOctree &out_octree);
  unsigned global_octree_count_and_mark_active_nodes_rec(GlobalOctree &octree, unsigned nodeId);
  void global_octree_eliminate_invalid_rec(const GlobalOctree &octree_old, unsigned oldNodeId, 
                                                 GlobalOctree &octree_new, unsigned newNodeId,
                                           unsigned *old_to_new_node_idx_remap = nullptr);

  template <typename Model, typename DistFunc>
  void fill_node(GlobalOctree &out_octree, std::vector<std::vector<NodeBuildData>> &checks, 
                 int layer, int i, uint32_t values_layer_offset, unsigned int v_size, const std::vector<uint32_t> &codes,
                 const PrimitiveListOctree &pl_octree, SparseOctreeSettings &settings, int pad, int size, 
                 std::vector<std::vector<PointAttributes>> &attributes, DistFunc &dist_func, const Model &model)
  {
    unsigned thread_id = omp_get_thread_num();
    unsigned node_id = checks[layer][i].global_idx;
    unsigned code = checks[layer][i].is_sign_working ? codes[checks[layer][i].prim_list_idx] : TYPE_UNKNOWN;
    float3 pos = 2.0f * (checks[layer][i].d * checks[layer][i].pos) - 1.0f;
    GlobalOctreeNode &cur_node = out_octree.nodes[node_id];

    // assume the node is empty until we fill it with values
    cur_node.val_off = values_layer_offset + v_size * v_size * v_size * i;
    cur_node.offset = 0;
    cur_node.bricks_count = 0;
    cur_node.type = GlobalOctreeNodeType::EMPTY;
    cur_node.is_surfaced = false;
    cur_node.is_outside = extract_type(code) != TYPE_VOLUME_INSIDE;

    if (out_octree.debug_info)
    {
      #pragma omp critical (debug_write)
      {
        auto &node_info = get_node_info(out_octree.debug_info, node_id);
        node_info.creation_type = GlobalOctreeDebugInfo::CreationType::EMPTY;
        node_info.position_code = uint4(checks[layer][i].pos.x, checks[layer][i].pos.y, checks[layer][i].pos.z, 1<<layer);
      }
    }

    // there is no primitves in this node, so it is empty and we skip it
    if (pl_octree.nodes[checks[layer][i].prim_list_idx].pid_intersect_count == 0)
      return;

    // this node is 1) guaranteed to have children
    //              2) won't be checked for split threshold
    //              3) it's values will not be used (!fill_all_nodes)
    //  thus there is no need to fill its values
    if (!settings.fill_all_nodes && (!settings.allow_early_stop) && layer < settings.depth)
      return;

    float min_val = 1000;
    float max_val = -1000;
    float coeffs[8] = {};
    unsigned prim_start = pl_octree.nodes[checks[layer][i].prim_list_idx].pid_offset;
    unsigned prim_count = pl_octree.nodes[checks[layer][i].prim_list_idx].pid_count;
    if (settings.use_point_cloud && size == 1 && pad == 0)
    {
      if constexpr (std::is_same_v<DistFunc, MeshDistFunc> && std::is_same_v<Model, SimpleMesh>)
      {
        point_cloud::SDF_by_point_cloud(model, settings, pl_octree.primitive_ids.data()+prim_start, prim_count, 
                                        pos, pos + 2 * (checks[layer][i].d / out_octree.header.brick_size),
                                        dist_func, coeffs);
      }
    }
    if constexpr (std::is_same_v<Model, SubpatchBoxSet>) 
    {
      std::array<float, 8> result;
      model.calculateSDFCoefs(pl_octree.primitive_ids.data()+prim_start, prim_count, pos, pos + 2 * (checks[layer][i].d / out_octree.header.brick_size), result);
      std::copy(result.begin(), result.end(), coeffs);
    }
    for (int x = -pad; x <= size + pad; ++x)
    {
      for (int y = -pad; y <= size + pad; ++y)
      {
        for (int z = -pad; z <= size + pad; ++z)
        {
          float3 ch_pos = pos + 2 * (checks[layer][i].d / out_octree.header.brick_size) * float3(x, y, z);
          unsigned idx = v_size * v_size * (x + pad) + v_size * (y + pad) + z + pad;
          if constexpr (std::is_same_v<Model, SubpatchBoxSet>) 
          {
            attributes[thread_id][idx] = PointAttributes{coeffs[x * 4 + y * 2 + z]};
          }
          else 
          {
            attributes[thread_id][idx] = dist_func.calculate(model, pl_octree.primitive_ids.data()+prim_start, prim_count, ch_pos);
          }
          float dist = attributes[thread_id][idx].distance;
          if (settings.use_point_cloud && size == 1 && pad == 0)
          {
            if constexpr (std::is_same_v<DistFunc, MeshDistFunc> && std::is_same_v<Model, SimpleMesh>)
            {
              dist = coeffs[x * 4 + y * 2 + z];
            }
          }
          out_octree.values_f[cur_node.val_off + idx] = dist;
          min_val = std::min(min_val, dist);
          max_val = std::max(max_val, dist);
        }
      }
    }
    // save texture coordinates from 8 corners
    if (out_octree.header.tc_channel_id >= 0)
    {
      auto &tex = out_octree.point_channels[out_octree.header.tc_channel_id];
      for (int j = 0; j < 8; ++j)
      {
        unsigned idx = out_octree.header.brick_size * (v_size * v_size * (j >> 2) + v_size * ((j >> 1) & 1) + (j & 1));
        //cur_node.tex_coords[j] = attributes[thread_id][idx].tex_coord;
        tex.data_f[tex.num_components * node_id * 8 + j * 2 + 0] = attributes[thread_id][idx].tex_coord.x;
        tex.data_f[tex.num_components * node_id * 8 + j * 2 + 1] = attributes[thread_id][idx].tex_coord.y;
      }
    }
    // save material id from center-ish point
    // TODO: better way to choose material
    if (out_octree.header.mat_channel_id >= 0)
    {
      auto &mat = out_octree.voxel_channels[out_octree.header.mat_channel_id];
      unsigned idx = v_size * v_size * (size / 2) + v_size * (size / 2) + size / 2;
      //cur_node.material_id = attributes[thread_id][idx].mat;
      mat.data_i[mat.num_components * node_id] = attributes[thread_id][idx].mat;
    }

    //save data channels if we have augmented mesh
    if constexpr (std::is_same_v<Model, cmesh4::AugmentedMesh>)
    {
      fill_data_channels_node(model, out_octree, v_size, size, attributes[thread_id], node_id);
    }
    else if constexpr (std::is_same_v<Model, vtk::UnstructuredGrid>)
    {
      fill_data_channels_node(model, out_octree, v_size, size, attributes[thread_id], node_id);
    }

    bool active_as_leaf = min_val <= 0 && (max_val >= 0 || settings.fill_internal_volume);
    cur_node.type = active_as_leaf ? GlobalOctreeNodeType::LEAF : GlobalOctreeNodeType::EMPTY;
    cur_node.bricks_count = active_as_leaf ? 1 : 0;
    cur_node.is_internal  = active_as_leaf && (max_val < 0);

    if (out_octree.debug_info)
    {
      #pragma omp critical (debug_write)
      {
        auto &node_info = get_node_info(out_octree.debug_info, node_id);
        node_info.creation_type = GlobalOctreeDebugInfo::CreationType::FILL_NODE;
        node_info.connected_components_count = 1;
        node_info.primitives_count = pl_octree.nodes[checks[layer][i].prim_list_idx].pid_count;
        node_info.surfaces_count = cur_node.bricks_count;
        node_info.is_surface_node = false;
      }
    }
  }

  template <typename Model, typename DistFunc>
  float abs_interp_metric(GlobalOctree &out_octree, std::vector<std::vector<NodeBuildData>> &checks, 
                          int layer, int i, unsigned int v_size,
                          const PrimitiveListOctree &pl_octree, SparseOctreeSettings &settings, int pad, int size, 
                          DistFunc &dist_func, const Model &model)
  {
  {
    const uint32_t *prim_list = pl_octree.primitive_ids.data() + pl_octree.nodes[checks[layer][i].prim_list_idx].pid_offset;
    const uint32_t prim_count = pl_octree.nodes[checks[layer][i].prim_list_idx].pid_count;
    const uint32_t vox_off = out_octree.nodes[checks[layer][i].global_idx].val_off;
    const uint32_t bricks_count = out_octree.nodes[checks[layer][i].global_idx].bricks_count;
    const float3 pos = 2.0f * (checks[layer][i].d * checks[layer][i].pos) - 1.0f;
    const float3 sz = 2.0f*float3(checks[layer][i].d);

    if (bricks_count == 0)
      return 0.0f;

    float error = 0.0f;
    for (int x=0;x<=2;x++)
    {
      for (int y=0;y<=2;y++)
      {
        for (int z=0;z<=2;z++)
        {
          float3 q = 0.5f*float3(x, y, z);
          float3 p = pos + sz*q;
          
          float real_dist = dist_func.calculate(model, prim_list, prim_count, p).distance;

          float best_error = 1e6f;
          for (int j = 0; j < bricks_count; j++)
          {
            float dist = trilinear_interp(out_octree.values_f.data() + vox_off + j*(v_size*v_size*v_size), q);
            best_error = std::min(best_error, std::abs(std::abs(dist) - std::abs(real_dist)));
          }
          error += best_error;
        }
      }
    }

    return error / 27.0f;
  }

    if (size != 1 || pad != 0)
    {
      printf("WARNING: abs_interp_metric doesn't work\n");
      return 0.f;
    }
    float3 pos = 2.0f * (checks[layer][i].d * checks[layer][i].pos) - 1.0f;
    unsigned node_id = checks[layer][i].global_idx;
    std::vector<float> data;
    data.resize(v_size * 2 * v_size * 2 * v_size * 2);
    int vox_off = out_octree.nodes[node_id].val_off;
    for (int x = -2 * pad; x <= 2 * size + 2 * pad; ++x)
    {
      for (int y = -2 * pad; y <= 2 * size + 2 * pad; ++y)
      {
        for (int z = -2 * pad; z <= 2 * size + 2 * pad; ++z)
        {
          float3 ch_pos = pos + 2 * (checks[layer][i].d / (out_octree.header.brick_size * 2)) * float3(x, y, z);
          data[x * v_size * 2 * v_size * 2 + 
                y * v_size * 2 + z] = dist_func.calculate(model, pl_octree.primitive_ids.data() + pl_octree.nodes[checks[layer][i].prim_list_idx].pid_offset, 
                                                          pl_octree.nodes[checks[layer][i].prim_list_idx].pid_count, ch_pos).distance;
        }
      }
    }
    float local_error = 0;
    for (int x = -2 * pad; x <= 2 * size + 2 * pad; ++x)
    {
      for (int y = -2 * pad; y <= 2 * size + 2 * pad; ++y)
      {
        for (int z = -2 * pad; z <= 2 * size + 2 * pad; ++z)
        {
          float tmp_error = 1000000;
          int x_old = x / 2, y_old = y / 2, z_old = z / 2;
          float x_new = x / 2.0, y_new = y / 2.0, z_new = z / 2.0;
          if (x_old == size + pad) --x_old;
          if (y_old == size + pad) --y_old;
          if (z_old == size + pad) --z_old;
          float vox[8];
          for (int cnt_2 = 0; cnt_2 < out_octree.nodes[node_id].bricks_count; cnt_2++)
          {
            for (int v_idx = 0; v_idx < 8; ++v_idx)
            {
              int x_real = x_old + (v_idx >> 2), y_real = y_old + ((v_idx >> 2) & 1), z_real = z_old + (v_idx & 1);
              vox[v_idx] = out_octree.values_f[v_size * v_size * v_size * cnt_2 + x_real * v_size * v_size + y_real * v_size + z];
            }
            float dist = trilinear_interp(vox, float3(x_new - x_old, y_new - x_old, z_new - z_old));
            if (tmp_error > std::abs(std::abs(dist) - std::abs(data[x * v_size * 2 * v_size * 2 + y * v_size * 2 + z])))
            {
              tmp_error = std::abs(std::abs(dist) - std::abs(data[x * v_size * 2 * v_size * 2 + y * v_size * 2 + z]));
            }
          }
          local_error += tmp_error;
        }
      }
    }
    return local_error;
  }
  
  template <typename Model, typename DistFunc>
  float normal_metric(GlobalOctree &out_octree, const PrimitiveListOctree &pl_octree,
                      NodeBuildData &nbd, uint32_t v_size, int pad, int size, 
                      DistFunc &dist_func, const Model &model)
  {
    const uint32_t *prim_list = pl_octree.primitive_ids.data() + pl_octree.nodes[nbd.prim_list_idx].pid_offset;
    const uint32_t prim_count = pl_octree.nodes[nbd.prim_list_idx].pid_count;
    const uint32_t vox_off = out_octree.nodes[nbd.global_idx].val_off;
    const uint32_t bricks_count = out_octree.nodes[nbd.global_idx].bricks_count;
    const float3 pos = 2.0f * (nbd.d * nbd.pos) - 1.0f;
    const float3 sz = 2.0f*float3(nbd.d);

    if (bricks_count == 0)
      return 0.0f;

    float error = 0.0f;
    for (int x=0;x<=2;x++)
    {
      for (int y=0;y<=2;y++)
      {
        for (int z=0;z<=2;z++)
        {
          float3 q = 0.5f*float3(x, y, z);
          float3 p = pos + sz*q;
          
          float3 original_norm_estimate = normalize(p-dist_func.calculate(model, prim_list, prim_count, p).closest_point);

          float closest_dist = 1e6f;
          float best_error = 1e6f;
          for (int j = 0; j < bricks_count; j++)
          {
            const float *brick = out_octree.values_f.data() + vox_off + j*(v_size*v_size*v_size);
            float dist = std::abs(trilinear_interp(brick, q));
            if (dist < closest_dist)
            {
              float3 norm_estimate = normalize(eval_dist_trilinear_diff(brick, q));
              // printf("norm %f %f %f, estimate %f %f %f -- %f\n", 
              //        original_norm_estimate.x, original_norm_estimate.y, original_norm_estimate.z,
              //        norm_estimate.x, norm_estimate.y, norm_estimate.z, dot(norm_estimate, original_norm_estimate));
              best_error = 1.0f - std::abs(dot(norm_estimate, original_norm_estimate));
              closest_dist = dist;
            }
          }
          error += best_error;
        }
      }
    }

    return error / 27.0f;
  }

  static inline bool is_complex_node(unsigned type)
  {
    /*return type == TYPE_UNKNOWN ||
          (type >= TYPE_UNDECIDED_N5 && type <= TYPE_UNDECIDED_N64) ||
           type == TYPE_PLANE_CORNER ||
           type == TYPE_PLANE_EDGE ||
           type == TYPE_PLANE_EDGE_BEND;*/
    
    return type == TYPE_UNKNOWN || 
    type == TYPE_UNDECIDED_N5  ||
    type == TYPE_UNDECIDED_N61 ||
    type == TYPE_UNDECIDED_N62 ||
    type == TYPE_UNDECIDED_N63 ||
    type == TYPE_UNDECIDED_N64 ||
    type == TYPE_ISOLATED           ||
    type == TYPE_WIRE_EDGE          ||
    type == TYPE_PLANE_CORNER       ||
    type == TYPE_PLANE_EDGE         ||
    type == TYPE_PLANE_EDGE_BEND    ||
    type == TYPE_VOLUME_CORNER      ||
    type == TYPE_VOLUME_EDGE        ||
    type == TYPE_VOLUME_EDGE_INV_5  ||
    type == TYPE_VOLUME_EDGE_INV_6  ||
    type == TYPE_VOLUME_CORNER_INV;
  }

  //take (N+1)^3 samples from both distance values
  //distance in each sample is a trilinear interpolation of 8 initial distance values
  //no bricks with larger sizes or padding allowed
  template <int N> 
  float surface_distance_MSE(const float *dist_1, const float *dist_2)
  {
    static_assert(N > 1);

    float res = 0.0f;
    for (int i = 0; i <= N; i++)
    {
      for (int j = 0; j <= N; j++)
      {
        for (int k = 0; k <= N; k++)
        {
          float3 q =  float3(i,j,k)/float(N);
          float vp = trilinear_interp(dist_1, q);
          float vc = trilinear_interp(dist_2, q);
          res += (vp - vc) * (vp - vc);
        }
      }
    }
    return res / ((N+1) * (N+1) * (N+1));
  }

  template <int N> 
  float surface_distance_RMSE(const float *dist_1, const float *dist_2)
  {
    static_assert(N > 1);

    float res = 0.0f;
    for (int i = 0; i <= N; i++)
    {
      for (int j = 0; j <= N; j++)
      {
        for (int k = 0; k <= N; k++)
        {
          float3 q =  float3(i,j,k)/float(N);
          float vp = trilinear_interp(dist_1, q);
          float vc = trilinear_interp(dist_2, q);
          res += (vp - vc) * (vp - vc);
        }
      }
    }
    return sqrtf(res) / ((N+1) * (N+1) * (N+1));
  }

  template <int N> 
  float voxel_distance_IoU(const float *dist_1, const float *dist_2, unsigned bricks_count_1 = 1, unsigned bricks_count_2 = 1)
  {
    int countUnion = 0;
    int countIntersection = 0;
    float step = 1.0 / N;
    
    for (int i = 0; i < N; ++i) {
      for (int j = 0; j < N; ++j) {
        for (int k = 0; k < N; ++k) {
          float3 point = float3((i + 0.5) * step, (j + 0.5) * step, (k + 0.5) * step) / float(N);
          bool inside1 = false;
          bool inside2 = false;
          float val = 1;

          for (int idx = 0; !inside1 && idx < bricks_count_1; ++idx)
          {
            val = trilinear_interp(dist_1 + 8 * idx, point);
            inside1 = (val < 0);
          }
          for (int idx = 0; !inside2 && idx < bricks_count_2; ++idx)
          {
            val = trilinear_interp(dist_2 + 8 * idx, point);
            inside2 = (val < 0);
          }
          
          // Updating counters
          if (inside1 && inside2) {
            countIntersection++;
            countUnion++;
          } else if (inside1 || inside2) {
            countUnion++;
          }
        }
      }
    }
    
    // Count IoU
    return (countUnion > 0) ? static_cast<double>(countIntersection) / countUnion : 0.0;
  }

  template <typename Model, typename DistFunc, bool Multithreaded>
  void prim_octree_to_global_octree_linear(SparseOctreeSettings settings,
                                           const Model &model,
                                           const PrimitiveListOctree &pl_octree,
                                           const std::vector<uint32_t> &sign_analysis_codes,
                                           GlobalOctree &out_octree,
                                           std::vector<unsigned> &node_offsets_by_layer,
                                           GlobalOctreeBuildStat *stat,
                                           unsigned max_threads)
  {
    assert(settings.min_depth <= settings.depth);
    
    //create temporary data structures and initialize octree
    int pad = out_octree.header.brick_pad;
    int size = out_octree.header.brick_size;
    unsigned v_size = size + pad * 2 + 1;
    std::vector<std::vector<PointAttributes>> attributes(max_threads, std::vector<PointAttributes>(v_size*v_size*v_size + 1));
    std::vector<std::vector<NodeBuildData>> checks(settings.depth + settings.local_inc_depth + 1);
    DistFunc dist_func;
    
    omp_set_num_threads(max_threads);
    out_octree.nodes.resize(1);
    out_octree.values_f.resize(0);
    checks[0].push_back(NodeBuildData());
    node_offsets_by_layer[0] = 0;

    if constexpr (std::is_same_v<Model, cmesh4::AugmentedMesh>)
    {
      init_data_channels(model.vertex_channels, model.face_channels, out_octree);
    }
    else if constexpr (std::is_same_v<Model, vtk::UnstructuredGrid>)
    {
      init_data_channels(model, out_octree);
    }
    if constexpr (std::is_same_v<Model, SimpleMesh>)
    {
      init_material_texture_data(out_octree, settings);
    }

    //main (top-down) pass: create nodes on current layer then determine if we need to split them
    //and create next layer until we reach desired depth or next layer is empty
    for (int layer = 0; checks[layer].size() > 0; ++layer)
    {
      std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

      uint32_t values_layer_offset = out_octree.values_f.size();
      out_octree.values_f.resize(out_octree.values_f.size() + v_size * v_size * v_size * checks[layer].size());

      //if constexpr (std::is_same_v<Model, cmesh4::AugmentedMesh> || std::is_same_v<Model, vtk::UnstructuredGrid>)
      {
        resize_data_channels(out_octree, checks[layer].size());
      }

      bool support_multi_nodes = std::is_same_v<DistFunc, MeshDistFunc> && std::is_same_v<Model, SimpleMesh> &&
                                 !settings.fill_internal_volume;// && size == 1 && pad == 0;

      if (support_multi_nodes && settings.allow_multi_nodes)
      {
        if constexpr (std::is_same_v<DistFunc, MeshDistFunc> && std::is_same_v<Model, SimpleMesh>)
        {
          if constexpr (Multithreaded)
          {
            #pragma omp parallel for schedule(static)
            for (int i = 0; i < checks[layer].size(); ++i)
            {
              fill_node_multi(out_octree, checks, layer, i, values_layer_offset, v_size, sign_analysis_codes, pl_octree, 
                              settings, pad, size, attributes, dist_func, model);
            }
          }
          else
          {
            for (int i = 0; i < checks[layer].size(); ++i)
            {
              fill_node_multi(out_octree, checks, layer, i, values_layer_offset, v_size, sign_analysis_codes, pl_octree, 
                              settings, pad, size, attributes, dist_func, model);
            }
          }
        }
      }
      else
      {
        if constexpr(Multithreaded)
        {
          #pragma omp parallel for schedule(static)
          for (int i = 0; i < checks[layer].size(); ++i)
          {
            fill_node<Model, DistFunc>(out_octree, checks, layer, i, values_layer_offset, v_size, sign_analysis_codes, pl_octree,
                                       settings, pad, size, attributes, dist_func, model);
          }
        }
        else
        {
          for (int i = 0; i < checks[layer].size(); ++i)
          {
            fill_node<Model, DistFunc>(out_octree, checks, layer, i, values_layer_offset, v_size, sign_analysis_codes, pl_octree,
                                       settings, pad, size, attributes, dist_func, model);
          }       
        }
      }

      if constexpr (std::is_same_v<Model, SimpleMesh>)
      {
        if (settings.allow_nodes_refill && pad == 0 && size == 1)
        {
          std::pair<float, float> edges_data[64];
          for (int i = 0; i < checks[layer].size(); ++i)
          {
            auto &cur_node = out_octree.nodes[checks[layer][i].global_idx];
            if (cur_node.bricks_count > 1)
              continue;
            for (int j = 0; j < 64; ++j)
            {
              edges_data[j].first = -1;
              edges_data[j].second = -1;
            }
            refill_node(out_octree, checks, layer, i, v_size, 
                        pl_octree, settings, edges_data, model);
          }
        }
      }

      std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
      if (stat)
        stat->time_calculate_distances += std::chrono::duration<float, std::milli>(t2 - t1).count();

      //no more subdividing
      if (layer == settings.depth + settings.local_inc_depth)
        break;

      checks[layer + 1].reserve(checks[layer].size() * 8);
      std::atomic<uint32_t> next_node_offset = out_octree.nodes.size();

      static stat_utils::RangeBins<float> error_bins({1e-6f, 1e-4f, 1e-3f, 0.01f, 0.05f, 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f});

      uint32_t max_internal_depth = settings.internal_depth == 0 ? settings.depth : settings.internal_depth;

      #pragma omp parallel for schedule(static)
      for (int i = 0; i < checks[layer].size(); ++i)
      {
        auto &cur_node = out_octree.nodes[checks[layer][i].global_idx];
        //should we divide this node further
        bool divide;

        if (pl_octree.nodes[checks[layer][i].prim_list_idx].pid_intersect_count == 0) //this node is empty already, stop here
          divide = false;
        else if (layer < settings.min_depth) //we must subdivide this node as required in settings
          divide = true;
        else if (cur_node.is_internal && layer >= max_internal_depth) //we reached the limit of depth for internal nodes, stop here
          divide = false;
        else 
        {
          divide = true;
          if (settings.allow_early_stop)
          {
            // calculate distences between node on this level and its parent
            // if it is very similar to parent, then we don't need to subdivide this node
            GlobalOctreeNode &parent_node = out_octree.nodes[checks[layer][i].parent_global_idx];
            GlobalOctreeNode &child_node = out_octree.nodes[checks[layer][i].global_idx];
            float dist = parent_child_distance(settings.voxel_metric, out_octree, parent_node, child_node, checks[layer][i].child_n);
            divide = dist >= settings.split_thr;
          }
          
          if (divide && settings.local_inc_depth != 0 && settings.depth <= layer)
          {
            float tmp = 0.0f;
            float err = 0.0f;
            switch (settings.local_depth_method)
            {
              case SparseOctreeSettings::LocalDepthMethod::PRIMITIVES:
                err = pl_octree.nodes[checks[layer][i].prim_list_idx].pid_intersect_count;
                break;
              case SparseOctreeSettings::LocalDepthMethod::MATERIALS:
                if constexpr (std::is_same_v<Model, SimpleMesh>)
                {
                  prim_octree_materials(model, pl_octree, checks[layer][i].prim_list_idx, err);
                }
                break;
              case SparseOctreeSettings::LocalDepthMethod::AV_SQUARE:
                if constexpr (std::is_same_v<Model, SimpleMesh>)
                {
                  prim_octree_node_squares(model, pl_octree, checks[layer][i].prim_list_idx, tmp, tmp, err);
                }
                break;
              case SparseOctreeSettings::LocalDepthMethod::MIN_SQUARE:
                if constexpr (std::is_same_v<Model, SimpleMesh>)
                {
                  prim_octree_node_squares(model, pl_octree, checks[layer][i].prim_list_idx, tmp, err, tmp);
                }
                break;
              case SparseOctreeSettings::LocalDepthMethod::MAX_SQUARE:
                if constexpr (std::is_same_v<Model, SimpleMesh>)
                {
                  prim_octree_node_squares(model, pl_octree, checks[layer][i].prim_list_idx, err, tmp, tmp);
                }
                break;
              case SparseOctreeSettings::LocalDepthMethod::COMPONENTS:
                if constexpr (std::is_same_v<Model, SimpleMesh>)
                {
                  err = count_connected_components(pl_octree, model, checks, layer, i, settings.local_depth_thr);
                }
                break;
              case SparseOctreeSettings::LocalDepthMethod::SURFACES:
                err = out_octree.nodes[checks[layer][i].global_idx].is_surfaced ? settings.local_depth_thr : 0.0f;
                break;
              case SparseOctreeSettings::LocalDepthMethod::ABS_METRIC:
                err = abs_interp_metric<Model, DistFunc>(out_octree, checks, layer, i, v_size, pl_octree,
                                                        settings, pad, size, dist_func, model);
                break;
              case SparseOctreeSettings::LocalDepthMethod::SIGN_ANALYSIS:
                err = is_complex_node(sign_analysis_codes[checks[layer][i].prim_list_idx]) ? 1000 : 0;//
                //it will take information about codes from the parent and it is normal if we want to subdivide such node on few levels
                break;
              case SparseOctreeSettings::LocalDepthMethod::NORMAL_ERROR:
                err = normal_metric<Model, DistFunc>(out_octree, pl_octree, checks[layer][i], v_size, pad, size, dist_func, model);
                break;
              default:
                err = 0.0f;
                break;
            }

            divide = err >= settings.local_depth_thr;

            // #pragma omp critical
            // {
            //   error_bins.add(err);
            //   if (error_bins.total_count % 10000 == 0)
            //     error_bins.print_bins();
            // }
            
            if (out_octree.debug_info)
            {
              #pragma omp critical
              {
                auto &ch_node_info = get_node_info(out_octree.debug_info, checks[layer][i].global_idx);
                ch_node_info.add_depth_error = err;
              }
            }
          }
        }

        //prepare task to create children
        if (divide)
        {
          //update type
          if (cur_node.type == GlobalOctreeNodeType::LEAF)
            cur_node.type = GlobalOctreeNodeType::NODE;
          else
            cur_node.type = GlobalOctreeNodeType::EMPTY_NODE;
          
          //set offset to future children (memory will be allocated later)
          cur_node.offset = next_node_offset.fetch_add(8);

          #pragma omp critical
          {
            int prim_ofs = pl_octree.nodes[checks[layer][i].prim_list_idx].offset;
            for (int j = 0; j < 8; ++j)
            {
              NodeBuildData elem;
              elem.d = checks[layer][i].d / 2.0;
              elem.pos = 2 * checks[layer][i].pos + float3(j >> 2, (j >> 1) & 1, j & 1);
              elem.is_sign_working = prim_ofs > 0;
              elem.prim_list_idx = prim_ofs > 0 ? prim_ofs + j : checks[layer][i].prim_list_idx;//HERE
              elem.global_idx = cur_node.offset + j;
              elem.parent_global_idx = checks[layer][i].global_idx;
              elem.child_n = j;
              checks[layer + 1].push_back(elem);
            }
          }
        }
      }

      //printf("level %d: %d nodes\n", layer + 1, (int)checks[layer + 1].size());

      //allocate memory for new nodes
      node_offsets_by_layer[layer + 1] = out_octree.nodes.size();
      out_octree.nodes.resize(next_node_offset.load());

      std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();
      if (stat)
        stat->time_decide_to_divide += std::chrono::duration<float, std::milli>(t3 - t2).count();
    }

    omp_set_num_threads(omp_get_max_threads());
  }

  template <typename Model, typename DistFunc, bool Multithreaded>
  void prim_octree_to_global_octree(SparseOctreeSettings settings, const Model &model, 
                                    const PrimitiveListOctree &pl_octree, GlobalOctree &out_octree,
                                    GlobalOctreeBuildStat *build_stat)
  {
    out_octree.header.brick_size = settings.brick_size;
    out_octree.header.brick_pad = settings.brick_pad;

    std::vector<unsigned> node_offsets_by_layer(settings.depth + settings.local_inc_depth + 2, 0);
    GlobalOctree tmp_octree;
    tmp_octree.header = out_octree.header;
    tmp_octree.nodes.resize(pl_octree.nodes.size());
    tmp_octree.debug_info = out_octree.debug_info;

    // I. Perform sign analysis (optional)
    // sign_analysis_codes contains a code for each PLO node that indicates should this node
    // has surface, it's type and signs.
    std::vector<uint32_t> sign_analysis_codes;
    if (settings.sign_analysis)
    {
      FloodedOctree fo;
      std::chrono::high_resolution_clock::time_point tm1 = std::chrono::high_resolution_clock::now();
      prim_octree_to_flooded_octree_linear(pl_octree, fo, settings.min_depth);
      std::chrono::high_resolution_clock::time_point tm2 = std::chrono::high_resolution_clock::now();
      sign_analysis_codes = calculate_node_codes(fo);
      std::chrono::high_resolution_clock::time_point tm3 = std::chrono::high_resolution_clock::now();

      if (build_stat)
      {
        build_stat->time_build_flooded_octree += std::chrono::duration_cast<std::chrono::milliseconds>(tm2 - tm1).count();
        build_stat->time_sign_analysis += std::chrono::duration_cast<std::chrono::milliseconds>(tm3 - tm2).count();
      }
    }
    else
    {
      sign_analysis_codes = std::vector<uint32_t>(pl_octree.nodes.size(), CODE_UNDECIDED);
    }

    // II. Create "raw" SDF octree
    // Created octree is not optimal and can contain a lot of empty nodes, branches or even be empty as a whole.
    prim_octree_to_global_octree_linear<Model, DistFunc, Multithreaded>(settings, model, pl_octree, sign_analysis_codes, tmp_octree, 
                                                                        node_offsets_by_layer, build_stat, omp_get_max_threads());
    
    sdf_converter::recursive_voxel_channels_merging(tmp_octree);

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    // III. Bottom-up LOD building (WIP) (optional)
    if (out_octree.header.brick_size == 1 && out_octree.header.brick_pad == 0 && 
        settings.advanced_LOD_generation && settings.fill_all_nodes)
    {
      generate_LODs(settings, node_offsets_by_layer, tmp_octree);
    }

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

    // IV. Node pruning (optional)
    // If parent node is very similar (< split_thr) to it's children, we remove children here
    if (settings.use_pruning && (settings.split_thr > 0 || settings.channel_split_thr > 0))
    {
      prune_octree(tmp_octree, settings, node_offsets_by_layer);
    }

    std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();

    // V. Find active nodes and mark them
    unsigned nn = global_octree_count_and_mark_active_nodes_rec(tmp_octree, 0);

    //sdf_converter::recursive_voxel_channels_merging(tmp_octree);
    
    //There are no active nodes, the octree is empty. It is not an error by itself and should be handled by caller.
    //The main reason of this is when we are trying to build shallow octree from a very small object
    if (nn == 0)
    {
      out_octree.nodes.clear();
      return;
    }

    std::chrono::high_resolution_clock::time_point t4 = std::chrono::high_resolution_clock::now();

    // VI. Eliminate empty nodes and branches
    assert(!is_leaf(tmp_octree.nodes[0].offset));
    out_octree.nodes.clear();
    out_octree.nodes.reserve(tmp_octree.nodes.size());
    out_octree.nodes.push_back(tmp_octree.nodes[0]);

    out_octree.values_f.reserve(tmp_octree.values_f.size());

    // put large positive values, all empty bricks (with val_off = 0) will point to these values
    unsigned size = out_octree.header.brick_pad * 2 + out_octree.header.brick_size + 1;
    size = size * size * size;
    for (int i=0;i<size;i++)
      out_octree.values_f.push_back(1000.0f);

    std::vector<unsigned> old_idx_to_new_idx(tmp_octree.nodes.size(), 0xFFFFFFFFu);
    global_octree_eliminate_invalid_rec(tmp_octree, 0, out_octree, 0, old_idx_to_new_idx.data());
    eliminate_empty_nodes_from_data_channels(tmp_octree, out_octree, old_idx_to_new_idx);
    out_octree.nodes.shrink_to_fit();

    if (out_octree.debug_info && out_octree.debug_info->nodes_info.size() > 0)
    {
      assert(out_octree.debug_info->nodes_info.size() == old_idx_to_new_idx.size());
      std::vector<GlobalOctreeDebugInfo::NodeInfo> new_nodes_info(out_octree.nodes.size());

      for (unsigned i = 0; i < old_idx_to_new_idx.size(); ++i)
      {
        if (old_idx_to_new_idx[i] != 0xFFFFFFFFu)
          new_nodes_info[old_idx_to_new_idx[i]] = out_octree.debug_info->nodes_info[i];
      }
      out_octree.debug_info->nodes_info = std::move(new_nodes_info);
    }

    std::chrono::high_resolution_clock::time_point t5 = std::chrono::high_resolution_clock::now();

    if (build_stat)
    {
      build_stat->active_nodes = nn;
      build_stat->settings = settings;
      build_stat->time_mark_empty_nodes = std::chrono::duration<float, std::milli>(t4 - t3).count();
      build_stat->time_eliminate_empty_nodes = std::chrono::duration<float, std::milli>(t5 - t4).count();
      //build_stat->time_generate_LODs += std::chrono::duration<float, std::milli>(t2 - t1).count();
      build_stat->time_pruning += std::chrono::duration<float, std::milli>(t3 - t2).count();
    }

    //print_octree_build_stat(stat);

    //printf("%u/%u nodes are active\n", nn, (unsigned)tmp_octree.nodes.size());
    //printf("%u/%u nodes are left after elimination\n", (unsigned)out_octree.nodes.size(), (unsigned)tmp_octree.nodes.size());
  }
}
