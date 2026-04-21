#include "global_octree.h"

REGISTER_ENUM(VoxelMetric,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"BASIC", (unsigned)SparseOctreeSettings::VoxelMetric::BASIC},
                     {"SAMPLES", (unsigned)SparseOctreeSettings::VoxelMetric::SAMPLES},
                     {"MULTI", (unsigned)SparseOctreeSettings::VoxelMetric::MULTI},
                     {"REFERENCE_IOU", (unsigned)SparseOctreeSettings::VoxelMetric::REFERENCE_IOU},
                 }; })());

REGISTER_ENUM(LocalDepthMethod,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"PRIMITIVES", (unsigned)SparseOctreeSettings::LocalDepthMethod::PRIMITIVES},
                     {"MATERIALS", (unsigned)SparseOctreeSettings::LocalDepthMethod::MATERIALS},
                     {"AV_SQUARE", (unsigned)SparseOctreeSettings::LocalDepthMethod::AV_SQUARE},
                     {"MIN_SQUARE", (unsigned)SparseOctreeSettings::LocalDepthMethod::MIN_SQUARE},
                     {"MAX_SQUARE", (unsigned)SparseOctreeSettings::LocalDepthMethod::MAX_SQUARE},
                     {"COMPONENTS", (unsigned)SparseOctreeSettings::LocalDepthMethod::COMPONENTS},
                     {"SURFACES", (unsigned)SparseOctreeSettings::LocalDepthMethod::SURFACES},
                     {"ABS_METRIC", (unsigned)SparseOctreeSettings::LocalDepthMethod::ABS_METRIC},
                     {"SIGN_ANALYSIS", (unsigned)SparseOctreeSettings::LocalDepthMethod::SIGN_ANALYSIS},
                     {"NORMAL_ERROR", (unsigned)SparseOctreeSettings::LocalDepthMethod::NORMAL_ERROR},
                 }; })());

void load_from_blk(SparseOctreeSettings &settings, const Block *block, const Block *another_block)
{
  if (block)
  {
    settings.local_inc_depth = block->get_int("local_add_depth", settings.local_inc_depth);
    settings.local_depth_thr = block->get_double("local_depth_thr", settings.local_depth_thr);
    settings.split_thr = block->get_double("split_thr", settings.split_thr);
    settings.channel_split_thr = block->get_double("channel_split_thr", settings.channel_split_thr);
    settings.voxel_metric = (SparseOctreeSettings::VoxelMetric)block->get_enum("voxel_metric", (unsigned int)settings.voxel_metric);
    settings.local_depth_method = (SparseOctreeSettings::LocalDepthMethod)block->get_enum("local_depth_method", (unsigned int)settings.local_depth_method);
  }

  if (another_block)
  {
    settings.use_point_cloud = another_block->get_bool("point_cloud", settings.use_point_cloud);
    settings.points_count = another_block->get_int("points_count", settings.points_count);
    settings.max_iterations = another_block->get_int("max_iters", settings.max_iterations);
    settings.inlier_threshold = another_block->get_double("inlier_thr", settings.inlier_threshold);
    settings.enough_points_percent = another_block->get_double("enough_pts_perc", settings.enough_points_percent);
    settings.minimum_iterations = another_block->get_double("min_iters_perc", settings.minimum_iterations);
    settings.learning_rate = another_block->get_double("learning_rate", settings.learning_rate);
    settings.gradient_steps = another_block->get_int("gradient_steps", settings.gradient_steps);
    settings.momentum = another_block->get_double("momentum", settings.momentum);
    settings.decay_rate = another_block->get_double("decay_rate", settings.decay_rate);
  }
}

void save_to_blk(const SparseOctreeSettings &settings, Block *block, Block *another_block)
{
  block->clear();
  another_block->clear();

  block->add_int("local_add_depth", settings.local_inc_depth);
  block->add_double("local_depth_thr", settings.local_depth_thr);
  block->add_double("split_thr", settings.split_thr);
  block->add_double("channel_split_thr", settings.channel_split_thr);

  another_block->add_bool("point_cloud", settings.use_point_cloud);
  another_block->add_int("points_count", settings.points_count);
  another_block->add_int("max_iters", settings.max_iterations);
  another_block->add_double("inlier_thr", settings.inlier_threshold);
  another_block->add_double("enough_pts_perc", settings.enough_points_percent);
  another_block->add_double("min_iters_perc", settings.minimum_iterations);
  another_block->add_double("learning_rate", settings.learning_rate);
  another_block->add_int("gradient_steps", settings.gradient_steps);
  another_block->add_double("momentum", settings.momentum);
  another_block->add_double("decay_rate", settings.decay_rate);
  
  block->add_enum("voxel_metric", "VoxelMetric", (unsigned int)settings.voxel_metric);
  block->add_enum("local_depth_method", "LocalDepthMethod", (unsigned int)settings.local_depth_method);
}

void load_from_blk(SparseOctreeSettings &settings, const Block *block)
{
  settings = SparseOctreeSettings();

  settings.brick_size = block->get_int("brick_size", settings.brick_size);
  settings.brick_pad = block->get_int("brick_pad", settings.brick_pad);
  settings.depth = block->get_int("depth", settings.depth);
  settings.min_depth = block->get_int("min_depth", settings.min_depth);
  settings.internal_depth = block->get_int("internal_depth", settings.internal_depth);

  settings.fill_all_nodes = block->get_bool("fill_all_nodes", settings.fill_all_nodes);
  settings.fill_internal_volume = block->get_bool("fill_internal_volume", settings.fill_internal_volume);
  settings.allow_multi_nodes = block->get_bool("allow_multi_nodes", settings.allow_multi_nodes);
  settings.allow_nodes_refill = block->get_bool("allow_nodes_refill", settings.allow_nodes_refill);
  settings.advanced_LOD_generation = block->get_bool("advanced_LOD_generation", settings.advanced_LOD_generation);
  settings.sign_analysis = block->get_bool("sign_analysis", settings.sign_analysis);
  settings.use_pruning = block->get_bool("use_pruning", settings.use_pruning);
  settings.calculate_mat_id = block->get_bool("calculate_mat_id", settings.calculate_mat_id);
  settings.calculate_tc = block->get_bool("calculate_tc", settings.calculate_tc);
  settings.allow_early_stop = block->get_bool("allow_early_stop", settings.allow_early_stop);

  load_from_blk(settings, block->get_block("octree_advanced_settings"), block->get_block("octree_point_cloud_settings"));
}

void save_to_blk(const SparseOctreeSettings &settings, Block *block)
{
  block->clear();

  block->add_int("brick_size", settings.brick_size);
  block->add_int("brick_pad", settings.brick_pad);
  block->add_int("depth", settings.depth);
  block->add_int("min_depth", settings.min_depth);
  block->add_int("internal_depth", settings.internal_depth);
  
  block->add_bool("fill_all_nodes", settings.fill_all_nodes);
  block->add_bool("fill_internal_volume", settings.fill_internal_volume);
  block->add_bool("allow_multi_nodes", settings.allow_multi_nodes);
  block->add_bool("allow_nodes_refill", settings.allow_nodes_refill);
  block->add_bool("advanced_LOD_generation", settings.advanced_LOD_generation);
  block->add_bool("sign_analysis", settings.sign_analysis);
  block->add_bool("use_pruning", settings.use_pruning);
  block->add_bool("calculate_tc", settings.calculate_tc);
  block->add_bool("calculate_mat_id", settings.calculate_mat_id);
  block->add_bool("allow_early_stop", settings.allow_early_stop);

  Block ch_b, another_ch_b;
  save_to_blk(settings, &ch_b, &another_ch_b);
  block->add_block("octree_advanced_settings", &ch_b);
  block->add_block("octree_point_cloud_settings", &another_ch_b);
}

namespace sdf_converter
{
  void print_octree_build_stat(const GlobalOctreeBuildStat &stat)
  {
    printf("depth:          %d\n", stat.settings.depth);
    printf("min_depth:      %d\n", stat.settings.min_depth);
    printf("fill_all_nodes: %s\n", stat.settings.fill_all_nodes ? "true" : "false");
    if (!stat.settings.use_pruning && !stat.settings.allow_early_stop)
      printf("no early stop or pruning\n");
    else if (stat.settings.use_pruning && !stat.settings.allow_early_stop)
      printf("use pruning (thr = %f)\n", stat.settings.split_thr);
    else if (!stat.settings.use_pruning && stat.settings.allow_early_stop)
      printf("use early stop (thr = %f)\n", stat.settings.split_thr);
    else // if (stat.settings.use_pruning && stat.settings.allow_early_stop)
      printf("use early stop and pruning (thr = %f)\n", stat.settings.split_thr);
    printf("active_nodes:   %d\n", stat.active_nodes);
    printf("build prim. octree:     %.1f\n", stat.time_build_PLO);
    printf("build flooded octree:   %.1f\n", stat.time_build_flooded_octree);
    printf("sign analysis:          %.1f\n", stat.time_sign_analysis);
    printf("build global octree:    %.1f\n", stat.time_calculate_distances + stat.time_decide_to_divide + stat.time_mark_empty_nodes +
                                                 stat.time_eliminate_empty_nodes + stat.time_pruning);
    printf("--calculate_distances:  --%.1f\n", stat.time_calculate_distances);
    printf("--decide_to_divide:     --%.1f\n", stat.time_decide_to_divide);
    printf("--mark_empty_nodes:     --%.1f\n", stat.time_mark_empty_nodes);
    printf("--eliminate_empty_nodes:--%.1f\n", stat.time_eliminate_empty_nodes);
    printf("--pruning:              --%.1f\n", stat.time_pruning);
  }
}