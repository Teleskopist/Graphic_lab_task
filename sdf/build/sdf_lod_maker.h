#pragma once
#include "sdf/common/global_octree.h"

namespace sdf_converter
{
  // if node_idx has at least one non-empty child, this function constructs its surface(s)
  // based on distance values of children. Works only with nodes (i.e 1x1x1 bricks without padding)
  void fill_LOD_node(GlobalOctree &octree, unsigned node_idx, bool verbose = false);
}