#pragma once
#include "sdf/common/global_octree.h"

/* Main scene clas from our app. Contains all the original
models, textures and SDF-based LODs for them along with
additional information for renderer on how to use it
*/
struct CompoundScene
{
  std::vector<cmesh4::SimpleMesh> compounds;
  std::vector<sdf_converter::GlobalOctree> octrees;
  std::vector<sdf_converter::GlobalOctreeDebugInfo> debug_infos;
};

namespace scene_converter
{
  enum class LevelDecisionFunction
  {
    AVERAGE_POLYGON_SIZE,
    MAX_POLYGON_SIZE,
  };

  enum class GroupingQualityFunction
  {
    AABB_TOTAL_VOLUME,
  };

  enum class GroupingAlgorithm
  {
    RANDOM_SEEDS,
  };

  struct Settings
  {
    unsigned target_compound_size = 1'000'000'000; // desired number of triangles in each compound. If it is too large, only one compound will be created
    unsigned compound_levels = 1; // number of levels in compound hierarchy (1 - all compounds are the same, 2 - there are large and small compounds)
    float levels_size_ratio  = 5; // ratio between number of compounds on each level, level 0 has the least number of compounds
    
    unsigned max_detail_increase = 0; // small parts are converted to SDF with higher detalization, with voxel depth up to 
                                      // octree_settings.depth + max_detail_increase.
    unsigned union_depth_parts = 0;
    float union_threshold = 0.0;
    
    bool is_vox_merge = true;     // should we try to merge list of voxels inside each node or not
    bool save_debug_info = false; // should we save debug info for each compound

    SparseOctreeSettings octree_settings;
    LevelDecisionFunction level_decision_function = LevelDecisionFunction::AVERAGE_POLYGON_SIZE;
    GroupingQualityFunction grouping_quality_function = GroupingQualityFunction::AABB_TOTAL_VOLUME;
    GroupingAlgorithm grouping_algorithm = GroupingAlgorithm::RANDOM_SEEDS;
  };

  // Converts array of cmesh4::SimpleMesh to CompoundScene
  // Array is expected to be a set of parts from a CAD model
  void convert_CAD_parts_array(const std::vector<cmesh4::SimpleMesh> &parts, Settings settings, CompoundScene &out_scene);
};

void load_from_blk(scene_converter::Settings &settings, const Block *block);
void save_to_blk(const scene_converter::Settings &settings, Block *block);