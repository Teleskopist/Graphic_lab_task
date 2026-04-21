#pragma once
#include "vtk_structures.h"
#include "vtk_cells.h"

#include <string>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <filesystem>

namespace vtk
{
  enum class BinaryBlockFormat
  {
    BINARY_BASE64,
    BINARY_RAW,
    ASCII
  };

  class BinaryBlock
  {
  public:
    //start with 64 Kb, increase 4 times up to 1 Gb
    //then increase lineary (1,2,3 Gb etc.)
    static constexpr unsigned START_BLOCK_SIZE  = 1 << 16;
    static constexpr unsigned LINEAR_BLOCK_SIZE = 1 << 30;
    static constexpr unsigned SIZE_MULT_FACTOR  = 4;

    BinaryBlock();
    BinaryBlock(const BinaryBlock &other);
    ~BinaryBlock();
    
    void add(unsigned char c);

    uint64_t size() const { return sz; }
    const unsigned char *data() const { return _data; }

  //private:  
    void resize();

    std::string filename = "";
    bool in_memory = true;
    unsigned char *_data = nullptr;
    uint64_t sz = 0;
    uint64_t capacity = 0;
  };

  struct RawDataset
  {
    BinaryBlock xml_data;
    std::vector<BinaryBlock> binary_blocks;
    std::vector<BinaryBlockFormat> binary_block_formats;
    int appended_data_block_id = -1;
  };

  template <typename T>
  void convert_values(const DataArray::Values &values, std::vector<T> &out_values)
  {
    out_values.resize(values.count);
    switch (values.type)
    {
    case DataArray::Type::Int8:
      for (int i = 0; i < values.count; i++)
        out_values[i] = values.i8[i];
      break;
    case DataArray::Type::UInt8:
      for (int i = 0; i < values.count; i++)
        out_values[i] = values.u8[i];
      break;
    case DataArray::Type::Int16:
      for (int i = 0; i < values.count; i++)
        out_values[i] = values.i16[i];
      break;
    case DataArray::Type::UInt16:
      for (int i = 0; i < values.count; i++)
        out_values[i] = values.u16[i];
      break;
    case DataArray::Type::Int32:
      for (int i = 0; i < values.count; i++)
        out_values[i] = values.i32[i];
      break;
    case DataArray::Type::UInt32:
      for (int i = 0; i < values.count; i++)
        out_values[i] = values.u32[i];
      break;
    case DataArray::Type::Int64:
      for (int i = 0; i < values.count; i++)
        out_values[i] = values.i64[i];
      break;
    case DataArray::Type::UInt64:
      for (int i = 0; i < values.count; i++)
        out_values[i] = values.u64[i];
      break;
    case DataArray::Type::Float32:
      for (int i = 0; i < values.count; i++)
        out_values[i] = values.f32[i];
      break;
    case DataArray::Type::Float64:
      for (int i = 0; i < values.count; i++)
        out_values[i] = values.f64[i];
      break;
    }
  }

  bool primal_parse(const std::filesystem::path& filename, RawDataset &dataset);
  bool read_ASCII_data(DataArray::Type type, const BinaryBlock &block, DataArray::Values &out_values);
}
