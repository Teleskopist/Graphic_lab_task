#pragma once

#include "LiteMath.h"
#include "utils/mesh/augmented_mesh.h"
#include "vtk_cells.h"

#include <cstdint>

namespace vtk
{
  using LiteMath::float3;

  struct DataArray
  {
    enum class Type
    {
      Int8,
      UInt8, 
      Int16, 
      UInt16, 
      Int32, 
      UInt32, 
      Int64, 
      UInt64, 
      Float32, 
      Float64
    };
    struct Values
    {
      uint64_t count = 0;
      Type type;
      bool range_set = false;
      double range_min = 0;
      double range_max = 0;
      union 
      {
        int8_t *i8;
        uint8_t *u8;
        int16_t *i16;
        uint16_t *u16;
        int32_t *i32;
        uint32_t *u32;
        int64_t *i64;
        uint64_t *u64;
        float *f32;
        double *f64;
      };
    };

    //rule of 5
    DataArray() = default;
    ~DataArray();
    DataArray(const DataArray &other);
    DataArray &operator=(const DataArray &other);
    DataArray(DataArray &&other);
    DataArray &operator=(DataArray &&other);

    void clear();

    Values values;
    uint64_t num_elements = 0;  //how many elements are in dataset
    uint64_t num_components = 0;//how many components (of type values.type) are in each element
    std::string name = "";
    std::vector<std::string> component_names;
  };

  //each piece of PolyData or UnstructuredGrid is parsed to this structure
  struct UnstructuredGrid
  {
    enum class Type
    {
      UNDEFINED,
      SURFACE,
      VOLUME,
      MIXED
    };
    std::string name = "";
    Type type = Type::UNDEFINED;
    
    uint64_t points_count = 0;
    uint64_t cells_count  = 0;

    std::vector<float3>    points;
    std::vector<DataArray> point_data_arrays; //each array has points_count elements

    std::vector<PackedCell> p_cells;
    std::vector<id_t> p_indices;
    
    std::vector<DataArray> cell_data_arrays;  //each array has cells_count elements
  };

  struct Dataset
  {
    std::vector<UnstructuredGrid> unstructured_grids;
  };
}