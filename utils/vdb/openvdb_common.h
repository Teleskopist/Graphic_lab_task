#pragma once

#include <iostream>
#include <string>

#ifndef KERNEL_SLICER

#include <LiteMath.h>
using LiteMath::float3;
using LiteMath::float4;
using LiteMath::uint3;

struct DensityGrid;
struct ColorDensityGrid;

#include "utils/misc/density_grid.h"


enum OpenVDBGridSampler {
  POINT_SAMPLER,
  BOX_SAMPLER,
  QUADRATIC_SAMPLER
};

struct OpenVDBDensityGridCreateInfo {
  int resolution = -1;
  OpenVDBGridSampler sampler = OpenVDBGridSampler::BOX_SAMPLER;
  float prune_tolerance = 0.0f;
};
// openvdb::FloatGrid::Ptr ResizeVDBGrid(openvdb::FloatGrid::Ptr src_grid, const int target_res);

#ifndef DISABLE_OPENVDB

struct OpenVDB_Grid
{
public:
    OpenVDB_Grid();
    // OpenVDB_Grid(const OpenVDB_Grid& obj);
    ~OpenVDB_Grid();
    float get_distance(float3 point);
    void LoadScene(const DensityGrid &grid, const float thr = 0.f);
    void LoadScene(const ColorDensityGrid &grid, const float4 thr = {0, 0, 0, 0});
    void SaveDensityGrid(const std::string &path) const;
    void LoadDensityGrid(const std::string &path);
    void SaveColorDensityGrid(const std::string &path) const;
    void LoadColorDensityGrid(const std::string &path);
    DensityGrid GetDensityGrid() const;
    ColorDensityGrid GetColorDensityGrid() const;
    float mem_usage() const;
    uint32_t get_voxels_count() const;
    void save_model(const std::string &file_name);
    void load_model(const std::string &file_name);

private:
    void* grid_ptr;
};
#else

struct OpenVDB_Grid
{
    bool foo;
    OpenVDB_Grid() { }
    // OpenVDB_Grid(const OpenVDB_Grid& obj);
    ~OpenVDB_Grid() { }
    float get_distance(float3 point) { return 0; }
    void LoadScene(const DensityGrid &grid, const float thr = 0.f) {}
    void LoadScene(const ColorDensityGrid &grid, const float4 thr = {0, 0, 0, 0}) {}
    void SaveDensityGrid(const std::string &path) const {}
    void LoadDensityGrid(const std::string &path) {}
    void SaveColorDensityGrid(const std::string &path) const {}
    void LoadColorDensityGrid(const std::string &path) {}
    DensityGrid GetDensityGrid() const { return DensityGrid{}; }
    ColorDensityGrid GetColorDensityGrid() const {return ColorDensityGrid{};}
    float mem_usage() const { return 0; }
    uint32_t get_voxels_count() const { return 0; }
    void save_model(const std::string &file_name) { }
    void load_model(const std::string &file_name) { }
};
#endif
#endif