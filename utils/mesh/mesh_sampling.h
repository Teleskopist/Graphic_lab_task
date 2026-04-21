#pragma once
#include "mesh.h"

namespace cmesh4
{
  // Samples points uniformly on mesh surface, e.g. chance of sampling a point on a triangle is proportional to its area
  // Uses the given seed for random if available
  void sample_random_points_on_mesh(const SimpleMesh &mesh, uint32_t num_points, 
                                    std::vector<float3> &out_points, uint32_t *rnd_seed = nullptr);

  // Samples points with normals uniformly on mesh surface, e.g. chance of sampling a point 
  // on a triangle is proportional to its area. Uses interpolated vertex normals or geometry normals
  // Uses the given seed for random if available
  void sample_random_points_normals_on_mesh(const SimpleMesh &mesh, uint32_t num_points, bool use_geometry_normals,
                                            std::vector<float3> &out_points, std::vector<float3> &out_normals, 
                                            uint32_t *rnd_seed = nullptr);
}