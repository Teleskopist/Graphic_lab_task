#include "procedural.h"

namespace vtk
{
  void create_dense_grid(const Block *grid_settings, UnstructuredGrid &grid)
  {
    int3 grid_size = grid_settings->get_ivec3("grid_size", int3(16,16,16));
    bool fill_point_channel = grid_settings->get_bool("fill_point_channel", true);
    bool fill_cell_channel = grid_settings->get_bool("fill_cell_channel", false);
    std::string function_point = grid_settings->get_string("function_point", "dist_from_center");

    auto get_p_idx = [&](uint3 idx) { return idx.x + idx.y*(grid_size.x+1) + idx.z*(grid_size.x+1)*(grid_size.y+1); };
    grid.points_count = (grid_size.x+1)*(grid_size.y+1)*(grid_size.z+1);
    grid.points.resize(grid.points_count);

    if (fill_point_channel)
    {
      grid.point_data_arrays.resize(1);
      grid.point_data_arrays[0].name = function_point;
      grid.point_data_arrays[0].num_components = 1;
      grid.point_data_arrays[0].num_elements = grid.points_count;
      grid.point_data_arrays[0].values.type = vtk::DataArray::Type::Float64;
      grid.point_data_arrays[0].values.count = grid.points_count;
      grid.point_data_arrays[0].values.f64 = new double[grid.points_count];
    }

    for (int i=0; i<grid.points_count; i++)
    {
      uint3 idx = uint3(i % (grid_size.x+1), (i / (grid_size.x+1)) % (grid_size.y+1), i / ((grid_size.x+1)*(grid_size.y+1)));
      float3 p_01 = float3(idx.x, idx.y, idx.z)/float3(grid_size.x, grid_size.y, grid_size.z);
      float3 pos = 0.9f*(2.0f*p_01 - 1.0f);
      grid.points[i] = pos;

      float val = 0.0f;
      if (function_point == "dist_from_center")
      {
        val = 0.5f*length(pos);
      }
      else if (function_point == "variable_frequency_waves")
      {
        float frequency = grid_settings->get_double("frequency", 10.0f);
        float d = 1.0f + pos.y;
        val = sinf(frequency*d*d);
      }
      else if (function_point == "variable_frequency_stripes")
      {
        float frequency = grid_settings->get_double("frequency", 10.0f);
        float d = 1.0f + pos.y;
        val = LiteMath::sign(sinf(frequency*d*d));
      }

      if (fill_point_channel)
        grid.point_data_arrays[0].values.f64[i] = val;
    }

    grid.cells_count = grid_size.x*grid_size.y*grid_size.z;

    if (fill_cell_channel)
    {
      grid.cell_data_arrays.resize(1);
      grid.cell_data_arrays[0].name = "column_index";
      grid.cell_data_arrays[0].num_components = 1;
      grid.cell_data_arrays[0].num_elements = grid.cells_count;
      grid.cell_data_arrays[0].values.type = vtk::DataArray::Type::Int32;
      grid.cell_data_arrays[0].values.count = grid.cells_count;
      grid.cell_data_arrays[0].values.i32 = new int32_t[grid.cells_count];
    }

    grid.p_cells.resize(grid.cells_count);
    grid.p_indices.resize(8*grid.cells_count);

    for (int i=0; i<grid.cells_count; i++)
    {
      uint3 idx = uint3(i % grid_size.x, (i / grid_size.x) % grid_size.y, i / (grid_size.x*grid_size.y));
      uint32_t indices[8] ={get_p_idx(idx + uint3(0,0,0)), 
                            get_p_idx(idx + uint3(1,0,0)), 
                            get_p_idx(idx + uint3(1,1,0)), 
                            get_p_idx(idx + uint3(0,1,0)),
                            get_p_idx(idx + uint3(0,0,1)), 
                            get_p_idx(idx + uint3(1,0,1)), 
                            get_p_idx(idx + uint3(1,1,1)), 
                            get_p_idx(idx + uint3(0,1,1))};

      for (int j=0; j<8; j++)
        grid.p_indices[8*i + j] = indices[j];

      grid.p_cells[i].type = vtk::VTK_HEXAHEDRON;
      grid.p_cells[i].indices_offset = 8*i;
      grid.p_cells[i].indices_count = 8;      

      if (fill_cell_channel)
        grid.cell_data_arrays[0].values.i32[i] = idx.x/4 + idx.z/4;
    }
  }
  
  void create_waves_grid(const Block *grid_settings, UnstructuredGrid &grid)
  {
    int3 grid_size = grid_settings->get_ivec3("grid_size", int3(16,16,16));
    int frequencies = grid_settings->get_int("frequencies", 2);

    auto get_p_idx = [&](uint3 idx) { return idx.x + idx.y*(grid_size.x+1) + idx.z*(grid_size.x+1)*(grid_size.y+1); };
    grid.points_count = (grid_size.x+1)*(grid_size.y+1)*(grid_size.z+1);
    grid.points.resize(grid.points_count);

    for (int i=0; i<grid.points_count; i++)
    {
      uint3 idx = uint3(i % (grid_size.x+1), (i / (grid_size.x+1)) % (grid_size.y+1), i / ((grid_size.x+1)*(grid_size.y+1)));
      float3 p_01 = float3(idx.x, idx.y, idx.z)/float3(grid_size.x, grid_size.y, grid_size.z);
      float3 pos = 0.9f*(2.0f*p_01 - 1.0f);
      grid.points[i] = pos;
    }

    grid.point_data_arrays.resize(frequencies*frequencies*frequencies);
    int n = 0;
    for (int x=1; x<=frequencies; x++)
    {
      for (int y=1; y<=frequencies; y++)
      {
        for (int z=1; z<=frequencies; z++)
        {
          grid.point_data_arrays[n].name = "freq_"+std::to_string(x)+"_"+std::to_string(y)+"_"+std::to_string(z);
          grid.point_data_arrays[n].num_components = 1;
          grid.point_data_arrays[n].num_elements = grid.points_count;
          grid.point_data_arrays[n].values.type = vtk::DataArray::Type::Float32;
          grid.point_data_arrays[n].values.count = grid.points_count;
          grid.point_data_arrays[n].values.f32 = new float[grid.points_count];

          for (int i=0; i<grid.points_count; i++)
          {
            float3 p = grid.points[i];
            grid.point_data_arrays[n].values.f32[i] = cos(LiteMath::M_PI*x*p.x)*cos(LiteMath::M_PI*y*p.y)*cos(LiteMath::M_PI*z*p.z);
          }

          n++;
        }
      }
    }

    grid.cells_count = grid_size.x*grid_size.y*grid_size.z;

    grid.p_cells.resize(grid.cells_count);
    grid.p_indices.resize(8*grid.cells_count);

    for (int i=0; i<grid.cells_count; i++)
    {
      uint3 idx = uint3(i % grid_size.x, (i / grid_size.x) % grid_size.y, i / (grid_size.x*grid_size.y));
      uint32_t indices[8] ={get_p_idx(idx + uint3(0,0,0)), 
                            get_p_idx(idx + uint3(1,0,0)), 
                            get_p_idx(idx + uint3(1,1,0)), 
                            get_p_idx(idx + uint3(0,1,0)),
                            get_p_idx(idx + uint3(0,0,1)), 
                            get_p_idx(idx + uint3(1,0,1)), 
                            get_p_idx(idx + uint3(1,1,1)), 
                            get_p_idx(idx + uint3(0,1,1))};

      for (int j=0; j<8; j++)
        grid.p_indices[8*i + j] = indices[j];

      grid.p_cells[i].type = vtk::VTK_HEXAHEDRON;
      grid.p_cells[i].indices_offset = 8*i;
      grid.p_cells[i].indices_count = 8;      
    }
  }

  void create_procedural_dataset(const Block &settings, Dataset &dataset)
  {
    for (int i=0; i<settings.size(); i++)
    {
      Block *b = settings.get_block(i);
      if (!b)
        continue;
      if (settings.get_name(i) == "unstructured_grid")
      {
        dataset.unstructured_grids.emplace_back();
        std::string model_name = b->get_string("model", "unknown");

        if (model_name == "dense_grid")
          create_dense_grid(b, dataset.unstructured_grids.back());
        else if (model_name == "waves_grid")
          create_waves_grid(b, dataset.unstructured_grids.back());
        else
          printf("[create_procedural_dataset] Unknown procedural model name \"%s\"\n", model_name.c_str());
      }
      else
      {
        printf("[create_procedural_dataset] Unknown procedural dataset type name \"%s\"\n", settings.get_name(i).c_str());
      }
    }
  }

  void create_procedural_dataset(const char *blk_file, Dataset &dataset)
  {
    Block blk;
    load_block_from_file(blk_file, blk);
    create_procedural_dataset(blk, dataset);
  }
}