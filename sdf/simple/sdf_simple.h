#pragma once
#include "sdf/simple/sdf_simple_shared.h"
#include "utils/misc/scene_common.h"
#include "blk/blk.h"
#include "core/scene_extension.h"

#include <filesystem>

struct SdfGrid
{
  uint3 size;
  std::vector<float> data; //size.x*size.y*size.z values 
};

struct GridSettings
{
  GridSettings() {};
  GridSettings(unsigned _size) { size = _size; }
  unsigned size = 16;
};

struct SdfSBS
{
  SdfSBSHeader header;
  std::vector<SdfSBSNode> nodes;
  std::vector<uint32_t> values;
  std::vector<float> values_f; //used with indexed SBS layouts, as it is easier to have float values as a separate vector
};

struct SdfGridView
{
  SdfGridView() = default;
  SdfGridView(const SdfGrid &grid)
  {
    size = grid.size;
    data = grid.data.data();
  }
  SdfGridView(uint3 a_size, const std::vector<float> &a_data)
  {
    size = a_size;
    data = a_data.data();
  }
  uint3 size;
  const float *data; //size.x*size.y*size.z values 
};

struct SdfFrameOctreeView
{
  SdfFrameOctreeView() = default;
  SdfFrameOctreeView(const std::vector<SdfFrameOctreeNode> &a_nodes)
  {
    size = a_nodes.size();
    nodes = a_nodes.data();
  }
  unsigned size;
  const SdfFrameOctreeNode *nodes;
};

struct SdfSVSView
{
  SdfSVSView() = default;
  SdfSVSView(const std::vector<SdfSVSNode> &a_nodes)
  {
    size = a_nodes.size();
    nodes = a_nodes.data();
  }
  unsigned size;
  const SdfSVSNode *nodes;
};

struct SdfSBSView
{
  SdfSBSView() = default;
  SdfSBSView(const SdfSBS &sbs)
  {
    header = sbs.header;
    size = sbs.nodes.size();
    nodes = sbs.nodes.data();
    values_count = sbs.values.size();
    values = sbs.values.data();
    values_f_count = sbs.values_f.size();
    values_f = sbs.values_f.data();
  }
  SdfSBSView(SdfSBSHeader a_header, const std::vector<SdfSBSNode> &a_nodes, 
             const std::vector<uint32_t> &a_values)
  {
    header = a_header;
    size = a_nodes.size();
    nodes = a_nodes.data();
    values_count = a_values.size();
    values = a_values.data();
    values_f_count = 0;
    values_f = nullptr;
  }

  SdfSBSView(SdfSBSHeader a_header, const std::vector<SdfSBSNode> &a_nodes,
             const std::vector<uint32_t> &a_values, const std::vector<float> &a_values_f)
  {
    header = a_header;
    size = a_nodes.size();
    nodes = a_nodes.data();
    values_count = a_values.size();
    values = a_values.data();
    values_f_count = a_values_f.size();
    values_f = a_values_f.data();
  }

  SdfSBSHeader header;
  unsigned size;
  const SdfSBSNode *nodes;
  unsigned values_count;
  const uint32_t *values;
  unsigned values_f_count;
  const float *values_f;
};

struct SdfFrameOctreeTexView
{
  SdfFrameOctreeTexView() = default;
  SdfFrameOctreeTexView(const std::vector<SdfFrameOctreeTexNode> &a_nodes)
  {
    size = a_nodes.size();
    nodes = a_nodes.data();
  }
  unsigned size;
  const SdfFrameOctreeTexNode *nodes;
};

void load_from_blk(GridSettings &settings, const Block *block);
void save_to_blk(const GridSettings &settings, Block *block);

void save_sdf_grid(const SdfGridView &scene, const std::string &path);
void load_sdf_grid(SdfGrid &scene, const std::string &path);
ModelInfo get_info_sdf_grid(const SdfGridView &scene);

void save_sdf_frame_octree(const SdfFrameOctreeView &scene, const std::string &path);
void load_sdf_frame_octree(std::vector<SdfFrameOctreeNode> &scene, const std::string &path);
ModelInfo get_info_sdf_frame_octree(const SdfFrameOctreeView &scene);

void save_sdf_SVS(const SdfSVSView &scene, const std::string &path);
void load_sdf_SVS(std::vector<SdfSVSNode> &scene, const std::string &path);
ModelInfo get_info_sdf_SVS(const SdfSVSView &scene);

void save_sdf_SBS(const SdfSBSView &scene, const std::filesystem::path &path);
void load_sdf_SBS(SdfSBS &scene, const std::filesystem::path &path);
ModelInfo get_info_sdf_SBS(const SdfSBSView &scene);

void save_sdf_frame_octree_tex(const SdfFrameOctreeTexView &scene, const std::string &path);
void load_sdf_frame_octree_tex(std::vector<SdfFrameOctreeTexNode> &scene, const std::string &path);
ModelInfo get_info_sdf_frame_octree_tex(const SdfFrameOctreeTexView &scene);

namespace LiteScene
{
  REGISTER_TYPE(TYPE_SDF_GRID,         SDFGridGeometry,        SdfGrid,                         "sdf_grid",         save_sdf_grid,         load_sdf_grid,         get_info_sdf_grid)
  REGISTER_TYPE(TYPE_SDF_FRAME_OCTREE, SDFFrameOctreeGeometry, std::vector<SdfFrameOctreeNode>, "sdf_frame_octree", save_sdf_frame_octree, load_sdf_frame_octree, get_info_sdf_frame_octree)
  REGISTER_TYPE(TYPE_SDF_SVS,          SDFSVSGeometry,         std::vector<SdfSVSNode>,         "sdf_svs",          save_sdf_SVS,          load_sdf_SVS,          get_info_sdf_SVS)
  REGISTER_TYPE(TYPE_SDF_SBS,          SDFSBSGeometry,         SdfSBS,                          "sdf_sbs",          save_sdf_SBS,          load_sdf_SBS,          get_info_sdf_SBS)
}