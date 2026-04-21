#include "kd_tree.h"
#include <algorithm>
namespace nn_search
{
  void KDTree::build(const Dataset &dataset, int max_leaf_size)
  {
    m_dataset = &dataset;
    m_count = dataset.data_points.size();
    m_dim = dataset.dim;
    m_max_leaf_size = max_leaf_size;
    m_indices.resize(m_count);

    // Initialize indices 0 to N-1
    for (int i = 0; i < m_count; ++i)
      m_indices[i] = i;

    m_nodes.clear();
    m_nodes.reserve(m_count / max_leaf_size); // Heuristic reservation

    // Build recursively
    build_recursive(0, m_count);
  }

  const float *KDTree::get_closest_point(const float *query, float max_dist, float *dist_to_nearest) const
  {
    const float *closest_point = nullptr;
    float closest_dist_sq = max_dist * max_dist + 1;
    scan_near(query, max_dist, [&](float dist, uint32_t, const DataPoint &dp, const float *point) -> bool {
      if (dist < closest_dist_sq)
      {
        closest_dist_sq = dist;
        closest_point = point;
      }

      return false;
    });

    if (dist_to_nearest)
      *dist_to_nearest = sqrtf(closest_dist_sq);

    return closest_point;
  }

  int KDTree::scan_near(const float *query, float max_dist, ScanFunction callback) const
  {
    if (m_count == 0)
      return 0;

    float max_dist_sq = max_dist * max_dist;
    int found_count = 0;

    // Start search from root (index 0)
    search_recursive(0, query, max_dist_sq, callback, found_count);

    return found_count;
  }

  uint32_t KDTree::build_recursive(uint32_t begin, uint32_t end)
  {
    uint32_t count = end - begin;
    uint32_t node_idx = (uint32_t)m_nodes.size();
    m_nodes.emplace_back(); // Create placeholder

    Node &node = m_nodes[node_idx];
    node.begin = begin;
    node.end = end;

    // Base case: create leaf
    if (count <= m_max_leaf_size)
    {
      node.is_leaf = true;
      return node_idx;
    }

    // Step 1: Find dimension with maximum spread (variance approximation)
    // We use min/max spread which is faster than variance and sufficient
    uint32_t best_axis = 0;
    int max_spread = -1;

    // To save time on huge datasets, we can sample points, but here we do full scan
    // consistent with robust construction.
    for (uint32_t d = 0; d < m_dim; ++d)
    {
      float min_val = get_vec(m_indices[begin])[d];
      float max_val = get_vec(m_indices[begin])[d];
      for (uint32_t i = begin; i < end; ++i)
      {
        float val = get_vec(m_indices[i])[d];
        if (val < min_val)
          min_val = val;
        if (val > max_val)
          max_val = val;
      }
      int spread = max_val - min_val;
      if (spread > max_spread)
      {
        max_spread = spread;
        best_axis = d;
      }
    }

    // If all points are identical (spread is 0), make it a leaf
    if (max_spread == 0)
    {
      node.is_leaf = true;
      return node_idx;
    }

    node.axis = (float)best_axis;

    // Step 2: Split by median
    uint32_t mid = begin + count / 2;

    // nth_element partially sorts m_indices so that the element at 'mid'
    // is the median, and elements to left are smaller, right are larger.
    std::nth_element(
        m_indices.begin() + begin,
        m_indices.begin() + mid,
        m_indices.begin() + end,
        [&](uint32_t a, uint32_t b)
        {
          return get_vec(a)[best_axis] < get_vec(b)[best_axis];
        });

    node.split_val = get_vec(m_indices[mid])[best_axis];

    // Recurse (Be careful with iterator invalidation if m_nodes reallocates,
    // so we use indices 'node_idx' and refetch pointer)
    uint32_t left_idx = build_recursive(begin, mid);
    uint32_t right_idx = build_recursive(mid, end);

    // Re-fetch node reference as recursion might have reallocated vector
    m_nodes[node_idx].left = left_idx;
    m_nodes[node_idx].right = right_idx;
    m_nodes[node_idx].is_leaf = false;

    return node_idx;
  }

  bool KDTree::search_recursive(uint32_t node_idx, const float *query, float max_dist_sq,
                                ScanFunction &callback, int &found_count) const
  {
    const Node &node = m_nodes[node_idx];

    // Leaf Case: Linear scan
    if (node.is_leaf)
    {
      //printf("in leaf %d to %d\n", node.begin, node.end);
      for (uint32_t i = node.begin; i < node.end; ++i)
      {
        uint32_t idx = m_indices[i];
        const float *point = get_vec(idx);
        float dist_sq = 0;

        // Compute squared Euclidean distance
        // Loop unrolling or SIMD often handled by compiler here
        for (uint32_t d = 0; d < m_dim; ++d)
        {
          float diff = point[d] - query[d];
          dist_sq += diff * diff;
        }

        if (dist_sq <= max_dist_sq)
        {
          //printf("dist_sq: %f\n", dist_sq);
          bool finish = callback(std::sqrt(dist_sq), idx, m_dataset->data_points[idx], point);
          found_count++;
          if (finish)
            return true;
        }
      }
      return false;
    }

    // Internal Node Case
    float diff = (float)query[node.axis] - (float)node.split_val;
    float diff_sq = diff * diff;

    // Decide traversal order: visit closer side first
    uint32_t near_child = (diff < 0) ? node.left : node.right;
    uint32_t far_child = (diff < 0) ? node.right : node.left;

    // 1. Always search the near side
    bool finish = search_recursive(near_child, query, max_dist_sq, callback, found_count);

    // 2. Search the far side ONLY if the hypersphere intersects the splitting plane
    if (diff_sq <= max_dist_sq && !finish)
    {
      finish = search_recursive(far_child, query, max_dist_sq, callback, found_count);
    }

    return finish;
  }
}