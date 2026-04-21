#include <vector>
#include <cstdint>
#include <queue>
#include <cassert>
#include <cmath>
#include <fstream>

#include "voxel_octree.h"
#include "voxel_octree_internal.h"
#include "utils/misc/scene_common.h"

using LiteMath::uint3;
using LiteMath::int3;
using LiteMath::uint4;
using LiteMath::int4;

static constexpr uint32_t INVALID_IDX = 0xFFFFFFFFu;

void save_SVO(const SparseVoxelOctree &scene, const std::string &path)
{
  std::ofstream fs(path, std::ios::binary);
  fs.write((const char *)&scene.header, sizeof(SparseVoxelOctreeHeader));
  uint32_t size = scene.data.size();
  fs.write((const char *)&size, sizeof(uint32_t));
  fs.write((const char *)scene.data.data(), size * sizeof(uint32_t));
  fs.flush();
  fs.close();  
}

void load_SVO(SparseVoxelOctree &scene, const std::string &path)
{
  std::ifstream fs(path, std::ios::binary);
  fs.read((char *)&scene.header, sizeof(SparseVoxelOctreeHeader));
  uint32_t size = 0;
  fs.read((char *)&size, sizeof(uint32_t));
  scene.data.resize(size);
  fs.read((char *)scene.data.data(), size * sizeof(uint32_t));
  fs.close();
}

ModelInfo get_info_SVO(const SparseVoxelOctree &scene)
{
  ModelInfo info;
  info.name = "SVO";
  info.num_primitives = scene.data.size();
  info.bytesize = sizeof(SparseVoxelOctreeHeader) + sizeof(uint32_t) + info.num_primitives * sizeof(uint32_t);
  return info;
}

struct TmpOctreeNode
{
  uint32_t value = 0;
  uint32_t ch_off = 0;
  uint32_t total_active_children = 0;
  uint8_t child_active_mask = 0;
  uint8_t child_leaf_mask = 0;
  uint8_t all_children_same_value = 0;
  uint8_t empty = 0;
};

void add_child_rec(std::vector<TmpOctreeNode> &tmp_octree, const std::vector<uint32_t> &grid,
                   const int3 &grid_size, const int3 &grid_pos, uint32_t max_size, 
                   uint32_t nodeId, uint4 code)
{
  int M = max_size/code.w;
  int3 node_box_min = int3(M*code.x, M*code.y, M*code.z);
  int3 node_box_max = node_box_min + int3(M, M, M);
  if (node_box_min.x > grid_pos.x + grid_size.x || node_box_max.x < grid_pos.x ||
      node_box_min.y > grid_pos.y + grid_size.y || node_box_max.y < grid_pos.y ||
      node_box_min.z > grid_pos.z + grid_size.z || node_box_max.z < grid_pos.z)
  {
    // we are outside the grid, node is guaranteed to be empty
    tmp_octree[nodeId].value = 0;
    return;
  }

  if (code.w == max_size)
  {
    int3 p = grid_pos + int3(code.x, code.y, code.z);
    if (p.x < 0 || p.y < 0 || p.z < 0 || p.x >= grid_size.x || p.y >= grid_size.y || p.z >= grid_size.z)
      tmp_octree[nodeId].value = 0;
    else
      tmp_octree[nodeId].value = grid[p.x + p.y * grid_size.x + p.z * grid_size.x * grid_size.y];
  }
  else
  {
    uint32_t off = tmp_octree.size();
    tmp_octree.resize(off + 8);
    tmp_octree[nodeId].ch_off = off;
    for (int i = 0; i < 8; i++)
    {
      uint4 ch_code = 2*code + uint4((i & 4) >> 2, (i & 2) >> 1, i & 1, 0);
      add_child_rec(tmp_octree, grid, grid_size, grid_pos, max_size, off + i, ch_code);
    }
  }
}

void fill_tmp_octree_rec(std::vector<TmpOctreeNode> &tmp_octree, bool merge_same_type, uint32_t nodeId)
{
  TmpOctreeNode &node = tmp_octree[nodeId];
  if (node.ch_off == 0)
  {
    node.all_children_same_value = 1;
    node.empty = (node.value == 0);
    node.total_active_children = node.empty ? 0 : 1;
  }
  else
  {
    for (uint32_t i = 0; i < 8; i++)
      fill_tmp_octree_rec(tmp_octree, merge_same_type, node.ch_off + i);
    
    node.value = tmp_octree[node.ch_off].value;
    node.all_children_same_value = 1;
    node.empty = 1;
    node.total_active_children = 0;
    for (uint32_t i = 0; i < 8; i++)
    {
      const TmpOctreeNode &ch_node = tmp_octree[node.ch_off + i];
      node.all_children_same_value &= ( ch_node.all_children_same_value && 
                                       (ch_node.value == node.value));
      node.empty &= ch_node.empty;
      node.total_active_children += ch_node.total_active_children;
    }
    //printf("node %d %d %d %d\n", node.value, node.empty, node.total_active_children, node.all_children_same_value);
    node.total_active_children += !node.empty;

    // remove all children, it is a leaf now 
    // (in an edge case of nodeId == 0, it is a root node and we don't want to remove it)
    if (node.empty || (merge_same_type && node.all_children_same_value && nodeId != 0))
    {
      node.ch_off = 0;
      node.total_active_children = node.empty ? 0 : 1;
    }
    else
    {
      node.child_active_mask = 0;
      node.child_leaf_mask = 0;
      for (uint32_t i = 0; i < 8; i++)
      {
        bool active = tmp_octree[node.ch_off + i].value != 0;
        bool leaf   = tmp_octree[node.ch_off + i].ch_off == 0;
        node.child_active_mask |= (active) << i;
        node.child_leaf_mask   |= (active && leaf) << i;
      }
    }
  }
}

void tmp_octree_to_octree_rec(std::vector<TmpOctreeNode> &tmp_octree, std::vector<uint32_t> &octree, 
                              uint32_t nodeId, uint32_t outNodeIdx, uint32_t longPtrIdx, uint4 code)
{
  const TmpOctreeNode &node = tmp_octree[nodeId];
  if (node.ch_off == 0)
  {
    octree[outNodeIdx] = node.value;
  }
  else
  {
    // create children masks
    uint8_t childIsLeaf = 0;
    uint8_t childHasData = 0;
    uint32_t activeChCount = 0;
    for (uint32_t i = 0; i < 8; i++)
    {
      const TmpOctreeNode &ch_node = tmp_octree[node.ch_off + i];
      childIsLeaf |= (ch_node.ch_off == 0) << i;
      childHasData |= (ch_node.value != 0 || ch_node.ch_off != 0) << i;
      activeChCount += (ch_node.value != 0 || ch_node.ch_off != 0);
    }

    uint32_t outChildrenPos = octree.size();
    uint32_t shortPtr = outChildrenPos - outNodeIdx;
    if (shortPtr <= MAX_CHILD_POINTER) // use short pointer
    {
      octree[outNodeIdx] = shortPtr | (childIsLeaf << 16) | (childHasData << 24);
    }
    else // use long pointer
    {
      //printf("%d %d %d %d - longPtrIdx = %d %d\n", code.x, code.y, code.z, code.w, outNodeIdx, longPtrIdx);
      // long pointer must have been expected by node's parent to be used here
      if (longPtrIdx == INVALID_IDX)
      {
        printf("longPtrIdx == INVALID_IDX\n");
        assert(false);
      }
      uint32_t shortPtrToLongPtr = longPtrIdx - outNodeIdx;
      if (shortPtrToLongPtr > MAX_CHILD_POINTER)
      {
        printf("shortPtrToLongPtr > MAX_CHILD_POINTER\n");
        assert(false);
      }
      octree[outNodeIdx] = shortPtrToLongPtr | IS_FAR_BIT | (childIsLeaf << 16) | (childHasData << 24);
      octree[longPtrIdx] = outChildrenPos;
    }

    // we must have some valid children
    // printf("node %d %d, code = %d %d %d %d, %d active children ch info %x %x\n", 
    //   nodeId, outNodeIdx, code.x, code.y, code.z, code.w, activeChCount,
    //   childIsLeaf, childHasData);
    assert(activeChCount > 0);

    // if total children size is > MAX_CHILD_POINTER, allocate space for far pointers
    uint32_t expectedTotalChildrenSize = 8;
    for (uint32_t i = 0; i < 8; i++)
      expectedTotalChildrenSize += tmp_octree[node.ch_off + i].total_active_children;
    bool allocateFarPointers = expectedTotalChildrenSize > MAX_CHILD_POINTER;
    octree.resize(outChildrenPos + activeChCount*(1+allocateFarPointers));

    // build children
    uint32_t ch_n = 0;
    for (uint32_t i = 0; i < 8; i++)
    {
      const TmpOctreeNode &ch_node = tmp_octree[node.ch_off + i];
      if (childHasData & (1 << i))
      {
        tmp_octree_to_octree_rec(tmp_octree, octree, node.ch_off + i, outChildrenPos + ch_n, 
                                 allocateFarPointers ? outChildrenPos + activeChCount + ch_n : INVALID_IDX,
                                 2*code + uint4((i & 4) >> 2, (i & 2) >> 1, i & 1, 0));
        ch_n++;
      }
    }
  }
}


// Main function to convert voxel grid to sparse voxel octree
SparseVoxelOctree voxel_grid_to_octree(const std::vector<uint32_t> &voxel_grid,
                                       const LiteMath::uint3 &size,
                                       bool merge_same_type)
{
  SparseVoxelOctree result;

  // Find the maximum dimension to determine octree depth
  uint32_t max_dim = std::max({size.x, size.y, size.z});
  uint32_t octree_depth = static_cast<uint32_t>(std::ceil(std::log2(max_dim)));
  uint32_t octree_size = 1u << octree_depth;

  result.header.max_level_size = octree_size;

  std::vector<TmpOctreeNode> tmp_octree;
  tmp_octree.resize(1);
  add_child_rec(tmp_octree, voxel_grid, int3(size), int3(0, 0, 0), octree_size, 0, uint4(0, 0, 0, 1));
  fill_tmp_octree_rec(tmp_octree, merge_same_type, 0);

  result.data.reserve(tmp_octree.size());
  result.data.resize(1);
  tmp_octree_to_octree_rec(tmp_octree, result.data, 0, 0, INVALID_IDX, uint4(0,0,0,1));

  return result;
}

void add_offset_to_far_links_rec(std::vector<uint32_t> &octree, uint32_t nodeId, int offset, uint4 code)
{
  PackedOctreeNode node = get_node(octree[nodeId]);
  uint32_t childrenOff = node.short_ptr + nodeId;
  if (node.is_far)
    childrenOff = octree[childrenOff];
  
  uint32_t ch_id = 0;
  for (uint32_t i = 0; i < 8; i++)
  {
    if ((node.child_has_data & (1 << i)) && !(node.child_is_leaf & (1 << i)))
    {
      add_offset_to_far_links_rec(octree, childrenOff + ch_id, offset, 2*code + uint4((i & 4) >> 2, (i & 2) >> 1, i & 1, 0));
    }
    ch_id += (node.child_has_data & (1 << i)) > 0;
  }
  if (node.is_far)
  {
    octree[node.short_ptr + nodeId] += offset;
  }
}

static bool contains(const uint4 &a, const uint4 &b)
{
  if (b.w < a.w)
    return false;
  uint4 a_min = a * (b.w / a.w);
  uint4 a_max = (a + 1) * (b.w / a.w);
  return b.x >= a_min.x && b.y >= a_min.y && b.z >= a_min.z &&
         b.x < a_max.x && b.y < a_max.y && b.z < a_max.z;
}

static bool equal(const uint4 &a, const uint4 &b)
{
  return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

void merge_voxel_octrees_rec(SparseVoxelOctree &res, std::vector<SparseVoxelOctree> &octrees, std::vector<LiteMath::uint4> &codes,
                             std::vector<uint32_t> &ids, uint32_t nodeId, uint32_t longPtrIdx, uint4 code)
{
  assert(codes.size() == octrees.size());
  std::vector<uint32_t> child_ids[8];
  for (int i = 0; i < 8; i++)
    child_ids[i].reserve(ids.size());

  //find, if there is an octree with equal code to current node
  for (uint32_t i : ids)
  {
    assert(i < codes.size());
    // octree goes to this node, move it to res and quit
    if (equal(codes[i], code))
    {
      // update max_level_size in res
      res.header.max_level_size = std::max(res.header.max_level_size, octrees[i].header.max_level_size*code.w);
      PackedOctreeNode cur_root = get_node(octrees[i].data[0]);
      uint32_t childrenOff = res.data.size();
      cur_root.is_far = 1;
      cur_root.short_ptr = longPtrIdx - nodeId;
      res.data[nodeId] = get_node_data(cur_root);
      res.data[longPtrIdx] = childrenOff;
      add_offset_to_far_links_rec(octrees[i].data, 0, childrenOff - 1, uint4(0,0,0,1));
      res.data.insert(res.data.end(), octrees[i].data.begin()+1, octrees[i].data.end());
      octrees[i] = SparseVoxelOctree();
      return;
    }
  }

  // all octrees should be put into children
  uint32_t active_ch_count = 0;
  for (int i = 0; i < 8; i++)
  {
    uint4 child_code = 2*code + uint4((i & 4) >> 2, (i & 2) >> 1, i & 1, 0);
    for (uint32_t j : ids)
    {
      if (contains(child_code, codes[j]))
        child_ids[i].push_back(j);
    }
    if (!child_ids[i].empty())
      active_ch_count++;
  }

  assert(active_ch_count > 0);

  uint32_t childrenOff = res.data.size();
  res.data.resize(childrenOff + 2*active_ch_count); //children and far links
  PackedOctreeNode cur_node;
  cur_node.short_ptr = longPtrIdx - nodeId;
  cur_node.is_far = 1;
  cur_node.child_is_leaf = 0;
  cur_node.child_has_data = 0;

  uint32_t ch_id = 0;
  for (int i = 0; i < 8; i++)
  {
    if (child_ids[i].empty())
      continue;
    uint4 child_code = 2*code + uint4((i & 4) >> 2, (i & 2) >> 1, i & 1, 0);
    merge_voxel_octrees_rec(res, octrees, codes, child_ids[i], childrenOff + ch_id,
                            childrenOff + active_ch_count + ch_id, child_code);
    cur_node.child_has_data |= (1 << i);
    ch_id++;
  }

  res.data[nodeId] = get_node_data(cur_node);
  res.data[longPtrIdx] = childrenOff;
}

SparseVoxelOctree merge_voxel_octrees(std::vector<SparseVoxelOctree> &octrees, std::vector<LiteMath::uint4> &codes)
{
  SparseVoxelOctree res;
  res.header.max_level_size = 1;
  std::vector<uint32_t> ids(octrees.size());
  for (int i = 0; i < octrees.size(); i++)
    ids[i] = i;
  res.data.resize(2);
  merge_voxel_octrees_rec(res, octrees, codes, ids, 0, 1, uint4(0,0,0,1));
  return res;
}