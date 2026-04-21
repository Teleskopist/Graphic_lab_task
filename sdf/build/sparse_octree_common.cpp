#include "sparse_octree_builder.h"
#include "sparse_octree_common.h"
#include "utils/common/redistancing.h"

namespace sdf_converter
{
  bool is_valid_surface(const float *values)
  {
    float mn = 1000, mx = -1000;

    mn = std::min(mn, values[0]);
    mn = std::min(mn, values[1]);
    mn = std::min(mn, values[2]);
    mn = std::min(mn, values[3]);
    mn = std::min(mn, values[4]);
    mn = std::min(mn, values[5]);
    mn = std::min(mn, values[6]);
    mn = std::min(mn, values[7]);

    mx = std::max(mx, values[0]);
    mx = std::max(mx, values[1]);
    mx = std::max(mx, values[2]);
    mx = std::max(mx, values[3]);
    mx = std::max(mx, values[4]);
    mx = std::max(mx, values[5]);
    mx = std::max(mx, values[6]);
    mx = std::max(mx, values[7]);

    return mn < 0 && mx > 0;
  }


  float3 voxel_grad(float v[8], const float3 &p)
  {
    float3 res = float3(0, 0, 0);
    for (unsigned i = 0; i < 8; ++i)
    {
      float3 elem = p;
      float3 sgn = float3(1, 1, 1);
      if (!(i & 4)) { elem.x = 1 - elem.x; sgn.x = -1; }
      if (!(i & 2)) { elem.y = 1 - elem.y; sgn.y = -1; }
      if (!(i & 1)) { elem.z = 1 - elem.z; sgn.z = -1; }
      res.x += sgn.x * v[i] * elem.y * elem.z;
      res.y += sgn.y * v[i] * elem.x * elem.z;
      res.x += sgn.z * v[i] * elem.x * elem.y;
    }
    return res;
  }

  float surface_dir_metric(float v1[8], float v2[8], float step = 0.25f)
  {
    float res = 0.0f;
    float3 grad_1 = float3(0, 0, 0);
    float3 grad_2 = float3(0, 0, 0);
    for (float i = 0.0f; i <= 1.0f; i += step)
    {
      for (float j = 0.0f; j <= 1.0f; j += step)
      {
        for (float k = 0.0f; k <= 1.0f; k += step)
        {
          float3 tmp = voxel_grad(v1, float3(i, j, k));
          if (length(tmp) > 1e-8)
          {
            grad_1 += normalize(tmp);
          }
          tmp = voxel_grad(v2, float3(i, j, k));
          if (length(tmp) > 1e-8)
          {
            grad_2 += normalize(tmp);
          }
        }
      }
    }
    return std::abs(dot(normalize(grad_1), normalize(grad_2)));
  }

  float surface_range_pos_metric(float v1[8], float v2[8], float step = 0.25f)
  {
    float res = 0.0f, count = 0.0f;
    for (float i = 0.0f; i <= 1.0f; i += step)
    {
      for (float j = 0.0f; j <= 1.0f; j += step)
      {
        for (float k = 0.0f; k <= 1.0f; k += step)
        {
          res += (trilinear_interp(v1, float3(i, j, k)) - trilinear_interp(v2, float3(i, j, k)));
          count += 1.0f;
        }
      }
    }
    return res / count;
  }

  unsigned remove_clustered_surfaces(float *cluster, unsigned count)
  {
    unsigned min_idx = 0, max_idx = 0;
    float tmp = 0;
    if (count <= 2) return count;
    tmp = surface_range_pos_metric(cluster, cluster + 8);
    if (tmp > 0) max_idx = 1;
    else if (tmp < 0) min_idx = 1;
    for (unsigned idx = 2; idx < count; ++idx)
    {
      tmp = surface_range_pos_metric(cluster + 8 * min_idx, cluster + 8 * idx);
      if (tmp < 0) min_idx = idx;
      tmp = surface_range_pos_metric(cluster + 8 * max_idx, cluster + 8 * idx);
      if (tmp > 0) max_idx = idx;
    }
    if (min_idx == max_idx)
    {
      for (unsigned i = 0; i < 8; ++i)
      {
        cluster[i] = cluster[i + 8 * min_idx];
      }
      return 1;
    }
    if (min_idx > max_idx)
    {
      max_idx ^= min_idx;
      min_idx ^= max_idx;
      max_idx ^= min_idx;
    }
    if (min_idx != 0)
    {
      for (unsigned i = 0; i < 8; ++i)
      {
        cluster[i] = cluster[i + 8 * min_idx];
      }
    }
    for (unsigned i = 0; i < 8; ++i)
    {
      cluster[i + 8] = cluster[i + 8 * max_idx];
    }
    return 2;
  }

  unsigned greedy_clustering(float *values_in, unsigned surfaces_count, float *values_clustered, unsigned *sizes)
  {
    const float metric_step = 0.25f;
    const float clustering_thr = 0.3f;

    constexpr int INVALID = -1;
    int remap[GlobalOctree::MAX_SURFACE_COUNT];
    float min_metric[GlobalOctree::MAX_SURFACE_COUNT];
    bool is_parent[GlobalOctree::MAX_SURFACE_COUNT];
    int cluster_ids[GlobalOctree::MAX_SURFACE_COUNT];

    for (unsigned i = 0; i < surfaces_count; i++)
    {
      min_metric[i] = clustering_thr;
      remap[i] = is_valid_surface(values_in + i * 8) ? i : INVALID;
      is_parent[i] = false;
      cluster_ids[i] = INVALID;
    }

    for (unsigned i = 0; i < surfaces_count; i++)
    {
      if (remap[i] != i)
        continue;

      for (unsigned j = i + 1; j < surfaces_count; j++)
      {
        if (is_parent[j] || remap[j] == INVALID)
          continue;
        float m = surface_dir_metric(values_in + i * 8, values_in + j * 8, metric_step);
        if (m < min_metric[j])
        {
          min_metric[j] = m;
          remap[j] = i;
          is_parent[i] = true;
        }
      }
    }

    unsigned cluster_count = 0;
    unsigned cur_off = 0;

    for (unsigned i = 0; i < surfaces_count; i++)
    {
      if (remap[i] < 0)
        continue;
      
        //new cluster
      if (cluster_ids[remap[i]] == INVALID)
      {
        cluster_ids[remap[i]] = cluster_count;
        sizes[cluster_count] = 0;
        cluster_count++;
      }

      cluster_ids[i] = cluster_ids[remap[i]];
      sizes[cluster_ids[remap[i]]]++;
    }

    int cluster_offsets[GlobalOctree::MAX_SURFACE_COUNT];

    cluster_offsets[0] = 0;
    for (int i=1; i<cluster_count; i++)
      cluster_offsets[i] = cluster_offsets[i-1] + 8*sizes[i-1];

    for (unsigned i = 0; i < surfaces_count; i++)
    {
      if (cluster_ids[i] < 0)
        continue;
      int c_id = cluster_ids[i];
      for (int j=0;j<8;j++)
        values_clustered[cluster_offsets[c_id] + j] = values_in[i*8 + j];
      cluster_offsets[c_id] += 8;
    }

  //   #pragma omp critical
  //   {
  //     printf("remaps:");
  //     for (int i=0;i<surfaces_count;i++)
  //       printf("%d ", remap[i]);
  //     printf("\n");
  //   printf("%d clusters with sizes: ", cluster_count);
  //   for (int i=0;i<cluster_count;i++)
  //     printf("%d ", sizes[i]);
  //   printf("\n");
  //   int off = 0;
  //   for (int i=0;i<cluster_count;i++)
  //   {
  //     printf("cluster %d\n", i);
  //     for (int j=0;j<sizes[i];j++)
  //     {
  //       for (int k=0;k<8;k++)
  //         printf("%.6f ", values_clustered[off+k]);
  //       off += 8;
  //       printf("\n");
  //     }
  //     printf("\n");
  //   }
  // }
   
    return cluster_count;
  }

  unsigned remove_surfaces(float *values_f, unsigned surfaces_count)
  {
    float values_clustered[MAX_DISTANCES_PER_NODE];
    unsigned cluster_sizes[GlobalOctree::MAX_SURFACE_COUNT];

    unsigned cluster_count = greedy_clustering(values_f, surfaces_count, values_clustered, cluster_sizes);
    unsigned offset = 0;
    unsigned c_off = 0;
    unsigned final_surfaces_count = 0;
    unsigned valid_surfaces_count = 0;
    for (int c_id = 0; c_id < cluster_count; c_id++)
    {
      valid_surfaces_count += cluster_sizes[c_id];
      if (cluster_sizes[c_id] == 1)
      {
        final_surfaces_count++;
        for (int i=0;i<8;i++)
          values_f[offset + i] = values_clustered[c_off + i];
        offset += 8;
      }
      else
      {
        unsigned new_size = remove_clustered_surfaces(values_clustered + c_off, cluster_sizes[c_id]);
        for (int i=0;i<8*new_size;i++)
          values_f[offset + i] = values_clustered[c_off + i];        
        final_surfaces_count += new_size;
        offset += 8*new_size;
      }
      c_off += 8*cluster_sizes[c_id];
    }

    //if (valid_surfaces_count > final_surfaces_count)
    //  printf("s count %d %d %d cl count %d\n", surfaces_count, valid_surfaces_count, final_surfaces_count, cluster_count);

    return final_surfaces_count;
  }

  void take_brick_part_by_pos_code(GlobalOctreeHeader header, uint4 region_code, const float *values, 
                                   float *res, float &min_val, float &max_val)
  {
    int v_size = header.brick_size + header.brick_pad * 2 + 1;
    uint4 rc = region_code;
    for (int i = -header.brick_pad; i <= header.brick_size + header.brick_pad; ++i)
    {
      for (int j = -header.brick_pad; j <= header.brick_size + header.brick_pad; ++j)
      {
        for (int k = -header.brick_pad; k <= header.brick_size + header.brick_pad; ++k)
        {
          int3 idxs = int3(i, j, k) + header.brick_pad;
          //uint4 rc = octree_pointers[oct_idx].region_code;
          float3 pos_code = (float3(rc.x, rc.y, rc.z) + float3(i, j, k) / header.brick_size)/float(rc.w);
          float tmp_vox[8];
          int min_vox_point_x = pos_code.x * header.brick_size + header.brick_pad;
          int min_vox_point_y = pos_code.y * header.brick_size + header.brick_pad;
          int min_vox_point_z = pos_code.z * header.brick_size + header.brick_pad;//count point idx inside brick
          if (min_vox_point_x == v_size) --min_vox_point_x;
          if (min_vox_point_y == v_size) --min_vox_point_y;
          if (min_vox_point_z == v_size) --min_vox_point_z;//avoid idx overflow
          int3 min_vox_p = int3(min_vox_point_x, min_vox_point_y, min_vox_point_z);
          float3 real_pos_code = pos_code * header.brick_size + header.brick_pad - float3(min_vox_p);
          for (int t = 0; t < 8; ++t)
          {
            int x = min_vox_p.x + ((t >> 2) & 1);
            int y = min_vox_p.y + ((t >> 1) & 1);
            int z = min_vox_p.z + (t & 1);
            tmp_vox[t] = *(values + x * v_size * v_size + y * v_size + z);
          }
          float val = trilinear_interp(tmp_vox, real_pos_code);
          min_val = std::min(min_val, val);
          max_val = std::max(max_val, val);
          res[idxs.x * v_size * v_size + idxs.y * v_size + idxs.z] = val;
        }
      }
    }
  }

  // in:  v_size_in^3, out:  v_size_out^3
  void resample_brick(const float* in, float* out, uint32_t v_size_in, uint32_t v_size_out)
  {
    const uint32_t size_x  = v_size_in  * v_size_in , size_y  = v_size_in;
    const uint32_t size_x2 = v_size_out * v_size_out, size_y2 = v_size_out;

    const float size_coef = float(v_size_in - 1) / (v_size_out - 1);

    for (int x2 = 0; x2 < v_size_out; ++x2)
      for (int y2 = 0; y2 < v_size_out; ++y2)
        for (int z2 = 0; z2 < v_size_out; ++z2)
        {
          float cx = x2 * size_coef, cy = y2 * size_coef, cz = z2 * size_coef;
          int x = cx, y = cy, z = cz; // old voxel offset
          cx -= int(cx); cy -= int(cy); cz -= int(cz); // trilinear interpolation coefs
          if (x >= v_size_in - 1)
          {
            x  -= 1;
            cx += 1;
          }
          if (y >= v_size_in - 1)
          {
            y  -= 1;
            cy += 1;
          }
          if (z >= v_size_in - 1)
          {
            z  -= 1;
            cz += 1;
          }
          cx = 1-cx; cy = 1-cy; cz = 1-cz;

          uint32_t v_offset  = x *size_x  + y *size_y  + z;
          uint32_t v_offset2 = x2*size_x2 + y2*size_y2 + z2;

          out[v_offset2] =    cx  *    cy  *    cz  * in[v_offset                      ] + 
                              cx  *    cy  * (1-cz) * in[v_offset                   + 1] + 
                              cx  * (1-cy) *    cz  * in[v_offset          + size_y    ] + 
                              cx  * (1-cy) * (1-cz) * in[v_offset          + size_y + 1] + 
                           (1-cx) *    cy  *    cz  * in[v_offset + size_x             ] + 
                           (1-cx) *    cy  * (1-cz) * in[v_offset + size_x          + 1] + 
                           (1-cx) * (1-cy) *    cz  * in[v_offset + size_x + size_y    ] + 
                           (1-cx) * (1-cy) * (1-cz) * in[v_offset + size_x + size_y + 1];
        }
  }

  void upsample_global_octree(GlobalOctree &octree, GlobalOctree &octree_new)
  {
    octree_new.values_f.reserve(octree.values_f.size()); // just to start somewhere

    octree_new.header = octree.header;
    octree_new.header.brick_size = 2*octree.header.brick_size;

    octree_new.point_channels = octree.point_channels;
    octree_new.voxel_channels = octree.voxel_channels;

    const uint32_t max_child_nodes = 8;
    uint32_t prev_v_size  = octree.header.brick_size + 2*octree.header.brick_pad + 1;
    uint32_t prev_v_count = prev_v_size*prev_v_size*prev_v_size;

    uint32_t new_b_size  = octree.header.brick_size * 2;
    uint32_t new_v_size  = new_b_size + 2*octree.header.brick_pad + 1;
    uint32_t new_v_count = new_v_size * new_v_size * new_v_size;

    // Merge data members
    MergeInData   in_data{};
    MergeTmpData tmp_data{};

    in_data.distances = new const float* [max_child_nodes]{};
    in_data.surface_counts = new uint16_t[max_child_nodes]{};
    in_data.rotations = new uint16_t[max_child_nodes]{};
    in_data.transpositions = {};

    tmp_data.counts = new uint32_t[new_v_count];            std::fill_n(tmp_data.counts, new_v_count, 0u);
    tmp_data.distances = new float[new_v_count];            std::fill_n(tmp_data.distances, new_v_count, 0.0f);
    tmp_data.frozen = new bool[new_v_count];                std::fill_n(tmp_data.frozen, new_v_count, false);
    tmp_data.updated_values = new bool[new_v_count];        std::fill_n(tmp_data.updated_values, new_v_count, false);
    tmp_data.voxel_is_active = new bool[new_v_count];       std::fill_n(tmp_data.voxel_is_active, new_v_count, false);
    tmp_data.dist_is_used = new bool[new_v_count];          std::fill_n(tmp_data.dist_is_used, new_v_count, false);

    tmp_data.surface_offsets = new uint32_t[max_child_nodes + 1]{};
    tmp_data.all_surfaces = new sdf_converter::SdfSurface[max_child_nodes*sdf_converter::GlobalOctree::MAX_SURFACE_COUNT]{};
    tmp_data.all_surface_values = new float[max_child_nodes*sdf_converter::GlobalOctree::MAX_SURFACE_COUNT*prev_v_count]{};
    tmp_data.surface_ids = new int[max_child_nodes]{};
    tmp_data.codes_stack = new int2[3*max_child_nodes]{};

    tmp_data.single_merge_in_data.distances = new const float* [max_child_nodes]{};
    tmp_data.single_merge_in_data.surface_counts = new uint16_t[max_child_nodes]{};

    // Upsampling part
    upsample_global_octree_node(octree, octree_new, in_data, tmp_data, 0, 0);
    //printf("node -> upsample_global_octree\n");
    upsample_global_octree_rec (octree, octree_new, in_data, tmp_data, 0, 0, 0);
    //printf("rec  -> upsample_global_octree\n");

    // Merge data members deallocation
    delete[] in_data.distances;
    delete[] in_data.surface_counts;
    delete[] in_data.rotations;

    delete[] tmp_data.counts;
    delete[] tmp_data.distances;
    delete[] tmp_data.frozen;
    delete[] tmp_data.updated_values;
    delete[] tmp_data.voxel_is_active;
    delete[] tmp_data.dist_is_used;

    delete[] tmp_data.surface_offsets;   
    delete[] tmp_data.all_surfaces;
    delete[] tmp_data.all_surface_values;
    delete[] tmp_data.surface_ids;
    delete[] tmp_data.codes_stack;

    delete[] tmp_data.single_merge_in_data.distances;
    delete[] tmp_data.single_merge_in_data.surface_counts;

    //printf("Exiting upsampling\n");
  }

  // For a parent node:
  //   1) merges its children;
  //   2) writes new children offset to parent node;
  //   3) called recursively for non-leaf children;
  void upsample_global_octree_rec(GlobalOctree &octree, GlobalOctree &octree_new, MergeInData &in,
                                  MergeTmpData &tmp, uint32_t level, uint32_t node_index, uint32_t new_parent_node_index)
  {
    //printf("upsample_global_octree_rec\n");
    uint32_t node_old_offset = octree.nodes[node_index].offset;
    uint32_t node_new_offset = octree_new.nodes.size(); // + 0..7

    if (node_old_offset == 0) return; // it's a leaf node, ignore and exit

    // We need to know whether all children are leaves or not. If true, node offset is 0 and no recursive calls to children
    bool only_leaves = true;
    for (int i = 0; i < 8; ++i)
    {
      if (octree.nodes[node_old_offset + i].offset != 0)
        only_leaves = false;
    }

    if (only_leaves) return; // This is a leaf in new tree, upsampled already, exiting


    // 1) 'Merge' part
    for (int i = 0; i < 8; ++i) // Applies merge to each of 8 children (or upsampling for leaves) and adds them to 'octree_new.nodes' back to back, same order as in 'octree.nodes'
      upsample_global_octree_node(octree, octree_new, in, tmp, level + 1, node_old_offset + i);

    // 2) 'Write offset' part
    // if (new_parent_node_index != INVALID_IDX)
    octree_new.nodes[new_parent_node_index].offset = node_new_offset;

    // 3) 'Recursive' part
    for (int i = 0; i < 8; ++i)
    {
      GlobalOctreeNode& child_node_old = octree.nodes[node_old_offset + i];
      if (child_node_old.offset != 0)
        upsample_global_octree_rec(octree, octree_new, in, tmp, level + 1, node_old_offset + i, node_new_offset + i);
    }
  }

  // Adds one node to 'octree_new.nodes': merged/upsampled 'octree.nodes[node_index]'
  void upsample_global_octree_node(GlobalOctree &octree, GlobalOctree &octree_new, MergeInData &in,
                                   MergeTmpData &tmp, uint32_t level, uint32_t node_index)
  {
    //printf("upsample_global_octree_node\n");
    constexpr float merge_surface_threshold = 0.01f;
    const uint32_t v_size_old = octree    .header.brick_size + 2*octree    .header.brick_pad + 1;
    const uint32_t v_size_new = octree_new.header.brick_size + 2*octree_new.header.brick_pad + 1;
    uint32_t out_brick_count = 0;

    octree_new.nodes.emplace_back();
    GlobalOctreeNode& node_old = octree.nodes[node_index];
    GlobalOctreeNode& node_new = octree_new.nodes.back();
    node_new.val_off = octree_new.values_f.size();
    // TODO: check if these members require any additional work
    node_new.is_outside  = node_old.is_outside;
    node_new.is_surfaced = node_old.is_surfaced;

    // It's a leaf node, upsample - we only call this function for empty node when its parent has at least one non-leaf child node
    if (octree.nodes[node_index].offset == 0)
    {
      node_new.bricks_count = node_old.bricks_count;
      octree_new.values_f.resize(octree_new.values_f.size() + node_new.bricks_count * (v_size_new*v_size_new*v_size_new));

      for (uint32_t brick_i = 0u; brick_i < node_new.bricks_count; ++brick_i)
      {
        sdf_converter::resample_brick(octree    .values_f.data() + node_new.val_off + brick_i * (v_size_old*v_size_old*v_size_old),
                                      octree_new.values_f.data() + node_old.val_off + brick_i * (v_size_new*v_size_new*v_size_new),
                                      v_size_old, v_size_new);
      }
    }
    else
    {
      // Merge
      int internal_children_count = 0; // zero when all children are leaves; used to set new node type
      for (int i = 0; i < 8; ++i)
      {
        GlobalOctreeNode& child_node_old = octree.nodes[node_old.offset + i];
        in.distances[i] = octree.values_f.data() + child_node_old.val_off;
        in.surface_counts[i] = child_node_old.bricks_count;
        if (child_node_old.offset != 0)
          internal_children_count += 1;
      }

      // if (internal_children_count > 0 && internal_children_count < 8)
      // {
      //   printf("MERGING %d: %d\n", node_index, node_old.type);
      //   fflush(stdout);
      // }

      //printf("merge begin: %d\n", internal_children_count);
      merge_child_bricks(octree.header.brick_size, octree.header.brick_pad, 2, 2.0f / (1 << level),
                         merge_surface_threshold, false, in, tmp, &node_new.bricks_count, octree_new.values_f);
      //printf("merge end\n");

      // Set node type
      float min_val =  1000;
      float max_val = -1000;
      for (int i = 0; i < node_new.bricks_count * (v_size_new*v_size_new*v_size_new); ++i)
      {
          min_val = std::min(min_val, octree_new.values_f[i + node_new.val_off]);
          max_val = std::max(max_val, octree_new.values_f[i + node_new.val_off]);
      }

      float min_val_old =  1000;
      float max_val_old = -1000;
      for (int i = 0; i < node_old.bricks_count * (v_size_old*v_size_old*v_size_old); ++i)
      {
          min_val_old = std::min(min_val_old, octree.values_f[i + node_old.val_off]);
          max_val_old = std::max(max_val_old, octree.values_f[i + node_old.val_off]);
      }

      // if (internal_children_count > 0 && internal_children_count < 8)
      // {
      //   printf("MERGING END: count = %d, [%f, %f] -> [%f, %f], size: %ld\n", node_new.bricks_count, min_val_old, max_val_old, min_val, max_val, octree_new.values_f.size() - node_new.val_off);
      //   fflush(stdout);
      //   exit(0);
      // }

      bool active_node = min_val <= 0 && (max_val >= 0 || true); // settings.fill_internal_volume

      if (internal_children_count == 0)
        node_new.type = active_node ? GlobalOctreeNodeType::LEAF : GlobalOctreeNodeType::EMPTY;
      else if (internal_children_count >  0)
        node_new.type = active_node ? GlobalOctreeNodeType::NODE : GlobalOctreeNodeType::EMPTY_NODE;
      // node_new.type = node_old.type;
    }
  }

  void print_grid(uint32_t brick_size, uint32_t brick_pad, const float *data)
  {
    printf("grid %dx%dx%d, pad = %d\n", brick_size, brick_size, brick_size, brick_pad);
    printf("=====================================\n");
    uint32_t v_size = 2*brick_pad + brick_size + 1;
    for (int y=0;y<v_size;y++)
    {
      for (int x=0;x<v_size;x++)
      {
        if (x != 0)
          printf("| ");
        for (int z=0;z<v_size;z++)
          printf("%8.5f ", data[x*v_size*v_size + y*v_size + z]);
      }
      printf("\n");
    }
  }

  constexpr float MISSING_DISTANCE_UNKNOWN_SIGN = 8.8888f;
  constexpr float MISSING_DISTANCE              = 9.9999f;

  void prepare_brick_for_redistancing(uint32_t out_v_size, uint32_t out_v_count, sdf_converter::MergeTmpData &tmp)
  {
    bool has_unknown_signs = true;
    uint32_t iter = 0;
    while (has_unknown_signs)
    {
      iter++;
      if (iter > 100)
      {
        printf("error brick\n");
        for (int x = 0; x < out_v_size; x++)
        {
          for (int y = 0; y < out_v_size; y++)
          {
            for (int z = 0; z < out_v_size; z++)
            {
              printf("%8.5f ", tmp.distances[x * out_v_size * out_v_size + y * out_v_size + z]);
            }
            printf("\n");
          }
          printf("####\n");
        }
      }
      has_unknown_signs = false;
      for (int x = 0; x < out_v_size; x++)
      {
        for (int y = 0; y < out_v_size; y++)
        {
          for (int z = 0; z < out_v_size; z++)
          {
            uint32_t has_pos = 0;
            uint32_t has_neg = 0;
            if (tmp.distances[x * out_v_size * out_v_size + y * out_v_size + z] == MISSING_DISTANCE_UNKNOWN_SIGN)
            {
              has_unknown_signs = true;
              float v[6];
              v[0] = x > 0 ? tmp.distances[(x - 1) * out_v_size * out_v_size + y * out_v_size + z] : MISSING_DISTANCE_UNKNOWN_SIGN;
              v[1] = x < out_v_size - 1 ? tmp.distances[(x + 1) * out_v_size * out_v_size + y * out_v_size + z] : MISSING_DISTANCE_UNKNOWN_SIGN;
              v[2] = y > 0 ? tmp.distances[x * out_v_size * out_v_size + (y - 1) * out_v_size + z] : MISSING_DISTANCE_UNKNOWN_SIGN;
              v[3] = y < out_v_size - 1 ? tmp.distances[x * out_v_size * out_v_size + (y + 1) * out_v_size + z] : MISSING_DISTANCE_UNKNOWN_SIGN;
              v[4] = z > 0 ? tmp.distances[x * out_v_size * out_v_size + y * out_v_size + z - 1] : MISSING_DISTANCE_UNKNOWN_SIGN;
              v[5] = z < out_v_size - 1 ? tmp.distances[x * out_v_size * out_v_size + y * out_v_size + z + 1] : MISSING_DISTANCE_UNKNOWN_SIGN;

              for (int w = 0; w < 6; w++)
              {
                has_pos += (v[w] >= 0 && v[w] != MISSING_DISTANCE_UNKNOWN_SIGN);
                has_neg += (v[w] <= 0 && v[w] != MISSING_DISTANCE_UNKNOWN_SIGN);
              }
              if (has_pos + has_neg > 0)
                tmp.distances[x * out_v_size * out_v_size + y * out_v_size + z] = has_pos >= has_neg ? MISSING_DISTANCE : -MISSING_DISTANCE;
              // else
              //   printf("%f %f %f %f %f %f\n", v[0], v[1], v[2], v[3], v[4], v[5]);
            }
          }
        }
      }
    }

    bool has_conflicting_signs = true;
    uint32_t steps = 0;
    while (has_conflicting_signs)
    {
      steps++;
      if (steps > out_v_count)
      {
        printf("infinite loop detected\n");
        break;
      }
      has_conflicting_signs = false;
      for (int x = 0; x < out_v_size; x++)
      {
        for (int y = 0; y < out_v_size; y++)
        {
          for (int z = 0; z < out_v_size; z++)
          {
            uint32_t has_pos = 0;
            uint32_t has_neg = 0;

            if (tmp.distances[x * out_v_size * out_v_size + y * out_v_size + z] == -MISSING_DISTANCE ||
                tmp.distances[x * out_v_size * out_v_size + y * out_v_size + z] == MISSING_DISTANCE)
            {
              float v[6];
              v[0] = x > 0 ? tmp.distances[(x - 1) * out_v_size * out_v_size + y * out_v_size + z] : MISSING_DISTANCE_UNKNOWN_SIGN;
              v[1] = x < out_v_size - 1 ? tmp.distances[(x + 1) * out_v_size * out_v_size + y * out_v_size + z] : MISSING_DISTANCE_UNKNOWN_SIGN;
              v[2] = y > 0 ? tmp.distances[x * out_v_size * out_v_size + (y - 1) * out_v_size + z] : MISSING_DISTANCE_UNKNOWN_SIGN;
              v[3] = y < out_v_size - 1 ? tmp.distances[x * out_v_size * out_v_size + (y + 1) * out_v_size + z] : MISSING_DISTANCE_UNKNOWN_SIGN;
              v[4] = z > 0 ? tmp.distances[x * out_v_size * out_v_size + y * out_v_size + z - 1] : MISSING_DISTANCE_UNKNOWN_SIGN;
              v[5] = z < out_v_size - 1 ? tmp.distances[x * out_v_size * out_v_size + y * out_v_size + z + 1] : MISSING_DISTANCE_UNKNOWN_SIGN;

              for (int w = 0; w < 6; w++)
              {
                has_pos += (v[w] >= 0 && v[w] != MISSING_DISTANCE_UNKNOWN_SIGN);
                has_neg += (v[w] <= 0 && v[w] != MISSING_DISTANCE_UNKNOWN_SIGN);
              }

              float new_dist = has_pos >= has_neg ? MISSING_DISTANCE : -MISSING_DISTANCE;
              has_conflicting_signs |= (new_dist != tmp.distances[x * out_v_size * out_v_size + y * out_v_size + z]);
              tmp.distances[x * out_v_size * out_v_size + y * out_v_size + z] = new_dist;
            }
          }
        }
      }
    }
  }

  void merge_child_bricks_single(uint32_t brick_size, uint32_t brick_pad, uint32_t node_grid_size,
                                 float merged_brick_length, float surface_match_threshold,
                                 MergeInData &in, MergeTmpData &tmp,
                                 uint32_t *out_surface_count, std::vector<float> &out_distances)
  {
    bool apply_rotations = in.rotations && (in.transpositions.size() > 0);

    uint32_t brick_count = node_grid_size*node_grid_size*node_grid_size;
    uint32_t v_size = 2*brick_pad + brick_size + 1;
    uint32_t out_v_size = brick_size*node_grid_size + 1;
    uint32_t out_v_count = out_v_size*out_v_size*out_v_size;

    std::fill_n(tmp.counts, out_v_count, 0);
    std::fill_n(tmp.distances, out_v_count, 0.0f);
    
    // save values from children to the distances grid
    for (uint32_t i = 0; i < brick_count; ++i)
    {
      if (in.distances[i] == nullptr)
        continue;

      find_active_voxels_and_distances(brick_size, in.distances[i], tmp.voxel_is_active, tmp.dist_is_used);

      uint3 b_pos = uint3(i/(node_grid_size*node_grid_size), i/node_grid_size%node_grid_size, i%node_grid_size);
      for (uint32_t j = 0; j < v_size*v_size*v_size; ++j)
      {
        uint3 v_pos = uint3(j/(v_size*v_size), j/v_size%v_size, j%v_size);
        uint32_t idx = dot(b_pos*brick_size + v_pos, uint3(out_v_size*out_v_size, out_v_size, 1));
        uint32_t voxel_idx = apply_rotations ? in.transpositions[in.rotations[i]][j] : j;
        tmp.counts[idx] += tmp.dist_is_used[voxel_idx];
        tmp.distances[idx] += tmp.dist_is_used[voxel_idx] * in.distances[i][voxel_idx];
      }
    }

    // averaging values in corners or mark them with MISSING_DISTANCE value
    uint32_t active_distances = 0;
    for (uint32_t i = 0; i < out_v_count; ++i)
    {
      if (tmp.counts[i] > 0)
      {
        active_distances++;
        tmp.distances[i] /= tmp.counts[i];
      }
      else
      {
        tmp.distances[i] = MISSING_DISTANCE_UNKNOWN_SIGN;
      }
    }

    // no active distances in merged brick, so no surfaces actually
    if (active_distances == 0)
    {
      *out_surface_count = 0;
      return;
    }

    uint32_t dist_off = out_distances.size();
    out_distances.resize(dist_off + out_v_count, 0.0f);
    *out_surface_count = 1;

    //printf("final grid RAW\n");
    //print_grid(brick_size*node_grid_size, 0, tmp.distances);

    prepare_brick_for_redistancing(out_v_size, out_v_count, tmp);

    //printf("final grid FIXED\n");
    //print_grid(brick_size*node_grid_size, 0, tmp.distances);

    // fill missing distances
    std::fill_n(tmp.frozen, out_v_count, false);
    std::fill_n(tmp.updated_values, out_v_count, false);
    memcpy(out_distances.data() + dist_off, tmp.distances, out_v_count * sizeof(float));
    redistance_fast_sweep(out_distances.data() + dist_off, uint3(out_v_size, out_v_size, out_v_size), false, merged_brick_length/(brick_size*node_grid_size),
                          out_v_size, tmp.distances, tmp.updated_values, tmp.frozen, MISSING_DISTANCE - 0.001f);

    //printf("final grid REDISTANCED\n");
    //print_grid(brick_size*node_grid_size, 0, out_distances.data() + dist_off);
  
    //for (uint32_t i = 0; i < out_v_count; ++i)
    //  assert(std::abs(out_distances[dist_off + i]) < 9);
  }

  bool is_valid_surface(const float *values, uint32_t cnt)
  {
    float mn = 1000, mx = -1000;

    for (uint32_t i = 0; i < cnt; ++i)
    {
      mn = std::min(mn, values[i]);
      mx = std::max(mx, values[i]);
    }

    return mn < 0 && mx > 0;
  }

  struct SideIter
  {
    uint32_t size     = 2; // v_size
    uint32_t offset   = 0;
    uint32_t stride_i = 2;
    uint32_t stride_j = 1;
  };

  static const int neighbors[8][SIDE_COUNT] =
  {
    {NONE,    4,    NONE,    2,    NONE,    1},
    {NONE,    5,    NONE,    3,    0,    NONE},
    {NONE,    6,    0,    NONE,    NONE,    3},
    {NONE,    7,    1,    NONE,    2,    NONE},
    {0,    NONE,    NONE,    6,    NONE,    5},
    {1,    NONE,    NONE,    7,    4,    NONE},
    {2,    NONE,    4,    NONE,    NONE,    7},
    {3,    NONE,    5,    NONE,    6,    NONE},
  };

  static const int codeA[8][8] = 
  {
    //        0,          1,          2,          3,          4,          5,          6,          7
    {      NONE, SIDE_Z_POS, SIDE_Y_POS,       NONE, SIDE_X_POS,       NONE,       NONE,       NONE},
    {SIDE_Z_NEG,       NONE,       NONE, SIDE_Y_POS,       NONE, SIDE_X_POS,       NONE,       NONE},
    {SIDE_Y_NEG,       NONE,       NONE, SIDE_Z_POS,       NONE,       NONE, SIDE_X_POS,       NONE},
    {      NONE, SIDE_Y_NEG, SIDE_Z_NEG,       NONE,       NONE,       NONE,       NONE, SIDE_X_POS},
    {SIDE_X_NEG,       NONE,       NONE,       NONE,       NONE, SIDE_Z_POS, SIDE_Y_POS,       NONE},
    {      NONE, SIDE_X_NEG,       NONE,       NONE, SIDE_Z_NEG,       NONE,       NONE, SIDE_Y_POS},
    {      NONE,       NONE, SIDE_X_NEG,       NONE, SIDE_Y_NEG,       NONE,       NONE, SIDE_Z_POS},
    {      NONE,       NONE,       NONE, SIDE_X_NEG,       NONE, SIDE_Y_NEG, SIDE_Z_NEG,       NONE},
  };

  SideIter get_side_iter(uint32_t v_size, int side)
  {
    switch (side)
    {
      case SIDE_X_NEG:
        return {v_size, 0, v_size, 1};
      case SIDE_X_POS:
        return {v_size, (v_size - 1) * v_size * v_size, v_size, 1};
      case SIDE_Y_NEG:
        return {v_size, 0, v_size * v_size, 1};
      case SIDE_Y_POS:
        return {v_size, (v_size - 1) * v_size, v_size * v_size, 1};
      case SIDE_Z_NEG:
        return {v_size, 0, v_size * v_size, v_size};
      case SIDE_Z_POS:
        return {v_size, (v_size - 1), v_size * v_size, v_size};
      default:
        assert(false);
    }
    return SideIter();
  }

  void calculate_side_intersect(SdfSurface &surface, uint32_t v_size)
  {
    for (int s = 0; s < SIDE_COUNT; s++)
    {
      SideIter iter = get_side_iter(v_size, s);
      float min = 1000, max = -1000;

      for (uint32_t i = 0; i < iter.size; i++)
      {
        for (uint32_t j = 0; j < iter.size; j++)
        {
          float val = surface.values[iter.offset + i * iter.stride_i + j * iter.stride_j];
          min = std::min(min, val);
          max = std::max(max, val);
        }
      }

      surface.side_intersect[s] = (min < 0 && max > 0);
    }
  }

  float sides_distance(const SdfSurface &s1, SideIter iter1, const SdfSurface &s2, SideIter iter2)
  {
    assert(iter1.size == iter2.size);
    float mse = 0.0f;
    for (uint32_t i = 0; i < iter1.size; i++)
    {
      for (uint32_t j = 0; j < iter1.size; j++)
      {
        float v1 = s1.values[iter1.offset + i * iter1.stride_i + j * iter1.stride_j];
        float v2 = s2.values[iter2.offset + i * iter2.stride_i + j * iter2.stride_j];
        mse += (v1 - v2) * (v1 - v2);
      }
    }
    return mse / (iter1.size * iter1.size);
  }

  void merge_child_bricks_multi(uint32_t brick_size, uint32_t brick_pad, uint32_t node_grid_size, 
                                float merged_brick_length, float surface_match_threshold,
                                MergeInData &in, MergeTmpData &tmp,
                                uint32_t *out_surface_count, std::vector<float> &out_distances)
  {
    static constexpr uint32_t MAX_BINS = 5;
    static uint32_t all_ms = 0;
    static uint32_t hanging_ms = 0;
    static uint32_t node_count = 0;
    static uint32_t surface_count_bins[MAX_BINS] = {0};

    const bool print_stat = false;
    const bool verbose = false;
    const bool discard_hanging_edges = false;
    const bool apply_rotations = in.rotations && (in.transpositions.size() > 0);

    const uint32_t brick_count = node_grid_size*node_grid_size*node_grid_size;
    const uint32_t v_size = 2*brick_pad + brick_size + 1;
    const uint32_t v_count = v_size*v_size*v_size;
    const uint32_t out_v_size = brick_size*node_grid_size + 1;
    const uint32_t out_v_count = out_v_size*out_v_size*out_v_size;

    uint32_t total_surface_count = 0;

    //fill array with initial surfaces
    tmp.surface_offsets[0] = 0;
    for (uint32_t i = 0; i < brick_count; ++i)
    {
      for (uint32_t j = 0; j < std::min<uint32_t>(GlobalOctree::MAX_SURFACE_COUNT, in.surface_counts[i]); ++j)
      {
        if (in.distances[i] != nullptr && is_valid_surface(in.distances[i] + j*v_count, v_count))
        {
          for (uint32_t idx = 0; idx < v_count; idx++)
          {
            uint32_t voxel_idx = apply_rotations ? in.transpositions[in.rotations[i]][idx] : idx;
            tmp.all_surface_values[total_surface_count*v_count + idx] = in.distances[i][j*v_count + voxel_idx];
          }

          tmp.all_surfaces[total_surface_count].values = tmp.all_surface_values + total_surface_count*v_count;
          tmp.all_surfaces[total_surface_count].flags = SURFACE_UNUSED;
          tmp.all_surfaces[total_surface_count].child_id = i;
          calculate_side_intersect(tmp.all_surfaces[total_surface_count], v_size);
          total_surface_count++;
        }
      }
      tmp.surface_offsets[i+1] = total_surface_count;
    }

    if (total_surface_count == 0)
    {
      *out_surface_count = 0;
      return;
    }

    if (verbose)
    {
      printf("Grid has size %dx%dx%d and %d surfaces in total\n", 
             node_grid_size, node_grid_size, node_grid_size, total_surface_count);
      for (int i = 0; i < brick_count; i++)
      {
        printf("Brick %d %d %d has %d surfaces\n", 
               i/(node_grid_size*node_grid_size), i/node_grid_size%node_grid_size, i%node_grid_size, 
               tmp.surface_offsets[i+1] - tmp.surface_offsets[i]);
        for (int j = tmp.surface_offsets[i]; j < tmp.surface_offsets[i+1]; j++)
        {
          printf("  SdfSurface %d: sides %d %d %d %d %d %d\n", 
                 j, tmp.all_surfaces[j].side_intersect[0], tmp.all_surfaces[j].side_intersect[1],
                    tmp.all_surfaces[j].side_intersect[2], tmp.all_surfaces[j].side_intersect[3],
                    tmp.all_surfaces[j].side_intersect[4], tmp.all_surfaces[j].side_intersect[5]);
          print_grid(brick_size, 0, tmp.all_surfaces[j].values);
        }
      }
    }

    //trying to merge surfaces into multi-surface (surface for parent that consists of multiple child surfaces)
    uint32_t surfaces_left = total_surface_count;
    uint32_t multi_surface_count = 0;

    // Neighbours and codeA arrays are only for 2x2x2 nodes currently
    // It is possible to create similar arrays for arbitrary 2^n-tree,
    // but it is not needed for now, we are probably use only octrees here
    assert(brick_count == 8);

    while (surfaces_left > 0)
    {
      //first, find some unused surface as a starting point
      uint32_t surface_id = 0;
      while (tmp.all_surfaces[surface_id].flags != SURFACE_UNUSED)
        surface_id++;
      
      //surface ids of current multi-surface
      int top = 0;
      for (int i = 0; i < brick_count; ++i)
        tmp.surface_ids[i] = NONE;

      int start_ch_id = tmp.all_surfaces[surface_id].child_id;
      tmp.surface_ids[start_ch_id] = surface_id;
      surfaces_left--;
      tmp.all_surfaces[surface_id].flags = SURFACE_USED;
      for (int i = 0; i < SIDE_COUNT; i++)
      {
        if (tmp.all_surfaces[surface_id].side_intersect[i] && neighbors[start_ch_id][i] != NONE)
          tmp.codes_stack[top++] = int2(start_ch_id, neighbors[start_ch_id][i]);
      }

      // we are trying to connect as many surfaces as possible to our current multi-surface
      // with three conditions:
      // 1. all surfaces in multi-surface should be connected to each other (i.e. have common side)
      // 2. all surfaces should match with each other (i.e. have similar values on the common side)
      // 3. multi surface should not have more that one surface in each child node
      bool hanging_edges = false;
      while (top > 0 && !hanging_edges)
      {
        int2 code = tmp.codes_stack[--top];
        //printf("testing %d %d side %d\n", code.x, code.y, codeA[code.x][code.y]);
        const auto &surf_a = tmp.all_surfaces[tmp.surface_ids[code.x]];
        const auto  side_a = get_side_iter(v_size, codeA[code.x][code.y]);
        
        //find surface in id2 with best match for surface in id1
        //if it is found, add it to multi-surface and add its neighbors to the stack
        if (tmp.surface_ids[code.y] == NONE)
        {
          float min_distance = surface_match_threshold;
          int min_surface_id = NONE;
          //printf("check surfaces %d - %d\n", surface_offsets[code.y], surface_offsets[code.y + 1]);
          for (int i = tmp.surface_offsets[code.y]; i < tmp.surface_offsets[code.y + 1]; i++)
          {
            if (tmp.all_surfaces[i].flags != SURFACE_UNUSED)
              continue;
            const auto &surf_b = tmp.all_surfaces[i];
            const auto  side_b = get_side_iter(v_size, codeA[code.y][code.x]);
            float dist = sides_distance(surf_a, side_a, surf_b, side_b);
            if (dist < min_distance)
            {
              min_distance = dist;
              min_surface_id = i;
            }
          }
          if (min_surface_id != NONE)
          {
            tmp.surface_ids[code.y] = min_surface_id;
            surfaces_left--;
            tmp.all_surfaces[min_surface_id].flags = SURFACE_USED;
            for (int i = 0; i < SIDE_COUNT; i++)
            {
              if (tmp.all_surfaces[min_surface_id].side_intersect[i] && neighbors[code.y][i] != NONE)
                tmp.codes_stack[top++] = int2(code.y, neighbors[code.y][i]);
            }
          }
          else
          {
            // no matching surface found, so we have hanging edge
            hanging_edges = true;

            if (verbose)
              printf("no matching surface found for %d %d, hanging edge\n", code.x, code.y);
          }
        }
        else
        {
          //check if distance between surface in id1 and id2 is small enough, otherwise it is hanging edge
          const auto &surf_b = tmp.all_surfaces[tmp.surface_ids[code.y]];
          const auto  side_b = get_side_iter(v_size, codeA[code.y][code.x]);
          if (sides_distance(surf_a, side_a, surf_b, side_b) >= surface_match_threshold)
          {
            hanging_edges = true;

            if (verbose)
              printf("hanging edge found between %d and %d, distance is too big (%f > %f)\n", code.x, code.y, 
                     sides_distance(surf_a, side_a, surf_b, side_b), surface_match_threshold);
          }
        }
      }

      if (hanging_edges && discard_hanging_edges)
      {
        printf("discarded multi-surface %d because of hanging edge\n", multi_surface_count);
        continue;
      }

      all_ms++;
      hanging_ms += hanging_edges;

      if (verbose)
        printf("found multi-surface %d\n", multi_surface_count);
      multi_surface_count++;

      // now we have a multi-surface, we need to merge it into the output grid
      // use merge_child_bricks_single to merge all surfaces in the multi-surface
      // into a single surface, and then add it to the output grid
      std::fill_n(tmp.single_merge_in_data.distances, brick_count, nullptr);
      std::fill_n(tmp.single_merge_in_data.surface_counts, brick_count, 0u);
      tmp.single_merge_in_data.rotations      = nullptr; // are are using already rotated bricks, so we don't need rotations again
      tmp.single_merge_in_data.transpositions = {};

      for (uint32_t i = 0; i < brick_count; ++i)
      {
        tmp.single_merge_in_data.distances[i] = ((tmp.surface_ids[i] == NONE) ? nullptr : tmp.all_surfaces[tmp.surface_ids[i]].values);
        tmp.single_merge_in_data.surface_counts[i] = (tmp.surface_ids[i] == NONE) ? 0 : 1;
      }

      merge_child_bricks_single(brick_size, brick_pad, node_grid_size, 
                                merged_brick_length, surface_match_threshold,
                                tmp.single_merge_in_data, tmp, out_surface_count, out_distances);
    }

    if (verbose)
    {
      printf("Merged %d surfaces in %d nodes to %d multi-surfaces\n",
             total_surface_count, brick_count, multi_surface_count);
      printf("Final grid has size %dx%dx%d and %d surfaces in total\n", 
             brick_size*node_grid_size, brick_size*node_grid_size, brick_size*node_grid_size, multi_surface_count);
      for (int i = 0; i < multi_surface_count; i++)
      {
        print_grid(brick_size*node_grid_size, 0, tmp.distances + i*out_v_count);
      }
    }

    *out_surface_count = multi_surface_count;

    node_count++;
    surface_count_bins[std::min(multi_surface_count, MAX_BINS-1)]++;

    if (node_count % 1000 == 0 && print_stat)
    {
      printf("%d nodes merged\n", node_count);
      printf("%d/%d surfaces hanging\n", hanging_ms, all_ms);
      printf("Multi-surface counts: %d %d %d %d %d\n", surface_count_bins[0],
             surface_count_bins[1], surface_count_bins[2], surface_count_bins[3], surface_count_bins[4]);
    }
  }

  void merge_child_bricks(uint32_t brick_size, uint32_t brick_pad, uint32_t node_grid_size, 
                          float merged_brick_length, float surface_match_threshold,
                          bool always_merge_single_bricks, MergeInData &in, MergeTmpData &tmp,
                          uint32_t *out_surface_count, std::vector<float> &out_distances)
  {
    // Supporting padding here is a tricky thing, possible but difficult
    // so no padding here, until it will be REALLY needed.
    assert(brick_pad == 0);

    uint16_t max_surface_count = 0;
    for (uint32_t i = 0; i < node_grid_size*node_grid_size*node_grid_size; ++i)
      max_surface_count = std::max(max_surface_count, in.surface_counts[i]);
    
    if (max_surface_count == 0)
    {
      *out_surface_count = 0;
    }
    else if (max_surface_count == 1 && always_merge_single_bricks)
    {
      // If all the children have 0 or 1 bricks (no multi-nodes here)
      // then we can use simplified procedure to merge the bricks
      // This procedure is faster, but assumes that all children surfaces
      // match with each other (which is not aways true)
      merge_child_bricks_single(brick_size, brick_pad, node_grid_size, 
                                merged_brick_length, surface_match_threshold,
                                in, tmp, out_surface_count, out_distances);
    }
    else
    {
      merge_child_bricks_multi(brick_size, brick_pad, node_grid_size, 
                               merged_brick_length, surface_match_threshold,
                               in, tmp, out_surface_count, out_distances);
    }
  }

  void find_active_voxels_and_distances(uint32_t brick_size, const float *distances, bool *voxel_is_active, bool *dist_is_used,
                                        float min_add, float max_add)
  {
    float vvals[8];
    uint32_t s = brick_size + 1;
    for (uint32_t x = 0; x < brick_size; x++)
    {
      for (uint32_t y = 0; y < brick_size; y++)
      {
        for (uint32_t z = 0; z < brick_size; z++)
        {
          uint32_t voxel_id = x * brick_size * brick_size + y * brick_size + z;
          uint32_t dist_id = x * s * s + y * s + z;

          vvals[0] = distances[dist_id];
          vvals[1] = distances[dist_id + 1];
          vvals[2] = distances[dist_id + s];
          vvals[3] = distances[dist_id + s + 1];
          vvals[4] = distances[dist_id + s * s];
          vvals[5] = distances[dist_id + s * s + 1];
          vvals[6] = distances[dist_id + s * s + s];
          vvals[7] = distances[dist_id + s * s + s + 1];

          float min_val = vvals[0];
          float max_val = vvals[0];
          for (int j = 1; j < 8; j++)
          {
            min_val = LiteMath::min(min_val, vvals[j]);
            max_val = LiteMath::max(max_val, vvals[j]);
          }

          voxel_is_active[voxel_id] = LiteMath::sign(min_val + min_add) != LiteMath::sign(max_val + max_add);
        }
      }
    }

    // find used distances
    std::fill_n(dist_is_used, s * s * s, false);
    for (uint32_t x = 0; x < brick_size; x++)
    {
      for (uint32_t y = 0; y < brick_size; y++)
      {
        for (uint32_t z = 0; z < brick_size; z++)
        {
          uint32_t voxel_id = x * brick_size * brick_size + y * brick_size + z;
          uint32_t dist_id = x * s * s + y * s + z;

          if (!voxel_is_active[voxel_id])
            continue;

          dist_is_used[dist_id] = true;
          dist_is_used[dist_id + 1] = true;
          dist_is_used[dist_id + s] = true;
          dist_is_used[dist_id + s + 1] = true;
          dist_is_used[dist_id + s * s] = true;
          dist_is_used[dist_id + s * s + 1] = true;
          dist_is_used[dist_id + s * s + s] = true;
          dist_is_used[dist_id + s * s + s + 1] = true;
        }
      }
    }
  }
}