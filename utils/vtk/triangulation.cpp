#include "parser.h"
#include "vtk_internal.h"
#include "utils/common/vector_comparators.h"
#include "utils/common/position_hash.h"
#include <cmath>
#include <algorithm>
#include <set>

#include <map>

namespace vtk
{
  using LiteMath::uint3;

  bool is_int(DataArray::Type type)
  {
    return type == DataArray::Type::Int8 ||
           type == DataArray::Type::UInt8 ||  
           type == DataArray::Type::Int16 ||
           type == DataArray::Type::UInt16 ||
           type == DataArray::Type::Int32 ||
           type == DataArray::Type::UInt32 ||
           type == DataArray::Type::Int64 ||
           type == DataArray::Type::UInt64;
  }

  uint3 get_polygon_code(const FullCell &fc, unsigned polygon_idx)
  {
    const id_t *p_indices = fc.indices + fc.polygons_offsets[polygon_idx];
    uint32_t sz = fc.polygons_offsets[polygon_idx+1] - fc.polygons_offsets[polygon_idx];
    uint32_t min_p = p_indices[0];
    uint32_t min_p_id = 0;

    for (uint32_t i=0; i<sz; i++)
    {
      if (p_indices[i] < min_p)
      {
        min_p = p_indices[i];
        min_p_id = i;
      }
    }

    uint3 code;
    code.x = min_p;
    if (p_indices[(min_p_id+sz-1)%sz] < p_indices[(min_p_id+1)%sz])
    {
      code.y = p_indices[(min_p_id+sz-1)%sz];
      code.z = p_indices[(min_p_id+sz-2)%sz];
    }
    else
    {
      code.y = p_indices[(min_p_id+1)%sz];
      code.z = p_indices[(min_p_id+2)%sz];
    }

    // printf("quad %u %u %u %u -- ", p_indices[0], p_indices[1], p_indices[2], p_indices[3]);
    // printf("code %u %u %u\n", code.x, code.y, code.z);

    return code;
  }

  void mark_border_cells(UnstructuredGrid &grid)
  {
    bool all_cell_2d = true;
    for (auto &cell : grid.p_cells)
    {
      if (is_2d_cell(cell.type))
      {
        cell.border_bits = 0xFFFF;
      }
      else
      {
        cell.border_bits = 0;
        all_cell_2d = false;
      }
    }

    if (all_cell_2d)
      return;
        
    struct PolygonBorderInfo
    {
      uint32_t cell_id;
      uint32_t polygon_id;
      uint3 code;
    };

    FullCell cur_fc;
    std::map<uint3, uint32_t, cmesh4::cmpUint3> PB_map;

    for (uint32_t i=0; i<grid.p_cells.size(); i++)
    {
      if (!is_3d_cell(grid.p_cells[i].type))
        continue;
      
      unpack_cell(grid.p_cells[i], grid.p_indices.data(), &cur_fc);

      for (uint32_t j=0; j<cur_fc.polygons_count; j++)
      {
        uint3 code = get_polygon_code(cur_fc, j);
        auto it = PB_map.find(code);
        if (it == PB_map.end())
          PB_map[code] = 1;
        else
          PB_map.erase(it);
      }
    }

    for (uint32_t i=0; i<grid.p_cells.size(); i++)
    {
      if (!is_3d_cell(grid.p_cells[i].type))
        continue;
      
      unpack_cell(grid.p_cells[i], grid.p_indices.data(), &cur_fc);

      for (uint32_t j=0; j<cur_fc.polygons_count; j++)
      {
        uint3 code = get_polygon_code(cur_fc, j);
        if (PB_map.find(code) != PB_map.end())
          grid.p_cells[i].border_bits |= (1 << j); 
      }
    }    
  }

  cmesh4::AugmentedMesh triangulate_grid_bounds(UnstructuredGrid &grid, bool compress_float_values)
  {
    assert(grid.points.size() > 0);
    assert(grid.p_cells.size() > 0);

    mark_border_cells(grid);

    cmesh4::AugmentedMesh mesh;

    //copy points and point data to mesh
    mesh.vertices = grid.points;
    mesh.vertex_channels.resize(grid.point_data_arrays.size());
    for (int i=0;i<grid.point_data_arrays.size();i++)
    {
      const auto &array = grid.point_data_arrays[i];
      mesh.vertex_channels[i].name = array.name;
      mesh.vertex_channels[i].num_components = array.num_components;

      //save all vertex channels as float
      mesh.vertex_channels[i].type = DataChannel::Type::FLOAT;
      convert_values<float>(array.values, mesh.vertex_channels[i].data_f);
      calculate_data_array_limits(array, &mesh.vertex_channels[i].min_val, &mesh.vertex_channels[i].max_val);

      if (compress_float_values)
        compress_float_channel(mesh.vertex_channels[i], DataChannel::Type::FP8);
    }

    std::vector<uint32_t> triangle_id_to_cell_id;
    mesh.indices.reserve(grid.p_cells.size() * 3 * 3); //ballpark estimation of mesh size

    //triangulate border cells
    FullCell cur_fc;
    for (int i=0;i<grid.p_cells.size();i++)
    {
      unpack_cell(grid.p_cells[i], grid.p_indices.data(), &cur_fc);
      for (uint32_t p_id = 0; p_id < cur_fc.polygons_count; p_id++)
      {
        if (grid.p_cells[i].border_bits & (1 << p_id))
        {
          //polygon triangulation
          //TODO: is it ok to use such naive approach?
          uint32_t idx_offset = cur_fc.polygons_offsets[p_id];
          uint32_t idx_count  = cur_fc.polygons_offsets[p_id+1] - cur_fc.polygons_offsets[p_id];
          for (int j=2;j<idx_count;j++)
          {
            mesh.indices.push_back(cur_fc.indices[idx_offset + 0]);
            mesh.indices.push_back(cur_fc.indices[idx_offset + j-1]);
            mesh.indices.push_back(cur_fc.indices[idx_offset + j]);
            triangle_id_to_cell_id.push_back(i);
          }
        }
      }
    }
    mesh.indices.shrink_to_fit();

    //copy cell data to mesh
    mesh.face_channels.resize(grid.cell_data_arrays.size());
    for (int i=0;i<grid.cell_data_arrays.size();i++)
    {
      const auto &array = grid.cell_data_arrays[i];
      mesh.face_channels[i].name = array.name;
      mesh.face_channels[i].num_components = array.num_components;

      int dim = array.num_components;
      if (is_int(array.values.type))
      {
        mesh.face_channels[i].type = DataChannel::Type::INT;
        mesh.face_channels[i].data_i.resize(dim*triangle_id_to_cell_id.size());

        std::vector<int> cell_values_i;
        convert_values<int>(array.values, cell_values_i);

        for (int j=0;j<triangle_id_to_cell_id.size();j++)
          for (int k=0;k<dim;k++)
            mesh.face_channels[i].data_i[j*dim+k] = cell_values_i[dim*triangle_id_to_cell_id[j]+k];
      
        calculate_data_array_limits(array, &mesh.face_channels[i].min_val, &mesh.face_channels[i].max_val);
      }
      else
      {
        mesh.face_channels[i].type = DataChannel::Type::FLOAT;
        mesh.face_channels[i].data_f.resize(dim*triangle_id_to_cell_id.size());

        std::vector<float> cell_values_f;
        convert_values<float>(array.values, cell_values_f);

        for (int j=0;j<triangle_id_to_cell_id.size();j++)
          for (int k=0;k<dim;k++)
            mesh.face_channels[i].data_f[j*dim+k] = cell_values_f[dim*triangle_id_to_cell_id[j]+k];
        
        calculate_data_array_limits(array, &mesh.face_channels[i].min_val, &mesh.face_channels[i].max_val);

        if (compress_float_values)
          compress_float_channel(mesh.face_channels[i], DataChannel::Type::FP8);
      }
    }

    cmesh4::recalculate_vertex_normals(mesh);
    return mesh;
  }

  struct LegacyCellData
  {
    std::vector<id_t>       indices;
    std::vector<Polygon>    polygons;
    std::vector<LegacyCell> cells;
  };

  void mark_plane_grid(UnstructuredGrid &grid, 
      const Plane& plane,
      std::vector<int>& marked_plane_cells, 
      std::vector<int>& marked_plane_points)
  {
    FullCell cur_fc;
    for (int cell_id = 0; cell_id < grid.cells_count; cell_id++)
    {
      uint32_t off = grid.p_cells[cell_id].indices_offset;
      uint32_t count = grid.p_cells[cell_id].indices_count;
      bool is_out = false, is_inside = false;

      for (uint32_t i_id = off; i_id < off + count; i_id++)
      {
        uint32_t idx = grid.p_indices[i_id];
        float p = dot(plane.normal, grid.points[idx] - plane.pos);

        if (p <= 0)
        {
          is_inside = true;
          marked_plane_points[idx] = 0;
        }
        else
        {
          is_out = true;
          marked_plane_points[idx] = 2;
        }
      }

      if (is_out && is_inside)
      {
        //  плоскость разделяет клетку
        marked_plane_cells[cell_id] = 1;
      }
      else if (is_out && !is_inside)
      {
        marked_plane_cells[cell_id] = 2;
      }
      else
      {
        marked_plane_cells[cell_id] = 0;
      }
    }
  }

  void add_attribute_cells_info(const UnstructuredGrid &grid, UnstructuredGrid &cutted_grid, std::map<int, int> &added_old_cells_indx)
  {
    cutted_grid.cell_data_arrays.resize(grid.cell_data_arrays.size());

    for (int i = 0; i < grid.cell_data_arrays.size(); i++)
    {
      auto& cur_data_arr = grid.cell_data_arrays[i];
      DataArray &new_data_arr = cutted_grid.cell_data_arrays[i];

      size_t size = added_old_cells_indx.size() * cur_data_arr.num_components;

      new_data_arr.name = cur_data_arr.name;
      new_data_arr.num_components = cur_data_arr.num_components;
      new_data_arr.num_elements = added_old_cells_indx.size();
      new_data_arr.values.count = size;
      new_data_arr.values.type = cur_data_arr.values.type;
      new_data_arr.component_names = cur_data_arr.component_names;

      new_data_arr.values.range_set = cur_data_arr.values.range_set;
      new_data_arr.values.range_min = std::numeric_limits<double>::max();
      new_data_arr.values.range_max = std::numeric_limits<double>::min();

      switch(cur_data_arr.values.type)
      {
        case vtk::DataArray::Type::UInt8:
        {
          new_data_arr.values.u8 = new uint8_t[size];

          for (int j = 0; j < new_data_arr.num_elements; j++)
          {
            uint32_t ind = added_old_cells_indx[j];

            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.u8[j * new_data_arr.num_components + k] = cur_data_arr.values.u8[ind * new_data_arr.num_components + k];

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.u8[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.u8[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::UInt16:
        {
          new_data_arr.values.u16 = new uint16_t[size];

          for (int j = 0; j < new_data_arr.num_elements; j++)
          {
            uint32_t ind = added_old_cells_indx[j];

            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.u16[j * new_data_arr.num_components + k] = cur_data_arr.values.u16[ind * new_data_arr.num_components + k];

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.u16[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.u16[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::UInt32:
        {
          new_data_arr.values.u32 = new uint32_t[size];

          for (int j = 0; j < new_data_arr.num_elements; j++)
          {
            uint32_t ind = added_old_cells_indx[j];

            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.u32[j * new_data_arr.num_components + k] = cur_data_arr.values.u32[ind * new_data_arr.num_components + k];

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.u32[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.u32[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::UInt64:
        {
          new_data_arr.values.u64 = new uint64_t[size];

          for (int j = 0; j < new_data_arr.num_elements; j++)
          {
            uint32_t ind = added_old_cells_indx[j];

            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.u64[j * new_data_arr.num_components + k] = cur_data_arr.values.u64[ind * new_data_arr.num_components + k];

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.u64[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.u64[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::Int8:
        {
          new_data_arr.values.i8 = new int8_t[size];

          for (int j = 0; j < new_data_arr.num_elements; j++)
          {
            uint32_t ind = added_old_cells_indx[j];

            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.i8[j * new_data_arr.num_components + k] = cur_data_arr.values.i8[ind * new_data_arr.num_components + k];

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.i8[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.i8[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::Int16:
        {
          new_data_arr.values.i16 = new int16_t[size];

          for (int j = 0; j < new_data_arr.num_elements; j++)
          {
            uint32_t ind = added_old_cells_indx[j];

            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.i16[j * new_data_arr.num_components + k] = cur_data_arr.values.i16[ind * new_data_arr.num_components + k];

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.i16[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.i16[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::Int32:
        {
          new_data_arr.values.i32 = new int[size];

          for (int j = 0; j < new_data_arr.num_elements; j++)
          {
            uint32_t ind = added_old_cells_indx[j];

            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.i32[j * new_data_arr.num_components + k] = cur_data_arr.values.i32[ind * new_data_arr.num_components + k];

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.i32[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.i32[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::Int64:
        {
          new_data_arr.values.i64 = new int64_t[size];

          for (int j = 0; j < new_data_arr.num_elements; j++)
          {
            uint32_t ind = added_old_cells_indx[j];

            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.i64[j * new_data_arr.num_components + k] = cur_data_arr.values.i64[ind * new_data_arr.num_components + k];

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.i64[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.i64[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::Float32:
        {
          new_data_arr.values.f32 = new float[size];

          for (int j = 0; j < new_data_arr.num_elements; j++)
          {
            uint32_t ind = added_old_cells_indx[j];

            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.f32[j * new_data_arr.num_components + k] = cur_data_arr.values.f32[ind * new_data_arr.num_components + k];

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.f32[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.f32[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::Float64:
        {
          new_data_arr.values.f64 = new double[size];

          for (int j = 0; j < new_data_arr.num_elements; j++)
          {
            uint32_t ind = added_old_cells_indx[j];

            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.f64[j * new_data_arr.num_components + k] = cur_data_arr.values.f64[ind * new_data_arr.num_components + k];

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.f64[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.f64[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        default:
          break;
      }

      new_data_arr.values.range_min -= 1e-6f;
      new_data_arr.values.range_max += 1e-6f;
    }
  }

  void add_attribute_points_info(const UnstructuredGrid &grid, UnstructuredGrid &cutted_grid, const std::map<int, float3> &data2interpolate)
  {
    cutted_grid.point_data_arrays.resize(grid.point_data_arrays.size());

    for (int i = 0; i < grid.point_data_arrays.size(); i++)
    {
      auto& cur_data_arr = grid.point_data_arrays[i];
      DataArray &new_data_arr = cutted_grid.point_data_arrays[i];

      size_t points_count = data2interpolate.size();
      size_t size = cutted_grid.points.size() * cur_data_arr.num_components;

      new_data_arr.name = cur_data_arr.name;
      new_data_arr.num_components = cur_data_arr.num_components;
      new_data_arr.num_elements = points_count;
      new_data_arr.values.count = size;
      new_data_arr.values.type = cur_data_arr.values.type;
      new_data_arr.component_names = cur_data_arr.component_names;

      new_data_arr.values.range_set = cur_data_arr.values.range_set;
      new_data_arr.values.range_min = std::numeric_limits<double>::max();
      new_data_arr.values.range_max = std::numeric_limits<double>::min();

      switch(cur_data_arr.values.type)
      {
        case vtk::DataArray::Type::UInt8:
        {
          new_data_arr.values.u8 = new uint8_t[size];

          for (int j = 0; j < data2interpolate.size(); j++)
          {
            float3 unpack = data2interpolate.at(j);
            uint32_t ind1 = (uint32_t)unpack.x;
            uint32_t ind2 = (uint32_t)unpack.y;
            float t = unpack.z;
            
            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.u8[j * new_data_arr.num_components + k] = cur_data_arr.values.u8[ind1 * cur_data_arr.num_components + k] + 
                    t * (float)(cur_data_arr.values.u8[ind2 * cur_data_arr.num_components + k] - cur_data_arr.values.u8[ind1 * cur_data_arr.num_components + k]);

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.u8[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.u8[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::UInt16:
        {
          new_data_arr.values.u16 = new uint16_t[size];

          for (int j = 0; j < data2interpolate.size(); j++)
          {
            float3 unpack = data2interpolate.at(j);
            uint32_t ind1 = (uint32_t)unpack.x;
            uint32_t ind2 = (uint32_t)unpack.y;
            float t = unpack.z;
            
            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.u16[j * new_data_arr.num_components + k] = cur_data_arr.values.u16[ind1 * cur_data_arr.num_components + k] + 
                    t * (float)(cur_data_arr.values.u16[ind2 * cur_data_arr.num_components + k] - cur_data_arr.values.u16[ind1 * cur_data_arr.num_components + k]);

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.u16[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.u16[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::UInt32:
        {
          new_data_arr.values.u32 = new uint32_t[size];

          for (int j = 0; j < data2interpolate.size(); j++)
          {
            float3 unpack = data2interpolate.at(j);
            uint32_t ind1 = (uint32_t)unpack.x;
            uint32_t ind2 = (uint32_t)unpack.y;
            float t = unpack.z;
            
            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.u32[j * new_data_arr.num_components + k] = cur_data_arr.values.u32[ind1 * cur_data_arr.num_components + k] + 
                    t * (float)(cur_data_arr.values.u32[ind2 * cur_data_arr.num_components + k] - cur_data_arr.values.u32[ind1 * cur_data_arr.num_components + k]);

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.u32[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.u32[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::UInt64:
        {
          new_data_arr.values.u64 = new uint64_t[size];

          for (int j = 0; j < data2interpolate.size(); j++)
          {
            float3 unpack = data2interpolate.at(j);
            uint32_t ind1 = (uint32_t)unpack.x;
            uint32_t ind2 = (uint32_t)unpack.y;
            float t = unpack.z;
            
            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.u64[j * new_data_arr.num_components + k] = cur_data_arr.values.u64[ind1 * cur_data_arr.num_components + k] + 
                    t * (float)(cur_data_arr.values.u64[ind2 * cur_data_arr.num_components + k] - cur_data_arr.values.u64[ind1 * cur_data_arr.num_components + k]);

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.u64[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.u64[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::Int8:
        {
          new_data_arr.values.i8 = new int8_t[size];

          for (int j = 0; j < data2interpolate.size(); j++)
          {
            float3 unpack = data2interpolate.at(j);
            uint32_t ind1 = (uint32_t)unpack.x;
            uint32_t ind2 = (uint32_t)unpack.y;
            float t = unpack.z;
            
            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.i8[j * new_data_arr.num_components + k] = cur_data_arr.values.i8[ind1 * cur_data_arr.num_components + k] + 
                    t * (float)(cur_data_arr.values.i8[ind2 * cur_data_arr.num_components + k] - cur_data_arr.values.i8[ind1 * cur_data_arr.num_components + k]);

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.i8[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.i8[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::Int16:
        {
          new_data_arr.values.i16 = new int16_t[size];

          for (int j = 0; j < data2interpolate.size(); j++)
          {
            float3 unpack = data2interpolate.at(j);
            uint32_t ind1 = (uint32_t)unpack.x;
            uint32_t ind2 = (uint32_t)unpack.y;
            float t = unpack.z;
            
            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.i16[j * new_data_arr.num_components + k] = cur_data_arr.values.i16[ind1 * cur_data_arr.num_components + k] + 
                    t * (float)(cur_data_arr.values.i16[ind2 * cur_data_arr.num_components + k] - cur_data_arr.values.i16[ind1 * cur_data_arr.num_components + k]);

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.i16[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.i16[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::Int32:
        {
          new_data_arr.values.i32 = new int[size];

          for (int j = 0; j < data2interpolate.size(); j++)
          {
            float3 unpack = data2interpolate.at(j);
            uint32_t ind1 = (uint32_t)unpack.x;
            uint32_t ind2 = (uint32_t)unpack.y;
            float t = unpack.z;
            
            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.i32[j * new_data_arr.num_components + k] = cur_data_arr.values.i32[ind1 * cur_data_arr.num_components + k] + 
                    t * (float)(cur_data_arr.values.i32[ind2 * cur_data_arr.num_components + k] - cur_data_arr.values.i32[ind1 * cur_data_arr.num_components + k]);

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.i32[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.i32[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::Int64:
        {
          new_data_arr.values.i64 = new int64_t[size];

          for (int j = 0; j < data2interpolate.size(); j++)
          {
            float3 unpack = data2interpolate.at(j);
            uint32_t ind1 = (uint32_t)unpack.x;
            uint32_t ind2 = (uint32_t)unpack.y;
            float t = unpack.z;
            
            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.i64[j * new_data_arr.num_components + k] = cur_data_arr.values.i64[ind1 * cur_data_arr.num_components + k] + 
                    t * (float)(cur_data_arr.values.i64[ind2 * cur_data_arr.num_components + k] - cur_data_arr.values.i64[ind1 * cur_data_arr.num_components + k]);

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.i64[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.i64[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::Float32:
        {
          new_data_arr.values.f32 = new float[size];

          for (int j = 0; j < data2interpolate.size(); j++)
          {
            float3 unpack = data2interpolate.at(j);
            uint32_t ind1 = (uint32_t)unpack.x;
            uint32_t ind2 = (uint32_t)unpack.y;
            float t = unpack.z;
            
            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.f32[j * new_data_arr.num_components + k] = cur_data_arr.values.f32[ind1 * cur_data_arr.num_components + k] + 
                    t * (cur_data_arr.values.f32[ind2 * cur_data_arr.num_components + k] - cur_data_arr.values.f32[ind1 * cur_data_arr.num_components + k]);

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.f32[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.f32[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        case vtk::DataArray::Type::Float64:
        {
          new_data_arr.values.f64 = new double[size];

          for (int j = 0; j < data2interpolate.size(); j++)
          {
            float3 unpack = data2interpolate.at(j);
            uint32_t ind1 = (uint32_t)unpack.x;
            uint32_t ind2 = (uint32_t)unpack.y;
            float t = unpack.z;
            
            for (int k = 0; k < new_data_arr.num_components; k++)
            {
              new_data_arr.values.f64[j * new_data_arr.num_components + k] = cur_data_arr.values.f64[ind1 * cur_data_arr.num_components + k] + 
                    t * (cur_data_arr.values.f64[ind2 * cur_data_arr.num_components + k] - cur_data_arr.values.f64[ind1 * cur_data_arr.num_components + k]);

              new_data_arr.values.range_min = std::min<double>(new_data_arr.values.range_min, new_data_arr.values.f64[j * new_data_arr.num_components + k]);
              new_data_arr.values.range_max = std::max<double>(new_data_arr.values.range_max, new_data_arr.values.f64[j * new_data_arr.num_components + k]);
            }
          }

          break;
        }
        default:
          break;
      }

      new_data_arr.values.range_min -= 1e-6f;
      new_data_arr.values.range_max += 1e-6f;
    }
  }

  void change_grid(const UnstructuredGrid &grid,
                   const LegacyCellData &cell_data,
                   const Plane &plane,
                   std::vector<int> &marked_plane_cells,
                   std::vector<int> &marked_plane_points,
                   UnstructuredGrid &cutted_grid,
                   LegacyCellData &cutted_cell_data)
  {
    size_t size = cell_data.cells.size();

    std::map<int, int> added_old_cells_indx;
    std::map<int, float3> point_data2interpolate;
    PositionMap<int> points_hash;
    int cells_count = 0, points_count = 0;

    auto add_point = [&](float3 point) -> int 
    {
      int point_idx = -1;
      if (points_hash.find(point) == points_hash.end())
      {
        points_hash[point] = cutted_grid.points.size();
        cutted_grid.points.push_back(point);
        point_idx = cutted_grid.points.size() - 1;
      }
      else
      {
        point_idx = points_hash[point];
      } 

      return point_idx;
    };

    for (int cell_id = 0; cell_id < size; cell_id++)
    { 
      LegacyCell new_cell;
      LegacyCell cur_cell = cell_data.cells[cell_id];
      new_cell.type = cur_cell.type;
      new_cell.polygons_offset = cutted_cell_data.polygons.size();
      new_cell.polygons_count = 0;

      if (marked_plane_cells[cell_id] == 0)
      {
        added_old_cells_indx[cells_count] = cell_id;
        cells_count++;
            
        for (uint32_t p_id = cur_cell.polygons_offset; p_id < cur_cell.polygons_offset + cur_cell.polygons_count; p_id++)
        {
          const Polygon &polygon = cell_data.polygons[p_id];
          
          Polygon new_polygon;
          new_polygon.is_1d = polygon.is_1d;
          new_polygon.indices_offset = cutted_cell_data.indices.size();
          new_polygon.indices_count = 0;

          for (uint32_t i_id = polygon.indices_offset; i_id < polygon.indices_offset + polygon.indices_count; i_id++)
          {
            uint32_t ind = cell_data.indices[i_id];
            cutted_cell_data.indices.push_back(add_point(grid.points[ind]));
            point_data2interpolate[cutted_cell_data.indices.back()] = {(float)ind, (float)ind, 0.5};
            new_polygon.indices_count++;
          }

          cutted_cell_data.polygons.push_back(new_polygon);
          new_cell.polygons_count++;
        }

        cutted_cell_data.cells.push_back(new_cell);
      }
      else if (marked_plane_cells[cell_id] == 1)
      {
        std::vector<float3> intersection_polygon;
        std::vector<float3> intersection_info;
        
        added_old_cells_indx[cells_count] = cell_id;
        cells_count++;

        for (uint32_t p_id = cur_cell.polygons_offset; p_id < cur_cell.polygons_offset + cur_cell.polygons_count; p_id++)
        {
          const Polygon &polygon = cell_data.polygons[p_id];
          
          Polygon new_polygon;
          new_polygon.is_1d = polygon.is_1d;
          new_polygon.indices_offset = cutted_cell_data.indices.size();
          new_polygon.indices_count = 0;

          for (uint32_t i_id = polygon.indices_offset; i_id < polygon.indices_offset + polygon.indices_count; i_id++)
          {
            uint32_t ind = cell_data.indices[i_id];

            if (marked_plane_points[ind] == 0)
            {
              cutted_cell_data.indices.push_back(add_point(grid.points[ind]));
              point_data2interpolate[cutted_cell_data.indices.back()] = {(float)ind, (float)ind, 0.5};

              new_polygon.indices_count++;

              int next_ind = cell_data.indices[polygon.indices_offset + (i_id - polygon.indices_offset + 1) % polygon.indices_count];

              if (marked_plane_points[next_ind] == 2)
              {
                float3 out_point = grid.points[next_ind];
                float3 in_point = grid.points[ind];

                float D = -(plane.normal.x * plane.pos.x + plane.normal.y * plane.pos.y + plane.normal.z * plane.pos.z);
                float t = -(plane.normal.x * in_point.x + plane.normal.y * in_point.y + plane.normal.z * in_point.z + D) / 
                  (plane.normal.x * (out_point.x - in_point.x) + plane.normal.y * (out_point.y - in_point.y) + plane.normal.z * (out_point.z - in_point.z));

                float3 intersection = in_point + t * (out_point - in_point);

                cutted_cell_data.indices.push_back(add_point(intersection));
                point_data2interpolate[cutted_cell_data.indices.back()] = {(float)ind, (float)next_ind, t};
                
                new_polygon.indices_count++;

                intersection_polygon.push_back(intersection);
              }
            }
            else
            {
              uint32_t next_ind = cell_data.indices[polygon.indices_offset + (i_id - polygon.indices_offset + 1) % polygon.indices_count];

              if (marked_plane_points[next_ind] == 0)
              {
                float3 out_point = grid.points[next_ind];
                float3 in_point = grid.points[ind];

                float D = -(plane.normal.x * plane.pos.x + plane.normal.y * plane.pos.y + plane.normal.z * plane.pos.z);
                float t = -(plane.normal.x * in_point.x + plane.normal.y * in_point.y + plane.normal.z * in_point.z + D) / 
                  (plane.normal.x * (out_point.x - in_point.x) + plane.normal.y * (out_point.y - in_point.y) + plane.normal.z * (out_point.z - in_point.z));

                float3 intersection = in_point + t * (out_point - in_point);

                cutted_cell_data.indices.push_back(add_point(intersection));
                point_data2interpolate[cutted_cell_data.indices.back()] = {(float)ind, (float)next_ind, t};

                new_polygon.indices_count++;

                intersection_polygon.push_back(intersection);
              }
            }
          }

          if (new_polygon.indices_count > 2)
          {
            cutted_cell_data.polygons.push_back(new_polygon);
            new_cell.polygons_count++;
          }
        }

        if (intersection_polygon.size() > 2)
        {
          Polygon tmp;
          tmp.indices_offset = cutted_cell_data.indices.size();
          tmp.indices_count = intersection_polygon.size();

          for (auto p : intersection_polygon)
          {
            cutted_cell_data.indices.push_back(add_point(p));
          }

          cutted_cell_data.polygons.push_back(tmp);
          new_cell.polygons_count++;
        }

        cutted_cell_data.cells.push_back(new_cell);
      }
    }

    add_attribute_cells_info(grid, cutted_grid, added_old_cells_indx);
    add_attribute_points_info(grid, cutted_grid, point_data2interpolate);

    cutted_grid.cells_count = cutted_cell_data.cells.size();
    cutted_grid.points_count = cutted_grid.points.size();
  }

  uint3 get_polygon_code_legacy(const Polygon &polygon, const LegacyCellData &cell_data)
  {
    const id_t *p_indices = cell_data.indices.data() + polygon.indices_offset;
    uint32_t sz = polygon.indices_count;
    uint32_t min_p = p_indices[0];
    uint32_t min_p_id = 0;

    for (uint32_t i=0; i<sz; i++)
    {
      if (p_indices[i] < min_p)
      {
        min_p = p_indices[i];
        min_p_id = i;
      }
    }

    uint3 code;
    code.x = min_p;
    if (p_indices[(min_p_id+sz-1)%sz] < p_indices[(min_p_id+1)%sz])
    {
      code.y = p_indices[(min_p_id+sz-1)%sz];
      code.z = p_indices[(min_p_id+sz-2)%sz];
    }
    else
    {
      code.y = p_indices[(min_p_id+1)%sz];
      code.z = p_indices[(min_p_id+2)%sz];
    }

    // printf("quad %u %u %u %u -- ", p_indices[0], p_indices[1], p_indices[2], p_indices[3]);
    // printf("code %u %u %u\n", code.x, code.y, code.z);

    return code;
  }

  void mark_border_cells_legacy(LegacyCellData &cell_data)
  {
    id_t total_polygon_count = 0;
    bool all_cell_2d = true;
    for (auto &cell : cell_data.cells)
    {
      total_polygon_count += cell.polygons_count;
      if (is_2d_cell(cell.type))
      {
        cell.is_border = true;
        for (uint32_t p_id = cell.polygons_offset; p_id < cell.polygons_offset + cell.polygons_count; p_id++)
          cell_data.polygons[p_id].is_border = true;
      }
      else
      {
        all_cell_2d = false;
      }
    }

    if (all_cell_2d)
      return;
        
    struct PolygonBorderInfo
    {
      uint32_t cell_id;
      uint32_t polygon_id;
      uint3 code;
    };

    std::vector<PolygonBorderInfo> PB_infos;
    std::map<uint3, uint32_t, cmesh4::cmpUint3> PB_map;

    PB_infos.reserve(total_polygon_count);
    for (uint32_t i=0; i<cell_data.cells.size(); i++)
    {
      if (!is_3d_cell(cell_data.cells[i].type))
        continue;
      for (uint32_t j=0; j<cell_data.cells[i].polygons_count; j++)
      {
        uint3 code = get_polygon_code_legacy(cell_data.polygons[cell_data.cells[i].polygons_offset + j], cell_data);
        PB_infos.push_back({i, j, code});
        PB_map[code]++;
      }
    }
    
    for (auto &PB_info : PB_infos)
      cell_data.polygons[cell_data.cells[PB_info.cell_id].polygons_offset + PB_info.polygon_id].is_border = PB_map[PB_info.code] == 1;
    
    for (LegacyCell &cell : cell_data.cells)
    {
      for (uint32_t p_id = cell.polygons_offset; p_id < cell.polygons_offset + cell.polygons_count; p_id++)
      {
        if (cell_data.polygons[p_id].is_border)
        {
          cell.is_border = true;
          break;
        }
      }
    } 
  }

  void fill_legacy_cell_arrays(UnstructuredGrid &grid, LegacyCellData &cell_data)
  {
    assert(grid.cells_count > 0);
    assert(grid.cells_count == grid.p_cells.size());
    assert(grid.p_indices.size() > 0);

    cell_data.cells.clear();
    cell_data.polygons.clear();
    cell_data.indices.clear();

    cell_data.cells.resize(grid.p_cells.size());
    cell_data.polygons.reserve(grid.p_cells.size() * 4);
    cell_data.indices.reserve(12*grid.p_cells.size());

    FullCell cur_fc;

    for (uint32_t i=0; i<grid.p_cells.size(); i++)
    {
      vtk::unpack_cell(grid.p_cells[i], grid.p_indices.data(), &cur_fc);

      cell_data.cells[i].type = cur_fc.type;
      cell_data.cells[i].is_border = grid.p_cells[i].border_bits > 0;
      cell_data.cells[i].polygons_offset = cell_data.polygons.size();
      cell_data.cells[i].polygons_count = cur_fc.polygons_count;

      uint32_t idx_offset = cell_data.indices.size();
      cell_data.indices.insert(cell_data.indices.end(), cur_fc.indices, cur_fc.indices + cur_fc.polygons_offsets[cur_fc.polygons_count]);
      for (uint32_t j=0; j<cur_fc.polygons_count; j++)
      {
        cell_data.polygons.emplace_back();
        cell_data.polygons.back().is_1d = false;
        cell_data.polygons.back().is_border = grid.p_cells[i].border_bits & (1 << j);
        cell_data.polygons.back().indices_offset = idx_offset + cur_fc.polygons_offsets[j];
        cell_data.polygons.back().indices_count = cur_fc.polygons_offsets[j+1] - cur_fc.polygons_offsets[j];
      }
    }
  }

  cmesh4::AugmentedMesh triangulate_grid_bounds_with_plane(UnstructuredGrid &grid, const Plane& plane, bool compress_float_values)
  {
    LegacyCellData cell_data;
    fill_legacy_cell_arrays(grid, cell_data);
    mark_border_cells_legacy(cell_data);

    assert(grid.points.size() > 0);
    assert(cell_data.cells.size() > 0);

    //  0 - плоскость дальше клетки
    //  1 - плоскость делит клетку на 2 части
    //  2 - плоскость перед клеткой
    std::vector<int> marked_plane_cells(grid.cells_count);
    std::vector<int> marked_plane_points(grid.points_count);

    mark_plane_grid(grid, plane, marked_plane_cells, marked_plane_points);
    
    UnstructuredGrid cutted_grid;
    LegacyCellData cutted_cell_data;

    change_grid(grid, cell_data, plane, marked_plane_cells, marked_plane_points, cutted_grid, cutted_cell_data);
    mark_border_cells_legacy(cutted_cell_data);

    // for (auto& cell: cell_data.cells)
    // {
    //   for (int p_id = cell.polygons_offset; p_id < cell.polygons_count + cell.polygons_offset; p_id++)
    //   {
    //     Polygon pol = cell_data.polygons[p_id];

    //     for (int i_id = pol.indices_offset; i_id < pol.indices_count + pol.indices_offset; i_id++)
    //     {
    //       printf("%d ", cell_data.indices[i_id]);
    //     }
    //     printf("\n");
    //   }
    // }

    // for (int i = 0; i < cutted_grid.cell_data_arrays.size(); i++)
    // {
    //   auto &da = cutted_grid.cell_data_arrays[i];
      
    //   printf("%s %llu %llu:\n", da.name.c_str(), da.num_components, da.num_elements);
      
    //   for (int j = 0; j < da.num_elements; j++)
    //   {
    //     for (int k = 0; k < da.num_components; k++)
    //     {
    //       printf("%f ", da.values.f32[j * da.num_components + k]);
    //     }

    //     printf("\n");
    //   }
    // }

    // printf("%d %d %d\n", cell_data.indices.size(), grid.points.size(), cell_data.polygons.size());

    cmesh4::AugmentedMesh mesh;

    //copy points and point data to mesh
    mesh.vertices = cutted_grid.points;
    mesh.vertex_channels.resize(cutted_grid.point_data_arrays.size());
    for (int i=0;i<cutted_grid.point_data_arrays.size();i++)
    {
      const auto &array = cutted_grid.point_data_arrays[i];
      mesh.vertex_channels[i].name = array.name;
      mesh.vertex_channels[i].num_components = array.num_components;

      //save all vertex channels as float
      mesh.vertex_channels[i].type = DataChannel::Type::FLOAT;
      convert_values<float>(array.values, mesh.vertex_channels[i].data_f);
      calculate_data_array_limits(array, &mesh.vertex_channels[i].min_val, &mesh.vertex_channels[i].max_val);

      if (compress_float_values)
        compress_float_channel(mesh.vertex_channels[i], DataChannel::Type::FP8);
    }

    std::vector<uint32_t> triangle_id_to_cell_id;
    mesh.indices.reserve(cutted_cell_data.cells.size() * 3 * 3); //ballpark estimation of mesh size

    //triangulate border cells
    for (int i=0;i<cutted_cell_data.cells.size();i++)
    {
      for (uint32_t p_id = cutted_cell_data.cells[i].polygons_offset; p_id < cutted_cell_data.cells[i].polygons_offset + cutted_cell_data.cells[i].polygons_count; p_id++)
      {
        if (cutted_cell_data.polygons[p_id].is_border)
        {
          //polygon triangulation
          //TODO: is it ok to use such naive approach?
          uint32_t idx_offset = cutted_cell_data.polygons[p_id].indices_offset;
          for (int j=2;j<cutted_cell_data.polygons[p_id].indices_count;j++)
          {
            mesh.indices.push_back(cutted_cell_data.indices[idx_offset + 0]);
            mesh.indices.push_back(cutted_cell_data.indices[idx_offset + j-1]);
            mesh.indices.push_back(cutted_cell_data.indices[idx_offset + j]);
            triangle_id_to_cell_id.push_back(i);
          }
        }
      }
    }
    mesh.indices.shrink_to_fit();

    //copy cell data to mesh
    mesh.face_channels.resize(cutted_grid.cell_data_arrays.size());
    for (int i=0;i<cutted_grid.cell_data_arrays.size();i++)
    {
      const auto &array = cutted_grid.cell_data_arrays[i];
      mesh.face_channels[i].name = array.name;
      mesh.face_channels[i].num_components = array.num_components;

      int dim = array.num_components;
      if (is_int(array.values.type))
      {
        mesh.face_channels[i].type = DataChannel::Type::INT;
        mesh.face_channels[i].data_i.resize(dim*triangle_id_to_cell_id.size());

        std::vector<int> cell_values_i;
        convert_values<int>(array.values, cell_values_i);

        for (int j=0;j<triangle_id_to_cell_id.size();j++)
          for (int k=0;k<dim;k++)
            mesh.face_channels[i].data_i[j*dim+k] = cell_values_i[dim*triangle_id_to_cell_id[j]+k];
      
        calculate_data_array_limits(array, &mesh.face_channels[i].min_val, &mesh.face_channels[i].max_val);
      }
      else
      {
        mesh.face_channels[i].type = DataChannel::Type::FLOAT;
        mesh.face_channels[i].data_f.resize(dim*triangle_id_to_cell_id.size());

        std::vector<float> cell_values_f;
        convert_values<float>(array.values, cell_values_f);

        for (int j=0;j<triangle_id_to_cell_id.size();j++)
          for (int k=0;k<dim;k++)
            mesh.face_channels[i].data_f[j*dim+k] = cell_values_f[dim*triangle_id_to_cell_id[j]+k];
        
        calculate_data_array_limits(array, &mesh.face_channels[i].min_val, &mesh.face_channels[i].max_val);

        if (compress_float_values)
          compress_float_channel(mesh.face_channels[i], DataChannel::Type::FP8);
      }
    }

    cmesh4::recalculate_vertex_normals(mesh);
    return mesh;
  }
}