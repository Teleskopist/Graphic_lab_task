#pragma once
#include "LiteMath.h"
#include <vector>
#include <string>
#include <memory>
#include <cassert>
#include <array>

using LiteMath::float2;
using LiteMath::float3;
using LiteMath::float4;
using LiteMath::uint2;
using LiteMath::uint3;
using LiteMath::uint4;

namespace sdf_converter
{
  static constexpr unsigned INVALID_IDX = 1u<<31u;

  // settings for building primitive list octree
  struct PLOSettings
  {
    PLOSettings() = default;
    explicit PLOSettings(unsigned _max_depth, unsigned _max_prim_per_leaf = 0, float _search_range_mult = 2.0f, 
                         bool _calc_intersections = true, unsigned _min_fill_level = 0)
    {
      max_depth = _max_depth;
      max_prim_per_leaf = _max_prim_per_leaf;
      search_range_mult = _search_range_mult;
      calc_intersections = _calc_intersections;
      min_fill_level = _min_fill_level;
    }

    unsigned max_depth = 1; // max depth of primitive list octree (PLO), usually matches the SDF octree depth
    unsigned max_prim_per_leaf = 0; // max primitives per leaf in PLO
    float search_range_mult = 2.0f; // multiplier for search range (bbox size of octree node where primitives are detected)
    bool calc_intersections = true; // calculate how many primitives actually interset with node bbox (not inflated by search_range_mult)
    unsigned min_fill_level = 0; // For all levels above this, list of primitives will not be saved only for leaves.
                                 // It slightly reduces build time and significantly reduces memory usage for PLO.
                                 // pid_offset for such node will point to a concatenation of all primitives for child nodes
                                 // and this list will contain duplicates. This option should be used carefully, because 
                                 // if other function will try to access primitive list for such node, it should know this behavior.
  };

  struct PrimitiveListOctree 
  {
    struct Node
    {
      uint32_t offset;// offset for children (they are stored together). 0 offset means it's a leaf
      uint32_t pid_offset;// start of Node's primitive ids in primitive_ids list, only for leaves
      uint32_t pid_count;// how many primitives affect this node
      uint32_t pid_intersect_count;// how many primitives actually intersect this node 
                                   // (calculated only for leaves and only with calc_intersections flag)
    };
    std::vector<Node> nodes;
    std::vector<uint32_t> primitive_ids;
  };

  static inline float contains(const float3 &p, const float3 &min, const float3 &max)
  {
    return min.x <= p.x && max.x >= p.x && min.y <= p.y && max.y >= p.y && min.z <= p.z && max.z >= p.z;
  }

  template <typename Model, typename PrimitiveInBBoxFunc, bool Multithreaded>
  static PrimitiveListOctree create_primitive_list_octree(const Model &model, unsigned total_prim_count, const PLOSettings &settings)
  {
    struct NodeDesc
    {
      unsigned idx = INVALID_IDX;
      float3 p = float3(0,0,0);
      float d = 0;
      unsigned level = 0;
      uint2 prims_idx = uint2(0,0);
      uint32_t prim_count = 0;
    };

    const uint32_t max_threads = Multithreaded ? omp_get_max_threads() : 1;
    std::array<std::vector<NodeDesc>, 2> node_descs;
    std::array<uint32_t, 2> nd_tops = {0, 0};
    std::array<std::vector<std::vector<uint32_t>>, 2> tmp_data;
    tmp_data[0].resize(max_threads);
    tmp_data[1].resize(max_threads);
    std::vector<uint32_t> tops(max_threads, 0);

    uint32_t prev = 0;
    uint32_t  cur = 1;

    PrimitiveListOctree plo;
    plo.nodes.resize(1);
    tmp_data[prev][0].resize(total_prim_count);
    for (int i=0;i<total_prim_count;i++)
      tmp_data[prev][0][i] = i;
    node_descs[prev].push_back({0, float3(0,0,0), 1.0f, 0, uint2(0, 0), total_prim_count});
    nd_tops[prev] = 1;

    while (nd_tops[prev] != 0)
    {
      nd_tops[cur] = 0;
      for (int i=0;i<max_threads;i++)
        tops[i] = 0;
      
      #pragma omp parallel for schedule(dynamic) if(Multithreaded)
      for (uint32_t desc_id=0;desc_id<nd_tops[prev];desc_id++)
      {
        const std::vector<std::vector<uint32_t>> &prim_data = tmp_data[prev];
        const uint32_t thread_id = omp_get_thread_num();
        std::vector<uint32_t> &tmp_buf = tmp_data[cur][thread_id];
        uint32_t &top = tops[thread_id];
        const NodeDesc nd = node_descs[prev][desc_id];
        if (tmp_buf.size() < top + nd.prim_count)
          tmp_buf.resize(top + nd.prim_count, INVALID_IDX);

        const float3 center = 2.0f * ((nd.p + float3(0.5, 0.5, 0.5)) * nd.d) - 1.0f;
        const float3 half_size = settings.search_range_mult*float3(nd.d);
        const float3 n_min = center - half_size;
        const float3 n_max = center + half_size;
        uint32_t prim_count = 0;

        for (uint32_t j = 0; j < nd.prim_count; ++j)
        {
          uint32_t pid = prim_data[nd.prims_idx.x][nd.prims_idx.y + j];
          if (PrimitiveInBBoxFunc::in_bbox(model, pid, n_min, n_max))
          {
            tmp_buf[top+prim_count] = pid;
            prim_count++;
          }
        }
        uint32_t intersect_count = prim_count;
        if (settings.calc_intersections)
        {
          const float3 real_half_size = (1+1e-4f)*float3(nd.d);
          const float3 real_n_min = center - real_half_size;
          const float3 real_n_max = center + real_half_size;
          intersect_count = 0;
          for (uint32_t j = 0; j < nd.prim_count; ++j)
          {
            uint32_t pid = prim_data[nd.prims_idx.x][nd.prims_idx.y + j];
            if (PrimitiveInBBoxFunc::in_bbox(model, pid, real_n_min, real_n_max))
              intersect_count++;
          }
        }

        bool is_leaf = (nd.level >= settings.max_depth || intersect_count <= settings.max_prim_per_leaf);

        // save primitive id list if it is needed (node is a leaf of have high enough level)
        uint32_t pid_offset = 0;      
        if (is_leaf || nd.level >= settings.min_fill_level)
        {
          #pragma omp critical
          {
            pid_offset = plo.primitive_ids.size();
            for (auto i = tmp_buf.begin() + top; i != tmp_buf.begin() + top + prim_count; ++i)
              assert(*i != INVALID_IDX);
            plo.primitive_ids.insert(plo.primitive_ids.end(), tmp_buf.begin() + top, tmp_buf.begin() + top + prim_count);
          }
        }

        #pragma omp critical
        {
          plo.nodes[nd.idx].pid_count = prim_count;
          plo.nodes[nd.idx].pid_intersect_count = intersect_count;
          plo.nodes[nd.idx].pid_offset = pid_offset;
          plo.nodes[nd.idx].offset = 0;
        }

        // if it is not a leaf, add children to the tree and  to the next NodeDesc list
        if (!is_leaf)
        {
          uint32_t n_off = INVALID_IDX;
          uint32_t nd_off = INVALID_IDX;
          #pragma omp critical
          {
            n_off = plo.nodes.size();
            nd_off = nd_tops[cur];

            plo.nodes[nd.idx].offset = n_off;
            plo.nodes.resize(n_off + 8);

            nd_tops[cur] += 8;
            if (node_descs[cur].size() < nd_tops[cur])
              node_descs[cur].resize(nd_tops[cur]);

            for (int i=0;i<8;i++)
            {
              float ch_d = nd.d/2;
              float3 ch_p = 2*nd.p + float3((i & 4) >> 2, (i & 2) >> 1, i & 1);
              node_descs[cur][nd_off+i] = {n_off+i, ch_p, ch_d, nd.level+1, uint2(thread_id, top), prim_count};
            }
          }
          // update top for current list
          top += prim_count;
        }
      }

      cur = prev;
      prev = (prev + 1) % 2;
    }

    return plo;
  }
}