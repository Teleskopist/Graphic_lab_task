#pragma once
#include "sdf/scom2/scom_builder.h"
#include "utils/nn_search/kd_tree.h"
#include "sdf/scom2/radiance_field/sh_rotate_table.h"
#include "sdf/scom2/radiance_field/sh_compute.h"
#include "sdf/scom2/scom_utils.h"

class SHColorBinner : public scom2::IDescriptorBinner
{
public:
  SHColorBinner(float _bin_mult = 3) : bin_mult(_bin_mult) {}
  virtual uint32_t calculate_bin_hash(const float *values, uint32_t size) const override
  {
    assert(size >= 3);

    constexpr uint32_t bits = 10;
    constexpr float max_val = (1<<bits)-1;

    uint32_t r = std::clamp<float>(bin_mult*std::abs(values[0]), 0, max_val);
    uint32_t g = std::clamp<float>(bin_mult*std::abs(values[1]), 0, max_val);
    uint32_t b = std::clamp<float>(bin_mult*std::abs(values[2]), 0, max_val);

    uint32_t bin = r | (g << bits) | (b << 2*bits);
    return bin;
  }
private:
  float bin_mult = 3;
};

class SHAdaptiveColorBinner : public scom2::IDescriptorBinner
{
public:
  SHAdaptiveColorBinner(uint32_t approx_bin_count, const float *sh_data, uint32_t sh_count, uint32_t sh_stride);
  virtual uint32_t calculate_bin_hash(const float *values, uint32_t size) const override;
private:
  std::vector<nn_search::KDTree::Node> m_nodes;
};

class SHTableRotDescriptorMaker : public scom2::IDescriptorMaker
{
public:
  SHTableRotDescriptorMaker(float _sh_importance) : sh_importance(_sh_importance) {}
  virtual void calculate_decriptors(DatasetInfo dataset_info, const Node *nodes)
  {
    // spherical harmonics in our implementation are bound to 3D space
    assert(dataset_info.dim == 3);

    // each set of SH coeffs should be rotated separately, so make sure we don't recieve concatenated blob here
    // if you got this assert, make sure you have BranchDescriptor::CONCATENATE for "sh" channel.
    assert(dataset_info.v_size == 1);

    // we hardcoded a number of sh coeffs for all_sh_rotate_matrices and expect to recieve exactly that number
    assert(dataset_info.num_components == SH_MAX_COEFFS * 3);

    for (uint32_t i = 0; i < dataset_info.node_count; i++)
    {
      for (uint32_t j = 0; j < SH_MAX_COEFFS; j++)
      {
        nodes[i].desc[j * 3 + 0] = 0.0f;
        nodes[i].desc[j * 3 + 1] = 0.0f;
        nodes[i].desc[j * 3 + 2] = 0.0f;
        for (uint32_t k = 0; k < SH_MAX_COEFFS; k++)
        {
          float q = all_sh_rotate_matrices[nodes[i].transform_id * SH_MAX_COEFFS * SH_MAX_COEFFS + j * SH_MAX_COEFFS + k];
          nodes[i].desc[j * 3 + 0] += nodes[i].data[k * 3 + 0] * q * sh_importance;
          nodes[i].desc[j * 3 + 1] += nodes[i].data[k * 3 + 1] * q * sh_importance;
          nodes[i].desc[j * 3 + 2] += nodes[i].data[k * 3 + 2] * q * sh_importance;
        }
      }
    }
  }
  virtual uint32_t get_descriptor_size(DatasetInfo dataset_info) const { return SH_MAX_COEFFS * 3; }

private:
  float sh_importance = 1.0f;
};