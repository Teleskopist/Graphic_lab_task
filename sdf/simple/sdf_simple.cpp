#include "sdf_simple.h"
#include "blk/blk.h"
#include <cassert>
#include <fstream>
#include <filesystem>

void load_from_blk(GridSettings &settings, const Block *block)
{
  settings = GridSettings();

  settings.size = block->get_int("size", settings.size);
}
void save_to_blk(const GridSettings &settings, Block *block)
{
  block->clear();

  block->add_int("size", settings.size);
}

void save_sdf_grid(const SdfGridView &scene, const std::string &path)
{
  std::ofstream fs(path, std::ios::binary);
  fs.write((const char *)&scene.size, 3 * sizeof(unsigned));
  fs.write((const char *)scene.data, scene.size.x*scene.size.y*scene.size.z * sizeof(float));
  fs.flush();
  fs.close();
}

void load_sdf_grid(SdfGrid &scene, const std::string &path)
{
  std::ifstream fs(path, std::ios::binary);
  fs.read((char *)&scene.size, 3 * sizeof(unsigned));
  scene.data.resize(scene.size.x*scene.size.y*scene.size.z);
  fs.read((char *)scene.data.data(), scene.size.x*scene.size.y*scene.size.z * sizeof(float));
  fs.close();
}

ModelInfo get_info_sdf_grid(const SdfGridView &scene)
{
  ModelInfo info;
  
  info.name = "sdf_grid";
  info.bytesize = scene.size.x*scene.size.y*scene.size.z * sizeof(float) + sizeof(unsigned);
  info.num_primitives = 1;

  return info;
}

void save_sdf_frame_octree(const SdfFrameOctreeView &scene, const std::string &path)
{
  std::ofstream fs(path, std::ios::binary);
  fs.write((const char *)&scene.size, sizeof(unsigned));
  fs.write((const char *)scene.nodes, scene.size * sizeof(SdfFrameOctreeNode));
  fs.flush();
  fs.close();
}

void load_sdf_frame_octree(std::vector<SdfFrameOctreeNode> &scene, const std::string &path)
{
  std::ifstream fs(path, std::ios::binary);
  unsigned sz = 0;
  fs.read((char *)&sz, sizeof(unsigned));
  scene.resize(sz);
  fs.read((char *)scene.data(), scene.size() * sizeof(SdfFrameOctreeNode));
  fs.close();
}

ModelInfo get_info_sdf_frame_octree(const SdfFrameOctreeView &scene)
{
  ModelInfo info;

  info.name = "sdf_frame_octree";
  info.bytesize = scene.size * sizeof(SdfFrameOctreeNode) + sizeof(unsigned);
  info.num_primitives = scene.size;

  return info;
}

void save_sdf_SVS(const SdfSVSView &scene, const std::string &path)
{
  std::ofstream fs(path, std::ios::binary);
  fs.write((const char *)&scene.size, sizeof(unsigned));
  fs.write((const char *)scene.nodes, scene.size * sizeof(SdfSVSNode));
  fs.flush();
  fs.close();
}

void load_sdf_SVS(std::vector<SdfSVSNode> &scene, const std::string &path)
{
  std::ifstream fs(path, std::ios::binary);
  unsigned sz = 0;
  fs.read((char *)&sz, sizeof(unsigned));
  scene.resize(sz);
  fs.read((char *)scene.data(), scene.size() * sizeof(SdfSVSNode));
  fs.close();
}

ModelInfo get_info_sdf_SVS(const SdfSVSView &scene)
{
  ModelInfo info;

  info.name = "sdf_svs";
  info.bytesize = scene.size * sizeof(SdfSVSNode) + sizeof(unsigned);
  info.num_primitives = scene.size;

  return info;
}

void save_sdf_SBS(const SdfSBSView &scene, const std::filesystem::path &path)
{
  std::ofstream fs(path.string(), std::ios::binary);
  fs.write((const char *)&scene.header, sizeof(SdfSBSHeader));
  fs.write((const char *)&scene.size, sizeof(unsigned));
  fs.write((const char *)scene.nodes, scene.size * sizeof(SdfSBSNode));
  fs.write((const char *)&scene.values_count, sizeof(unsigned));
  fs.write((const char *)scene.values, scene.values_count * sizeof(unsigned));
  fs.write((const char *)&scene.values_f_count, sizeof(unsigned));
  if (scene.values_f_count > 0)
    fs.write((const char *)scene.values_f, scene.values_f_count * sizeof(float));
  fs.flush();
  fs.close();
}

void  load_sdf_SBS(SdfSBS &scene, const std::filesystem::path &path)
{
  std::ifstream fs(path.string(), std::ios::binary);
  fs.read((char *)&scene.header, sizeof(SdfSBSHeader));
  unsigned sz, cnt, cnt_f;
  fs.read((char *)&sz, sizeof(unsigned));
  scene.nodes.resize(sz);
  fs.read((char *)scene.nodes.data(), sz * sizeof(SdfSBSNode));
  fs.read((char *)&cnt, sizeof(unsigned));
  scene.values.resize(cnt);
  fs.read((char *)scene.values.data(), cnt * sizeof(unsigned));
  fs.read((char *)&cnt_f, sizeof(unsigned));
  if (cnt_f > 0)
  {
    scene.values_f.resize(cnt_f);
    fs.read((char *)scene.values_f.data(), cnt_f * sizeof(float));
  }
  fs.close();
}

ModelInfo get_info_sdf_SBS(const SdfSBSView &scene)
{
  ModelInfo info;

  info.name = "sdf_sbs";
  info.bytesize = sizeof(SdfSBSHeader) + 
                  scene.size * sizeof(SdfSBSNode) + sizeof(unsigned) + 
                  scene.values_count * sizeof(unsigned);
  info.num_primitives = scene.size;

  return info;
}

void save_sdf_frame_octree_tex(const SdfFrameOctreeTexView &scene, const std::string &path)
{
  std::ofstream fs(path, std::ios::binary);
  fs.write((const char *)&scene.size, sizeof(unsigned));
  fs.write((const char *)scene.nodes, scene.size * sizeof(SdfFrameOctreeTexNode));
  fs.flush();
  fs.close();
}

void load_sdf_frame_octree_tex(std::vector<SdfFrameOctreeTexNode> &scene, const std::string &path)
{
  std::ifstream fs(path, std::ios::binary);
  unsigned sz = 0;
  fs.read((char *)&sz, sizeof(unsigned));
  scene.resize(sz);
  fs.read((char *)scene.data(), scene.size() * sizeof(SdfFrameOctreeTexNode));
  fs.close();
}

ModelInfo get_info_sdf_frame_octree_tex(const SdfFrameOctreeTexView &scene)
{
  ModelInfo info;

  info.name = "sdf_frame_octree_tex";
  info.bytesize = scene.size * sizeof(SdfFrameOctreeTexNode) + sizeof(unsigned);
  info.num_primitives = scene.size;

  return info;
}