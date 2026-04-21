#define HALFFLOAT
#include "openvdb_common.h"
#include "open_vdb_convert.h"

#ifndef DISABLE_OPENVDB

#include <openvdb/openvdb.h>
#include <openvdb/Grid.h>
#include <openvdb/Types.h>
#include <openvdb/tools/ChangeBackground.h>
#include <openvdb/tools/FastSweeping.h>
#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/VolumeToMesh.h>
#include <openvdb/tools/Interpolation.h>
#include <openvdb/tools/Dense.h>
#include <openvdb/tools/GridTransformer.h>
#include <openvdb/math/Mat4.h>

//does not work, fails with TypeError: mesh to volume conversion is supported only for scalar floating-point grids
//using GridImpl = openvdb::Grid<openvdb::tree::Tree4<LiteMath::half, 3, 3, 3>::Type>; 
using Vec4f = openvdb::math::Vec4s;
using GridVec4Impl = openvdb::Grid<openvdb::tree::Tree4<Vec4f, 3, 3, 3>::Type>;
using GridImpl = openvdb::Grid<openvdb::tree::Tree4<float, 3, 3, 3>::Type>;
using DenseXYZ = openvdb::tools::Dense<float, openvdb::tools::LayoutXYZ>;
using DenseVec4XYZ = openvdb::tools::Dense<Vec4f, openvdb::tools::LayoutXYZ>;

bool grid_registered = false;

void init_vdb()
{
    openvdb::initialize(); 
    if (!grid_registered)
    {
      GridImpl::registerGrid();
      GridVec4Impl::registerGrid();
      grid_registered = true;
    }
}

openvdb::FloatGrid::Ptr ResizeVDBGrid(openvdb::FloatGrid::Ptr src_grid, const int target_res)
{
    openvdb::FloatGrid::Ptr tempGrid = src_grid->deepCopy();

    openvdb::CoordBBox bbox = tempGrid->evalActiveVoxelBoundingBox();
    openvdb::Vec3d centerIndex = bbox.min().asVec3d() + bbox.max().asVec3d();
    centerIndex *= 0.5;

    openvdb::Vec3d sizeIndex = bbox.max().asVec3d() - bbox.min().asVec3d();
    
    double maxIndexDim = std::max({sizeIndex.x(), sizeIndex.y(), sizeIndex.z()});
    double newSrcVoxelSize = 2.0 / maxIndexDim;
    auto tempTransform = openvdb::math::Transform::createLinearTransform(newSrcVoxelSize);

    openvdb::Vec3d offset = -centerIndex * newSrcVoxelSize;
    tempTransform->postTranslate(offset);

    tempGrid->setTransform(tempTransform);

    double dstVoxelSize = 2.0 / (double)target_res;

    auto dstTransform = openvdb::math::Transform::createLinearTransform(dstVoxelSize);
    
    dstTransform->postTranslate(openvdb::Vec3d(-1.0, -1.0, -1.0));

    openvdb::FloatGrid::Ptr dstGrid = openvdb::FloatGrid::create(0.0f);
    dstGrid->setTransform(dstTransform);
    dstGrid->setGridClass(openvdb::GRID_FOG_VOLUME);
    dstGrid->setName("density");
    
    openvdb::tools::resampleToMatch<openvdb::tools::BoxSampler>(*tempGrid, *dstGrid);
    dstGrid->pruneGrid(0.0f);
    
    return dstGrid;
}

openvdb::FloatGrid::Ptr ChangeGridResolution(openvdb::FloatGrid::Ptr src_grid, 
                                            openvdb::CoordBBox src_box, 
                                            int target_res, OpenVDBGridSampler sampler)
{
    using GridT = openvdb::FloatGrid;

    openvdb::Vec3i src_size = src_box.extents().asVec3i();  // nx, ny, nz
    double src_max_dim = std::max({double(src_size.x()), 
                                   double(src_size.y()), 
                                   double(src_size.z())});

    double old_voxel_size = src_grid->transform().voxelSize()[0];

    double new_voxel_size = old_voxel_size * ((src_max_dim-1) / double(target_res-1));

    GridT::Ptr dstGrid = GridT::create(src_grid->background());
    dstGrid->setGridClass(src_grid->getGridClass());
    dstGrid->setName(src_grid->getName());

    auto dstT = openvdb::math::Transform::createLinearTransform(new_voxel_size);
    dstGrid->setTransform(dstT);
    
    if (sampler == OpenVDBGridSampler::BOX_SAMPLER) {
      openvdb::tools::resampleToMatch<openvdb::tools::BoxSampler>(*src_grid, *dstGrid);
    } else if (sampler == OpenVDBGridSampler::POINT_SAMPLER) {
      openvdb::tools::resampleToMatch<openvdb::tools::PointSampler>(*src_grid, *dstGrid);
    } else if (sampler == OpenVDBGridSampler::QUADRATIC_SAMPLER) {
      openvdb::tools::resampleToMatch<openvdb::tools::QuadraticSampler>(*src_grid, *dstGrid);
    }

    return dstGrid;
}
struct MatrixTransformer {
    openvdb::math::Mat4d mat;
    openvdb::math::Mat4d invMat;

    MatrixTransformer(const openvdb::math::Mat4d& m) : mat(m) {
        try {
            invMat = mat.inverse();
        } catch (...) {
            invMat = openvdb::math::Mat4d::identity();
        }
    }

    bool isAffine() const { return true; }

    openvdb::Vec3d transform(const openvdb::Vec3d& xyz) const {
        return mat.transform(xyz);
    }

    openvdb::Vec3d invTransform(const openvdb::Vec3d& xyz) const {
        return invMat.transform(xyz);
    }
};

float GetVBDModelSize(const std::string &path)
{
    init_vdb();
    openvdb::io::File file(path);
    file.open();
    
    auto allGrids = file.getGrids();
    openvdb::v12_0::Index64 memUsage = 0;
    for (auto &base: *allGrids) {
      auto grid = openvdb::gridPtrCast<openvdb::FloatGrid>(base);
      openvdb::tools::minMax(grid->tree());
      memUsage += grid->memUsage();
    }

    return float(memUsage);
}

uint3 GetVDBModelResolution(const std::string &path)
{
    init_vdb();

    openvdb::io::File file(path);
    file.open();
    openvdb::GridBase::Ptr base = file.readGrid("density");
    file.close();

    openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(base);
    
    auto meta = grid->getMetadata<openvdb::Int32Metadata>("resolution");
    int resolution = meta->value();

    return uint3(resolution, resolution, resolution);
}

void SaveDensityGridAsVDB(const std::string &path, const DensityGrid &src, OpenVDBDensityGridCreateInfo info)
{
    init_vdb();

    const int nx = src.size.x;
    const int ny = src.size.y;
    const int nz = src.size.z;

    openvdb::CoordBBox bbox(
        openvdb::Coord(0, 0, 0),
        openvdb::Coord(nx - 1, ny - 1, nz - 1)
    );

    DenseXYZ dense(bbox, const_cast<float*>(src.data.data()));
    openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create(0.0f);
    grid->setName("density");
    grid->setGridClass(openvdb::GRID_FOG_VOLUME);

    const float tolerance = 1e-6f;
    openvdb::tools::copyFromDense(dense, grid->tree(), tolerance, true);

    grid->setTransform(openvdb::math::Transform::createLinearTransform(1.0));

    openvdb::FloatGrid::Ptr resized_grid = ChangeGridResolution(grid, bbox, info.resolution, info.sampler);
    resized_grid->pruneGrid(info.prune_tolerance);
    resized_grid->tree().prune(info.prune_tolerance);
    resized_grid->setName("density");
    resized_grid->setGridClass(openvdb::GRID_FOG_VOLUME);
    resized_grid->insertMeta("resolution", openvdb::Int32Metadata(info.resolution));

    openvdb::io::File file(path);
    openvdb::GridPtrVec grids;
    grids.push_back(resized_grid);
    file.write(grids);
    file.close();
}

void SaveDensityMultiGridAsVDB(const std::string &path,
                               const DensityMultiGrid &src,
                               OpenVDBDensityGridCreateInfo info) {
  init_vdb();

  const int nx = src.size.x;
  const int ny = src.size.y;
  const int nz = src.size.z;
  uint32_t oneGridSize = nx * ny * nz;

  openvdb::CoordBBox bbox(openvdb::Coord(0, 0, 0),
                          openvdb::Coord(nx - 1, ny - 1, nz - 1));
  openvdb::GridPtrVec grids;

  for (uint32_t frameId = 0; frameId < src.frames; ++frameId) {
    float timeStamp = src.timestamps[frameId];

    DenseXYZ dense(
        bbox, const_cast<float *>(src.data.data() + frameId * oneGridSize));

    openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create(0.0f);
    grid->setName("density_frame#" + std::to_string(frameId));
    grid->setGridClass(openvdb::GRID_FOG_VOLUME);

    const float tolerance = 1e-6f;
    openvdb::tools::copyFromDense(dense, grid->tree(), tolerance, true);

    grid->setTransform(openvdb::math::Transform::createLinearTransform(1.0));

    openvdb::FloatGrid::Ptr resized_grid = ChangeGridResolution(grid, bbox, info.resolution, info.sampler);
    resized_grid->pruneGrid(info.prune_tolerance);
    resized_grid->tree().prune(info.prune_tolerance);
    resized_grid->setName("density_frame#" + std::to_string(frameId));
    resized_grid->setGridClass(openvdb::GRID_FOG_VOLUME);
    resized_grid->insertMeta("resolution",
                             openvdb::Int32Metadata(info.resolution));
    resized_grid->insertMeta("timeStamp", openvdb::FloatMetadata(timeStamp));
    resized_grid->insertMeta("frameId", openvdb::Int32Metadata(frameId));

    grids.push_back(resized_grid);
  }

  openvdb::io::File file(path);
  file.write(grids);
  file.close();
}

void SaveDensityMultiGridAsVDB(const std::string &path,
                               const DensityMultiGrid &src,
                               int resolution) {
  OpenVDBDensityGridCreateInfo createInfo = {};
  createInfo.resolution = resolution;
  SaveDensityMultiGridAsVDB(path, src, createInfo);
}

void SaveDensityGridAsVDB(const std::string &path,
                               const DensityGrid &src,
                               int resolution) {
  OpenVDBDensityGridCreateInfo createInfo = {};
  createInfo.resolution = resolution;
  SaveDensityGridAsVDB(path, src, createInfo);
}

DensityGrid ConvertVDBToDensityGrid(const std::string &path)
{
    init_vdb();

    openvdb::io::File file(path);
    file.open();
    openvdb::GridBase::Ptr base = file.readGrid("density");
    file.close();

    openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(base);

    auto meta = grid->getMetadata<openvdb::Int32Metadata>("resolution");
    int resolution = meta->value();

    int total_size = resolution * resolution * resolution;
    DensityGrid dg;
    dg.data.resize(total_size, 0.0f);
    dg.size = uint3(resolution, resolution, resolution);

    openvdb::CoordBBox bbox(openvdb::Coord(0, 0, 0), openvdb::Coord(resolution - 1, resolution - 1, resolution - 1));
    DenseXYZ dense(bbox, dg.data.data());
    openvdb::tools::copyToDense(*grid, dense, false);

    return dg;
}

DensityMultiGrid ConvertVDBToDensityMultiGrid(const std::string &path)
{
  init_vdb();

  openvdb::io::File file(path);
  file.open();
  auto allGrids = file.getGrids();
  file.close();

  DensityMultiGrid result;
  result.frames = allGrids->size();
  result.timestamps.resize(result.frames);

  for (auto &gridBase: *allGrids) {
    openvdb::FloatGrid::Ptr floatGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(gridBase);
    int resolution = floatGrid->getMetadata<openvdb::Int32Metadata>("resolution")->value();
    int total_size = resolution * resolution * resolution;

    float timeStamp = floatGrid->getMetadata<openvdb::FloatMetadata>("timeStamp")->value();
    int frameId = floatGrid->getMetadata<openvdb::Int32Metadata>("frameId")->value();

    if (result.data.empty()) {
      result.data.resize(total_size * result.frames, 0.0f);
      result.size = uint3(resolution, resolution, resolution);
    }

    float *currentGridData = result.data.data() + total_size * frameId;
    openvdb::CoordBBox bbox(openvdb::Coord(0, 0, 0), openvdb::Coord(resolution - 1, resolution - 1, resolution - 1));
    DenseXYZ dense(bbox, currentGridData);
    openvdb::tools::copyToDense(*floatGrid, dense, false);

    result.timestamps[frameId] = timeStamp;
  }

  return result;
}

DensityGrid CreateModelFromVDBToDensityGrid(const std::string &path, const int grid_resolution)
{
    init_vdb();

    DensityGrid dense_grid;

    openvdb::io::File file(path);
    file.open();


    if (!file.hasGrid("density")) 
    {
        printf("[VDB info]: no grid is named \"density\"\n"); 
        file.close();
        
        return dense_grid;
    }

    openvdb::GridBase::Ptr baseGrid = file.readGrid("density");
    file.close();

    openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);
    openvdb::FloatGrid::Ptr resized_grid = ResizeVDBGrid(grid, grid_resolution);

    float density_multiplier = 20.0f;

    for (openvdb::FloatGrid::ValueOnIter iter = resized_grid->beginValueOn(); iter; ++iter) 
    {
        float current_val = *iter;
        iter.setValue(current_val * density_multiplier);
    }

    openvdb::math::Mat4d swap_y_z_axis_mat(
        1, 0, 0, 0,
        0, 0, 1, 0,
        0, 1, 0, 0,
        0, 0, 0, 1
    );
    MatrixTransformer xform(swap_y_z_axis_mat);
    
    openvdb::FloatGrid::Ptr result_grid = openvdb::FloatGrid::create(0.0f);
    result_grid->setGridClass(resized_grid->getGridClass());
    result_grid->setName(resized_grid->getName());

    openvdb::tools::GridResampler resampler;
    resampler.transformGrid<openvdb::tools::BoxSampler>(xform, *resized_grid, *result_grid);

    const openvdb::Coord min(0, 0, 0);
    const openvdb::Coord max(grid_resolution - 1, grid_resolution - 1, grid_resolution - 1);
    const openvdb::CoordBBox bbox(min, max);

    dense_grid.data = std::vector<float>(grid_resolution * grid_resolution * grid_resolution, 0.f);
    dense_grid.size = LiteMath::uint3(grid_resolution);

    DenseXYZ openvdb_dense(bbox, dense_grid.data.data());
    openvdb::tools::copyToDense(*result_grid, openvdb_dense, false);

    return dense_grid;
}

DensityGrid ConvertMeshToDensityGridByOpenVDB(const cmesh4::SimpleMesh &mesh, const int grid_size, const float narrow_band)
{
    init_vdb();

    std::vector<openvdb::Vec3s> points;
    std::vector<openvdb::Vec3I> indices;

    for (auto point: mesh.vPos4f)
    {
        points.push_back(openvdb::Vec3s(point.x, point.y, point.z));
    }

    for (int i = 0; i < mesh.IndicesNum(); i += 3)
    {
        indices.push_back(openvdb::Vec3I(mesh.indices[i], mesh.indices[i + 1], mesh.indices[i + 2]));
    }

    float domainSize = 2.0f;
    float voxelSize = domainSize / grid_size;

    openvdb::math::Transform::Ptr transform = openvdb::math::Transform::createLinearTransform(voxelSize);

    auto grid = openvdb::tools::meshToLevelSet<openvdb::FloatGrid>(
        *transform,
        points,
        indices,
        narrow_band
    );

    openvdb::tools::sdfToFogVolume(*grid);

    int halfRes = grid_size / 2;
    openvdb::Coord minIdx(-halfRes, -halfRes, -halfRes);
    openvdb::Coord maxIdx(halfRes - 1, halfRes - 1, halfRes - 1);
    openvdb::CoordBBox bbox(minIdx, maxIdx);

    DenseXYZ openvdb_dense(bbox);
    openvdb::tools::copyToDense(*grid, openvdb_dense);

    float* raw_data = openvdb_dense.data();
    size_t voxel_count = openvdb_dense.valueCount();

    DensityGrid dense_grid;
    dense_grid.size = LiteMath::uint3(grid_size);
    dense_grid.data.resize(voxel_count);
    std::memcpy(dense_grid.data.data(), raw_data, voxel_count * sizeof(float));

    return dense_grid;
}

void
OpenVDB_Grid::LoadScene(const DensityGrid &grid, const float thr)
{
    float voxel_size = 1.f;
    const openvdb::Coord min(0, 0, 0);
    const openvdb::Coord max(grid.size.x - 1, grid.size.y - 1, grid.size.z - 1);
    const openvdb::CoordBBox bbox(min, max);

    DenseXYZ openvdb_dense(bbox, const_cast<float*>(grid.data.data()));

    auto openvdb_sparse = openvdb::FloatGrid::create(0.0f);
    openvdb_sparse->setName("openvdb_density_grid");
    openvdb_sparse->setGridClass(openvdb::GRID_FOG_VOLUME);
    openvdb_sparse->setTransform(openvdb::math::Transform::createLinearTransform(voxel_size));

    const float tolerance = thr;
    openvdb::tools::copyFromDense(openvdb_dense, *openvdb_sparse, tolerance, false);
    
    openvdb_sparse->insertMeta("dim_size", openvdb::Int32Metadata(grid.size.x));
    openvdb_sparse->pruneGrid(0.0f);
    
    openvdb::FloatGrid::Ptr* ptr = new decltype(openvdb_sparse)(openvdb_sparse);
    grid_ptr = (void*)ptr;
}

void
OpenVDB_Grid::LoadScene(const ColorDensityGrid &grid, const float4 thr)
{
    float voxel_size = 1.f;
    const openvdb::Coord min(0, 0, 0);
    const openvdb::Coord max(grid.size.x - 1, grid.size.y - 1, grid.size.z - 1);
    const openvdb::CoordBBox bbox(min, max);

    std::vector<Vec4f> grid_data;

    for (auto v : grid.data)
    {
        grid_data.push_back(Vec4f(v.x, v.y, v.z, v.w));
    }

    DenseVec4XYZ openvdb_dense(bbox, const_cast<Vec4f*>(grid_data.data()));

    auto openvdb_sparse = GridVec4Impl::create(Vec4f(0, 0, 0, 0));
    openvdb_sparse->setName("openvdb_color_density_grid");
    openvdb_sparse->setGridClass(openvdb::GRID_FOG_VOLUME);
    openvdb_sparse->setTransform(openvdb::math::Transform::createLinearTransform(voxel_size));

    const Vec4f tolerance = {thr.x, thr.y, thr.z, thr.w};
    openvdb::tools::copyFromDense(openvdb_dense, *openvdb_sparse, tolerance, false);

    openvdb_sparse->insertMeta("dim_size", openvdb::Int32Metadata(grid.size.x));

    GridVec4Impl::Ptr* ptr = new decltype(openvdb_sparse)(openvdb_sparse);
    grid_ptr = (void*)ptr;
}

DensityGrid
OpenVDB_Grid::GetDensityGrid() const
{
    auto openvdb_sparse = *(openvdb::FloatGrid::Ptr*)(grid_ptr);
    
    int size = openvdb_sparse->getMetadata<openvdb::Int32Metadata>("dim_size")->value();  
    const openvdb::Coord min(0, 0, 0);
    const openvdb::Coord max(size - 1, size - 1, size - 1);
    const openvdb::CoordBBox bbox(min, max);

    DensityGrid out_grid;
    out_grid.data = std::vector<float>(size * size * size, 0.f);
    out_grid.size = LiteMath::uint3(size);

    DenseXYZ openvdb_dense(bbox, out_grid.data.data());
    openvdb::tools::copyToDense(*openvdb_sparse, openvdb_dense, false);

    return out_grid;
}

void OpenVDB_Grid::SaveDensityGrid(const std::string &path) const
{
    auto openvdb_sparse = *(openvdb::FloatGrid::Ptr*)(grid_ptr);

    openvdb::GridPtrVec grids;

    openvdb_sparse->setName("grid");

    grids.push_back(openvdb_sparse);

    openvdb::io::File file(path);
    file.write(grids);
    file.close();
}

void OpenVDB_Grid::LoadDensityGrid(const std::string &path)
{
    openvdb::io::File file(path);
    
    file.open();
    openvdb::GridBase::Ptr baseGrid = file.readGrid("grid");
    file.close();

    openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);
    openvdb::tools::minMax(grid->tree());//forces grid to load all its values
    openvdb::FloatGrid::Ptr* ptr = new decltype(grid)(grid);

    grid_ptr = (void*)(ptr);
}

ColorDensityGrid
OpenVDB_Grid::GetColorDensityGrid() const
{
    auto openvdb_sparse = *(GridVec4Impl::Ptr*)(grid_ptr);

    int size = openvdb_sparse->getMetadata<openvdb::Int32Metadata>("dim_size")->value();  
    const openvdb::Coord min(0, 0, 0);
    const openvdb::Coord max(size - 1, size - 1, size - 1);
    const openvdb::CoordBBox bbox(min, max);

    ColorDensityGrid out_grid;
    out_grid.size = LiteMath::uint3(size);

    std::vector<Vec4f> grid_data(size * size * size, Vec4f(0, 0, 0, 0));

    DenseVec4XYZ openvdb_dense(bbox, grid_data.data());
    openvdb::tools::copyToDense(*openvdb_sparse, openvdb_dense, false);

    for (auto v : grid_data)
    {
        out_grid.data.push_back({v[0], v[1], v[2], v[3]});
    }

    return out_grid;
}

void OpenVDB_Grid::SaveColorDensityGrid(const std::string &path) const
{
    auto openvdb_sparse = *(GridVec4Impl::Ptr*)(grid_ptr);

    openvdb::GridPtrVec grids;

    openvdb_sparse->setName("grid");

    grids.push_back(openvdb_sparse);

    openvdb::io::File file(path);
    file.write(grids);
    file.close();
}

void OpenVDB_Grid::LoadColorDensityGrid(const std::string &path)
{
    openvdb::io::File file(path);
    
    file.open();
    openvdb::GridBase::Ptr baseGrid = file.readGrid("grid");
    file.close();

    GridVec4Impl::Ptr grid = openvdb::gridPtrCast<GridVec4Impl>(baseGrid);
    openvdb::tools::minMax(grid->tree());//forces grid to load all its values
    GridVec4Impl::Ptr* ptr = new decltype(grid)(grid);

    grid_ptr = (void*)(ptr);
}

OpenVDB_Grid::OpenVDB_Grid() 
{ 
    init_vdb();
}

OpenVDB_Grid::~OpenVDB_Grid()
{
    // auto ptr = (GridImpl::Ptr*)(grid_ptr);

    // if (ptr != nullptr)
    // {
    //     delete ptr;
    // }
}

float 
OpenVDB_Grid::get_distance(LiteMath::float3 point)
{
    auto grid = *(openvdb::FloatGrid::Ptr*)(grid_ptr);

    openvdb::math::Transform& transform = grid->transform();
    openvdb::Vec3f indexPoint = transform.worldToIndex(openvdb::Vec3f(point.x, point.y, point.z));
    
    return openvdb::tools::BoxSampler::sample(grid->tree(), indexPoint);
}

float 
OpenVDB_Grid::mem_usage() const
{
    auto grid = *(openvdb::FloatGrid::Ptr*)(grid_ptr);
    float mem = grid->memUsage();
    //grid->treePtr()->print(std::cerr, 3);

    return mem;
}

uint32_t 
OpenVDB_Grid::get_voxels_count() const
{
    auto grid = *(openvdb::FloatGrid::Ptr*)(grid_ptr);
    return grid->tree().activeVoxelCount();
}

void 
OpenVDB_Grid::save_model(const std::string& file_name)
{
    auto grid = *(openvdb::FloatGrid::Ptr*)(grid_ptr);
    openvdb::GridPtrVec grids;

    grid->setName("grid");

    grids.push_back(grid);

    openvdb::io::File file(file_name);
    file.write(grids);
    file.close();
}

void 
OpenVDB_Grid::load_model(const std::string &file_name)
{
    openvdb::io::File file(file_name);
    
    file.open();
    openvdb::GridBase::Ptr baseGrid = file.readGrid("grid");
    file.close();

    openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);
    openvdb::tools::minMax(grid->tree());//forces grid to load all its values
    openvdb::FloatGrid::Ptr* ptr = new decltype(grid)(grid);

    grid_ptr = (void*)(ptr);
}

#else

DensityGrid ConvertMeshToDensityGridByOpenVDB(const cmesh4::SimpleMesh &mesh, const int voxel_size, const float narrowBand) { return DensityGrid{}; }
DensityGrid ConvertVDBToDensityGrid(const std::string &path) { return DensityGrid{}; }
DensityMultiGrid ConvertVDBToDensityMultiGrid(const std::string &path) { return DensityMultiGrid{}; }
DensityGrid CreateModelFromVDBToDensityGrid(const std::string &path, const int grid_resolution) { return DensityGrid{}; }
void SaveDensityGridAsVDB(const std::string &path, const DensityGrid &grid, const int vdb_grid_resolution) {}
void SaveDensityMultiGridAsVDB(const std::string &path, const DensityMultiGrid &grid, const int vdb_grid_resolution) {}
void SaveDensityGridAsVDB(const std::string &path, const DensityGrid &grid, OpenVDBDensityGridCreateInfo) {}
void SaveDensityMultiGridAsVDB(const std::string &path, const DensityMultiGrid &grid, OpenVDBDensityGridCreateInfo) {}
float GetVBDModelSize(const std::string &path) { return 0; }
uint3 GetVDBModelResolution(const std::string &path) { return uint3{}; }

#endif