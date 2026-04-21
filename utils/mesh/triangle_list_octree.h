#pragma once
#include "utils/common/primitive_list_octree.h"
#include "utils/mesh/mesh.h"

namespace sdf_converter
{
  using LiteMath::float3;
  
  struct TriangleInBBoxFunc
  {
    static inline bool in_bbox(const cmesh4::SimpleMesh &mesh, unsigned t_i, const float3 &bbox_min, const float3 &bbox_max)
    {
      float3 a = to_float3(mesh.vPos4f[mesh.indices[3*t_i+0]]);
      float3 b = to_float3(mesh.vPos4f[mesh.indices[3*t_i+1]]);
      float3 c = to_float3(mesh.vPos4f[mesh.indices[3*t_i+2]]);

      if (contains(a, bbox_min, bbox_max))
        return true;
      if (contains(b, bbox_min, bbox_max))
        return true;
      if (contains(c, bbox_min, bbox_max))
        return true;

      return cmesh4::triangle_aabb_intersect(a, b, c, 0.5f*(bbox_min + bbox_max), 0.5f*(bbox_max - bbox_min));
    }
  };

  static PrimitiveListOctree create_triangle_list_octree(const cmesh4::SimpleMesh &mesh, const PLOSettings &settings, 
                                                         bool multithreaded = true)
  {
    if (multithreaded)
      return create_primitive_list_octree<cmesh4::SimpleMesh, TriangleInBBoxFunc,  true>(mesh, mesh.TrianglesNum(), settings);
    else
      return create_primitive_list_octree<cmesh4::SimpleMesh, TriangleInBBoxFunc, false>(mesh, mesh.TrianglesNum(), settings);
  }
}