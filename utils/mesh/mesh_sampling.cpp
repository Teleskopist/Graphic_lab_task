#include "mesh_sampling.h"
#include "utils/common/rand.h"
#include <algorithm>

namespace cmesh4
{
  void sample_random_points_on_mesh(const SimpleMesh &mesh, uint32_t num_points,
                                    std::vector<float3> &out_points, uint32_t *rnd_seed)
  {
    assert(check_is_valid(mesh));
    uint32_t seed = rnd_seed ? *rnd_seed : (uint32_t)rand();
    
    out_points.clear();
    out_points.reserve(num_points);
    
    // Compute triangle areas
    std::vector<float> areas(mesh.TrianglesNum());
    float total_area = 0.0f;
    for (size_t i = 0; i < mesh.TrianglesNum(); ++i) {
      size_t idx = i * 3;
      uint32_t ia = mesh.indices[idx];
      uint32_t ib = mesh.indices[idx + 1];
      uint32_t ic = mesh.indices[idx + 2];
      float3 a = to_float3(mesh.vPos4f[ia]);
      float3 b = to_float3(mesh.vPos4f[ib]);
      float3 c = to_float3(mesh.vPos4f[ic]);
      areas[i] = 0.5f * length(cross(b - a, c - a));
      total_area += areas[i];
    }
    
    // Cumulative areas for sampling
    std::vector<float> cum_areas(areas.size());
    if (!areas.empty()) {
      cum_areas[0] = areas[0];
      for (size_t i = 1; i < areas.size(); ++i) {
        cum_areas[i] = cum_areas[i - 1] + areas[i];
      }
    }
    
    // Sample points
    for (uint32_t i = 0; i < num_points; ++i) {
      float r = urand_r(&seed, 0.0f, total_area);
      auto it = std::lower_bound(cum_areas.begin(), cum_areas.end(), r);
      size_t tri_idx = it - cum_areas.begin();
      
      // Sample point in triangle
      size_t idx = tri_idx * 3;
      uint32_t ia = mesh.indices[idx];
      uint32_t ib = mesh.indices[idx + 1];
      uint32_t ic = mesh.indices[idx + 2];
      float3 a = to_float3(mesh.vPos4f[ia]);
      float3 b = to_float3(mesh.vPos4f[ib]);
      float3 c = to_float3(mesh.vPos4f[ic]);
      
      float u = urand_r(&seed);
      float v = urand_r(&seed);
      if (u + v > 1.0f) {
        u = 1.0f - u;
        v = 1.0f - v;
      }
      
      float3 point = a + u * (b - a) + v * (c - a);
      out_points.push_back(point);
    }
  }

  void sample_random_points_normals_on_mesh(const SimpleMesh &mesh, uint32_t num_points, bool use_geometry_normal,
                                            std::vector<float3> &out_points, std::vector<float3> &out_normals,
                                            uint32_t *rnd_seed)
  {
    assert(check_is_valid(mesh));
    uint32_t seed = rnd_seed ? *rnd_seed : (uint32_t)rand();
    
    out_points.clear();
    out_normals.clear();
    out_points.reserve(num_points);
    out_normals.reserve(num_points);
    
    // Compute triangle areas
    std::vector<float> areas(mesh.TrianglesNum());
    float total_area = 0.0f;
    for (size_t i = 0; i < mesh.TrianglesNum(); ++i) {
      size_t idx = i * 3;
      uint32_t ia = mesh.indices[idx];
      uint32_t ib = mesh.indices[idx + 1];
      uint32_t ic = mesh.indices[idx + 2];
      float3 a = to_float3(mesh.vPos4f[ia]);
      float3 b = to_float3(mesh.vPos4f[ib]);
      float3 c = to_float3(mesh.vPos4f[ic]);
      areas[i] = 0.5f * length(cross(b - a, c - a));
      total_area += areas[i];
    }
    
    // Cumulative areas for sampling
    std::vector<float> cum_areas(areas.size());
    if (!areas.empty()) {
      cum_areas[0] = areas[0];
      for (size_t i = 1; i < areas.size(); ++i) {
        cum_areas[i] = cum_areas[i - 1] + areas[i];
      }
    }
    
    // Sample points and normals
    for (uint32_t i = 0; i < num_points; ++i) {
      float r = urand_r(&seed, 0.0f, total_area);
      auto it = std::lower_bound(cum_areas.begin(), cum_areas.end(), r);
      size_t tri_idx = it - cum_areas.begin();
      
      // Sample point and normal in triangle
      size_t idx = tri_idx * 3;
      uint32_t ia = mesh.indices[idx];
      uint32_t ib = mesh.indices[idx + 1];
      uint32_t ic = mesh.indices[idx + 2];
      float3 a = to_float3(mesh.vPos4f[ia]);
      float3 b = to_float3(mesh.vPos4f[ib]);
      float3 c = to_float3(mesh.vPos4f[ic]);
      
      float u = urand_r(&seed);
      float v = urand_r(&seed);
      if (u + v > 1.0f) {
        u = 1.0f - u;
        v = 1.0f - v;
      }
      
      float3 point = a + u * (b - a) + v * (c - a);
      out_points.push_back(point);
      
      float3 normal;
      if (use_geometry_normal) {
        normal = normalize(cross(b - a, c - a));
      } else {
        float3 na = to_float3(mesh.vNorm4f[ia]);
        float3 nb = to_float3(mesh.vNorm4f[ib]);
        float3 nc = to_float3(mesh.vNorm4f[ic]);
        normal = normalize(na + u * (nb - na) + v * (nc - na));
      }
      out_normals.push_back(normal);
    }
  }
}