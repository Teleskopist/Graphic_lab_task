#include "lsh.h"
#include <random>
namespace nn_search
{
  RandomProjectionHash::RandomProjectionHash(int dimension)
  {
    std::mt19937 gen(rand());
    std::normal_distribution<float> dist(0.0, 1.0);

    hyperplane.resize(dimension);
    for (int i = 0; i < dimension; ++i)
    {
      hyperplane[i] = dist(gen);
    }
  }

  int RandomProjectionHash::hash(const float *vector, int dim) const
  {
    float dot_product = 0.0f;
    for (int i = 0; i < dim; ++i)
    {
      dot_product += vector[i] * hyperplane[i];
    }
    return (dot_product >= 0) ? 1 : 0;
  }

  void LSHSearchAS::build(const Dataset &dataset, int max_leaf_size)
  {
    m_dataset = dataset;
    m_dim = dataset.dim;

    // Initialize hash tables
    tables.resize(m_num_tables);

    // Create hash functions for each table
    for (int i = 0; i < m_num_tables; ++i)
    {
      tables[i].hash_functions.resize(m_hash_bits);
      for (int j = 0; j < m_hash_bits; ++j)
      {
        tables[i].hash_functions[j] = RandomProjectionHash(m_dim);
      }
    }

    // Hash all points into buckets
    #pragma omp parallel for 
    for (int table_idx = 0; table_idx < m_num_tables; ++table_idx)
    {
      for (uint32_t point_idx = 0; point_idx < m_dataset.data_points.size(); ++point_idx)
      {
        const float *point = m_dataset.all_points.data() + m_dataset.data_points[point_idx].data_offset;

        uint32_t hash_value = computeHashValue(point, table_idx);
        tables[table_idx].buckets[hash_value].push_back(point_idx);
      }
    }
  }

  const float *LSHSearchAS::get_closest_point(const float *query, float max_dist,
                                              float *dist_to_nearest) const
  {
    std::set<uint32_t> candidates = getCandidates(query);

    const float *closest_point = nullptr;
    float closest_dist_sq = max_dist * max_dist;

    for (uint32_t idx : candidates)
    {
      const float *point = m_dataset.all_points.data() + m_dataset.data_points[idx].data_offset;
      float dist_sq = computeDistanceSquared(query, point, m_dim);

      if (dist_sq < closest_dist_sq)
      {
        closest_dist_sq = dist_sq;
        closest_point = point;
      }
    }

    if (dist_to_nearest && closest_point)
    {
      *dist_to_nearest = sqrtf(closest_dist_sq);
    }

    return closest_point;
  }

  int LSHSearchAS::scan_near(const float *query, float max_dist,
                             ScanFunction callback) const
  {
    int scan_count = 0;
    float max_dist_sq = max_dist * max_dist;

    for (int table_idx = 0; table_idx < m_num_tables; ++table_idx)
    {
      uint32_t hash_value = computeHashValue(query, table_idx);

      auto it = tables[table_idx].buckets.find(hash_value);
      if (it != tables[table_idx].buckets.end())
      {
        for (uint32_t idx : it->second)
        {
          const float *point = m_dataset.all_points.data() + m_dataset.data_points[idx].data_offset;
          float dist_sq = 0.0f;
          for (uint32_t i = 0; i < m_dim && dist_sq < max_dist_sq; ++i)
          {
            float diff = query[i] - point[i];
            dist_sq += diff * diff;
          }
          if (dist_sq < max_dist_sq)
          {
            bool finish = callback(sqrtf(dist_sq), idx, m_dataset.data_points[idx], point);
            scan_count++;
            if (finish)
              return scan_count;
          }
        }
      }
    }

    return scan_count;
  }

  uint32_t LSHSearchAS::computeHashValue(const float *vector, int table_idx) const
  {
    uint32_t hash_value = 0;

    for (int i = 0; i < m_hash_bits; ++i)
    {
      int bit = tables[table_idx].hash_functions[i].hash(vector, m_dim);
      hash_value = (hash_value << 1) | bit;
    }

    return hash_value;
  }

  std::set<uint32_t> LSHSearchAS::getCandidates(const float *query) const
  {
    std::set<uint32_t> candidates;

    for (int table_idx = 0; table_idx < m_num_tables; ++table_idx)
    {
      uint32_t hash_value = computeHashValue(query, table_idx);

      auto it = tables[table_idx].buckets.find(hash_value);
      if (it != tables[table_idx].buckets.end())
      {
        for (uint32_t idx : it->second)
        {
          candidates.insert(idx);
        }
      }
    }

    return candidates;
  }

  float LSHSearchAS::computeDistanceSquared(const float *a, const float *b, uint32_t dim)
  {
    float dist_sq = 0.0f;
    for (uint32_t i = 0; i < dim; ++i)
    {
      float diff = a[i] - b[i];
      dist_sq += diff * diff;
    }
    return dist_sq;
  }
}