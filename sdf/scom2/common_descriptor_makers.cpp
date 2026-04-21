#include "scom_internal.h"
#include "common_descriptor_makers.h"
#include "utils/common/int_pow.h"

namespace scom2
{

  uint32_t IDescriptorMaker::get_max_v_size(const SdfDAGHeader &header)
  {
    if (header.transform_subgroup != (uint32_t)TransformSubgroup::DEFAULT)
      return 9;
    else if (header.dim == 2)
      return 65;
    else if (header.dim == 3)
      return 17;
    else 
      return 9;
  }

  std::vector<const Subgroup *> create_subgroups(const SdfDAG *dag)
  {
    std::vector<const Subgroup *> subgroups(IDescriptorMaker::get_max_v_size(dag->header) + 1, nullptr);

    for (uint32_t v_size = 1; v_size < subgroups.size(); v_size++)
      subgroups[v_size] = create_subgroup((TransformSubgroup)dag->header.transform_subgroup, v_size, dag->header.dim);
    return subgroups;
  }

  class EmptyDescriptorMaker : public IDescriptorMaker
  {
  public:
    EmptyDescriptorMaker() {}
    virtual uint32_t get_descriptor_size(DatasetInfo dataset_info) const override { return 0; }
    virtual void calculate_decriptors(DatasetInfo dataset_info, const Node *nodes) override
    {
      printf("[EmptyDescriptorMaker] calculate_decriptors is not implemented and must not be called\n");
      assert(false);
    }
  };
  std::shared_ptr<IDescriptorMaker> create_empty_descriptor_maker() 
  { 
    return std::make_shared<EmptyDescriptorMaker>(); 
  }

  class TrivialDescriptorMaker : public IDescriptorMaker
  {
  public:
    TrivialDescriptorMaker(const SdfDAG *_dag,
                           TransformSubgroup _transform_subgroup)
    {
      transform_subgroup = _transform_subgroup;
      subgroups = create_subgroups(_dag);
    }

    virtual uint32_t get_descriptor_size(DatasetInfo dataset_info) const override
    {
      return int_pow(dataset_info.v_size, dataset_info.dim) * dataset_info.num_components;
    }

    virtual void calculate_decriptors(DatasetInfo dataset_info, const Node *nodes) override
    {
      //printf("compute %d trivial descriptors v_size=%d, dim=%d, comp_n=%d\n", 
      //       dataset_info.node_count, dataset_info.v_size, dataset_info.dim, dataset_info.num_components);
      assert(dataset_info.v_size < subgroups.size());
      const Subgroup *subgroup = subgroups[dataset_info.v_size];

      uint32_t comp_n = dataset_info.num_components;
      uint32_t points_per_node = int_pow(dataset_info.v_size, dataset_info.dim);

      if (dataset_info.v_size == 1)// simplified version without rotations
      {
        #pragma omp parallel for schedule(static)
        for (uint32_t i = 0; i < dataset_info.node_count; i++)
        {
          for (int c = 0; c < comp_n; c++)
            nodes[i].desc[c] = (nodes[i].data[c] - nodes[i].average_val);
        }
      }
      else
      {
        #pragma omp parallel for schedule(static)
        for (uint32_t i = 0; i < dataset_info.node_count; i++)
        {
          for (uint32_t idx = 0; idx < points_per_node; idx++)
          {
            uint32_t idx_transformed = subgroup->elements[nodes[i].transform_id][idx];
            for (int c = 0; c < comp_n; c++)
              nodes[i].desc[idx*comp_n+c] = (nodes[i].data[idx_transformed*comp_n+c] - nodes[i].average_val);
          }
        }
      }
    }
  private:
    TransformSubgroup transform_subgroup;
    std::vector<const Subgroup *> subgroups;
  };

  std::shared_ptr<IDescriptorMaker> create_trivial_descriptor_maker(const SdfDAG *_dag, TransformSubgroup _transform_subgroup) 
  { 
    return std::make_shared<TrivialDescriptorMaker>(_dag, _transform_subgroup); 
  }

  class IntegerDescriptorMaker : public IDescriptorMaker
  {
  public:
    IntegerDescriptorMaker(const SdfDAG *_dag,
                           TransformSubgroup _transform_subgroup)
    {
      transform_subgroup = _transform_subgroup;
      subgroups = create_subgroups(_dag);
    }

    virtual uint32_t get_descriptor_size(DatasetInfo dataset_info) const override
    {
      return int_pow(dataset_info.v_size, dataset_info.dim) * dataset_info.num_components;
    }

    virtual void calculate_decriptors(DatasetInfo dataset_info, const Node *nodes) override
    {
      assert(dataset_info.v_size < subgroups.size());
      const Subgroup *subgroup = subgroups[dataset_info.v_size];

      uint32_t comp_n = dataset_info.num_components;
      uint32_t points_per_node = int_pow(dataset_info.v_size, dataset_info.dim);

      if (dataset_info.v_size == 1)// simplified version without rotations
      {
        #pragma omp parallel for schedule(static)
        for (uint32_t i = 0; i < dataset_info.node_count; i++)
        {
          for (int c = 0; c < comp_n; c++)
            nodes[i].desc[c] = 1'000'000 * (nodes[i].data[c]);
        }
      }
      else
      {
        #pragma omp parallel for schedule(static)
        for (uint32_t i = 0; i < dataset_info.node_count; i++)
        {
          for (uint32_t idx = 0; idx < points_per_node; idx++)
          {
            uint32_t idx_transformed = subgroup->elements[nodes[i].transform_id][idx];
            for (int c = 0; c < comp_n; c++)
              nodes[i].desc[idx*comp_n+c] = 1'000'000 * (nodes[i].data[idx_transformed*comp_n+c]);
          }
        }
      }
    }
  private:
    TransformSubgroup transform_subgroup;
    std::vector<const Subgroup *> subgroups;
  };

  std::shared_ptr<IDescriptorMaker> create_integer_descriptor_maker(const SdfDAG *_dag, TransformSubgroup _transform_subgroup) 
  {
    return std::make_shared<IntegerDescriptorMaker>(_dag, _transform_subgroup);
  }

  class DefaultSDFDescriptorMaker : public IDescriptorMaker
  {
  public:
    DefaultSDFDescriptorMaker(const SdfDAG *_dag,
                              TransformSubgroup _transform_subgroup,
                              float _surface_sensitivity)
    {
      base_level = get_base_level(*_dag);
      transform_subgroup = _transform_subgroup;
      surface_sensitivity = _surface_sensitivity;
      subgroups = create_subgroups(_dag);
      base_values_count = int_pow(_dag->header.brick_size + 2*_dag->header.brick_pad + 1, _dag->header.dim);
      for (int i = 0; i <= IDescriptorMaker::MAX_OCTREE_DEPTH; i++)
      {
        level_sensitivity[i] = get_level_sensitivity(i, base_level, 2.0f);
      }
    }

    virtual uint32_t get_descriptor_size(DatasetInfo dataset_info) const override
    {
      return int_pow(dataset_info.v_size, dataset_info.dim);
    }

    virtual void calculate_decriptors(DatasetInfo dataset_info, const Node *nodes) override
    {
      assert(dataset_info.v_size > 1);          // SDF node cannot have single values, at least 2x2x2 voxels
      assert(dataset_info.num_components == 1); // SDF node cannot has 1 component, the distance itself

      assert(dataset_info.v_size < subgroups.size());
      const Subgroup *subgroup = subgroups[dataset_info.v_size];

      uint32_t data_per_node = get_descriptor_size(dataset_info);

      #pragma omp parallel for schedule(static)
      for (uint32_t i = 0; i < dataset_info.node_count; i++)
      {
        float sum = 0.0f;
        for (uint32_t idx = 0; idx < data_per_node; idx++)
        {
          uint32_t idx_transformed = subgroup->elements[nodes[i].transform_id][idx];
          float v = (nodes[i].data[idx_transformed] - nodes[i].average_val);
          nodes[i].desc[idx] = v;
          sum += v * v;
        }

        float i_sum = (float(data_per_node)/base_values_count) / sqrtf(sum);
        for (int j = 0; j < data_per_node; j++)
          nodes[i].desc[j] *= i_sum;
        
        encode_sdf_brick_default(data_per_node, nodes[i].type, level_sensitivity[nodes[i].level], surface_sensitivity, nodes[i].desc);
      }
    }
    
  private:
    float get_level_sensitivity(uint32_t level, uint32_t _base_level, float sensitivity_pow) const
    {
      return powf(sensitivity_pow, _base_level - std::min(_base_level, level));
    }

    // default encoding for SDF brick:
    // - apply given level sensitivity (1.0f for base level, higher for higher levels)
    // - apply surface sensitivity multiplier if node is surface
    // - encode internal bricks with -1.0 for all values
    void encode_sdf_brick_default(uint32_t brick_values_count, BrickInfo::Type type, float ls,
                                  float _surface_sensitivity, float *desc) const
    {
      assert(type != BrickInfo::Type::EMPTY);
      if (type == BrickInfo::Type::INTERNAL)
      {
        // all internal bricks are equal, we can merge all of the together
        for (int j = 0; j < brick_values_count; j++)
          desc[j] = -1.0f;
      }
      else if (type == BrickInfo::Type::SURFACE)
      {
        // add surface sensitivity multiplier
        for (int j = 0; j < brick_values_count; j++)
          desc[j] = ls * _surface_sensitivity * desc[j];
      }
      else
      {
        for (int j = 0; j < brick_values_count; j++)
          desc[j] = ls * desc[j];
      }
    }

    float surface_sensitivity = 1.0f;
    TransformSubgroup transform_subgroup;
    std::vector<const Subgroup *> subgroups;
    float level_sensitivity[IDescriptorMaker::MAX_OCTREE_DEPTH + 1];
    uint32_t base_level;
    uint32_t base_values_count = 1;
  };

  std::shared_ptr<IDescriptorMaker> create_default_sdf_descriptor_maker(const SdfDAG *_dag, TransformSubgroup _transform_subgroup, float _surface_sensitivity) 
  {
    return std::make_shared<DefaultSDFDescriptorMaker>(_dag, _transform_subgroup, _surface_sensitivity);
  }

  class NormalizedDescriptorMaker : public IDescriptorMaker
  {
  public:
    NormalizedDescriptorMaker(const SdfDAG *_dag,
                              TransformSubgroup _transform_subgroup,
                              float2 _min_max_range,
                              float target_range)
    {
      min_max_range = _min_max_range;
      range_inv = target_range / (min_max_range.y - min_max_range.x);
      transform_subgroup = _transform_subgroup;
      subgroups = create_subgroups(_dag);
    }

    virtual uint32_t get_descriptor_size(DatasetInfo dataset_info) const override
    {
      return int_pow(dataset_info.v_size, dataset_info.dim) * dataset_info.num_components;
    }

    virtual void calculate_decriptors(DatasetInfo dataset_info, const Node *nodes) override
    {
      assert(dataset_info.v_size < subgroups.size());
      const Subgroup *subgroup = subgroups[dataset_info.v_size];

      if (dataset_info.v_size == 1)// simplified version without rotations
      {
        uint32_t desc_size = get_descriptor_size(dataset_info);

        #pragma omp parallel for schedule(static)
        for (uint32_t i = 0; i < dataset_info.node_count; i++)
        {
          for (int j = 0; j < desc_size; j++)
            nodes[i].desc[j] = (nodes[i].data[j] - nodes[i].average_val - min_max_range.x) * range_inv;
        }
      }
      else // default version
      {
        uint32_t comp_n = dataset_info.num_components;
        uint32_t points_per_node = int_pow(dataset_info.v_size, dataset_info.dim);

        #pragma omp parallel for schedule(static)
        for (uint32_t i = 0; i < dataset_info.node_count; i++)
        {
          for (uint32_t idx = 0; idx < points_per_node; idx++)
          {
            uint32_t idx_transformed = subgroup->elements[nodes[i].transform_id][idx];
            for (int c = 0; c < comp_n; c++)
              nodes[i].desc[idx*comp_n+c] = (nodes[i].data[idx_transformed*comp_n+c] - nodes[i].average_val - min_max_range.x) * range_inv;
          }
        }
      }
    }
  private:
    float2 min_max_range = {0.0f, 1.0f};
    float range_inv = 1.0f;
    TransformSubgroup transform_subgroup;
    std::vector<const Subgroup *> subgroups;
  };

  std::shared_ptr<IDescriptorMaker> create_normalized_descriptor_maker(const SdfDAG *_dag, TransformSubgroup _transform_subgroup, float2 _min_max_range, float target_range) 
  {
    return std::make_shared<NormalizedDescriptorMaker>(_dag, _transform_subgroup, _min_max_range, target_range);
  }
}