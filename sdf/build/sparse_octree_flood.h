#pragma once
#include "sdf/common/global_octree.h"
#include "utils/common/primitive_list_octree.h"
#include <vector>
namespace sdf_converter
{
  struct FloodedOctreeNode
  {
    enum NodeType
    {
      UNKNOWN,
      OUTSIDE,
      BORDER1,
      BORDER2,
      BORDER3,
      INSIDE,
      DELETED_BORDER,
    };
    enum Neigh
    {
      START = 0,
      Z_P = 0,
      Z_M,
      Y_P,
      Y_M,
      X_P,
      X_M,
      N_C
    };
    NodeType type = UNKNOWN;
    unsigned offset = 0;//0 if leaf
    unsigned code = 0;
    unsigned neighs_offsets[6];
    unsigned neighs_count[6];
    uint32_t pid_intersect_count = 0;// how many primitives actually intersect this node

    unsigned parent;
    unsigned local_position;//0-7 number of child inside parent
    unsigned depth;
    uint4 pos_code;
  };

  constexpr FloodedOctreeNode::Neigh NEIGH_START = FloodedOctreeNode::Neigh::Z_P;

  struct FloodedOctree
  {
    unsigned max_depth;
    std::vector<FloodedOctreeNode> nodes;
    std::vector<unsigned> neighbours;
  };

  std::vector<uint32_t> calculate_node_codes(FloodedOctree &octree); //one code for each node
  void fill_flood_type(FloodedOctree &octree, unsigned diff_out_voxel_size);
  void fill_all_flood_type(FloodedOctree &octree, unsigned flood_start_depth);
  void prim_octree_to_flooded_octree_linear(const PrimitiveListOctree &pl_octree, FloodedOctree &out_octree, unsigned flood_start_depth);

  bool is_border(const FloodedOctreeNode &node);
  bool is_full_node(const FloodedOctreeNode &node);
  int2 border_neighbors_code(const FloodedOctree &octree, const FloodedOctreeNode &node);
}