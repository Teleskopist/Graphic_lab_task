#include "parser.h"
#include "vtk_internal.h"
// #include "byte_order.h"
#include "conversions.h"
#include "pugixml.hpp"
#include "LiteMath.h"

#include <codecvt>
#include <string>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <filesystem>
#include <fstream>
#include <locale>

namespace fs = std::filesystem;

using LiteMath::float3;
namespace vtk
{
  std::string ws2s(const std::wstring &wstr)
  {
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;
    return converterX.to_bytes(wstr);
  }

  int get_data_array_id(pugi::xml_node data_array_node, const RawDataset &data)
  {
    int data_array_id = -1;
    if (std::wstring(data_array_node.attribute(L"format").value()) == L"appended")
    {
      uint64_t offset = data_array_node.attribute(L"offset").as_ullong((uint64_t)-1);
      if (offset == (uint64_t)-1)
      {
        printf("[load_PiecePoints] points DataArray has appended format but offset is missing\n");
        return -1;
      }
      else if (data.appended_data_block_id == -1)
      {
        printf("[load_PiecePoints] points DataArray has appended format but appended_data_block_id is missing\n");
        return -1;
      }
      else
      {
        data_array_id = data.appended_data_block_id;
      }
    }
    else 
    {
      pugi::xml_node points_data_node = data_array_node.child(L"data");
      if (!points_data_node)
      {
        printf("[load_PiecePoints] points DataArray data node is missing\n");
        return -1;
      }
      data_array_id = points_data_node.attribute(L"block_id").as_int(-1);
      if (data_array_id == -1)
      {
        printf("[load_PiecePoints] points DataArray data block_id is missing\n");
        return -1;
      }
    }

    return data_array_id;
  }

  DataArray::Type type_name_to_type(const std::wstring &type_name)
  {
    if (type_name == L"Int8")
      return DataArray::Type::Int8;
    else if (type_name == L"UInt8")
      return DataArray::Type::UInt8;
    else if (type_name == L"Int16")
      return DataArray::Type::Int16;
    else if (type_name == L"UInt16")
      return DataArray::Type::UInt16;
    else if (type_name == L"Int32")
      return DataArray::Type::Int32;
    else if (type_name == L"UInt32")
      return DataArray::Type::UInt32;
    else if (type_name == L"Int64")
      return DataArray::Type::Int64;
    else if (type_name == L"UInt64")
      return DataArray::Type::UInt64;
    else if (type_name == L"Float32")
      return DataArray::Type::Float32;
    else if (type_name == L"Float64")
      return DataArray::Type::Float64;
    
    printf("[type_name_to_type] unknown type name: %ls\n", type_name.c_str());
    return DataArray::Type::UInt8;
  }

  DataArray load_data_array(const RawDataset &data, pugi::xml_node data_array_node, int data_array_id)
  {
    DataArray data_array;
    std::wstring type_name = data_array_node.attribute(L"type").value();
    std::wstring format_name = data_array_node.attribute(L"format").value();
    int num_components = data_array_node.attribute(L"NumberOfComponents").as_int(-1);
    int num_tuples = data_array_node.attribute(L"NumberOfTuples").as_int(-1);
    if (num_components == -1)
    {
      if (num_tuples == -1)
        num_components = 1;
      else
        num_components = num_tuples;
    }
    else if (num_tuples != -1)
      printf("[load_data_array] both NumberOfComponents and NumberOfTuples are specified, NumberOfComponents will be used\n");
    
    auto type = type_name_to_type(type_name);
    if (format_name == L"ascii")
    {
      bool parsed_ascii = read_ASCII_data(type, data.binary_blocks[data_array_id], data_array.values);
      if (!parsed_ascii)
        return DataArray();
    }
    else if (format_name == L"binary" || format_name == L"appended")
    {
      bool correct_endianess = false; //TODO: load endianess from xml

      //we have one empty byte at the start of append data, so we need to skip it (offset+1)
      size_t offset = format_name == L"appended" ? data_array_node.attribute(L"offset").as_ullong(0)+1 : 0;
      const unsigned char *raw_data_size = data.binary_blocks[data_array_id].data() + offset;
      size_t bytesize = *(size_t *)raw_data_size; //the first 8 bytes are the expected number of elements
      const unsigned char *raw_data = raw_data_size + sizeof(size_t);

      if (format_name == L"binary")
      {
        size_t target_bytesize = data.binary_blocks[data_array_id].size() - sizeof(size_t);
        if (target_bytesize != bytesize)
        {
          printf("[load_data_array] binary file states %zu bytes but the actual number is %zu\n",
                 bytesize, target_bytesize);
          return DataArray();
        }
      }
      else if (format_name == L"appended")
      {
        size_t max_bytesize = data.binary_blocks[data_array_id].size() - sizeof(size_t);
        if (bytesize > max_bytesize)
        {
          printf("[load_data_array] data array expects %zu bytes but only %zu left in appended data section\n",
                 bytesize, max_bytesize);
          return DataArray();
        }
      }

      data_array.values.type = type;
      switch (type)
      {
        case DataArray::Type::Int8:
          data_array.values.count = bytesize / sizeof(int8_t);
          data_array.values.i8 = new int8_t[data_array.values.count];
          memcpy(data_array.values.i8, raw_data, bytesize);
          break;
        case DataArray::Type::UInt8:
          data_array.values.count = bytesize / sizeof(uint8_t);
          data_array.values.u8 = new uint8_t[data_array.values.count];
          memcpy(data_array.values.u8, raw_data, bytesize);
          break;
        case DataArray::Type::Int16:
          data_array.values.count = bytesize / sizeof(int16_t);
          data_array.values.i16 = new int16_t[data_array.values.count];
          memcpy(data_array.values.i16, raw_data, bytesize);
          if (correct_endianess)
            switch_byte_order<int16_t>(data_array.values.i16, data_array.values.count);
          break;
        case DataArray::Type::UInt16:
          data_array.values.count = bytesize / sizeof(uint16_t);
          data_array.values.u16 = new uint16_t[data_array.values.count];
          memcpy(data_array.values.u16, raw_data, bytesize);
          if (correct_endianess)
            switch_byte_order<uint16_t>(data_array.values.u16, data_array.values.count);
          break;
        case DataArray::Type::Int32:
          data_array.values.count = bytesize / sizeof(int32_t);
          data_array.values.i32 = new int32_t[data_array.values.count];
          memcpy(data_array.values.i32, raw_data, bytesize);
          if (correct_endianess)
            switch_byte_order<int32_t>(data_array.values.i32, data_array.values.count);
          break;
        case DataArray::Type::UInt32:
          data_array.values.count = bytesize / sizeof(uint32_t);
          data_array.values.u32 = new uint32_t[data_array.values.count];
          memcpy(data_array.values.u32, raw_data, bytesize);
          if (correct_endianess)
            switch_byte_order<uint32_t>(data_array.values.u32, data_array.values.count);
          break;
        case DataArray::Type::Int64:
          data_array.values.count = bytesize / sizeof(int64_t);
          data_array.values.i64 = new int64_t[data_array.values.count];
          memcpy(data_array.values.i64, raw_data, bytesize);
          if (correct_endianess)
            switch_byte_order<int64_t>(data_array.values.i64, data_array.values.count);
          break;
        case DataArray::Type::UInt64:
          data_array.values.count = bytesize / sizeof(uint64_t);
          data_array.values.u64 = new uint64_t[data_array.values.count];
          memcpy(data_array.values.u64, raw_data, bytesize);
          if (correct_endianess)
            switch_byte_order<uint64_t>(data_array.values.u64, data_array.values.count);
          break;
        case DataArray::Type::Float32:
          data_array.values.count = bytesize / sizeof(float);
          data_array.values.f32 = new float[data_array.values.count];
          memcpy(data_array.values.f32, raw_data, bytesize);
          if (correct_endianess)
            switch_byte_order<float>(data_array.values.f32, data_array.values.count);
          break;
        case DataArray::Type::Float64:
          data_array.values.count = bytesize / sizeof(double);
          data_array.values.f64 = new double[data_array.values.count];
          memcpy(data_array.values.f64, raw_data, bytesize);
          if (correct_endianess)
            switch_byte_order<double>(data_array.values.f64, data_array.values.count);
          break;
        default:
          printf("[load_data_array] unknown type\n");
          return DataArray();
      }
    }

    data_array.num_components = num_components;
    data_array.num_elements = data_array.values.count / num_components;


    if (data_array_node.attribute(L"Name"))
      data_array.name = ws2s(data_array_node.attribute(L"Name").value());
    else if (data_array_node.attribute(L"name"))
      data_array.name = ws2s(data_array_node.attribute(L"name").value());
    else
      data_array.name = "unnamed_data_array";
    
    for (int i=0;i<num_components;i++)
    {
      std::wstring name = L"ComponentName"+std::to_wstring(i);
      if (data_array_node.attribute(name.c_str()))
        data_array.component_names.push_back(ws2s(data_array_node.attribute(name.c_str()).value()));
      else
        data_array.component_names.push_back("unnamed");
    }
    
    return data_array;
  }

  DataArray load_data_array(const RawDataset &data, pugi::xml_node data_array_node)
  {
    int data_array_id = get_data_array_id(data_array_node, data);
    if (data_array_id == -1)
      return DataArray();
    return load_data_array(data, data_array_node, data_array_id);
  }

  // splits data array (creates separate arrays for components) if necessary and saves it
  // in vector. Destroys the original array
  void save_split_array(DataArray &array, std::vector<DataArray> &split_arrays)
  {
    bool split = false;
    if (array.num_components > 1 && array.component_names.size() == array.num_components &&
        ( array.values.type == DataArray::Type::Float32 || array.values.type == DataArray::Type::Float64))
    {
      for (int i=0;i<array.num_components;i++)
      {
        if (array.component_names[i] != "unnamed")
        {
          split = true;
          break;
        }
      }
    }

    if (!split)
    {
      split_arrays.push_back(std::move(array));
    }
    else
    {
      int off = split_arrays.size();
      split_arrays.resize(off + array.num_components);

      if (array.values.type == DataArray::Type::Float32)
      {
        for (int i=0;i<array.num_components;i++)
        {
          DataArray &a = split_arrays[off+i];
          a.name = array.name+"_"+array.component_names[i];
          a.num_components = 1;
          a.num_elements = array.num_elements;
          a.values.type = DataArray::Type::Float32;
          a.values.count = array.num_elements;
          a.values.f32 = new float[array.num_elements];
          for (int j=0;j<array.num_elements;j++)
            a.values.f32[j] = array.values.f32[j*array.num_components+i];
        }
      }
      else if (array.values.type == DataArray::Type::Float64)
      {
        for (int i=0;i<array.num_components;i++)
        {
          DataArray &a = split_arrays[off+i];
          a.name = array.name+"_"+array.component_names[i];
          a.num_components = 1;
          a.num_elements = array.num_elements;
          a.values.type = DataArray::Type::Float64;
          a.values.count = array.num_elements;
          a.values.f64 = new double[array.num_elements];
          for (int j=0;j<array.num_elements;j++)
            a.values.f64[j] = array.values.f64[j*array.num_components+i];
        }
      }
    }
  }

  bool read_vtk_dataset(const fs::path& filename, Dataset& dataset, const std::string &name_prefix);
  bool load_mutiblock_data_set(pugi::xml_node set_node, std::filesystem::path dir_path,
                               const std::string &name_prefix, Dataset &dataset)
  {
    for (const auto &child : set_node.children())
    {
      if (wcscmp(child.name(), L"DataSet") == 0)
      {
        std::string name = name_prefix+"unnamed_dataset_"+std::to_string(dataset.unstructured_grids.size());
        if (child.attribute(L"name"))
          name = name_prefix+ws2s(child.attribute(L"name").value());
        else if (child.attribute(L"index"))
          name = name_prefix+"dataset_"+std::to_string(child.attribute(L"index").as_uint());

        const auto attribute_file = child.attribute(L"file");
        if (!attribute_file)
        {
          printf("[load_mutiblock_data_set] file attribute is missing\n");
          return false;
        }

        const auto maybe_path = attribute_file.as_string(nullptr);
        if (nullptr == maybe_path)
        {
          printf("[load_mutiblock_data_set] file attribute should be a string\n");
          return false;
        }

        const auto full_path = dir_path / maybe_path;
        if (!read_vtk_dataset(full_path, dataset, name))
        {
          printf("[load_mutiblock_data_set] failed to read vtk dataset from '%s'\n", full_path.string().c_str());
          return false;
        }
      }
      else if (wcscmp(child.name(), L"Block") == 0)
      {
        std::string new_prefix;
        if (child.attribute(L"name"))
          new_prefix = name_prefix+ws2s(child.attribute(L"name").value())+"_";
        else if (child.attribute(L"index"))
          new_prefix = name_prefix+"block_"+std::to_string(child.attribute(L"index").as_uint())+"_";
        else
          new_prefix = name_prefix;

        if (!load_mutiblock_data_set(child, dir_path, new_prefix, dataset))
          return false;
      }
    }

    return true;
  }

  bool load_PiecePoints(pugi::xml_node piece_node, int points_count, const RawDataset &data, std::vector<float3> &out_points)
  {
    pugi::xml_node points_node = piece_node.child(L"Points");
    if (!points_node)
    {
      printf("[load_PiecePoints] Points node is missing\n");
      return false;
    }

    pugi::xml_node points_array_node = points_node.child(L"DataArray");
    if (!points_array_node)
    {
      printf("[load_PiecePoints] points DataArray node is missing\n");
      return false;
    }

    //load points data array
    DataArray points_data_array = load_data_array(data, points_array_node);
    
    if (points_data_array.num_elements == 0)
    {
      printf("[load_PiecePoints] points data array is empty\n");
      return false;
    }

    if (points_data_array.num_elements != points_count)
    {
      printf("[load_PiecePoints] points data array has %d elements but expected %d\n", (int)points_data_array.num_elements, points_count);
      return false;
    }
    if (points_data_array.num_components != 3)
    {
      printf("[load_PiecePoints] points data array has %d components but expected 3\n", (int)points_data_array.num_components);
      return false;
    }
    if (points_data_array.values.type == DataArray::Type::Float32)
    {
      out_points.resize(points_count);
      for (int i = 0; i < points_count; i++)
      {
        out_points[i].x = points_data_array.values.f32[i * 3 + 0];
        out_points[i].y = points_data_array.values.f32[i * 3 + 1];
        out_points[i].z = points_data_array.values.f32[i * 3 + 2];
      }
    }
    else if (points_data_array.values.type == DataArray::Type::Float64)
    {
      out_points.resize(points_count);
      for (int i = 0; i < points_count; i++)
      {
        out_points[i].x = points_data_array.values.f64[i * 3 + 0];
        out_points[i].y = points_data_array.values.f64[i * 3 + 1];
        out_points[i].z = points_data_array.values.f64[i * 3 + 2];
      }
    }
    else
    {
      printf("[load_PiecePoints] points data array has unsupported type %d\n", (int)points_data_array.values.type);
      return false;
    }
    return true;
  }

  bool load_PiecePointData(pugi::xml_node piece_node, int points_count, const RawDataset &data, std::vector<DataArray> &point_data_arrays)
  {
    auto point_data_node = piece_node.child(L"PointData");
    if (!point_data_node)
    {
      //it is totally ok, we can have all the data in cells or have a vtk file describing the
      //structure of the model, without actual data inside
      return true;
    }

    for (pugi::xml_node data_array_node : point_data_node.children(L"DataArray"))
    {
      DataArray data_array = load_data_array(data, data_array_node);
      if (data_array.num_elements != points_count)
      {
        printf("[load_PiecePointData] data array %s has %d elements but expected %d, skipping it.\n", 
               data_array.name.c_str(), (int)data_array.num_elements, points_count);
      }
      else
      {
        save_split_array(data_array, point_data_arrays);
      }
    }
    return true;
  }

  bool load_PieceCellData(pugi::xml_node piece_node, int cells_count, const RawDataset &data, std::vector<DataArray> &cell_data_arrays)
  {
    auto cell_data_node = piece_node.child(L"CellData");
    if (!cell_data_node)
    {
      //it is totally ok, we can have all the data in points or have a vtk file describing the
      //structure of the model, without actual data inside
      return true;
    }

    for (pugi::xml_node data_array_node : cell_data_node.children(L"DataArray"))
    {
      DataArray data_array = load_data_array(data, data_array_node);
      if (data_array.num_elements != cells_count)
      {
        printf("[load_PieceCellData] data array %s has %d elements but expected %d, skipping it.\n", 
               data_array.name.c_str(), (int)data_array.num_elements, cells_count);
      }
      else
      {
        save_split_array(data_array, cell_data_arrays);
      }
    }
    return true;
  }

  bool load_PieceCells(pugi::xml_node piece_node, const RawDataset &data, UnstructuredGrid &out_grid,
                       const wchar_t *cells_name = L"Cells", VTKCellType fixed_cell_type = VTKCellType::VTK_EMPTY_CELL)
  {
    auto cells_node = piece_node.child(cells_name);
    if (!cells_node)
    {
      printf("[load_PieceCells] %ls node is missing\n", cells_name);
      return false;
    }

    pugi::xml_node connectivity_node, offsets_node, types_node;

    for (pugi::xml_node data_array_node : cells_node.children(L"DataArray"))
    {
      std::wstring name = data_array_node.attribute(L"Name").value();
      if (name == L"connectivity")
        connectivity_node = data_array_node;
      else if (name == L"offsets")
        offsets_node = data_array_node;
      else if (name == L"types")
        types_node = data_array_node;
    }

    if (!connectivity_node)
    {
      printf("[load_PieceCells] connectivity node is missing in %ls\n", cells_name);
      return false;
    }
    if (!offsets_node)
    {
      printf("[load_PieceCells] offsets node is missing in %ls\n", cells_name);
      return false;
    }
    if (fixed_cell_type == VTKCellType::VTK_EMPTY_CELL && !types_node)
    {
      printf("[load_PieceCells] types node is missing in %ls\n", cells_name);
      return false;
    }

    std::vector<id_t> connectivity;
    std::vector<id_t> offsets;
    std::vector<int> types;

    {
      DataArray connectivity_data_array = load_data_array(data, connectivity_node);
      if (connectivity_data_array.num_elements == 0)
      {
        printf("[load_PieceCells] connectivity data array is empty\n");
        return false;
      }
      convert_values<id_t>(connectivity_data_array.values, connectivity);
    }
    {
      DataArray offsets_data_array = load_data_array(data, offsets_node);
      if (offsets_data_array.num_elements != out_grid.cells_count)
      {
        printf("[load_PieceCells] offsets data array has %d elements but expected %d\n", 
               (int)offsets_data_array.num_elements, (int)out_grid.cells_count);
        return false;
      }
      convert_values<id_t>(offsets_data_array.values, offsets);
    }

    if (fixed_cell_type == VTKCellType::VTK_EMPTY_CELL)
    {
      DataArray types_data_array = load_data_array(data, types_node);
      if (types_data_array.num_elements != out_grid.cells_count)
      {
        printf("[load_PieceCells] types data array has %d elements but expected %d\n", 
               (int)types_data_array.num_elements, (int)out_grid.cells_count);
        return false;
      }
      convert_values<int>(types_data_array.values, types);
    }
    else
    {
      types.resize(out_grid.cells_count, (int)fixed_cell_type);
    }

    id_t prev_offset = 0;
    id_t prev_cells_size = out_grid.p_cells.size();
    out_grid.p_cells.resize(out_grid.cells_count);

    for (int i = 0; i < out_grid.cells_count; i++)
    {
      if (prev_offset >= connectivity.size())
      {
        printf("[load_PieceCells] invalid offset for cell %d: %d >= %zu (connectivity size)\n", i, prev_offset, connectivity.size());
        return false;
      }

      out_grid.p_cells[i].indices_offset = prev_offset;
      out_grid.p_cells[i].indices_count  = offsets[i] - prev_offset;
      out_grid.p_cells[i].type = (VTKCellType)types[i];
      prev_offset = offsets[i];
    }

    out_grid.p_indices = std::move(connectivity);

    return true;
  }

  bool load_PolyDataPiece(pugi::xml_node piece_node, const RawDataset &data, UnstructuredGrid &out_u_grid)
  {
    uint64_t NumberOfPoints = piece_node.attribute(L"NumberOfPoints").as_ullong(0);
    uint64_t NumberOfVerts  = piece_node.attribute(L"NumberOfVerts").as_ullong(0);
    uint64_t NumberOfLines  = piece_node.attribute(L"NumberOfLines").as_ullong(0);
    uint64_t NumberOfStrips = piece_node.attribute(L"NumberOfStrips").as_ullong(0);
    uint64_t NumberOfPolys  = piece_node.attribute(L"NumberOfPolys").as_ullong(0);

    if (NumberOfPoints == 0)
    {
      printf("[load_PolyDataPiece] NumberOfPoints is 0 or missing\n");
      return false;
    }
    else if (NumberOfVerts + NumberOfLines + NumberOfStrips + NumberOfPolys == 0)
    {
      printf("[load_PolyDataPiece] at least one of NumberOfVerts, NumberOfLines, NumberOfStrips, NumberOfPolys should be non-zero\n");
      return false;
    }

    out_u_grid.points_count = NumberOfPoints;
    out_u_grid.cells_count  = NumberOfVerts + NumberOfLines + NumberOfStrips + NumberOfPolys;

    bool ok = load_PiecePoints(piece_node, out_u_grid.points_count, data, out_u_grid.points);
    if (!ok)
      return false;
    
    ok = load_PiecePointData(piece_node, out_u_grid.points_count, data, out_u_grid.point_data_arrays);
    if (!ok)
      return false;
    
    if (NumberOfVerts > 0)
    {
      ok = load_PieceCells(piece_node, data, out_u_grid, L"Verts", VTKCellType::VTK_VERTEX);
      if (!ok)
        return false;
    }

    if (NumberOfLines > 0)
    {
      ok = load_PieceCells(piece_node, data, out_u_grid, L"Lines", VTKCellType::VTK_LINE);
      if (!ok)
        return false;
    }

    if (NumberOfStrips > 0)
    {
      ok = load_PieceCells(piece_node, data, out_u_grid, L"Strips", VTKCellType::VTK_TRIANGLE_STRIP);
      if (!ok)
        return false;
    }

    if (NumberOfPolys > 0)
    {
      ok = load_PieceCells(piece_node, data, out_u_grid, L"Polys", VTKCellType::VTK_POLYGON);
      if (!ok)
        return false;
    }

    ok = load_PieceCellData(piece_node, out_u_grid.cells_count, data, out_u_grid.cell_data_arrays);
    if (!ok)
      return false;

    return true;
  }

  bool load_PolyData(pugi::xml_node node, const RawDataset &data, const std::string &name, std::vector<UnstructuredGrid> &out_unstructured_grids)
  {
    int num_pieces = 0;
    for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling())
    {
      if (std::wstring(child.name()) == L"Piece")
      {
        out_unstructured_grids.emplace_back();
        out_unstructured_grids.back().name = name + "_" + std::to_string(num_pieces);
        bool ok = load_PolyDataPiece(child, data, out_unstructured_grids.back());
        if (!ok)
          out_unstructured_grids.erase(out_unstructured_grids.end() - 1);
        else
          num_pieces++;
      }
    }

    if (out_unstructured_grids.size() == 0)
    {
      printf("[load_PolyData] no valid PolyData pieces found\n");
      return false;
    }

    if (num_pieces == 1)
      out_unstructured_grids.back().name = name;

    return true;
  }

  bool load_UnstructuredGridPiece(pugi::xml_node piece_node, const RawDataset &data, UnstructuredGrid &out_u_grid)
  {
    uint64_t NumberOfPoints = piece_node.attribute(L"NumberOfPoints").as_ullong(0);
    uint64_t NumberOfCells  = piece_node.attribute(L"NumberOfCells").as_ullong(0);

    if (NumberOfPoints == 0)
    {
      printf("[load_UnstructuredGridPiece] NumberOfPoints is 0 or missing\n");
      return false;
    }
    if (NumberOfCells == 0)
    {
      printf("[load_UnstructuredGridPiece] NumberOfCells is 0 or missing\n");
      return false;
    }
    out_u_grid.points_count = NumberOfPoints;
    out_u_grid.cells_count  = NumberOfCells;

    bool ok = load_PiecePoints(piece_node, out_u_grid.points_count, data, out_u_grid.points);
    if (!ok)
      return false;
    
    ok = load_PiecePointData(piece_node, out_u_grid.points_count, data, out_u_grid.point_data_arrays);
    if (!ok)
      return false;
    
    ok = load_PieceCells(piece_node, data, out_u_grid, L"Cells");
    if (!ok)
      return false;

    ok = load_PieceCellData(piece_node, out_u_grid.cells_count, data, out_u_grid.cell_data_arrays);
    if (!ok)
      return false;

    return true;    
  }

  bool load_DataArrays_from_FieldData(pugi::xml_node node, const RawDataset &data, unsigned pieces, 
                                      std::vector<UnstructuredGrid> &out_unstructured_grids)
  {
    for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling())
    {
      if (std::wstring(child.name()) == L"DataArray")
      {
        std::string da_name = ws2s(child.attribute(L"Name").as_string());
        uint64_t tuples_count = child.attribute(L"NumberOfTuples").as_ullong(0);
        if (tuples_count == 0)
        {
          printf("[load_DataArrays_from_FieldData] Unstructured grid has field data with data array %s, but it's size is undefined\n", da_name.c_str());
          return false;
        }

        int point_piece_id = -1;
        int cell_piece_id = -1;
        for (int i=out_unstructured_grids.size()-pieces;i<out_unstructured_grids.size();i++)
        {
          if (tuples_count == out_unstructured_grids[i].cells_count)
          {
            cell_piece_id = i;
            break;
          }
          if (tuples_count == out_unstructured_grids[i].points_count)
          {
            point_piece_id = i;
            break;
          }
        }

        if (point_piece_id != -1)
        {
          DataArray data_array = load_data_array(data, child);
          if (data_array.num_elements != tuples_count)
          {
            printf("[load_DataArrays_from_FieldData] data array %s has %d elements but expected %d, skipping it.\n", 
                   data_array.name.c_str(), (int)data_array.num_elements, (int)tuples_count);
          }
          else
          {
            save_split_array(data_array, out_unstructured_grids[point_piece_id].point_data_arrays);
          }
        }
        else if (cell_piece_id != -1)
        {
          DataArray data_array = load_data_array(data, child);
          if (data_array.num_elements != tuples_count)
          {
            printf("[load_DataArrays_from_FieldData] data array %s has %d elements but expected %d, skipping it.\n", 
                   data_array.name.c_str(), (int)data_array.num_elements, (int)tuples_count);
          }
          else
          {
            save_split_array(data_array, out_unstructured_grids[cell_piece_id].cell_data_arrays);
          }
        }
      }
    }
    return true;
  }

  bool load_UnstructuredGrid(pugi::xml_node node, const RawDataset &data, const std::string &name,
                             std::vector<UnstructuredGrid> &out_unstructured_grids)
  {
    int num_pieces = 0;
    for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling())
    {
      if (std::wstring(child.name()) == L"Piece")
      {
        out_unstructured_grids.emplace_back();
        out_unstructured_grids.back().name = name+"_"+std::to_string(num_pieces);
        bool ok = load_UnstructuredGridPiece(child, data, out_unstructured_grids.back());
        if (!ok)
          out_unstructured_grids.erase(out_unstructured_grids.end() - 1);
        else
          num_pieces++;
      }
    }

    for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling())
    {
      if (std::wstring(child.name()) == L"FieldData")
      {
        load_DataArrays_from_FieldData(child, data, num_pieces, out_unstructured_grids);
      }
    }

    if (out_unstructured_grids.size() == 0)
    {
      printf("[load_UnstructuredGrid] no valid PolyData pieces found\n");
      return false;
    }

    if (num_pieces == 1)
      out_unstructured_grids.back().name = name;

    return true;
  }

  int get_type_size(const DataArray &arr)
  {
    unsigned elem_size = 1;
    switch (arr.values.type)
    {
    case vtk::DataArray::Type::Int8:    elem_size = sizeof(int8_t); break;
    case vtk::DataArray::Type::UInt8:   elem_size = sizeof(uint8_t); break;
    case vtk::DataArray::Type::Int16:   elem_size = sizeof(int16_t); break;
    case vtk::DataArray::Type::UInt16:  elem_size = sizeof(uint16_t); break;
    case vtk::DataArray::Type::Int32:   elem_size = sizeof(int32_t); break;
    case vtk::DataArray::Type::UInt32:  elem_size = sizeof(uint32_t); break;
    case vtk::DataArray::Type::Int64:   elem_size = sizeof(int64_t); break;
    case vtk::DataArray::Type::UInt64:  elem_size = sizeof(uint64_t); break;
    case vtk::DataArray::Type::Float32: elem_size = sizeof(float); break;
    case vtk::DataArray::Type::Float64: elem_size = sizeof(double); break;
    default: break;
    }
    return elem_size;
  }

  void print_dataset_info(vtk::Dataset &dataset)
  {
    printf("%zu unstructured grids\n", dataset.unstructured_grids.size());
    for (int i = 0; i < dataset.unstructured_grids.size(); i++)
    {
      const UnstructuredGrid &ug = dataset.unstructured_grids[i];
      std::vector<std::string> type_names = {"UNDEFINED", "SURFACE", "VOLUME", "MIXED"};

      bool small_dataset = ug.points.size() < 20;
      printf("unstructured grid %d\n", i);
      printf("type = %s\n", type_names[(int)ug.type].c_str());
      printf("%zu points\n", ug.points.size());
      if (small_dataset)
      {
        printf("points: ");
        for (int j = 0; j < ug.points.size(); j++)
        {
          printf("(%f %f %f) ", ug.points[j].x,
                 ug.points[j].y, ug.points[j].z);
        }
        printf("\n");
      }
      printf("%zu cells\n", ug.p_cells.size());

      const char *typeNames[] = {"Int8", "UInt8", "Int16", "UInt16", "Int32", "UInt32", "Int64", "UInt64", "Float32", "Float64"};

      printf("%zu point data arrays\n", ug.point_data_arrays.size());
      for (int j = 0; j < ug.point_data_arrays.size(); j++)
      {
        const auto &arr = ug.point_data_arrays[j];
        printf("Array %d: \"%s\", type = %dx%s, %d elements\n", j, arr.name.c_str(), (int)arr.num_components,
               typeNames[(int)arr.values.type], (int)arr.num_elements);
      }

      printf("%d cell data arrays\n", (int)ug.cell_data_arrays.size());
      for (int j = 0; j < ug.cell_data_arrays.size(); j++)
      {
        const auto &arr = ug.cell_data_arrays[j];
        printf("Array %d: \"%s\", type = %dx%s, %d elements\n", j, arr.name.c_str(), (int)arr.num_components,
               typeNames[(int)arr.values.type], (int)arr.num_elements);
      }

      std::vector<int> used_cell_types(VTK_NUMBER_OF_CELL_TYPES, 0);

      for (int j = 0; j < ug.p_cells.size(); j++)
        used_cell_types[(int)ug.p_cells[j].type]++;

      int used_cell_types_count = 0;
      for (int j = 0; j < used_cell_types.size(); j++)
      {
        if (used_cell_types[j])
          used_cell_types_count++;
      }

      printf("%d cell types used:\n", used_cell_types_count);
      for (int j = 0; j < used_cell_types.size(); j++)
      {
        if (used_cell_types[j])
          printf("%s : %d\n", get_name((VTKCellType)j).c_str(), used_cell_types[j]);
      }
      printf("\n");
    }

    uint64_t points_mem = 0;
    uint64_t points_data_mem = 0;
    uint64_t packed_cells_mem = 0;
    uint64_t cells_data_mem = 0;

    for (const auto &ug : dataset.unstructured_grids)
    {
      points_mem += sizeof(float3) * ug.points.capacity();

      for (const auto &arr : ug.point_data_arrays)
        points_data_mem += get_type_size(arr) * arr.num_elements * arr.num_components;
      
      for (const auto &arr : ug.cell_data_arrays)
        cells_data_mem += get_type_size(arr) * arr.num_elements * arr.num_components;

      packed_cells_mem += sizeof(PackedCell) * ug.p_cells.capacity();
      packed_cells_mem += sizeof(id_t) * ug.p_indices.capacity();
    }

    printf("points mem = %.2f Mb\n", points_mem / 1024.0f / 1024.0f);
    printf("points data mem = %.2f Mb\n", points_data_mem / 1024.0f / 1024.0f);
    printf("cells mem = %.2f Mb\n", packed_cells_mem / 1024.0f / 1024.0f);
    printf("cells data mem = %.2f Mb\n", cells_data_mem / 1024.0f / 1024.0f);
    printf("total mem = %.2f Mb\n", (points_mem + points_data_mem + cells_data_mem + packed_cells_mem) / 1024.0f / 1024.0f);
  }

  bool read_vtk_dataset(const fs::path& filename, Dataset& dataset)
  {
    dataset.unstructured_grids.clear();
    return read_vtk_dataset(filename, dataset, filename.stem().string() + "_");
  }
  bool read_vtk_dataset(const fs::path& filename, Dataset& dataset, const std::string &name_prefix)
  {
    bool verbose = true;
    RawDataset raw_dataset;
    bool primal_parsed = primal_parse(filename, raw_dataset); 
    if (!primal_parsed)
      return false;
    
    pugi::xml_document xml_doc;
    auto loaded = xml_doc.load_buffer((void *)raw_dataset.xml_data.data(), raw_dataset.xml_data.size());
    if (!loaded)
    {
      printf("file = %s\n", raw_dataset.xml_data.data());
      printf("left = %s\n", raw_dataset.xml_data.data() + 344);
      printf("[read_vtk_dataset] pugixml failed to load xml file. xml_parse_status: %d\n", (int)loaded.status);
      printf("offset = %d\n", (unsigned)loaded.offset);
      printf("description = %s\n", loaded.description());
      return false;
    }

    if (verbose)
    {
      printf("%zu binary blocks\n", raw_dataset.binary_blocks.size());
      // for (int i = 0; i < raw_dataset.binary_blocks.size(); i++)
      // {
      //   printf("block %d, type = %s, size = %zu\n", i, 
      //          raw_dataset.binary_block_formats[i] == BinaryBlockFormat::ASCII ? "ascii" : (
      //          raw_dataset.binary_block_formats[i] == BinaryBlockFormat::BINARY_BASE64 ? "base64" : "binary_raw"), 
      //          raw_dataset.binary_blocks[i].size());
      // }
      // printf("xml\n %s\n", raw_dataset.xml_data.data());
    }

    pugi::xml_node root = xml_doc.child(L"VTKFile");
    if (root == nullptr)
    {
      printf("[read_vtk_dataset] root node not found\n");
      return false;
    }

    const auto attribute_type = root.attribute(L"type");

    if (!attribute_type)
    {
      printf("[read_vtk_dataset] missing type attribute on VTKFile\n");
      return false;
    }

    const auto vtk_type = attribute_type.as_string(nullptr);

    if (nullptr == vtk_type)
    {
      printf("[read_vtk_dataset] type attribute on VTKFile should be a string\n");
      return false;
    }

    if (0 == wcscmp(vtk_type, L"vtkMultiBlockDataSet"))
    {
      const auto multi_block_data_set_node = root.child(L"vtkMultiBlockDataSet");

      if (!multi_block_data_set_node)
      {
        printf("[read_vtk_dataset] missing vtkMultiBlockDataSet");
        return false;
      }

      if (!load_mutiblock_data_set(multi_block_data_set_node, filename.parent_path(), name_prefix, dataset))
      {
        printf("[read_vtk_dataset] failed to load multiblock data");
        return false;
      }
    }
    else
    {
      pugi::xml_node unstructured_grid_node = root.child(L"UnstructuredGrid");
      pugi::xml_node poly_data_node = root.child(L"PolyData");

      if (unstructured_grid_node != nullptr && poly_data_node != nullptr)
      {
        printf(
          "[read_vtk_dataset] both UnstructuredGrid and PolyData are present\n"
        );
        return false;
      }

      if (unstructured_grid_node == nullptr && poly_data_node == nullptr)
      {
        printf(
          "[read_vtk_dataset] UnstructuredGrid or PolyData node not found\n"
        );
        return false;
      }

      if (unstructured_grid_node != nullptr)
      {
        bool ok = load_UnstructuredGrid(unstructured_grid_node, raw_dataset, name_prefix, dataset.unstructured_grids);

        if (!ok)
        {
          return false;
        }
      }

      if (poly_data_node != nullptr)
      {
        bool ok = load_PolyData(poly_data_node, raw_dataset, name_prefix, dataset.unstructured_grids);

        if (!ok)
        {
          return false;
        }
      }
    }

    for (int i = 0; i < dataset.unstructured_grids.size(); i++)
    {
      std::vector<bool> used_cell_types(VTK_NUMBER_OF_CELL_TYPES, false);
        
      for (int j = 0; j < dataset.unstructured_grids[i].p_cells.size(); j++)
        used_cell_types[(int)dataset.unstructured_grids[i].p_cells[j].type] = true;
    
      bool all_2d = true;
      bool all_3d = true;

      for (int j = 0; j < VTK_NUMBER_OF_CELL_TYPES; j++)
      {
        if (used_cell_types[j])
        {
          all_2d = all_2d && is_2d_cell((VTKCellType)j);
          all_3d = all_3d && !is_2d_cell((VTKCellType)j);
        }
      }

      if (all_2d)
        dataset.unstructured_grids[i].type = UnstructuredGrid::Type::SURFACE;
      else if (all_3d)
        dataset.unstructured_grids[i].type = UnstructuredGrid::Type::VOLUME;
      else
        dataset.unstructured_grids[i].type = UnstructuredGrid::Type::MIXED;
    }
    
    return true;
  }

  void get_bbox(const Dataset &dataset, float3 *min_pos, float3 *max_pos)
  {
    *min_pos = float3(1e9,1e9,1e9);
    *max_pos = float3(-1e9,-1e9,-1e9);

    for (const auto &grid : dataset.unstructured_grids)
    {
      for (const float3 &p : grid.points)
      {
        *min_pos = min(*min_pos, p);
        *max_pos = max(*max_pos, p);
      }
    }
  }

  LiteMath::float4x4 rescale_dataset(Dataset &dataset, float3 min_pos, float3 max_pos)
  {
    assert(dataset.unstructured_grids.size() > 0);

    float3 mesh_min, mesh_max;
    get_bbox(dataset, &mesh_min, &mesh_max);

    float3 mesh_size = mesh_max - mesh_min;
    float3 target_size = max_pos - min_pos;
    float3 scale3 = target_size/mesh_size;
    float scale = std::min(scale3.x, std::min(scale3.y, scale3.z));

    //changing poditions, .w coord is preserved
    for (auto &grid : dataset.unstructured_grids)
      for (float3 &p : grid.points)
        p = min_pos + scale*(p - mesh_min);

    //it is only move and rescale, so now changes to normals are required

    LiteMath::float4x4 trans = translate4x4(min_pos)*scale4x4(float3(scale))*translate4x4(-mesh_min);
    return trans;
  }

  void calculate_data_array_limits(const DataArray &array, double *min_val, double *max_val)
  {
    if (array.values.range_set)
    {
      *min_val = array.values.range_min;
      *max_val = array.values.range_max;
      //printf("Data array %s range is set to [%f, %f]\n", array.name.c_str(), array.values.range_min, array.values.range_max);
      return;
    }
    assert(array.values.count);
    switch (array.values.type)
    {
    case DataArray::Type::Int8:
      *min_val = array.values.i8[0];
      *max_val = array.values.i8[0];
      for (uint64_t i = 0; i < array.values.count; i++)
      {
        *min_val = std::min<double>(*min_val, array.values.i8[i]);
        *max_val = std::max<double>(*max_val, array.values.i8[i]);
      }
      break;
    case DataArray::Type::UInt8:
      *min_val = array.values.u8[0];
      *max_val = array.values.u8[0];
      for (uint64_t i = 0; i < array.values.count; i++)
      {
        *min_val = std::min<double>(*min_val, array.values.u8[i]);
        *max_val = std::max<double>(*max_val, array.values.u8[i]);
      }
      break;
    case DataArray::Type::Int16:
      *min_val = array.values.i16[0];
      *max_val = array.values.i16[0];
      for (uint64_t i = 0; i < array.values.count; i++)
      {
        *min_val = std::min<double>(*min_val, array.values.i16[i]);
        *max_val = std::max<double>(*max_val, array.values.i16[i]);
      }
      break;
    case DataArray::Type::UInt16:
      *min_val = array.values.u16[0];
      *max_val = array.values.u16[0];
      for (uint64_t i = 0; i < array.values.count; i++)
      {
        *min_val = std::min<double>(*min_val, array.values.u16[i]);
        *max_val = std::max<double>(*max_val, array.values.u16[i]);
      }
      break;
    case DataArray::Type::Int32:
      *min_val = array.values.i32[0];
      *max_val = array.values.i32[0];
      for (uint64_t i = 0; i < array.values.count; i++)
      {
        *min_val = std::min<double>(*min_val, array.values.i32[i]);
        *max_val = std::max<double>(*max_val, array.values.i32[i]);
      }
      break;
    case DataArray::Type::UInt32:
      *min_val = array.values.u32[0];
      *max_val = array.values.u32[0];
      for (uint64_t i = 0; i < array.values.count; i++)
      {
        *min_val = std::min<double>(*min_val, array.values.u32[i]);
        *max_val = std::max<double>(*max_val, array.values.u32[i]);
      }
      break;
    case DataArray::Type::Int64:
      *min_val = array.values.i64[0];
      *max_val = array.values.i64[0];
      for (uint64_t i = 0; i < array.values.count; i++)
      {
        *min_val = std::min<double>(*min_val, array.values.i64[i]);
        *max_val = std::max<double>(*max_val, array.values.i64[i]);
      }
      break;
    case DataArray::Type::UInt64:
      *min_val = array.values.u64[0];
      *max_val = array.values.u64[0];
      for (uint64_t i = 0; i < array.values.count; i++)
      {
        *min_val = std::min<double>(*min_val, array.values.u64[i]);
        *max_val = std::max<double>(*max_val, array.values.u64[i]);
      }
      break;
    case DataArray::Type::Float32:
      *min_val = array.values.f32[0];
      *max_val = array.values.f32[0];
      for (uint64_t i = 0; i < array.values.count; i++)
      {
        *min_val = std::min<double>(*min_val, array.values.f32[i]);
        *max_val = std::max<double>(*max_val, array.values.f32[i]);
      }
      break;
    case DataArray::Type::Float64:
      *min_val = array.values.f64[0];
      *max_val = array.values.f64[0];
      for (uint64_t i = 0; i < array.values.count; i++)
      {
        *min_val = std::min<double>(*min_val, array.values.f64[i]);
        *max_val = std::max<double>(*max_val, array.values.f64[i]);
      }
      break;
    }
  }

  

  DataArray::~DataArray()
  {
    clear();
  }

  void DataArray::clear()
  {
    if (values.count == 0)
      return;

    //printf("%p delete %lu\n", values.i8, values.count);
    num_elements = 0;
    num_components = 0;
    values.count = 0;
    values.type = DataArray::Type::Int8;
    name = "";

    switch (values.type)
    {
    case DataArray::Type::Int8:
      delete[] values.i8;
      break;
    case DataArray::Type::UInt8:
      delete[] values.u8;
      break;
    case DataArray::Type::Int16:
      delete[] values.i16;
      break;
    case DataArray::Type::UInt16:
      delete[] values.u16;
      break;
    case DataArray::Type::Int32:
      delete[] values.i32;
      break;
    case DataArray::Type::UInt32:
      delete[] values.u32;
      break;
    case DataArray::Type::Int64:
      delete[] values.i64;
      break;
    case DataArray::Type::UInt64:
      delete[] values.u64;
      break;
    case DataArray::Type::Float32:
      delete[] values.f32;
      break;
    case DataArray::Type::Float64:
      delete[] values.f64;
      break;
    }

    values.i8 = nullptr;
  }

  DataArray::DataArray(const DataArray &other)
  {
    *this = other;
  }

  DataArray &DataArray::operator=(const DataArray &other)
  {
    if (this == &other)
      return *this;
    clear();
    //printf("%p copy %lu\n", other.values.i8, other.values.count);
    num_components = other.num_components;
    num_elements = other.num_elements;
    name = other.name;
    component_names = other.component_names;
    values.count = other.values.count;
    values.type = other.values.type;
    values.range_min = other.values.range_min;
    values.range_max = other.values.range_max;
    values.range_set = other.values.range_set;

    switch (other.values.type)
    {
    case DataArray::Type::Int8:
      values.i8 = new int8_t[other.values.count];
      memcpy(values.i8, other.values.i8, sizeof(int8_t)*other.values.count);
      break;
    case DataArray::Type::UInt8:
      values.u8 = new uint8_t[other.values.count];
      memcpy(values.u8, other.values.u8, sizeof(uint8_t)*other.values.count);
      break;
    case DataArray::Type::Int16:
      values.i16 = new int16_t[other.values.count];
      memcpy(values.i16, other.values.i16, sizeof(int16_t)*other.values.count);
      break;
    case DataArray::Type::UInt16:
      values.u16 = new uint16_t[other.values.count];
      memcpy(values.u16, other.values.u16, sizeof(uint16_t)*other.values.count);
      break;
    case DataArray::Type::Int32:
      values.i32 = new int32_t[other.values.count];
      memcpy(values.i32, other.values.i32, sizeof(int32_t)*other.values.count);
      break;
    case DataArray::Type::UInt32:
      values.u32 = new uint32_t[other.values.count];
      memcpy(values.u32, other.values.u32, sizeof(uint32_t)*other.values.count);
      break;
    case DataArray::Type::Int64:
      values.i64 = new int64_t[other.values.count];
      memcpy(values.i64, other.values.i64, sizeof(int64_t)*other.values.count);
      break;
    case DataArray::Type::UInt64:
      values.u64 = new uint64_t[other.values.count];
      memcpy(values.u64, other.values.u64, sizeof(uint64_t)*other.values.count);
      break;
    case DataArray::Type::Float32:
      values.f32 = new float[other.values.count];
      memcpy(values.f32, other.values.f32, sizeof(float)*other.values.count);
      break;
    case DataArray::Type::Float64:
      values.f64 = new double[other.values.count];
      memcpy(values.f64, other.values.f64, sizeof(double)*other.values.count);
      break;
    }
    return *this;
  }

  DataArray::DataArray(DataArray &&other)
  {
    *this = std::move(other);
  }

  DataArray &DataArray::operator=(DataArray &&other)
  {
    if (this == &other)
      return *this;
    //printf("%p move %lu\n", other.values.i8, other.values.count);
    clear();
    values = other.values;
    num_components = other.num_components;
    num_elements = other.num_elements;
    name = other.name;
    component_names = other.component_names;

    other.values.count = 0;
    other.values.type = DataArray::Type::Int8;
    other.values.i8 = nullptr;

    return *this;
  }

  void calculate_common_data_array_ranges(Dataset &dataset)
  {
    struct DataArrayDescriptor
    {
      std::string name;
      DataArray::Type type = DataArray::Type::Int8;
      unsigned num_components = 0;
      bool is_point_data = false; // point or cell data
      double range_min = 0, range_max = 0;
      std::vector<DataArray *> arrays; //all arrays that fit this description, one for each unstructured grid
    };

    //find all unique data arrays
    std::vector<DataArrayDescriptor> data_array_descriptors;
    auto find_descriptor = [&data_array_descriptors](DataArray *array, bool is_point_data)
    {
      int d_index = -1;
      for (int i=0;i<data_array_descriptors.size();i++)
      {
        if (data_array_descriptors[i].name == array->name &&
            data_array_descriptors[i].type == array->values.type &&
            data_array_descriptors[i].num_components == array->num_components &&
            data_array_descriptors[i].is_point_data == is_point_data)
        {
          d_index = i;
          break;
        }
      }
      return d_index;
    };
    
    for (int ug_idx=0;ug_idx<dataset.unstructured_grids.size();ug_idx++)
    {
      for (auto &pa : dataset.unstructured_grids[ug_idx].point_data_arrays)
      {
        int d_index = find_descriptor(&pa, true);
        if (d_index == -1)
        {
          DataArrayDescriptor desc;
          desc.name = pa.name;
          desc.type = pa.values.type;
          desc.num_components = pa.num_components;
          desc.is_point_data = true;
          desc.arrays = std::vector<DataArray *>(dataset.unstructured_grids.size(), nullptr);
          data_array_descriptors.push_back(desc);
          d_index = data_array_descriptors.size()-1;
        }
        data_array_descriptors[d_index].arrays[ug_idx] = &pa;
      }
      for (auto &ca : dataset.unstructured_grids[ug_idx].cell_data_arrays)
      {
        int d_index = find_descriptor(&ca, false);
        if (d_index == -1)
        {
          DataArrayDescriptor desc;
          desc.name = ca.name;
          desc.type = ca.values.type;
          desc.num_components = ca.num_components;
          desc.is_point_data = false;
          desc.arrays = std::vector<DataArray *>(dataset.unstructured_grids.size(), nullptr);
          data_array_descriptors.push_back(desc);
          d_index = data_array_descriptors.size()-1;
        }
        data_array_descriptors[d_index].arrays[ug_idx] = &ca;
      }
    }

    //find ranges for each descriptor and save them in arrays
    for (auto &desc : data_array_descriptors)
    {
      desc.range_min = std::numeric_limits<double>::max();
      desc.range_max = std::numeric_limits<double>::min();
      for (auto *array : desc.arrays)
      {
        if (array == nullptr)
          continue;
        switch (array->values.type)
        {
          case DataArray::Type::Int8:
          for (uint64_t i = 0; i < array->values.count; i++)
          {
            desc.range_min = std::min<double>(desc.range_min, array->values.i8[i]);
            desc.range_max = std::max<double>(desc.range_max, array->values.i8[i]);
          }
          break;
          case DataArray::Type::UInt8:
          for (uint64_t i = 0; i < array->values.count; i++)
          {
            desc.range_min = std::min<double>(desc.range_min, array->values.u8[i]);
            desc.range_max = std::max<double>(desc.range_max, array->values.u8[i]);
          }
          break;
          case DataArray::Type::Int16:
          for (uint64_t i = 0; i < array->values.count; i++)
          {
            desc.range_min = std::min<double>(desc.range_min, array->values.i16[i]);
            desc.range_max = std::max<double>(desc.range_max, array->values.i16[i]);
          }
          break;
          case DataArray::Type::UInt16:
          for (uint64_t i = 0; i < array->values.count; i++)
          {
            desc.range_min = std::min<double>(desc.range_min, array->values.u16[i]);
            desc.range_max = std::max<double>(desc.range_max, array->values.u16[i]);
          }
          break;
          case DataArray::Type::Int32:
          for (uint64_t i = 0; i < array->values.count; i++)
          {
            desc.range_min = std::min<double>(desc.range_min, array->values.i32[i]);
            desc.range_max = std::max<double>(desc.range_max, array->values.i32[i]);
          }
          break;
          case DataArray::Type::UInt32:
          for (uint64_t i = 0; i < array->values.count; i++)
          {
            desc.range_min = std::min<double>(desc.range_min, array->values.u32[i]);
            desc.range_max = std::max<double>(desc.range_max, array->values.u32[i]);
          }
          break;
          case DataArray::Type::Int64:
          for (uint64_t i = 0; i < array->values.count; i++)
          {
            desc.range_min = std::min<double>(desc.range_min, array->values.i64[i]);
            desc.range_max = std::max<double>(desc.range_max, array->values.i64[i]);
          }
          break;
          case DataArray::Type::UInt64:
          for (uint64_t i = 0; i < array->values.count; i++)
          {
            desc.range_min = std::min<double>(desc.range_min, array->values.u64[i]);
            desc.range_max = std::max<double>(desc.range_max, array->values.u64[i]);
          }
          break;
          case DataArray::Type::Float32:
          for (uint64_t i = 0; i < array->values.count; i++)
          {
            desc.range_min = std::min<double>(desc.range_min, array->values.f32[i]);
            desc.range_max = std::max<double>(desc.range_max, array->values.f32[i]);
          }
          break;
          case DataArray::Type::Float64:
          for (uint64_t i = 0; i < array->values.count; i++)
          {
            desc.range_min = std::min<double>(desc.range_min, array->values.f64[i]);
            desc.range_max = std::max<double>(desc.range_max, array->values.f64[i]);
          }
          break;
        }
      }

      //expand range to prevent issues with all values being the same
      desc.range_min -= 1e-5f;
      desc.range_max += 1e-5f;
    }

    //check if all grids have the same arrays in exactly the same order
    //it is a fairly common case
    bool all_same_arrays = true;
    for (int i = 1; i < dataset.unstructured_grids.size() && all_same_arrays; i++)
    {
      if (dataset.unstructured_grids[i].point_data_arrays.size() != dataset.unstructured_grids[0].point_data_arrays.size())
      {
        all_same_arrays = false;
        break;
      }
      else
      {
        for (int j = 0; j < dataset.unstructured_grids[i].point_data_arrays.size() && all_same_arrays; j++)
        {
          if (dataset.unstructured_grids[i].point_data_arrays[j].name != dataset.unstructured_grids[0].point_data_arrays[j].name ||
              dataset.unstructured_grids[i].point_data_arrays[j].values.type != dataset.unstructured_grids[0].point_data_arrays[j].values.type ||
              dataset.unstructured_grids[i].point_data_arrays[j].num_components != dataset.unstructured_grids[0].point_data_arrays[j].num_components)
          {
            all_same_arrays = false;
            break;
          }
        }
      }

      if (dataset.unstructured_grids[i].cell_data_arrays.size() != dataset.unstructured_grids[0].cell_data_arrays.size())
      {
        all_same_arrays = false;
        break;
      }
      else
      {
        for (int j = 0; j < dataset.unstructured_grids[i].cell_data_arrays.size() && all_same_arrays; j++)
        {
          if (dataset.unstructured_grids[i].cell_data_arrays[j].name != dataset.unstructured_grids[0].cell_data_arrays[j].name ||
              dataset.unstructured_grids[i].cell_data_arrays[j].values.type != dataset.unstructured_grids[0].cell_data_arrays[j].values.type ||
              dataset.unstructured_grids[i].cell_data_arrays[j].num_components != dataset.unstructured_grids[0].cell_data_arrays[j].num_components)
          {
            all_same_arrays = false;
            break;
          }
        }
      }
    }

    //if all grids have the same arrays, we just set the range for all of them from descriptors
    //otherwise we ned to recreate the arrays in each grid, that includes 
    if (all_same_arrays)
    {
      for (auto &desc : data_array_descriptors)
      {
        for (auto *arr : desc.arrays)
        {
          assert(arr);
          arr->values.range_set = true;
          arr->values.range_min = desc.range_min;
          arr->values.range_max = desc.range_max;
        }
      }
    }
    else
    {
      int point_data_arrays_count = 0;
      int cell_data_arrays_count = 0;
      for (auto &desc : data_array_descriptors)
      {
        if (desc.is_point_data)
          point_data_arrays_count++;
        else
          cell_data_arrays_count++;
      }

      for (int ug_idx = 0; ug_idx < dataset.unstructured_grids.size(); ug_idx++)
      {
        std::vector<DataArray> new_point_data_arrays(point_data_arrays_count);
        std::vector<DataArray> new_cell_data_arrays(cell_data_arrays_count);
        int cur_pda = 0;
        int cur_cda = 0;

        for (auto &desc : data_array_descriptors)
        {
          if (desc.arrays[ug_idx]) //this grid has current array, move it to the new vector
          {
            if (desc.is_point_data)
              new_point_data_arrays[cur_pda] = *desc.arrays[ug_idx];
            else
              new_cell_data_arrays[cur_cda] = *desc.arrays[ug_idx];
          }
          else //creare entirely new array and fill it with average value
          {
            DataArray &arr = desc.is_point_data ? new_point_data_arrays[cur_pda] : new_cell_data_arrays[cur_cda];
            arr.num_components = desc.num_components;
            arr.num_elements = desc.is_point_data ? dataset.unstructured_grids[ug_idx].points_count : dataset.unstructured_grids[ug_idx].cells_count;
            arr.name = desc.name;
            arr.values.count = arr.num_components * arr.num_elements;
            arr.values.type = desc.type;
            double val = 0.5f * (desc.range_min + desc.range_max);
            printf("%s: %d x %f type %d\n", arr.name.c_str(), (int)arr.values.count, val, (int)desc.type);
            switch (desc.type)
            {
              case DataArray::Type::Int8:
                arr.values.i8 = new int8_t[arr.values.count];
                std::fill_n(arr.values.i8, arr.values.count, (int8_t)val);
                break;
              case DataArray::Type::UInt8:
                arr.values.u8 = new uint8_t[arr.values.count];
                std::fill_n(arr.values.u8, arr.values.count, (uint8_t)val);
                break;
              case DataArray::Type::Int16:
                arr.values.i16 = new int16_t[arr.values.count];
                std::fill_n(arr.values.i16, arr.values.count, (int16_t)val);
                break;
              case DataArray::Type::UInt16:
                arr.values.u16 = new uint16_t[arr.values.count];
                std::fill_n(arr.values.u16, arr.values.count, (uint16_t)val);
                break;
              case DataArray::Type::Int32:
                arr.values.i32 = new int32_t[arr.values.count];
                std::fill_n(arr.values.i32, arr.values.count, (int32_t)val);
                break;
              case DataArray::Type::UInt32:
                arr.values.u32 = new uint32_t[arr.values.count];
                std::fill_n(arr.values.u32, arr.values.count, (uint32_t)val);
                break;
              case DataArray::Type::Int64:
                arr.values.i64 = new int64_t[arr.values.count];
                std::fill_n(arr.values.i64, arr.values.count, (int64_t)val);
                break;
              case DataArray::Type::UInt64:
                arr.values.u64 = new uint64_t[arr.values.count];
                std::fill_n(arr.values.u64, arr.values.count, (uint64_t)val);
                break;
              case DataArray::Type::Float32:
                arr.values.f32 = new float[arr.values.count];
                std::fill_n(arr.values.f32, arr.values.count, (float)val);
                break;
              case DataArray::Type::Float64:
                arr.values.f64 = new double[arr.values.count];
                std::fill_n(arr.values.f64, arr.values.count, (double)val);
                break;
            }
          }

          if (desc.is_point_data)
          {
            new_point_data_arrays[cur_pda].values.range_set = true;
            new_point_data_arrays[cur_pda].values.range_min = desc.range_min;
            new_point_data_arrays[cur_pda].values.range_max = desc.range_max;
            cur_pda++; 
          }
          else
          {
            new_cell_data_arrays[cur_cda].values.range_set = true;
            new_cell_data_arrays[cur_cda].values.range_min = desc.range_min;
            new_cell_data_arrays[cur_cda].values.range_max = desc.range_max;
            cur_cda++;
          }
        }

        dataset.unstructured_grids[ug_idx].point_data_arrays = new_point_data_arrays;
        dataset.unstructured_grids[ug_idx].cell_data_arrays = new_cell_data_arrays;

        printf("%d descriptors\n", (int)data_array_descriptors.size());
        for (auto &pda : dataset.unstructured_grids[ug_idx].point_data_arrays)
        {
          if (pda.values.type != DataArray::Type::Float32)
            continue;
          printf("arr = {");
          for (int i = 0; i < pda.values.count; i++)
          {
            printf("%f ", pda.values.f32[i]);
          }
          printf("}\n");
        }
      }
    }
  }

  template <typename T>
  void prune_named_array(std::vector<T> &arr, std::vector<std::string> &whitelist)
  {
    std::vector<bool> keep_elems(arr.size(), false);
    for (int i = 0; i < arr.size(); i++)
    {
      keep_elems[i] = std::find(whitelist.begin(), whitelist.end(), arr[i].name) != whitelist.end();
    }

    uint32_t elems_remains = std::count(keep_elems.begin(), keep_elems.end(), true);
    std::vector<T> new_arr(elems_remains);
    uint32_t g_id = 0;
    for (int i = 0; i < arr.size(); i++)
    {
      if (keep_elems[i])
        new_arr[g_id++] = std::move(arr[i]);
      arr[i] = T();
    }
    arr = new_arr;
  }

  void prune_dataset(Dataset &dataset, bool prune_grids, std::vector<std::string> &grid_whitelist, 
                     bool prune_point_arrays, std::vector<std::string> &point_arrays_whitelist,
                     bool prune_cell_arrays, std::vector<std::string> &cell_arrays_whitelist)
  {
    //remove grids not in whitelist
    if (prune_grids)
      prune_named_array<UnstructuredGrid>(dataset.unstructured_grids, grid_whitelist);

    //for each of the remaining grids, remove arrays not in whitelist
    for (auto &g : dataset.unstructured_grids)
    {
      if (prune_point_arrays)
        prune_named_array<DataArray>(g.point_data_arrays, point_arrays_whitelist);
      if (prune_cell_arrays)
        prune_named_array<DataArray>(g.cell_data_arrays, cell_arrays_whitelist);
    }
  }

  void compress_float_channel(DataChannel &channel, DataChannel::Type comp_type, bool log_scale)
  {
    if (channel.type != DataChannel::Type::FLOAT)
    {
      printf("[compress_float_channel] Only float channels can be compressed\n");
      return;
    }

    //find values range
    float min_val = channel.data_f[0];
    float max_val = channel.data_f[0] + 1e-9f;
    if (channel.max_val == DataChannel::LIMIT_UNDEFINED || channel.min_val == DataChannel::LIMIT_UNDEFINED)
    {
      for (int i=1; i<channel.data_f.size(); i++)
      {
        min_val = std::min(min_val, channel.data_f[i]);
        max_val = std::max(max_val, channel.data_f[i]);
      }
    }
    else
    {
      min_val = channel.min_val;
      max_val = channel.max_val;
    }

    //transform all values to [0, 1] range
    if (log_scale)
    {
      //TODO
    }
    else
    {
      for (int i=0; i<channel.data_f.size(); i++)
        channel.data_f[i] = (channel.data_f[i] - min_val) / (max_val - min_val);
    }

    //compress them to 8-bit fixed-point
    if (comp_type == DataChannel::Type::FP8)
    {
      channel.data_fp8.resize(channel.data_f.size());
      for (int i=0; i<channel.data_f.size(); i++)
        channel.data_fp8[i] = 0xFF*channel.data_f[i] + 0.5f;

      // uint32_t hist[256] = {0};      
      // for (int i=0; i<channel.data_f.size(); i++)
      //   hist[channel.data_fp8[i]]++;
      // printf("hist = [");
      // for (int i=0; i<256; i++)
      //   printf("%u, ", hist[i]);
      // printf("]\n");

      channel.data_f.clear();
      channel.type = DataChannel::Type::FP8;
    }
    else
    {
      printf("[compress_float_channel] Unknown compression type\n");
    }
  }

  // rescales dataset, prints som einfo about it and drops grids and channels that are not used (set in load_options_blk, optional). 
  // TODO: load only metadata on preloading, load real data arrays here
  void finish_loading_vtk_dataset(Dataset &dataset, Block *load_options_blk)
  {
    print_dataset_info(dataset);
    rescale_dataset(dataset, float3(-0.9f), float3(0.9f));
    calculate_common_data_array_ranges(dataset);

    if (!load_options_blk)
      return;
    
    //prune dataset if the necessary arrays are present in load_options_blk
    bool prune_grids = load_options_blk->get_id("grids_whitelist") >= 0;
    bool prune_point_arrays = load_options_blk->get_id("point_arrays_whitelist") >= 0;
    bool prune_cell_arrays = load_options_blk->get_id("cell_arrays_whitelist") >= 0;
    std::vector<std::string> grid_whitelist, point_arrays_whitelist, cell_arrays_whitelist;
    load_options_blk->get_arr("grids_whitelist", grid_whitelist);
    load_options_blk->get_arr("point_arrays_whitelist", point_arrays_whitelist);
    load_options_blk->get_arr("cell_arrays_whitelist", cell_arrays_whitelist);
    prune_dataset(dataset, prune_grids, grid_whitelist, prune_point_arrays, point_arrays_whitelist, prune_cell_arrays, cell_arrays_whitelist);

    print_dataset_info(dataset);
  }
}
