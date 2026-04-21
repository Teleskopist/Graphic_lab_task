#pragma once
#include "openvdb_common.h"
#include "utils/mesh/mesh.h"
using cmesh4::SimpleMesh;

struct Block;
void load_from_blk(OpenVDBDensityGridCreateInfo &settings, const Block *block);
void save_to_blk(const OpenVDBDensityGridCreateInfo &settings, Block *block);

DensityGrid ConvertMeshToDensityGridByOpenVDB(const cmesh4::SimpleMesh &mesh, const int voxel_size, const float narrowBand);
DensityGrid ConvertVDBToDensityGrid(const std::string &path);
DensityMultiGrid ConvertVDBToDensityMultiGrid(const std::string &path);
DensityGrid CreateModelFromVDBToDensityGrid(const std::string &path, const int grid_resolution);
void SaveDensityGridAsVDB(const std::string &path, const DensityGrid &grid, OpenVDBDensityGridCreateInfo info);
void SaveDensityMultiGridAsVDB(const std::string &path, const DensityMultiGrid &grid, OpenVDBDensityGridCreateInfo info);
void SaveDensityGridAsVDB(const std::string &path, const DensityGrid &grid, int resolution);
void SaveDensityMultiGridAsVDB(const std::string &path, const DensityMultiGrid &grid, int resolution);
float GetVBDModelSize(const std::string &path);
uint3 GetVDBModelResolution(const std::string &path);