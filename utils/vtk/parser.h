#pragma once
#include "vtk_structures.h"
#include "utils/common/plane.h"
#include "blk/blk.h"
#include <filesystem>

namespace vtk
{
  bool read_vtk_dataset(const std::filesystem::path& filename, Dataset& dataset);
  cmesh4::AugmentedMesh triangulate_grid_bounds(UnstructuredGrid &grid, bool compress_float_values = false);
  cmesh4::AugmentedMesh triangulate_grid_bounds_with_plane(UnstructuredGrid &grid, const Plane& plane, bool compress_float_values = false);
  void mark_border_cells(UnstructuredGrid &grid);
  LiteMath::float4x4 rescale_dataset(Dataset &mesh, float3 min_pos, float3 max_pos);
  void calculate_data_array_limits(const DataArray &array, double *min_val, double *max_val);
  void print_dataset_info(vtk::Dataset &dataset);
  void calculate_common_data_array_ranges(Dataset &dataset);
  void prune_dataset(Dataset &dataset, bool prune_grids, std::vector<std::string> &grid_whitelist, 
                     bool prune_point_arrays, std::vector<std::string> &point_arrays_whitelist,
                     bool prune_cell_arrays, std::vector<std::string> &cell_arrays_whitelist);
  void compress_float_channel(DataChannel &channel, 
                              DataChannel::Type comp_type = DataChannel::Type::FP8,
                              bool log_scale = false);
  
  void finish_loading_vtk_dataset(vtk::Dataset &dataset, Block *load_options_blk = nullptr);
}
