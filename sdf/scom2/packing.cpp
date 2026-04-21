#include "scom_builder.h"
#include "clustering/clustering.h"
#include "scom_utils.h"
#include "sdf/build/sparse_octree_common.h"
#include "utils/common/position_hash.h"
#include "utils/common/stat_box.h"

namespace scom2
{
  constexpr static uint32_t OCTREE_MIN_CHILDREN_COUNT = 4;
  constexpr static uint32_t OCTREE_CHILDREN_COUNT = 8;
  constexpr static uint32_t OCTREE_MAX_CHILDREN_COUNT = 16;
  constexpr static uint32_t BITMASK_MAX_VOXEL_COUNT = 27*3; //3*3*3
  constexpr static uint32_t BITMASK_MAX_DIST_COUNT  = 64*4; //4*4*4

  static uint64_t quantize_all_vals = 0;
  static uint64_t quantize_clamped_vals = 0;

  static uint64_t bytes_node_headers    = 0;
  static uint64_t bytes_node_offsets    = 0;
  static uint64_t bytes_node_child_refs = 0;
  static uint64_t bytes_brick_bitmasks  = 0;
  static uint64_t bytes_brick_distances = 0;

  bool check_settings(const Settings& settings)
  {
    if (settings.bits_per_value < 4 || settings.bits_per_value > 32)
    {
      printf("[scom2::check_settings] bits_per_value must be in range [4, 32]\n");
      return false;
    }

    uint32_t dist_count = (settings.brick_size + 1) * (settings.brick_size + 1) * (settings.brick_size + 1);//WTF
    if (settings.packed_brick_type == PackedBrickType::BITMASK && dist_count > BITMASK_MAX_DIST_COUNT)
    {
      printf("[scom2::check_settings] PackedBrickType::BITMASK is not supported for brick_size > 3\n");
      return false;
    }

    if (settings.packed_brick_type == PackedBrickType::BITMASK && settings.separate_unique_bricks)
    {
      printf("[scom2::check_settings] PackedBrickType::BITMASK is not supported for separate_unique_bricks\n");
      return false;
    }

    return true;
  }

  SCom2Header get_SCom2Header(const Settings& settings, bool DAG_has_channels, unsigned dimension)
  {
    if (!check_settings(settings))
      return get_default_SCom2Header();

    SCom2Header header;

    header.brick_size = settings.brick_size;
    header.v_size = settings.brick_size + 1;
    header.bits_per_value = settings.bits_per_value;
    header.values_per_uint = 32 / settings.bits_per_value;
    header.value_mask = ((uint64_t)1 << settings.bits_per_value) - 1;
    header.dimension = dimension;

    uint32_t dist_count = std::pow(settings.brick_size + 1, dimension);//(settings.brick_size + 1) * (settings.brick_size + 1) * (settings.brick_size + 1);
    if (settings.packed_brick_type == PackedBrickType::BITMASK)
    {
      header.bitmask_len = (dist_count + 31) / 32;
      header.bricks_step = 1;
    }
    else if (settings.packed_brick_type == PackedBrickType::FULL)
    {
      header.bitmask_len = 0;
      header.bricks_step = (dist_count + header.values_per_uint - 1) / header.values_per_uint;
    }
    else
    {
      assert(false);
      return get_default_SCom2Header();
    }

    if (settings.packed_reference_type == PackedReferenceType::SHORT_6_8_18)
    {
      assert(dimension != 4);
      header.child_rot_shift = 26;
      header.child_rot_mask = 0x0000003Fu;
      header.child_add_shift = 18;
      header.child_add_mask = 0x000000FFu;
      header.child_offset_mask = 0x0003FFFFu;
      header.node_offset_mask  = 0x03FFFFFFu;
      header.child_offset_off = 0;
      header.unique_brick_prefix = 0x30 << header.child_rot_shift;
      header.unique_brick_offset_mask = 0x3FFFFFFFu;
    }
    else if (settings.packed_reference_type == PackedReferenceType::LONG_6_8_32)
    {
      assert(dimension != 4);
      header.child_rot_shift = 8;
      header.child_rot_mask = 0x0000003Fu;
      header.child_add_shift = 0;
      header.child_add_mask = 0x000000FFu;
      header.child_offset_mask = 0xFFFFFFFFu;
      header.node_offset_mask  = 0xFFFFFFFFu;
      header.child_offset_off = 1;
      header.unique_brick_prefix = 0x30 << header.child_rot_shift;
      header.unique_brick_offset_mask = 0xFFFFFFFFu;
    }
    else if (settings.packed_reference_type == PackedReferenceType::LONG_6_26_32)
    {
      assert(dimension != 4);
      header.child_rot_shift = 26;
      header.child_rot_mask = 0x0000003Fu;
      header.child_add_shift = 0;
      header.child_add_mask = 0x03FFFFFFu;
      header.child_offset_mask = 0xFFFFFFFFu;
      header.node_offset_mask  = 0xFFFFFFFFu;
      header.child_offset_off = 1;
      header.unique_brick_prefix = 0x30 << header.child_rot_shift;
      header.unique_brick_offset_mask = 0xFFFFFFFFu;
    }
    else if (settings.packed_reference_type == PackedReferenceType::LONG_9_23_32)
    {
      header.child_rot_shift = 23;
      header.child_rot_mask = 0x000001FFu;
      header.child_add_shift = 0;
      header.child_add_mask = 0x007FFFFFu;
      header.child_offset_mask = 0xFFFFFFFFu;
      header.node_offset_mask  = 0xFFFFFFFFu;
      header.child_offset_off = 1;
      header.unique_brick_prefix = 0x180 << header.child_rot_shift;
      header.unique_brick_offset_mask = 0xFFFFFFFFu;
    }
    else
    {
      assert(false);
      return get_default_SCom2Header();
    }

    // if average value transform is disabled, set child_add_mask to 0
    if (settings.bricks_use_average_val_transform == false)
      header.child_add_mask = 0x00000000u;

    if (settings.packed_surface_limit == PackedSurfaceLimit::STRICT)
    {
      header.reference_bits = 4;
      header.reference_mask = 0xFu;
      header.references_per_uint = 8;
    }
    else if (settings.packed_surface_limit == PackedSurfaceLimit::RELAXED)
    {
      header.reference_bits = 8;
      header.reference_mask = 0xFFu;
      header.references_per_uint = 4;
    }
    else
    {
      assert(false);
      return get_default_SCom2Header();
    }

    header.has_channels = DAG_has_channels && settings.support_channels;
    header.has_multi_nodes = settings.support_multi_nodes;
    header.has_surfaces = settings.support_surfaces;

    header.uints_per_link = settings.support_surfaces ? 1 + header.child_offset_off : 1;
    header.children_types_shift = header.dimension < 4 ? 8 : 0;
    header.children_types_mask = header.dimension == 3 ? 0xFFFFu : (header.dimension == 4 ? 0xFFFFFFFFu : 0xFFu);
    header.base_reference_shift = 24;
    header.children_active_bits_shift = header.dimension < 4 ? 24 : 8;
    header.children_active_bits_mask = header.dimension == 3 ? 0xFFu : (header.dimension == 4 ? 0xFFFFu : 0xFu);
    header.references_offset          = settings.support_multi_nodes ? 1 : 0;
    header.links_offset               = settings.support_multi_nodes ? header.references_offset + 8/header.references_per_uint : (header.dimension < 4 ? 1 : 2);
    header.max_surface_count          = settings.support_multi_nodes ? (1 << header.reference_bits) - 1 : std::pow(2, dimension)+1+header.has_channels;
    header.max_surface_count_per_leaf = settings.support_multi_nodes ? (1 << header.reference_bits) - 1 : settings.support_surfaces ? 1 : 0;

    header.bricks_arr_offset = 0;
    header.nodes_arr_offset  = 0;
    header.root_node_off = 0;

    header.max_val = -1;

    return header;
  }

  void find_max_node_level_rec(const SdfDAG &dag, uint32_t node_id, uint32_t level, std::vector<uint32_t> &max_levels_nodes)
  {
    if (level > max_levels_nodes[node_id])
      max_levels_nodes[node_id] = level;
    if (dag.nodes[node_id].children_edges_offset != 0 &&
        level <= sdf_converter::GlobalOctree::MAX_OCTREE_DEPTH)
    {
      for (int i = 0; i < std::pow(dag.header.node_grid_size, dag.header.dim); i++)
      {
        auto off = dag.child_edges[dag.nodes[node_id].children_edges_offset + i].child_offset;
        if (off != 0)
          find_max_node_level_rec(dag, off, level + 1, max_levels_nodes);
      }
    }

    if (level > sdf_converter::GlobalOctree::MAX_OCTREE_DEPTH)
    {
      printf("[find_max_node_level_rec] found node with depth (%d) > MAX_DEPTH (%d). It probably indicates a loop in the DAG (bug in DAG creation)\n",
             level, sdf_converter::GlobalOctree::MAX_OCTREE_DEPTH);
    }
  }

  uint32_t quantize_value(float val, float max_val, uint32_t value_mask)
  {
    float clamped_val = LiteMath::clamp(val, -max_val, max_val);
    uint32_t quantized_val = (uint32_t)(((clamped_val + max_val) / (2*max_val)) * value_mask) & value_mask;
    //printf("val = %f, clamped_val = %f, quantized_val = %u\n", val, clamped_val, quantized_val);

    quantize_all_vals++;
    quantize_clamped_vals += (clamped_val != val);

    return quantized_val;
  }

  /*uint32_t quantize_value(float val, float2 min_max_val, uint32_t value_mask)
  {
    float clamped_val = LiteMath::clamp(val, min_max_val.x, min_max_val.y);
    uint32_t quantized_val = (uint32_t)(((clamped_val - min_max_val.x) / (min_max_val.y - min_max_val.x)) * value_mask) & value_mask;
    
    quantize_all_vals++;
    quantize_clamped_vals += (clamped_val != val);

    return quantized_val;
  }*/

  struct DAGBrickInfo
  {
    uint32_t min_level = 1000;
    uint32_t max_level = 0;
    uint32_t min_rot = 1000;
    uint32_t max_rot = 0;
    float min_add =  1000.0f;
    float max_add = -1000.0f;
    uint32_t links_count = 0;
  };

  void unpack_distances(const SCom2Header &header, float max_val, const uint32_t *brick_packed, float *out_distances);
  void pack_bricks_full(const Settings& settings, const SCom2Header &header, const SdfDAG &dag, 
                        const std::vector<DAGBrickInfo> &brick_infos,
                        std::vector<uint32_t> &brick_id_to_offset, std::vector<uint32_t> &bricks)
  {
    std::vector<int4> rot_modifiers;
    //initialize_rot_modifiers(rot_modifiers, header.v_size);
    auto subgroup = scom2::create_subgroup((scom2::TransformSubgroup) dag.header.transform_subgroup, header.v_size, header.dimension);
    //printf("size == %u\n", (unsigned)subgroup->elements.size());
    uint32_t dist_count = std::pow(header.v_size, header.dimension);//header.v_size * header.v_size * header.v_size;
    uint32_t brick_count = dag.distances.size() / dist_count;
    uint32_t uint_per_brick = (dist_count + header.values_per_uint - 1) / header.values_per_uint;

    bricks.resize(uint_per_brick*brick_count, 0);

    uint32_t cur_brick_n = 0;
    for (uint32_t is_unique = 0; is_unique <=1; is_unique++)
    {
      for (uint32_t i = 0; i < brick_count; i++)
      {
        if ((is_unique == 0 && brick_infos[i].links_count <= 1) || (is_unique == 1 && brick_infos[i].links_count > 1))
          continue;
        
        bool unique_transform = settings.separate_unique_bricks && is_unique;
        if (unique_transform)
        {
          assert((brick_infos[i].max_add - brick_infos[i].min_add) < 1e-6f);
          assert(brick_infos[i].max_rot == brick_infos[i].min_rot);
        }

        float def_max = get_max_sdf_val(powf(2, brick_infos[i].max_level), header.dimension);
        for (uint32_t j = 0; j < dist_count; j++)
        {
          // int4 v_pos = int4(j/(header.v_size*header.v_size), (j/header.v_size)%header.v_size, j%header.v_size, 1);
          // if (header.dimension == 4)
          //   v_pos = int4(j/(header.v_size*header.v_size)%header.v_size, (j/header.v_size)%header.v_size, j%header.v_size, j/(header.v_size*header.v_size*header.v_size));
          // else if (header.dimension == 2)
          //   v_pos = int4(1, (j/header.v_size), j%header.v_size, 1);
          uint32_t rot_id = unique_transform ? brick_infos[i].min_rot : 0;
          uint32_t v_idx = i * dist_count + subgroup->elements[rot_id][j];// + dot(v_pos, rot_modifiers[rot_id]);
          float val = unique_transform ? dag.distances[v_idx] + brick_infos[i].min_add : dag.distances[v_idx];
          uint32_t val_quant;
          if (header.max_val > 0)
            val_quant = quantize_value(val, header.max_val, header.value_mask);
          else
            val_quant = quantize_value(val, def_max, header.value_mask);
          bricks[uint_per_brick*cur_brick_n + j/header.values_per_uint] |= val_quant << ((j%header.values_per_uint)*header.bits_per_value);
        }

        // float vals_check[64];
        // unpack_distances(header, def_max, bricks.data() + uint_per_brick*i, vals_check);
        // printf("before %f %f %f %f %f %f %f %f\n", 
        //        dag.distances[i * dist_count + 0], dag.distances[i * dist_count + 1], dag.distances[i * dist_count + 2], 
        //        dag.distances[i * dist_count + 3], dag.distances[i * dist_count + 4], dag.distances[i * dist_count + 5],
        //        dag.distances[i * dist_count + 6], dag.distances[i * dist_count + 7]);
        // printf("after  %f %f %f %f %f %f %f %f\n", 
        //        vals_check[0], vals_check[1], vals_check[2], vals_check[3], vals_check[4], vals_check[5],
        //        vals_check[6], vals_check[7]);

        brick_id_to_offset[i] = cur_brick_n;
        cur_brick_n++;
        bytes_brick_distances += sizeof(uint32_t)*uint_per_brick;
        bytes_brick_bitmasks  += 0;
      }
    }
  }

  void pack_bricks_with_bitmask(const Settings& settings, const SCom2Header &header, const SdfDAG &dag, 
                                const std::vector<DAGBrickInfo> &brick_infos,
                                std::vector<uint32_t> &brick_id_to_offset, std::vector<uint32_t> &bricks)
  {
    bool voxel_is_active[BITMASK_MAX_DIST_COUNT];
    bool dist_is_used[BITMASK_MAX_DIST_COUNT];
    uint32_t active_dist_list[BITMASK_MAX_DIST_COUNT];

    uint32_t dist_count = std::pow(header.v_size, header.dimension);//header.v_size * header.v_size * header.v_size;
    uint32_t brick_count = dag.distances.size() / dist_count;
    uint32_t max_uint_per_brick = (dist_count + header.values_per_uint - 1) / header.values_per_uint + header.bitmask_len;

    bricks.reserve(max_uint_per_brick*brick_count);

    for (uint32_t i = 0; i < brick_count; i++)
    {
      //active distances -> active voxels -> used distances
      sdf_converter::find_active_voxels_and_distances(header.brick_size, dag.distances.data() + i*dist_count, 
                                                      voxel_is_active, dist_is_used, 
                                                      brick_infos[i].min_add, brick_infos[i].max_add);

      //create bitmask and fill active_dist_list
      uint32_t active_dist_count = 0;
      uint64_t bitmask0 = 0;
      uint64_t bitmask1 = 0;
      for (uint32_t j = 0; j < dist_count; j++)
      {
        if (j < 32)
          bitmask0 |= (uint32_t)dist_is_used[j] << j;
        else
          bitmask1 |= (uint32_t)dist_is_used[j] << (j-32);
        if (dist_is_used[j])
          active_dist_list[active_dist_count++] = j;
      }

      //pack active distances
      float def_max = get_max_sdf_val(powf(2, brick_infos[i].max_level), header.dimension);
      uint32_t uint_per_brick = (active_dist_count + header.values_per_uint - 1) / header.values_per_uint + header.bitmask_len;
      uint32_t off = bricks.size();
      bricks.resize(bricks.size() + uint_per_brick);
      bricks[off + 0] = bitmask0;
      if (header.bitmask_len > 1)
        bricks[off + 1] = bitmask1;
      for (uint32_t j = 0; j < active_dist_count; j++)
      {
        uint32_t val_quant;
        if (header.max_val > 0)
          val_quant = quantize_value(dag.distances[i*dist_count + active_dist_list[j]], header.max_val, header.value_mask);
        else
          val_quant = quantize_value(dag.distances[i*dist_count + active_dist_list[j]], def_max, header.value_mask);
        bricks[off + header.bitmask_len + j/header.values_per_uint] |= val_quant << ((j%header.values_per_uint)*header.bits_per_value);
      }
      brick_id_to_offset[i] = off;

      bytes_brick_distances += sizeof(uint32_t)*(uint_per_brick-header.bitmask_len);
      bytes_brick_bitmasks  += sizeof(uint32_t)*header.bitmask_len;
    }

    bricks.shrink_to_fit();
  }


  void pack_brick_link(const SCom2Header &header, uint32_t rot, uint32_t offset,
                       float add, uint32_t level, bool is_unique, uint32_t *out_packed_link)
  {
    out_packed_link[0] = 0;
    out_packed_link[header.child_offset_off] = 0;

    if (is_unique)
    {
      assert((offset & header.unique_brick_offset_mask) == offset);

      out_packed_link[0] |= header.unique_brick_prefix;
      out_packed_link[header.child_offset_off] |= offset;
    }
    else
    {
      assert((rot & header.child_rot_mask) == rot);
      assert((offset & header.child_offset_mask) == offset);

      float def_max = get_max_sdf_val(powf(2, level), header.dimension);
      uint32_t add_val_quant;
      if (header.max_val > 0)
        add_val_quant = quantize_value(add, header.max_val, header.child_add_mask);
      else
        add_val_quant = quantize_value(add, def_max, header.child_add_mask);

      out_packed_link[0] |= (add_val_quant << header.child_add_shift) | (rot << header.child_rot_shift);
      out_packed_link[header.child_offset_off] |= offset;
    }
  }

  void pack_node_link(const SCom2Header &header, uint32_t rot, uint32_t offset,
                      uint32_t *out_packed_link)
  {
    assert((rot & header.child_rot_mask) == rot);
    assert((offset & header.node_offset_mask) == offset); 

    out_packed_link[0] = 0;
    out_packed_link[header.child_offset_off] = 0;

    out_packed_link[0] |= (rot << header.child_rot_shift);
    out_packed_link[header.child_offset_off] |= offset;
  }

  void pack_nodes_rec(const Settings& settings, const SCom2Header &header, const SdfDAG &dag,
                      const std::vector<uint32_t> &brick_id_to_offset, const std::vector<DAGBrickInfo> &brick_infos,
                      uint32_t node_id, uint32_t level,
                      std::vector<uint32_t> &node_id_to_offset, std::vector<uint32_t> &nodes)
  {
    if (dag.nodes[node_id].children_edges_offset == 0)
      return;

    uint32_t octree_children_cnt = std::pow(2, header.dimension);

    for (int i = 0; i < octree_children_cnt; i++)
    {
      auto off = dag.child_edges[dag.nodes[node_id].children_edges_offset + i].child_offset;
      if (off != 0 &&
          dag.nodes[off].children_edges_offset != 0 &&
          node_id_to_offset[off] == INVALID_IDX)
      {
        pack_nodes_rec(settings, header, dag, brick_id_to_offset, brick_infos, off, level + 1, node_id_to_offset, nodes);
      }
    }

    const uint32_t ch_off = dag.nodes[node_id].children_edges_offset;
    const uint32_t offset = nodes.size();
    const uint32_t dist_count = std::pow(header.v_size, header.dimension);//header.v_size * header.v_size * header.v_size;

    uint32_t base_type = SCOM2_CHILD_EMPTY;
    uint32_t children_types_bits = 0;
    uint32_t base_surf_end = 0;
    uint32_t surf_ends[OCTREE_MAX_CHILDREN_COUNT] = {0};
    uint32_t surf_end = 0;

    node_id_to_offset[node_id] = offset;
    nodes.resize(nodes.size() + header.links_offset, 0u);

    //add link to the bricks of the node itself
    {
      const uint32_t full_surf_count = DAG_extract_count(dag.nodes[node_id].voxel_count_flags);
      const uint32_t surf_count = std::min(full_surf_count, header.max_surface_count_per_leaf);
      const uint32_t has_channels = ((surf_count > 0 || header.has_surfaces == 0) && header.has_channels > 0) ? 1 : 0; //TODO: fix, we can have valid channels and no surfaces
      nodes.resize(nodes.size() + (surf_count+has_channels)*header.uints_per_link, 0u);

      if (full_surf_count > header.max_surface_count_per_leaf && header.has_surfaces > 0)
      {
        printf("[scom2::pack_nodes_rec] Surface count(%d) exceeded max_surface_count_per_leaf(%d). Check DAG pre-packing processing\n",
                full_surf_count, header.max_surface_count_per_leaf);
      }
      
      for (uint32_t i = 0; i < surf_count; i++)
      {
        const auto &d_edge = dag.data_edges[dag.nodes[node_id].data_edges_offset + i];
        uint32_t brick_id = d_edge.data_offset / dist_count;
        pack_brick_link(header, d_edge.rotation_id, brick_id_to_offset[brick_id], d_edge.add, level, 
                        (brick_infos[brick_id].links_count == 1) && settings.separate_unique_bricks,
                        nodes.data() + offset + header.links_offset + (surf_end + i)*header.uints_per_link);
      }

      if (has_channels && (header.has_surfaces || DAG_node_is_full(dag.nodes[node_id].voxel_count_flags)))
      {
        pack_node_link(header, dag.nodes[node_id].channels_edge.rotation_id, dag.nodes[node_id].channels_edge.child_offset,
                       nodes.data() + offset + header.links_offset + (surf_end + surf_count)*header.uints_per_link);
      }

      surf_end += surf_count+has_channels;
      base_surf_end = surf_end;
      if ((surf_count == 0 && header.has_surfaces) || (!header.has_surfaces && !DAG_node_is_full(dag.nodes[node_id].voxel_count_flags)))
        base_type = SCOM2_CHILD_EMPTY;
      else if (header.has_surfaces && DAG_extract_is_surface(dag.nodes[node_id].voxel_count_flags))
        base_type = SCOM2_CHILD_LEAF_SURFACE;
      else
        base_type = SCOM2_CHILD_LEAF_VOLUME;
    }

    uint32_t channels_local_offset = surf_end, channels_local_count = 0;

    for (int i = 0; i < octree_children_cnt; i++)
    {
      auto off = dag.child_edges[ch_off + i].child_offset;
      if (off != 0)
      {
        if (dag.nodes[off].children_edges_offset == 0)
        {
          const uint32_t full_surf_count = DAG_extract_count(dag.nodes[off].voxel_count_flags);
          uint32_t surf_count = std::min(full_surf_count, std::min(header.max_surface_count_per_leaf, header.max_surface_count - surf_end));
          if ((full_surf_count != 0 && surf_count != 0) || !header.has_surfaces)
          {
            uint32_t has_channels = ((full_surf_count > 0 || header.has_surfaces == 0) && header.has_channels > 0) ? 1 : 0;
            nodes.resize(nodes.size() + (surf_count+has_channels)*header.uints_per_link, 0u);
            channels_local_offset += surf_count;
            if (has_channels)
            {
              ++channels_local_count;
            }
          }
        }
        else
        {
          uint32_t surf_count = 1;
          nodes.resize(nodes.size() + surf_count*header.uints_per_link, 0u);
          channels_local_offset += surf_count;
        }
      }
    }

    for (int i = 0; i < octree_children_cnt; i++)
    {
      uint32_t is_active = 0;
      uint32_t type = 0;
      uint32_t surf_count = 0;
      uint32_t has_channels = 0;

      auto off = dag.child_edges[ch_off + i].child_offset;
      if (off == 0 || (!header.has_surfaces && !DAG_node_is_full(dag.nodes[off].voxel_count_flags))) //no child
      {
        is_active = 0;
        type = SCOM2_CHILD_EMPTY;
        surf_count = 0;
        has_channels = 0;
      }
      else if (dag.nodes[off].children_edges_offset == 0) //child is leaf
      {
        const uint32_t full_surf_count = DAG_extract_count(dag.nodes[off].voxel_count_flags);
        surf_count = std::min(full_surf_count, std::min(header.max_surface_count_per_leaf, header.max_surface_count - surf_end));
        has_channels = ((surf_count > 0 || header.has_surfaces == 0) && header.has_channels > 0) ? 1 : 0;
        if (full_surf_count == 0 && header.has_surfaces > 0) // child is present, but empty, it is ok if there is some channel data
        {
          // no channel data or channels not supported, so this node is completely redundant, might be some mistake in DAG building settings
          if (header.has_channels == 0 || dag.nodes[off].channels_edge.child_offset == 0)
            printf("[scom2::pack_nodes_rec] Empty leaf. Check DAG building settings and pruning\n");
          is_active = 0;
          type = SCOM2_CHILD_EMPTY;
          surf_count = 0;
          has_channels = 0;
        }
        else if (surf_count == 0 && header.has_surfaces > 0) // surface limit reached. We must prune the surfaces before packing, just skip here
        {
          printf("[scom2::pack_nodes_rec] Total surface count (%d) exceeded max_surface_count(%d). Check DAG pre-packing processing\n",
                 surf_end + full_surf_count, header.max_surface_count);
          is_active = 0;
          type = SCOM2_CHILD_EMPTY;
          surf_count = 0;
          has_channels = 0;
        }
        else // there are surfaces to pack
        {
          if (full_surf_count > header.max_surface_count_per_leaf && header.has_surfaces > 0)
          {
            printf("[scom2::pack_nodes_rec] Surface count(%d) exceeded max_surface_count_per_leaf(%d). Check DAG pre-packing processing\n",
                    full_surf_count, header.max_surface_count_per_leaf);
          }
          if (full_surf_count > header.max_surface_count - surf_end && header.has_surfaces > 0)
          {
            printf("[scom2::pack_nodes_rec] Total surface count (%d) exceeded max_surface_count(%d). Check DAG pre-packing processing\n",
                  surf_end + full_surf_count, header.max_surface_count);
          }
          
          is_active = 1;
          if (header.has_surfaces > 0)
            type = DAG_extract_is_surface(dag.nodes[off].voxel_count_flags) ? SCOM2_CHILD_LEAF_SURFACE : SCOM2_CHILD_LEAF_VOLUME;
          else
            type = SCOM2_CHILD_LEAF_VOLUME;

          //nodes.resize(nodes.size() + (surf_count+has_channels)*header.uints_per_link, 0u);
          for (uint32_t j = 0; j < surf_count; j++)
          {
            const auto &d_edge = dag.data_edges[dag.nodes[off].data_edges_offset + j];
            uint32_t brick_id = d_edge.data_offset / dist_count;
            pack_brick_link(header, d_edge.rotation_id, brick_id_to_offset[brick_id], d_edge.add, level+1, 
                            (brick_infos[brick_id].links_count == 1) && settings.separate_unique_bricks,
                            nodes.data() + offset + header.links_offset + (surf_end + j)*header.uints_per_link);
          }

          if (has_channels)
          {
            if (header.has_multi_nodes || !header.has_surfaces)
            {
              pack_node_link(header, dag.nodes[off].channels_edge.rotation_id, dag.nodes[off].channels_edge.child_offset,
                             nodes.data() + offset + header.links_offset + (surf_end + surf_count)*header.uints_per_link);
            }
            else
            {
              pack_node_link(header, dag.nodes[off].channels_edge.rotation_id, dag.nodes[off].channels_edge.child_offset,
                             nodes.data() + offset + header.links_offset + (channels_local_offset++)*header.uints_per_link);
            }
          }
        }
      }
      else //child is node
      {
        if (surf_end >= header.max_surface_count)
        {
          printf("[scom2::pack_nodes_rec] Surface limit reached on node. PANIC! Check DAG pre-packing processing\n");
          return;
        }

        is_active = 1;
        type = SCOM2_CHILD_NODE;
        surf_count = 1;
        has_channels = 0;

        auto node_off = node_id_to_offset[off];
        //printf("off - %d, node_off - %d\n", off, node_off);
        assert(node_off != INVALID_IDX);
        //nodes.resize(nodes.size() + surf_count*header.uints_per_link, 0u);
        pack_node_link(header, dag.child_edges[ch_off + i].rotation_id, node_off, nodes.data() + offset + header.links_offset + surf_end*header.uints_per_link);
      }
      
      if (header.has_multi_nodes || !header.has_surfaces)
      {
        surf_end += surf_count + has_channels;
      }
      else
        surf_end += surf_count;

      children_types_bits     |= type << (i*SCOM2_CHILD_TYPE_BITS);
      surf_ends[i] = surf_end;
    }

    if (!header.has_multi_nodes && header.has_surfaces)
      surf_end += channels_local_count;

    if (header.has_multi_nodes)
    {
      // store base reference + references for children if multi-nodes are supported
      nodes[offset] = base_type | (children_types_bits << header.children_types_shift) | (base_surf_end << header.base_reference_shift);

      for (int i = 0; i < octree_children_cnt; i++)
        nodes[offset + header.references_offset + i/header.references_per_uint] = 0;
      for (int i = 0; i < octree_children_cnt; i++)
      {
        nodes[offset + header.references_offset + i/header.references_per_uint] |= surf_ends[i] << ((i % header.references_per_uint) * header.reference_bits);
      }
    }
    else
    {
      // store children_active bits instead of base_reference
      // other references are not needed, as we can infer them from these bits + base_type
      uint32_t children_active = 0;
      for (int i = 0; i < octree_children_cnt; i++)
      {
        uint32_t type = (children_types_bits >> (i*SCOM2_CHILD_TYPE_BITS)) & SCOM2_CHILD_TYPE_MASK;
        children_active |= (type != SCOM2_CHILD_EMPTY) << i;
      }
      if (header.dimension < 4)
      {
        nodes[offset] = base_type | (children_types_bits << header.children_types_shift) | (children_active << header.children_active_bits_shift);
      }
      else
      {
        nodes[offset] = base_type | (children_active << header.children_active_bits_shift);
        nodes[offset + header.links_offset - 1] = children_types_bits << header.children_types_shift;
      }
    }//FIX MASKS FOR 4D TREE


    bytes_node_headers += sizeof(uint32_t);
    if (header.has_multi_nodes)
      bytes_node_offsets += sizeof(uint32_t)*(header.links_offset - header.references_offset);
    bytes_node_child_refs += sizeof(uint32_t)*(surf_end * header.uints_per_link);
  }

  void pack_SCom2(const Settings& settings, const SdfDAG &dag, SCom2Tree &out_tree)
  {
    assert(dag.header.brick_pad == 0);
    assert(dag.header.node_grid_size == 2);
    assert(dag.header.dim == 3 || !settings.support_multi_nodes);

    //only default transform subgroup is supported
    assert(dag.header.transform_subgroup == 0);

    quantize_all_vals = 0;
    quantize_clamped_vals = 0;
    bytes_node_headers = 0;
    bytes_node_offsets = 0;
    bytes_node_child_refs = 0;
    bytes_brick_bitmasks = 0;
    bytes_brick_distances = 0;

    std::vector<uint32_t> max_levels_nodes(dag.nodes.size(), 0);
    find_max_node_level_rec(dag, 0, 0, max_levels_nodes);

    SCom2Header header = get_SCom2Header(settings, (dag.point_channels.size() + dag.voxel_channels.size()) > 0, dag.header.dim);

    static_assert(sizeof(header.user_params) == sizeof(dag.header.user_params), "sizeof(header.user_params) == sizeof(dag.header.user_params)");
    for (int i=0;i<sizeof(header.user_params)/sizeof(header.user_params[0]);i++)
      header.user_params[i] = dag.header.user_params[i];
    header.mat_id_off = dag.header.mat_id_off;
    header.tex_id_off = dag.header.tex_id_off;
    header.all_float_tex_id_off = dag.header.all_float_tex_id_off;
    header.all_int_mat_id_off = dag.header.all_int_mat_id_off;

    header.max_depth = calculate_DAG_max_depth_rec(dag, 0);

    if (settings.custom_max_val)
    {
      header.max_val = std::max(std::abs(*std::min_element(dag.distances.begin(), dag.distances.end())), 
                                std::abs(*std::max_element(dag.distances.begin(), dag.distances.end())));
    }

    out_tree.header = header;

    out_tree.point_channels = dag.point_channels;
    out_tree.voxel_channels = dag.voxel_channels;

    uint32_t dist_count = std::pow(header.v_size, header.dimension);//header.v_size * header.v_size * header.v_size;
    uint32_t brick_count = dag.distances.size() / dist_count;
    std::vector<DAGBrickInfo> brick_infos(brick_count);
    for (int i = 0; i < dag.nodes.size(); i++)
    {
      const auto &n = dag.nodes[i];
      if (DAG_extract_count(n.voxel_count_flags) > 0 && n.data_edges_offset != 0)
      {
        for (int j = 0; j < DAG_extract_count(n.voxel_count_flags); j++)
        {
          uint32_t brick_id = dag.data_edges[n.data_edges_offset + j].data_offset / dist_count;
          brick_infos[brick_id].max_level = std::max(brick_infos[brick_id].max_level, max_levels_nodes[i]);
          brick_infos[brick_id].min_level = std::min(brick_infos[brick_id].min_level, max_levels_nodes[i]);
          brick_infos[brick_id].min_rot = std::min(brick_infos[brick_id].min_rot, dag.data_edges[n.data_edges_offset + j].rotation_id);
          brick_infos[brick_id].max_rot = std::max(brick_infos[brick_id].max_rot, dag.data_edges[n.data_edges_offset + j].rotation_id);
          brick_infos[brick_id].min_add = std::min(brick_infos[brick_id].min_add, dag.data_edges[n.data_edges_offset + j].add);
          brick_infos[brick_id].max_add = std::max(brick_infos[brick_id].max_add, dag.data_edges[n.data_edges_offset + j].add);
          brick_infos[brick_id].links_count++;
        }
      }
    }

    uint32_t unique_bricks = 0;
    uint32_t shared_bricks = 0;
    uint32_t unused_bricks = 0;
    uint32_t level_shared_bricks = 0;
    for (const auto &bi : brick_infos)
    {
      unique_bricks += bi.links_count == 1;
      shared_bricks += bi.links_count > 1;
      unused_bricks += (bi.links_count == 0);
      level_shared_bricks += (bi.links_count >= 1 && bi.max_level != bi.min_level);
    }

    printf("clamped %zu/%zu values\n", quantize_clamped_vals, quantize_all_vals);
    printf("DAG has %d unique and %d shared bricks\n", unique_bricks, shared_bricks);
    if (unused_bricks > 0)
      printf("[pack_SCom2]Warining: DAG has %d unused bricks\n", unused_bricks);
    if (level_shared_bricks > 0)
      printf("[pack_SCom2]Warining: DAG %d level-shared bricks\n", level_shared_bricks);

    std::vector<uint32_t> brick_id_to_offset(brick_count, 0);
    if (settings.packed_brick_type == PackedBrickType::FULL)
      pack_bricks_full(settings, header, dag, brick_infos, brick_id_to_offset, out_tree.bricks);
    else
      pack_bricks_with_bitmask(settings, header, dag, brick_infos, brick_id_to_offset, out_tree.bricks);

    std::vector<uint32_t> node_id_to_offset(dag.nodes.size(), INVALID_IDX);
    pack_nodes_rec(settings, header, dag, brick_id_to_offset, brick_infos, 0, 0, node_id_to_offset, out_tree.nodes);
    out_tree.header.root_node_off = node_id_to_offset[0];
    
    if (true)
    {
      uint64_t total_channels_bytes = 0;
      for (const auto &ch : dag.point_channels)
        total_channels_bytes += ch.data_f.size()*sizeof(float) + ch.data_fp8.size()*sizeof(uint8_t) + ch.data_i.size()*sizeof(int32_t);
      for (const auto &ch : dag.voxel_channels)
        total_channels_bytes += ch.data_f.size()*sizeof(float) + ch.data_fp8.size()*sizeof(uint8_t) + ch.data_i.size()*sizeof(int32_t);

      printf("packed %8d bricks: %7.2f MB -> %7.2f MB\n", brick_count, 
            sizeof(float)*dag.distances.size()/1024.0f/1024.0f,
            sizeof(uint32_t)*out_tree.bricks.size()/1024.0f/1024.0f);

      printf("packed %8d nodes:  %7.2f MB -> %7.2f MB\n", (int)dag.nodes.size(), 
            (sizeof(SdfDAGNode)*dag.nodes.size() + sizeof(SdfDAGChildEdge)*dag.child_edges.size() + 
            sizeof(SdfDAGDataEdge)*dag.data_edges.size())/1024.0f/1024.0f,
            sizeof(uint32_t)*out_tree.nodes.size()/1024.0f/1024.0f);

      printf("channels:                             %7.2f MB\n", total_channels_bytes/1024.0f/1024.0f);

      printf("%.2f MB total\n", (total_channels_bytes + sizeof(uint32_t)*out_tree.bricks.size() + 
             sizeof(uint32_t)*out_tree.nodes.size())/1024.0f/1024.0f);

      printf("bytes node headers:   %11zu\n", bytes_node_headers);
      printf("bytes node offsets:   %11zu\n", bytes_node_offsets);
      printf("bytes node child refs:%11zu\n", bytes_node_child_refs);
      printf("bytes brick bitmasks: %11zu\n", bytes_brick_bitmasks);
      printf("bytes brick distances:%11zu\n", bytes_brick_distances);
      printf("bytes channels       :%11zu\n", total_channels_bytes);
    }
  }

  #ifdef _MSC_VER
  #define bitCount(x) __popcnt(x)
  #else
  #define bitCount(x) __builtin_popcount(x)
  #endif

  void unpack_distances(const SCom2Header &header, float max_val, const uint32_t *brick_packed, float *out_distances)
  {
    uint32_t dist_count = std::pow(header.v_size, header.dimension);//header.v_size * header.v_size * header.v_size;

    if (header.bitmask_len == 0) //PackedBrickType::FULL
    {
      for (uint32_t i = 0; i < dist_count; i++)
      {
        uint32_t p_val = brick_packed[i/header.values_per_uint];
        uint32_t p_off = (i%header.values_per_uint)*header.bits_per_value;
        if (header.max_val > 0)
          out_distances[i] = header.max_val*(2.0f*((p_val >> p_off)&header.value_mask) / float(header.value_mask) - 1);
        else
          out_distances[i] = max_val*(2.0f*((p_val >> p_off)&header.value_mask) / float(header.value_mask) - 1);
      }
    }
    else //PackedBrickType::BITMASK
    {
      for (uint32_t i = 0; i < dist_count; i++)
      {
        uint32_t active = i >= 32 ? (brick_packed[1] & (1<<(i-32))) : (brick_packed[0] & (1<<i));
        if (active == 0)
        {
          out_distances[i] = 1000;
        }
        else
        {
          uint32_t bitmask_0 = i >= 32 ? brick_packed[0] : (brick_packed[0] & ((1<<i)-1));
          uint32_t bitmask_1 = i >= 32 ? (brick_packed[1] & ((1<<(i-32))-1)) : 0;
          uint32_t off = bitCount(bitmask_0)+bitCount(bitmask_1);
          uint32_t p_val = brick_packed[header.bitmask_len + off/header.values_per_uint];
          uint32_t p_off = (off%header.values_per_uint)*header.bits_per_value;
          if (header.max_val > 0)
            out_distances[i] = header.max_val*(2.0f*((p_val >> p_off)&header.value_mask) / float(header.value_mask) - 1);
          else
            out_distances[i] = max_val*(2.0f*((p_val >> p_off)&header.value_mask) / float(header.value_mask) - 1);
        }
      }

      //scan near distances to find sign of missing distances
      //sdf_converter::print_grid(header.brick_size, 0, out_distances);
      bool has_changes = true;
      while (has_changes)
      {
        has_changes = false;
        for (uint32_t i = 0; i < dist_count; i++)
        {
          if (out_distances[i] == 1000)
          {
            int4 pos = int4(i/(header.v_size*header.v_size)%header.v_size, i/header.v_size%header.v_size, i%header.v_size, i/(header.v_size*header.v_size*header.v_size));
            float min_val = 1000.0f;
            for (int dx = -std::min(pos.x, 1); dx <= std::min((int)header.v_size - pos.x - 1, 1); ++dx)
            {
              for (int dy = -std::min(pos.y, 1); dy <= std::min((int)header.v_size - pos.y - 1, 1); ++dy)
              {
                for (int dz = -std::min(pos.z, 1); dz <= std::min((int)header.v_size - pos.z - 1, 1); ++dz)
                {
                  for (int dw = -std::min(pos.w, 1); dw <= std::min((int)header.v_size - pos.w - 1, 1); ++dw)
                  {
                    int4 p = pos + int4(dx, dy, dz, dw);
                    min_val = std::min(min_val, out_distances[p.x*header.v_size*header.v_size + p.y*header.v_size + p.z + p.w*header.v_size*header.v_size*header.v_size]);
                  }
                }
              }
            }
            if (min_val < 0)
            {
              out_distances[i] = -1000.0f;
              has_changes = true;
            }
          }
        }
      } //while

    }
  }

  struct CodeSegment
  {
    std::string name = "???";
    uint32_t value = 0;
    uint32_t bits  = 1;
    uint32_t elems = 1;
  };

  struct CodeBlock
  {
    CodeBlock() = default;
    explicit CodeBlock(const std::string &a_name) : name(a_name) {}
    std::string name = "???";
    std::vector<CodeSegment> segments;
  };

  uint32_t mask_to_bits(uint32_t mask)
  {
    uint32_t bits = 0;
    while (mask)
    {
      bits++;
      mask >>= 1;
    }
    return bits;
  }

  void print_blocks(std::vector<CodeBlock> blocks)
  {
    std::string block_names, segment_names, bits, values;
    for (auto &block : blocks)
    {
      if (block.segments.empty())
        continue;
      uint32_t total_bits = 0;
      for (const auto &segment : block.segments)
        total_bits += segment.bits;
      if (total_bits % 32 != 0)
      {
        block.segments.push_back(CodeSegment{"pad", 0, 32 - total_bits % 32, 1});
      }
      block_names   += "|";
      uint32_t prev_size = bits.size();
      for (const auto &segment : block.segments)
      {
        segment_names += "|";
        bits          += "|";
        values        += "|";
        uint32_t s_prev_size = bits.size();
        for (int i = 0; i < segment.elems; i++)
        {
          uint32_t bits_per_elem = segment.bits / segment.elems;
          uint32_t elem_mask = bits_per_elem == 32 ? 0xFFFFFFFFu : (1 << bits_per_elem) - 1;
          uint32_t val = (segment.value >> (bits_per_elem * i)) & elem_mask;
          char bits_inv[33];
          for (uint32_t j = 0; j < bits_per_elem; j++)
          {
            bits_inv[bits_per_elem - j - 1] = (val & (1 << j)) ? '1' : '0';
          }
          bits_inv[bits_per_elem] = '\0';
          bits += bits_inv;
          for (uint32_t j = 0; j < bits_per_elem; j++)
          {
            bits_inv[bits_per_elem - j - 1] = (val == 0 && j > 0) ? ' ' : '0' + (char)(val % 10);
            val /= 10;
          }
          bits_inv[bits_per_elem] = '\0';
          values += bits_inv;
        }
        while (bits.size() < s_prev_size + segment.name.size() + 2)
        {
          bits += " ";
          values += " ";
        }
        uint32_t segment_len = bits.size() - s_prev_size;
        uint32_t segment_left_pad = (segment_len - segment.name.size()) / 2;
        uint32_t segment_right_pad = segment_len - segment.name.size() - segment_left_pad;
        for (uint32_t i = 0; i < segment_left_pad; i++)
        {
          segment_names += " ";
        }
        segment_names += segment.name;
        for (uint32_t i = 0; i < segment_right_pad; i++)
        {
          segment_names += " ";
        }

        segment_names   += "|";
        bits            += "|";
        values          += "|";
      }
      while (bits.size() < prev_size + block.name.size() + 4)
      {
        segment_names += " ";
        bits += " ";
        values += " ";
      }
      uint32_t block_len = bits.size() - prev_size;
      uint32_t block_left_pad = (block_len - block.name.size()) / 2;
      uint32_t block_right_pad = block_len - block.name.size() - block_left_pad;
      for (uint32_t i = 1; i < block_left_pad; i++)
      {
        block_names += " ";
      }
      block_names += block.name;
      for (uint32_t i = 0; i < block_right_pad-1; i++)
      {
        block_names += " ";
      }
      block_names   += "|";
    }
    printf("%s\n%s\n%s\n%s\n\n", block_names.c_str(), segment_names.c_str(), bits.c_str(), values.c_str());
  }

  CodeMap<uint32_t> ref_codes;

  void unpack_SCom2_rec(const SCom2Tree &in_tree, SdfDAG &out_dag, std::vector<uint32_t> &brick_offset_to_id,
                        std::vector<uint32_t> &node_offset_to_id, uint32_t node_off, uint32_t level, bool verbose)
  {
    CodeBlock header_blk("header");
    CodeBlock values_blk("values");
    CodeBlock children_blk("children");

    uint32_t dist_count = std::pow(in_tree.header.v_size, in_tree.header.dimension);//in_tree.header.v_size * in_tree.header.v_size * in_tree.header.v_size;
    float max_val = get_max_sdf_val(powf(2, level), in_tree.header.dimension);
    uint32_t n_off = out_dag.nodes.size();
    uint32_t che_off = out_dag.child_edges.size();
    uint32_t de_off = out_dag.data_edges.size();

    uint32_t octree_children_cnt = std::pow(2, in_tree.header.dimension);

    NodeHeadUnpacked head_unpacked = unpack_node_head(in_tree.header, in_tree.nodes[node_off], in_tree.nodes[node_off + in_tree.header.links_offset - 1]);
    header_blk.segments.push_back({"type", head_unpacked.base_type, mask_to_bits(0xFF), 1});
    header_blk.segments.push_back({"children_types", head_unpacked.children_types, mask_to_bits(0xFFFF), octree_children_cnt});
    if (in_tree.header.has_multi_nodes)
      header_blk.segments.push_back({"l_end", head_unpacked.base_links_end, mask_to_bits(in_tree.header.reference_mask), 1});
    else
      header_blk.segments.push_back({"ch_active", head_unpacked.children_active, octree_children_cnt, octree_children_cnt});

    node_offset_to_id[node_off] = n_off;

    out_dag.nodes.emplace_back();
    out_dag.child_edges.resize(che_off + octree_children_cnt);

    out_dag.nodes[n_off].children_edges_offset = che_off;
    out_dag.nodes[n_off].channels_edge.child_offset = 0;

    uint32_t node1 = in_tree.nodes[node_off + in_tree.header.references_offset];
    uint32_t node2 = in_tree.nodes[node_off + in_tree.header.links_offset - 1];

    if (in_tree.header.has_multi_nodes)
    {
      uint4 ref_code = uint4(head_unpacked.base_links_end, node1, node2, 0);
      if (ref_codes.find(ref_code) == ref_codes.end())
        ref_codes[ref_code] = 1;
      else
        ref_codes[ref_code]++;
      if (in_tree.header.references_per_uint == 8)
      {
        header_blk.segments.push_back({"references 0-7", node1, 32, 8});
      }
      else if (in_tree.header.references_per_uint == 4)
      {
        header_blk.segments.push_back({"references 0-3", node1, 32, 4});
        header_blk.segments.push_back({"references 4-7", node2, 32, 4});
      }
    }

    if (head_unpacked.base_links_end == 0)
    {
      out_dag.nodes[n_off].data_edges_offset = 0;
      out_dag.nodes[n_off].voxel_count_flags = DAG_pack_voxel_count_flags(0, false, false);
    }
    else
    {
      const uint32_t surf_count = head_unpacked.base_links_end - in_tree.header.has_channels;
      if (in_tree.header.has_surfaces)
      {
        out_dag.data_edges.resize(de_off + surf_count);
        for (int i = 0; i < surf_count; i++)
        {
          uint32_t link_off = node_off + in_tree.header.links_offset + i*in_tree.header.uints_per_link;
          SdfDAGDataEdge de = unpack_data_edge(in_tree.header, max_val, 
                                              in_tree.nodes[link_off], 
                                              in_tree.nodes[link_off + in_tree.header.child_offset_off]);
          bool is_unique = (in_tree.nodes[link_off] & in_tree.header.unique_brick_prefix) == in_tree.header.unique_brick_prefix;
          if (is_unique)
          {
            values_blk.segments.push_back({"offset (unique)", de.data_offset, 32, 1});
          }
          else
          {
            values_blk.segments.push_back({"offset", de.data_offset, mask_to_bits(in_tree.header.child_offset_mask), 1});
            values_blk.segments.push_back({"rot", (in_tree.nodes[link_off] >> in_tree.header.child_rot_shift) & in_tree.header.child_rot_mask, 
                                          mask_to_bits(in_tree.header.child_rot_mask), 1});
            values_blk.segments.push_back({"add", (in_tree.nodes[link_off] >> in_tree.header.child_add_shift) & in_tree.header.child_add_mask, 
                                          mask_to_bits(in_tree.header.child_add_mask), 1});
          }
          
          uint32_t dag_brick_off = brick_offset_to_id[de.data_offset];
          if (dag_brick_off == INVALID_IDX)
          {
            // printf("packing brick %d\n", (int)(out_dag.distances.size()/dist_count));
            dag_brick_off = out_dag.distances.size();
            brick_offset_to_id[de.data_offset] = out_dag.distances.size();
            out_dag.distances.resize(out_dag.distances.size() + dist_count);
            unpack_distances(in_tree.header, max_val, 
                            in_tree.bricks.data() + in_tree.header.bricks_step*de.data_offset, 
                            out_dag.distances.data() + dag_brick_off);
          }
          
          out_dag.data_edges[de_off + i].add = de.add;
          out_dag.data_edges[de_off + i].rotation_id = de.rotation_id;
          out_dag.data_edges[de_off + i].type_id = de.type_id;
          out_dag.data_edges[de_off + i].data_offset = dag_brick_off;
        }
        bool is_surface = head_unpacked.base_type == SCOM2_CHILD_LEAF_SURFACE;
        out_dag.nodes[n_off].data_edges_offset = de_off;
        out_dag.nodes[n_off].voxel_count_flags = DAG_pack_voxel_count_flags(dist_count, is_surface, false);
      }
      else
      {
        out_dag.data_edges.resize(de_off + 1);
        out_dag.data_edges[de_off].add = 0;
        out_dag.data_edges[de_off].rotation_id = 0;
        out_dag.data_edges[de_off].type_id = 0;
        out_dag.data_edges[de_off].data_offset = 0;
        bool is_surface = head_unpacked.base_type == SCOM2_CHILD_LEAF_SURFACE;
        out_dag.nodes[n_off].data_edges_offset = de_off;
        out_dag.nodes[n_off].voxel_count_flags = DAG_pack_voxel_count_flags(dist_count, is_surface, false);
      }

      //TODO: fix it!
      if (in_tree.header.has_channels)
      {
        const uint32_t ch_link_off = node_off + in_tree.header.links_offset + surf_count*in_tree.header.uints_per_link;
        out_dag.nodes[n_off].channels_edge = unpack_child_edge(in_tree.header, in_tree.nodes[ch_link_off], in_tree.nodes[ch_link_off + in_tree.header.child_offset_off]);
      }
    }

    float ch_max_val = get_max_sdf_val(powf(2, level+1), in_tree.header.dimension);

    uint32_t channels_off_link = get_links_end(in_tree.header, octree_children_cnt - 1, node1, node2);

    for (int ch_n = 0; ch_n < octree_children_cnt; ch_n++)
    {
      uint32_t ch_type = (head_unpacked.children_types >> SCOM2_CHILD_TYPE_BITS*ch_n) & SCOM2_CHILD_TYPE_MASK;
      uint32_t ch_start_link = ch_n == 0 ? head_unpacked.base_links_end : get_links_end(in_tree.header, ch_n-1, node1, node2);
      uint32_t ch_end_link = get_links_end(in_tree.header, ch_n, node1, node2);
      if (ch_type == SCOM2_CHILD_EMPTY)
      {
        // printf("child %d empty\n", ch_n);
        out_dag.child_edges[che_off + ch_n].child_offset = 0;
      }
      else if (ch_type == SCOM2_CHILD_NODE)
      {
        uint32_t link_off = node_off + in_tree.header.links_offset + ch_start_link*in_tree.header.uints_per_link;
        SdfDAGChildEdge ce = unpack_child_edge(in_tree.header, in_tree.nodes[link_off], 
                                               in_tree.nodes[link_off + in_tree.header.child_offset_off]);
             
        children_blk.segments.push_back({"offset (child)", ce.child_offset, mask_to_bits(in_tree.header.node_offset_mask), 1});
        children_blk.segments.push_back({"rot", (in_tree.nodes[link_off] >> in_tree.header.child_rot_shift) & in_tree.header.child_rot_mask, 
                                         32- mask_to_bits(in_tree.header.node_offset_mask), 1});

        if (node_offset_to_id[ce.child_offset] == INVALID_IDX)
        {
          unpack_SCom2_rec(in_tree, out_dag, brick_offset_to_id, node_offset_to_id, ce.child_offset, level + 1, verbose);
        }

        assert(node_offset_to_id[ce.child_offset] != INVALID_IDX);
        
        out_dag.child_edges[che_off + ch_n].child_offset = node_offset_to_id[ce.child_offset];
        out_dag.child_edges[che_off + ch_n].rotation_id = ce.rotation_id;
      }
      else //SCOM2_CHILD_LEAF_SURFACE or SCOM2_CHILD_LEAF_VOLUME
      {
        bool is_surface = ch_type == SCOM2_CHILD_LEAF_SURFACE;
        uint32_t surf_count = ch_end_link - ch_start_link;
        if (in_tree.header.has_multi_nodes)
        {
          surf_count -= in_tree.header.has_channels;
        }
        uint32_t ch_de_off = out_dag.data_edges.size();

        out_dag.child_edges[che_off + ch_n].child_offset = out_dag.nodes.size();
        out_dag.child_edges[che_off + ch_n].rotation_id = 0;

        out_dag.nodes.emplace_back();
        out_dag.nodes.back().channels_edge.child_offset = 0;
        out_dag.nodes.back().children_edges_offset = 0;
        out_dag.nodes.back().data_edges_offset = ch_de_off;
        out_dag.nodes.back().voxel_count_flags |= DAG_pack_voxel_count_flags(surf_count, is_surface, false);

        if (in_tree.header.has_surfaces)
        {
          out_dag.data_edges.resize(ch_de_off + surf_count);
          for (int i = 0; i < surf_count; i++)
          {
            uint32_t link_off = node_off + in_tree.header.links_offset + (ch_start_link + i)*in_tree.header.uints_per_link;
            SdfDAGDataEdge de = unpack_data_edge(in_tree.header, ch_max_val, 
                                                in_tree.nodes[link_off], 
                                                in_tree.nodes[link_off + in_tree.header.child_offset_off]);
            bool is_unique = (in_tree.nodes[link_off] & in_tree.header.unique_brick_prefix) == in_tree.header.unique_brick_prefix;
            if (is_unique)
            {
              children_blk.segments.push_back({"offset (unique)", de.data_offset, 32, 1});
            }
            else
            {
              children_blk.segments.push_back({"offset", de.data_offset, mask_to_bits(in_tree.header.child_offset_mask), 1});
              children_blk.segments.push_back({"rot", (in_tree.nodes[link_off] >> in_tree.header.child_rot_shift) & in_tree.header.child_rot_mask, 
                                              mask_to_bits(in_tree.header.child_rot_mask), 1});
              children_blk.segments.push_back({"add", (in_tree.nodes[link_off] >> in_tree.header.child_add_shift) & in_tree.header.child_add_mask, 
                                              mask_to_bits(in_tree.header.child_add_mask), 1});
            }         

            uint32_t dag_brick_off = brick_offset_to_id[de.data_offset];
            if (dag_brick_off == INVALID_IDX)
            {
              dag_brick_off = out_dag.distances.size();
              brick_offset_to_id[de.data_offset] = out_dag.distances.size();
              out_dag.distances.resize(out_dag.distances.size() + dist_count);
              unpack_distances(in_tree.header, ch_max_val, 
                              in_tree.bricks.data() + in_tree.header.bricks_step*de.data_offset, 
                              out_dag.distances.data() + dag_brick_off);
            }
            
            out_dag.data_edges[ch_de_off + i].add = de.add;
            out_dag.data_edges[ch_de_off + i].rotation_id = de.rotation_id;
            out_dag.data_edges[ch_de_off + i].type_id = de.type_id;
            out_dag.data_edges[ch_de_off + i].data_offset = dag_brick_off;
            //printf("data %d %d %d %f\n", dag_brick_off, de.rotation_id, de.type_id, de.add);
          }
        }
        else
        {
          out_dag.data_edges.resize(ch_de_off + 1);
          out_dag.data_edges[ch_de_off].add = 0;
          out_dag.data_edges[ch_de_off].rotation_id = 0;
          out_dag.data_edges[ch_de_off].type_id = 0;
          out_dag.data_edges[ch_de_off].data_offset = 0;
        }

        if (in_tree.header.has_channels)
        {
          if (in_tree.header.has_multi_nodes || !in_tree.header.has_surfaces)
          {
            const uint32_t ch_off = ch_start_link + (in_tree.header.has_surfaces ? surf_count : 0);
            const uint32_t link_off = node_off + in_tree.header.links_offset + ch_off*in_tree.header.uints_per_link;
            auto ce = unpack_child_edge(in_tree.header, in_tree.nodes[link_off], 
                                        in_tree.nodes[link_off + in_tree.header.child_offset_off]);
            out_dag.nodes.back().channels_edge = ce;
            children_blk.segments.push_back({"offset (channel)", ce.child_offset, mask_to_bits(in_tree.header.node_offset_mask), 1});
            children_blk.segments.push_back({"rot", (in_tree.nodes[link_off] >> in_tree.header.child_rot_shift) & in_tree.header.child_rot_mask, 
                                              mask_to_bits(in_tree.header.child_rot_mask), 1});
          }
          else
          {
            const uint32_t link_off = node_off + in_tree.header.links_offset + (channels_off_link++)*in_tree.header.uints_per_link;
            out_dag.nodes.back().channels_edge = unpack_child_edge(in_tree.header, in_tree.nodes[link_off], 
                                                                   in_tree.nodes[link_off + in_tree.header.child_offset_off]);
            auto ce = unpack_child_edge(in_tree.header, in_tree.nodes[link_off], 
                                        in_tree.nodes[link_off + in_tree.header.child_offset_off]);
            out_dag.nodes.back().channels_edge = ce;
            children_blk.segments.push_back({"offset (channel)", ce.child_offset, mask_to_bits(in_tree.header.node_offset_mask), 1});
            children_blk.segments.push_back({"rot", (in_tree.nodes[link_off] >> in_tree.header.child_rot_shift) & in_tree.header.child_rot_mask, 
                                              mask_to_bits(in_tree.header.child_rot_mask), 1});
          }
        }
        else
        {
          out_dag.nodes.back().channels_edge.child_offset = 0;
        }
      }
    }
    if (verbose)
      print_blocks({header_blk, values_blk, children_blk});
  }

  void unpack_SCom2(const SCom2Tree &in_tree, SdfDAG &out_dag, bool verbose)
  {
    std::vector<uint32_t> node_offset_to_id(in_tree.nodes.size(), INVALID_IDX);
    std::vector<uint32_t> brick_offset_to_id(in_tree.bricks.size(), INVALID_IDX);

    //empty links in 0 place, so 0 link is always means empty
    out_dag.child_edges.push_back(SdfDAGChildEdge{0, 0});
    out_dag.data_edges.push_back(SdfDAGDataEdge{0, 0, 0, 0.0f});
    
    out_dag.header = get_default_SdfDAGHeader();
    out_dag.header.node_grid_size = 2;
    out_dag.header.brick_size = in_tree.header.brick_size;
    out_dag.header.brick_pad = 0;
    out_dag.header.mat_id_off = in_tree.header.mat_id_off;
    out_dag.header.tex_id_off = in_tree.header.tex_id_off;
    out_dag.header.all_float_tex_id_off = in_tree.header.all_float_tex_id_off;
    out_dag.header.all_int_mat_id_off = in_tree.header.all_int_mat_id_off;

    out_dag.point_channels = in_tree.point_channels;
    out_dag.voxel_channels = in_tree.voxel_channels;

    out_dag.header.dim = in_tree.header.dimension;

    unpack_SCom2_rec(in_tree, out_dag, brick_offset_to_id, node_offset_to_id, in_tree.header.root_node_off, 0, verbose);

    if (!in_tree.header.has_surfaces)
    {
      out_dag.distances.resize(8, -1.0f);
    }

    uint32_t total_count = 0;
    std::vector<uint32_t> code_counts;
    code_counts.reserve(ref_codes.size());
    for (auto &p : ref_codes)
    {
      total_count += p.second;
      code_counts.push_back(p.second);
    }
    std::sort(code_counts.begin(), code_counts.end(), [](auto a, auto b) { return a > b; });
    uint32_t sum_count = 0;
    for (auto &c : code_counts)
    {
      sum_count += c;
      //printf("%d %.2f%%\n", c, 100.0f*sum_count/total_count);
    }

    printf("%d ref codes total\n", (int)ref_codes.size());
  }

  void replace_channel_links_to_values_rec(SCom2Tree &in_tree, std::vector<bool> &visited, uint32_t node_off)
  {
    visited[node_off] = true;

    //printf("node %x\n", in_tree.nodes[node_off]);
    
    NodeHeadUnpacked head_unpacked = unpack_node_head(in_tree.header, in_tree.nodes[node_off], 
                                                      in_tree.nodes[node_off + in_tree.header.links_offset - 1]);

    uint32_t octree_children_cnt = int_pow(2u, in_tree.header.dimension);
    uint32_t node1 = in_tree.nodes[node_off + in_tree.header.references_offset];
    uint32_t node2 = in_tree.nodes[node_off + in_tree.header.links_offset - 1];

    if (head_unpacked.base_links_end != 0)
    {
      const uint32_t surf_count = head_unpacked.base_links_end - in_tree.header.has_channels;

      if (in_tree.header.has_channels)
      {
        const uint32_t ch_link_off = node_off + in_tree.header.links_offset + surf_count*in_tree.header.uints_per_link;
        auto ce = unpack_child_edge(in_tree.header, in_tree.nodes[ch_link_off], in_tree.nodes[ch_link_off + in_tree.header.child_offset_off]);

        in_tree.nodes[ch_link_off] = 0;
        in_tree.nodes[ch_link_off + in_tree.header.child_offset_off] = in_tree.voxel_channels[0].data_i[ce.child_offset];
      }
    }

    uint32_t channels_off_link = get_links_end(in_tree.header, octree_children_cnt - 1, node1, node2);

    for (int ch_n = 0; ch_n < octree_children_cnt; ch_n++)
    {
      uint32_t ch_type = (head_unpacked.children_types >> SCOM2_CHILD_TYPE_BITS*ch_n) & SCOM2_CHILD_TYPE_MASK;
      uint32_t ch_start_link = ch_n == 0 ? head_unpacked.base_links_end : get_links_end(in_tree.header, ch_n-1, node1, node2);
      uint32_t ch_end_link = get_links_end(in_tree.header, ch_n, node1, node2);
      //printf("start end %d %d\n", ch_start_link, ch_end_link);
      
      if (ch_type == SCOM2_CHILD_EMPTY)
      {
        continue;
      }
      else if (ch_type == SCOM2_CHILD_NODE)
      {
        uint32_t link_off = node_off + in_tree.header.links_offset + ch_start_link*in_tree.header.uints_per_link;
        SdfDAGChildEdge ce = unpack_child_edge(in_tree.header, in_tree.nodes[link_off], 
                                               in_tree.nodes[link_off + in_tree.header.child_offset_off]);
             
        if (visited[ce.child_offset] == false)
        {
          replace_channel_links_to_values_rec(in_tree, visited, ce.child_offset);
        }
      }
      else //SCOM2_CHILD_LEAF_SURFACE or SCOM2_CHILD_LEAF_VOLUME
      {
        bool is_surface = ch_type == SCOM2_CHILD_LEAF_SURFACE;
        uint32_t surf_count = ch_end_link - ch_start_link;
        if (in_tree.header.has_multi_nodes)
        {
          surf_count -= in_tree.header.has_channels;
        }

        if (in_tree.header.has_channels)
        {
          uint32_t link_off = 0;
          if (in_tree.header.has_multi_nodes || !in_tree.header.has_surfaces)
          {
            const uint32_t ch_off = ch_start_link + (in_tree.header.has_surfaces ? surf_count : 0);
            link_off = node_off + in_tree.header.links_offset + ch_off*in_tree.header.uints_per_link;
          }
          else
          {
            link_off = node_off + in_tree.header.links_offset + (channels_off_link++)*in_tree.header.uints_per_link;
          }
          auto ce = unpack_child_edge(in_tree.header, in_tree.nodes[link_off], 
                                      in_tree.nodes[link_off + in_tree.header.child_offset_off]);

          in_tree.nodes[link_off] = 0;
          in_tree.nodes[link_off + in_tree.header.child_offset_off] = in_tree.voxel_channels[0].data_i[ce.child_offset];                      
        }
      }
    }
  }

  void replace_channel_links_to_values(SCom2Tree &tree)
  {
    if (tree.header.has_channels == 0 ||
        tree.point_channels.size() > 0 ||
        tree.voxel_channels.size() != 1 ||
        tree.voxel_channels[0].type != DataChannel::Type::INT ||
        tree.voxel_channels[0].num_components != 1)
    {
      printf("ERROR: Cannot replace channel links with values, not all conditions are met\n");
      return;
    }

    std::vector<bool> visited(tree.nodes.size(), false);
    replace_channel_links_to_values_rec(tree, visited, tree.header.root_node_off);
  }
}