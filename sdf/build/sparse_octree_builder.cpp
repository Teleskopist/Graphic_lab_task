#include "sparse_octree_builder_core.h"

namespace sdf_converter
{
  using cmesh4::SimpleMesh;

  void mesh_octree_to_global_octree(SparseOctreeSettings settings, const cmesh4::SimpleMesh &mesh, 
                                    const PrimitiveListOctree &pl_octree, GlobalOctree &out_octree,
                                    GlobalOctreeBuildStat *build_stat, bool multithreaded)
  {
    if (multithreaded)
      prim_octree_to_global_octree<cmesh4::SimpleMesh, MeshDistFunc, true>(settings, mesh, pl_octree, out_octree, build_stat);
    else
      prim_octree_to_global_octree<cmesh4::SimpleMesh, MeshDistFunc, false>(settings, mesh, pl_octree, out_octree, build_stat);
  }

  void augmented_mesh_to_global_octree(SparseOctreeSettings settings, const cmesh4::AugmentedMesh &model, 
                                       const PrimitiveListOctree &pl_octree, GlobalOctree &out_octree,
                                       GlobalOctreeBuildStat *build_stat, bool multithreaded)
  {
    if (multithreaded)
      prim_octree_to_global_octree<cmesh4::AugmentedMesh, AugmentedMeshDistFunc, true>(settings, model, pl_octree, out_octree, build_stat);
    else
      prim_octree_to_global_octree<cmesh4::AugmentedMesh, AugmentedMeshDistFunc, false>(settings, model, pl_octree, out_octree, build_stat);
  }

  void unstructured_grid_to_global_octree(SparseOctreeSettings settings, const vtk::UnstructuredGrid &model,
                                          const PrimitiveListOctree &pl_octree, GlobalOctree &out_octree,
                                          GlobalOctreeBuildStat *build_stat, bool multithreaded)
  {
    if (multithreaded)
      prim_octree_to_global_octree<vtk::UnstructuredGrid, UGCellDistFunc, true>(settings, model, pl_octree, out_octree, build_stat);
    else
      prim_octree_to_global_octree<vtk::UnstructuredGrid, UGCellDistFunc, false>(settings, model, pl_octree, out_octree, build_stat);
  }

  void add_global_node_rec(std::vector<GlobalOctreeNode> &nodes, std::vector<float> &values_f, SparseOctreeSettings settings, MultithreadedDistanceFunction sdf,
                           unsigned thread_id, unsigned node_idx, unsigned depth, unsigned max_depth, float3 p, float d, int brick_size, int brick_pad)
  {
    int val_size = brick_size + brick_pad * 2 + 1;
    float value_center = sdf(2.0f * ((p + float3(0.5, 0.5, 0.5)) * d) - float3(1, 1, 1), thread_id);
    float min_val = 1000;
    float max_val = -1000;
    nodes[node_idx].val_off = values_f.size();
    nodes[node_idx].offset = 0;
    nodes[node_idx].bricks_count = 1;

    /*if (out_octree.tc_channel_id >= 0)
    {
      auto &tex = out_octree.point_channels[out_octree.tc_channel_id];
      for (int i = 0; i < 8; ++i)
      {
        //nodes[node_idx].tex_coords[i] = float2(0, 0);
        tex.data_f[tex.num_components * node_idx + i * 2 + 0] = 0;
        tex.data_f[tex.num_components * node_idx + i * 2 + 1] = 0;
      }
    }*/
    for (int i = -brick_pad; i <= brick_size + brick_pad; ++i)
    {
      for (int j = -brick_pad; j <= brick_size + brick_pad; ++j)
      {
        for (int k = -brick_pad; k <= brick_size + brick_pad; ++k)
        {
          float val = sdf(2.0f * ((p + float3((float)i / (float)brick_size, (float)j / (float)brick_size, (float)k / (float)brick_size)) * d) - float3(1, 1, 1), thread_id);
          values_f.push_back(val);
          min_val = std::min(min_val, val);
          max_val = std::max(max_val, val);
        }
      }
    }
    /*for (int cid = 0; cid < 8; cid++)
    {
      nodes[node_idx].values[cid] = sdf(2.0f * ((p + float3((float)i / (float)brick_size, (float)j / (float)brick_size, (float)k / (float)brick_size)) * d) - float3(1, 1, 1), thread_id);
      min_val = std::min(min_val, nodes[node_idx].values[cid]);
      max_val = std::max(max_val, nodes[node_idx].values[cid]);
    }*/
    bool is_parent = false;
    if (depth < max_depth && (std::abs(value_center) < sqrtf(3) * d || min_val*max_val <= 0))
    {
      is_parent = true;
      nodes[node_idx].offset = nodes.size();
      
      nodes.resize(nodes.size() + 8);
      unsigned idx = nodes[node_idx].offset;
      for (unsigned cid = 0; cid < 8; cid++)
      {
        add_global_node_rec(nodes, values_f, settings, sdf, thread_id, 
                     idx + cid, depth + 1, max_depth, 2 * p + float3((cid & 4) >> 2, (cid & 2) >> 1, cid & 1), d / 2, brick_size, brick_pad);
      }
    }

    // determine node type
    bool active_as_leaf = min_val <= 0 && (max_val >= 0 || settings.fill_internal_volume);
    if (active_as_leaf)
    {
      nodes[node_idx].type = is_parent ? GlobalOctreeNodeType::NODE : GlobalOctreeNodeType::LEAF;
      nodes[node_idx].is_internal = (max_val < 0);
    }
    else
    {
      nodes[node_idx].bricks_count = 0;
      nodes[node_idx].type = is_parent ? GlobalOctreeNodeType::EMPTY_NODE : GlobalOctreeNodeType::EMPTY;
    }
  }

  void sdf_to_global_octree(SparseOctreeSettings settings, MultithreadedDistanceFunction sdf, 
                                      unsigned max_threads, GlobalOctree &octree)
  {
    octree.header.brick_size = settings.brick_size;
    octree.header.brick_pad = settings.brick_pad;

    struct LargeNode
    {
      float3 p;
      float d;
      unsigned level;
      unsigned group_idx;
      unsigned thread_id;
      unsigned children_idx;
      float value;
    };
    
    std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

    omp_set_num_threads(max_threads);

    unsigned min_remove_level = std::min(settings.depth, 4u);
    std::vector<LargeNode> large_nodes;
    int lg_size = pow(2, min_remove_level);
    large_nodes.push_back({float3(0,0,0), 1.0f, 0u, 0u, 0u, 0u, 1000.0f});

    {
      unsigned i = 0;
      while (i < large_nodes.size())
      {
        if (large_nodes[i].level < min_remove_level)
        {
          large_nodes[i].children_idx = large_nodes.size();
          for (int j=0;j<8;j++)
          {
            float ch_d = large_nodes[i].d / 2;
            float3 ch_p = 2 * large_nodes[i].p + float3((j & 4) >> 2, (j & 2) >> 1, j & 1);
            large_nodes.push_back({ch_p, ch_d, large_nodes[i].level+1, 0u, 0u, 0u, 1000.0f});
          }
        }
        i++;
      }
    }

    std::vector<unsigned> border_large_nodes;

    for (int i=0;i<large_nodes.size();i++)
    {
      float3 pos = 2.0f * ((large_nodes[i].p + float3(0.5, 0.5, 0.5)) * large_nodes[i].d) - float3(1, 1, 1); 
      large_nodes[i].value = sdf(pos, 0);
      if (large_nodes[i].children_idx == 0 && is_border(large_nodes[i].value, large_nodes[i].level))
        border_large_nodes.push_back(i);
    }

    unsigned step = (border_large_nodes.size() + max_threads - 1) / max_threads;
    std::vector<std::vector<GlobalOctreeNode>> all_nodes(max_threads);
    std::vector<std::vector<float>> all_distances(max_threads);
    std::vector<std::vector<uint2>> all_groups(max_threads);

    for (int i=0;i<max_threads;i++)
    {
      all_nodes[i].reserve(std::min(1ul << 20, 1ul << 3*settings.depth));
    }

    std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();

    #pragma omp parallel for
    for (int thread_id=0;thread_id<max_threads;thread_id++)
    {
      unsigned start = thread_id * step;
      unsigned end = std::min(start + step, (unsigned)border_large_nodes.size());
      for (unsigned i=start;i<end;i++)
      {
        unsigned idx = border_large_nodes[i];
        unsigned local_root_idx = all_nodes[thread_id].size();
        large_nodes[idx].group_idx = all_groups[thread_id].size();
        large_nodes[idx].thread_id = thread_id;
        all_nodes[thread_id].emplace_back();
        add_global_node_rec(all_nodes[thread_id], all_distances[thread_id], settings, sdf, thread_id, 
                            local_root_idx, large_nodes[idx].level, settings.depth, large_nodes[idx].p, large_nodes[idx].d, 
                            octree.header.brick_size, octree.header.brick_pad);
      
        all_groups[thread_id].push_back(uint2(local_root_idx, all_nodes[thread_id].size()));
      }
    }

    std::chrono::steady_clock::time_point t3 = std::chrono::steady_clock::now();

    std::vector<GlobalOctreeNode> res_nodes(large_nodes.size());
    std::vector<float> values_f;

    int v_size = octree.header.brick_size + 2 * octree.header.brick_pad + 1;

    for (int i=0;i<large_nodes.size();i++)
    {
      res_nodes[i].val_off = values_f.size();
      res_nodes[i].type = GlobalOctreeNodeType::NODE; //some shady stuff, can be wrong
      res_nodes[i].is_outside = true;                 //some shady stuff, can be wrong

      for (int i0 = -(int)octree.header.brick_pad; i0 <= (int)octree.header.brick_size + (int)octree.header.brick_pad; ++i0)
      {
        for (int j0 = -(int)octree.header.brick_pad; j0 <= (int)octree.header.brick_size + (int)octree.header.brick_pad; ++j0)
        {
          for (int k0 = -(int)octree.header.brick_pad; k0 <= (int)octree.header.brick_size + (int)octree.header.brick_pad; ++k0)
          {
            float val = sdf(2.0f * ((large_nodes[i].p + float3((float)i0 / (float)octree.header.brick_size, (float)j0 / (float)octree.header.brick_size, (float)k0 / (float)octree.header.brick_size)) * large_nodes[i].d) - float3(1, 1, 1), 0);
            values_f.push_back(val);
          }
        }
      }
    
      res_nodes[i].offset = large_nodes[i].children_idx;
      if (large_nodes[i].children_idx == 0 && is_border(large_nodes[i].value, large_nodes[i].level))
      {
        unsigned start = all_groups[large_nodes[i].thread_id][large_nodes[i].group_idx].x + 1;
        unsigned end = all_groups[large_nodes[i].thread_id][large_nodes[i].group_idx].y;
        
        if (start == end) //this region is empty
          continue;
        
        unsigned prev_size = res_nodes.size();
        int shift = int(prev_size) - int(start);
        res_nodes[i].offset = all_nodes[large_nodes[i].thread_id][start-1].offset + shift;
        res_nodes.insert(res_nodes.end(), all_nodes[large_nodes[i].thread_id].begin() + start, all_nodes[large_nodes[i].thread_id].begin() + end);

        unsigned val_start = res_nodes[prev_size].val_off;
        int val_shift = values_f.size() - int(val_start);
        int cnt = end - start;
        for (int j=prev_size;j<res_nodes.size();j++)
        {
          if (res_nodes[j].offset != 0)
            res_nodes[j].offset += shift;
          res_nodes[j].val_off += val_shift;
        }

        values_f.insert(values_f.end(), all_distances[large_nodes[i].thread_id].begin() + val_start, all_distances[large_nodes[i].thread_id].begin() + val_start + cnt * v_size * v_size * v_size);
      }
    }

    std::chrono::steady_clock::time_point t4 = std::chrono::steady_clock::now();

    //printf("%u/%u nodes are active\n", nn, (unsigned)res_nodes.size());
    //printf("%u/%u nodes are left after elimination\n", (unsigned)frame_3.size(), (unsigned)res_nodes.size());
  
    std::chrono::steady_clock::time_point t5 = std::chrono::steady_clock::now();
  
    float time_1 = 1e-3f*std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    float time_2 = 1e-3f*std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();
    float time_3 = 1e-3f*std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();
    float time_4 = 1e-3f*std::chrono::duration_cast<std::chrono::microseconds>(t5 - t4).count();

    //printf("total nodes = %d, time = %6.2f ms (%.1f+%.1f+%.1f+%.1f)\n", 
    //       (int)res_nodes.size(), time_1 + time_2 + time_3 + time_4, time_1, time_2, time_3, time_4);

    omp_set_num_threads(omp_get_max_threads());

    octree.nodes = res_nodes;
    octree.values_f = values_f;
  }
}