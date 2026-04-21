#pragma once
#include "scom.h"
#include "scom_builder.h"
#include "scom_internal.h"

namespace scom2
{
  class SComDatasetChannels : public ISComDataset
  {
  public:
    SComDatasetChannels(const SdfDAG *_dag,
                        const std::vector<std::shared_ptr<IDescriptorMaker>> &_point_channel_dms,
                        const std::vector<std::shared_ptr<IDescriptorMaker>> &_voxel_channel_dms,
                        const std::vector<std::shared_ptr<IDescriptorBinner>> &_point_channel_binners,
                        const std::vector<std::shared_ptr<IDescriptorBinner>> &_voxel_channel_binners);
    virtual ~SComDatasetChannels() { }
    virtual const Subgroup *get_transform_group() const override { return transform_subgroup; }
    virtual uint32_t get_descriptor_size() const override { return descriptor_size; };
    virtual uint32_t get_point_count() const override { 
      auto it = sub_datasets.find(cur_sub_desc);
      if (it == sub_datasets.end())
        return 0;
      return it->second.size();
    };
    virtual void calculate_descriptors(uint32_t count, const uint32_t *p_ids, const uint16_t *rot_ids, 
                                       float *out_desc, uint32_t desc_stride) const override;

    uint32_t get_bin_hash(uint32_t p_id, uint32_t rot_id, float *tmp_val_vector, uint32_t *tmp_bin_vector);

    uint32_t descriptor_size;
    SubDatasetDescriptor cur_sub_desc;
    bool has_channel_binners = false;

    const SdfDAG *dag;
    const Subgroup *transform_subgroup = nullptr;
    SubDatasetMap<std::vector<uint32_t>> sub_datasets;
    std::vector<std::shared_ptr<IDescriptorMaker>> point_channel_dms; // one for each point channel
    std::vector<std::shared_ptr<IDescriptorMaker>> voxel_channel_dms; // one for each voxel channel
    std::vector<std::shared_ptr<IDescriptorBinner>> point_channel_binners; // one for each point channel, optional
    std::vector<std::shared_ptr<IDescriptorBinner>> voxel_channel_binners; // one for each voxel channel, optional
    mutable std::vector<IDescriptorMaker::Node> nodes;
  };  
}