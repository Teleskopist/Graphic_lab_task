#include "sdf_multi.h"
#include "utils/common/array_map.h"
#include "sdf/build/sparse_octree_builder.h"
#include <cassert>
#include <fstream>
#include <filesystem>

void save_sdf_multi_octree(const SdfMultiOctreeView &scene, const std::string &path)
{
  std::ofstream fs(path, std::ios::binary);
  
  fs.write((const char *)&scene.nodes_size, sizeof(unsigned));
  fs.write((const char *)&scene.values_size, sizeof(unsigned));
  fs.write((const char *)scene.nodes, scene.nodes_size * sizeof(SdfMultiOctreeNode));
  fs.write((const char *)scene.values, scene.values_size * sizeof(float));
  
  fs.flush();
  fs.close();
}

void load_sdf_multi_octree(SdfMultiOctree &scene, const std::string &path)
{
  std::ifstream fs(path, std::ios::binary);
  unsigned nodes_size = 0;
  unsigned values_size = 0; 
  fs.read((char *)&nodes_size, sizeof(unsigned));
  fs.read((char *)&values_size, sizeof(unsigned));

  scene.nodes.resize(nodes_size);
  scene.values.resize(values_size);

  fs.read((char *)scene.nodes.data(), nodes_size * sizeof(SdfMultiOctreeNode));
  fs.read((char *)scene.values.data(), values_size * sizeof(float));
  fs.close();
}

ModelInfo get_info_sdf_multi_octree(const SdfMultiOctreeView &scene)
{
  ModelInfo info;

  info.name = "sdf_multi_octree";
  info.bytesize = scene.nodes_size * sizeof(SdfMultiOctreeNode) + scene.values_size * sizeof(float) + sizeof(unsigned) * 2;
  info.num_primitives = scene.nodes_size;

  return info;
}

void save_sdf_augmented_MO(const SdfMultiOctreeAugmented &scene, const std::string &path)
{
  std::ofstream fs(path, std::ios::binary);
  unsigned nodes_size = scene.octree.nodes.size();
  unsigned values_size = scene.octree.values.size();
  unsigned vc_count = scene.point_channels.size();
  unsigned fc_count = scene.voxel_channels.size();
  fs.write((const char *)&nodes_size, sizeof(unsigned));
  fs.write((const char *)&values_size, sizeof(unsigned));
  fs.write((const char *)&vc_count, sizeof(unsigned));
  fs.write((const char *)&fc_count, sizeof(unsigned));

  fs.write((const char *)scene.octree.nodes.data(), scene.octree.nodes.size() * sizeof(SdfMultiOctreeNode));
  fs.write((const char *)scene.octree.values.data(), scene.octree.values.size() * sizeof(float));

  for (auto &ch : scene.point_channels)
    save_data_channel(fs, ch);

  for (auto &ch : scene.voxel_channels)
    save_data_channel(fs, ch);
  
  fs.flush();
  fs.close();
}

void load_sdf_augmented_MO(SdfMultiOctreeAugmented &scene, const std::string &path)
{
  std::ifstream fs(path, std::ios::binary);
  unsigned nodes_size = 0;
  unsigned values_size = 0; 
  unsigned vc_count = 0;
  unsigned fc_count = 0;
  fs.read((char *)&nodes_size, sizeof(unsigned));
  fs.read((char *)&values_size, sizeof(unsigned));
  fs.read((char *)&vc_count, sizeof(unsigned));
  fs.read((char *)&fc_count, sizeof(unsigned));

  scene.octree.nodes.resize(nodes_size);
  scene.octree.values.resize(values_size);

  scene.point_channels.resize(vc_count);
  scene.voxel_channels.resize(fc_count);

  fs.read((char *)scene.octree.nodes.data(), nodes_size * sizeof(SdfMultiOctreeNode));
  fs.read((char *)scene.octree.values.data(), values_size * sizeof(float));

  for (auto &ch : scene.point_channels)
    load_data_channel(fs, ch);

  for (auto &ch : scene.voxel_channels)
    load_data_channel(fs, ch);

  fs.close();
}

ModelInfo get_info_sdf_augmented_MO(const SdfMultiOctreeAugmented &scene)
{
  size_t octree_size = scene.octree.nodes.size() * sizeof(SdfMultiOctreeNode) + scene.octree.values.size() * sizeof(float) + sizeof(unsigned) * 4;
  size_t augment_size = 0;
  
  for (auto &ch : scene.point_channels)
    augment_size += calculate_bytesize(ch);

  for (auto &ch : scene.voxel_channels)
    augment_size += calculate_bytesize(ch);

  ModelInfo info;
  info.name = "sdf_augmented_multi_octree";
  info.bytesize = octree_size + augment_size;
  info.num_primitives = scene.octree.nodes.size();

  return info;
}

namespace sdf_converter
{
  void global_octree_to_multi_octree(const GlobalOctree &octree, SdfMultiOctree &multi_octree)
  {
    assert(octree.header.brick_size == 1);
    assert(octree.header.brick_pad == 0);

    multi_octree.nodes.resize(octree.nodes.size());
    multi_octree.values.resize(octree.values_f.size());

    std::unordered_map<unsigned, unsigned> data;
    
    //we should remove emty nodes, but for now just add them with voxel_count = 0
    //so they will be present in structure, but skipped during rendering
    for (int i = 0; i < octree.nodes.size(); ++i)
    {
      multi_octree.nodes[i].children_offset = octree.nodes[i].offset;
      multi_octree.nodes[i].data_offset = octree.nodes[i].val_off;
      multi_octree.nodes[i].voxel_count = octree.nodes[i].bricks_count;
      multi_octree.nodes[i].flags = octree.nodes[i].is_surfaced * MULTI_OCTREE_FLAG_SURFACE;
      
      if (data.find(octree.nodes[i].bricks_count) == data.end())
        data[octree.nodes[i].bricks_count] = 0;
      data[octree.nodes[i].bricks_count]++;
    }

    unsigned bc_min[6] = {0,0,0,0,0,0};
    unsigned bc_10   = 0;
    unsigned bc_25   = 0;
    unsigned bc_more = 0;
    unsigned bc_max  = 0;

    for (auto i : data)
    {
      bc_max = std::max(bc_max, i.first);
      if (i.first <= 5)
        bc_min[i.first] += i.second;
      else if (i.first <= 10)
        bc_10 += i.second;
      else if (i.first <= 25)
        bc_25 += i.second;
      else
        bc_more += i.second;
    }

    printf("number of voxels with different surface count\n");
    printf("0 surfaces:     %d\n", bc_min[0]);
    printf("1 surface:      %d\n", bc_min[1]);
    printf("2 surfaces:     %d\n", bc_min[2]);
    printf("3 surfaces:     %d\n", bc_min[3]);
    printf("4 surfaces:     %d\n", bc_min[4]);
    printf("5 surfaces:     %d\n", bc_min[5]);
    printf("6-10 surfaces:  %d\n", bc_10);
    printf("11-25 surfaces: %d\n", bc_25);
    printf(">25 surfaces:   %d\n", bc_more);
    printf("largest node has %d surfaces\n", bc_max);

    multi_octree.values = octree.values_f;
  }

  void global_octree_to_augmented_multi_octree(const GlobalOctree &octree, SdfMultiOctreeAugmented &multi_octree)
  {
    global_octree_to_multi_octree(octree, multi_octree.octree);
    
    multi_octree.point_channels = octree.point_channels;
    multi_octree.voxel_channels = octree.voxel_channels;
  }

  SdfMultiOctree create_sdf_multi_octree(SparseOctreeSettings settings, MultithreadedDistanceFunction sdf, unsigned max_threads)
  {
    GlobalOctree g;
    SdfMultiOctree so;
    sdf_to_global_octree(settings, sdf, max_threads, g);
    global_octree_to_multi_octree(g, so);

    return so;
  }

  SdfMultiOctree create_sdf_multi_octree(SparseOctreeSettings settings, const cmesh4::SimpleMesh &mesh)
  {
    GlobalOctree g;
    {
      auto plo = create_triangle_list_octree(mesh, PLOSettings(settings.depth));
      mesh_octree_to_global_octree(settings, mesh, plo, g);
    }
    SdfMultiOctree so;
    global_octree_to_multi_octree(g, so);

    return so;
  }

  SdfMultiOctree create_sdf_multi_octree(SparseOctreeSettings settings, const std::vector<cmesh4::SimpleMesh> &mesh, bool is_vox_merge)
  {
    GlobalOctree g;
    std::vector<GlobalOctree> gs(mesh.size());
    for (int i = 0; i < mesh.size(); ++i)
    {
      auto plo = create_triangle_list_octree(mesh[i], PLOSettings(settings.depth));
      mesh_octree_to_global_octree(settings, mesh[i], plo, gs[i]);
    }
    std::vector<GlobalOctree> active_gs;
    std::vector<uint4> codes;
    for (int i = 0; i < gs.size(); ++i)
    {
      if (gs[i].nodes.size() > 0)
      {
        active_gs.push_back(gs[i]);
        codes.push_back(uint4(0, 0, 0, 1));
      }
    }
    merge_global_octrees(g, active_gs, codes, is_vox_merge);
    SdfMultiOctree so;
    global_octree_to_multi_octree(g, so);

    return so;
  }

  SdfMultiOctreeAugmented create_sdf_augmented_multi_octree(SparseOctreeSettings settings, MultithreadedDistanceFunction sdf, unsigned max_threads)
  {
    GlobalOctree g;
    SdfMultiOctreeAugmented so;
    sdf_to_global_octree(settings, sdf, max_threads, g);
    global_octree_to_augmented_multi_octree(g, so);

    return so;
  }

  SdfMultiOctreeAugmented create_sdf_augmented_multi_octree(SparseOctreeSettings settings, const cmesh4::SimpleMesh &mesh)
  {
    GlobalOctree g;
    {
      auto plo = create_triangle_list_octree(mesh, PLOSettings(settings.depth));
      mesh_octree_to_global_octree(settings, mesh, plo, g);
    }
    SdfMultiOctreeAugmented so;
    global_octree_to_augmented_multi_octree(g, so);

    return so;
  }

  SdfMultiOctreeAugmented create_sdf_augmented_multi_octree(SparseOctreeSettings settings, const std::vector<cmesh4::SimpleMesh> &mesh, bool is_vox_merge)
  {
    GlobalOctree g;
    std::vector<GlobalOctree> gs(mesh.size());
    for (int i = 0; i < mesh.size(); ++i)
    {
      auto plo = create_triangle_list_octree(mesh[i], PLOSettings(settings.depth));
      mesh_octree_to_global_octree(settings, mesh[i], plo, gs[i]);
    }
    std::vector<GlobalOctree> active_gs;
    std::vector<uint4> codes;
    for (int i = 0; i < gs.size(); ++i)
    {
      if (gs[i].nodes.size() > 0)
      {
        active_gs.push_back(gs[i]);
        codes.push_back(uint4(0, 0, 0, 1));
      }
    }
    merge_global_octrees(g, active_gs, codes, is_vox_merge);
    SdfMultiOctreeAugmented so;
    global_octree_to_augmented_multi_octree(g, so);

    return so;
  }
}