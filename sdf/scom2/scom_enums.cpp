#include "scom_enums.h"
#include "scom_builder.h"  

REGISTER_ENUM(ClusteringAlgorithm,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"REPLACEMENT", (unsigned)scom2::ClusteringAlgorithm::REPLACEMENT},
                     {"COMPONENTS_RECURSIVE_FILL", (unsigned)scom2::ClusteringAlgorithm::COMPONENTS_RECURSIVE_FILL},
                     {"HIERARCHICAL", (unsigned)scom2::ClusteringAlgorithm::HIERARCHICAL},
                     {"INVERSE_REPLACEMENT", (unsigned)scom2::ClusteringAlgorithm::INVERSE_REPLACEMENT},
                     {"LEADER", (unsigned)scom2::ClusteringAlgorithm::LEADER},
                 }; })());

REGISTER_ENUM(SearchAlgorithm,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"BALL_TREE", (unsigned)scom2::SearchAlgorithm::BALL_TREE},
                     {"LINEAR_SEARCH", (unsigned)scom2::SearchAlgorithm::LINEAR_SEARCH},
                     {"LSH", (unsigned)scom2::SearchAlgorithm::LSH},
                     {"EXACT_HASH", (unsigned)scom2::SearchAlgorithm::EXACT_HASH},
                     {"KD_TREE", (unsigned)scom2::SearchAlgorithm::KD_TREE},
                 }; })());

REGISTER_ENUM(SimMetric,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"EUCLIDEAN", (unsigned)scom2::SimMetric::EUCLIDEAN},
                     {"L1", (unsigned)scom2::SimMetric::L1},
                     {"CUSTOM", (unsigned)scom2::SimMetric::CUSTOM},
                 }; })());

REGISTER_ENUM(CombinedLoss,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"L1", (unsigned)scom2::CombinedLoss::L1},
                     {"L2", (unsigned)scom2::CombinedLoss::L2},
                     {"CUSTOM", (unsigned)scom2::CombinedLoss::CUSTOM},
                 }; })());

REGISTER_ENUM(OutputType,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"DAG8", (unsigned)scom2::OutputType::DAG8},
                     {"DAG64", (unsigned)scom2::OutputType::DAG64},
                 }; })());

REGISTER_ENUM(EmbeddingType,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"DIRECT", (unsigned)scom2::EmbeddingType::DIRECT},
                     {"NEURAL", (unsigned)scom2::EmbeddingType::NEURAL},
                     {"NO_EMBEDDING", (unsigned)scom2::EmbeddingType::NO_EMBEDDING},
                     {"TRIVIAL", (unsigned)scom2::EmbeddingType::TRIVIAL},
                     {"CUSTOM", (unsigned)scom2::EmbeddingType::CUSTOM},
                 }; })());

REGISTER_ENUM(PackedBrickType,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"BITMASK", (unsigned)scom2::PackedBrickType::BITMASK},
                     {"FULL", (unsigned)scom2::PackedBrickType::FULL},
                 }; })());

REGISTER_ENUM(PackedSurfaceLimit,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"STRICT", (unsigned)scom2::PackedSurfaceLimit::STRICT},
                     {"RELAXED", (unsigned)scom2::PackedSurfaceLimit::RELAXED},
                 }; })());

REGISTER_ENUM(PackedReferenceType,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"LONG_6_26_32", (unsigned)scom2::PackedReferenceType::LONG_6_26_32},
                     {"LONG_6_8_32", (unsigned)scom2::PackedReferenceType::LONG_6_8_32},
                     {"SHORT_6_8_18", (unsigned)scom2::PackedReferenceType::SHORT_6_8_18},
                     {"LONG_9_23_32", (unsigned)scom2::PackedReferenceType::LONG_9_23_32},
                 }; })());

REGISTER_ENUM(InternalBrickUse,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"DROP", (unsigned)scom2::InternalBrickUse::DROP},
                     {"MERGE_ALL", (unsigned)scom2::InternalBrickUse::MERGE_ALL},
                     {"PROPER_CLUSTERING", (unsigned)scom2::InternalBrickUse::PROPER_CLUSTERING},
                 }; })());

REGISTER_ENUM(BranchDescriptor,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"DEFAULT", (unsigned)scom2::BranchDescriptor::DEFAULT},
                     {"SIMPLE_MERGE", (unsigned)scom2::BranchDescriptor::SIMPLE_MERGE},
                     {"SDF_MERGE", (unsigned)scom2::BranchDescriptor::SDF_MERGE},
                     {"SDF_MULTI_MERGE", (unsigned)scom2::BranchDescriptor::SDF_MULTI_MERGE},
                     {"CONCATENATE", (unsigned)scom2::BranchDescriptor::CONCATENATE},
                     {"VOXEL_CONCATENATE", (unsigned)scom2::BranchDescriptor::VOXEL_CONCATENATE},
                 }; })());

REGISTER_ENUM(TransformSubgroup,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"DEFAULT", (unsigned)scom2::TransformSubgroup::DEFAULT},
                     {"LARGE_DIAGONAL", (unsigned)scom2::TransformSubgroup::LARGE_DIAGONAL},
                     {"SIDE_SWAP", (unsigned)scom2::TransformSubgroup::SIDE_SWAP},
                     {"ALL_TRANSPOSITIONS", (unsigned)scom2::TransformSubgroup::ALL_TRANSPOSITIONS},
                     {"NO_TRANSFORMS", (unsigned)scom2::TransformSubgroup::NO_TRANSFORMS},
                 }; })());

void load_from_blk(scom2::Settings &settings, const Block *block)
{
  settings = scom2::Settings();

  settings.output_type = (scom2::OutputType)block->get_enum("output_type", (unsigned)settings.output_type);
  settings.brick_size = block->get_int("brick_size", settings.brick_size);
  settings.octree_levels_to_bricks = block->get_int("octree_levels_to_bricks", settings.octree_levels_to_bricks);
  settings.embedding_type = (scom2::EmbeddingType)block->get_enum("embedding_type", (unsigned)settings.embedding_type);
  settings.clustering_algorithm = (scom2::ClusteringAlgorithm)block->get_enum("clustering_algorithm", (unsigned)settings.clustering_algorithm);
  settings.search_algorithm = (scom2::SearchAlgorithm)block->get_enum("search_algorithm", (unsigned)settings.search_algorithm);
  settings.sim_metric = (scom2::SimMetric)block->get_enum("sim_metric", (unsigned)settings.sim_metric);

  settings.bricks_use_similarity_compression = block->get_bool("bricks_use_similarity_compression", settings.bricks_use_similarity_compression);
  settings.bricks_similarity_threshold = block->get_double("bricks_similarity_threshold", settings.bricks_similarity_threshold);
  settings.bricks_target_count = block->get_int("bricks_target_count", settings.bricks_target_count);
  settings.bricks_transform_subgroup = (scom2::TransformSubgroup)block->get_enum("bricks_transform_subgroup", (unsigned)settings.bricks_transform_subgroup);
  settings.surface_sensitivity = block->get_double("surface_sensitivity", settings.surface_sensitivity);

  settings.channels_use_similarity_compression = block->get_bool("channels_use_similarity_compression", settings.channels_use_similarity_compression);
  settings.channels_similarity_threshold = block->get_double("channels_similarity_threshold", settings.channels_similarity_threshold);
  settings.channels_target_count = block->get_int("channels_target_count", settings.channels_target_count);

  settings.branches_use_similarity_compression = block->get_bool("branches_use_similarity_compression", settings.branches_use_similarity_compression);
  settings.branches_similarity_compression_levels = block->get_int("branches_similarity_compression_levels", settings.branches_similarity_compression_levels);
  settings.branches_similarity_threshold = block->get_double("branches_similarity_threshold", settings.branches_similarity_threshold);
  settings.branches_target_count = block->get_int("branches_target_count", settings.branches_target_count);
  settings.branches_merge_surface_threshold = block->get_double("branches_merge_surface_threshold", settings.branches_merge_surface_threshold);
  settings.branches_threshold_reduction_factor = block->get_double("branches_threshold_reduction_factor", settings.branches_threshold_reduction_factor);
  settings.branches_transform_subgroup = (scom2::TransformSubgroup)block->get_enum("branches_transform_subgroup", (unsigned)settings.branches_transform_subgroup);

  settings.bits_per_value = block->get_int("bits_per_value", settings.bits_per_value);
  settings.separate_unique_bricks = block->get_bool("separate_unique_bricks", settings.separate_unique_bricks);
  settings.support_channels = block->get_bool("support_channels", settings.support_channels);
  settings.support_surfaces = block->get_bool("support_surfaces", settings.support_surfaces);
  settings.support_multi_nodes = block->get_bool("support_multi_nodes", settings.support_multi_nodes);

  settings.custom_max_val = block->get_bool("custom_max_val", settings.custom_max_val);

  settings.use_close_match_heuristic = block->get_bool("use_close_match_heuristic", settings.use_close_match_heuristic);
  settings.bricks_use_average_val_transform = block->get_bool("bricks_use_average_val_transform", settings.bricks_use_average_val_transform);
  settings.clustering_dataset_size_limit_GB = block->get_int("clustering_dataset_size_limit_GB", settings.clustering_dataset_size_limit_GB);
  settings.packed_brick_type = (scom2::PackedBrickType)block->get_enum("packed_brick_type", (unsigned)settings.packed_brick_type);
  settings.packed_surface_limit = (scom2::PackedSurfaceLimit)block->get_enum("packed_surface_limit", (unsigned)settings.packed_surface_limit);
  settings.packed_reference_type = (scom2::PackedReferenceType)block->get_enum("packed_reference_type", (unsigned)settings.packed_reference_type);
  settings.internal_brick_use = (scom2::InternalBrickUse)block->get_enum("internal_brick_use", (unsigned)settings.internal_brick_use);
  settings.branch_descriptor = (scom2::BranchDescriptor)block->get_enum("branch_descriptor", (unsigned)settings.branch_descriptor);
}

void save_to_blk(const scom2::Settings &settings, Block *block)
{
  block->clear();

  block->add_enum("output_type", "OutputType", (unsigned)settings.output_type);
  block->add_int("brick_size", settings.brick_size);
  block->add_int("octree_levels_to_bricks", settings.octree_levels_to_bricks);
  block->add_enum("embedding_type", "EmbeddingType", (unsigned)settings.embedding_type);
  block->add_enum("clustering_algorithm", "ClusteringAlgorithm", (unsigned)settings.clustering_algorithm);
  block->add_enum("search_algorithm", "SearchAlgorithm", (unsigned)settings.search_algorithm);
  block->add_enum("sim_metric", "SimMetric", (unsigned)settings.sim_metric);

  block->add_bool("bricks_use_similarity_compression", settings.bricks_use_similarity_compression);
  block->add_double("bricks_similarity_threshold", settings.bricks_similarity_threshold);
  block->add_int("bricks_target_count", settings.bricks_target_count);
  block->add_enum("bricks_transform_subgroup", "TransformSubgroup", (unsigned)settings.bricks_transform_subgroup);
  block->add_double("surface_sensitivity", settings.surface_sensitivity);

  block->add_bool("channels_use_similarity_compression", settings.channels_use_similarity_compression);
  block->add_double("channels_similarity_threshold", settings.channels_similarity_threshold);
  block->add_int("channels_target_count", settings.channels_target_count);

  block->add_bool("branches_use_similarity_compression", settings.branches_use_similarity_compression);
  block->add_int("branches_similarity_compression_levels", settings.branches_similarity_compression_levels);
  block->add_double("branches_similarity_threshold", settings.branches_similarity_threshold);
  block->add_int("branches_target_count", settings.branches_target_count);
  block->add_double("branches_merge_surface_threshold", settings.branches_merge_surface_threshold);
  block->add_double("branches_threshold_reduction_factor", settings.branches_threshold_reduction_factor);
  block->add_enum("branches_transform_subgroup", "TransformSubgroup", (unsigned)settings.branches_transform_subgroup);

  block->add_int("bits_per_value", settings.bits_per_value);
  block->set_bool("separate_unique_bricks", settings.separate_unique_bricks);
  block->set_bool("support_channels", settings.support_channels);
  block->set_bool("support_surfaces", settings.support_surfaces);
  block->set_bool("support_multi_nodes", settings.support_multi_nodes);

  block->set_bool("custom_max_val", settings.custom_max_val);
  
  block->add_bool("use_close_match_heuristic", settings.use_close_match_heuristic);
  block->add_bool("bricks_use_average_val_transform", settings.bricks_use_average_val_transform);
  block->add_int("clustering_dataset_size_limit_GB", settings.clustering_dataset_size_limit_GB);
  block->add_enum("packed_brick_type", "PackedBrickType", (unsigned)settings.packed_brick_type);
  block->add_enum("packed_surface_limit", "PackedSurfaceLimit", (unsigned)settings.packed_surface_limit);
  block->add_enum("packed_reference_type", "PackedReferenceType", (unsigned)settings.packed_reference_type);
  block->add_enum("internal_brick_use", "InternalBrickUse", (unsigned)settings.internal_brick_use);
  block->add_enum("branch_descriptor", "BranchDescriptor", (unsigned)settings.branch_descriptor);
}

void force_register_scom_enums()
{

}