#include "sparse_octree_density_grid.h"

namespace sdf_converter
{
  float sample(const uint3 &grid_size, const float *grid_data, const float3 &p01)
  {
    float3 fp = float3(grid_size - 1) * p01;
    uint3 ip = uint3(fp);
    uint3 ip0 = clamp(ip, uint3(0u), uint3(grid_size - 1));
    uint3 ip1 = clamp(ip + uint3(1u), uint3(0u), uint3(grid_size - 1));
    float3 dp = fp - float3(ip);
    
    return(1.0f-dp.z) * (1.0f-dp.y) * (1.0f-dp.x) * grid_data[ip0.x*grid_size.y*grid_size.z + ip0.y*grid_size.z + ip0.z] +
          (     dp.z) * (1.0f-dp.y) * (1.0f-dp.x) * grid_data[ip0.x*grid_size.y*grid_size.z + ip0.y*grid_size.z + ip1.z] +
          (1.0f-dp.z) * (     dp.y) * (1.0f-dp.x) * grid_data[ip0.x*grid_size.y*grid_size.z + ip1.y*grid_size.z + ip0.z] +
          (     dp.z) * (     dp.y) * (1.0f-dp.x) * grid_data[ip0.x*grid_size.y*grid_size.z + ip1.y*grid_size.z + ip1.z] +
          (1.0f-dp.z) * (1.0f-dp.y) * (     dp.x) * grid_data[ip1.x*grid_size.y*grid_size.z + ip0.y*grid_size.z + ip0.z] +
          (     dp.z) * (1.0f-dp.y) * (     dp.x) * grid_data[ip1.x*grid_size.y*grid_size.z + ip0.y*grid_size.z + ip1.z] +
          (1.0f-dp.z) * (     dp.y) * (     dp.x) * grid_data[ip1.x*grid_size.y*grid_size.z + ip1.y*grid_size.z + ip0.z] +
          (     dp.z) * (     dp.y) * (     dp.x) * grid_data[ip1.x*grid_size.y*grid_size.z + ip1.y*grid_size.z + ip1.z];
  }

  void density_grid_to_global_octree_rec(SparseOctreeSettings settings, const uint3 &grid_size, const float *grid_data, 
                                         GlobalOctree &octree, GlobalOctreeBuildStat *build_stat,
                                         float *vals_tmp, uint32_t nodeId, uint32_t level, uint4 code)
  {
    const int bsz = octree.header.brick_size;
    const int v_size = octree.header.brick_size + 2 * octree.header.brick_pad + 1;
    float min_val = 1e6f;
    float max_val = -1e6f;
    for (int i = -octree.header.brick_pad; i <= bsz + octree.header.brick_pad; i++)
    {
      for (int j = -octree.header.brick_pad; j <= bsz + octree.header.brick_pad; j++)
      {
        for (int k = -octree.header.brick_pad; k <= bsz + octree.header.brick_pad; k++)
        {
          float3 p = float3(code.x*bsz + i, code.y*bsz + j, code.z*bsz + k) / float(code.w*bsz);
          float val = sample(grid_size, grid_data, p);
          // adding negative values here, this way we can render this grid as a voxel octree
          // structure by MultiRenderer for debug purposes
          vals_tmp[i*v_size*v_size + j*v_size + k] = -val;
          min_val = min(min_val, val);
          max_val = max(max_val, val);
        }
      }
    }

    const bool has_useful_values = std::max(std::abs(min_val), std::abs(max_val)) > settings.split_thr;
    const bool has_values_grad = max_val - min_val > settings.split_thr;
    const bool should_split = (has_values_grad && level < settings.depth) || level < settings.min_depth;
    const bool should_fill_values = has_useful_values && !should_split;
    
    if (should_fill_values)
    {
      octree.nodes[nodeId].is_outside = false;
      octree.nodes[nodeId].is_surfaced = false;
      octree.nodes[nodeId].offset = 0;
      octree.nodes[nodeId].bricks_count = 1;
      octree.nodes[nodeId].type = GlobalOctreeNodeType::LEAF;
      octree.nodes[nodeId].val_off = octree.values_f.size();
      octree.values_f.insert(octree.values_f.end(), vals_tmp, vals_tmp + v_size*v_size*v_size);
    }
    else
    {
      octree.nodes[nodeId].is_outside = false;
      octree.nodes[nodeId].is_surfaced = false;
      octree.nodes[nodeId].offset = 0;
      octree.nodes[nodeId].bricks_count = 0;
      octree.nodes[nodeId].type = GlobalOctreeNodeType::EMPTY;
      octree.nodes[nodeId].val_off = 0;
    }

    if (should_split)
    {
      octree.nodes[nodeId].type = GlobalOctreeNodeType::EMPTY_NODE;
      octree.nodes[nodeId].offset = octree.nodes.size();
      octree.nodes.resize(octree.nodes.size() + 8);
      for (int i = 0; i < 8; i++)
      {
        uint4 ch_code = 2*code + uint4((i & 4) >> 2, (i & 2) >> 1, i & 1, 0);
        density_grid_to_global_octree_rec(settings, grid_size, grid_data, octree, build_stat, vals_tmp, 
                                          octree.nodes[nodeId].offset + i, level + 1, ch_code);
      }
    }
  }

  void density_grid_to_global_octree(SparseOctreeSettings settings, const DensityGrid &grid, 
                                     GlobalOctree &octree, GlobalOctreeBuildStat *build_stat)
  {
    const int v_size = octree.header.brick_size + 2 * octree.header.brick_pad + 1;
    std::vector<float> tmp_vec(v_size*v_size*v_size, 0.0f);

    assert(settings.brick_pad == 0);
    octree.header.brick_pad = 0;
    octree.header.brick_size = settings.brick_size;
    octree.header.dim = 3;
    octree.nodes.resize(1);
    density_grid_to_global_octree_rec(settings, grid.size, grid.data.data(), octree, build_stat, 
                                      tmp_vec.data(), 0, 0, uint4(0, 0, 0, 1));

    printf("created global octree with %llu nodes and %llu values\n", (unsigned long long)octree.nodes.size(), (unsigned long long)octree.values_f.size());
  }  

  void density_grid_slice_to_global_octree(SparseOctreeSettings settings, const DensityMultiGrid &grid, uint32_t time_step,
                                           GlobalOctree &octree, GlobalOctreeBuildStat *build_stat)
  {
    const int v_size = octree.header.brick_size + 2 * octree.header.brick_pad + 1;
    std::vector<float> tmp_vec(v_size*v_size*v_size, 0.0f);

    assert(settings.brick_pad == 0);
    octree.header.brick_pad = 0;
    octree.header.brick_size = settings.brick_size;
    octree.header.dim = 3;
    octree.nodes.resize(1);
    const uint64_t grid_data_off = (uint64_t)time_step * (grid.size.x * grid.size.y * grid.size.z);
    density_grid_to_global_octree_rec(settings, grid.size, grid.data.data() + grid_data_off, octree, build_stat, 
                                      tmp_vec.data(), 0, 0, uint4(0, 0, 0, 1));

    printf("created global octree with %llu nodes and %llu values\n", (unsigned long long)octree.nodes.size(), (unsigned long long)octree.values_f.size());    
  }

  float sample4(const uint4 &gs, const float *grid_data, const float4 &p01)
  {
    float4 fp = float4(gs - 1) * p01;
    uint4 ip = uint4(fp);
    uint4 ip0 = clamp(ip, uint4(0u), uint4(gs - 1));
    uint4 ip1 = clamp(ip + uint4(1u), uint4(0u), uint4(gs - 1));
    float4 dp = fp - float4(ip);
    
    return(1.0f-dp.z) * (1.0f-dp.y) * (1.0f-dp.x) * (1.0f-dp.w) * grid_data[ip0.w*gs.x*gs.y*gs.z + ip0.x*gs.y*gs.z + ip0.y*gs.z + ip0.z] +
          (     dp.z) * (1.0f-dp.y) * (1.0f-dp.x) * (1.0f-dp.w) * grid_data[ip0.w*gs.x*gs.y*gs.z + ip0.x*gs.y*gs.z + ip0.y*gs.z + ip1.z] +
          (1.0f-dp.z) * (     dp.y) * (1.0f-dp.x) * (1.0f-dp.w) * grid_data[ip0.w*gs.x*gs.y*gs.z + ip0.x*gs.y*gs.z + ip1.y*gs.z + ip0.z] +
          (     dp.z) * (     dp.y) * (1.0f-dp.x) * (1.0f-dp.w) * grid_data[ip0.w*gs.x*gs.y*gs.z + ip0.x*gs.y*gs.z + ip1.y*gs.z + ip1.z] +
          (1.0f-dp.z) * (1.0f-dp.y) * (     dp.x) * (1.0f-dp.w) * grid_data[ip0.w*gs.x*gs.y*gs.z + ip1.x*gs.y*gs.z + ip0.y*gs.z + ip0.z] +
          (     dp.z) * (1.0f-dp.y) * (     dp.x) * (1.0f-dp.w) * grid_data[ip0.w*gs.x*gs.y*gs.z + ip1.x*gs.y*gs.z + ip0.y*gs.z + ip1.z] +
          (1.0f-dp.z) * (     dp.y) * (     dp.x) * (1.0f-dp.w) * grid_data[ip0.w*gs.x*gs.y*gs.z + ip1.x*gs.y*gs.z + ip1.y*gs.z + ip0.z] +
          (     dp.z) * (     dp.y) * (     dp.x) * (1.0f-dp.w) * grid_data[ip0.w*gs.x*gs.y*gs.z + ip1.x*gs.y*gs.z + ip1.y*gs.z + ip1.z] +
          (1.0f-dp.z) * (1.0f-dp.y) * (1.0f-dp.x) * (     dp.w) * grid_data[ip1.w*gs.x*gs.y*gs.z + ip0.x*gs.y*gs.z + ip0.y*gs.z + ip0.z] +
          (     dp.z) * (1.0f-dp.y) * (1.0f-dp.x) * (     dp.w) * grid_data[ip1.w*gs.x*gs.y*gs.z + ip0.x*gs.y*gs.z + ip0.y*gs.z + ip1.z] +
          (1.0f-dp.z) * (     dp.y) * (1.0f-dp.x) * (     dp.w) * grid_data[ip1.w*gs.x*gs.y*gs.z + ip0.x*gs.y*gs.z + ip1.y*gs.z + ip0.z] +
          (     dp.z) * (     dp.y) * (1.0f-dp.x) * (     dp.w) * grid_data[ip1.w*gs.x*gs.y*gs.z + ip0.x*gs.y*gs.z + ip1.y*gs.z + ip1.z] +
          (1.0f-dp.z) * (1.0f-dp.y) * (     dp.x) * (     dp.w) * grid_data[ip1.w*gs.x*gs.y*gs.z + ip1.x*gs.y*gs.z + ip0.y*gs.z + ip0.z] +
          (     dp.z) * (1.0f-dp.y) * (     dp.x) * (     dp.w) * grid_data[ip1.w*gs.x*gs.y*gs.z + ip1.x*gs.y*gs.z + ip0.y*gs.z + ip1.z] +
          (1.0f-dp.z) * (     dp.y) * (     dp.x) * (     dp.w) * grid_data[ip1.w*gs.x*gs.y*gs.z + ip1.x*gs.y*gs.z + ip1.y*gs.z + ip0.z] +
          (     dp.z) * (     dp.y) * (     dp.x) * (     dp.w) * grid_data[ip1.w*gs.x*gs.y*gs.z + ip1.x*gs.y*gs.z + ip1.y*gs.z + ip1.z];
  }

  void density_grid_to_global_octree_4D_rec(SparseOctreeSettings settings, uint4 grid_size, const float *grid_data, 
                                            GlobalOctree &octree, GlobalOctreeBuildStat *build_stat,
                                            float *vals_tmp, uint32_t nodeId, uint32_t level, uint4 pos_code)
  {
    const int bsz = octree.header.brick_size;
    const int v_size = octree.header.brick_size + 2 * octree.header.brick_pad + 1;
    const int v_count = v_size*v_size*v_size*v_size;
    float min_val = 1e6f;
    float max_val = -1e6f;
    for (int i = -octree.header.brick_pad; i <= bsz + octree.header.brick_pad; i++)
    {
      for (int j = -octree.header.brick_pad; j <= bsz + octree.header.brick_pad; j++)
      {
        for (int k = -octree.header.brick_pad; k <= bsz + octree.header.brick_pad; k++)
        {
          for (int l = -octree.header.brick_pad; l <= bsz + octree.header.brick_pad; l++)
          {
            float sz = bsz*(1 << level);
            float4 p = float4(pos_code.x*bsz + i, pos_code.y*bsz + j, pos_code.z*bsz + k, pos_code.w*bsz + l) / sz;
            float val = sample4(grid_size, grid_data, p);
            // adding negative values here, this way we can render this grid as a voxel octree
            // structure by MultiRenderer for debug purposes
            vals_tmp[l*v_size*v_size*v_size + i*v_size*v_size + j*v_size + k] = -val;
            min_val = min(min_val, val);
            max_val = max(max_val, val);
          }
        }
      }
    }

    const bool has_useful_values = std::max(std::abs(min_val), std::abs(max_val)) > settings.split_thr;
    const bool has_values_grad = max_val - min_val > settings.split_thr;
    const bool should_split = (has_values_grad && level < settings.depth) || level <= settings.min_depth;
    const bool should_fill_values = has_useful_values && !should_split;
    
    if (should_fill_values)
    {
      octree.nodes[nodeId].is_outside = false;
      octree.nodes[nodeId].is_surfaced = false;
      octree.nodes[nodeId].offset = 0;
      octree.nodes[nodeId].bricks_count = 1;
      octree.nodes[nodeId].type = GlobalOctreeNodeType::LEAF;
      octree.nodes[nodeId].val_off = octree.values_f.size();
      octree.values_f.insert(octree.values_f.end(), vals_tmp, vals_tmp + v_count);
    }
    else
    {
      octree.nodes[nodeId].is_outside = false;
      octree.nodes[nodeId].is_surfaced = false;
      octree.nodes[nodeId].offset = 0;
      octree.nodes[nodeId].bricks_count = 0;
      octree.nodes[nodeId].type = GlobalOctreeNodeType::EMPTY;
      octree.nodes[nodeId].val_off = 0;
    }

    if (should_split)
    {
      octree.nodes[nodeId].type = GlobalOctreeNodeType::EMPTY_NODE;
      octree.nodes[nodeId].offset = octree.nodes.size();
      octree.nodes.resize(octree.nodes.size() + 16);
      for (int i = 0; i < 16; i++)
      {
        uint4 ch_code = 2*pos_code + uint4((i & 4) >> 2, (i & 2) >> 1, i & 1, (i & 8) >> 3);
        density_grid_to_global_octree_4D_rec(settings, grid_size, grid_data, octree, build_stat, vals_tmp, 
                                             octree.nodes[nodeId].offset + i, level + 1, ch_code);
      }
    }
  }

  void density_grid_to_global_octree(SparseOctreeSettings settings, const DensityMultiGrid &grid,
                                     GlobalOctree &octree, GlobalOctreeBuildStat *build_stat)
  {
    assert(settings.brick_pad == 0);
    octree.header.brick_pad = 0;
    octree.header.brick_size = settings.brick_size;
    octree.header.dim = 4;

    const uint32_t v_size = octree.header.brick_size + 2 * octree.header.brick_pad + 1;
    std::vector<float> tmp_vec(v_size*v_size*v_size*v_size, 0.0f);
    octree.nodes.resize(1);
    density_grid_to_global_octree_4D_rec(settings, uint4(grid.size.x, grid.size.y, grid.size.z, grid.frames), grid.data.data(),
                                         octree, nullptr, tmp_vec.data(), 0, 0, uint4(0,0,0,0));
    printf("created global 2^4-tree with %llu nodes and %llu values\n", (long long unsigned)octree.nodes.size(), (long long unsigned)octree.values_f.size()); 
  }
}