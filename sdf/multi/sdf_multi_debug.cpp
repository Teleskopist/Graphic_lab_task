#include "sdf_multi_debug.h"
#include "sdf/build/sparse_octree_flood_codes.h"

namespace sdf_converter
{
  void visualize_flooded_octree(const FloodedOctree &octree, SdfMultiOctree &out_octree,
                                bool only_border, bool show_code)
  {
    out_octree.nodes.resize(octree.nodes.size());
    out_octree.values.reserve(8 * octree.nodes.size());

    for (int i = 0; i < octree.nodes.size(); i++)
    {
      out_octree.nodes[i].children_offset = octree.nodes[i].offset;
      if (show_code)
      {
        out_octree.nodes[i].flags = extract_type(octree.nodes[i].code) >> 24;
        // int type = extract_type(octree.nodes[i].code);
        // if (type == TYPE_UNDECIDED_N5)
        //   out_octree.nodes[i].flags = 2;
        // else if (type == TYPE_UNDECIDED_N61)
        //   out_octree.nodes[i].flags = 4;
        // else if (type == TYPE_UNDECIDED_N62)
        //   out_octree.nodes[i].flags = 6;
        // else
        //   out_octree.nodes[i].flags = 0;
      }
      else
      {
        int2 b = border_neighbors_code(octree, octree.nodes[i]);
        if (b.x < 0)
          b.x = 0xFF;
        if (b.y < 0)
          b.y = 0xFF;
        // out_octree.nodes[i].flags = (b.x << 24) | (b.y << 16) | ((unsigned)octree.nodes[i].type << 8);
        out_octree.nodes[i].flags = ((unsigned)octree.nodes[i].type << 1);
      }

      bool outside = octree.nodes[i].type == FloodedOctreeNode::OUTSIDE;
      bool border = (octree.nodes[i].type == FloodedOctreeNode::BORDER1) ||
                    (octree.nodes[i].type == FloodedOctreeNode::BORDER2) ||
                    (octree.nodes[i].type == FloodedOctreeNode::BORDER3) ||
                    (octree.nodes[i].type == FloodedOctreeNode::DELETED_BORDER);

      if ((only_border && border) || (!only_border && !outside))
      {
        out_octree.nodes[i].data_offset = out_octree.values.size();
        out_octree.nodes[i].voxel_count = 1;
        for (int j = 0; j < 8; j++)
          out_octree.values.push_back(0.0f);
      }
      else
      {
        out_octree.nodes[i].data_offset = 0;
        out_octree.nodes[i].voxel_count = 0;
      }
    }

    out_octree.values.shrink_to_fit();
  }
}