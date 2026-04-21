#include "branches_dataset.h"
#include "utils/common/vector_map.h"
#include <chrono>

namespace scom2
{

  std::vector<ChannelOffsets> calculate_channel_offsets(const SdfDAGHeader &header, const std::vector<ChannelInfo> &channels, uint32_t branch_level)
  {
    std::vector<ChannelOffsets> offsets;

    ChannelOffsets cur_offsets;
    cur_offsets.data_off = 0;
    cur_offsets.transforms_off = 0;
    cur_offsets.descriptor_off = 0;

    uint32_t v_size_merge = int_pow(header.node_grid_size, branch_level) + 1;
    uint32_t v_count_merge = int_pow(v_size_merge, header.dim);
    uint32_t v_size_concat = int_pow(header.node_grid_size, branch_level);
    uint32_t v_count_concat = int_pow(v_size_concat, header.dim);

    for (const auto &info : channels)
    {
      offsets.push_back(cur_offsets);

      if (info.settings.descriptor_maker == nullptr)
        continue;

      IDescriptorMaker::DatasetInfo dataset_info;
      dataset_info.dim = header.dim;
      dataset_info.node_count = 1;
      dataset_info.num_components = info.channel->num_components;
      if (info.settings.branch_descriptor == BranchDescriptor::SIMPLE_MERGE)
      {
        assert(info.is_point_channel);

        dataset_info.v_size = v_size_merge;

        cur_offsets.data_off += info.channel->num_components * v_count_merge;
        cur_offsets.transforms_off += 1;
        cur_offsets.descriptor_off += info.settings.descriptor_maker->get_descriptor_size(dataset_info);
      }
      else if (info.settings.branch_descriptor == BranchDescriptor::CONCATENATE ||
               info.settings.branch_descriptor == BranchDescriptor::VOXEL_CONCATENATE)
      {
        dataset_info.v_size = info.is_point_channel ? 2 : 1;

        cur_offsets.data_off += info.channel->num_components * (info.is_point_channel ? int_pow(2u, header.dim) : 1) * v_count_concat;
        cur_offsets.transforms_off += v_count_concat;
        cur_offsets.descriptor_off += v_count_concat * info.settings.descriptor_maker->get_descriptor_size(dataset_info);
      }
      else
      {
        assert(false);
      }
    }

    offsets.push_back(cur_offsets);
    return offsets;
  }

  static inline float get_val(const DataChannel &ch, uint32_t idx)
  {
    if (ch.type == DataChannel::Type::FLOAT)
      return ch.data_f[idx];
    else if (ch.type == DataChannel::Type::INT)
      return ch.data_i[idx];
    else if (ch.type == DataChannel::Type::FP8)
      return ch.data_fp8[idx];
    else
      assert(false);
    return 0.0f;
  }

  // just merge a bunch of child bricks into one larger brick for the branch
  // all child bricks must have 0 or 1 surfaces
  void trivial_merge_child_bricks(uint32_t brick_size, uint32_t brick_pad, uint32_t brick_dim, 
                                  uint32_t node_grid_size, uint32_t num_components, float default_value, 
                                  MergeInData &in, uint32_t *tmp_counts, float *tmp_distances,
                                  uint32_t *out_surface_count, std::vector<float> &out_distances,
                                  uint32_t dist_off = INVALID_IDX)
  {
    // we use int4 dot for indices, therefore only 4D is supported, but more is impractical anyway 
    assert(brick_dim <= 4);

    bool apply_rotations = in.rotations && (in.transpositions.size() > 0);

    uint32_t brick_count = int_pow(node_grid_size, brick_dim);
    uint32_t v_size = 2*brick_pad + brick_size + 1;
    uint32_t v_count = int_pow(v_size, brick_dim);
    uint32_t out_v_size = brick_size*node_grid_size + 1;
    uint32_t out_v_count = int_pow(out_v_size, brick_dim);

    std::fill_n(tmp_counts, out_v_count, 0);
    std::fill_n(tmp_distances, num_components*out_v_count, 0.0f);
    
    // save values from children to the distances grid
    for (uint32_t i = 0; i < brick_count; ++i)
    {
      if (in.distances[i] == nullptr)
        continue;

      uint4 b_pos = idx_to_code(i, node_grid_size);
      for (uint32_t j = 0; j < v_count; ++j)
      {
        uint4 v_pos = idx_to_code(j, v_size);
        uint32_t idx = dot(b_pos*brick_size + v_pos, uint4(out_v_size*out_v_size, out_v_size, 1, out_v_size*out_v_size*out_v_size));
        uint32_t voxel_idx = apply_rotations ? in.transpositions[in.rotations[i]][j] : j;
        tmp_counts[idx] += 1;
        for (uint32_t k = 0; k < num_components; ++k)
          tmp_distances[idx*num_components + k] += in.distances[i][voxel_idx*num_components + k];
      }
    }

    // averaging values in corners or default value
    if (dist_off == INVALID_IDX)
    {
      dist_off = out_distances.size();
      out_distances.resize(dist_off + out_v_count, 0.0f);
    }

    *out_surface_count = 1;

    for (uint32_t i = 0; i < out_v_count; ++i)
    {
      if (tmp_counts[i] > 0)
      {
        for (uint32_t k = 0; k < num_components; ++k)
          out_distances[dist_off+i*num_components + k] = tmp_distances[i*num_components + k]/tmp_counts[i];
      }
      else
      {
        for (uint32_t k = 0; k < num_components; ++k)
          out_distances[dist_off+i*num_components + k] = default_value;
      }
    }
  }

  uint32_t SComDatasetBranches::get_descriptor_size() const
  {
    IDescriptorMaker::DatasetInfo dataset_info;
    dataset_info.dim = dag->header.dim;
    dataset_info.v_size = cons_levels.back().brick_size + 2*cons_levels.back().brick_pad + 1;
    dataset_info.node_count = 1;
    dataset_info.num_components = 1;

    uint32_t    desc_size = cur_sub_desc.surface_count * brick_descriptor_maker->get_descriptor_size(dataset_info);
    uint32_t ch_desc_size = cons_levels.back().channel_offsets.back().descriptor_off;

    return desc_size + ch_desc_size;
  }

  void SComDatasetBranches::calculate_descriptors(uint32_t count, const uint32_t *p_ids, const uint16_t *rot_ids, 
                                                  float *out_descriptors, uint32_t desc_stride) const
  {
    const ConsolidationLevel &cur_level = cons_levels.back();
    uint32_t s_count = cur_sub_desc.surface_count;
    uint32_t v_size = cur_level.brick_size + 2*cur_level.brick_pad + 1;
    uint32_t v_count = int_pow(v_size, dag->header.dim);

    auto subdataset = cur_level.sub_datasets.find(cur_sub_desc);
    if (subdataset == cur_level.sub_datasets.end())
    {
      assert(false);
      return;
    }

    IDescriptorMaker::DatasetInfo dataset_info;
    dataset_info.dim = dag->header.dim;
    dataset_info.v_size = v_size;
    dataset_info.node_count = s_count*count;
    dataset_info.num_components = 1;

    //surface descriptor size
    uint32_t b_desc_size = brick_descriptor_maker->get_descriptor_size(dataset_info);
    assert(b_desc_size*s_count <= desc_stride);

    uint32_t max_descriptors_per_node = std::max(1u, s_count);

    if (cur_level.channels_descriptor_size > 0)
    {
      for (uint32_t channel_n = 0; channel_n < channels.size(); channel_n++)
      {
        if (channels[channel_n].settings.descriptor_maker != nullptr &&
            channels[channel_n].settings.branch_descriptor == BranchDescriptor::CONCATENATE)
        {
          uint32_t concat_v_size = int_pow(dag->header.node_grid_size, uint32_t(cons_levels.size()-1));
          uint32_t max_separate_ch_descs = int_pow(concat_v_size, dag->header.dim);
          max_descriptors_per_node = std::max(max_descriptors_per_node, max_separate_ch_descs);
        }
      }
    }

    if (nodes.size() < max_descriptors_per_node*count)
      nodes.resize(max_descriptors_per_node*count);

    auto t1 = std::chrono::high_resolution_clock::now();

    if (b_desc_size*s_count > 0)
    {
      for (uint32_t i = 0; i < count; i++)
      {
        auto &anode = cur_level.active_nodes[subdataset->second[p_ids[i]]];
        for (uint32_t j = 0; j < s_count; j++)
        {
          nodes[s_count*i+j].transform_id = rot_ids[i];
          nodes[s_count*i+j].level = anode.level;
          nodes[s_count*i+j].type = anode.type;
          nodes[s_count*i+j].average_val = 0.0f; // we do not use average value for branches
          nodes[s_count*i+j].data = cur_level.values.data() + anode.values_offset + j*v_count;
          nodes[s_count*i+j].desc = out_descriptors + i*desc_stride + j*b_desc_size;
        }
      }
    }

    auto t2 = std::chrono::high_resolution_clock::now();

    if (b_desc_size*s_count > 0)
      brick_descriptor_maker->calculate_decriptors(dataset_info, nodes.data());

    auto t3 = std::chrono::high_resolution_clock::now();

    float time_1 = std::chrono::duration<float, std::micro>(t2 - t1).count();
    float time_2 = std::chrono::duration<float, std::micro>(t3 - t2).count();
    // if (count > 1000)
    //   printf("calculate_descriptors: %.1f %.1f %.1f us\n", time_1, time_2, time_3);  

    //offset of current part of the full branch descriptor inside each descriptor
    //i.e. <descriptor> = <brick_descriptor><channels_descriptor>
    //<brick_descriptor> = <surface_0_descriptor>...<surface_n_descriptor>
    uint32_t cur_desc_part_off = b_desc_size*s_count;

    if (cur_level.channels_descriptor_size > 0)
    {
      assert(cur_desc_part_off + cur_level.channels_descriptor_size <= desc_stride);
      const Subgroup *ch_subgroup = cur_level.channel_transform_subgroup;
      uint32_t concat_v_size = int_pow(dag->header.node_grid_size, uint32_t(cons_levels.size()-1));
      uint32_t max_separate_ch_descs = int_pow(concat_v_size, dag->header.dim);
      uint32_t ch_v_size = int_pow(dag->header.node_grid_size, uint32_t(cons_levels.size()-1)) + 1;

      for (uint32_t channel_n = 0; channel_n < channels.size(); channel_n++)
      {
        if (channels[channel_n].settings.descriptor_maker == nullptr)
          continue;

        const DataChannel &ch = *(channels[channel_n].channel);
        const ChannelCustomSettings &ch_settings = channels[channel_n].settings;
        uint32_t chOff = cur_level.channel_offsets[channel_n].data_off;
        uint32_t chDescOff = cur_level.channel_offsets[channel_n].descriptor_off;

        dataset_info.dim = dag->header.dim;
        dataset_info.num_components = ch.num_components;

        if (ch_settings.branch_descriptor == BranchDescriptor::SIMPLE_MERGE)
        {
          dataset_info.v_size = ch_v_size;
          dataset_info.node_count = count;

          for (uint32_t i = 0; i < count; i++)
          {
            auto &anode = cur_level.active_nodes[subdataset->second[p_ids[i]]];
            const float *in_data = cur_level.ch_values.data() + anode.channels_values_offset;
            const uint16_t *in_transforms = cur_level.ch_transforms.data() + anode.ch_transforms_offset;

            nodes[i].transform_id = rot_ids[i];
            nodes[i].level = anode.level;
            nodes[i].type = anode.type;
            nodes[i].average_val = 0.0f; // we do not use average value for branches
            nodes[i].data = in_data + chOff;
            nodes[i].desc = out_descriptors + i*desc_stride + cur_desc_part_off + chDescOff;
          }
        }
        else if (ch_settings.branch_descriptor == BranchDescriptor::CONCATENATE)
        // or voxels with rotations applied inside
        {
          dataset_info.v_size = channels[channel_n].is_point_channel ? 2 : 1;
          dataset_info.node_count = max_separate_ch_descs*count;

          uint32_t ch_data_size = ch.num_components * int_pow(dataset_info.v_size, dag->header.dim);
          uint32_t ch_desc_size = ch_settings.descriptor_maker->get_descriptor_size(dataset_info);

          for (uint32_t i = 0; i < count; i++)
          {
            auto &anode = cons_levels.back().active_nodes[subdataset->second[p_ids[i]]];
            const float *in_data = cons_levels.back().ch_values.data() + anode.channels_values_offset;
            const uint16_t *in_transforms = cons_levels.back().ch_transforms.data() + anode.ch_transforms_offset;

            for (uint32_t j = 0; j < max_separate_ch_descs; j++)
            {
              nodes[i*max_separate_ch_descs + j].transform_id = ch_subgroup->cayley_table[in_transforms[j]][rot_ids[i]];
              nodes[i*max_separate_ch_descs + j].level = anode.level;
              nodes[i*max_separate_ch_descs + j].type = anode.type;
              nodes[i*max_separate_ch_descs + j].average_val = 0.0f; // we do not use average value for branches
              nodes[i*max_separate_ch_descs + j].data = in_data + chOff + ch_subgroup->elements[rot_ids[i]][j]*ch_data_size;
              nodes[i*max_separate_ch_descs + j].desc = out_descriptors + i*desc_stride + cur_desc_part_off + chDescOff + j*ch_desc_size;
            }
          }
        }
        else if (ch_settings.branch_descriptor == BranchDescriptor::VOXEL_CONCATENATE)
        {
          dataset_info.v_size = concat_v_size;
          dataset_info.node_count = count;

          uint32_t ch_data_size = ch.num_components * max_separate_ch_descs;
          uint32_t ch_desc_size = ch_settings.descriptor_maker->get_descriptor_size(dataset_info);

          for (uint32_t i = 0; i < count; i++)
          {
            auto &anode = cons_levels.back().active_nodes[subdataset->second[p_ids[i]]];
            const float *in_data = cons_levels.back().ch_values.data() + anode.channels_values_offset;

            nodes[i].transform_id = rot_ids[i];
            nodes[i].level = anode.level;
            nodes[i].type = anode.type;
            nodes[i].average_val = 0.0f; // we do not use average value for branches
            nodes[i].data = in_data + chOff;
            nodes[i].desc = out_descriptors + i * desc_stride + cur_desc_part_off + chDescOff;
          }
        }
        else
        {
          assert(false);
        }

        ch_settings.descriptor_maker->calculate_decriptors(dataset_info, nodes.data());
      }
    }
  }

  SComDatasetBranches::SComDatasetBranches(const SdfDAG *_dag, 
                                           std::shared_ptr<IDescriptorMaker> _brick_descriptor_maker,
                                           std::shared_ptr<IDescriptorBinner> _brick_descriptor_binner,
                                           const std::vector<ChannelCustomSettings> &_pc_custom_settings,
                                           const std::vector<ChannelCustomSettings> &_vc_custom_settings,
                                           EmbeddingType _brick_embedding_type,
                                           InternalBrickUse _internal_brick_use,
                                           TransformSubgroup _bricks_transform_subgroup,
                                           TransformSubgroup _transform_subgroup,
                                           BranchDescriptor _branch_descriptor,
                                           float _merge_surface_threshold)
  {
    dag = _dag;
    brick_descriptor_maker = _brick_descriptor_maker;
    brick_descriptor_binner = _brick_descriptor_binner;
    brick_embedding_type = _brick_embedding_type;
    internal_brick_use = _internal_brick_use;
    branch_descriptor = _branch_descriptor;
    merge_surface_threshold = _merge_surface_threshold;
    bricks_transform_subgroup = _bricks_transform_subgroup;
    transform_subgroup = _transform_subgroup;

    assert(dag->point_channels.size() == _pc_custom_settings.size());
    for (int i=0; i<dag->point_channels.size(); i++)
    {
      ChannelInfo info;
      info.is_point_channel = true;
      info.channel = dag->point_channels.data() + i;
      info.settings = _pc_custom_settings[i];
      channels.push_back(info);
    }

    assert(dag->voxel_channels.size() == _vc_custom_settings.size());
    for (int i=0; i<dag->voxel_channels.size(); i++)
    {
      ChannelInfo info;
      info.is_point_channel = false;
      info.channel = dag->voxel_channels.data() + i;
      info.settings = _vc_custom_settings[i];
      channels.push_back(info);
    }

    create_initial_consolidated_level();
  }

  void SComDatasetBranches::create_initial_consolidated_level_rec(uint32_t node_id, uint32_t level, uint4 code)
  {
    uint32_t v_size = get_v_size(dag->header);
    auto &node = dag->nodes[node_id];
  
    // leaf nodes with some surface inside
    if (node.data_edges_offset != 0 && DAG_node_is_full(node.voxel_count_flags) && node.children_edges_offset == 0)
    {
      uint32_t cons_values_offset = cons_levels[0].values.size();
      uint32_t surface_count = DAG_extract_count(node.voxel_count_flags);
      uint32_t active_brick_count = 0;
      BrickInfo::Type sum_type = BrickInfo::Type::EMPTY;

      // check all surfaces, copy active surfaces to cons_levels[0].values
      // and calculate overall type of this node
      for (uint32_t i = 0; i < surface_count; ++i)
      {
        BrickInfo::Type type = get_surface_type(dag, node_id, node.data_edges_offset + i, internal_brick_use);
        sum_type = merge_types(sum_type, type);

        if (type != BrickInfo::Type::EMPTY)
        {
          active_brick_count++;
          uint32_t cur_off = cons_levels[0].values.size();
          uint32_t val_off = dag->data_edges[node.data_edges_offset + i].data_offset;
          uint32_t v_count = get_v_count(dag->header);
          cons_levels[0].values.resize(cur_off + v_count);

          for (uint32_t idx = 0; idx < v_count; idx++)
          {
            uint32_t rotation_id = dag->data_edges[node.data_edges_offset + i].rotation_id;
            uint32_t idx_rot = cons_levels[0].transform_subgroup->elements[rotation_id][idx];
            float v = (dag->distances[val_off + idx_rot] + dag->data_edges[node.data_edges_offset + i].add);
            cons_levels[0].values[cur_off + idx] = v;
          }
        }
      }

      nodes_cons_level[node_id] = active_brick_count > 0 ? 0 : NO_LEVEL;

      if (active_brick_count > 0)
      {
        ConsolidatedNode cnode;
        cnode.brick_count = active_brick_count;
        cnode.level = level;
        cnode.original_id = node_id;
        cnode.type = sum_type;
        cnode.values_offset = cons_values_offset;

        if (cons_levels[0].channels_descriptor_size > 0)
        {
          cnode.channels_values_offset = cons_levels[0].ch_values.size();
          cnode.ch_transforms_offset = cons_levels[0].ch_transforms.size();
          cons_levels[0].ch_values.resize(cnode.channels_values_offset + cons_levels[0].channels_data_size);
          cons_levels[0].ch_transforms.resize(cnode.ch_transforms_offset + cons_levels[0].channels_transforms_size);

          uint32_t chId = node.channels_edge.child_offset;
          uint32_t trId = node.channels_edge.rotation_id;

          for (int i = 0; i < channels.size(); i++)
          {
            if (channels[i].settings.descriptor_maker == nullptr)
              continue;

            const DataChannel &ch = *(channels[i].channel);

            uint32_t chOff = cons_levels[0].channel_offsets[i].data_off;
            uint32_t trOff = cons_levels[0].channel_offsets[i].transforms_off;

            uint32_t pCount = int_pow(2u, dag->header.dim);
            uint32_t nc = ch.num_components;
            if (channels[i].settings.branch_descriptor == BranchDescriptor::SIMPLE_MERGE)
            {
              for (int j = 0; j < pCount; j++)
              {
                uint32_t idx_rot = cons_levels[0].transform_subgroup->elements[trId][j];
                for (int k = 0; k < nc; k++)
                {
                  cons_levels[0].ch_values[cnode.channels_values_offset + chOff + nc * j + k] = get_val(ch, nc * (pCount * chId + idx_rot) + k);
                }
              }
              cons_levels[0].ch_transforms[cnode.ch_transforms_offset + trOff] = 0;
            }
            else if (channels[i].settings.branch_descriptor == BranchDescriptor::CONCATENATE ||
                     channels[i].settings.branch_descriptor == BranchDescriptor::VOXEL_CONCATENATE)
            {
              uint32_t pSz = nc * (channels[i].is_point_channel ? pCount : 1);
              for (int j = 0; j < pSz; j++)
                cons_levels[0].ch_values[cnode.channels_values_offset + chOff + j] = get_val(ch, pSz * chId + j);
              cons_levels[0].ch_transforms[cnode.ch_transforms_offset + trOff] = trId;
            }
          }
        }

        cons_levels[0].active_nodes.push_back(cnode);
        cons_levels[0].active_nodes_count++;
        cons_levels[0].node_id_by_global_id[node_id] = cons_levels[0].active_nodes.size() - 1;
      }

      if (bc_log)
      {
        write_initial_cons_node_to_bc_log(level, active_brick_count, v_size, code);
      }
    }
    else if (node.children_edges_offset != 0) // node with children
    {
      uint32_t ch_n = dag->header.node_grid_size;
      uint32_t ch_count = get_children_count(dag->header);
      for (int i = 0; i < ch_count; ++i)
      {
        uint32_t child_id = dag->child_edges[node.children_edges_offset + i].child_offset;
        if (child_id != 0)
          create_initial_consolidated_level_rec(child_id, level + 1, ch_n*code + idx_to_code(i, ch_n));
      }
    }
    else //empty node
    {
      nodes_cons_level[node_id] = NO_LEVEL;
    }
  }

  void SComDatasetBranches::create_initial_consolidated_level()
  {
    assert(cons_levels.size() == 0);

    uint32_t v_size = dag->header.brick_size + 2*dag->header.brick_pad + 1;

    cons_levels.emplace_back();
    cons_levels[0].node_id_by_global_id.resize(dag->nodes.size(), INVALID_IDX);
    cons_levels[0].brick_size = dag->header.brick_size;
    cons_levels[0].brick_pad  = dag->header.brick_pad;
    cons_levels[0].descriptor_size = 0;
    cons_levels[0].channel_offsets = calculate_channel_offsets(dag->header, channels, 0);
    cons_levels[0].channels_descriptor_size = cons_levels[0].channel_offsets.back().descriptor_off;
    cons_levels[0].channels_data_size = cons_levels[0].channel_offsets.back().data_off;
    cons_levels[0].channels_transforms_size = cons_levels[0].channel_offsets.back().transforms_off;
    cons_levels[0].transform_subgroup = create_subgroup(bricks_transform_subgroup, v_size, dag->header.dim);
    cons_levels[0].channel_transform_subgroup = create_subgroup(transform_subgroup, 2, dag->header.dim);

    nodes_cons_level.resize(dag->nodes.size(), LEVEL_UNKNOWN);

    create_initial_consolidated_level_rec(0, 0, uint4(0,0,0,0));
  }

  void SComDatasetBranches::write_initial_cons_node_to_bc_log(uint32_t level, uint32_t active_brick_count, uint32_t v_size, uint4 code)
  {
    BranchesCompressionLog::BranchInfo info;
    info.start_depth = level;
    if (active_brick_count == 0)
    {
      info.type = BranchesCompressionLog::BranchType::EMPTY;
    }
    else
    {
      info.type = BranchesCompressionLog::BranchType::LEAF;
      info.levels.resize(1);

      uint32_t v_count = int_pow(v_size, dag->header.dim);

      info.levels[0].grid_size = 1;
      info.levels[0].multibricks.resize(1);
      info.levels[0].multibricks[0].v_size = v_size;
      info.levels[0].multibricks[0].type = cons_levels[0].active_nodes.back().type;
      info.levels[0].multibricks[0].brick_count = active_brick_count;
      info.levels[0].multibricks[0].values.resize(active_brick_count * v_count);
      for (int i = 0; i < active_brick_count * v_count; i++)
        info.levels[0].multibricks[0].values[i] = cons_levels[0].values[cons_levels[0].active_nodes.back().values_offset + i];
    }

    bc_log->branch_map[code] = info;
  }

    void SComDatasetBranches::load_ch_data(uint32_t channel_id, const SdfDAGNode &node, bool is_point, ConsolidatedNode &cnode, 
                                           MergeInData &in, MergeTmpData &tmp)
    {
      const DataChannel &ch = *(channels[channel_id].channel);
      const ChannelCustomSettings &ch_settings = channels[channel_id].settings;
      uint32_t num_comp = (is_point ? int_pow(2u, dag->header.dim) : 1) * ch.num_components;
      uint32_t ch_n = dag->header.node_grid_size;
      uint32_t ch_count = int_pow(ch_n, dag->header.dim);
      uint32_t next_level_n = cons_levels.size() - 1;
      uint32_t prev_level_n = cons_levels.size() - 2;
      ConsolidationLevel &next_level = cons_levels[next_level_n];
      ConsolidationLevel &prev_level = cons_levels[prev_level_n];

      uint32_t chOff = next_level.channel_offsets[channel_id].data_off;
      uint32_t trOff = next_level.channel_offsets[channel_id].transforms_off;

      float *out_values = next_level.ch_values.data() + cnode.channels_values_offset + chOff;
      uint16_t *out_transforms = next_level.ch_transforms.data() + cnode.ch_transforms_offset + trOff;

      {
        uint32_t nextConcatSz = int_pow(dag->header.node_grid_size, next_level_n);
        uint32_t prevConcatSz = int_pow(dag->header.node_grid_size, prev_level_n);
        uint32_t nextConcatCnt = int_pow(nextConcatSz, dag->header.dim);
        uint32_t prevConcatCnt = int_pow(prevConcatSz, dag->header.dim);

        uint32_t active_children = 0;
        uint32_t upsampled_children = 0;

        // Iterate over all children, put their data into the new consolidated layer
        // the data itself is stored without transfomations, because we do not know
        // how to transform original data in general case (i.e. spehrical harmonics coefficients)
        // We store the combined transform code for each voxel instead
        for (int i = 0; i < ch_count; ++i)
        {
          uint32_t child_id = dag->child_edges[node.children_edges_offset + i].child_offset;
          uint32_t rot_id = dag->child_edges[node.children_edges_offset + i].rotation_id;

          // find consolidated level on which the child resides and its local id
          uint32_t localId = INVALID_IDX;
          uint32_t childLevel = INVALID_IDX;
          for (int level = prev_level_n; level >= 0; level--)
          {
            if (cons_levels[level].node_id_by_global_id[child_id] != INVALID_IDX)
            {
              localId = cons_levels[level].node_id_by_global_id[child_id];
              childLevel = level;
              break;
            }
          }

          if (childLevel != INVALID_IDX) // child node is found
          {
            const ConsolidationLevel &orig_level = cons_levels[childLevel];
            assert(localId < orig_level.active_nodes.size());
            const ConsolidatedNode &orig_node = orig_level.active_nodes[localId];
            float *ch_values;
            uint16_t *ch_transforms;
            if (childLevel == prev_level_n) // child node is in previous level
            {
              ch_values = prev_level.ch_values.data() + orig_node.channels_values_offset + prev_level.channel_offsets[channel_id].data_off;
              ch_transforms = prev_level.ch_transforms.data() + orig_node.ch_transforms_offset + prev_level.channel_offsets[channel_id].transforms_off;
            }
            else //child node is in higher level, upsample
            {
              uint32_t factor = int_pow(dag->header.node_grid_size, prev_level_n - childLevel);

              const float *in_ch_values = orig_level.ch_values.data() + orig_node.channels_values_offset + orig_level.channel_offsets[channel_id].data_off;
              const uint16_t *in_ch_transforms = orig_level.ch_transforms.data() + orig_node.ch_transforms_offset + orig_level.channel_offsets[channel_id].transforms_off;

              if (ch_settings.branch_descriptor == BranchDescriptor::SIMPLE_MERGE)
              {
                upsample_brick<true>(in_ch_values, upsample_tmp_values[i].data(), int_pow(dag->header.node_grid_size, childLevel)+1,
                                     factor, ch.num_components);
              }
              else if (ch_settings.branch_descriptor == BranchDescriptor::CONCATENATE ||
                       ch_settings.branch_descriptor == BranchDescriptor::VOXEL_CONCATENATE)
              {
                // TODO: is we concatenate point data, this process is not actually correct, as it does not interpolate anything,
                // However, it is a rare case that I never met with real data
                upsample_voxel_grid<float>(in_ch_values, upsample_tmp_values[i].data(), int_pow(dag->header.node_grid_size, childLevel),
                                           factor, num_comp);
                upsample_voxel_grid<uint16_t>(in_ch_transforms, upsample_tmp_transforms[i].data(),
                                              int_pow(dag->header.node_grid_size, childLevel), factor, 1);
              }

              ch_values = upsample_tmp_values[i].data();
              ch_transforms = upsample_tmp_transforms[i].data();
            }

            if (ch_settings.branch_descriptor == BranchDescriptor::SIMPLE_MERGE)
            {
              active_children++;
              in.distances[i] = ch_values;
              in.surface_counts[i] = orig_node.brick_count;
              in.rotations[i] = rot_id;
            }
            else if (prevConcatSz == 1) // no rotations of nodes, just copy data and calculate combined transforms
            {
              out_transforms[i] = prev_level.channel_transform_subgroup->cayley_table[ch_transforms[0]][rot_id];
              for (int k = 0; k < num_comp; k++)
                out_values[i * num_comp + k] = ch_values[k];
            }
            else
            {
              for (uint32_t j = 0; j < prevConcatCnt; j++)
              {
                uint4 b_code = idx_to_code(i, dag->header.node_grid_size);
                uint4 code = idx_to_code(j, prevConcatSz);
                uint32_t outIdx = code_to_idx(b_code*prevConcatSz + code, nextConcatSz);
                uint32_t chIdx = prev_level.channel_transform_subgroup->elements[rot_id][j];

                out_transforms[outIdx] = prev_level.channel_transform_subgroup->cayley_table[ch_transforms[chIdx]][rot_id];
                for (int k = 0; k < num_comp; k++)
                  out_values[outIdx * num_comp + k] = ch_values[chIdx * num_comp + k];
              }
            }
          }
          else // child node is empty, put default values in channel descriptor here
          {
            if (ch_settings.branch_descriptor == BranchDescriptor::SIMPLE_MERGE)
            {
              in.distances[i] = nullptr;
              in.surface_counts[i] = 0;
              in.rotations[i] = ROT_ID_IDENTITY;
            }
            else if (prevConcatSz == 1)
            {
              out_transforms[i] = rot_id;
              for (int k = 0; k < num_comp; k++)
                out_values[i * num_comp + k] = ch_settings.default_values[k];
            }
            else
            {
              for (uint32_t j = 0; j < prevConcatCnt; j++)
              {
                uint4 b_code = idx_to_code(i, dag->header.node_grid_size);
                uint4 code = idx_to_code(j, prevConcatSz);
                uint32_t outIdx = code_to_idx(b_code*prevConcatSz + code, nextConcatSz);

                out_transforms[outIdx] = rot_id;
                for (int k = 0; k < num_comp; k++)
                  out_values[outIdx * num_comp + k] = ch_settings.default_values[k];
              }
            }
          }
        }

        if (ch_settings.branch_descriptor == BranchDescriptor::SIMPLE_MERGE)
        {
          uint32_t out_surface_count;
          trivial_merge_child_bricks(prevConcatSz, 0, dag->header.dim, dag->header.node_grid_size, 
                                     ch.num_components, ch_settings.default_values[0], in, tmp.counts, tmp.distances, 
                                     &out_surface_count, next_level.ch_values, cnode.channels_values_offset + chOff);
        }
      }
    }

  void SComDatasetBranches::create_next_consolidated_level_rec(MergeInData &in, MergeTmpData &tmp,
                                                               uint32_t node_id, uint32_t level, uint4 code)
  {
    uint32_t next_level_n = cons_levels.size() - 1;
    uint32_t prev_level_n = cons_levels.size() - 2;
    ConsolidationLevel &next_level = cons_levels[cons_levels.size() - 1];
    ConsolidationLevel &prev_level = cons_levels[prev_level_n];

    BrickInfo::Type sum_type = BrickInfo::Type::EMPTY;
    uint32_t active_children = 0;
    uint32_t upsampled_children = 0;
    const SdfDAGNode &node = dag->nodes[node_id];

    // early return if this node is empty or was consolidated before
    // note that create_next_consolidated_level_rec is applied to _every_
    // node in the DAG for every level, but must not be applied twice
    if (node.children_edges_offset == 0 || nodes_cons_level[node_id] < LEVEL_UNKNOWN)
      return;

    std::string log = "";

    uint32_t ch_n = dag->header.node_grid_size;
    uint32_t ch_count = int_pow(ch_n, dag->header.dim);
    for (int i = 0; i < ch_count; ++i)
    {
      uint32_t child_id = dag->child_edges[node.children_edges_offset + i].child_offset;
      uint32_t rot_id = dag->child_edges[node.children_edges_offset + i].rotation_id;

      if (prev_level.node_id_by_global_id[child_id] != INVALID_IDX) // child node is in previous level
      {
        ConsolidatedNode prev_node = prev_level.active_nodes[prev_level.node_id_by_global_id[child_id]];
        sum_type = merge_types(sum_type, prev_node.type);
        active_children++;
        in.distances[i] = prev_level.values.data() + prev_node.values_offset;
        in.surface_counts[i] = prev_node.brick_count;
        in.rotations[i] = rot_id;

        log += " P ";
      }
      else if (!DAG_node_is_full(dag->nodes[child_id].voxel_count_flags) &&
               dag->nodes[child_id].children_edges_offset == 0) // child node is empty
      {
        in.distances[i] = nullptr;
        in.surface_counts[i] = 0;
        in.rotations[i] = ROT_ID_IDENTITY;

        log += " E ";
      }
      else // child node is leaf/some other level (variable depth octree)
      {
        // Check if exists on lower levels
        uint32_t last_active_level = INVALID_IDX;
        for (int lev_i = cons_levels.size() - 3; lev_i >= 0; --lev_i)
          if (cons_levels[lev_i].node_id_by_global_id[child_id] != INVALID_IDX)
          {
            last_active_level = lev_i;
            break;
          }

        if (last_active_level != INVALID_IDX) // child node is in lower level, resample it to the prev level
        {
          ConsolidationLevel &last_level = cons_levels[last_active_level];
          ConsolidatedNode last_node = last_level.active_nodes[last_level.node_id_by_global_id[child_id]];

          uint32_t v_size_in = last_level.brick_size + 2 * last_level.brick_pad + 1;
          uint32_t v_count_in = int_pow(v_size_in, dag->header.dim);
          uint32_t v_size_out = prev_level.brick_size + 2 * prev_level.brick_pad + 1;
          uint32_t v_count_out = int_pow(v_size_out, dag->header.dim);

          uint32_t val_i = upsampled_children;
          upsample_tmp_indices[val_i] = i;

          for (uint32_t brick_i = 0u; brick_i < last_node.brick_count; ++brick_i)
          {
            upsample_brick<false> (last_level.values.data() + last_node.values_offset + brick_i * v_count_in,
                                   upsample_tmp_values[val_i].data() + brick_i * v_count_out, v_size_in, 
                                   prev_level.brick_size / last_level.brick_size);
          }

          sum_type = merge_types(sum_type, last_node.type);
          active_children++;
          upsampled_children++;
          in.surface_counts[i] = last_node.brick_count;
          in.rotations[i] = rot_id;

          log += "V"+std::to_string(last_active_level)+" ";
        }
        else // child node is on higher/similar level. We should not process this node at all
        {
          log += " F ";

          nodes_cons_level[node_id] = LEVEL_UNCONSOLIDATED;
          active_children = 0;
          break;
        }
      }
    }

    if (active_children > 0 ||
       (nodes_cons_level[node_id] == LEVEL_UNKNOWN && sum_type == BrickInfo::Type::EMPTY))
    {
      //Node can be compressed on this stage.
      //Perform upsampling of data if needed and add to the list for
      //similarity compression.
      //If node is empty, set surface count to 0 and default channel values
      assert(sum_type == BrickInfo::Type::EMPTY || active_children > 0);

      ConsolidatedNode cnode;
      cnode.level = level;
      cnode.original_id = node_id;
      cnode.type = sum_type;

      if (active_children == 0)
      {
        cnode.brick_count = 0;
        cnode.values_offset = 0;
      }
      else
      {
        for (uint32_t i = 0u; i < upsampled_children; ++i)
          in.distances[upsample_tmp_indices[i]] = upsample_tmp_values[i].data();

        uint32_t out_surface_count = 0;
        uint32_t new_values_offset = next_level.values.size();
        if (branch_descriptor == BranchDescriptor::DEFAULT ||
            branch_descriptor == BranchDescriptor::SDF_MERGE ||
            branch_descriptor == BranchDescriptor::SDF_MULTI_MERGE)
        {
          //sdf_converter::merge_child_bricks works for 3D only
          assert(dag->header.dim == 3);
          sdf_converter::merge_child_bricks(prev_level.brick_size, prev_level.brick_pad, dag->header.node_grid_size,
                                            2.0f / (1 << level), merge_surface_threshold, branch_descriptor == BranchDescriptor::SDF_MERGE,
                                            in, tmp, &out_surface_count, next_level.values);
        }
        else if (branch_descriptor == BranchDescriptor::SIMPLE_MERGE)
        {
          trivial_merge_child_bricks(prev_level.brick_size, prev_level.brick_pad, dag->header.dim, dag->header.node_grid_size, 
                                    1, 0.0f, in, tmp.counts, tmp.distances, &out_surface_count, next_level.values); 
        }
        else if (branch_descriptor == BranchDescriptor::CONCATENATE)
        {
          printf("BranchDescriptor::CONCATENATE is not allowed for bricks, only for channels\n");
          assert(false);
        }
        else
        {
          assert(false);
        }
        
        cnode.brick_count = out_surface_count;
        cnode.values_offset = new_values_offset;
      }

      if (next_level.channels_descriptor_size > 0)
      {
        cnode.channels_values_offset = next_level.ch_values.size();
        next_level.ch_values.resize(cnode.channels_values_offset + next_level.channels_data_size, 0.0f);
        cnode.ch_transforms_offset = next_level.ch_transforms.size();
        next_level.ch_transforms.resize(cnode.ch_transforms_offset + next_level.channels_transforms_size, 0.0f);

        if (active_children > 0)
        {
          uint32_t childChOff = 0;
          uint32_t childTrOff = 0;

          uint32_t chOff = 0;
          uint32_t trOff = 0;

          for (int channel_id = 0; channel_id < channels.size(); ++channel_id)
          {
            if (channels[channel_id].settings.descriptor_maker)
              load_ch_data(channel_id, node, channels[channel_id].is_point_channel, cnode, in, tmp);
          }
        }
      }

      next_level.active_nodes.push_back(cnode);
      next_level.active_nodes_count++;
      next_level.node_id_by_global_id[node_id] = next_level.active_nodes.size() - 1;
      nodes_cons_level[node_id] = cons_levels.size() - 1;

      if (bc_log)
      {
        write_cons_node_to_bc_log(prev_level, next_level, level, cnode, ch_n, ch_count, node, in, code);
      }
    }

    for (int i = 0; i < ch_count; ++i)
    {
      uint32_t child_id = dag->child_edges[node.children_edges_offset + i].child_offset;
      if (child_id != 0)
        create_next_consolidated_level_rec(in, tmp, child_id, level + 1, ch_n * code + idx_to_code(i, ch_n));
    }
  }

  void SComDatasetBranches::write_cons_node_to_bc_log(scom2::ConsolidationLevel &prev_level, scom2::ConsolidationLevel &next_level, 
                                                      uint32_t level, scom2::ConsolidatedNode &cnode, uint32_t ch_n, uint32_t ch_count, 
                                                      const SdfDAGNode &node, sdf_converter::MergeInData &in, uint4 code)
  {
    uint32_t p_v_size = prev_level.brick_size + 2 * prev_level.brick_pad + 1;
    uint32_t p_v_count = int_pow(p_v_size, dag->header.dim);
    uint32_t n_v_size = next_level.brick_size + 2 * next_level.brick_pad + 1;
    uint32_t n_v_count = int_pow(n_v_size, dag->header.dim);
    BranchesCompressionLog::BranchInfo info;
    info.start_depth = level;
    info.type = BranchesCompressionLog::BranchType::CONSOLIDATED;
    info.levels.resize(cons_levels.size());

    info.levels[0].grid_size = 1;
    info.levels[0].multibricks.resize(1);
    info.levels[0].multibricks[0].v_size = n_v_size;
    info.levels[0].multibricks[0].type = cnode.type;
    info.levels[0].multibricks[0].brick_count = cnode.brick_count;
    info.levels[0].multibricks[0].values.resize(cnode.brick_count * n_v_count);
    for (int i = 0; i < cnode.brick_count * n_v_count; i++)
      info.levels[0].multibricks[0].values[i] = next_level.values[cnode.values_offset + i];

    info.levels[1].grid_size = ch_n;
    info.levels[1].multibricks.resize(ch_count);
    for (int i = 0; i < ch_count; i++)
    {
      uint32_t child_id = dag->child_edges[node.children_edges_offset + i].child_offset;
      uint32_t rot_id = dag->child_edges[node.children_edges_offset + i].rotation_id;

      if (prev_level.node_id_by_global_id[child_id] != INVALID_IDX) // child node is in previous level
      {
        ConsolidatedNode prev_node = prev_level.active_nodes[prev_level.node_id_by_global_id[child_id]];

        info.levels[1].multibricks[i].v_size = p_v_size;
        info.levels[1].multibricks[i].type = prev_node.type;
        info.levels[1].multibricks[i].brick_count = prev_node.brick_count;
        info.levels[1].multibricks[i].values.resize(prev_node.brick_count * p_v_count);

        for (int s = 0; s < prev_node.brick_count; s++)
        {
          for (uint32_t idx = 0; idx < p_v_count; idx++)
          {
            uint32_t off   = s * p_v_count + idx;
            uint32_t r_off = s * p_v_count + in.transpositions[rot_id][idx];
            info.levels[1].multibricks[i].values[off] = prev_level.values[prev_node.values_offset + r_off];
          }
        }
      }
      // else if (!tmp_indices.empty())
      // {
      //   // TODO:
      // }
      else
      {
        info.levels[1].multibricks[i].v_size = p_v_size;
        info.levels[1].multibricks[i].type = BrickInfo::Type::EMPTY;
        info.levels[1].multibricks[i].brick_count = 0;
      }
    }

    bc_log->branch_map[code] = info;
  }

  void SComDatasetBranches::create_next_consolidated_level()
  {
    assert(cons_levels.size() > 0);

    uint32_t prev_b_size = cons_levels.back().brick_size;
    uint32_t prev_v_size = prev_b_size + 2*cons_levels.back().brick_pad + 1;
    uint32_t prev_v_count = int_pow(prev_v_size, dag->header.dim);
    uint32_t prev_channels_data_size = cons_levels.back().channels_data_size;

    uint32_t new_b_size  = cons_levels.back().brick_size * dag->header.node_grid_size;
    uint32_t new_v_size  = new_b_size + 2*dag->header.brick_pad + 1;
    uint32_t new_v_count = int_pow(new_v_size, dag->header.dim);

    uint32_t max_child_nodes = int_pow(dag->header.node_grid_size, dag->header.dim);
    uint32_t cur_level = cons_levels.size();

    cons_levels.emplace_back();
    cons_levels.back().node_id_by_global_id.resize(dag->nodes.size(), INVALID_IDX);
    cons_levels.back().brick_size = new_b_size;
    cons_levels.back().brick_pad  = dag->header.brick_pad;
    cons_levels.back().descriptor_size = brick_embedding_type == EmbeddingType::NO_EMBEDDING ? 0 : new_v_count;
    cons_levels.back().channel_offsets = calculate_channel_offsets(dag->header, channels, cur_level);
    cons_levels.back().channels_descriptor_size = cons_levels.back().channel_offsets.back().descriptor_off;
    cons_levels.back().channels_data_size = cons_levels.back().channel_offsets.back().data_off;
    cons_levels.back().channels_transforms_size = cons_levels.back().channel_offsets.back().transforms_off;
    cons_levels.back().transform_subgroup = create_subgroup(transform_subgroup, new_v_size, dag->header.dim);
    cons_levels.back().channel_transform_subgroup = create_subgroup(transform_subgroup, int_pow<uint32_t>(dag->header.node_grid_size, cur_level), dag->header.dim);

    MergeInData  in_data;
    MergeTmpData tmp_data;

    in_data.distances = new const float* [max_child_nodes]; std::fill_n(in_data.distances, max_child_nodes, nullptr);
    in_data.surface_counts = new uint16_t[max_child_nodes]; std::fill_n(in_data.surface_counts, max_child_nodes, 0u);
    in_data.rotations = new uint16_t[max_child_nodes];       std::fill_n(in_data.rotations, max_child_nodes, 0u);

    in_data.transpositions.resize(cons_levels[cons_levels.size()-2].transform_subgroup->elements.size());
    for (uint32_t i = 0; i < cons_levels[cons_levels.size()-2].transform_subgroup->elements.size(); i++)
      in_data.transpositions[i] = cons_levels[cons_levels.size()-2].transform_subgroup->elements[i].indices;

    int max_components = 1;
    for (auto &pc : dag->point_channels)
      max_components = std::max(pc.num_components, max_components);
    for (auto &vc : dag->voxel_channels)
      max_components = std::max(vc.num_components, max_components);

    tmp_data.counts = new uint32_t[new_v_count];                  std::fill_n(tmp_data.counts, new_v_count, 0u);
    tmp_data.distances = new float[max_components*new_v_count];   std::fill_n(tmp_data.distances, max_components*new_v_count, 0.0f);
    tmp_data.frozen = new bool[new_v_count];                      std::fill_n(tmp_data.frozen, new_v_count, false);
    tmp_data.updated_values = new bool[new_v_count];              std::fill_n(tmp_data.updated_values, new_v_count, false);
    tmp_data.voxel_is_active = new bool[new_v_count];             std::fill_n(tmp_data.voxel_is_active, new_v_count, false);
    tmp_data.dist_is_used = new bool[new_v_count];                std::fill_n(tmp_data.dist_is_used, new_v_count, false);
    tmp_data.surface_offsets = new uint32_t[max_child_nodes + 1];   
    tmp_data.all_surfaces = new sdf_converter::SdfSurface[max_child_nodes*sdf_converter::GlobalOctree::MAX_SURFACE_COUNT];
    tmp_data.all_surface_values = new float[max_child_nodes*sdf_converter::GlobalOctree::MAX_SURFACE_COUNT*prev_v_count];
    tmp_data.surface_ids = new int[max_child_nodes];
    tmp_data.codes_stack = new int2[3*max_child_nodes];

    tmp_data.single_merge_in_data.distances = new const float* [max_child_nodes];
    tmp_data.single_merge_in_data.surface_counts = new uint16_t[max_child_nodes];

    uint32_t max_values_per_child = std::max<uint32_t>(max_components, sdf_converter::GlobalOctree::MAX_SURFACE_COUNT);
    uint32_t max_count = std::max(new_v_count, int_pow(int_pow(dag->header.node_grid_size, cur_level-1), dag->header.dim));
    upsample_tmp_values = std::vector<std::vector<float>>(max_child_nodes, std::vector<float>(max_values_per_child * max_count));
    upsample_tmp_transforms = std::vector<std::vector<uint16_t>>(max_child_nodes, std::vector<uint16_t>(max_count));
    upsample_tmp_indices = std::vector<uint32_t>(max_child_nodes, 0u);

    // reserve space for values in the new consolidated level to make recursive building faster with less allocations
    // this is a ballpark estimate of how much space we'll need, so some allocations may still happen
    cons_levels.back().values.reserve(cons_levels[cons_levels.size()-2].values.size());

    create_next_consolidated_level_rec(in_data, tmp_data, 0, 0, uint4(0,0,0,0));
    
    cons_levels.back().values.shrink_to_fit();

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

    // create subdatasets from active nodes

    std::vector<uint32_t> binning_partial_hashes(channels.size() + 1);

    for (uint32_t idx = 0; idx < cons_levels.back().active_nodes.size(); idx++)
    {
      const auto &node = cons_levels.back().active_nodes[idx];

      uint32_t h_id = 0;
      if (brick_descriptor_binner)
      {
        const float *brick = cons_levels.back().values.data() + node.values_offset;
        uint32_t size = cons_levels.back().descriptor_size;
        binning_partial_hashes[h_id++] = brick_descriptor_binner->calculate_bin_hash(brick, size);
      }
      for (uint32_t ch_n = 0; ch_n < channels.size(); ch_n++)
      {
        if (channels[ch_n].settings.descriptor_binner == nullptr)
          continue;

        const float *data = cons_levels.back().ch_values.data() + 
                            node.channels_values_offset + cons_levels.back().channel_offsets[ch_n].data_off;
        uint32_t size = cons_levels.back().channel_offsets[ch_n+1].data_off - cons_levels.back().channel_offsets[ch_n].data_off;
        binning_partial_hashes[h_id++] = channels[ch_n].settings.descriptor_binner->calculate_bin_hash(data, size);
      }

      SubDatasetDescriptor desc;
      desc.channels_count = cons_levels.back().channels_descriptor_size;
      desc.dist_count_per_surface = cons_levels.back().descriptor_size;
      desc.level = node.level;
      desc.brick_type = (uint32_t)node.type;
      desc.surface_count = node.brick_count;
      desc.bin_hash = array_hash(binning_partial_hashes.data(), h_id);

      cons_levels.back().sub_datasets[desc].push_back(idx);
    }
  }  
}