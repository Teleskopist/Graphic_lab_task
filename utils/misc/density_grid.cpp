#include "density_grid.h"
#include "scene_common.h"
#include <fstream>

void save_density_grid(const DensityGrid &scene, const std::string &path)
{
  std::ofstream fs(path, std::ios::binary);
  fs.write((const char *)&scene.size, 3 * sizeof(unsigned));
  fs.write((const char *)scene.data.data(), scene.size.x*scene.size.y*scene.size.z * sizeof(float));
  fs.flush();
  fs.close();
}

void load_density_grid(DensityGrid &scene, const std::string &path)
{
  std::ifstream fs(path, std::ios::binary);
  fs.read((char *)&scene.size, 3 * sizeof(unsigned));
  scene.data.resize(scene.size.x*scene.size.y*scene.size.z);
  fs.read((char *)scene.data.data(), scene.size.x*scene.size.y*scene.size.z * sizeof(float));
  fs.close();
}

void save_density_grid(const ColorDensityGrid &scene, const std::string &path)
{
  std::ofstream fs(path, std::ios::binary);
  fs.write((const char *)&scene.size, 3 * sizeof(unsigned));
  fs.write((const char *)scene.data.data(), scene.size.x*scene.size.y*scene.size.z * sizeof(LiteMath::float4));
  fs.flush();
  fs.close();
}

void load_density_grid(ColorDensityGrid &scene, const std::string &path)
{
  std::ifstream fs(path, std::ios::binary);
  fs.read((char *)&scene.size, 3 * sizeof(unsigned));
  scene.data.resize(scene.size.x*scene.size.y*scene.size.z);
  fs.read((char *)scene.data.data(), scene.size.x*scene.size.y*scene.size.z * sizeof(LiteMath::float4));
  fs.close();
}

void save_density_grid(const DensityMultiGrid &scene, const std::string &path)
{
  std::ofstream fs(path, std::ios::binary);
  fs.write((const char *)&scene.frames, sizeof(unsigned));
  fs.write((const char *)scene.timestamps.data(), scene.frames * sizeof(float));
  fs.write((const char *)&scene.size, 3 * sizeof(unsigned));
  fs.write((const char *)scene.data.data(), scene.frames*scene.size.x*scene.size.y*scene.size.z * sizeof(float));
  fs.flush();
  fs.close();
}

void load_density_grid(DensityMultiGrid &scene, const std::string &path)
{
  std::ifstream fs(path, std::ios::binary);
  fs.read((char *)&scene.frames, sizeof(unsigned));
  scene.timestamps.resize(scene.frames);
  fs.read((char *)scene.timestamps.data(), scene.frames * sizeof(float));
  fs.read((char *)&scene.size, 3 * sizeof(unsigned));
  scene.data.resize(scene.frames*scene.size.x*scene.size.y*scene.size.z);
  fs.read((char *)scene.data.data(), scene.frames*scene.size.x*scene.size.y*scene.size.z * sizeof(float));
  fs.close();
}

void save_density_grid(const ColorDensityMultiGrid &scene, const std::string &path)
{
  std::ofstream fs(path, std::ios::binary);
  fs.write((const char *)&scene.frames, sizeof(unsigned));
  fs.write((const char *)scene.timestamps.data(), scene.frames * sizeof(float));
  fs.write((const char *)&scene.size, 3 * sizeof(unsigned));
  fs.write((const char *)scene.data.data(), scene.frames*scene.size.x*scene.size.y*scene.size.z * sizeof(LiteMath::float4));
  fs.flush();
  fs.close();
}
void load_density_grid(ColorDensityMultiGrid &scene, const std::string &path)
{
  std::ifstream fs(path, std::ios::binary);
  fs.read((char *)&scene.frames, sizeof(unsigned));
  scene.timestamps.resize(scene.frames);
  fs.read((char *)scene.timestamps.data(), scene.frames * sizeof(float));
  fs.read((char *)&scene.size, 3 * sizeof(unsigned));
  scene.data.resize(scene.frames*scene.size.x*scene.size.y*scene.size.z);
  fs.read((char *)scene.data.data(), scene.frames*scene.size.x*scene.size.y*scene.size.z * sizeof(LiteMath::float4));
  fs.close();
}

struct ModelInfo;
ModelInfo get_info_density_grid(const DensityGrid &scene)
{
  ModelInfo info;
  info.name = "density_grid";
  info.bytesize = scene.size.x*scene.size.y*scene.size.z * sizeof(float);
  info.num_primitives = 1;

  return info;
}

ModelInfo get_info_density_grid(const ColorDensityGrid &scene)
{
  ModelInfo info;
  info.name = "color_density_grid";
  info.bytesize = scene.size.x*scene.size.y*scene.size.z * sizeof(LiteMath::float4);
  info.num_primitives = 1;

  return info;
}

ModelInfo get_info_density_grid(const DensityMultiGrid &scene)
{
  ModelInfo info;
  info.name = "density_multi_grid";
  info.bytesize = scene.frames*scene.size.x*scene.size.y*scene.size.z * sizeof(float);
  info.num_primitives = 1;

  return info;
}

ModelInfo get_info_density_grid(const ColorDensityMultiGrid &scene)
{
  ModelInfo info;
  info.name = "color_density_multi_grid";
  info.bytesize = scene.frames*scene.size.x*scene.size.y*scene.size.z * sizeof(LiteMath::float4);
  info.num_primitives = 1;
  
  return info;
}