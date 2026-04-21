#pragma once
#include <cstdint>
#include "utils/common/bit_cast.h"

struct PackedOctreeNode
{
  uint32_t short_ptr : 15;
  uint32_t is_far : 1;
  uint32_t child_is_leaf : 8;
  uint32_t child_has_data : 8;
};

inline PackedOctreeNode get_node(uint32_t data)
{
  return bit_cast<PackedOctreeNode>(data);
}

inline uint32_t get_node_data(PackedOctreeNode node)
{
  return bit_cast<uint32_t>(node);
}