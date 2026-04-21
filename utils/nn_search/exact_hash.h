#pragma once
#include "near_neighbor_common.h"
#include <unordered_map>
#include <vector>

namespace nn_search
{
  class ExactHashAS : public INNSearchAS
  {
  public:
    ExactHashAS() {}

    void build(const Dataset &dataset, int max_leaf_size) override;
    const float *get_closest_point(const float *query, float max_dist, float *dist_to_nearest) const override;
    int scan_near(const float *query, float max_dist, ScanFunction callback) const override;

  private:
    Dataset m_dataset;
    uint32_t m_dim;
    std::unordered_map<uint32_t, std::vector<uint32_t>> m_hash_map;
    uint32_t computeHashValue(const float *vector) const;
  };
}