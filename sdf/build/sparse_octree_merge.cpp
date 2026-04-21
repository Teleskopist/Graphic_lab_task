#include "sparse_octree_builder.h"
#include "sparse_octree_common.h"
#include "sparse_octree_augmentation.h"
#include "utils/common/int_pow.h"

#include <set>
#include <map>

namespace sdf_converter
{
  GlobalOctreeNodeType operator||(const GlobalOctreeNodeType &a, const GlobalOctreeNodeType &b)
  {
    if (a == GlobalOctreeNodeType::NODE || b == GlobalOctreeNodeType::NODE) return GlobalOctreeNodeType::NODE;
    if (a == GlobalOctreeNodeType::EMPTY_NODE && b == GlobalOctreeNodeType::LEAF) return GlobalOctreeNodeType::NODE;
    if (a == GlobalOctreeNodeType::LEAF && b == GlobalOctreeNodeType::EMPTY_NODE) return GlobalOctreeNodeType::NODE;
    if (a == GlobalOctreeNodeType::EMPTY) return b;
    return a;
  }

  void count_edges_ratio(float A, unsigned edge_A, float &B, unsigned edge_B, 
                         float vox_sgn[8], float *voxels, unsigned vox_q, 
                         const std::set<unsigned> &usefull_vox)
  {
    if (vox_sgn[edge_A] * vox_sgn[edge_B] > 0)
    {
      float A_m = 0, B_m = 0;
      unsigned cnt = 0;
      for (auto i : usefull_vox)
      {
        if ((vox_sgn[edge_A] > 0 && voxels[8 * i + edge_A] > 0 && voxels[8 * i + edge_B] > 0) || 
            (vox_sgn[edge_A] < 0 && voxels[8 * i + edge_A] <= 0 && voxels[8 * i + edge_B] <= 0))
        {
          ++cnt;
          A_m += voxels[8 * i + edge_A];
          B_m += voxels[8 * i + edge_B];
        }
      }
      if (cnt > 0)
      {
        if (B_m == 0)
        {
          B = 0;
          return;
        }
        if (A_m == 0)
        {
          B = 10 * vox_sgn[edge_B];
          return;
        }
        B = A * B_m / A_m;
        return;
      }
      B = A;
      return;
    }
    float coeff = 2;
    for (auto i : usefull_vox)
    {
      if (vox_sgn[edge_A] > 0 && voxels[8 * i + edge_A] > 0 && voxels[8 * i + edge_B] <= 0)
      {
        if (voxels[8 * i + edge_A] / (voxels[8 * i + edge_A] - voxels[8 * i + edge_B]) < coeff && 
            voxels[8 * i + edge_A] / (voxels[8 * i + edge_A] - voxels[8 * i + edge_B]) > 0)
        {
          coeff = voxels[8 * i + edge_A] / (voxels[8 * i + edge_A] - voxels[8 * i + edge_B]);
        }
      }
      else if (vox_sgn[edge_B] > 0 && voxels[8 * i + edge_A] <= 0 && voxels[8 * i + edge_B] > 0)
      {
        if (voxels[8 * i + edge_B] / (-voxels[8 * i + edge_A] + voxels[8 * i + edge_B]) < coeff && 
            voxels[8 * i + edge_B] / (-voxels[8 * i + edge_A] + voxels[8 * i + edge_B]) > 0)
        {
          coeff = voxels[8 * i + edge_B] / (-voxels[8 * i + edge_A] + voxels[8 * i + edge_B]);
        }
      }
    }
    if (vox_sgn[edge_B] > 0)
    {
      if (coeff == 0)
      {
        B = 0;
      }
      else if (coeff == 1)
      {
        B = 10 * vox_sgn[edge_B];
      }
      else
      {
        B = A * coeff / (coeff - 1);
        //printf(" # %f # ", coeff);
      }
      return;
    }
    if (coeff == 0)
    {
      B = 10 * vox_sgn[edge_B];
    }
    else if (coeff == 1)
    {
      B = 0;
    }
    else
    {
      //printf(" # %f # ", coeff);
      B = A * (coeff - 1) / coeff;
    }
  }

  unsigned voxel_used_on_edge(unsigned edge_A, unsigned edge_B, 
                              float vox_sgn[8], float *voxels, unsigned vox_q)
  {
    float coeff = 2;
    unsigned res = 0;
    for (unsigned i = 0; i < vox_q; ++i)
    {
      if (vox_sgn[edge_A] > 0 && voxels[8 * i + edge_A] > 0 && voxels[8 * i + edge_B] <= 0)
      {
        if (voxels[8 * i + edge_A] / (voxels[8 * i + edge_A] - voxels[8 * i + edge_B]) < coeff && 
            voxels[8 * i + edge_A] / (voxels[8 * i + edge_A] - voxels[8 * i + edge_B]) > 0)
        {
          coeff = voxels[8 * i + edge_A] / (voxels[8 * i + edge_A] - voxels[8 * i + edge_B]);
          res = i;
        }
      }
      else if (vox_sgn[edge_B] > 0 && voxels[8 * i + edge_A] <= 0 && voxels[8 * i + edge_B] > 0)
      {
        if (voxels[8 * i + edge_B] / (-voxels[8 * i + edge_A] + voxels[8 * i + edge_B]) < coeff && 
            voxels[8 * i + edge_B] / (-voxels[8 * i + edge_A] + voxels[8 * i + edge_B]) > 0)
        {
          coeff = voxels[8 * i + edge_B] / (-voxels[8 * i + edge_A] + voxels[8 * i + edge_B]);
          res = i;
        }
      }
    }
    return res;
  }

  bool is_merged_voxel_correct(float vox[8])
  {
    unsigned counts[6] = {0, 0, 0, 0, 0, 0};
    for (unsigned i = 0; i < 8; ++i)
    {
      //printf("%f ", vox[i]);
      for (unsigned j = 0; j < 3; ++j)
      {
        if (i != (i | (1 << j)) && vox[i] * vox[i | (1 << j)] <= 0)
        {
          for (unsigned u = 0; u < 3; ++u)
          {
            if (j != u)
            {
              unsigned num = (3 * (i & (1 << u)) / (1 << u)) + u;
              counts[num]++;
            }
          }
        }
      }
    }
    //printf("\n");
    for (unsigned i = 0; i < 6; ++i)
    {
      assert(counts[i] <= 4 && counts[i] % 2 == 0);
      if (counts[i] == 4) return false;
    }
    return true;
  }

  static float get_max_sdf_val(float level_sz)
  {
    return /*sqrt(3)*/ 1.7320508f * 2.0f / level_sz;
  }

  void normalize_bricks_rec(GlobalOctree &octree, unsigned node, unsigned depth)
  {
    unsigned size = octree.header.brick_pad * 2 + octree.header.brick_size + 1;
    size = size * size * size;
    unsigned off = octree.nodes[node].val_off;
    float sq_sum = 0;
    for (unsigned i = 0; i < octree.nodes[node].bricks_count; ++i)
    {
      float max_abs_val = 1e-9f;
      for (unsigned j = 0; j < size; ++j)
        max_abs_val = std::max(max_abs_val, std::abs(octree.values_f[off + size * i + j]));
      
      // without (1 << depth) there are some rendering issues
      float norm = get_max_sdf_val(1 << depth) / max_abs_val;
      for (unsigned j = 0; j < size; ++j)
        octree.values_f[octree.nodes[node].val_off + size * i + j] *= norm;
    }
    if (octree.nodes[node].offset != 0)
    {
      for (unsigned i = 0; i < 8; ++i)
      {
        normalize_bricks_rec(octree, octree.nodes[node].offset + i, depth + 1);
      }
    }
  }

  void normalize_bricks(GlobalOctree &octree, uint4 octree_pos_code)
  {
    normalize_bricks_rec(octree, 0, 1 + round(log2(octree_pos_code.w)));
  }

  bool merge_voxels(float result[8], float *voxels, unsigned vox_quantity)
  {
    float voxel_sgn[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    float res_sq_sum = 0;
    for (unsigned i = 0; i < vox_quantity; ++i)
    {
      for (unsigned j = 0; j < 8; ++j)
      {
        if (i == 0)
        {
          res_sq_sum += voxels[j] * voxels[j];
        }
        //printf("%f ", voxels[i * 8 + j]);
        if (voxels[i * 8 + j] <= 0)
          voxel_sgn[j] = -1;
      }
      //printf("\n");
    }
    res_sq_sum = std::sqrt(res_sq_sum);
    if (!is_merged_voxel_correct(voxel_sgn))
    {
      //printf("AAA\n");
      return false;
    }
    bool counted[8] = {1, 0, 0, 0, 0, 0, 0, 0};
    float sq_sum = 0;
    std::vector<unsigned> counting(1, 0);
    std::vector<unsigned> count_pair(1, 0);
    std::set<unsigned> usefull_vox_nums;
    std::vector<unsigned> connections[8];
    result[0] = 10 * voxel_sgn[0];
    sq_sum += result[0] * result[0];
    while (counting.size() < 8)
    {
      for (unsigned i = counting.size() - 1; i < counting.size(); ++i)
      {
        for (unsigned j = 0; j < 3; ++j)
        {
          if (!counted[counting[i] ^ (1 << j)])
          {
            if (voxel_sgn[counting[i]] * voxel_sgn[counting[i] ^ (1 << j)] <= 0)
            {
              connections[counting[i]].push_back(counting[i] ^ (1 << j));
              connections[counting[i] ^ (1 << j)].push_back(counting[i]);
              counted[counting[i] ^ (1 << j)] = true;
              counting.push_back(counting[i] ^ (1 << j));
              count_pair.push_back(counting[i]);
              //printf("%u - %u: ", counting[i], counting[i] ^ (1 << j));
              /*count_edges_ratio(result[counting[i]], counting[i], 
                                result[counting[i] ^ (1 << j)], counting[i] ^ (1 << j), 
                                voxel_sgn, voxels, vox_quantity);*/
              usefull_vox_nums.insert(voxel_used_on_edge(counting[i], counting[i] ^ (1 << j), 
                                                         voxel_sgn, voxels, vox_quantity));
              //printf("%f\n", result[counting[i] ^ (1 << j)]);
              //sq_sum += result[counting[i] ^ (1 << j)] * result[counting[i] ^ (1 << j)];
            }
          }
        }
      }
      if (counting.size() < 8)
      {
        for (unsigned i = 0; i < 8; ++i)
        {
          if (!counted[i])
          {
            for (unsigned j = 0; j < 3; ++j)
            {
              if ((i & (1 << j)) != 0)
              {
                connections[i].push_back(i ^ (1 << j));
                connections[i ^ (1 << j)].push_back(i);
                counted[i] = true;
                counting.push_back(i);
                count_pair.push_back(i ^ (1 << j));
                //printf("%u - %u; ", i, i ^ (1 << j));
                //count_edges_ratio(result[i ^ (1 << j)], i ^ (1 << j), result[i], i, voxel_sgn, voxels, vox_quantity);
                //printf("%f\n", result[i]);
                //sq_sum += result[i] * result[i];
                break;
              }
            }
            break;
          }
        }
      }
    }
    for (unsigned i = 1; i < 8; ++i)
    {
      count_edges_ratio(result[count_pair[i]], count_pair[i], 
                        result[counting[i]], counting[i], 
                        voxel_sgn, voxels, vox_quantity, 
                        usefull_vox_nums);
      //printf("%f ", result[counting[i]]);
      sq_sum += result[counting[i]] * result[counting[i]];
    }
    //printf("\n");
    for (unsigned i = 0; i < 8; ++i)
    {
      result[i] /= std::sqrt(sq_sum);
      result[i] *= res_sq_sum;
    }
    return true;
  }

  struct MergeOctreePointer
  {
    MergeOctreePointer() : octree_idx(INVALID_IDX), node_idx(INVALID_IDX), region_code(0, 0, 0, 1) {}
    MergeOctreePointer(uint32_t _octree_idx, uint32_t _node_idx, uint4 _region_code) :
      octree_idx(_octree_idx), node_idx(_node_idx), region_code(_region_code) {}
    uint32_t octree_idx; // index of the octree in the list
    uint32_t node_idx;   // index of the node in this octree
    uint4 region_code;   // code for the region of interest inside node (0,0,0,1 is the whole node)
  };

  void merge_global_octrees_rec(GlobalOctree &out, const std::vector<GlobalOctree> &octrees, const std::vector<uint4> &codes,
                                const std::vector<MergeOctreePointer> &octree_pointers, 
                                uint4 code, uint32_t node_idx, bool is_vox_merge)
  {
    bool is_outside = true, is_surfaced = false;
    for (int oct_idx = 0; oct_idx < octree_pointers.size() && is_outside; ++oct_idx)
    {
      if (octree_pointers[oct_idx].node_idx == INVALID_IDX)
        continue;
      const GlobalOctreeNode &node = octrees[octree_pointers[oct_idx].octree_idx].nodes[octree_pointers[oct_idx].node_idx];
      is_outside  &= node.is_outside;
      is_surfaced |= node.is_surfaced;
    }

    out.nodes[node_idx].type = GlobalOctreeNodeType::EMPTY;
    out.nodes[node_idx].is_outside = is_outside;
    out.nodes[node_idx].is_surfaced = is_surfaced;
    out.nodes[node_idx].bricks_count = 0;
    out.nodes[node_idx].offset = 0;

    //fill debug info for this node (if it is not empty, more information will be added later)
    if (out.debug_info)
    {
      #pragma omp critical (debug_write)
      {
        auto &node_info = get_node_info(out.debug_info, node_idx);
        node_info.creation_type = GlobalOctreeDebugInfo::CreationType::EMPTY;
        node_info.position_code = code;
        node_info.is_surface_node = is_surfaced;
        node_info.merged_nodes_offset = out.debug_info->merged_nodes_info.size();
        node_info.primitives_count = 0;
        node_info.connected_components_count = 0;
        node_info.error_from_parent = -1.0f;
        node_info.add_depth_error = -1.0f;

        unsigned merged_nodes_count = 0;
        for (int oct_idx = 0; oct_idx < octree_pointers.size(); ++oct_idx)
        {
          if (octree_pointers[oct_idx].node_idx == INVALID_IDX)
          continue;
        const GlobalOctreeNode &node = octrees[octree_pointers[oct_idx].octree_idx].nodes[octree_pointers[oct_idx].node_idx];
          if (octrees[octree_pointers[oct_idx].octree_idx].debug_info)
            out.debug_info->merged_nodes_info.push_back(octrees[octree_pointers[oct_idx].octree_idx].debug_info->nodes_info[octree_pointers[oct_idx].node_idx]);
          else
            out.debug_info->merged_nodes_info.push_back(GlobalOctreeDebugInfo::NodeInfo());
          node_info.primitives_count += out.debug_info->merged_nodes_info.back().primitives_count;
          node_info.connected_components_count += out.debug_info->merged_nodes_info.back().connected_components_count;
          node_info.error_from_parent = std::max(node_info.error_from_parent, out.debug_info->merged_nodes_info.back().error_from_parent);
          node_info.add_depth_error   = std::max(node_info.add_depth_error,   out.debug_info->merged_nodes_info.back().add_depth_error);
          merged_nodes_count++;
        }
        node_info.merged_nodes_count = merged_nodes_count;
      }
    }

    //this node is inside one of the parts. It will not be visible and can be skipped along with all possible children
    if (!is_outside)
      return;
    
    bool saved_mat_and_tc = false;

    std::map<int, int> mat_cnt;
    for (int oct_idx = 0; oct_idx < octree_pointers.size(); ++oct_idx)
    {
      //node_indices[i] == INVALID_IDX if octree is inside this node, but smaller that the node
      //it will be processed deeper in the recursion
      if (octree_pointers[oct_idx].node_idx == INVALID_IDX)
        continue;
      const GlobalOctree &octree = octrees[octree_pointers[oct_idx].octree_idx];
      const GlobalOctreeNode &node = octree.nodes[octree_pointers[oct_idx].node_idx];

      if (node.type == GlobalOctreeNodeType::EMPTY || node.type == GlobalOctreeNodeType::EMPTY_NODE) //no surface
        continue;
      
      //take material and texture coordinates from first surface
      //counting only nodes with (0,0,0,1) code, i.e. node with the same level as the target one
      //TODO: find better way to do it
      if (true)//equal(octree_pointers[oct_idx].region_code, uint4(0, 0, 0, 1)))
      {
        if (octree.header.mat_channel_id >= 0)
        {
          const auto &mat_data = octree.voxel_channels[octree.header.mat_channel_id];
          int material = mat_data.data_i[mat_data.num_components * octree_pointers[oct_idx].node_idx];
          if (mat_cnt.find(material) != mat_cnt.end())
          {
            mat_cnt[material] += 1;
          }
          else
          {
            mat_cnt[material] = 1;
          }
        }
      }
    }
    int res_mat = 0, cnt = 0;
    for (auto mat : mat_cnt)
    {
      if (mat.second > cnt)
      {
        cnt = mat.second;
        res_mat = mat.first;
      }
    }

    //check all active octrees and find ones that have a surface, add all surfaces to the main node
    for (int oct_idx = 0; oct_idx < octree_pointers.size(); ++oct_idx)
    {
      //node_indices[i] == INVALID_IDX if octree is inside this node, but smaller that the node
      //it will be processed deeper in the recursion
      if (octree_pointers[oct_idx].node_idx == INVALID_IDX)
        continue;
      const GlobalOctree &octree = octrees[octree_pointers[oct_idx].octree_idx];
      const GlobalOctreeNode &node = octree.nodes[octree_pointers[oct_idx].node_idx];

      if (node.type == GlobalOctreeNodeType::EMPTY || node.type == GlobalOctreeNodeType::EMPTY_NODE) //no surface
        continue;
      
      //take material and texture coordinates from first surface
      //counting only nodes with (0,0,0,1) code, i.e. node with the same level as the target one
      //TODO: find better way to do it
      if (!saved_mat_and_tc)// && equal(octree_pointers[oct_idx].region_code, uint4(0, 0, 0, 1)))
      {
        saved_mat_and_tc = true;
        if (octree.header.tc_channel_id >= 0)
        {
          const auto &tc_data = octree.point_channels[octree.header.tc_channel_id];
          auto &tc_res = out.point_channels[out.header.tc_channel_id];
          for (int i = 0; i < 8; ++i)
          {
            tc_res.data_f[tc_res.num_components * node_idx * 8 + i * 2 + 0] = tc_data.data_f[tc_data.num_components * octree_pointers[oct_idx].node_idx + i * 2 + 0];
            tc_res.data_f[tc_res.num_components * node_idx * 8 + i * 2 + 1] = tc_data.data_f[tc_data.num_components * octree_pointers[oct_idx].node_idx + i * 2 + 1];
          }
        }
        if (octree.header.mat_channel_id >= 0)
        {
          //const auto &mat_data = octree.voxel_channels[octree.header.mat_channel_id];
          auto &mat_res = out.voxel_channels[out.header.mat_channel_id];
          mat_res.data_i[mat_res.num_components * node_idx] = res_mat;//mat_data.data_i[mat_res.num_components * octree_pointers[oct_idx].node_idx];
        }
      }

      if (out.nodes[node_idx].bricks_count == 0)
        out.nodes[node_idx].val_off = out.values_f.size();

      out.nodes[node_idx].type = out.nodes[node_idx].type || node.type;

      //add values -> add surface(s) to main node
      int v_size = out.header.brick_size + 2*out.header.brick_pad + 1;
      const float *node_values = octree.values_f.data() + node.val_off;

      if (equal(octree_pointers[oct_idx].region_code, uint4(0, 0, 0, 1)))
      {
        //this node is the same size as the target one, just copy values
        out.nodes[node_idx].bricks_count += node.bricks_count;
        int node_values_cnt = node.bricks_count * v_size*v_size*v_size;
        out.values_f.insert(out.values_f.end(), node_values, node_values + node_values_cnt);
      }
      else if (v_size == 2)
      {
        //interpolate values to the target node
        //we assume that this can happen only for 2x2x2 bricks, otherwise we need to change the interpolation
        float tmp_values[8];
        assert(v_size == 2);

        for (int s = 0; s < node.bricks_count; s++)
        {
          float min_val = 1000.0f;
          float max_val = -1000.0f;
          for (int i = 0; i < 8; i++)
          {
            uint4 rc = octree_pointers[oct_idx].region_code;
            float3 pos_code = (float3(rc.x, rc.y, rc.z) + float3(i >> 2, (i >> 1) & 1, i & 1))/float(rc.w);
            float val = trilinear_interp(node_values + s*v_size*v_size*v_size, pos_code);
            min_val = std::min(min_val, val);
            max_val = std::max(max_val, val);
            tmp_values[i] = val;
          }

          if (max_val*min_val <= 0)
          {
            out.values_f.insert(out.values_f.end(), tmp_values, tmp_values + 8);
            out.nodes[node_idx].bricks_count++;
          }
        }
      }
      else
      {
        float *tmp_values = new float[v_size * v_size * v_size];

        for (int s = 0; s < node.bricks_count; s++)
        {
          float min_val = 1000.0f;
          float max_val = -1000.0f;
          /*for (int i = -out.header.brick_pad; i <= out.header.brick_size + out.header.brick_pad; ++i)
          {
            for (int j = -out.header.brick_pad; j <= out.header.brick_size + out.header.brick_pad; ++j)
            {
              for (int k = -out.header.brick_pad; k <= out.header.brick_size + out.header.brick_pad; ++k)
              {
                int3 idxs = int3(i, j, k) + out.header.brick_pad;
                uint4 rc = octree_pointers[oct_idx].region_code;
                float3 pos_code = (float3(rc.x, rc.y, rc.z) + float3(i, j, k) / out.header.brick_size)/float(rc.w);
                float tmp_vox[8];
                int min_vox_point_x = pos_code.x * out.header.brick_size + out.header.brick_pad;
                int min_vox_point_y = pos_code.y * out.header.brick_size + out.header.brick_pad;
                int min_vox_point_z = pos_code.z * out.header.brick_size + out.header.brick_pad;//count point idx inside brick
                if (min_vox_point_x == v_size) --min_vox_point_x;
                if (min_vox_point_y == v_size) --min_vox_point_y;
                if (min_vox_point_z == v_size) --min_vox_point_z;//avoid idx overflow
                int3 min_vox_p = int3(min_vox_point_x, min_vox_point_y, min_vox_point_z);
                float3 real_pos_code = pos_code * out.header.brick_size + out.header.brick_pad - float3(min_vox_p);
                for (int t = 0; t < 8; ++t)
                {
                  int x = min_vox_p.x + ((t >> 2) & 1);
                  int y = min_vox_p.y + ((t >> 1) & 1);
                  int z = min_vox_p.z + (t & 1);
                  tmp_vox[t] = *(node_values + s*v_size*v_size*v_size + x * v_size * v_size + y * v_size + z);
                }
                float val = trilinear_interp(tmp_vox, real_pos_code);
                min_val = std::min(min_val, val);
                max_val = std::max(max_val, val);
                tmp_values[idxs.x * v_size * v_size + idxs.y * v_size + idxs.z] = val;
              }
            }
          }*/

          take_brick_part_by_pos_code(out.header, octree_pointers[oct_idx].region_code, node_values + s*v_size*v_size*v_size, 
                                      tmp_values, min_val, max_val);


          if (max_val*min_val <= 0)
          {
            out.values_f.insert(out.values_f.end(), tmp_values, tmp_values + v_size*v_size*v_size);
            out.nodes[node_idx].bricks_count++;
          }
        }
        delete [] tmp_values;
      }
    }

    //there is probably an empty node here, so we can set 0s to both material and texture coordinates
    if (!(out.nodes[node_idx].bricks_count == 0 || saved_mat_and_tc))
    {
      if (out.header.tc_channel_id >= 0)
      {
        auto &tc_res = out.point_channels[out.header.tc_channel_id];
        for (int i = 0; i < 8; ++i)
        {
          tc_res.data_f[tc_res.num_components * node_idx * 8 + i * 2 + 0] = 0;
          tc_res.data_f[tc_res.num_components * node_idx * 8 + i * 2 + 1] = 0;
        }
      }
      if (out.header.mat_channel_id >= 0)
      {
        auto &mat_res = out.voxel_channels[out.header.mat_channel_id];
        mat_res.data_i[mat_res.num_components * node_idx] = 0;
      }
    }

    // merge voxels if needed
    bool voxel_merge_done = false;
    if (out.header.brick_pad == 0 && out.header.brick_size == 1 && 
        out.nodes[node_idx].bricks_count > 1 && !is_surfaced && is_vox_merge)
    {
      if (is_surfaced)
      {
        unsigned bricks_cnt = remove_surfaces(out.values_f.data() + out.nodes[node_idx].val_off, out.nodes[node_idx].bricks_count);
        out.nodes[node_idx].bricks_count = bricks_cnt;
      }
      else
      {
        voxel_merge_done = true;
        float voxel[8];
        if (merge_voxels(voxel, out.values_f.data() + out.nodes[node_idx].val_off, out.nodes[node_idx].bricks_count))
        {
          out.values_f.resize(out.nodes[node_idx].val_off + 8);
          out.nodes[node_idx].bricks_count = 1;
          bool is_vox_fill = false;
          for (unsigned i = 0; i < 8; ++i)
          {
            is_vox_fill = is_vox_fill || (voxel[0] * voxel[i] <= 0);
            out.values_f[out.nodes[node_idx].val_off + i] = voxel[i];
          }
          if (!is_vox_fill)
          {
            out.values_f.resize(out.nodes[node_idx].val_off);
            out.nodes[node_idx].bricks_count = 0;
          }
        }
      }
    }

    if (out.nodes[node_idx].bricks_count > 0 && out.nodes[node_idx].val_off == 0)
    {
      printf("WARNING: something went wrong with voxel merging\n");
      out.nodes[node_idx].bricks_count = 0;
    }

    if (out.debug_info)
    {
      #pragma omp critical (debug_write)
      {
        auto &node_info = get_node_info(out.debug_info, node_idx);
        node_info.creation_type = voxel_merge_done ?
                                  GlobalOctreeDebugInfo::CreationType::MERGE_WITH_VOXEL_MERGE :
                                  GlobalOctreeDebugInfo::CreationType::MERGE;        
        node_info.surfaces_count = out.nodes[node_idx].bricks_count;
      }
    }

    //make list of active octrees for every child of main node and recursion if it is not empty
    for (int ch_idx = 0; ch_idx < 8; ch_idx++)
    {
      uint4 child_code = 2*code + uint4((ch_idx & 4) >> 2, (ch_idx & 2) >> 1, ch_idx & 1, 0);
      std::vector<MergeOctreePointer> child_octree_pointers;
      child_octree_pointers.reserve(octree_pointers.size());
      int next_level_children_count = 0;

      for (int iidx = 0; iidx < octree_pointers.size(); iidx++)
      {
        int oidx = octree_pointers[iidx].octree_idx;
        //this octree was too small for current node, check if child contains it 
        if (octree_pointers[iidx].node_idx == INVALID_IDX)
        {
          //this octree root occupies the same space as the child and its surface should be added to the child
          if (equal(child_code, codes[oidx]))
          {
            child_octree_pointers.push_back(MergeOctreePointer(oidx, 0, uint4(0, 0, 0, 1)));
            next_level_children_count++;
          }
          else if (contains(child_code, codes[oidx])) //child contains this octree
          {
            child_octree_pointers.push_back(MergeOctreePointer(oidx, INVALID_IDX, uint4(0, 0, 0, 1)));
            next_level_children_count++;
          }
        }
        else
        {
          //go deeper into this octree if we can
          const GlobalOctreeNode &node = octrees[oidx].nodes[octree_pointers[iidx].node_idx];
          if (node.type == GlobalOctreeNodeType::NODE || node.type == GlobalOctreeNodeType::EMPTY_NODE) //use it's child on next level
          {
            child_octree_pointers.push_back(MergeOctreePointer(oidx, node.offset + ch_idx, uint4(0, 0, 0, 1))); 
            next_level_children_count++;          
          }
          else if (node.type == GlobalOctreeNodeType::LEAF) //use the same node, but narrow the region
          {
            uint4 new_code = 2*octree_pointers[iidx].region_code + uint4(ch_idx >> 2, (ch_idx & 2) >> 1, ch_idx & 1, 0);
            child_octree_pointers.push_back(MergeOctreePointer(oidx, octree_pointers[iidx].node_idx, new_code));
          }
        }
      }

      //nothing to merge to this child
      if (next_level_children_count == 0)
        continue;
      
      //first child, add space for children to main node
      if (out.nodes[node_idx].offset == 0)
      {
        out.nodes[node_idx].offset = out.nodes.size();
        out.nodes.insert(out.nodes.end(), 8, GlobalOctreeNode());
        {
          resize_data_channels(out, 8);
        }
      }

      //recursion
      merge_global_octrees_rec(out, octrees, codes, child_octree_pointers, child_code, out.nodes[node_idx].offset + ch_idx, is_vox_merge);
    }

    //recalculate main node type
    bool active_as_leaf = out.nodes[node_idx].type != GlobalOctreeNodeType::EMPTY && out.nodes[node_idx].type != GlobalOctreeNodeType::EMPTY_NODE;
    bool has_active_children = false;
    for (int ch_idx = 0; ch_idx < 8; ch_idx++)
    {
      if (out.nodes[out.nodes[node_idx].offset + ch_idx].type != GlobalOctreeNodeType::EMPTY)
        has_active_children = true;
    }
    if (has_active_children)
    {
      if (active_as_leaf)
        out.nodes[node_idx].type = GlobalOctreeNodeType::NODE;
      else
        out.nodes[node_idx].type = GlobalOctreeNodeType::EMPTY_NODE;
    }
    else 
    {
      if (active_as_leaf)
        out.nodes[node_idx].type = GlobalOctreeNodeType::LEAF;
      else
        out.nodes[node_idx].type = GlobalOctreeNodeType::EMPTY;
    }
  }

  void fill_neighbours_along_axis(const GlobalOctree &octree, unsigned upper_node_idx, unsigned upper_neigh_data[6],
                                  unsigned child_num, unsigned axis_slice, unsigned neigh_offsets[6]) // last will change
  {
    unsigned close_neigh = 3 * ((child_num & axis_slice) / axis_slice) + (3 & axis_slice) + (1 & axis_slice); // magic formula to find neigh_offsets idx
    unsigned anoth_neigh = 3 * (1 - (child_num & axis_slice) / axis_slice) + (3 & axis_slice) + (1 & axis_slice);
    neigh_offsets[close_neigh] = octree.nodes[upper_node_idx].offset + (child_num ^ axis_slice);
    if (octree.nodes[upper_neigh_data[anoth_neigh]].offset == 0)
      neigh_offsets[anoth_neigh] = upper_neigh_data[anoth_neigh];
    else
      neigh_offsets[anoth_neigh] = octree.nodes[upper_neigh_data[anoth_neigh]].offset + (child_num ^ axis_slice);
  }

  void fill_outside_set_with_childs_on_axis_rec(GlobalOctree &octree, std::set<unsigned> &buffer,
                                                unsigned node_idx, unsigned neigh_num, const std::vector<unsigned> &o2n)
  {
    unsigned shift_c = 2 - (neigh_num % 3), shift_i, shift_j, c = neigh_num / 3;
    shift_i = (shift_c + 1) % 3;
    shift_j = (shift_c + 2) % 3;
    for (unsigned i = 0; i <= 1; ++i)
    {
      for (unsigned j = 0; j <= 1; ++j)
      {
        unsigned child = (i << shift_i) + (j << shift_j) + (c << shift_c);
        if (octree.nodes[octree.nodes[node_idx].offset + child].offset == 0)
        {
          if (!octree.nodes[octree.nodes[node_idx].offset + child].is_outside)
          {
            octree.nodes[octree.nodes[node_idx].offset + child].is_outside = true;
            buffer.insert(o2n[octree.nodes[node_idx].offset + child]);
          }
        }
        else
        {
          fill_outside_set_with_childs_on_axis_rec(octree, buffer, octree.nodes[node_idx].offset + child,
                                                   neigh_num, o2n);
        }
      }
    }
  }

  bool fill_all_outside_flag(GlobalOctree &octree, unsigned node)
  {
    if (octree.nodes[node].offset != 0)
    {
      bool counter = false;
      for (int i = 0; i < 8; ++i)
      {
        counter |= fill_all_outside_flag(octree, octree.nodes[node].offset + i);
      }
      octree.nodes[node].is_outside = counter;
    }
    return octree.nodes[node].is_outside;
  }

  void fill_global_octree_out_flag(GlobalOctree &octree)
  {
    class neighbours
    {
    public:
      unsigned neigh_offsets[6]; // x+ y+ z+ x- y- z-
      unsigned node_offset;
      neighbours()
      {
        for (int i = 0; i < 6; ++i)
          neigh_offsets[i] = 0;
        node_offset = 0;
      }
      bool is_on_border()
      {
        return neigh_offsets[0] == 0 || neigh_offsets[1] == 0 || neigh_offsets[2] == 0 ||
               neigh_offsets[3] == 0 || neigh_offsets[4] == 0 || neigh_offsets[5] == 0;
      }
    };
    std::set<unsigned> tmp;
    std::set<unsigned> buffer;
    std::vector<neighbours> neigh(1);
    std::vector<unsigned> octree_to_neigh(octree.nodes.size());
    octree_to_neigh[0] = 0;
    octree.nodes[0].is_outside = true;
    for (int i = 0; i < neigh.size(); ++i)
    {
      octree_to_neigh[neigh[i].node_offset] = i;
      if (octree.nodes[neigh[i].node_offset].offset != 0)
      {
        unsigned childs_offset = neigh.size();
        neigh.resize(neigh.size() + 8);
        for (unsigned j = 0; j < 8; ++j)
        {
          neigh[childs_offset + j].node_offset = octree.nodes[neigh[i].node_offset].offset + j;
          for (unsigned axis = 4; axis > 0; axis >>= 1)
          {
            fill_neighbours_along_axis(octree, neigh[i].node_offset, neigh[i].neigh_offsets,
                                       j, axis, neigh[childs_offset + j].neigh_offsets);
          }
          octree.nodes[neigh[childs_offset + j].node_offset].is_outside = neigh[childs_offset + j].is_on_border();
          if (neigh[childs_offset + j].is_on_border() &&
              octree.nodes[neigh[childs_offset + j].node_offset].offset == 0 &&
              octree.nodes[neigh[childs_offset + j].node_offset].type == GlobalOctreeNodeType::EMPTY)
          {
            buffer.insert(i);
          }
          octree.nodes[neigh[childs_offset + j].node_offset].is_outside |=
              (octree.nodes[neigh[childs_offset + j].node_offset].type == GlobalOctreeNodeType::LEAF ||
               octree.nodes[neigh[childs_offset + j].node_offset].type == GlobalOctreeNodeType::NODE);
        }
      }
    }
    // TODO fill octree nodes flag
    while (buffer.size() > 0)
    {
      for (auto i : buffer)
      {
        for (unsigned j = 0; j < 6; ++j)
        {
          if (octree.nodes[neigh[i].neigh_offsets[j]].offset != 0)
          {
            fill_outside_set_with_childs_on_axis_rec(octree, tmp, neigh[i].neigh_offsets[j], j, octree_to_neigh);
          }
          else if (!octree.nodes[neigh[i].neigh_offsets[j]].is_outside)
          {
            octree.nodes[neigh[i].neigh_offsets[j]].is_outside = true;
            tmp.insert(octree_to_neigh[neigh[i].neigh_offsets[j]]);
          }
        }
      }
      buffer.clear();
      buffer.insert(tmp.begin(), tmp.end());
      tmp.clear();
    }
    fill_all_outside_flag(octree, 0);
  }

  void merge_global_octrees(GlobalOctree &out, std::vector<GlobalOctree> &octrees, const std::vector<uint4> &codes, bool is_vox_merge)
  {
    assert(octrees.size() == codes.size());

    unsigned max_nodes_num = 0;
    for (int i = 0; i < octrees.size(); ++i)
    {
      normalize_bricks(octrees[i], codes[i]);
      if (octrees[i].nodes.size() > max_nodes_num) max_nodes_num = octrees[i].nodes.size();
      //printf("sizes: %u %u\n", octree.header.brick_size, octrees[0].header.brick_size);
      assert(octrees[i].header.brick_size == octrees[0].header.brick_size);
      assert(octrees[i].header.brick_pad == octrees[0].header.brick_pad);
    }

    //fill input octrees to determine outside flags for nodes
    //TODO: merge it with what is done in sign analysis 
    for (auto &octree : octrees)
      fill_global_octree_out_flag(octree);

    std::vector<MergeOctreePointer> octree_pointers;
    octree_pointers.reserve(octrees.size());

    for (int i = 0; i < octrees.size(); ++i)
    {
      if (equal(uint4(0, 0, 0, 1), codes[i]))
        octree_pointers.push_back(MergeOctreePointer(i, 0, uint4(0, 0, 0, 1)));
      else
        octree_pointers.push_back(MergeOctreePointer(i, INVALID_IDX, uint4(0, 0, 0, 1)));
    }

    //fill debug info if required
    if (out.debug_info)
    {
      unsigned cur_offset = 0;
      for (int i = 0; i < octrees.size(); ++i)
      {
        if (!octrees[i].debug_info)
          printf("WARNING: octree %u has no debug info, merged octree won't have full debug info either\n", i);
        else
        {
          for (auto &node_info : octrees[i].debug_info->nodes_info)
            node_info.part_id = octree_pointers[i].octree_idx;
        }
        cur_offset += octrees[i].nodes.size();
      }
      out.debug_info->merged_nodes_info.reserve(cur_offset);
    }

    out.header = octrees[0].header;
    out.nodes.reserve(max_nodes_num);
    out.nodes.resize(1);
    //put an empty brick at the start of the data vector to make sure no valid brick has a 0 data offset
    //(data offset 0 is often treated as invalid)
    out.values_f.resize(int_pow(out.header.brick_size, out.header.dim), 0.0f); 
    copy_data_channels(octrees[0], out);
    reserve_data_channels(out, max_nodes_num);
    resize_data_channels(out, 1);
    merge_global_octrees_rec(out, octrees, codes, octree_pointers, uint4(0, 0, 0, 1), 0, is_vox_merge);
  }
}