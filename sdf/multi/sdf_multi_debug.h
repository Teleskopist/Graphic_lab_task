#pragma once
#include "sdf_multi.h"
#include "sdf/build/sparse_octree_flood.h"

namespace sdf_converter
{
  void visualize_flooded_octree(const FloodedOctree &octree, SdfMultiOctree &out_octree,
                                bool only_border, bool show_code);
}