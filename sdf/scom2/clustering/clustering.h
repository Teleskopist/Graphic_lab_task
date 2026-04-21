#pragma once
#include "utils/nn_search/near_neighbor_common.h"
#include "symmetric_groups.h"
#include <vector>
#include <memory>

namespace scom2
{
  static constexpr int ROT_ID_IDENTITY = 0;

  enum class ClusteringAlgorithm
  {
    REPLACEMENT,               //fast, guarantees that no replacement with loss > similarity_threshold will happen
                               //can ofter miss possible merges, does not use target_leaf_count 
    COMPONENTS_RECURSIVE_FILL, //faster than REPLACEMENT (because of multithreading) with similar quality
                               //guarantees that no replacement with loss > similarity_threshold will happen
                               //does not use target_leaf_count 
    HIERARCHICAL,              //the most theoretically sound, but slower than REPLACEMENT and eats more memory
                               //guarantees that no replacement with loss > similarity_threshold will happen
                               //can use target_leaf_count for more fine-tured compression to the exact size
    INVERSE_REPLACEMENT,       // fast, guarantees that no replacement with loss > similarity_threshold will happen
                               // can ofter miss possible merges, does not use target_leaf_count.
                               // faster and uses less memory than REPLACEMENT, very similar quality
    LEADER                     //[DEPRECATED]
  };

  enum class SearchAlgorithm
  {
    LINEAR_SEARCH, //for cycle for all nodes
    BALL_TREE,     //use ball tree acceleration structure
    LSH,           // Locality Sensitive Hashing
    EXACT_HASH,    // search only for exact matches using vector hash
    KD_TREE        // use kd-tree acceleration structure
  };

  struct Cluster
  {
    int lead_id = -1;
    int count = 0;
    std::vector<int> point_ids;
    std::vector<uint16_t> rotations;
  };

  struct ClusteringSettings
  {
    std::shared_ptr<nn_search::INNSearchAS> nn_search_as = nullptr;
    ClusteringAlgorithm clustering_algorithm = ClusteringAlgorithm::REPLACEMENT;
    SearchAlgorithm search_algorithm = SearchAlgorithm::BALL_TREE;
    float similarity_threshold = 0.0f; // set negative value to disable and use only target_leaf_count
    int   target_leaf_count = -1;      // set negative value to disable and use only similarity_threshold
    uint32_t clustering_dataset_size_limit_GB = 4; // if dataset is larger than this, clustering will be done in chunks
    bool use_close_match_heuristic = false; // for INVERSE_REPLACEMENT only, use close match heuristic increasing clustering speed with
                                            // a very small loss in quality
  };

  class ISComDataset
  {
  public:
    virtual ~ISComDataset() = default;
    virtual const Subgroup *get_transform_group() const = 0;
    virtual uint32_t get_descriptor_size() const = 0; // size of the descriptor in floats
    virtual uint32_t get_point_count() const = 0; // number of points in the dataset

    // Get dimension size with alignment for SIMD optimizations.
    // To support AVX2/AVX512 optimizations, descriptors should be aligned to 8/16 floats, 
    // depending on the instruction set available at runtime.
    static uint32_t get_dim_with_alignment(uint32_t base_dim);

    // The main function that is called during clustering.
    // It can change the internal state of dataset and descriptor makers, thus
    // it is  not thread-safe and should not be called concurrently.
    virtual void calculate_descriptors(uint32_t count, const uint32_t *p_ids, const uint16_t *rot_ids, 
                                       float *out_desc, uint32_t desc_stride) const = 0;
  };

  struct CompressionOutput
  {
    std::vector<Cluster> clusters;
  };
  
  void perform_clustering(const ISComDataset *in_dataset, const ClusteringSettings &settings, CompressionOutput &output, bool verbose = false);
}