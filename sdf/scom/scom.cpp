#include "scom.h"
#include "similarity_compression.h"
#include "sdf/build/sparse_octree_builder.h"
#include <cassert>
#include <fstream>
#include <filesystem>

REGISTER_ENUM(SComClusteringAlgorithm,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"REPLACEMENT", (unsigned)scom::ClusteringAlgorithm::REPLACEMENT},
                     {"COMPONENTS_RECURSIVE_FILL", (unsigned)scom::ClusteringAlgorithm::COMPONENTS_RECURSIVE_FILL},
                     {"HIERARCHICAL", (unsigned)scom::ClusteringAlgorithm::HIERARCHICAL},
                 }; })());

REGISTER_ENUM(SComSearchAlgorithm,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"BALL_TREE", (unsigned)scom::SearchAlgorithm::BALL_TREE},
                     {"LINEAR_SEARCH", (unsigned)scom::SearchAlgorithm::LINEAR_SEARCH}
                 }; })());

REGISTER_ENUM(COctreeLeafType,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"COCTREE_LEAF_TYPE_NOT_A_LEAF", COCTREE_LEAF_TYPE_NOT_A_LEAF},
                     {"COCTREE_LEAF_TYPE_GRID", COCTREE_LEAF_TYPE_GRID},
                     {"COCTREE_LEAF_TYPE_BIT_PACK", COCTREE_LEAF_TYPE_BIT_PACK},
                     {"COCTREE_LEAF_TYPE_SLICES", COCTREE_LEAF_TYPE_SLICES},
                     {"COCTREE_USE_BEST_LEAF_TYPE", COCTREE_USE_BEST_LEAF_TYPE},
                 }; })());

void load_from_blk(COctreeV3Settings &settings, const Block *block)
{
  settings = COctreeV3Settings();

  settings.brick_size = block->get_int("brick_size", settings.brick_size);
  settings.brick_pad = block->get_int("brick_pad", settings.brick_pad);
  settings.bits_per_value = block->get_int("bits_per_value", settings.bits_per_value);
  settings.uv_size = block->get_int("uv_size", settings.uv_size);
  settings.sim_compression = block->get_int("sim_compression", settings.sim_compression);
  settings.default_leaf_type = block->get_enum("default_leaf_type", settings.default_leaf_type);
  settings.allow_fallback_to_unpacked_leaves = block->get_bool("allow_fallback_to_unpacked_leaves", settings.allow_fallback_to_unpacked_leaves);
  settings.use_lods = block->get_bool("use_lods", settings.use_lods);
  settings.print_build_stats = block->get_bool("print_build_stats", settings.print_build_stats);
}

void save_to_blk(const COctreeV3Settings &settings, Block *block)
{
  block->clear();

  block->add_int("brick_size", settings.brick_size);
  block->add_int("brick_pad", settings.brick_pad);
  block->add_int("bits_per_value", settings.bits_per_value);
  block->add_int("uv_size", settings.uv_size);
  block->add_int("sim_compression", settings.sim_compression);
  block->add_enum("default_leaf_type", "COctreeLeafType", settings.default_leaf_type);
  block->add_bool("allow_fallback_to_unpacked_leaves", settings.allow_fallback_to_unpacked_leaves);
  block->add_bool("use_lods", settings.use_lods);
  block->add_bool("print_build_stats", settings.print_build_stats);
}

void load_from_blk(scom::Settings &settings, const Block *block)
{
  settings = scom::Settings();

  settings.clustering_algorithm = (scom::ClusteringAlgorithm)block->get_enum("clustering_algorithm", (unsigned)settings.clustering_algorithm);
  settings.search_algorithm = (scom::SearchAlgorithm)block->get_enum("search_algorithm", (unsigned)settings.search_algorithm);
  settings.similarity_threshold = block->get_double("similarity_threshold", settings.similarity_threshold);
  settings.target_leaf_count = block->get_int("target_leaf_count", settings.target_leaf_count);
  settings.distance_importance = block->get_vec3("distance_importance", settings.distance_importance);
  settings.cluster_non_leafs = block->get_bool("cluster_non_leafs", settings.cluster_non_leafs);
}

void save_to_blk(const scom::Settings &settings, Block *block)
{
  block->clear();

  block->add_enum("clustering_algorithm", "SComClusteringAlgorithm", (unsigned)settings.clustering_algorithm);
  block->add_enum("search_algorithm", "SComSearchAlgorithm", (unsigned)settings.search_algorithm);
  block->add_double("similarity_threshold", settings.similarity_threshold);
  block->add_int("target_leaf_count", settings.target_leaf_count);
  block->add_vec3("distance_importance", settings.distance_importance);
}

void save_coctree_v3(const COctreeV3View &scene, const std::string &path)
{
  std::ofstream fs(path, std::ios::binary);
  fs.write((const char *)&COctreeV3::VERSION, sizeof(uint32_t));
  fs.write((const char *)&scene.header, sizeof(COctreeV3Header));
  fs.write((const char *)&scene.size, sizeof(uint32_t));
  fs.write((const char *)scene.data, scene.size * sizeof(uint32_t));
  fs.flush();
  fs.close();
}

void load_coctree_v3(COctreeV3 &scene, const std::string &path)
{
  std::ifstream fs(path, std::ios::binary);
  uint32_t version = 0;
  fs.read((char *)&version, sizeof(uint32_t));
  if (version != COctreeV3::VERSION) {
    printf("COctreeV3 version mismatch (save is version %u, current version is %u)\n",
           version, COctreeV3::VERSION);
  }
  fs.read((char *)&scene.header, sizeof(COctreeV3Header));
  uint32_t sz = 0;
  fs.read((char *)&sz, sizeof(uint32_t));
  scene.data.resize(sz);
  fs.read((char *)scene.data.data(), sz * sizeof(uint32_t));
  fs.close();
}

ModelInfo get_info_coctree_v3(const COctreeV3View &scene)
{
  ModelInfo info;

  info.name = "sdf_coctree_v3";
  info.bytesize = sizeof(uint32_t) + sizeof(COctreeV3Header) + 
                  scene.size * sizeof(uint32_t) + sizeof(uint32_t);
  info.num_primitives = scene.size;

  return info;
}

namespace sdf_converter
{
  COctreeV3 create_COctree_v3(SparseOctreeSettings settings, 
                              COctreeV3Settings co_settings, 
                              const cmesh4::SimpleMesh &mesh,
                              bool verbose)
  {
    //we must fill nodes on all levels to have good LODs
    settings.fill_all_nodes = settings.fill_all_nodes || co_settings.use_lods;
    co_settings.use_lods    = settings.fill_all_nodes || co_settings.use_lods;
    COctreeV3 coctree;
    GlobalOctree g;
    settings.brick_size = co_settings.brick_size;
    settings.brick_pad = co_settings.brick_pad;
    {
      auto plo = create_triangle_list_octree(mesh, PLOSettings(settings.depth, 0, 2 + co_settings.brick_pad/(float)co_settings.brick_size));
      mesh_octree_to_global_octree(settings, mesh, plo, g);
    }
    global_octree_to_COctreeV3(g, coctree, co_settings);
    
    return coctree;
  }

  COctreeV3 create_COctree_v3(SparseOctreeSettings settings, 
                              COctreeV3Settings co_settings, 
                              scom::Settings  scom_settings, 
                              const cmesh4::SimpleMesh &mesh,
                              bool verbose)
  {
    scom_settings.cluster_non_leafs = co_settings.use_lods;
    
    //we must fill nodes on all levels to have good LODs
    settings.fill_all_nodes = settings.fill_all_nodes || co_settings.use_lods;
    co_settings.use_lods    = settings.fill_all_nodes || co_settings.use_lods;
    
    COctreeV3 coctree;
    GlobalOctree g;
    settings.brick_size = co_settings.brick_size;
    settings.brick_pad = co_settings.brick_pad;
    {
      auto plo = create_triangle_list_octree(mesh, PLOSettings(settings.depth, 0, 2 + co_settings.brick_pad/(float)co_settings.brick_size));
      mesh_octree_to_global_octree(settings, mesh, plo, g);
    }
    global_octree_to_COctreeV3(g, coctree, co_settings, scom_settings);

    return coctree;
  }

  COctreeV3 create_COctree_v3(SparseOctreeSettings settings,
                              COctreeV3Settings co_settings,
                              scom::Settings scom_settings,
                              MultithreadedDistanceFunction sdf, 
                              unsigned max_threads,
                              bool verbose)
  {
    scom_settings.cluster_non_leafs = co_settings.use_lods;

    // we must fill nodes on all levels to have good LODs
    settings.fill_all_nodes = settings.fill_all_nodes || co_settings.use_lods;
    co_settings.use_lods    = settings.fill_all_nodes || co_settings.use_lods;

    COctreeV3 coctree;
    GlobalOctree g;
    settings.brick_size = co_settings.brick_size;
    settings.brick_pad = co_settings.brick_pad;

    sdf_to_global_octree(settings, sdf, max_threads, g);
    global_octree_to_COctreeV3(g, coctree, co_settings, scom_settings);

    return coctree;
  }
}