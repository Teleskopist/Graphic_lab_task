#pragma once
#include "near_neighbor_common.h"
#include <unordered_map>
#include <set>

namespace nn_search
{
  // Generate random hyperplane with normal vector w
  // Hash function: h(v) = sign(w · v)
  class RandomProjectionHash
  {
  public:
    RandomProjectionHash() = default;
    RandomProjectionHash(int dimension);
    int hash(const float *vector, int dim) const;

  private:
    std::vector<float> hyperplane; // Random normal vector
  };

  class LSHSearchAS : public INNSearchAS
  {
  private:
    struct LSHTable
    {
      std::vector<RandomProjectionHash> hash_functions;
      std::unordered_map<uint32_t, std::vector<uint32_t>> buckets;
    };

    std::vector<LSHTable> tables;
    Dataset m_dataset;
    uint32_t m_dim;
    uint32_t m_num_tables; // L parameter
    uint32_t m_hash_bits;  // k parameter

  public:
    LSHSearchAS(int num_tables = 10, int hash_bits = 20)
        : m_num_tables(num_tables), m_hash_bits(hash_bits) {}

    void build(const Dataset &dataset, int max_leaf_size) override;
    const float *get_closest_point(const float *query, float max_dist, float *dist_to_nearest) const override;

    int scan_near(const float *query, float max_dist, ScanFunction callback) const override;

  private:
    uint32_t computeHashValue(const float *vector, int table_idx) const;
    std::set<uint32_t> getCandidates(const float *query) const;
    static float computeDistanceSquared(const float *a, const float *b, uint32_t dim);
  };
}