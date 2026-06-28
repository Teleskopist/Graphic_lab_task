#include <vector>
#include <cstdint>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <cstdio>
#include <limits>
#include <array>

#include "mesh_voxelizer.h"
#include "voxel_octree_internal.h"
#include "utils/mesh/mesh.h"
#include "utils/mesh/mesh_internal.h"
#include "utils/mesh/triangle_list_octree.h"

#include <omp.h>


using LiteMath::float3;
using LiteMath::uint3;

using namespace Voxelizer;

namespace
{
  static constexpr uint32_t INVALID_IDX = 0xFFFFFFFFu;

  struct BuildNode
  {
    uint32_t value = 0;
    uint32_t ch_off = 0;
    uint32_t total_active_children = 0;
  };

  struct SolidTriangle
  {
    float3 a;
    float3 b;
    float3 c;
    float max_x = 0.0f;
  };

  struct SolidFillAccel
  {
    uint32_t bin_count = 0;
    std::vector<SolidTriangle> triangles;
    std::vector<uint32_t> bin_offsets;
    std::vector<uint32_t> triangle_ids;
  };

  struct BinRange { uint32_t y0, y1, z0, z1; };

  static uint3 octree_child_code(uint32_t child_id)
  {
    return uint3((child_id & 4) >> 2, (child_id & 2) >> 1, child_id & 1);
  }

  static uint32_t round_up_power_of_two(uint32_t v)
  {
    if (v <= 1)
      return 1;

    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1;
  }

  static bool triangle_intersects_final_voxel(const cmesh4::SimpleMesh &mesh,
                                              uint32_t triangle_id,
                                              const uint3 &voxel_pos,
                                              uint32_t final_resolution)
  {
    const uint32_t i0 = mesh.indices[3 * triangle_id + 0];
    const uint32_t i1 = mesh.indices[3 * triangle_id + 1];
    const uint32_t i2 = mesh.indices[3 * triangle_id + 2];
    const float3 a = LiteMath::to_float3(mesh.vPos4f[i0]);
    const float3 b = LiteMath::to_float3(mesh.vPos4f[i1]);
    const float3 c = LiteMath::to_float3(mesh.vPos4f[i2]);

    const float voxel_size = 2.0f / float(final_resolution);
    const float3 voxel_min = float3(-1.0f) + voxel_size * float3(voxel_pos);
    const float3 voxel_max = voxel_min + float3(voxel_size);
    return cmesh4::triangle_aabb_intersect(a, b, c,
                                           0.5f * (voxel_min + voxel_max),
                                           0.5f * (voxel_max - voxel_min));
  }

  static bool ray_intersects_triangle_x(const float3 &ray_origin,
                                        const float3 &a,
                                        const float3 &b,
                                        const float3 &c,
                                        float *out_t)
  {
    constexpr float epsilon = std::numeric_limits<float>::epsilon();

    const float3 edge1 = b - a;
    const float3 edge2 = c - a;
    const float3 ray_vector(1.0f, 0.0f, 0.0f);
    const float3 ray_cross_e2 = LiteMath::cross(ray_vector, edge2);
    const float det = LiteMath::dot(edge1, ray_cross_e2);
    if (std::abs(det) < epsilon)
      return false;

    const float inv_det = 1.0f / det;
    const float3 s = ray_origin - a;
    const float u = LiteMath::dot(s, ray_cross_e2) * inv_det;
    if (u < 0 || u > 1)
      return false;

    const float3 s_cross_e1 = LiteMath::cross(s, edge1);
    const float v = LiteMath::dot(ray_vector, s_cross_e1) * inv_det;
    if (v < 0 || u + v > 1)
      return false;

    const float t = LiteMath::dot(edge2, s_cross_e1) * inv_det;
    if (t <= epsilon)
      return false;

    *out_t = t;
    return true;
  }

  static uint32_t solid_bin_coord(float v, uint32_t bin_count)
  {
    const float normalized = 0.5f * (v + 1.0f);
    const int i = static_cast<int>(std::floor(normalized * float(bin_count)));
    return static_cast<uint32_t>(std::max(0, std::min(i, int(bin_count) - 1)));
  }

  static uint32_t solid_bin_index(uint32_t y, uint32_t z, uint32_t bin_count)
  {
    return y * bin_count + z;
  }

  static uint32_t choose_solid_bin_count(uint32_t top_tree_resolution,
                                         uint32_t final_resolution)
  {
    uint32_t bin_count = std::max(top_tree_resolution, 16u);
    bin_count = std::min(bin_count, final_resolution);
    bin_count = std::min(bin_count, 1024u);
    return round_up_power_of_two(bin_count);
  }

  static BinRange triangle_bin_range(const SolidTriangle &t, uint32_t bin_count)
  {
    BinRange range;
    range.y0 = solid_bin_coord(std::min({t.a.y, t.b.y, t.c.y}), bin_count);
    range.y1 = solid_bin_coord(std::max({t.a.y, t.b.y, t.c.y}), bin_count);
    range.z0 = solid_bin_coord(std::min({t.a.z, t.b.z, t.c.z}), bin_count);
    range.z1 = solid_bin_coord(std::max({t.a.z, t.b.z, t.c.z}), bin_count);
    return range;
  }

  static SolidFillAccel build_solid_fill_accel(const cmesh4::SimpleMesh &mesh,
                                               uint32_t bin_count)
  {
    SolidFillAccel accel;
    accel.bin_count = std::max(bin_count, 1u);
    accel.triangles.resize(mesh.TrianglesNum());

    const uint64_t bin_total_64 = uint64_t(accel.bin_count) * uint64_t(accel.bin_count);
    if (bin_total_64 > std::numeric_limits<uint32_t>::max())
      throw std::runtime_error("solid fill bin grid is too large");

    const uint32_t bin_total = static_cast<uint32_t>(bin_total_64);
    std::vector<uint32_t> bin_counts(bin_total, 0);

    #pragma omp parallel for schedule(dynamic)
    for (uint64_t triangle_id = 0; triangle_id < mesh.TrianglesNum(); ++triangle_id)
    {
      const uint32_t i0 = mesh.indices[3 * triangle_id + 0];
      const uint32_t i1 = mesh.indices[3 * triangle_id + 1];
      const uint32_t i2 = mesh.indices[3 * triangle_id + 2];
      SolidTriangle triangle;
      triangle.a = LiteMath::to_float3(mesh.vPos4f[i0]);
      triangle.b = LiteMath::to_float3(mesh.vPos4f[i1]);
      triangle.c = LiteMath::to_float3(mesh.vPos4f[i2]);
      triangle.max_x = std::max({triangle.a.x, triangle.b.x, triangle.c.x});
      accel.triangles[triangle_id] = triangle;
      
      BinRange range = triangle_bin_range(triangle, accel.bin_count);

      for (uint32_t y = range.y0; y <= range.y1; ++y)
      {
        for (uint32_t z = range.z0; z <= range.z1; ++z) 
        {
          const uint32_t bin_id = solid_bin_index(y, z, accel.bin_count);
          #pragma omp atomic update
          ++bin_counts[bin_id];
        }
      }
    }

    accel.bin_offsets.resize(size_t(bin_total) + 1, 0);
    uint64_t total_refs = 0;
    for (uint32_t i = 0; i < bin_total; ++i)
    {
      accel.bin_offsets[i] = static_cast<uint32_t>(total_refs);
      total_refs += bin_counts[i];
      if (total_refs > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("solid fill triangle-bin reference list is too large");
    }
    accel.bin_offsets[bin_total] = static_cast<uint32_t>(total_refs);
    accel.triangle_ids.resize(static_cast<size_t>(total_refs));

    std::vector<uint32_t> write_offsets = accel.bin_offsets;
    #pragma omp parallel for schedule(dynamic)
    for (uint64_t triangle_id = 0; triangle_id < uint64_t(accel.triangles.size()); ++triangle_id)
    {
      const SolidTriangle &triangle = accel.triangles[triangle_id];
      BinRange range = triangle_bin_range(triangle, accel.bin_count);

      for (uint32_t y = range.y0; y <= range.y1; ++y)
      {
        for (uint32_t z = range.z0; z <= range.z1; ++z) 
        {
          const uint32_t bin_id = solid_bin_index(y, z, accel.bin_count);

          uint32_t write_pos = 0;
          #pragma omp atomic capture
          write_pos = write_offsets[bin_id]++;

          accel.triangle_ids[write_pos] = uint32_t(triangle_id);
        }
      }
    }

    return accel;
  }

  static bool point_inside_mesh(const SolidFillAccel &accel, const float3 &p)
  {
    if (p.y < -1.0f || p.y > 1.0f || p.z < -1.0f || p.z > 1.0f)
      return false;

    const uint32_t y = solid_bin_coord(p.y, accel.bin_count);
    const uint32_t z = solid_bin_coord(p.z, accel.bin_count);
    const uint32_t bin_id = solid_bin_index(y, z, accel.bin_count);
    const uint32_t begin = accel.bin_offsets[bin_id];
    const uint32_t end = accel.bin_offsets[bin_id + 1];

    std::vector<float> intersections;
    intersections.reserve(32);

    for (uint32_t ref_id = begin; ref_id < end; ++ref_id)
    {
      const SolidTriangle &triangle = accel.triangles[accel.triangle_ids[ref_id]];

      if (triangle.max_x <= p.x)
        continue;

      float t = 0.0f;
      if (ray_intersects_triangle_x(p, triangle.a, triangle.b, triangle.c, &t))
        intersections.push_back(t);
    }

    if (intersections.empty())
      return false;

    std::sort(intersections.begin(), intersections.end());
    uint32_t unique_count = 0;
    float last_t = -1.0f;
    for (float t : intersections)
    {
      if (last_t < 0.0f || std::abs(t - last_t) > 1e-5f)
      {
        ++unique_count;
        last_t = t;
      }
    }

    return (unique_count & 1u) != 0;
  }

  static float3 final_voxel_center(const uint3 &voxel_pos, uint32_t final_resolution)
  {
    const float voxel_size = 2.0f / float(final_resolution);
    return float3(-1.0f) + voxel_size * (float3(voxel_pos) + float3(0.5f));
  }

  static float3 top_node_center(const uint3 &top_node_pos, uint32_t top_node_size,
                                uint32_t leaf_grid_size, uint32_t final_resolution)
  {
    const float voxel_size = 2.0f / float(final_resolution);
    const float3 final_pos = leaf_grid_size *
        (float3(top_node_pos) + 0.5f * float(top_node_size));
    return float3(-1.0f) + voxel_size * final_pos;
  }

  static bool leaf_grid_cell_has_surface(const cmesh4::SimpleMesh &mesh,
                                         const sdf_converter::PrimitiveListOctree &triangle_tree,
                                         const sdf_converter::PrimitiveListOctree::Node &triangle_node,
                                         const uint3 &final_voxel_pos, uint32_t final_resolution)
  {
    for (uint32_t i = 0; i < triangle_node.pid_count; ++i)
    {
      const uint32_t triangle_id = triangle_tree.primitive_ids[triangle_node.pid_offset + i];
      if (triangle_intersects_final_voxel(mesh, triangle_id, final_voxel_pos, final_resolution))
        return true;
    }
    return false;
  }

  static uint32_t build_leaf_grid_rec(const cmesh4::SimpleMesh &mesh,
                                      const sdf_converter::PrimitiveListOctree &triangle_tree,
                                      const sdf_converter::PrimitiveListOctree::Node &triangle_node,
                                      std::vector<BuildNode> &nodes, uint32_t out_node_id,
                                      const uint3 &top_leaf_pos, const uint3 &local_pos,
                                      uint32_t local_size, uint32_t leaf_grid_size,
                                      uint32_t final_resolution, uint32_t voxel_value,
                                      const SolidFillAccel *solid_accel)
  {
    if (local_size == 1)
    {
      const uint3 final_voxel_pos = top_leaf_pos * leaf_grid_size + local_pos;
      const bool has_surface =
          leaf_grid_cell_has_surface(mesh, triangle_tree, triangle_node, final_voxel_pos, final_resolution);
      const bool inside =
          solid_accel && !has_surface &&
          point_inside_mesh(*solid_accel, final_voxel_center(final_voxel_pos, final_resolution));
      nodes[out_node_id].value = has_surface || inside ? voxel_value : 0;
      nodes[out_node_id].total_active_children = nodes[out_node_id].value == 0 ? 0 : 1;
      return nodes[out_node_id].total_active_children;
    }

    const uint32_t child_offset = static_cast<uint32_t>(nodes.size());
    nodes.resize(child_offset + 8);
    nodes[out_node_id].ch_off = child_offset;
    nodes[out_node_id].value = 0;
    nodes[out_node_id].total_active_children = 0;

    const uint32_t child_size = local_size / 2;
    for (uint32_t i = 0; i < 8; ++i)
    {
      const uint3 child_pos = local_pos + child_size * octree_child_code(i);
      build_leaf_grid_rec(mesh, triangle_tree, triangle_node, nodes, child_offset + i,
                          top_leaf_pos, child_pos, child_size, leaf_grid_size,
                          final_resolution, voxel_value, solid_accel);
      nodes[out_node_id].total_active_children += nodes[child_offset + i].total_active_children;
    }

    if (nodes[out_node_id].total_active_children == 0)
      nodes[out_node_id].ch_off = 0;
    else
      nodes[out_node_id].total_active_children += 1;

    return nodes[out_node_id].total_active_children;
  }

  static uint64_t count_active_leaf_nodes_rec(const std::vector<BuildNode> &nodes,
                                              uint32_t node_id)
  {
    const BuildNode &node = nodes[node_id];
    if (node.ch_off == 0)
      return node.total_active_children > 0 ? 1u : 0u;

    uint64_t count = 0;
    for (uint32_t i = 0; i < 8; ++i)
      count += count_active_leaf_nodes_rec(nodes, node.ch_off + i);
    return count;
  }

  static uint32_t build_nodes_from_triangle_tree_rec(
      const cmesh4::SimpleMesh &mesh,
      const sdf_converter::PrimitiveListOctree &triangle_tree,
      std::vector<BuildNode> &nodes,
      uint32_t triangle_node_id,
      uint32_t out_node_id,
      const uint3 &top_node_pos,
      uint32_t top_node_size,
      uint32_t leaf_grid_size,
      uint32_t final_resolution,
      uint32_t voxel_value,
      const SolidFillAccel *solid_accel)
  {
    const sdf_converter::PrimitiveListOctree::Node &triangle_node =
        triangle_tree.nodes[triangle_node_id];

    if (triangle_node.offset == 0)
    {
      if (triangle_node.pid_intersect_count > 0 && leaf_grid_size > 1)
      {
        return build_leaf_grid_rec(mesh, triangle_tree, triangle_node, nodes, out_node_id,
                                   top_node_pos, uint3(0, 0, 0), leaf_grid_size, leaf_grid_size,
                                   final_resolution, voxel_value, solid_accel);
      }

      const bool inside =
          solid_accel && triangle_node.pid_intersect_count == 0 &&
          point_inside_mesh(*solid_accel, top_node_center(top_node_pos, top_node_size,
                                                         leaf_grid_size, final_resolution));
      nodes[out_node_id].value = triangle_node.pid_intersect_count > 0 || inside ? voxel_value : 0;
      nodes[out_node_id].total_active_children = nodes[out_node_id].value == 0 ? 0 : 1;
      return nodes[out_node_id].total_active_children;
    }

    const uint32_t child_offset = static_cast<uint32_t>(nodes.size());
    nodes.resize(child_offset + 8);
    nodes[out_node_id].ch_off = child_offset;
    nodes[out_node_id].value = 0;
    nodes[out_node_id].total_active_children = 0;

    for (uint32_t i = 0; i < 8; ++i)
    {
      build_nodes_from_triangle_tree_rec(mesh, triangle_tree, nodes, triangle_node.offset + i,
                                         child_offset + i, 2u * top_node_pos + octree_child_code(i),
                                         top_node_size / 2, leaf_grid_size, final_resolution,
                                         voxel_value, solid_accel);
      const BuildNode &child_node = nodes[child_offset + i];
      nodes[out_node_id].total_active_children += child_node.total_active_children;
    }

    if (nodes[out_node_id].total_active_children == 0)
    {
      nodes[out_node_id].ch_off = 0;
    }
    else
    {
      nodes[out_node_id].total_active_children += 1;
    }

    return nodes[out_node_id].total_active_children;
  }

  static void pack_svo_rec(const std::vector<BuildNode> &build_nodes, std::vector<uint32_t> &svo_data,
                           uint32_t node_id, uint32_t out_node_id, uint32_t long_ptr_id)
  {
    const BuildNode &node = build_nodes[node_id];
    if (node.ch_off == 0)
    {
      svo_data[out_node_id] = node.value;
      return;
    }

    uint8_t child_is_leaf = 0;
    uint8_t child_has_data = 0;
    uint32_t active_child_count = 0;
    uint32_t expected_total_child_size = 8;
    for (uint32_t i = 0; i < 8; ++i)
    {
      const BuildNode &child_node = build_nodes[node.ch_off + i];
      const bool active = child_node.total_active_children > 0;
      const bool leaf = child_node.ch_off == 0;
      child_is_leaf |= static_cast<uint8_t>(leaf) << i;
      child_has_data |= static_cast<uint8_t>(active) << i;
      active_child_count += active ? 1 : 0;
      expected_total_child_size += child_node.total_active_children;
    }
    assert(active_child_count > 0);

    const uint32_t out_children_pos = static_cast<uint32_t>(svo_data.size());
    const uint32_t short_ptr = out_children_pos - out_node_id;
    if (short_ptr <= MAX_CHILD_POINTER)
    {
      svo_data[out_node_id] =
          short_ptr | (uint32_t(child_is_leaf) << 16) | (uint32_t(child_has_data) << 24);
    }
    else
    {
      if (long_ptr_id == INVALID_IDX)
        throw std::runtime_error("SVO node needs a far pointer, but no far pointer slot was reserved");

      const uint32_t short_ptr_to_long_ptr = long_ptr_id - out_node_id;
      if (short_ptr_to_long_ptr > MAX_CHILD_POINTER)
        throw std::runtime_error("SVO far pointer slot is too far from its node");

      svo_data[out_node_id] =
          short_ptr_to_long_ptr | IS_FAR_BIT |
          (uint32_t(child_is_leaf) << 16) | (uint32_t(child_has_data) << 24);
      svo_data[long_ptr_id] = out_children_pos;
    }

    const bool allocate_far_pointers = expected_total_child_size > MAX_CHILD_POINTER;
    svo_data.resize(out_children_pos + active_child_count * (1 + allocate_far_pointers));

    uint32_t active_child_id = 0;
    for (uint32_t i = 0; i < 8; ++i)
    {
      const BuildNode &child_node = build_nodes[node.ch_off + i];
      if (child_node.total_active_children == 0)
        continue;

      pack_svo_rec(build_nodes, svo_data, node.ch_off + i,
                   out_children_pos + active_child_id,
                   allocate_far_pointers ? out_children_pos + active_child_count + active_child_id
                                         : INVALID_IDX);
      ++active_child_id;
    }
  }

  static void copy_build_subtree_into(const std::vector<BuildNode> &src_nodes, uint32_t src_node_id,
                                    std::vector<BuildNode> &dst_nodes, uint32_t dst_node_id)
  {
    dst_nodes[dst_node_id] = src_nodes[src_node_id];

    if (src_nodes[src_node_id].ch_off != 0)
    {
      const uint32_t dst_child_offset = static_cast<uint32_t>(dst_nodes.size());
      dst_nodes[dst_node_id].ch_off = dst_child_offset;
      dst_nodes.resize(dst_child_offset + 8);

      for (uint32_t i = 0; i < 8; ++i)
      {
        copy_build_subtree_into(src_nodes, src_nodes[src_node_id].ch_off + i,
                                dst_nodes, dst_child_offset + i);
      }
    }
  }

  static SparseVoxelOctree pack_triangle_tree_as_svo(
      const cmesh4::SimpleMesh &mesh,
      const sdf_converter::PrimitiveListOctree &triangle_tree,
      uint32_t resolution, uint32_t leaf_grid_size,
      uint64_t *surface_voxel_count, uint32_t voxel_value,
      const SolidFillAccel *solid_accel)
  {
    SparseVoxelOctree svo;
    svo.header.max_level_size = resolution;

    std::vector<BuildNode> build_nodes(1);

    const uint32_t root_top_node_size = resolution / leaf_grid_size;
    
    if (triangle_tree.nodes[0].offset == 0)
    {
      build_nodes_from_triangle_tree_rec(mesh, triangle_tree, build_nodes, 0, 0,
                                       uint3(0, 0, 0), root_top_node_size,
                                       leaf_grid_size, resolution, voxel_value,
                                       solid_accel);
    }
    else
    {
      std::array<std::vector<BuildNode>, 8> child_build_nodes;
      
      const uint32_t child_top_node_size = root_top_node_size / 2;
      const uint32_t root_child_offset = triangle_tree.nodes[0].offset;

      #pragma omp parallel for schedule(dynamic)
      for (int i = 0; i < 8; ++i)
      {
        child_build_nodes[i].resize(1);

        build_nodes_from_triangle_tree_rec(mesh, triangle_tree, child_build_nodes[i], root_child_offset + uint32_t(i), 0,
                                          octree_child_code(uint32_t(i)), child_top_node_size, leaf_grid_size,
                                          resolution, voxel_value, solid_accel);
      }

      build_nodes.resize(9);
      build_nodes[0].ch_off = 1;
      build_nodes[0].value = 0;
      build_nodes[0].total_active_children = 0;

      for (uint32_t i = 0; i < 8; ++i)
      {
        copy_build_subtree_into(child_build_nodes[i], 0, build_nodes, 1 + i);
        build_nodes[0].total_active_children += build_nodes[1 + i].total_active_children;
      }

      if (build_nodes[0].total_active_children == 0)
      {
        build_nodes[0].ch_off = 0;
      }
      else
      {
        build_nodes[0].total_active_children += 1;
      }
    }

    if (surface_voxel_count)
      *surface_voxel_count = count_active_leaf_nodes_rec(build_nodes, 0);

    svo.data.resize(1);
    pack_svo_rec(build_nodes, svo.data, 0, 0, INVALID_IDX);
    return svo;
  }

  static float3 default_padding_bounds(uint32_t resolution, uint32_t padding)
  {
    const float pad = 2.0f * float(padding) / float(resolution);
    return float3(-1.0f + pad, 1.0f - pad, 0.0f);
  }
}

bool Voxelizer::mesh_supports_solid_voxelization(const cmesh4::SimpleMesh &mesh,
                                                 bool verbose)
{
  return cmesh4::check_watertight_mesh(mesh, verbose);
}

SparseVoxelOctree Voxelizer::build_sparse_voxel_octree_from_mesh(
    const cmesh4::SimpleMesh &mesh,
    const SparseVoxelOctreeBuildSettings &settings)
{
  if (mesh.TrianglesNum() == 0)
    throw std::invalid_argument("cannot voxelize a mesh with no triangles");

  const uint32_t resolution = round_up_power_of_two(std::max(settings.resolution, 2u));
  const uint32_t leaf_grid_size = std::min(round_up_power_of_two(std::max(settings.leaf_grid_size, 1u)), resolution);
  if (resolution % leaf_grid_size != 0)
    throw std::invalid_argument("voxelization leaf grid size must divide the final resolution");
  const uint32_t top_tree_resolution = resolution / leaf_grid_size;

  bool solid_fill = settings.mode == SparseVoxelizationMode::SOLID;
  if (solid_fill && !mesh_supports_solid_voxelization(mesh, settings.verbose)) {
    solid_fill = false;
    printf("[mesh_voxelizer_svo] solid mode requested, but mesh is not watertight; using surface-only tree\n");
  }

  uint32_t padding = settings.padding;
  if (padding == 0xFFFFFFFFu)
    padding = !solid_fill ? 1u : 0u;
  if (padding * 2u >= resolution)
    throw std::invalid_argument("voxelization padding leaves no space for the mesh");

  cmesh4::SimpleMesh normalized_mesh = mesh;
  const float3 pad_bounds = default_padding_bounds(resolution, padding);
  cmesh4::rescale_mesh(normalized_mesh, float3(pad_bounds.x), float3(pad_bounds.y));

  const uint32_t max_depth = static_cast<uint32_t>(std::ceil(std::log2(top_tree_resolution)));
  const uint32_t thread_count = std::max(1, omp_get_max_threads());
  sdf_converter::PLOSettings tree_settings(max_depth, 0, 1.0f, true, max_depth);
  sdf_converter::PrimitiveListOctree triangle_tree =
      sdf_converter::create_triangle_list_octree(normalized_mesh, tree_settings, thread_count > 1);

  SolidFillAccel solid_accel;
  const SolidFillAccel *solid_accel_ptr = nullptr;
  if (solid_fill)
  {
    const uint32_t solid_bin_count =
        choose_solid_bin_count(top_tree_resolution, resolution);
    solid_accel = build_solid_fill_accel(normalized_mesh, solid_bin_count);
    solid_accel_ptr = &solid_accel;
    if (settings.verbose)
    {
      printf("[mesh_voxelizer_svo] solid fill yz bins: %u x %u, triangle refs: %zu\n",
             solid_accel.bin_count, solid_accel.bin_count,
             solid_accel.triangle_ids.size());
    }
  }

  uint64_t surface_voxel_count = 0;
  SparseVoxelOctree svo =
      pack_triangle_tree_as_svo(normalized_mesh, triangle_tree, resolution,
                                leaf_grid_size, &surface_voxel_count,
                                settings.voxel_value,
                                solid_accel_ptr);

  return svo;
}
