#include "sdf_bricks_dataset.h"
#include <chrono>

namespace scom2
{
  uint32_t SComDatasetSDFBricks::get_descriptor_size() const
  {
    IDescriptorMaker::DatasetInfo dataset_info;
    dataset_info.dim = dag->header.dim;
    dataset_info.v_size = v_size;
    dataset_info.num_components = 1;

    return descriptor_maker->get_descriptor_size(dataset_info);
  }

  SComDatasetSDFBricks::SComDatasetSDFBricks(const SdfDAG *_dag, 
                                             std::shared_ptr<IDescriptorMaker> _descriptor_maker,
                                             std::shared_ptr<IDescriptorBinner> _descriptor_binner,
                                             InternalBrickUse _internal_brick_use,
                                             TransformSubgroup _transform_subgroup,
                                             bool _use_average_val_transform,
                                             float _surface_sensitivity)
  {
    descriptor_maker = _descriptor_maker;
    descriptor_binner = _descriptor_binner;
    internal_brick_use = _internal_brick_use;
    brick_size = _dag->header.brick_size;
    brick_pad = _dag->header.brick_pad;
    v_size = get_v_size(_dag->header);
    brick_values_count = get_v_count(_dag->header);
    surface_sensitivity = _surface_sensitivity;
    use_average_val_transform = _use_average_val_transform;
    dag = _dag;

    transform_subgroup = create_subgroup(_transform_subgroup, v_size, dag->header.dim);

    data_points.reserve(dag->nodes.size());
    fill_data_points_rec(0, 0);
    data_points.shrink_to_fit();

    base_level = get_base_level(*dag);

    for (uint32_t idx = 0; idx < data_points.size(); idx++)
    {
      SubDatasetDescriptor desc;
      desc.brick_type     = (uint32_t)data_points[idx].type;
      desc.surface_count  = 1;
      desc.channels_count = 0;
      desc.dist_count_per_surface = brick_values_count;
      desc.level = data_points[idx].level;

      if (descriptor_binner)
        desc.bin_hash = descriptor_binner->calculate_bin_hash(dag->distances.data() + data_points[idx].values_offset, brick_values_count);

      sub_datasets[desc].push_back(idx);
    }
  }

  void SComDatasetSDFBricks::fill_data_points_rec(uint32_t node_id, uint32_t level)
  {
    auto &node = dag->nodes[node_id];
    if (node.data_edges_offset != 0)
    {
      uint32_t surface_count = DAG_extract_count(node.voxel_count_flags);
      for (uint32_t i = 0; i < surface_count; ++i)
      {
        uint32_t val_off = dag->data_edges[node.data_edges_offset + i].data_offset;
        float sum = 0.0f;
        for (uint32_t j = 0; j < brick_values_count; ++j)
        {
          float v = dag->distances[val_off + j];
          sum += v;
        }

        BrickInfo::Type type = get_surface_type(dag, node_id, node.data_edges_offset + i, internal_brick_use);

        if (type != BrickInfo::Type::EMPTY)
        {
          data_points.push_back(BrickInfo());
          BrickInfo &dp = data_points.back();
          dp.original_id = node_id;
          dp.surface_id = i;
          dp.type = type;
          dp.level = level;
          dp.values_offset = val_off;
          dp.average_val = use_average_val_transform ? sum / brick_values_count : 0;
        }
      }
    }

    if (node.children_edges_offset != 0)
    {
      uint32_t ch_count = get_children_count(dag->header);
      for (int i = 0; i < ch_count; ++i)
      {
        uint32_t child_id = dag->child_edges[node.children_edges_offset + i].child_offset;
        if (child_id != 0)
          fill_data_points_rec(child_id, level + 1);
      }
    }
  }

  void SComDatasetSDFBricks::calculate_descriptors(uint32_t count, const uint32_t *p_ids, const uint16_t *rot_ids, 
                                                   float *out_descriptors, uint32_t desc_stride) const
  {
    auto subdataset = sub_datasets.find(cur_sub_desc);
    if (subdataset == sub_datasets.end())
    {
      assert(false);
      return;
    }

    IDescriptorMaker::DatasetInfo dataset_info;
    dataset_info.dim = dag->header.dim;
    dataset_info.v_size = v_size;
    dataset_info.node_count = count;
    dataset_info.num_components = 1;

    auto t1 = std::chrono::high_resolution_clock::now();

    if (nodes.size() < count)
      nodes.resize(count);
    
    for (uint32_t i = 0; i < count; i++)
    {
      auto &node = data_points[subdataset->second[p_ids[i]]];
      nodes[i].transform_id = rot_ids[i];
      nodes[i].level = node.level;
      nodes[i].type = node.type;
      nodes[i].average_val = node.average_val;
      nodes[i].data = dag->distances.data() + node.values_offset;
      nodes[i].desc = out_descriptors + i * desc_stride;
    }

    auto t2 = std::chrono::high_resolution_clock::now();

    descriptor_maker->calculate_decriptors(dataset_info, nodes.data());

    auto t3 = std::chrono::high_resolution_clock::now();
    float time_1 = std::chrono::duration<float, std::micro>(t2 - t1).count();
    float time_2 = std::chrono::duration<float, std::micro>(t3 - t2).count();
    // if (count > 1000)
      // printf("calculate_descriptors: %.1f %.1f us\n", time_1, time_2);
  }
}