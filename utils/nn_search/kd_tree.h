#pragma once
#include "near_neighbor_common.h"

namespace nn_search
{
  class KDTree : public INNSearchAS
  {
  public:
    struct Node
    {
      uint32_t left = 0;     // Index of left child in m_nodes
      uint32_t right = 0;    // Index of right child in m_nodes
      uint32_t begin = 0;    // Start index in m_indices (for leaves)
      uint32_t end = 0;      // End index in m_indices (exclusive)
      float split_val = 0;   // Split value for internal nodes
      uint8_t axis = 0;      // Split dimension
      bool is_leaf = false;
    };

    KDTree() = default;

    void build(const Dataset &dataset, int max_leaf_size) override;
    const float *get_closest_point(const float *query, float max_dist, float *dist_to_nearest = nullptr) const override;
    int scan_near(const float *query, float max_dist, ScanFunction callback) const override;

    const std::vector<uint32_t> &get_indices() const { return m_indices; }
    const     std::vector<Node> &get_nodes() const { return m_nodes; }

  private:

    uint32_t m_count = 0;
    uint32_t m_dim = 0;
    uint32_t m_max_leaf_size = 16;
    const Dataset *m_dataset = nullptr;
    std::vector<uint32_t> m_indices; // Permuted indices pointing to m_dataset
    std::vector<Node> m_nodes;       // Flat tree storage

    const float *get_vec(uint32_t idx) const { return m_dataset->all_points.data() + m_dataset->data_points[idx].data_offset; }
    uint32_t build_recursive(uint32_t begin, uint32_t end);
    bool search_recursive(uint32_t node_idx, const float* query, float max_dist_sq, 
                          ScanFunction& callback, int& found_count) const;
  };
}