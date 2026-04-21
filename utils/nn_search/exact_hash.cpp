#include "exact_hash.h"
#include <cstring>

namespace nn_search
{
  void ExactHashAS::build(const Dataset &dataset, int max_leaf_size)
  {
    m_dataset = dataset;
    m_dim = dataset.dim;

    //#pragma omp parallel for 
    for (uint32_t point_idx = 0; point_idx < m_dataset.data_points.size(); ++point_idx)
    {
      const float *point = m_dataset.all_points.data() + m_dataset.data_points[point_idx].data_offset;
      uint32_t hash_value = computeHashValue(point);
      m_hash_map[hash_value].push_back(point_idx);
    }
  }

  const float *ExactHashAS::get_closest_point(const float *query, float max_dist, float *dist_to_nearest) const
  {
    uint32_t hash_value = computeHashValue(query);
    auto it = m_hash_map.find(hash_value);
    if (it == m_hash_map.end())
      return nullptr;

    for (uint32_t point_idx : it->second)
    {
      const float *point = m_dataset.all_points.data() + m_dataset.data_points[point_idx].data_offset;
      if (memcmp(query, point, m_dim * sizeof(float)) == 0)
      {
        *dist_to_nearest = 0.0f;
        return point;
      }
    }
    return nullptr;
  }

  int ExactHashAS::scan_near(const float *query, float max_dist, ScanFunction callback) const
  {
    uint32_t hash_value = computeHashValue(query);
    auto it = m_hash_map.find(hash_value);
    if (it == m_hash_map.end())
      return 0;

    int num_visited = 0;
    for (uint32_t point_idx : it->second)
    {
      const float *point = m_dataset.all_points.data() + m_dataset.data_points[point_idx].data_offset;
      if (memcmp(query, point, m_dim * sizeof(float)) == 0)
      {
        bool finish = callback(0.0f, point_idx, m_dataset.data_points[point_idx], point);
        num_visited++;
        if (finish)
          return num_visited;
      }
    }
    return num_visited;
  }

  uint32_t ExactHashAS::computeHashValue(const float *point) const
  {
    //https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector/72073933#72073933
    uint32_t hash_value = m_dim;
    for (int i = 0; i < m_dim; ++i)
    {
      uint32_t x = *(uint32_t *)(point + i);
      x = ((x >> 16) ^ x) * 0x45d9f3b;
      x = ((x >> 16) ^ x) * 0x45d9f3b;
      x = (x >> 16) ^ x;
      hash_value ^= x + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
    }
    return hash_value;
  }
}