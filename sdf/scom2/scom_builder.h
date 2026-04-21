#pragma once
#include "utils/nn_search/near_neighbor_common.h"
#include "sdf/common/global_octree.h"
#include "blk/blk.h"
#include "scom.h"
#include "packing.h"
#include "clustering/clustering.h"
#include "clustering/symmetric_groups.h"

#include <vector>
#include <map>

namespace scom2
{
  enum class OutputType
  {
    DAG8,  // octree-like (2x2x2 children) directed acyclic graph, uncompressed structure, used for debug similarity compression
    DAG64, // 64-tree DAG
  };

  enum class EmbeddingType
  {
    DIRECT,      // use distance values from bricks directly for similarity compression (default)
    NEURAL,      // use neural embeddings (see converter_neural.cpp for details)
    NO_EMBEDDING,// do not use diven values at all, merge each subdataset into one cluster immediately
    TRIVIAL,     // use trivial embeddings (no SDF-specific transforms)
    CUSTOM       // use custom embeddings
  };

  enum class SimMetric
  {
    EUCLIDEAN,
    L1,
    CUSTOM,
  };

  enum class CombinedLoss
  {
    L1,
    L2,
    CUSTOM,
  };

  enum class PackedBrickType
  {
    FULL,    // <all_distances>
    BITMASK  // <bitmask for all distances><active distances>
  };

  enum class PackedSurfaceLimit
  {
    STRICT, // max 15  surfaces for node with children combined
    RELAXED // max 255 surfaces for node with children combined
  };

  enum class PackedReferenceType
  {
    SHORT_6_8_18, // 6 bit rotation, 8 bit offset, 18 bit brick offset (1 uint32_t)
    LONG_6_8_32,  // 6 bit rotation, 8 bit offset, 32 bit brick offset (2 uint32_t)
    LONG_6_26_32, // 6 bit rotation, 26 bit offset, 32 bit brick offset (2 uint32_t)
    LONG_9_23_32  // 9 bit rotation, 23 bit offset, 32 bit brick offset (2 uint32_t)
  };

  // what to do with internal bricks (bricks with all negative values)
  enum class InternalBrickUse
  {
    DROP,             // drop all internal bricks alltogether
    MERGE_ALL,        // treat all internal bricks as indistinguishable from each other and merge to one cluster 
    PROPER_CLUSTERING // use proper clustering for internal bricks, i.e. treat them as border ones
  };

  // how distance values from children are combined to create branch descriptor for similarity compression
  enum class BranchDescriptor
  {
    DEFAULT,        // use default rules: SDF_MULTI_MERGE for bricks, SIMPLE_MERGE for point channels, CONCATENATE for voxel channels
    SIMPLE_MERGE,   // child grids (with size (brick_size+1)^3 values) are merged to larger grid ((2*brick_size+1)^3 values)
                    // multi nodes are not supported, empty values are set to 0
    SDF_MERGE,      // child grids (with size (brick_size+1)^3 values) are merged to larger grid ((2*brick_size+1)^3 values)
                    // multi nodes are not supported, values are assumed to be SDF values, empty values are calculated using redistancing
    SDF_MULTI_MERGE,// child grids ((brick_size+1)^3 values per surface) are merged to larger grid ((2*brick_size+1)^3 values per surface)
                    // using merge_child_bricks function. Values are assumed to be SDF values, multi nodes are supported, empty values
                    // are calculated using redistancing
    CONCATENATE,    // concatenation of child grids
    VOXEL_CONCATENATE, // simplier concatenation of child grids for voxel channels (for cases when channel values are not rotated)
  }; 

  struct BrickInfo
  {
    enum class Type : uint8_t
    {
      EMPTY,    // all SDF values are positive, this brick should be ignored
      INTERNAL, // internal brick, all SDF values are negative
      VOLUME,   // typical SDF brick
      SURFACE,  // surfaced brick
    };
    uint32_t original_id;   // id of node in DAG
    uint16_t surface_id;    // id of surface in DAG node
    Type type;              // type of the brick
    uint8_t level;          // level of the node in the octree
    uint32_t values_offset; // offset in the vector of SDF values
    float average_val;      // average value of the brick
  };

  class IDescriptorMaker
  {
  public:
    struct DatasetInfo
    {
      uint32_t dim = 3;            // dimensionality of input data (2, 3, 4)
      uint32_t node_count = 0;     // number of nodes in the dataset
      uint32_t v_size = 1;         // each node is a grid with v_size^dim points, with v_size = 1 this is a single point
      uint32_t num_components = 1; // each point has num_components values
    };

    struct Node
    {
      uint16_t transform_id; // id of the transformation that should be applied to this node
      uint8_t level;         // level of the node in the initial tree
      BrickInfo::Type type;  // type of the node, mostly makes sense for SDF nodes
      float average_val;     // average value of the node, must be set to 0 if compression doesn't use it
      const float *data;     // pointer to the node data, must have at least num_components * (v_size^dim) values
      float *desc;           // pointer to where the descriptor for this node should be written
    };

    // many rendering methods rely on uint16 values to store current depth, so it is a limit that is rather
    // hard to change
    static constexpr unsigned MAX_OCTREE_DEPTH = 16;

    // soft cap, increasing it will require more memory for storing subgroups and will probably be impractical
    // due to huge descriptor sizes (4913 for 17^3-point 3D node)
    // non-standard cases (e.g. 4D nodes, large transform subgroups) require even more memory
    static uint32_t get_max_v_size(const SdfDAGHeader &header);

    virtual ~IDescriptorMaker() { }
    virtual void calculate_decriptors(DatasetInfo dataset_info, const Node *nodes) = 0;
    virtual uint32_t get_descriptor_size(DatasetInfo dataset_info) const = 0;
  };

  class IDescriptorBinner
  {
  public:
    virtual ~IDescriptorBinner() { }
    virtual uint32_t calculate_bin_hash(const float *values, uint32_t size) const = 0;
  };

  struct ChannelCustomSettings
  {
    std::shared_ptr<nn_search::IDistanceFunction> dist_func = nullptr;
    std::shared_ptr<IDescriptorMaker>  descriptor_maker = nullptr;
    std::shared_ptr<IDescriptorBinner> descriptor_binner = nullptr;
    BranchDescriptor branch_descriptor = BranchDescriptor::DEFAULT;
    float importance = 1.0f; // how important is the error on this channel to the overall error during similarity compression
    std::vector<float> default_values; // size = num_components
  };

  struct Settings
  {
    OutputType output_type = OutputType::DAG8;
    uint32_t brick_size = 1; // size of the brick (default is 1, i.e. 2x2x2 values)
    uint32_t octree_levels_to_bricks = 0; // how many levels of octree are condensed into bricks (upsample_global_octree)
    
    EmbeddingType       embedding_type            = EmbeddingType::DIRECT;
    ClusteringAlgorithm clustering_algorithm      = ClusteringAlgorithm::INVERSE_REPLACEMENT;
    SearchAlgorithm     search_algorithm          = SearchAlgorithm::BALL_TREE;
    SimMetric           sim_metric                = SimMetric::EUCLIDEAN;
    SimMetric           sim_metric_channel        = SimMetric::EUCLIDEAN;
    CombinedLoss        combined_loss             = CombinedLoss::L2;
    InternalBrickUse    internal_brick_use        = InternalBrickUse::MERGE_ALL;
    BranchDescriptor    branch_descriptor         = BranchDescriptor::DEFAULT;
    BranchDescriptor    branch_descriptor_channel = BranchDescriptor::DEFAULT;
    uint32_t clustering_dataset_size_limit_GB = 4; // if dataset is larger than this, clustering will be done in chunks
    bool use_close_match_heuristic = false; // for INVERSE_REPLACEMENT only, close match heuristic

    bool  bricks_use_similarity_compression = false;
    bool  bricks_use_average_val_transform  = true; // subtract average value from brick values and store it in transformation
    float bricks_similarity_threshold = 0.0f; // set negative value to disable and use only target_count
    int   bricks_target_count = -1;           // set negative value to disable and use only similarity_threshold
    float surface_sensitivity = 1.0f;         // changes the sensitivity of the surface merging, default is 1
    TransformSubgroup bricks_transform_subgroup = TransformSubgroup::DEFAULT;

    bool  channels_use_similarity_compression = false;
    float channels_similarity_threshold = 0.0f; // set negative value to disable and use only target_count
    int   channels_target_count = -1;           // set negative value to disable and use only similarity_threshold

    bool  branches_use_similarity_compression = false;
    int   branches_similarity_compression_levels = 1; // how many levels of octree can be compressed with SCom
    float branches_similarity_threshold = 0.0f; // set negative value to disable and use only target_count
    int   branches_target_count = -1;           // set negative value to disable and use only similarity_threshold
    float branches_merge_surface_threshold = 0.01f; // if branch_descriptor == BranchDescriptor::SDF_MULTI_MERGE, this is the threshold for merging surfaces
    float branches_threshold_reduction_factor = 0.5f; // on each level, sim. threshold is multiplied by this factor and 2^dim
    TransformSubgroup branches_transform_subgroup = TransformSubgroup::DEFAULT;

    std::shared_ptr<nn_search::IDistanceFunction> custom_dist_func = nullptr;
    std::shared_ptr<IDescriptorMaker> custom_descriptor_maker = nullptr;
    std::shared_ptr<IDescriptorBinner> custom_descriptor_binner = nullptr;
    std::shared_ptr<nn_search::ILossFunction> custom_combined_loss_func = nullptr;
    std::shared_ptr<nn_search::ILossFunction> custom_branch_loss_func = nullptr;
    std::map<std::string, ChannelCustomSettings> custom_channel_settings;

    uint32_t bits_per_value = 8;
    bool separate_unique_bricks = true;
    bool support_channels = true;
    bool support_surfaces = true;
    bool support_multi_nodes = true;
    bool custom_max_val = false;//for quantization
    
    PackedBrickType     packed_brick_type     = PackedBrickType::FULL;
    PackedSurfaceLimit  packed_surface_limit  = PackedSurfaceLimit::RELAXED;
    PackedReferenceType packed_reference_type = PackedReferenceType::SHORT_6_8_18;
  };
}

// forces the caller to include serialization.o translation unit 
// and therefore register enums declared there. It is useful for
// some executable that work with blks and have to reason about
// them, but do not use the actual data (e.g. benchmark, testing).
extern void force_register_scom_enums();

void load_from_blk(scom2::Settings &settings, const Block *block);
void save_to_blk(const scom2::Settings &settings, Block *block);

namespace sdf_converter
{
  struct GlobalOctree;
  // no similarity compression, just convert one structure to another
  void global_octree_to_DAG8_direct(const sdf_converter::GlobalOctree &octree, SdfDAG &dag);

  //conversion + similarity compression
  void global_octree_to_DAG(const GlobalOctree &octree, SdfDAG &dag, scom2::Settings settings);

  SdfDAG create_sdf_dag(SparseOctreeSettings settings, scom2::Settings scom2_settings, const cmesh4::SimpleMesh &mesh);

  SCom2Tree create_scom2_tree(SparseOctreeSettings settings, scom2::Settings scom2_settings, const cmesh4::SimpleMesh &mesh);
}