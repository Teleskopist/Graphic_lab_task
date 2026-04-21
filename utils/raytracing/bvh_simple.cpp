#include "bvh_simple.h"

#include <algorithm>

using LiteMath::BBox3f;
static constexpr float SAH_OVERSPLIT_TRESHOLD = 1000000.0f;
static constexpr float EMPTY_NODE_COST_TRAVERSE = 1.0f;

struct SplitData
{
  float sah = 0.0f;
  bool subdivideNext = false;
  int axis = 0;
  int primsOnLeftSide = 0;
  int primsOnRightSide = 0;
  BBox3f leftBounds;
  BBox3f rightBounds;
};
struct Primitive
{
  int id = 0;
  BBox3f box;
};
struct PrimitiveList
{
  Primitive *data;
  size_t size;
};

float calculate_surface_area(const BBox3f&a_box)
{
  float3 sz = a_box.boxMax - a_box.boxMin;
  return 2 * (sz.x * sz.y + sz.x * sz.z + sz.y * sz.z);
}

SplitData FindObjectSplit(PrimitiveList a_plistX, PrimitiveList a_plistY, PrimitiveList a_plistZ, const BBox3f&a_box)
{
  struct BoxLessMax
  {
    BoxLessMax(int a) { axis = a; }
    bool operator()(const Primitive &p1, const Primitive &p2) const
    {
      return (p1.box.boxMax[axis] < p2.box.boxMax[axis]);
    }
    int axis;
  };

  PrimitiveList *plists[3] = {&a_plistX, &a_plistY, &a_plistZ};

  SplitData res;
  res.sah = SAH_OVERSPLIT_TRESHOLD * calculate_surface_area(a_box) * a_plistX.size; // SAH_OVERSPLIT_TRESHOLD usually equals to 1.0f
  res.subdivideNext = false;
  res.axis = -1;

  std::vector<BBox3f> rightBounds(a_plistX.size);

  for (int dim = 0; dim < 3; dim++)
  {
    // sort data according to axis
    PrimitiveList &plist = *plists[dim];

    std::sort(plist.data, plist.data + plist.size, BoxLessMax(dim));

    // Sweep right to left and determine bounds.
    BBox3f rightBounds1;
    rightBounds1.boxMin = plist.data[plist.size - 1].box.boxMin;
    rightBounds1.boxMax = plist.data[plist.size - 1].box.boxMax;

    for (uint i = plist.size - 1; i > 0; i--)
    {
      rightBounds1.boxMax = max(rightBounds1.boxMax, plist.data[i].box.boxMax);
      rightBounds1.boxMin = min(rightBounds1.boxMin, plist.data[i].box.boxMin);
      rightBounds[i - 1] = rightBounds1;
    }

    // Sweep left to right and select lowest SAH.
    BBox3f leftBounds;
    leftBounds.boxMin = plist.data[0].box.boxMin;
    leftBounds.boxMax = plist.data[0].box.boxMax;

    for (uint i = 1; i < plist.size; i++)
    {

      leftBounds.boxMin = min(leftBounds.boxMin, plist.data[i].box.boxMin);
      leftBounds.boxMax = max(leftBounds.boxMax, plist.data[i].box.boxMax);

      float sah = EMPTY_NODE_COST_TRAVERSE + calculate_surface_area(leftBounds) * (i) + calculate_surface_area(rightBounds[i - 1]) * (plist.size - i);

      if (sah < res.sah)
      {
        res.sah = sah;
        res.axis = dim;
        res.primsOnLeftSide = i;
        res.primsOnRightSide = plist.size - i;
        res.leftBounds = leftBounds;
        res.rightBounds = rightBounds[i - 1];
      }
    }
  } // for (int dim=0;dim<3;dim++)

  res.subdivideNext = (res.axis != -1) && (res.sah < SAH_OVERSPLIT_TRESHOLD * calculate_surface_area(a_box) * a_plistX.size);

  return res;
}

void BuildBVHSimple_rec(PrimitiveList primitives, const BBox3f&bbox, int maxPrimsInLeaf, BVHTree &tree, int curNodeId, bool force_leaf)
{
  if (primitives.size <= maxPrimsInLeaf || force_leaf)
  {
    int start = tree.indicesReordered.size();
    
    BVHNode node;
    node.boxMin = bbox.boxMin;
    node.boxMax = bbox.boxMax;
    node.escapeIndex = LEAF_NORMAL;
    node.leftOffset = LEAF_NORMAL; 
    
    for (int i = 0; i < primitives.size; i++)
      tree.indicesReordered.push_back(primitives.data[i].id);
    tree.intervals[curNodeId] = Interval(start, primitives.size);
    tree.nodes[curNodeId] = node;
    //printf("created leaf with %d primitives\n", (int)primitives.size);
  }
  else
  {
    std::vector<Primitive> pl_x(primitives.data, primitives.data + primitives.size);
    std::vector<Primitive> pl_y(primitives.data, primitives.data + primitives.size);
    std::vector<Primitive> pl_z(primitives.data, primitives.data + primitives.size);
    std::vector<PrimitiveList> pl = {{pl_x.data(), pl_x.size()}, {pl_y.data(), pl_y.size()}, {pl_z.data(), pl_z.size()}};
    auto res = FindObjectSplit(pl[0], pl[1], pl[2], bbox);

    if (res.axis != -1)
    {
      int nextNodeId = tree.nodes.size(); 
      tree.nodes.resize(tree.nodes.size() + 2);
      tree.intervals.resize(tree.intervals.size() + 2);

      BuildBVHSimple_rec({pl[res.axis].data, (size_t)res.primsOnLeftSide}, res.leftBounds, maxPrimsInLeaf, tree, nextNodeId+0, false);
      BuildBVHSimple_rec({pl[res.axis].data + (size_t)res.primsOnLeftSide, (size_t)res.primsOnRightSide}, 
                         res.rightBounds, maxPrimsInLeaf, tree, nextNodeId+1, false);

      BVHNode node;
      node.boxMin = bbox.boxMin;
      node.boxMax = bbox.boxMax;
      node.escapeIndex = 0;
      node.leftOffset = nextNodeId; 
      tree.intervals[curNodeId] = Interval(tree.intervals[nextNodeId+0].start, 
                                           tree.intervals[nextNodeId+0].count + tree.intervals[nextNodeId+1].count);
      tree.nodes[curNodeId] = node;
      //printf("created node with %d primitives\n", tree.intervals[curNodeId].count);
    }
    else
    {
      // leaf
      BuildBVHSimple_rec(primitives, bbox, maxPrimsInLeaf, tree, curNodeId, true);
    }
  }
}

BVHTree BuildBVHSimple(const LiteMath::float4 *a_vertices, size_t a_vertNum, const uint *a_indices, size_t a_indexNum, BVHPresets a_presets)
{
  BVHTree tree;
  tree.nodes.resize(1);
  tree.intervals.resize(1);

  int prim_count = a_indexNum / 3;
  std::vector<Primitive> primitives(prim_count);
  for (int i = 0; i < prim_count; i++)
  {
    float3 p1 = LiteMath::to_float3(a_vertices[a_indices[3 * i + 0]]);
    float3 p2 = LiteMath::to_float3(a_vertices[a_indices[3 * i + 1]]);
    float3 p3 = LiteMath::to_float3(a_vertices[a_indices[3 * i + 2]]);

    primitives[i].id = i;
    primitives[i].box.boxMin = min(p1, min(p2, p3));
    primitives[i].box.boxMax = max(p1, max(p2, p3));
  }

  BBox3f box;
  box.boxMin = primitives[0].box.boxMin;
  box.boxMax = primitives[0].box.boxMax;
  for (uint i = 0; i < primitives.size(); i++)
  {
    box.boxMin = min(box.boxMin, primitives[i].box.boxMin);
    box.boxMax = max(box.boxMax, primitives[i].box.boxMax);
  }
  BuildBVHSimple_rec({primitives.data(), primitives.size()}, box, a_presets.primsInLeaf, tree, 0, false);

  return tree;
}