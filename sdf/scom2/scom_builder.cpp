#include "scom_builder.h"
#include "clustering/clustering.h"
#include "scom_internal.h"
#include "similarity_compression.h"
#include "scom_utils.h"
#include "sdf/build/sparse_octree_builder.h"
#include "sdf/build/sparse_octree_common.h"
#include "utils/common/position_hash.h"
#include "utils/common/stat_box.h"
#include <chrono>

namespace sdf_converter
{
  void global_octree_to_DAG8_direct_rec(const sdf_converter::GlobalOctree &octree, SdfDAG &dag, uint32_t node_id)
  {
    uint32_t v_count = get_v_count(dag.header);

    const auto &node = octree.nodes[node_id];
    dag.nodes[node_id].channels_edge.rotation_id = scom2::ROT_ID_IDENTITY;
    dag.nodes[node_id].channels_edge.child_offset = node_id;

    if (node.bricks_count == 0)
    {
      dag.nodes[node_id].data_edges_offset = 0;
      dag.nodes[node_id].voxel_count_flags = DAG_pack_voxel_count_flags(0, false, !node.is_outside);
    }
    else
    {
      uint32_t d_o = dag.data_edges.size();
      uint32_t v_o = dag.distances.size();

      dag.nodes[node_id].data_edges_offset = d_o;
      dag.nodes[node_id].voxel_count_flags = DAG_pack_voxel_count_flags(node.bricks_count, node.is_surfaced, !node.is_outside);

      dag.data_edges.resize(d_o + node.bricks_count);
      for (int i = 0; i < node.bricks_count; ++i)
      {
        dag.data_edges[d_o + i].add = 0.0f;
        dag.data_edges[d_o + i].data_offset = v_o + i*v_count;
        dag.data_edges[d_o + i].rotation_id = 0;
        dag.data_edges[d_o + i].type_id = 0;
      }

      dag.distances.insert(dag.distances.end(), octree.values_f.begin() + node.val_off, 
                           octree.values_f.begin() + node.val_off + node.bricks_count*v_count);
    }

    if (node.offset == 0)
    {
      dag.nodes[node_id].children_edges_offset = 0;
    }
    else
    {
      uint32_t ch_count = get_children_count(dag.header);
      uint32_t c_o = dag.child_edges.size();
      dag.nodes[node_id].children_edges_offset = c_o;
      dag.child_edges.resize(c_o + ch_count);
      for (int i = 0; i < ch_count; ++i)
      {
        dag.child_edges[c_o + i].child_offset = node.offset + i;
        dag.child_edges[c_o + i].rotation_id = 0;
      }

      for (int i = 0; i < ch_count; ++i)
      {
        global_octree_to_DAG8_direct_rec(octree, dag, node.offset + i);
      }
    }
  }
  //no similarity compression, just convert one structure to another
  void global_octree_to_DAG8_direct(const sdf_converter::GlobalOctree &octree, SdfDAG &dag)
  {
    dag.nodes.resize(octree.nodes.size());
    dag.child_edges.reserve(octree.nodes.size());
    dag.data_edges.reserve(octree.nodes.size());
    dag.distances.reserve(octree.values_f.size());

    //empty links in 0 place, so 0 link is always means empty
    dag.child_edges.push_back(SdfDAGChildEdge{0, 0});
    dag.data_edges.push_back(SdfDAGDataEdge{0, 0, 0, 0.0f});    

    dag.voxel_channels = octree.voxel_channels;
    dag.point_channels = octree.point_channels;
    
    int f_off = 0, i_off = 0;
    for (int i = 0; i < octree.voxel_channels.size(); ++i)
    {
      if (octree.voxel_channels[i].type == DataChannel::Type::INT)
      {
        i_off++;
        if (i == octree.header.tc_channel_id)
          dag.header.all_int_mat_id_off = i_off;
      }
    }
    for (int i = 0; i < octree.voxel_channels.size(); ++i)
    {
      if (octree.voxel_channels[i].type == DataChannel::Type::FLOAT)
      {
        f_off++;
        if (i == octree.header.tc_channel_id)
          dag.header.all_float_tex_id_off = f_off;
      }
    }
    dag.header.mat_id_off = octree.header.mat_channel_id;
    dag.header.tex_id_off = octree.header.tc_channel_id;
    dag.header.brick_size = octree.header.brick_size;
    dag.header.brick_pad  = octree.header.brick_pad;
    dag.header.dim        = octree.header.dim;
    dag.header.node_grid_size = 2;

    global_octree_to_DAG8_direct_rec(octree, dag, 0);
  }

  void extract_subbrick_with_code(const float *brick, uint32_t size, uint4 code, float *out_brick)
  {
    float3 q0 = float3(code.x, code.y, code.z)/float3(code.w);
    float3 dq = float3(1.0f/(size*code.w));

    int v_sz = size + 1;
    for (int i=0;i<v_sz;i++)
    {
      for (int j=0;j<v_sz;j++)
      {
        for (int k=0;k<v_sz;k++)
        {
          float3 p = size*(q0 + float3(i,j,k)*dq);
          int3 pi = int3(floor(p));
          float3 dp = p - float3(pi);
          out_brick[i*v_sz*v_sz + j*v_sz + k] = 
            (1-dp.x)*(1-dp.y)*(1-dp.z)*brick[(pi.x+0)*v_sz*v_sz + (pi.y+0)*v_sz + pi.z+0] +
            (1-dp.x)*(1-dp.y)*(  dp.z)*brick[(pi.x+0)*v_sz*v_sz + (pi.y+0)*v_sz + pi.z+1] +
            (1-dp.x)*(  dp.y)*(1-dp.z)*brick[(pi.x+0)*v_sz*v_sz + (pi.y+1)*v_sz + pi.z+0] +
            (1-dp.x)*(  dp.y)*(  dp.z)*brick[(pi.x+0)*v_sz*v_sz + (pi.y+1)*v_sz + pi.z+1] +
            (  dp.x)*(1-dp.y)*(1-dp.z)*brick[(pi.x+1)*v_sz*v_sz + (pi.y+0)*v_sz + pi.z+0] +
            (  dp.x)*(1-dp.y)*(  dp.z)*brick[(pi.x+1)*v_sz*v_sz + (pi.y+0)*v_sz + pi.z+1] +
            (  dp.x)*(  dp.y)*(1-dp.z)*brick[(pi.x+1)*v_sz*v_sz + (pi.y+1)*v_sz + pi.z+0] +
            (  dp.x)*(  dp.y)*(  dp.z)*brick[(pi.x+1)*v_sz*v_sz + (pi.y+1)*v_sz + pi.z+1];
        }
      }
    }
  }

  void add_voxel_slice(const SdfDAG &dag, SdfDAG &out_dag, uint32_t in_node_id, uint32_t out_node_id, uint4 code)
  {
    const SdfDAGNode &node = dag.nodes[in_node_id];
    SdfDAGNode &out_node = out_dag.nodes[out_node_id];
    uint32_t v_size = 2*dag.header.brick_pad + dag.header.brick_size + 1;
    uint32_t s_count = DAG_extract_count(node.voxel_count_flags);
    out_node.voxel_count_flags = node.voxel_count_flags;

    if (s_count == 0)
    {
      out_node.data_edges_offset = 0;
    }
    else
    {
      //add new node and distances to it
      assert(node.data_edges_offset != 0);
      uint32_t de_off = out_dag.data_edges.size();
      uint32_t d_off = out_dag.distances.size();

      out_dag.data_edges.resize(de_off + s_count);
      out_dag.distances.resize(d_off + s_count*v_size*v_size*v_size);
      out_node.data_edges_offset = de_off;

      for (int s = 0; s < s_count; s++)
      {
        out_dag.data_edges[de_off + s] = dag.data_edges[node.data_edges_offset + s];
        out_dag.data_edges[de_off + s].data_offset = d_off + s * v_size * v_size * v_size;
        if (equal(code, uint4(0, 0, 0, 1)))
        {
          memcpy(out_dag.distances.data() + d_off + s * v_size * v_size * v_size,
                 dag.distances.data() + dag.data_edges[node.data_edges_offset + s].data_offset,
                 sizeof(float) * v_size * v_size * v_size);
        }
        else
        {
          extract_subbrick_with_code(dag.distances.data() + dag.data_edges[node.data_edges_offset + s].data_offset,
                                     dag.header.brick_size, code, out_dag.distances.data() + d_off + s * v_size * v_size * v_size);
        }
      }
    }
  }

  void make_shallow_DAG_rec(const SdfDAG &dag, SdfDAG &out_dag, uint32_t in_node_id, uint32_t out_node_id)
  {
    const SdfDAGNode &node = dag.nodes[in_node_id];
    assert(node.children_edges_offset != 0);

    add_voxel_slice(dag, out_dag, in_node_id, out_node_id, uint4(0,0,0,1));

    uint32_t old_cg_size = dag.header.node_grid_size;
    uint32_t cg_size = dag.header.node_grid_size*dag.header.node_grid_size;
    uint32_t cg_count = cg_size*cg_size*cg_size;

    uint32_t ce_off = out_dag.child_edges.size();
    uint32_t n_off = out_dag.nodes.size();
    out_dag.child_edges.resize(ce_off + cg_count);
    out_dag.nodes.resize(n_off + cg_count);

    out_dag.nodes[out_node_id].children_edges_offset = ce_off;

    std::vector<uint2> next_ids(cg_count);
    uint32_t next_ids_cnt = 0;
    for (int i0=0;i0<old_cg_size*old_cg_size*old_cg_size;i0++)
    {
      uint3 idx0 = uint3(i0/(old_cg_size*old_cg_size), i0/old_cg_size%old_cg_size, i0%old_cg_size);
      uint32_t ch_node_id = dag.child_edges[node.children_edges_offset + i0].child_offset;
      const auto &ch_node = dag.nodes[ch_node_id];
      
      for (int i1=0;i1<old_cg_size*old_cg_size*old_cg_size;i1++)
      {
        uint3 idx1 = uint3(i1/(old_cg_size*old_cg_size), i1/old_cg_size%old_cg_size, i1%old_cg_size);
        uint3 out_idx = old_cg_size*idx0 + idx1;
        uint32_t out_i = out_idx.x*cg_size*cg_size + out_idx.y*cg_size + out_idx.z;
        out_dag.child_edges[ce_off + out_i].rotation_id = 0;
        out_dag.child_edges[ce_off + out_i].child_offset = n_off + out_i;
        if (ch_node.children_edges_offset == 0)
        {
          add_voxel_slice(dag, out_dag, ch_node_id, n_off + out_i, uint4(idx1.x, idx1.y, idx1.z, old_cg_size));
        }
        else
        {
          uint32_t gch_node_id = dag.child_edges[ch_node.children_edges_offset + i1].child_offset;
          if (dag.nodes[gch_node_id].children_edges_offset == 0)
            add_voxel_slice(dag, out_dag, gch_node_id, n_off + out_i, uint4(0,0,0,1));
          else
            next_ids[next_ids_cnt++] = uint2(gch_node_id, n_off + out_i);
        }
      }
    }

    for (int i=0; i<next_ids_cnt; i++)
      make_shallow_DAG_rec(dag, out_dag, next_ids[i].x, next_ids[i].y);
  }

  void make_shallow_DAG(SdfDAG &dag, uint32_t steps)
  {
    for (int i=0; i<steps; i++)
    {
      SdfDAG new_dag; 
      new_dag.nodes.push_back(dag.nodes[0]);
      new_dag.child_edges.push_back(SdfDAGChildEdge{0, 0});
      new_dag.data_edges.push_back(SdfDAGDataEdge{0, 0, 0, 0.0f}); 
      new_dag.header = dag.header;
      new_dag.header.node_grid_size = 2*dag.header.node_grid_size;
      make_shallow_DAG_rec(dag, new_dag, 0, 0);
      
      dag = new_dag;
    }
  }

  void global_octree_to_DAG(const sdf_converter::GlobalOctree &octree, SdfDAG &dag, scom2::Settings settings)
  {
    auto t1 = std::chrono::high_resolution_clock::now();
    if (settings.output_type == scom2::OutputType::DAG8)
    {
      global_octree_to_DAG8_direct(octree, dag);
    }
    else if (settings.output_type == scom2::OutputType::DAG64)
    {
      assert(octree.header.dim == 3);
      global_octree_to_DAG8_direct(octree, dag);
      make_shallow_DAG(dag, 1);
    } 
    else
    {
      printf("[scom2::global_octree_to_DAG] Unsupported output type %d\n", (int)settings.output_type);
      return;
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    float time_ms = std::chrono::duration<float, std::milli>(t2 - t1).count();
    printf("global_octree_to_DAG: %.1f ms\n", time_ms);

    scom2::DAG_similarity_compression(dag, settings);

    auto t3 = std::chrono::high_resolution_clock::now();
    time_ms = std::chrono::duration<float, std::milli>(t3 - t2).count();
    printf("DAG_similarity_compression: %.1f ms\n", time_ms);
  }

  SdfDAG create_sdf_dag(SparseOctreeSettings settings, scom2::Settings scom2_settings, const cmesh4::SimpleMesh &mesh)
  {
    uint32_t depth_increase_mult = 1 << scom2_settings.octree_levels_to_bricks;
    uint32_t base_GO_brick_size = scom2_settings.brick_size / depth_increase_mult;
    if (scom2_settings.brick_size % depth_increase_mult != 0)
    {
      printf("WARNING: scom2_settings.brick_size (%d) is not a multiple of depth_increase_mult (%d)\n",
             scom2_settings.brick_size, depth_increase_mult);
      printf("Global octree will be created with initial brick_size = %d instead of upsampling\n",
             scom2_settings.brick_size);
      base_GO_brick_size = scom2_settings.brick_size;
      depth_increase_mult = 1;
    }
    sdf_converter::GlobalOctree g;
    sdf_converter::GlobalOctreeBuildStat build_stat;
    settings.brick_size = base_GO_brick_size;

    {
      auto t1 = std::chrono::steady_clock::now();
      auto plo = sdf_converter::create_triangle_list_octree(mesh, sdf_converter::PLOSettings(settings.depth));
      auto t2 = std::chrono::steady_clock::now();
      build_stat.time_build_PLO = std::chrono::duration<float, std::milli>(t2 - t1).count();
      mesh_octree_to_global_octree(settings, mesh, plo, g, &build_stat);
    }

    if (depth_increase_mult > 1)
    {
      sdf_converter::GlobalOctree go_ring[2];
      go_ring[0] = std::move(g);
      for (uint32_t i = 0; i < scom2_settings.octree_levels_to_bricks; i++)
        sdf_converter::upsample_global_octree(go_ring[i % 2], go_ring[(i + 1) % 2]);
      g = std::move(go_ring[scom2_settings.octree_levels_to_bricks % 2]);
    }

    SdfDAG dag;
    sdf_converter::global_octree_to_DAG(g, dag, scom2_settings);
    sdf_converter::print_octree_build_stat(build_stat);
    return dag;
  }

  SCom2Tree create_scom2_tree(SparseOctreeSettings settings, scom2::Settings scom2_settings, const cmesh4::SimpleMesh &mesh)
  {
    SdfDAG dag = create_sdf_dag(settings, scom2_settings, mesh);
    SCom2Tree tree;
    pack_SCom2(scom2_settings, dag, tree);
    return tree;
  }
}