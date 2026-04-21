#pragma once
#include "scom.h"
#include "scom_builder.h"
#include "scom_internal.h"

namespace scom2
{
  class SComDatasetSDFBricks : public ISComDataset
  {
  public:
    SComDatasetSDFBricks(const SdfDAG *_dag, 
                         std::shared_ptr<IDescriptorMaker> _descriptor_maker,
                         std::shared_ptr<IDescriptorBinner> _descriptor_binner,
                         InternalBrickUse _internal_brick_use,
                         TransformSubgroup _transform_subgroup,
                         bool _use_average_val_transform,
                         float _surface_sensitivity);
    virtual ~SComDatasetSDFBricks() { }
    virtual const Subgroup *get_transform_group() const override { return transform_subgroup; }
    virtual uint32_t get_point_count() const override { 
      auto it = sub_datasets.find(cur_sub_desc);
      if (it == sub_datasets.end())
        return 0;
      return it->second.size();
    };
    virtual uint32_t get_descriptor_size() const override;
    virtual void calculate_descriptors(uint32_t count, const uint32_t *p_ids, const uint16_t *rot_ids, 
                                       float *out_desc, uint32_t desc_stride) const override;

    void fill_data_points_rec(uint32_t node_id, uint32_t level);

    InternalBrickUse internal_brick_use;
    uint32_t brick_size;
    uint32_t brick_pad;
    uint32_t v_size;
    uint32_t brick_values_count;
    uint32_t base_level;
    float surface_sensitivity;
    bool use_average_val_transform;
    SubDatasetDescriptor cur_sub_desc;

    const Subgroup *transform_subgroup = nullptr;
    std::vector<BrickInfo> data_points;
    SubDatasetMap<std::vector<uint32_t>> sub_datasets; // separate datasets with different descriptors
    const SdfDAG *dag;
    std::shared_ptr<IDescriptorMaker> descriptor_maker = nullptr;
    std::shared_ptr<IDescriptorBinner> descriptor_binner = nullptr;
    mutable std::vector<IDescriptorMaker::Node> nodes;
  };
}