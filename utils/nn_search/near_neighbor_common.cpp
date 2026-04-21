#include "near_neighbor_common.h"
#include <cassert>
#include <fstream>

namespace nn_search
{
  void save_dataset(const Dataset &dataset, const std::string &filename)
  {
  std::ofstream fs(filename, std::ios::binary);
  size_t points_size = dataset.data_points.size();
  size_t all_points_size = dataset.all_points.size();
  fs.write((const char *)&points_size, sizeof(size_t));
  fs.write((const char *)dataset.data_points.data(), points_size * sizeof(DataPoint));
  fs.write((const char *)&all_points_size, sizeof(size_t));
  fs.write((const char *)dataset.all_points.data(), all_points_size * sizeof(float));
  
  fs.flush();
  fs.close();
  }
  
  void load_dataset(const std::string &filename, Dataset &dataset)
  {
  std::ifstream fs(filename, std::ios::binary);
  size_t points_size = 0;
  size_t all_points_size = 0;
  fs.read((char *)&points_size, sizeof(size_t));
  dataset.data_points.resize(points_size);
  fs.read((char *)dataset.data_points.data(), points_size * sizeof(DataPoint));
  
  fs.read((char *)&all_points_size, sizeof(size_t));
  dataset.all_points.resize(all_points_size);
  fs.read((char *)dataset.all_points.data(), all_points_size * sizeof(float));
  
  assert(all_points_size % points_size == 0);
  dataset.dim = all_points_size / points_size;

  fs.close();
  }
}