#include "sdf_converter.h"
#include "utils/mesh/mesh_bvh.h"
#include "sdf/build/sparse_octree_builder.h"
#include "utils/mesh/mesh.h"
#include "sdf/build/sparse_octree_common.h"
#include "omp.h"
#include <chrono>

namespace sdf_converter
{
  void global_octree_to_grid(const GlobalOctree &octree, SdfGrid &grid)
  {
    std::vector<uint3> offsets(octree.nodes.size());
    uint32_t max_off = 0;
    offsets[0] = uint3{0, 0, 0};
    for (int i = 0; i < octree.nodes.size(); ++i)
    {
      for (int j = 0; j < 8; ++j)
      {
        if (octree.nodes[i].offset != 0)
        {
          offsets[octree.nodes[i].offset + j] = offsets[i] * 2 + uint3{(unsigned(j) >> 2), (unsigned(j) >> 1) & 1, unsigned(j) & 1};
          if (offsets[octree.nodes[i].offset + j].x > max_off)
          {
            max_off = offsets[octree.nodes[i].offset + j].x;
          }
        }
      }
    }
    grid.size = uint3{(max_off + 1) * octree.header.brick_size + 1, (max_off + 1) * octree.header.brick_size + 1, (max_off + 1) * octree.header.brick_size + 1};
    uint32_t sz = grid.size.x;
    uint32_t max_data_brick = octree.header.brick_size + 1 + octree.header.brick_pad * 2;
    for (int i = 0; i < octree.nodes.size(); ++i)
    {
      if (octree.nodes[i].offset == 0)
      {
        for (int x = octree.header.brick_pad; x <= octree.header.brick_pad + octree.header.brick_size; ++x)
        {
          for (int y = octree.header.brick_pad; y <= octree.header.brick_pad + octree.header.brick_size; ++y)
          {
            for (int z = octree.header.brick_pad; z <= octree.header.brick_pad + octree.header.brick_size; ++z)
            {
              uint3 off = uint3{x - octree.header.brick_pad, y - octree.header.brick_pad, z - octree.header.brick_pad};
              uint3 pos = offsets[i] * octree.header.brick_size + off;
              grid.data[pos.z*sz*sz + pos.y*sz + pos.x] = octree.values_f[octree.nodes[i].val_off + x * max_data_brick * max_data_brick + y * max_data_brick + z];
            }
          }
        }
      }
    }
  }

  void global_octree_to_frame_octree(const GlobalOctree &octree, std::vector<SdfFrameOctreeNode> &out_frame)
  {
    assert(octree.header.brick_size == 1);
    assert(octree.header.brick_pad == 0);

    out_frame.resize(octree.nodes.size());
    for (int i = 0; i < octree.nodes.size(); ++i)
    {
      out_frame[i].offset = octree.nodes[i].offset;
      for (int j=0;j<8;j++)
        out_frame[i].values[j] = octree.values_f[octree.nodes[i].val_off + j];
    }
  }

  void global_octree_to_frame_octree_tex(const GlobalOctree &octree, std::vector<SdfFrameOctreeTexNode> &out_frame)
  {
    assert(octree.header.brick_size == 1);
    assert(octree.header.brick_pad == 0);

    out_frame.resize(octree.nodes.size());
    for (int i = 0; i < octree.nodes.size(); ++i)
    {
      out_frame[i].offset = octree.nodes[i].offset;
      //out_frame[i].material_id = octree.nodes[i].material_id;
      if (octree.header.mat_channel_id >= 0)
      {
        auto &mat_channel = octree.voxel_channels[octree.header.mat_channel_id];
        out_frame[i].material_id = mat_channel.data_i[mat_channel.num_components * i];
      }
      else
      {
        out_frame[i].material_id = 0;
      }
      for (int j=0;j<8;j++)
      {
        out_frame[i].values[j] = octree.values_f[octree.nodes[i].val_off + j];

        if (octree.header.tc_channel_id >= 0)
        {
          auto &tc_channel = octree.point_channels[octree.header.mat_channel_id];
          out_frame[i].tex_coords[j * 2 + 0] = tc_channel.data_f[tc_channel.num_components * 8 * i + j * 2 + 0];
          out_frame[i].tex_coords[j * 2 + 1] = tc_channel.data_f[tc_channel.num_components * 8 * i + j * 2 + 1];
        }
        else
        {
          out_frame[i].tex_coords[j * 2 + 0] = 0;
          out_frame[i].tex_coords[j * 2 + 1] = 0;
        }
      }
    }
  }

  void global_octree_to_SVS_rec(const GlobalOctree &octree, std::vector<SdfSVSNode> &svs,
                                unsigned node_idx, unsigned lod_size, uint3 p)
  {
    if (octree.nodes[node_idx].offset == 0)
    {
      //unsigned v_size = sbs.header.brick_size + 2 * sbs.header.brick_pad + 1;
      unsigned v_off = octree.nodes[node_idx].val_off;

      if (octree.nodes[node_idx].type != GlobalOctreeNodeType::EMPTY)
      {
        unsigned n_off = svs.size();
        float d_max = 2 * sqrt(3) / lod_size;
        unsigned bits = 8;
        unsigned max_val = ((1 << bits) - 1);
        unsigned vals_per_int = 4;

        svs.emplace_back();

        svs[n_off].pos_xy = (p.x << 16) | p.y;
        svs[n_off].pos_z_lod_size = (p.z << 16) | lod_size;

        svs[n_off].values[0] = 0u;
        svs[n_off].values[1] = 0u;

        for (int i = 0; i < 8; i++)
        {
          unsigned d_compressed = std::max(0.0f, max_val * ((octree.values_f[v_off + i] + d_max) / (2 * d_max)) + 0.5f);
          d_compressed = std::min(d_compressed, max_val);
          svs[n_off].values[i / vals_per_int] |= d_compressed << (bits * (i % vals_per_int));
        }
      }
    }
    else
    {
      for (int i = 0; i < 8; i++)
      {
        float ch_d = lod_size / 2;
        uint3 ch_p = 2 * p + uint3((i & 4) >> 2, (i & 2) >> 1, i & 1);
        global_octree_to_SVS_rec(octree, svs, octree.nodes[node_idx].offset + i, 2*lod_size, ch_p);
      }
    }
  }

  void global_octree_to_SVS(const GlobalOctree &octree, std::vector<SdfSVSNode> &svs)
  {
    assert(1 == octree.header.brick_size);
    assert(0 == octree.header.brick_pad);

    global_octree_to_SVS_rec(octree, svs, 0, 1, uint3(0,0,0));
  }

  void global_octree_to_SBS_rec(const GlobalOctree &octree, SdfSBS &sbs,
                                unsigned node_idx, unsigned lod_size, uint3 p)
  {
    if (octree.nodes[node_idx].offset == 0)
    {
      unsigned v_size = sbs.header.brick_size + 2 * sbs.header.brick_pad + 1;
      unsigned v_off = octree.nodes[node_idx].val_off;

      if (octree.nodes[node_idx].type != GlobalOctreeNodeType::EMPTY)
      {
        unsigned off = sbs.values.size();
        unsigned n_off = sbs.nodes.size();
        float d_max = 2 * sqrt(3) / lod_size;
        unsigned bits = 8 * sbs.header.bytes_per_value;
        unsigned max_val = sbs.header.bytes_per_value == 4 ? 0xFFFFFFFF : ((1 << bits) - 1);
        unsigned vals_per_int = 4 / sbs.header.bytes_per_value;
        unsigned texs_size = (sbs.header.aux_data & SDF_SBS_NODE_LAYOUT_MASK) == SDF_SBS_NODE_LAYOUT_DX_UV16 ? 32 : 0;
        unsigned dist_size = (v_size * v_size * v_size + vals_per_int - 1) / vals_per_int;

        sbs.nodes.emplace_back();
        sbs.values.resize(sbs.values.size() + dist_size + texs_size);

        sbs.nodes[n_off].data_offset = off;
        sbs.nodes[n_off].pos_xy = (p.x << 16) | p.y;
        sbs.nodes[n_off].pos_z_lod_size = (p.z << 16) | lod_size;

        for (int i = 0; i < v_size * v_size * v_size; i++)
        {
          unsigned d_compressed = std::max(0.0f, max_val * ((octree.values_f[v_off + i] + d_max) / (2 * d_max)));
          d_compressed = std::min(d_compressed, max_val);
          sbs.values[off + i / vals_per_int] |= d_compressed << (bits * (i % vals_per_int));
        }
        if ((sbs.header.aux_data & SDF_SBS_NODE_LAYOUT_MASK) == SDF_SBS_NODE_LAYOUT_DX_UV16)
        {
          float tc_x = 0, tc_y = 0;
          for (int i = 0; i < 8; ++i)
          {
            if (octree.header.tc_channel_id >= 0)
            {
              auto &tc_data = octree.point_channels[octree.header.tc_channel_id];
              tc_x = tc_data.data_f[node_idx * tc_data.num_components * 8 + i * 2 + 0];
              tc_y = tc_data.data_f[node_idx * tc_data.num_components * 8 + i * 2 + 1];
            }
            unsigned packed_u = ((1<<16) - 1)*LiteMath::clamp(tc_x/*octree.nodes[node_idx].tex_coords[i].x*/, 0.0f, 1.0f);
            unsigned packed_v = ((1<<16) - 1)*LiteMath::clamp(tc_y/*octree.nodes[node_idx].tex_coords[i].y*/, 0.0f, 1.0f);
            sbs.values[off + dist_size + i] = (packed_u << 16) | (packed_v & 0x0000FFFF);
          }
        }
      }
    }
    else
    {
      for (int i = 0; i < 8; i++)
      {
        float ch_d = lod_size / 2;
        uint3 ch_p = 2 * p + uint3((i & 4) >> 2, (i & 2) >> 1, i & 1);
        global_octree_to_SBS_rec(octree, sbs, octree.nodes[node_idx].offset + i, 2*lod_size, ch_p);
      }
    }
  }

  void global_octree_to_SBS(const GlobalOctree &octree, SdfSBS &sbs)
  {
    assert(sbs.header.brick_size == octree.header.brick_size);
    assert(sbs.header.brick_pad == octree.header.brick_pad);
    assert((sbs.header.aux_data & SDF_SBS_NODE_LAYOUT_MASK) == SDF_SBS_NODE_LAYOUT_DX ||
           (sbs.header.aux_data & SDF_SBS_NODE_LAYOUT_MASK) == SDF_SBS_NODE_LAYOUT_DX_UV16);

    global_octree_to_SBS_rec(octree, sbs, 0, 1, uint3(0,0,0));
  }

  SdfGrid create_sdf_grid(GridSettings settings, MultithreadedDistanceFunction sdf, unsigned max_threads)
  {
    assert(settings.size >= 1);

    unsigned sz = settings.size + 1;
    SdfGrid grid;
    grid.size = uint3(sz, sz, sz);
    //printf("creating grid: %ux%ux%u\n", grid.size.x, grid.size.y, grid.size.z);

    unsigned long total_size = grid.size.x * grid.size.y * grid.size.z;
    if (total_size > 512 * 1000 * 1000)
      printf("Warning: large grid size: %lu\n", total_size);

    grid.data.resize(total_size);

    omp_set_num_threads(max_threads);
    #pragma omp parallel
    for (int i = 0; i < sz; i++)
      for (int j = 0; j < sz; j++)
        for (int k = 0; k < sz; k++)
          grid.data[i*sz*sz + j*sz + k] = sdf(2.0f*(float3(k, j, i)/float(sz - 1)) - 1.0f, omp_get_thread_num());
    omp_set_num_threads(omp_get_max_threads());

    return grid;
  }

  SdfGrid create_sdf_grid(GridSettings settings, const cmesh4::SimpleMesh &mesh)
  {
    unsigned max_threads = omp_get_max_threads();

    std::vector<MeshBVH> bvh(max_threads);
    for (unsigned i = 0; i < max_threads; i++)
      bvh[i].init(mesh);
    
    return create_sdf_grid(settings, [&](const float3 &p, unsigned idx) -> float 
                           { return bvh[idx].get_signed_distance(p); }, max_threads);
  }

  std::vector<SdfFrameOctreeNode> create_sdf_frame_octree(SparseOctreeSettings settings, MultithreadedDistanceFunction sdf, unsigned max_threads)
  {
    GlobalOctree g;
    std::vector<SdfFrameOctreeNode> frame;
    sdf_to_global_octree(settings, sdf, max_threads, g);
    global_octree_to_frame_octree(g, frame);

    return frame;
  }

  std::vector<SdfFrameOctreeNode> create_sdf_frame_octree(SparseOctreeSettings settings, const cmesh4::SimpleMesh &mesh)
  {
    GlobalOctree g;
    std::vector<SdfFrameOctreeNode> frame;
    {
      auto plo = create_triangle_list_octree(mesh, PLOSettings(settings.depth));
      mesh_octree_to_global_octree(settings, mesh, plo, g);
    }
    global_octree_to_frame_octree(g, frame);

    return frame;
  }
  
  std::vector<SdfSVSNode> create_sdf_SVS(SparseOctreeSettings settings, MultithreadedDistanceFunction sdf, unsigned max_threads)
  {
    GlobalOctree g;
    std::vector<SdfSVSNode> svs;
    sdf_to_global_octree(settings, sdf, max_threads, g);
    global_octree_to_SVS(g, svs);

    return svs;
  }

  std::vector<SdfSVSNode> create_sdf_SVS(SparseOctreeSettings settings, const cmesh4::SimpleMesh &mesh)
  {
    GlobalOctree g;
    std::vector<SdfSVSNode> svs;
    {
      auto plo = create_triangle_list_octree(mesh, PLOSettings(settings.depth));
      mesh_octree_to_global_octree(settings, mesh, plo, g);
    }
    global_octree_to_SVS(g, svs);

    return svs;
  }

  SdfSBS create_sdf_SBS(SparseOctreeSettings settings, SdfSBSHeader header, MultithreadedDistanceFunction sdf, unsigned max_threads)
  {
    GlobalOctree g;
    settings.brick_size = header.brick_size;
    settings.brick_pad = header.brick_pad;
    SdfSBS sbs;
    sbs.header = header;
    sdf_to_global_octree(settings, sdf, max_threads, g);
    global_octree_to_SBS(g, sbs);

    return sbs;
  }

  SdfSBS create_sdf_SBS(SparseOctreeSettings settings, SdfSBSHeader header, const cmesh4::SimpleMesh &mesh)
  {
    assert(settings.depth > 1);
    assert(header.brick_size >= 1 && header.brick_size <= 16);
    assert(header.brick_pad == 0 || header.brick_pad == 1);
    assert(header.bytes_per_value == 1 || header.bytes_per_value == 2 || header.bytes_per_value == 4);

    GlobalOctree g;
    settings.brick_size = header.brick_size;
    settings.brick_pad = header.brick_pad;
    SdfSBS sbs;
    sbs.header = header;
    {
      auto plo = create_triangle_list_octree(mesh, PLOSettings(settings.depth));
      mesh_octree_to_global_octree(settings, mesh, plo, g);
    }
    global_octree_to_SBS(g, sbs);

    return sbs;
  }

  std::vector<SdfFrameOctreeTexNode> create_sdf_frame_octree_tex(SparseOctreeSettings settings, const cmesh4::SimpleMesh &mesh)
  {
    // we set max_triangles_per_leaf = 0 to prevent issues with big, textured triangles (see test with textured cube)
    GlobalOctree g;
    {
      auto plo = create_triangle_list_octree(mesh, PLOSettings(settings.depth));
      mesh_octree_to_global_octree(settings, mesh, plo, g);
    }
    std::vector<SdfFrameOctreeTexNode> frame;
    global_octree_to_frame_octree_tex(g, frame);
    return frame;
  }

  SdfSBS create_sdf_SBS_tex(SparseOctreeSettings settings, SdfSBSHeader header, const cmesh4::SimpleMesh &mesh, bool noisy)
  {
    header.aux_data = SDF_SBS_NODE_LAYOUT_DX_UV16;

    unsigned max_threads = omp_get_max_threads();
    GlobalOctree g;
    settings.brick_size = header.brick_size;
    settings.brick_pad = header.brick_pad;
    auto plo = create_triangle_list_octree(mesh, PLOSettings(settings.depth));

    if (noisy)
    {    
      std::vector<MeshBVH> bvh(max_threads);
      for (unsigned i = 0; i < max_threads; i++)
        bvh[i].init(mesh);
      MultithreadedDistanceFunction mt_sdf = [&](const float3 &p, unsigned idx) -> float 
                                            {auto x = (fmod(p.x * 153 + p.y * 427 + p.z * 311, 2.0) - 1) * 0.0075;
                                             return bvh[idx].get_signed_distance(p) + x; };
      sdf_to_global_octree(settings, mt_sdf, max_threads, g);
    }
    else
    {
      mesh_octree_to_global_octree(settings, mesh, plo, g);
    }

    SdfSBS sbs;
    sbs.header = header;
    global_octree_to_SBS(g, sbs);

    return sbs;
  }

  GlobalOctree create_global_octree_by_mesh(SparseOctreeSettings settings, const cmesh4::SimpleMesh &mesh, 
                                            GlobalOctreeHeader &header)
  {
    assert(header.brick_size != 0);
    float mult = (float) (2 * header.brick_size + header.brick_pad);
    mult /= (float) header.brick_size;
    GlobalOctree g;
    g.header = header;
    {
      auto plo = create_triangle_list_octree(mesh, PLOSettings(settings.depth, 0, mult));//change 1.0f to another mult depend of the pad
      mesh_octree_to_global_octree(settings, mesh, plo, g);
    }
    return g;
  }
}