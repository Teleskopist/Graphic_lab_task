#include "channels_dataset.h"
#include "utils/common/vector_map.h"
#include <chrono>

namespace scom2
{
  // how many int channel values are stored per voxel (both voxel and point channels)
  uint32_t get_int_component_num(const SdfDAG &dag)
  {
    uint32_t num = 0;
    for (auto &ch : dag.point_channels)
    {
      if (ch.type == DataChannel::Type::INT)
        num += int_pow(2u, dag.header.dim)*ch.num_components;
    }
    for (auto &ch : dag.voxel_channels)
    {
      if (ch.type == DataChannel::Type::INT)
        num += ch.num_components;
    }
    return num;
  }

  void get_int_channels(const SdfDAG &dag, const std::vector<Transposition> &transpositions,
                        uint32_t p_id, uint32_t rot_id, int *out_desc)
  {
    int pos = 0;
    for (const auto &ch : dag.point_channels)
    {
      if (ch.type != DataChannel::Type::INT)
        continue;

      for (uint32_t idx = 0; idx < int_pow(2u, dag.header.dim); idx++)
      {
        uint32_t idx_rot = transpositions[rot_id][idx];
        for (int c_id = 0; c_id < ch.num_components; c_id++)
        {
          uint32_t off = (int_pow(2u, dag.header.dim) * p_id + idx_rot) * ch.num_components + c_id;
          out_desc[pos++] = ch.data_i[off];
        }
      }
    }
    for (const auto &ch : dag.voxel_channels)
    {
      if (ch.type != DataChannel::Type::INT)
        continue;
      for (int c_id = 0; c_id < ch.num_components; c_id++)
      {
        uint32_t off = p_id * ch.num_components + c_id;
        out_desc[pos++] = ch.data_i[off];
      }
    }
  }

  uint32_t SComDatasetChannels::get_bin_hash(uint32_t p_id, uint32_t rot_id, float *tmp_val_vector, uint32_t *tmp_bin_vector)
  {
    if (!has_channel_binners)
      return 0;
    
    const uint32_t nv = int_pow(2u, dag->header.dim);
    int bin_n = 0;
    for (uint32_t ch_n = 0; ch_n<dag->point_channels.size(); ch_n++)
    {
      const DataChannel &ch = dag->point_channels[ch_n];
      if (ch.type != DataChannel::Type::FLOAT || point_channel_binners[ch_n] == nullptr)
        continue;

      int pos = 0;
      for (uint32_t idx = 0; idx < nv; idx++)
      {
        uint32_t idx_rot = transform_subgroup->elements[rot_id][idx];
        for (int c_id = 0; c_id < ch.num_components; c_id++)
        {
          uint32_t off = (nv * p_id + idx_rot) * ch.num_components + c_id;
          tmp_val_vector[pos++] = ch.data_f[off];
        }
      }
      tmp_bin_vector[bin_n++] = point_channel_binners[ch_n]->calculate_bin_hash(tmp_val_vector, nv*ch.num_components);
    }
    for (uint32_t ch_n = 0; ch_n<dag->voxel_channels.size(); ch_n++)
    {
      const DataChannel &ch = dag->voxel_channels[ch_n];
      if (ch.type != DataChannel::Type::FLOAT || voxel_channel_binners[ch_n] == nullptr)
        continue;

      int pos = 0;
      for (int c_id = 0; c_id < ch.num_components; c_id++)
      {
        uint32_t off = p_id * ch.num_components + c_id;
        tmp_val_vector[pos++] = ch.data_f[off];
      }
      tmp_bin_vector[bin_n++] = voxel_channel_binners[ch_n]->calculate_bin_hash(tmp_val_vector, ch.num_components);
    }

    assert(bin_n > 0);

    if (bin_n == 1)
      return tmp_bin_vector[0];
    else
      return array_hash(tmp_bin_vector, bin_n); 
  }

  SComDatasetChannels::SComDatasetChannels(const SdfDAG *_dag,
                                           const std::vector<std::shared_ptr<IDescriptorMaker>> &_point_channel_dms,
                                           const std::vector<std::shared_ptr<IDescriptorMaker>> &_voxel_channel_dms,
                                           const std::vector<std::shared_ptr<IDescriptorBinner>> &_point_channel_binners,
                                           const std::vector<std::shared_ptr<IDescriptorBinner>> &_voxel_channel_binners)
  {
    dag = _dag;
    point_channel_dms = _point_channel_dms;
    voxel_channel_dms = _voxel_channel_dms;
    point_channel_binners = _point_channel_binners;
    voxel_channel_binners = _voxel_channel_binners;
    transform_subgroup = create_subgroup(TransformSubgroup::DEFAULT, 2, dag->header.dim); // only default is supported here

    has_channel_binners = false;
    for (auto &b : point_channel_binners)
      has_channel_binners = has_channel_binners || b != nullptr;
    for (auto &b : voxel_channel_binners)
      has_channel_binners = has_channel_binners || b != nullptr;    

    descriptor_size = 0; 
    for (int i = 0; i < dag->point_channels.size(); i++)
    {
      if (point_channel_dms[i])
        descriptor_size += point_channel_dms[i]->get_descriptor_size({dag->header.dim, 0, 2, (uint32_t)dag->point_channels[i].num_components});
    }
    for (int i = 0; i < dag->voxel_channels.size(); i++)
    {
      if (voxel_channel_dms[i])
        descriptor_size += voxel_channel_dms[i]->get_descriptor_size({dag->header.dim, 0, 1, (uint32_t)dag->voxel_channels[i].num_components});
    }

    // for each unique combination of integer channel values, create a subdataset
    // and store its index here
    VectorMap<int, uint32_t> int_vector_to_subdataset_id;
    std::vector<int> int_channels(get_int_component_num(*dag));
    std::vector<uint32_t> tmp_bin_vector;
    std::vector<float> tmp_val_vector;

    {
      uint32_t max_num_comp = 0;
      for (const auto &ch : dag->point_channels)
        max_num_comp = std::max(max_num_comp, int_pow(2u, dag->header.dim) * ch.num_components);
      for (const auto &ch : dag->voxel_channels)
        max_num_comp = std::max(max_num_comp, (uint32_t)ch.num_components);

      tmp_bin_vector.resize(dag->point_channels.size() + dag->voxel_channels.size());
      tmp_val_vector.resize(max_num_comp);
    }

    for (uint32_t n_id = 0; n_id < dag->nodes.size(); n_id++)
    {
      if (dag->nodes[n_id].channels_edge.child_offset != 0)
      {
        get_int_channels(*dag, transform_subgroup->elements, n_id, 0, int_channels.data());
        auto it = int_vector_to_subdataset_id.find(int_channels);
        uint32_t int_set_idx = INVALID_IDX;
        if (it == int_vector_to_subdataset_id.end())
        {
          int_set_idx = int_vector_to_subdataset_id.size();
          int_vector_to_subdataset_id[int_channels] = int_set_idx;
        }
        else
        {
          int_set_idx = it->second;
        }
        
        SubDatasetDescriptor desc;
        desc.channels_count = descriptor_size;
        desc.int_channels_idx = int_set_idx;
        desc.bin_hash = get_bin_hash(n_id, 0, tmp_val_vector.data(), tmp_bin_vector.data());
        sub_datasets[desc].push_back(n_id);
      }
    }

    if (int_vector_to_subdataset_id.size() == 0)
    {
      printf("[SComDatasetChannels] Error: no active nodes found\n");
    }
  }

  void SComDatasetChannels::calculate_descriptors(uint32_t count, const uint32_t *p_ids, const uint16_t *rot_ids, 
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
    dataset_info.node_count = count;

    auto t1 = std::chrono::high_resolution_clock::now();

    if (nodes.size() < count)
      nodes.resize(count);

    for (uint32_t i = 0; i < count; i++)
    {
      nodes[i].transform_id = rot_ids[i];
      nodes[i].level = 0; //TODO: set level
      nodes[i].type = BrickInfo::Type::VOLUME; //some default type, should not really matter
      nodes[i].average_val = 0.0f; //we do not calculate average for channels
    }

    auto t2 = std::chrono::high_resolution_clock::now();

    //offset of current part of the full descriptor inside each descriptor
    //i.e. <descriptor> = <point_channel_1_desc>...<point_channel_n_desc><voxel_channel_1_desc>...<voxel_channel_k_desc>
    uint32_t cur_desc_part_off = 0;

    assert(point_channel_dms.size() == dag->point_channels.size());
    for (int ch_id = 0; ch_id < dag->point_channels.size(); ch_id++)
    {
      // this channel is omitted and is not used to make descriptors
      if (point_channel_dms[ch_id] == nullptr)
        continue;
      assert(dag->point_channels[ch_id].type == DataChannel::Type::FLOAT);
      dataset_info.v_size = 2; //2x2x2
      dataset_info.num_components = dag->point_channels[ch_id].num_components;

      uint32_t ch_step = dataset_info.num_components * int_pow(dataset_info.v_size, dataset_info.dim);
      for (uint32_t i = 0; i < count; i++)
      {
        nodes[i].data = dag->point_channels[ch_id].data_f.data() + ch_step*subdataset->second[p_ids[i]];
        nodes[i].desc = out_descriptors + i * desc_stride + cur_desc_part_off;
      }
      
      point_channel_dms[ch_id]->calculate_decriptors(dataset_info, nodes.data());

      cur_desc_part_off += point_channel_dms[ch_id]->get_descriptor_size(dataset_info);
    }
    
    assert(voxel_channel_dms.size() == dag->voxel_channels.size());
    for (int ch_id = 0; ch_id < dag->voxel_channels.size(); ch_id++)
    {
      // this channel is omitted and is not used to make descriptors
      if (voxel_channel_dms[ch_id] == nullptr)
        continue;
      assert(dag->voxel_channels[ch_id].type == DataChannel::Type::FLOAT);
      dataset_info.v_size = 1; //1x1x1
      dataset_info.num_components = dag->voxel_channels[ch_id].num_components;

      uint32_t ch_step = dataset_info.num_components * int_pow(dataset_info.v_size, dataset_info.dim);
      for (uint32_t i = 0; i < count; i++)
      {
        nodes[i].data = dag->voxel_channels[ch_id].data_f.data() + ch_step*subdataset->second[p_ids[i]];
        nodes[i].desc = out_descriptors + i * desc_stride + cur_desc_part_off;
      }
      
      voxel_channel_dms[ch_id]->calculate_decriptors(dataset_info, nodes.data());

      cur_desc_part_off += voxel_channel_dms[ch_id]->get_descriptor_size(dataset_info);
    }
  }
}