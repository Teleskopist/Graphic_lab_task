#include "mesh.h"
#include "mesh_internal.h"
#include "LiteMath.h"
#include "utils/common/vector_comparators.h"
#include "utils/common/strings.h"
#include "tiny_obj_loader.h"
#include "ply_reader.h"
#include "stl_reader.h"
#include "procedural_meshes.h"

#include <math.h> 
#include <stack>
#include <map>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <chrono>
#include <cmath>

namespace cmesh4
{
  using namespace LiteMath;

  bool check_is_valid(const SimpleMesh &mesh, bool verbose)
  {
    bool valid = true;

    if (mesh.vPos4f.size() == 0)
    {
      if (verbose)
        printf("[check_is_valid::ERROR]: mesh has no vertices\n");
      valid = false;
    }

    if (mesh.vPos4f.size() != mesh.vNorm4f.size())
    {
      if (verbose)
        printf("[check_is_valid::ERROR]: mesh has different number of vertices and normals\n");
      valid = false;
    }

    if (mesh.vPos4f.size() != mesh.vTang4f.size())
    {
      if (verbose)
        printf("[check_is_valid::ERROR]: mesh has different number of vertices and tangents\n");
      valid = false;
    }

    if (mesh.vPos4f.size() != mesh.vTexCoord2f.size())
    {
      if (verbose)
        printf("[check_is_valid::ERROR]: mesh has different number of vertices and texture coordinates\n");
      valid = false;
    }

    if (mesh.indices.size() == 0)
    {
      if (verbose)
        printf("[check_is_valid::ERROR]: mesh has no indices\n");
      valid = false;
    }

    if (mesh.indices.size() % 3 != 0)
    {
      if (verbose)
        printf("[check_is_valid::ERROR]: mesh has invalid number of indices (should be 3*num_of_triangles)\n");
      valid = false;
    }

    if (3*mesh.matIndices.size() != mesh.indices.size())
    {
      if (verbose)
        printf("[check_is_valid::ERROR]: mesh has different number of triangles and material indices\n");
      valid = false;
    }

    for (int i=0;i<mesh.indices.size();i+=3)
    {
      int i1 = mesh.indices[i];
      int i2 = mesh.indices[i+1];
      int i3 = mesh.indices[i+2];

      if (i1 == i2 || i1 == i3 || i2 == i3)
      {
        if (verbose)
          printf("[check_is_valid::ERROR]: triangle %d has duplicate vertices (%d, %d, %d)\n", i/3, i1, i2, i3);
        valid = false;
      }

      if (i1 >= mesh.vPos4f.size() || i2 >= mesh.vPos4f.size() || i3 >= mesh.vPos4f.size())
      {
        if (verbose)
          printf("[check_is_valid::ERROR]: triangle %d has out of range vertices (%d, %d, %d)\n", i/3, i1, i2, i3);
        valid = false;
      }
    }

    return valid;
  }

  void fix_missing(SimpleMesh &mesh, int default_mat_id)
  {
    const float4 default_norm = float4(0, 0, 1, 0);
    const float4 default_tangent = float4(1, 0, 0, 0);
    const float2 default_texcoord = float2(0, 0);

    assert(mesh.vPos4f.size() >= mesh.vNorm4f.size());
    mesh.vNorm4f.resize(mesh.vPos4f.size(), default_norm);

    assert(mesh.vPos4f.size() >= mesh.vTang4f.size());
    mesh.vTang4f.resize(mesh.vPos4f.size(), default_tangent);

    assert(mesh.vPos4f.size() >= mesh.vTexCoord2f.size());
    mesh.vTexCoord2f.resize(mesh.vPos4f.size(), default_texcoord);

    assert(mesh.indices.size() >= 3*mesh.matIndices.size());
    assert(mesh.indices.size() % 3 == 0);
    mesh.matIndices.resize(mesh.indices.size() / 3, default_mat_id);
  }

  void get_bbox(const SimpleMesh &mesh, float3 *min_pos, float3 *max_pos)
  {
    *min_pos = float3(1e9,1e9,1e9);
    *max_pos = float3(-1e9,-1e9,-1e9);

    for (const float4 &p : mesh.vPos4f)
    {
      *min_pos = min(*min_pos, to_float3(p));
      *max_pos = max(*max_pos, to_float3(p));
    }
  }

  float4x4 rescale_mesh(SimpleMesh &mesh, float3 min_pos, float3 max_pos)
  {
    assert(mesh.vPos4f.size() >= 3);

    float3 mesh_min, mesh_max;
    get_bbox(mesh, &mesh_min, &mesh_max);

    float3 mesh_size = mesh_max - mesh_min;
    float3 target_size = max_pos - min_pos;
    float3 scale3 = target_size/mesh_size;
    float scale = min(scale3.x, min(scale3.y, scale3.z));
    
    float4 scale_4 = float4(scale,scale,scale,1);
    float4 min_4 = to_float4(min_pos, 0.0f);
    float4 mesh_min_4 = to_float4(mesh_min, 0.0f);

    //changing poditions, .w coord is preserved
    for (float4 &p : mesh.vPos4f)
      p = min_4 + scale_4*(p - mesh_min_4);

    //it is only move and rescale, so now changes to normals are required

    float4x4 trans = translate4x4(min_pos)*scale4x4(float3(scale))*translate4x4(-mesh_min);
    return trans;
  }

  void transform_mesh(SimpleMesh &mesh, LiteMath::float4x4 transform)
  {
    LiteMath::float4x4 norm_transform = transpose(inverse4x4(transform));
    for (float4 &p : mesh.vPos4f)
    {
      float w = p.w;
      p.w = 1;
      p = transform * p;
      p.w = w;
    }
    for (float4 &p : mesh.vNorm4f)
    {
      float w = p.w;
      p.w = 0;
      p = norm_transform * p;
      p.w = w;
    }
  }

  void set_mat_id(SimpleMesh &mesh, int mat_id)
  {
    std::fill(mesh.matIndices.begin(), mesh.matIndices.end(), mat_id);
  }

  bool triangle_aabb_intersect(const float3 &a, const float3 &b, const float3 &c, 
                               const float3 &aabb_center, const float3 &aabb_half_size)
  {
    // Move triangle vertices so that AABB is at the origin
    float3 v0 = a - aabb_center;
    float3 v1 = b - aabb_center;
    float3 v2 = c - aabb_center;

    // Compute triangle edges
    float3 e0 = v1 - v0;
    float3 e1 = v2 - v1;
    float3 e2 = v0 - v2;

    // Test axes a00..a22 (axes from cross products of edges and coordinate axes)
    auto axis_test = [&](const float3 &axis) -> bool {
        // Project triangle onto axis
        float p0 = dot(v0, axis);
        float p1 = dot(v1, axis);
        float p2 = dot(v2, axis);
        float r = aabb_half_size.x * fabsf(axis.x) +
                  aabb_half_size.y * fabsf(axis.y) +
                  aabb_half_size.z * fabsf(axis.z);
        float min_p = fminf(p0, fminf(p1, p2));
        float max_p = fmaxf(p0, fmaxf(p1, p2));
        return (min_p <= r && max_p >= -r);
    };

    // Test the 9 cross product axes
    float3 axes[9] = {
        float3(0, -e0.z, e0.y), float3(0, -e1.z, e1.y), float3(0, -e2.z, e2.y),
        float3(e0.z, 0, -e0.x), float3(e1.z, 0, -e1.x), float3(e2.z, 0, -e2.x),
        float3(-e0.y, e0.x, 0), float3(-e1.y, e1.x, 0), float3(-e2.y, e2.x, 0)
    };

    for (int i = 0; i < 9; ++i) {
        if (!axis_test(axes[i])) return false;
    }

    // Test overlap in the x, y, and z directions (the AABB's face normals)
    auto test_axis = [&](int axis_index) -> bool {
        float min_v = fminf(v0[axis_index], fminf(v1[axis_index], v2[axis_index]));
        float max_v = fmaxf(v0[axis_index], fmaxf(v1[axis_index], v2[axis_index]));
        if (min_v > aabb_half_size[axis_index] || max_v < -aabb_half_size[axis_index])
            return false;
        return true;
    };

    if (!test_axis(0)) return false;
    if (!test_axis(1)) return false;
    if (!test_axis(2)) return false;

    // Test triangle normal axis
    float3 normal = cross(e0, e1);
    float d = -dot(normal, v0);
    float r = aabb_half_size.x * fabsf(normal.x) +
              aabb_half_size.y * fabsf(normal.y) +
              aabb_half_size.z * fabsf(normal.z);
    if (d > r || d < -r) return false;

    // If no separating axis is found, intersection exists
    return true;
  }

  struct double3
  {
    double x, y, z;
    double3() {x = 0, y = 0, z = 0;}
    double3(float3 a) {x = a.x; y = a.y; z = a.z;}
    double3(double _x, double _y, double _z) {x = _x; y = _y; z = _z;}
  };

  static inline double dot(const double3 a, const double3 b)  { return a.x*b.x + a.y*b.y + a.z*b.z; }

  static inline double3 shuffle_yzx(double3 a) { return double3{a.y, a.z, a.x}; }

  static inline double3 cross(const double3 a, const double3 b) 
  {
    const double3 a_yzx = shuffle_yzx(a);
    const double3 b_yzx = shuffle_yzx(b);
    double3 r = {a.x * b_yzx.x - a_yzx.x * b.x,
                 a.y * b_yzx.y - a_yzx.y * b.y,
                 a.z * b_yzx.z - a_yzx.z * b.z};
    return shuffle_yzx(r);
  }

  float3 to_float3(double3 a)
  {
    return {float(a.x), float(a.y), float(a.z)};
  }

  double3 operator+(double3 a, double3 b)
  {
    double x = a.x + b.x;
    double y = a.y + b.y;
    double z = a.z + b.z;
    return double3(x, y, z);
  }

  double3 operator-(double3 a, double3 b)
  {
    double x = a.x - b.x;
    double y = a.y - b.y;
    double z = a.z - b.z;
    return double3(x, y, z);
  }

  double3 operator*(double a, double3 b)
  {
    double3 ret = b;
    ret.x *= a;
    ret.y *= a;
    ret.z *= a;
    return ret;
  }

  float3 closest_point_triangle(const float3 &pp, const float3 &aa, const float3 &bb, const float3 &cc)
  {
    // implementation taken from Embree library
    const double3 a = aa;
    const double3 b = bb;
    const double3 c = cc;
    const double3 p = pp;
    const double3 ab = b - a;
    const double3 ac = c - a;
    const double3 ap = p - a;

    const double d1 = dot(ab, ap);
    const double d2 = dot(ac, ap);
    if (d1 <= 0.f && d2 <= 0.f)
      return to_float3(a); // #1

    const double3 bp = p - b;
    const double d3 = dot(ab, bp);
    const double d4 = dot(ac, bp);
    if (d3 >= 0.f && d4 <= d3)
      return to_float3(b); // #2

    const double3 cp = p - c;
    const double d5 = dot(ab, cp);
    const double d6 = dot(ac, cp);
    if (d6 >= 0.f && d5 <= d6)
      return to_float3(c); // #3

    const double vc = d1 * d4 - d3 * d2;
    if (vc <= 0.f && d1 >= 0.f && d3 <= 0.f)
    {
      const double v = d1 / (d1 - d3);
      return to_float3(a + v * ab); // #4
    }

    const double vb = d5 * d2 - d1 * d6;
    if (vb <= 0.f && d2 >= 0.f && d6 <= 0.f)
    {
      const double v = d2 / (d2 - d6);
      return to_float3(a + v * ac); // #5
    }

    const double va = d3 * d6 - d5 * d4;
    if (va <= 0.f && (d4 - d3) >= 0.f && (d5 - d6) >= 0.f)
    {
      const double v = (d4 - d3) / ((d4 - d3) + (d5 - d6));
      return to_float3(b + v * (c - b)); // #6
    }

    const double denom = 1.f / (va + vb + vc);
    const double v = vb * denom;
    const double w = vc * denom;
    return to_float3(a + v * ab + w * ac); // #0
  }

  // Compute barycentric coordinates (u, v, w) for
  // point p with respect to triangle (a, b, c)
  float3 barycentric(const float3 &p, const float3 &a, const float3 &b, const float3 &c)
  {
    float3 v0 = b - a, v1 = c - a, v2 = p - a;
    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d11 = dot(v1, v1);
    float d20 = dot(v2, v0);
    float d21 = dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;
    return float3(u, v, w);
  }

  bool intersect_segment_triangle(const float3 &pp, const float3 &qq, 
                                  const float3 &aa, const float3 &bb, const float3 &cc, 
                                  float &offset)
  {
    const double3 a = aa;
    const double3 b = bb;
    const double3 c = cc;
    const double3 p = pp;
    const double3 q = qq;
    const double3 pq = q - p;
    const double3 ab = b - a;
    const double3 ac = c - a;
    const double3 norm = cross(ab, ac);
    const double d = dot(norm, pq);
    if (std::abs(d) < 1e-8)
      return false;
    offset = dot(a - p, norm) / d;
    if (offset < 0.0f || offset > 1.0f)
      return false;
    double3 bar = barycentric(pp + offset * (qq - pp), aa, bb, cc);
    return bar.x >= 0.0f && bar.z <= 1.0f && 
           bar.y >= 0.0f && bar.y <= 1.0f && 
           bar.z >= 0.0f && bar.z <= 1.0f;
  }

  inline bool ray_intersects_triangle(const float3 &ray_origin,
                                      const float3 &ray_vector,
                                      const float3 &a,
                                      const float3 &b,
                                      const float3 &c)
  {
    constexpr float epsilon = std::numeric_limits<float>::epsilon();

    float3 edge1 = b - a;
    float3 edge2 = c - a;
    float3 ray_cross_e2 = cross(ray_vector, edge2);
    float det = dot(edge1, ray_cross_e2);

    if (det > -epsilon && det < epsilon)
      return false; // This ray is parallel to this triangle.

    float inv_det = 1.0 / det;
    float3 s = ray_origin - a;
    float u = inv_det * dot(s, ray_cross_e2);

    if (u < 0 || u > 1)
      return false;

    float3 s_cross_e1 = cross(s, edge1);
    float v = inv_det * dot(ray_vector, s_cross_e1);

    if (v < 0 || u + v > 1)
      return false;

    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = inv_det * dot(edge2, s_cross_e1);

    if (t > epsilon) // ray intersection
    {
      // out_intersection_point = ray_origin + ray_vector * t;
      return true;
    }
    else // This means that there is a line intersection but not a ray intersection.
      return false;
  }

  int intersection_count(const SimpleMesh &mesh, const float3 &ray_origin, const float3 &ray_vector)
  {
    int cnt = 0;
    for (int i=0;i<mesh.TrianglesNum();i++)
    {
      float3 a = to_float3(mesh.vPos4f[mesh.indices[3*i+0]]);
      float3 b = to_float3(mesh.vPos4f[mesh.indices[3*i+1]]);
      float3 c = to_float3(mesh.vPos4f[mesh.indices[3*i+2]]);
      if (ray_intersects_triangle(ray_origin, ray_vector, a, b, c))
        cnt++;
    }
    return cnt;
  }

  void fix_normals(SimpleMesh &mesh, bool verbose)
  {
    {
      int broken_normals_cnt = 0;
      std::vector<bool> broken_normals(mesh.vNorm4f.size(), false);
      for (int i=0;i<mesh.vNorm4f.size();i++)
      {
        float3 n = to_float3(mesh.vNorm4f[i]);
        float len = dot(n, n);
        if (len < 1e-6 || std::isnan(len) || std::isinf(len))
        {
          broken_normals[i] = true;
          broken_normals_cnt++;
        } 
        else
          mesh.vNorm4f[i] = to_float4(n/sqrt(len), 1);
      }
      if (broken_normals_cnt > 0)
      {
        if (verbose)
          printf("WARNING: mesh has %d broken normals\n", broken_normals_cnt);
        //TODO: fix broken normals
      }
      else
      {
        if (verbose)
          printf("OK: mesh has no broken normals\n");
      }
    }

    int total_visited = 0;
    int flipped_normals = 0;
    std::vector<std::vector<unsigned>> edges(mesh.vPos4f.size(), std::vector<unsigned>());
    std::vector<bool> vertex_visited(mesh.vPos4f.size(), false);
    for (int i = 0; i < mesh.indices.size(); i += 3)
    {
      unsigned a = mesh.indices[i];
      unsigned b = mesh.indices[i + 1];
      unsigned c = mesh.indices[i + 2];
      edges[a].push_back(b);
      edges[a].push_back(c);
      edges[b].push_back(a);
      edges[b].push_back(c);
      edges[c].push_back(a);
      edges[c].push_back(b);
    }

    while (total_visited < mesh.vPos4f.size())
    {
      //TODO: check if the first normal in pointing in the right direction
      int start_index = 0;
      for (int i=0;i<mesh.vPos4f.size();i++)
      {
        if (!vertex_visited[i])
        {
          start_index = i;
          break;
        }
      }
      int intersection_cnt = intersection_count(mesh, 
                                                to_float3(mesh.vPos4f[start_index]) + 1e-6f*to_float3(mesh.vNorm4f[start_index]), 
                                                to_float3(mesh.vNorm4f[start_index]));

      if (verbose)
        printf("intersection_cnt %d\n", intersection_cnt);
      if (intersection_cnt % 2)
        mesh.vNorm4f[start_index] *= -1;

      std::stack<unsigned> stack;
      stack.push(start_index);
      while (!stack.empty())
      {
        unsigned v = stack.top();
        stack.pop();
        if (!vertex_visited[v])
        {
          vertex_visited[v] = true;
          float3 n = to_float3(mesh.vNorm4f[v]);
          for (auto &v2 : edges[v])
          {
            if (!vertex_visited[v2])
            {
              float3 n2 = to_float3(mesh.vNorm4f[v2]);
              if (dot(n, n2) < 0)
              {
                flipped_normals++;
                mesh.vNorm4f[v2] = to_float4(-n2, 1);
              }
              stack.push(v2);
            }
          }
        }
      }

      total_visited = 0;
      for (int i=0;i<vertex_visited.size();i++)
        total_visited += vertex_visited[i];

      if (verbose)
      {
        printf("visited %d/%d vertices\n", total_visited, (int)vertex_visited.size());

        if (flipped_normals == 0 )
          printf("OK: all normals pointing in the same direction\n");
        else
          printf("WARNING: %d/%d were pointing in the wrong direction and were fixed\n", flipped_normals, (int)mesh.vNorm4f.size());
      }
    }
  }

  void compress_close_vertices(SimpleMesh &mesh, double threshold, bool force_merge_distinct_normals, bool verbose)
  {
    std::map<int3, std::vector<unsigned>, cmpInt3> vert_indices;
    float inv_thr = 1.0/std::max(1e-6, threshold);

    for (int i=0;i<mesh.vPos4f.size();i++)
    {
      int3 v_idx0 = int3(inv_thr*LiteMath::to_float3(mesh.vPos4f[i]));
      std::vector<int3> v_idxes = 
        {v_idx0 + int3(0, 0, 0), v_idx0 + int3(0, 0, 1), v_idx0 + int3(0, 1, 0), v_idx0 + int3(0, 1, 1),
         v_idx0 + int3(1, 0, 0), v_idx0 + int3(1, 0, 1), v_idx0 + int3(1, 1, 0), v_idx0 + int3(1, 1, 1)};
      for (auto &v_idx : v_idxes)
      {
        auto it = vert_indices.find(v_idx);
        if (it == vert_indices.end())
          vert_indices[v_idx] = {(unsigned)i};
        else
          vert_indices[v_idx].push_back(i);
      
        //printf("v_idx %d %d %d size %d\n", v_idx.x, v_idx.y, v_idx.z, (int)vert_indices[v_idx].size());
      }
      if (verbose && (i==0 || i%1000000==0))
        printf("[compress_close_vertices::INFO] (%d/%d) Filling spatial hash\n", i, (int)mesh.vPos4f.size());
    }

    if (verbose)
      printf("[compress_close_vertices::INFO] Filled spatial hash\n");

    std::vector<int> vert_remap(mesh.vPos4f.size(), -1);
    std::vector<bool> remapped(mesh.vPos4f.size(), false);
    std::vector<int> vert_remap_inv(mesh.vPos4f.size(), -1);
    unsigned new_idx = 0;

    int idx = 0;
    for (auto &it : vert_indices)
    {
      //printf("Checking %d vertices\n", (int)it.second.size());
      for (auto &idx1 : it.second)
      {
        if (remapped[idx1])
          continue;
        for (auto &idx2 : it.second)
        {
          if (idx1 >= idx2)
            continue;
          float3 p1 = to_float3(mesh.vPos4f[idx1]);
          float3 p2 = to_float3(mesh.vPos4f[idx2]);

          float3 n1 = to_float3(mesh.vNorm4f[idx1]);
          float3 n2 = to_float3(mesh.vNorm4f[idx2]);
          if (!remapped[idx1] && !remapped[idx2] &&
              sqrt((double)(p1.x-p2.x)*(double)(p1.x-p2.x) + (double)(p1.y-p2.y)*(double)(p1.y-p2.y) + (double)(p1.z-p2.z)*(double)(p1.z-p2.z)) < threshold)
          {
            double d = sqrt((double)(p1.x-p2.x)*(double)(p1.x-p2.x) + (double)(p1.y-p2.y)*(double)(p1.y-p2.y) + (double)(p1.z-p2.z)*(double)(p1.z-p2.z));
            if (force_merge_distinct_normals || length(n1) < 1e-6f || length(n2) < 1e-6f || dot(n1, n2) > 0.99f) //we can safely merge the vertices
            {
              remapped[idx2] = true;
              if (vert_remap[idx1] == -1)
              {
                vert_remap[idx1] = new_idx;
                vert_remap_inv[new_idx] = idx1;
                new_idx++;
              }
              vert_remap[idx2] = vert_remap[idx1];
            }
          }
        }
      }

      if (verbose && (idx==0 || idx%1000000==0))
        printf("[compress_close_vertices::INFO] (%d/%d) Processing spatial hash\n", idx, (int)vert_indices.size());
      idx++;
    }

    for (int i=0;i<mesh.vPos4f.size();i++)
    {
      if (vert_remap[i] == -1)
      {
        vert_remap[i] = new_idx;
        vert_remap_inv[new_idx] = i;
        new_idx++;
      }
    }

    if (new_idx == mesh.vPos4f.size())
    {
      if (verbose)
        printf("[compress_close_vertices::INFO] mesh has no close vertices\n");
    }
    else
    {
      if (verbose)
        printf("[compress_close_vertices::INFO] %d/%d vertices were close and were compressed\n", (int)(mesh.vPos4f.size() - new_idx), (int)mesh.vPos4f.size());
    
      //    std::vector<LiteMath::float4> vPos4f; 
      //    std::vector<LiteMath::float4> vNorm4f;
      //    std::vector<LiteMath::float4> vTang4f;
      //    std::vector<float2>           vTexCoord2f;
      //    std::vector<unsigned int>     indices;
      //    std::vector<unsigned int>     matIndices;

      std::vector<LiteMath::float4> new_pos4f(new_idx);
      std::vector<LiteMath::float4> new_norm4f(new_idx);
      std::vector<LiteMath::float4> new_tang4f(new_idx);
      std::vector<float2>           new_tex2f(new_idx);
      std::vector<unsigned int>     new_indices;
      std::vector<unsigned int>     new_matIndices;

      for (int i=0;i<new_idx;i++)
      {
        new_pos4f[i] = mesh.vPos4f[vert_remap_inv[i]];
        new_norm4f[i] = mesh.vNorm4f[vert_remap_inv[i]];
        new_tang4f[i] = mesh.vTang4f[vert_remap_inv[i]];
        new_tex2f[i] = mesh.vTexCoord2f[vert_remap_inv[i]];
      }

      for (int i=0;i<mesh.indices.size();i+=3)
      {
        unsigned idx0 = vert_remap[mesh.indices[i+0]];
        unsigned idx1 = vert_remap[mesh.indices[i+1]];
        unsigned idx2 = vert_remap[mesh.indices[i+2]];

        if (idx0 == idx1 || idx0 == idx2 || idx1 == idx2)
          continue;

        new_indices.push_back(idx0);
        new_indices.push_back(idx1);
        new_indices.push_back(idx2);

        assert(i / 3 < mesh.matIndices.size());
        new_matIndices.push_back(mesh.matIndices[i/3]);
      }

      int old_triangles = mesh.matIndices.size();

      mesh.vPos4f = new_pos4f;
      mesh.vNorm4f = new_norm4f;
      mesh.vTang4f = new_tang4f;
      mesh.vTexCoord2f = new_tex2f;
      mesh.indices = new_indices;
      mesh.matIndices = new_matIndices;

      if (verbose)
        printf("[compress_close_vertices::INFO] %d/%d triangles were removed after vertex compression\n",old_triangles - (int)mesh.matIndices.size(), old_triangles);
    }
  }

  LiteMath::float4x4 normalize_mesh(SimpleMesh &mesh, bool verbose)
  {
    LiteMath::float4x4 transform = rescale_mesh(mesh, 0.999f*float3(-1, -1, -1), 0.999f*float3(1, 1, 1));
    //compress_close_vertices(mesh, 1e-12f, verbose); //should not be used until I rule out why it can break normals (on dragon model at least)

    bool is_watertight = true; //check_watertight_mesh(mesh, verbose);
    if (verbose)
    {
      if (is_watertight)
        printf("OK: mesh is watertight\n");
      else
        printf("WARNING: mesh is not watertight\n");
    }

    //fix_normals(mesh, verbose);

    return transform;
  }

  void recalculate_vertex_normals(SimpleMesh &mesh)
  {
    assert(mesh.vPos4f.size() == mesh.vNorm4f.size());
    std::vector<float4> vec_norm_count(mesh.vPos4f.size());

    for (int i=0;i<mesh.indices.size();i+=3)
    {
      unsigned idx0 = mesh.indices[i+0];
      unsigned idx1 = mesh.indices[i+1];
      unsigned idx2 = mesh.indices[i+2];

      float3 p0 = to_float3(mesh.vPos4f[idx0]); 
      float3 p1 = to_float3(mesh.vPos4f[idx1]);
      float3 p2 = to_float3(mesh.vPos4f[idx2]);

      float4 triangle_normal = to_float4(normalize(cross(p1 - p0, p2 - p0)), 1.0f);

      vec_norm_count[idx0] += triangle_normal;
      vec_norm_count[idx1] += triangle_normal;
      vec_norm_count[idx2] += triangle_normal;
    }

    for (int i=0;i<mesh.vNorm4f.size();i++)
      mesh.vNorm4f[i] = to_float4(to_float3(vec_norm_count[i]) / vec_norm_count[i].w, 0.0f);
  }

  SimpleMesh LoadMesh(const char* a_fileName, bool apply_basic_transforms, bool verbose, 
                              LiteMath::float4x4 *out_transform, bool rescale)
  {
    SimpleMesh dummy;
    SimpleMesh mesh;
    auto substrings = split(a_fileName, '.');
    if (substrings.size() == 1)
    {
      printf("[LoadMesh::ERROR] File %s has no extension. Use specific loaderer if you are sure what type it is\n", a_fileName);
      return dummy;
    }

    if (substrings[substrings.size() - 1] == "obj")
    {
      if (verbose)
        printf("[LoadMesh::INFO] Loading OBJ file %s\n", a_fileName);
      
      mesh = LoadMeshFromObj(a_fileName, verbose);
    }
    else if (substrings[substrings.size() - 1] == "vsgf")
    {
      if (verbose)
        printf("[LoadMesh::INFO] Loading VSGF file %s\n", a_fileName);
      mesh = LoadMeshFromVSGF(a_fileName);
    }
    else if (substrings[substrings.size() - 1] == "ply")
    {
      if (verbose)
        printf("[LoadMesh::INFO] Loading PLY file %s\n", a_fileName);
      mesh = LoadMeshFromPly(a_fileName, verbose);
    }
    else if (substrings[substrings.size() - 1] == "stl")
    {
      if (verbose)
        printf("[LoadMesh::INFO] Loading STL file %s\n", a_fileName);
      mesh = LoadMeshFromStl(a_fileName, verbose);
    }
    else if (substrings[substrings.size() - 1] == "blk")
    {
      if (verbose)
        printf("[LoadMesh::INFO] Create procedural mesh, settings taken from %s\n", a_fileName);
      mesh = create_procedural_mesh(a_fileName);
    }
    else
    {
      printf("[LoadMesh::ERROR] File %s has unknown extension \"%s\".\n", a_fileName, substrings[substrings.size() - 1].c_str());
      return dummy;
    }

    fix_missing(mesh);
    assert(check_is_valid(mesh, true));

    if (verbose)
      printf("[LoadMesh::INFO] Normalizing mesh\n");

    float4x4 transform = float4x4();
    
    if (rescale)
      transform = normalize_mesh(mesh, verbose);

    if (out_transform)
      *out_transform = transform;
    
    if (apply_basic_transforms)
    {
      if (verbose)
        printf("[LoadMesh::INFO] Compressing close vertices\n");
      compress_close_vertices(mesh, 1e-9f, false, verbose);

      if (verbose)
        printf("[LoadMesh::INFO] Recalculating normals\n");
      recalculate_vertex_normals(mesh);
    }

    if (verbose)
      printf("[LoadMesh::INFO] Mesh %s has %d unique vertices and %d triangles\n", a_fileName, (int)mesh.vPos4f.size(), (int)mesh.indices.size() / 3);

    return mesh;
  }

  void SaveMeshToObj(const char* a_fileName, const SimpleMesh &mesh)
  {
    std::string header = "#obj file created by custom obj loader\n";
    std::string o_data = "o MainModel\n";
    std::string v_data;
    std::string tc_data;
    std::string n_data;
    std::string s_data = "s off\n";
    std::string f_data;

    int sz = mesh.vPos4f.size();
    assert(mesh.vNorm4f.size() == sz);
    assert(mesh.vTexCoord2f.size() == sz);

    for (int i = 0; i < sz; ++i)
    {
      v_data += "v " + std::to_string(mesh.vPos4f[i].x) + " " + std::to_string(mesh.vPos4f[i].y) + " " + std::to_string(mesh.vPos4f[i].z) + "\n";
      n_data += "vn " + std::to_string(mesh.vNorm4f[i].x) + " " + std::to_string(mesh.vNorm4f[i].y) + " " + std::to_string(mesh.vNorm4f[i].z) + "\n";
      tc_data += "vt " + std::to_string(mesh.vTexCoord2f[i].x) + " " + std::to_string(mesh.vTexCoord2f[i].y) + "\n";
    }
    for (int i = 0; i < mesh.indices.size() / 3; ++i)
    {
      f_data += "f " + std::to_string(mesh.indices[3*i]+1) + "/" + std::to_string(mesh.indices[3*i]+1) + "/" + std::to_string(mesh.indices[3*i]+1) + " " +
                std::to_string(mesh.indices[3*i+1]+1) + "/" + std::to_string(mesh.indices[3*i+1]+1) + "/" + std::to_string(mesh.indices[3*i+1]+1) + " " +
                std::to_string(mesh.indices[3*i+2]+1) + "/" + std::to_string(mesh.indices[3*i+2]+1) + "/" + std::to_string(mesh.indices[3*i+2]+1) + "\n";
    }

    std::ofstream out(a_fileName);
    out << header;
    out << o_data;
    out << v_data;
    out << tc_data;
    out << n_data;
    out << s_data;
    out << f_data;
    out.close();
  }

  SimpleMesh MergeMeshes(const std::vector<SimpleMesh> &meshes)
  {
    SimpleMesh result;
    for (auto &m : meshes)
    {
      size_t vert = result.VerticesNum();
      size_t ind = result.IndicesNum();
      result.vPos4f.insert(result.vPos4f.end(), m.vPos4f.begin(), m.vPos4f.end());
      result.vNorm4f.insert(result.vNorm4f.end(), m.vNorm4f.begin(), m.vNorm4f.end());
      result.vTang4f.insert(result.vTang4f.end(), m.vTang4f.begin(), m.vTang4f.end());
      result.vTexCoord2f.insert(result.vTexCoord2f.end(), m.vTexCoord2f.begin(), m.vTexCoord2f.end());
      result.indices.insert(result.indices.end(), m.indices.begin(), m.indices.end());
      result.matIndices.insert(result.matIndices.end(), m.matIndices.begin(), m.matIndices.end());
      for (size_t i = ind; i < result.IndicesNum(); ++i)
      {
        result.indices[i] += vert;
      }
    }
    return result;
  }

  struct AlignedAxis
  {
    int axis;// 0=X, 1=Y, 2=Z
    float3 direction;
    float totalArea;
  };

  struct NormalCluster
  {
    float3 direction;
    float totalArea;
    std::vector<size_t> triangleIndices;
  };

  float3 getAxisDirection(int axis)
  {
    switch (axis)
    {
      case 0: return float3(1, 0, 0);
      case 1: return float3(0, 1, 0);
      case 2: return float3(0, 0, 1);
      default: return float3(0, 0, 1);
    }
  }

  int findBestAlignmentAxis(const float3& direction)
  {
    float dotX = std::abs(dot(direction, float3(1, 0, 0)));
    float dotY = std::abs(dot(direction, float3(0, 1, 0)));
    float dotZ = std::abs(dot(direction, float3(0, 0, 1)));
    
    if (dotX >= dotY && dotX >= dotZ) return 0; // X
    if (dotY >= dotZ) return 1; // Y
    return 2; // Z
  }

  int findAlternativeAxis(const float3& direction, int excludeAxis)
  {
    float dots[3] = {
        std::abs(dot(direction, float3(1, 0, 0))),
        std::abs(dot(direction, float3(0, 1, 0))),
        std::abs(dot(direction, float3(0, 0, 1)))};
    
    dots[excludeAxis] = -1.0f;
    
    int bestAxis = 0;
    for (int i = 1; i < 3; ++i)
    {
      if (dots[i] > dots[bestAxis])
      {
        bestAxis = i;
      }
    }
    
    return bestAxis;
  }


  NormalCluster findMaxAreaCluster(const std::vector<NormalCluster>& clusters)
  {
    NormalCluster maxCluster;
    maxCluster.totalArea = 0;
    
    for (const auto& cluster : clusters)
    {
      if (cluster.totalArea > maxCluster.totalArea)
      {
        maxCluster = cluster;
      }
    }
    
    return maxCluster;
  }

  void computeTriangleNormalsAndAreas(const SimpleMesh& mesh, 
                                      std::vector<float3>& normals, 
                                      std::vector<float>& areas)
  {
    normals.clear();
    areas.clear();
    normals.reserve(mesh.TrianglesNum());
    areas.reserve(mesh.TrianglesNum());
    
    for (size_t i = 0; i < mesh.TrianglesNum(); ++i)
    {
      uint32_t idx0 = mesh.indices[i * 3 + 0];
      uint32_t idx1 = mesh.indices[i * 3 + 1];
      uint32_t idx2 = mesh.indices[i * 3 + 2];
      
      float3 v0 = to_float3(mesh.vPos4f[idx0]);
      float3 v1 = to_float3(mesh.vPos4f[idx1]);
      float3 v2 = to_float3(mesh.vPos4f[idx2]);
      
      float3 edge1 = v1 - v0;
      float3 edge2 = v2 - v0;
      float3 normal = cross(edge1, edge2);
      
      float area = length(normal) * 0.5f;
      
      if (area > 1e-10f)
      {
        areas.push_back(area);
        normals.push_back(normalize(normal));
      }
    }
  }

  std::vector<NormalCluster> clusterNormalsByDirection(const std::vector<float3>& normals, 
                                                       const std::vector<float>& areas, 
                                                       float tolerance)
  {
    std::vector<NormalCluster> clusters;
    
    for (size_t i = 0; i < normals.size(); ++i)
    {
      float3 currentNormal = normals[i];
      float currentArea = areas[i];
      
      bool foundCluster = false;
      for (auto& cluster : clusters)
      {
        float similarity = std::abs(dot(currentNormal, cluster.direction));
        
        if (similarity > (1.0f - tolerance))
        {
          cluster.totalArea += currentArea;
          cluster.triangleIndices.push_back(i);
          
          float3 weightedDirection = cluster.direction * cluster.totalArea + currentNormal * currentArea;
          cluster.direction = normalize(weightedDirection);
          foundCluster = true;
          break;
        }
      }
      
      if (!foundCluster) {
          NormalCluster newCluster;
          newCluster.direction = currentNormal;
          newCluster.totalArea = currentArea;
          newCluster.triangleIndices.push_back(i);
          clusters.push_back(newCluster);
      }
    }
    
    return clusters;
  }

  float4x4 createRotationMatrix(const float3& axis, float angle)
  {
    float4x4 result;
    result.identity();
    
    float c = std::cos(angle);
    float s = std::sin(angle);
    float t = 1.0f - c;
    
    float x = axis.x;
    float y = axis.y;
    float z = axis.z;
    
    result(0, 0) = t * x * x + c;
    result(0, 1) = t * x * y - s * z;
    result(0, 2) = t * x * z + s * y;
    result(1, 0) = t * x * y + s * z;
    result(1, 1) = t * y * y + c;
    result(1, 2) = t * y * z - s * x;
    result(2, 0) = t * x * z - s * y;
    result(2, 1) = t * y * z + s * x;
    result(2, 2) = t * z * z + c;
    
    return result;
  }

  float4x4 createOptimalRotation(const float3& fromDirection, int toAxis, float tolerance)
  {
    float3 targetDirection;
    switch (toAxis)
    {
      case 0: targetDirection = float3(1, 0, 0); break;
      case 1: targetDirection = float3(0, 1, 0); break;
      case 2: targetDirection = float3(0, 0, 1); break;
      default: targetDirection = float3(0, 0, 1); break;
    }
    
    float dotProduct = dot(fromDirection, targetDirection);
    
    if (std::abs(dotProduct) > 0.999f)
    {
      float4x4 identity;
      identity.identity();
      return identity;
    }
    
    float3 rotationAxis = normalize(cross(fromDirection, targetDirection));
    float angle = std::acos(std::abs(dotProduct));
    
    return createRotationMatrix(rotationAxis, -angle);
  }

  float4x4 createConstrainedRotation(const float3& fromDirection, int toAxis, int constraintAxis, float tolerance)
  {
    float3 targetDirection = getAxisDirection(toAxis);
    float3 constraintDir = getAxisDirection(constraintAxis);
    
    float3 projectedFrom = fromDirection - constraintDir * dot(fromDirection, constraintDir);
    projectedFrom = normalize(projectedFrom);
    
    float3 projectedTo = targetDirection - constraintDir * dot(targetDirection, constraintDir);
    projectedTo = normalize(projectedTo);
    
    float dotProduct = dot(projectedFrom, projectedTo);
    
    if (std::abs(dotProduct) > 0.999f)
    {
        float4x4 identity;
        identity.identity();
        return identity;
    }
    
    float3 rotationAxis = constraintDir;
    
    float angle = std::acos(clamp(dotProduct, -1.0f, 1.0f));
    
    float3 cross_product = cross(projectedFrom, projectedTo);
    if (dot(cross_product, constraintDir) < 0)
    {
      angle = -angle;
    }
    
    return createRotationMatrix(rotationAxis, angle);
  }

  std::vector<NormalCluster> filterAlignedClusters(const std::vector<NormalCluster>& clusters,
                                                   const std::vector<AlignedAxis>& alignedAxes,
                                                   float tolerance)
  {
    std::vector<NormalCluster> filtered;
    
    for (const auto& cluster : clusters)
    {
      bool isAligned = false;
      
      for (const auto& aligned : alignedAxes)
      {
        float similarity = std::abs(dot(cluster.direction, aligned.direction));
        if (similarity > tolerance)
        {
          isAligned = true;
          break;
        }
      }
        
      if (!isAligned)
      {
        filtered.push_back(cluster);
      }
    }
    
    return filtered;
  }

  void performInitialAlignment(SimpleMesh& mesh, 
                               const std::vector<float3>& normals,
                               const std::vector<float>& areas,
                               std::vector<AlignedAxis>& alignedAxes,
                               float tolerance)
  {
    
    auto clusters = clusterNormalsByDirection(normals, areas, tolerance);
    auto maxCluster = findMaxAreaCluster(clusters);
    
    if (maxCluster.totalArea == 0) return;
    
    int bestAxis = findBestAlignmentAxis(maxCluster.direction);
    float4x4 rotation = createOptimalRotation(maxCluster.direction, bestAxis, tolerance);
    
    transform_mesh(mesh, rotation);
    
    AlignedAxis aligned;
    aligned.axis = bestAxis;
    aligned.direction = getAxisDirection(bestAxis);
    aligned.totalArea = maxCluster.totalArea;
    alignedAxes.push_back(aligned);
  }

  void performSecondaryAlignment(SimpleMesh& mesh,
                                 const std::vector<float3>& normals,
                                 const std::vector<float>& areas,
                                 std::vector<AlignedAxis>& alignedAxes,
                                 float tolerance)
  {
    auto clusters = clusterNormalsByDirection(normals, areas, tolerance);
    auto validClusters = filterAlignedClusters(clusters, alignedAxes, tolerance);
    
    if (validClusters.empty()) return;
    
    auto maxCluster = findMaxAreaCluster(validClusters);
    int bestAxis = findBestAlignmentAxis(maxCluster.direction);
    
    if (bestAxis == alignedAxes[0].axis)
    {
      bestAxis = findAlternativeAxis(maxCluster.direction, alignedAxes[0].axis);
    }
    
    float4x4 rotation = createConstrainedRotation(maxCluster.direction, bestAxis, alignedAxes[0].axis, tolerance);
    
    transform_mesh(mesh, rotation);
    
    AlignedAxis aligned;
    aligned.axis = bestAxis;
    aligned.direction = getAxisDirection(bestAxis);
    aligned.totalArea = maxCluster.totalArea;
    alignedAxes.push_back(aligned);
  }

  void align_mesh_to_axes(SimpleMesh& mesh, float tolerance)
  {
    if (mesh.TrianglesNum() == 0) return;
    
    std::vector<float3> normals;
    std::vector<float> areas;
    std::vector<AlignedAxis> alignedAxes;
    
    computeTriangleNormalsAndAreas(mesh, normals, areas);
    performInitialAlignment(mesh, normals, areas, alignedAxes, tolerance);
    
    if (alignedAxes.size() == 1) {
        computeTriangleNormalsAndAreas(mesh, normals, areas);
        performSecondaryAlignment(mesh, normals, areas, alignedAxes, tolerance);
    }
  }

  struct TinyObjIndexEqual
  {
    bool operator()(const tinyobj::index_t &lhs, const tinyobj::index_t &rhs) const
    {
      return lhs.vertex_index == rhs.vertex_index &&
            lhs.normal_index == rhs.normal_index &&
            lhs.texcoord_index == rhs.texcoord_index;
    }
  };

  struct TinyObjIndexHasher
  {
    size_t operator()(const tinyobj::index_t &index) const
    {
      return ((std::hash<int>()(index.vertex_index) ^
              (std::hash<int>()(index.normal_index) << 1)) >> 1) ^
              (std::hash<int>()(index.texcoord_index) << 1);
    }
  };
  void load_mesh_array_from_obj(const char* a_fileName, std::vector<SimpleMesh>& meshes, 
                                std::vector<std::string> *mesh_names, bool aVerbose)
  {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    
    std::string warn;
    std::string err;

    bool loading_result = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, a_fileName, nullptr);

    if (!loading_result)
    {
      printf("[load_mesh_array_from_obj::ERROR] Failed to load obj file: %s\n", err.c_str());
      return;
    }

    if (aVerbose)
    {
      if (warn.empty())
        printf("[load_mesh_array_from_obj::INFO] Loaded obj file: %s\n", a_fileName);
      else
        printf("[load_mesh_array_from_obj::WARNING] Loaded obj file %s with warnings: %s\n", a_fileName, warn.c_str());
    }

    const LiteMath::float4 default_norm = float4(0, 0, 1, 0);
    const LiteMath::float4 default_tangent = float4(1, 0, 0, 0);
    const LiteMath::float2 default_texcoord = float2(0, 0);

    for (const auto& shape : shapes)
    {
      std::unordered_map<tinyobj::index_t, uint32_t, TinyObjIndexHasher, TinyObjIndexEqual> uniqueVertIndices = {};

      meshes.emplace_back();
      SimpleMesh &mesh = meshes.back();

      if (mesh_names)
      {
        mesh_names->push_back(shape.name);
      }

      int sz = shape.mesh.indices.size();
      mesh.vPos4f.reserve(sz);
      mesh.vNorm4f.reserve(sz);
      mesh.vTang4f.reserve(sz);
      mesh.vTexCoord2f.reserve(sz);
      mesh.indices.reserve(sz);
      mesh.matIndices.insert(std::end(mesh.matIndices), std::begin(shape.mesh.material_ids), std::end(shape.mesh.material_ids));

      for (const auto& index : shape.mesh.indices)
      {
        uint32_t my_index = 0;
        auto it = uniqueVertIndices.find(index);
        if (it != uniqueVertIndices.end())
        {
          my_index = it->second;
        }
        else
        {
          uniqueVertIndices.insert({index, static_cast<uint32_t>(mesh.vPos4f.size())});
          my_index = static_cast<uint32_t>(mesh.vPos4f.size());

          assert(index.vertex_index >= 0 && index.vertex_index < attrib.vertices.size() / 3);
          mesh.vPos4f.push_back({attrib.vertices[3 * index.vertex_index + 0],
                                 attrib.vertices[3 * index.vertex_index + 1],
                                 attrib.vertices[3 * index.vertex_index + 2],
                                 1.0f});
          if(index.normal_index >= 0)
          {
            mesh.vNorm4f.push_back({attrib.normals[3 * index.normal_index + 0],
                                    attrib.normals[3 * index.normal_index + 1],
                                    attrib.normals[3 * index.normal_index + 2],
                                    0.0f});
          }
          else
          {
            mesh.vNorm4f.push_back(default_norm);
          }
          if(index.texcoord_index >= 0)
          {
            mesh.vTexCoord2f.push_back({attrib.texcoords[2 * index.texcoord_index + 0],
                                        attrib.texcoords[2 * index.texcoord_index + 1]});
          }
          else
          {
            mesh.vTexCoord2f.push_back(default_texcoord);
          }
          mesh.vTang4f.push_back(default_tangent);
        }

        mesh.indices.push_back(my_index);
      }

      mesh.vPos4f.shrink_to_fit();
      mesh.vNorm4f.shrink_to_fit();
      mesh.vTang4f.shrink_to_fit();
      mesh.vTexCoord2f.shrink_to_fit();
      mesh.indices.shrink_to_fit();
      mesh.matIndices.shrink_to_fit();
      
      // fix material id
      for (unsigned &mid : mesh.matIndices) 
      { 
        if(mid == uint32_t(-1)) 
          mid = 0;
      }
      
      if (aVerbose)
      {
        printf("[load_mesh_array_from_obj::INFO] Loaded mesh with %d vertices and %d indices\n", 
               (unsigned)mesh.vPos4f.size(), (unsigned)mesh.indices.size());
      }
    } //end for
  }

  struct edge
  {
    edge() = default;
    edge(unsigned _a, unsigned _b) {a = std::min(_a, _b); b = std::max(_a, _b); }
    unsigned a;
    unsigned b;
  };
  bool operator==(const edge &e1, const edge &e2)
  {
    return (e1.a == e2.a && e1.b == e2.b) ||
        (e1.a == e2.b && e1.b == e2.a);
  }
  struct edge_hash
  {
    std::size_t operator()(const edge &e) const
    {
      return std::hash<unsigned>()(e.a) ^ (std::hash<unsigned>()(e.b) << 1);
    }
  };
  struct edge_cmp {
    bool operator()(const edge& a, const edge& b) const 
    {
      if (a.a < b.a)
        return true;
      return a.b < b.b;
      
    }
  };

  std::vector<SimpleMesh> separate_components(const SimpleMesh &mesh)
  {
    std::vector<unsigned> components_map;
    components_map.resize(mesh.TrianglesNum(), 0);

    const auto &ids = mesh.indices;
    auto t1 = std::chrono::steady_clock::now();
    std::unordered_map<edge, std::vector<unsigned>, edge_hash> neighbors;
    for (unsigned i = 0; i < mesh.TrianglesNum(); ++i)
    {
      edge edges[3] = {edge(ids[3 * i], ids[3 * i + 1]), edge(ids[3 * i], ids[3 * i + 2]), edge(ids[3 * i + 1], ids[3 * i + 2])};
      for (int j = 0; j < 3; j++)
      {
        auto it = neighbors.find(edges[j]);
        if (it == neighbors.end())
          neighbors[edges[j]] = {i};
        else
          it->second.push_back(i);
      }
    }
    auto t2 = std::chrono::steady_clock::now();
    float time_ms = std::chrono::duration<float, std::milli>(t2 - t1).count();
    printf("edge map created in %f ms\n", time_ms);

    unsigned i = 0;
    unsigned curr_component = 1;
    std::vector<std::pair<unsigned, unsigned>> stack;
    std::vector<SimpleMesh> meshes;

    while (i < components_map.size())
    {
      while (i < components_map.size() && components_map[i] != 0)
        i++;
      if (i >= components_map.size())
        break;

      components_map[i] = curr_component;
      unsigned j = 0;
      stack.push_back({i, j++});

      SimpleMesh meshComp;
      meshComp.indices.push_back(0); // add the first triangle of the component
      meshComp.indices.push_back(1);
      meshComp.indices.push_back(2);
      meshComp.vPos4f.push_back(mesh.vPos4f[ids[3 * i]]);
      meshComp.vPos4f.push_back(mesh.vPos4f[ids[3 * i + 1]]);
      meshComp.vPos4f.push_back(mesh.vPos4f[ids[3 * i + 2]]);
      meshComp.vNorm4f.push_back(mesh.vNorm4f[ids[3 * i]]);
      meshComp.vNorm4f.push_back(mesh.vNorm4f[ids[3 * i + 1]]);
      meshComp.vNorm4f.push_back(mesh.vNorm4f[ids[3 * i + 2]]);
      meshComp.vTang4f.push_back(mesh.vTang4f[ids[3 * i]]);
      meshComp.vTang4f.push_back(mesh.vTang4f[ids[3 * i + 1]]);
      meshComp.vTang4f.push_back(mesh.vTang4f[ids[3 * i + 2]]);
      meshComp.vTexCoord2f.push_back(mesh.vTexCoord2f[ids[3 * i]]);
      meshComp.vTexCoord2f.push_back(mesh.vTexCoord2f[ids[3 * i + 1]]);
      meshComp.vTexCoord2f.push_back(mesh.vTexCoord2f[ids[3 * i + 2]]);
      meshComp.matIndices.push_back(mesh.matIndices[i]);
      unsigned next_vert_num = 3;

      while (!stack.empty())
      {
        auto [idx, comp_idx] = stack.back();
        stack.pop_back();

        for (int e = 0; e < 3; ++e) // iterate over edges of triangle
        {
          unsigned vert1 = ids[3 * idx + e];
          unsigned vert2 = ids[3 * idx + (e + 1) % 3];
          unsigned comp_vert1 = meshComp.indices[3 * comp_idx + e];
          unsigned comp_vert2 = meshComp.indices[3 * comp_idx + (e + 1) % 3];
          auto edge_range = neighbors[edge(vert1, vert2)];
          for (auto it : edge_range) // iterate over triangles adjacent to the edge
          {
            if (components_map[it] != 0)
              continue;

            for (int c = 0; c < 3; ++c) // choose how to add each corner of triangle
            {
              unsigned new_vert = ids[3 * (it) + c];
              if (new_vert == vert1)
              {
                meshComp.indices.push_back(comp_vert1);
              }
              else if (new_vert == vert2)
              {
                meshComp.indices.push_back(comp_vert2);
              }
              else
              {
                bool found = false;
                for (auto &[a, b] : stack)
                {
                  if (ids[3 * a] == new_vert)
                  {
                    meshComp.indices.push_back(meshComp.indices[3 * b]);
                    found = true;
                    break;
                  }
                  else if (ids[3 * a + 1] == new_vert)
                  {
                    meshComp.indices.push_back(meshComp.indices[3 * b + 1]);
                    found = true;
                    break;
                  }
                  else if (ids[3 * a + 2] == new_vert)
                  {
                    meshComp.indices.push_back(meshComp.indices[3 * b + 2]);
                    found = true;
                    break;
                  }
                }
                if (!found)
                {
                  meshComp.indices.push_back(next_vert_num++);
                  meshComp.vPos4f.push_back(mesh.vPos4f[new_vert]);
                  meshComp.vTang4f.push_back(mesh.vTang4f[new_vert]);
                  meshComp.vNorm4f.push_back(mesh.vNorm4f[new_vert]);
                  meshComp.vTexCoord2f.push_back(mesh.vTexCoord2f[new_vert]);
                }
              }
            }
            meshComp.matIndices.push_back(mesh.matIndices[it]);

            components_map[it] = curr_component;
            stack.push_back({it, j++});
          }
        }
      }
      meshes.push_back(meshComp);
      curr_component++;
    }
    return meshes;
  }

  SimpleMesh preprocess_mesh(const SimpleMesh &in_mesh, PreprocessSettings settings, float4x4 *out_transform)
  {
    LiteMath::float4x4 t1, t2;
    SimpleMesh mesh = in_mesh;
    fix_missing(mesh);
    assert(check_is_valid(mesh, settings.verbose));
    
    if (settings.align_to_axes)
      align_mesh_to_axes(mesh, settings.align_to_axes_tolerance);
    if (settings.rescale)
      t1 = rescale_mesh(mesh, settings.min_p, settings.max_p);
    if (settings.compress_close_vertices)
      compress_close_vertices(mesh, settings.close_vertices_thr, settings.force_merge_distinct_normals, settings.verbose);
    if (settings.recalculate_normals)
      recalculate_vertex_normals(mesh);

    if (out_transform)
      *out_transform = t1*t2;
    
    return mesh;
  }

  std::vector<SimpleMesh> preprocess_mesh_parts(const std::vector<SimpleMesh> &orig_meshes, PreprocessSettings settings, 
                                                bool recreate_parts, float4x4 *out_transform)
  {
    SimpleMesh combined_mesh = MergeMeshes(orig_meshes);
    float4x4 transform;
    combined_mesh = preprocess_mesh(combined_mesh, settings, &transform);
    
    std::vector<SimpleMesh> meshes;
    if (recreate_parts)
    {
      auto t1 = std::chrono::steady_clock::now();
      meshes = separate_components(combined_mesh);
      auto t2 = std::chrono::steady_clock::now();
      float time_ms = std::chrono::duration<float, std::milli>(t2 - t1).count();
      printf("components: %d ---> %d, took %f ms\n", (int)orig_meshes.size(), (int)meshes.size(), time_ms);
    }
    else
    {
      meshes = orig_meshes;
    }

    for (int i=0;i<meshes.size();i++)
    {
      PreprocessSettings ps = settings;
      ps.align_to_axes = false;
      ps.rescale = false;
      transform_mesh(meshes[i], transform);
      meshes[i] = preprocess_mesh(meshes[i], ps);
    }

    return meshes;
  }
}