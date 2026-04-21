#include "point_cloud_process.h"
#include "utils/common/rand.h"
#include <thread>
#include <cstdlib>
#include <ctime>

namespace point_cloud
{

  inline unsigned int& get_thread_seed_storage()
  {
    thread_local unsigned int seed = [](){
      return static_cast<unsigned int>(
        std::hash<std::thread::id>{}(std::this_thread::get_id()) ^
        (static_cast<unsigned int>(clock()) * 2654435769u)
      );
    }();
    return seed;
  }

  inline float randomFloat01()
  {
    unsigned int& seed = get_thread_seed_storage();
    return static_cast<float>(irand_r(&seed)) / static_cast<float>(RAND_MAX);
  }

  inline int randomInt(int max)
  {
    unsigned int& seed = get_thread_seed_storage();
    return irand_r(&seed) % max;
  }

  struct VoxelBounds
  {
    LiteMath::float3 min_corner;
    LiteMath::float3 max_corner;
  };

  struct SurfacePoint
  {
    LiteMath::float3 position;
    float distance;
    LiteMath::float3 normal;

    SurfacePoint() {}
    
    SurfacePoint(const LiteMath::float3& pos, float dist) : position(pos), distance(dist)
    {
      normal = LiteMath::float3(0, 0, 0);
    }
  };

  struct TrilinearCoefficients
  {
    float A, B, C, D, E, F, G, H;
    
    TrilinearCoefficients() : A(0), B(0), C(0), D(0), E(0), F(0), G(0), H(0) {}
    
    float evaluate(float x, float y, float z) const
    {
      return A * (1-x) * (1-y) * (1-z) +
             B * (1-x) * (1-y) * z +
             C * (1-x) * y * (1-z) +
             D * (1-x) * y * z +
             E * x * (1-y) * (1-z) +
             F * x * (1-y) * z +
             G * x * y * (1-z) +
             H * x * y * z;
    }
    void coeff_gradient(float *grad, float x, float y, float z)
    {
      grad[0] = (1-x) * (1-y) * (1-z);
      grad[1] = (1-x) * (1-y) * z;
      grad[2] = (1-x) * y * (1-z);
      grad[3] = (1-x) * y * z;
      grad[4] = x * (1-y) * (1-z);
      grad[5] = x * (1-y) * z;
      grad[6] = x * y * (1-z);
      grad[7] = x * y * z;
    }
  };

  LiteMath::float3 computeTrilinearGradient(const TrilinearCoefficients& coeffs, float x, float y, float z)
  {
    float df_dx = -coeffs.A * (1-y) * (1-z) +
                  -coeffs.B * (1-y) * z +
                  -coeffs.C * y * (1-z) +
                  -coeffs.D * y * z +
                   coeffs.E * (1-y) * (1-z) +
                   coeffs.F * (1-y) * z +
                   coeffs.G * y * (1-z) +
                   coeffs.H * y * z;
    
    float df_dy = -coeffs.A * (1-x) * (1-z) +
                  -coeffs.B * (1-x) * z +
                   coeffs.C * (1-x) * (1-z) +
                   coeffs.D * (1-x) * z +
                  -coeffs.E * x * (1-z) +
                  -coeffs.F * x * z +
                   coeffs.G * x * (1-z) +
                   coeffs.H * x * z;
    
    float df_dz = -coeffs.A * (1-x) * (1-y) +
                   coeffs.B * (1-x) * (1-y) +
                  -coeffs.C * (1-x) * y +
                   coeffs.D * (1-x) * y +
                  -coeffs.E * x * (1-y) +
                   coeffs.F * x * (1-y) +
                  -coeffs.G * x * y +
                   coeffs.H * x * y;
    
    return LiteMath::float3(df_dx, df_dy, df_dz);
  }

  LiteMath::float3 worldToVoxel(const LiteMath::float3& world_pos, const VoxelBounds& bounds)
  {
    return (world_pos - bounds.min_corner) / (bounds.max_corner - bounds.min_corner);
  }

  LiteMath::float3 sampleTriangle(const LiteMath::float3& a, 
                                  const LiteMath::float3& b, 
                                  const LiteMath::float3& c)
  {
    float r1 = randomFloat01();
    float r2 = randomFloat01();
    
    if (r1 + r2 > 1.0f)
    {
      r1 = 1.0f - r1;
      r2 = 1.0f - r2;
    }
    
    return a + r1 * (b - a) + r2 * (c - a);
  }

  LiteMath::float3 sampleBox(const VoxelBounds& voxel_bounds)
  {
    float r1 = randomFloat01();
    float r2 = randomFloat01();
    float r3 = randomFloat01();

    float3 dif = voxel_bounds.max_corner - voxel_bounds.min_corner;
    dif.x = dif.x * r1;
    dif.y = dif.y * r2;
    dif.z = dif.z * r3;
    
    return voxel_bounds.min_corner + dif;
  }

  LiteMath::float3 sampleBoundBox(const VoxelBounds& voxel_bounds)
  {
    int r1 = randomInt(6);
    float r2 = randomFloat01();
    float r3 = randomFloat01();
    float3 dif = voxel_bounds.max_corner - voxel_bounds.min_corner;
    if (r1 / 2 == 0)
    {
      dif.x *= (r1 % 2);
      dif.y = dif.y * r2;
      dif.z = dif.z * r3;
    }
    else if (r1 / 2 == 1)
    {
      dif.y *= (r1 % 2);
      dif.x = dif.x * r2;
      dif.z = dif.z * r3;
    }
    else
    {
      dif.z *= (r1 % 2);
      dif.y = dif.y * r2;
      dif.x = dif.x * r3;
    }
    
    return voxel_bounds.min_corner + dif;
  }

  LiteMath::float3 sampleSegment(const LiteMath::float3& a, 
                                  const LiteMath::float3& b)
  {
    float r1 = randomFloat01();
    
    return a + r1 * (b - a);
  }

  std::vector<LiteMath::float3> clipTriangleByVoxel(
        const LiteMath::float3& a, const LiteMath::float3& b, const LiteMath::float3& c,
        const VoxelBounds& voxel_bounds)
  {
    
    std::vector<LiteMath::float3> polygon = {a, b, c};
    std::vector<LiteMath::float3> next_polygon = {};
      
    for (int axis = 0; axis < 3; ++axis)
    {
      for (int face = 0; face < 2; ++face)
      {
        float plane_coord = (face == 0) ? voxel_bounds.min_corner[axis] : voxel_bounds.max_corner[axis];
        int face_sign_comp = face * 2 - 1;

        for (int i = 0; i < polygon.size(); ++i)
        {
          int j = (i + 1) % polygon.size();
          bool add_clip = polygon[j][axis] * face_sign_comp > plane_coord * face_sign_comp;;
          if (polygon[i][axis] * face_sign_comp <= plane_coord * face_sign_comp)
          {
            next_polygon.push_back(polygon[i]);
          }
          else
          {
            add_clip = !add_clip;
          }
          if (add_clip)
          {
            LiteMath::float3 dir = polygon[j] - polygon[i];
            LiteMath::float3 p;
            if (std::abs(dir[axis]) > 1e-6)
            {
              p = polygon[i] + dir * (plane_coord - polygon[i][axis]) / dir[axis];
            }
            else
            {
              p = polygon[j];
            }
            next_polygon.push_back(p);
          }
        }
        polygon = next_polygon;
        next_polygon.clear();
        //printf("\n");
      }
    }
    
    return polygon;
  }

  std::vector<LiteMath::float3> getTriangleVoxelIntersections(
        const LiteMath::float3& a, const LiteMath::float3& b, const LiteMath::float3& c,
        const VoxelBounds& voxel_bounds)
  {
        
    std::vector<LiteMath::float3> intersections;
    
    std::vector<LiteMath::float3> triangle_edges[] = {
        {a, b}, {b, c}, {c, a}
    };
      
    for (int axis = 0; axis < 3; ++axis)
    {
      for (int face = 0; face < 2; ++face)
      {
        float plane_coord = (face == 0) ? voxel_bounds.min_corner[axis] : voxel_bounds.max_corner[axis];

        bool in_bounds = false;
        int last_size = intersections.size();
        for (const auto& edge : triangle_edges)
        {
          LiteMath::float3 start = edge[0];
          LiteMath::float3 end = edge[1];
          LiteMath::float3 dir = end - start;
          
          if (std::abs(dir[axis]) > 1e-6f)
          {
            float t = (plane_coord - start[axis]) / dir[axis];
            //printf("t: %f\n", t);
            if (t >= -1e-6f && t - 1.0f <= 1e-6f)
            {
              //printf("approved\n");
              in_bounds = true;
              LiteMath::float3 intersection = start + t * dir;
              intersections.push_back(intersection);
              /*if (intersection.x >= voxel_bounds.min_corner.x && 
                  intersection.x <= voxel_bounds.max_corner.x &&
                  intersection.y >= voxel_bounds.min_corner.y && 
                  intersection.y <= voxel_bounds.max_corner.y &&
                  intersection.z >= voxel_bounds.min_corner.z && 
                  intersection.z <= voxel_bounds.max_corner.z)
              {
                intersections.push_back(intersection);
              }*/
            }
          }
        }
        //printf("\n");
        if (in_bounds)
        {
          if (intersections.size() - last_size == 2)
          {
            LiteMath::float3 direct = intersections[intersections.size() - 2] - intersections[intersections.size() - 1];
            float t_enter = 0.0f, t_exit = 1.0f;
            if (std::abs(direct.x) > 1e-6f)
            {
              float t1 = (voxel_bounds.min_corner.x - intersections[intersections.size() - 1].x) / direct.x;
              float t2 = (voxel_bounds.max_corner.x - intersections[intersections.size() - 1].x) / direct.x;
              if (t1 > t2) 
              {
                float tmp = t1;
                t1 = t2;
                t2 = tmp;
              }
              t_enter = std::max(t_enter, t1);
              t_exit = std::min(t_exit, t2);
            }
            if (std::abs(direct.y) > 1e-6f)
            {
              float t1 = (voxel_bounds.min_corner.y - intersections[intersections.size() - 1].y) / direct.y;
              float t2 = (voxel_bounds.max_corner.y - intersections[intersections.size() - 1].y) / direct.y;
              if (t1 > t2) 
              {
                float tmp = t1;
                t1 = t2;
                t2 = tmp;
              }
              t_enter = std::max(t_enter, t1);
              t_exit = std::min(t_exit, t2);
            }
            if (std::abs(direct.z) > 1e-6f)
            {
              float t1 = (voxel_bounds.min_corner.z - intersections[intersections.size() - 1].z) / direct.z;
              float t2 = (voxel_bounds.max_corner.z - intersections[intersections.size() - 1].z) / direct.z;
              if (t1 > t2) 
              {
                float tmp = t1;
                t1 = t2;
                t2 = tmp;
              }
              t_enter = std::max(t_enter, t1);
              t_exit = std::min(t_exit, t2);
            }
            if (t_enter < t_exit)
            {
              intersections[intersections.size() - 2] = intersections[intersections.size() - 1] + direct * t_exit;
              intersections[intersections.size() - 1] = intersections[intersections.size() - 1] + direct * t_enter;
            }
            else
            {
              intersections.resize(last_size);
            }
          }
          else
          {
            //printf("WARNING IN BORDER SEARCH\n");
            //printf("DIFF IS %d\n", (int)(intersections.size() - last_size));
            intersections.resize(last_size);
          }
        }
      }
    }
    
    return intersections;
  }

  std::vector<SurfacePoint> generatePointCloud(
        const cmesh4::SimpleMesh& mesh,
        const uint32_t* triangle_ids,
        uint32_t triangle_count,
        const VoxelBounds& voxel_bounds,
        sdf_converter::MeshDistFunc &dist_func,
        int points_count = 100,
        float boundary_points_multiplier = 1)
  {
    std::vector<SurfacePoint> point_cloud;
    //SurfacePoint sp(voxel_bounds.min_corner, 0.1f);
    //point_cloud.push_back(sp);
    //printf("tri_count: %d\n", (int)triangle_count);
    float full_square = 0.0f;
    for (uint32_t tri_idx = 0; tri_idx < triangle_count; ++tri_idx)
    {
      uint32_t t_i = triangle_ids[tri_idx];
      
      LiteMath::float3 a = to_float3(mesh.vPos4f[mesh.indices[3*t_i+0]]);
      LiteMath::float3 b = to_float3(mesh.vPos4f[mesh.indices[3*t_i+1]]);
      LiteMath::float3 c = to_float3(mesh.vPos4f[mesh.indices[3*t_i+2]]);

      full_square += LiteMath::length(LiteMath::cross(b - a, c - a));
    }

    for (int i = 0; i < points_count / 2; ++i)
    {
      LiteMath::float3 pos = sampleBox(voxel_bounds);
      // dist_func.calculate(mesh, triangle_ids, triangle_count, pos).distance;
      SurfacePoint sp(pos, dist_func.calculate(mesh, triangle_ids, triangle_count, pos).distance);
      //printf("real num: %d\n", i);
      //printf("sp: %f %f %f\n", pos.x, pos.y, pos.z);
      point_cloud.push_back(sp);
    }

    for (int i = 0; i < points_count / 2; ++i)
    {
      LiteMath::float3 pos = sampleBoundBox(voxel_bounds);
      // dist_func.calculate(mesh, triangle_ids, triangle_count, pos).distance;
      SurfacePoint sp(pos, dist_func.calculate(mesh, triangle_ids, triangle_count, pos).distance);
      //printf("real num: %d\n", i);
      //printf("sp: %f %f %f\n", pos.x, pos.y, pos.z);
      point_cloud.push_back(sp);
    }
    
    for (uint32_t tri_idx = 0; tri_idx < triangle_count; ++tri_idx)
    {
      uint32_t t_i = triangle_ids[tri_idx];
      
      LiteMath::float3 a1 = to_float3(mesh.vPos4f[mesh.indices[3*t_i+0]]);
      LiteMath::float3 b1 = to_float3(mesh.vPos4f[mesh.indices[3*t_i+1]]);
      LiteMath::float3 c1 = to_float3(mesh.vPos4f[mesh.indices[3*t_i+2]]);

      std::vector<LiteMath::float3> boundary_intersections = 
            getTriangleVoxelIntersections(a1, b1, c1, voxel_bounds);

      int points_per_triangle = (LiteMath::length(LiteMath::cross(b1 - a1, c1 - a1)) / full_square) * points_count + 1;
      LiteMath::float3 normal = LiteMath::normalize(LiteMath::cross(b1 - a1, c1 - a1));
        
      int boundary_samples = points_per_triangle * boundary_points_multiplier;
      for (int i = 0; i < boundary_intersections.size() / 2; ++i)
      {
        for (int j = 0; j < boundary_samples; ++j)
        {
          LiteMath::float3 sample_pos = sampleSegment(boundary_intersections[i * 2], boundary_intersections[i * 2 + 1]);
          SurfacePoint sp(sample_pos, 0.0f);
          //printf("bound num: %d %d\n", i, j);
          //printf("sp: %f %f %f\n", sample_pos.x, sample_pos.y, sample_pos.z);
          sp.normal = normal;
          point_cloud.push_back(sp);
        }
      }
      // printf("L\n");
      std::vector<LiteMath::float3> poly = clipTriangleByVoxel(a1, b1, c1, voxel_bounds);
      // printf("%d X\n", (int)poly.size());
      // printf("%f %f %f\n", a1.x, a1.y, a1.z);
      // printf("%f %f %f\n", b1.x, b1.y, b1.z);
      // printf("%f %f %f\n", c1.x, c1.y, c1.z);
      // printf("%f %f %f\n", voxel_bounds.min_corner.x, voxel_bounds.min_corner.y, voxel_bounds.min_corner.z);
      // printf("%f %f %f\n", voxel_bounds.max_corner.x, voxel_bounds.max_corner.y, voxel_bounds.max_corner.z);
      for (int poly_idx = 1; poly_idx < (int)poly.size() - 1; ++poly_idx)
      {
        LiteMath::float3 a = poly[0];
        LiteMath::float3 b = poly[poly_idx];
        LiteMath::float3 c = poly[poly_idx + 1];
        //printf("A: %f %f %f; B: %f %f %f; C: %f %f %f\n", a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z);
        
        normal = LiteMath::normalize(LiteMath::cross(b - a, c - a));
        points_per_triangle = (LiteMath::length(LiteMath::cross(b - a, c - a)) / full_square) * points_count + 1;
        //printf("div: %f %f %d\n", full_square, LiteMath::length(LiteMath::cross(b - a, c - a)), points_per_triangle);
        
        //for (int i = 0; i < points_per_triangle; ++i)
        //{
          //LiteMath::float3 sample_pos = sampleTriangle(a, b, c);
          
          /*if (truesample_pos.x >= voxel_bounds.min_corner.x && sample_pos.x <= voxel_bounds.max_corner.x &&
              sample_pos.y >= voxel_bounds.min_corner.y && sample_pos.y <= voxel_bounds.max_corner.y &&
              sample_pos.z >= voxel_bounds.min_corner.z && sample_pos.z <= voxel_bounds.max_corner.z*///)
          //{
            //printf("tri num: %d\n", i);
            //printf("sp: %f %f %f\n", sample_pos.x, sample_pos.y, sample_pos.z);
            //SurfacePoint sp(sample_pos, 0.0f);
            //sp.normal = normal;
            //point_cloud.push_back(sp);
          //}
        //}
      }
    }
    
    return point_cloud;
  }

  inline float pythag(float a, float b)
  {
    a = std::fabs(a); b = std::fabs(b);
    return (a > b) ? a * std::sqrt(1.f + (b / a) * (b / a))
                  : (b ? b * std::sqrt(1.f + (a / b) * (a / b)) : 0.f);
  }

  void bidiagonalize(std::vector<std::vector<float>> &A, std::vector<float> &d, std::vector<float> &e, 
                     std::vector<std::vector<float>> &U, std::vector<std::vector<float>> &V)
  {
    int n = A.size();
    
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            U[i][j] = V[i][j] = (i == j) ? 1.0f : 0.0f;
        }
        d[i] = e[i] = 0.0f;
    }
    
    for (int i = 0; i < n; ++i) {
        int l = i + 1;
        
        float scale = 0.0f;
        for (int k = i; k < n; ++k) {
            scale += std::fabs(A[k][i]);
        }
        
        if (scale > 1e-12f) {
            float s = 0.0f;
            for (int k = i; k < n; ++k) {
                A[k][i] /= scale;
                s += A[k][i] * A[k][i];
            }
            
            float f = A[i][i];
            float g = -std::copysign(std::sqrt(s), f);
            float h = f * g - s;
            A[i][i] = f - g;
            
            for (int j = l; j < n; ++j) {
                s = 0.0f;
                for (int k = i; k < n; ++k) {
                    s += A[k][i] * A[k][j];
                }
                f = s / h;
                for (int k = i; k < n; ++k) {
                    A[k][j] += f * A[k][i];
                }
            }
            
            for (int j = i; j < n; ++j) {
                s = 0.0f;
                for (int k = i; k < n; ++k) {
                    s += A[k][i] * U[k][j];
                }
                f = s / h;
                for (int k = i; k < n; ++k) {
                    U[k][j] += f * A[k][i];
                }
            }
            
            for (int k = i; k < n; ++k) {
                A[k][i] *= scale;
            }
        }
        
        d[i] = A[i][i];
        
        
        if (i < n - 1) {
            scale = 0.0f;
            for (int k = l; k < n; ++k) {
                scale += std::fabs(A[i][k]);
            }
            
            if (scale > 1e-12f) {
                float s = 0.0f;
                for (int k = l; k < n; ++k) {
                    A[i][k] /= scale;
                    s += A[i][k] * A[i][k];
                }
                
                float f = A[i][l];
                float g = -std::copysign(std::sqrt(s), f);
                float h = f * g - s;
                A[i][l] = f - g;
                
                for (int k = l; k < n; ++k) {
                    e[k] = A[i][k] / h;
                }
                
                for (int j = l; j < n; ++j) {
                    s = 0.0f;
                    for (int k = l; k < n; ++k) {
                        s += A[j][k] * A[i][k];
                    }
                    for (int k = l; k < n; ++k) {
                        A[j][k] += s * e[k];
                    }
                }
                
                for (int j = l; j < n; ++j) {
                    s = 0.0f;
                    for (int k = l; k < n; ++k) {
                        s += V[j][k] * A[i][k];
                    }
                    for (int k = l; k < n; ++k) {
                        V[j][k] += s * e[k];
                    }
                }
                
                for (int k = l; k < n; ++k) {
                    A[i][k] *= scale;
                }
            }
        }
        
        e[i] = (i < n - 1) ? A[i][i + 1] : 0.0f;
    }
  }

  void diagonalize(std::vector<float> &d, std::vector<float> &e, 
                   std::vector<std::vector<float>> &U, std::vector<std::vector<float>> &V)
  {
    int n = d.size();
    const int MAX_ITER = 50;
    const float EPS = 1e-12f;
    
    for (int k = n - 1; k >= 0; --k) {
        for (int iter = 0; iter < MAX_ITER; ++iter) {
            
            int m = k;
            while (m > 0 && std::fabs(e[m - 1]) > EPS) {
                m--;
            }
            
            if (m == k) break;
            
            bool zero_diag = false;
            for (int l = m; l <= k; ++l) {
                if (std::fabs(d[l]) <= EPS) {
                    zero_diag = true;
                    if (l < k) {
                        float c = 0.0f, s = 1.0f;
                        for (int i = l + 1; i <= k; ++i) {
                            float f = s * e[i - 1];
                            e[i - 1] = c * e[i - 1];
                            if (std::fabs(f) <= EPS) break;
                            float g = d[i];
                            d[i] = pythag(f, g);
                            if (d[i] > 0.0f) {
                                c = g / d[i];
                                s = -f / d[i];
                            }
                            for (int j = 0; j < n; ++j) {
                                float y = U[j][l], z = U[j][i];
                                U[j][l] = y * c + z * s;
                                U[j][i] = z * c - y * s;
                            }
                        }
                    }
                    break;
                }
            }
            
            if (zero_diag) continue;
            
            float x = d[m];
            float y = d[k - 1]; 
            float g = e[k - 1];
            float h = e[m];
            float f = ((y - d[k]) * (y + d[k]) + (g - h) * (g + h)) / (2.0f * h * y);
            g = pythag(f, 1.0f);
            f = ((x - d[k]) * (x + d[k]) + h * (y / (f + std::copysign(g, f)) - h)) / x;
            
            float c = 1.0f, s = 1.0f;
            for (int i = m; i < k; ++i) {
                g = e[i];
                y = d[i + 1];
                h = s * g;
                g = c * g;
                float z = pythag(f, h);
                e[i] = z;
                
                if (z > 0.0f) {
                    c = f / z;
                    s = h / z;
                }
                
                f = x * c + g * s;
                g = g * c - x * s;
                h = y * s;
                y *= c;
                
                for (int j = 0; j < n; ++j) {
                    x = V[j][i]; 
                    z = V[j][i + 1];
                    V[j][i] = x * c + z * s;
                    V[j][i + 1] = z * c - x * s;
                }
                
                z = pythag(f, h);
                d[i] = z;
                
                if (z > 0.0f) {
                    c = f / z;
                    s = h / z;
                }
                
                f = c * g + s * y;
                x = c * y - s * g;
                
                for (int j = 0; j < n; ++j) {
                    y = U[j][i]; 
                    z = U[j][i + 1];
                    U[j][i] = y * c + z * s;
                    U[j][i + 1] = z * c - y * s;
                }
            }
            
            e[k - 1] = f;
            d[k] = x;
        }
    }
    
    for (int i = 0; i < n - 1; ++i) {
        int max_idx = i;
        for (int j = i + 1; j < n; ++j) {
            if (std::fabs(d[j]) > std::fabs(d[max_idx])) {
                max_idx = j;
            }
        }
        
        if (max_idx != i) {
            std::swap(d[i], d[max_idx]);
            
            for (int j = 0; j < n; ++j) {
                std::swap(U[j][i], U[j][max_idx]);
                std::swap(V[j][i], V[j][max_idx]);
            }
        }
        
        if (d[i] < 0) {
            d[i] = -d[i];
            for (int j = 0; j < n; ++j) {
                U[j][i] = -U[j][i];
            }
        }
    }
  } 

  std::vector<float> solveSVD(const std::vector<std::vector<float>> &A, const std::vector<float> &b, float lambda = 1e-6f)
  {
    int n = A.size();
    if (n == 0 || A[0].size() != n || b.size() != n) {
        printf("ERROR: Invalid matrix dimensions\n");
        return std::vector<float>(n, 0.0f);
    }
    std::vector<std::vector<float>> B = A;
    std::vector<std::vector<float>> U(n, std::vector<float>(n, 0.0f));
    std::vector<std::vector<float>> V(n, std::vector<float>(n, 0.0f));
    std::vector<float> d(n), e(n);
    
    bidiagonalize(B, d, e, U, V);
    diagonalize(d, e, U, V);

    for (int i = 0; i < A.size(); ++i)
    {
      printf("A ");
      for (int j = 0; j < A.size(); ++j)
      {
        printf("%f ", A[i][j]);
      }
      printf("| %f\n", b[i]);
    }
    printf("\n");

    for (int i = 0; i < A.size(); ++i)
    {
      printf("UV ");
      for (int j = 0; j < A.size(); ++j)
      {
        printf("(%f | %f) ", U[i][j], V[i][j]);
      }
      printf("\n");
    }
    printf("\n");
    
    for (int i = 0; i < n; ++i) {
        printf("σ[%d] = %f , e[%d] = %f\n", i, d[i], i, e[i]);
    }
    printf("\n");
    
    std::vector<float> inv_sigma(n);
    for (int i = 0; i < n; ++i) {
        if (std::fabs(d[i]) > 1e-12f) {
            inv_sigma[i] = 1.0f / (d[i] + lambda * d[0]);
        } else {
            inv_sigma[i] = 0.0f;
        }
    }
    
    std::vector<float> Utb(n, 0.0f);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            Utb[i] += U[j][i] * b[j];
        }
    }
    
    std::vector<float> temp(n, 0.0f);
    for (int i = 0; i < n; ++i) {
        temp[i] = inv_sigma[i] * Utb[i];
    }
    
    std::vector<float> x(n, 0.0f);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            x[i] += V[i][j] * temp[j];
        }
    }
    
    return x;
  }

  std::vector<float> solveLinearSystemGaussian(
        std::vector<std::vector<float>>& A, 
        std::vector<float>& b)
  {
        
    int n = A.size();
    std::vector<float> x(n, 1);

    //printf("started\n");
    /*for (int i = 0; i < A.size(); ++i)
    {
      for (int j = 0; j < A[i].size(); ++j)
      {
        //printf("%f ", A[i][j]);
      }
      //printf("| %f\n", b[i]);
    }*/
    
    for (int i = 0; i < n; i++)
    {
      int max_row = i;
      for (int k = i + 1; k < n; k++)
      {
        if (std::abs(A[k][i]) > std::abs(A[max_row][i]))
        {
          max_row = k;
        }
      }
      std::swap(A[max_row], A[i]);
      std::swap(b[max_row], b[i]);
      
      if (std::abs(A[i][i]) < 1e-9)
      {
        continue;
      }
      
      for (int k = i + 1; k < n; k++)
      {
        float c = A[k][i] / A[i][i];
        for (int j = i; j < n; j++)
        {
          A[k][j] -= c * A[i][j];
        }
        b[k] -= c * b[i];
      }
    }

    //printf("ended\n");
    /*for (int i = 0; i < A.size(); ++i)
    {
      for (int j = 0; j < A[i].size(); ++j)
      {
        //printf("%f ", A[i][j]);
      }
      //printf("| %f\n", b[i]);
    }*/
    
    for (int i = n - 1; i >= 0; i--)
    {
      if (std::abs(A[i][i]) > 1e-9)
      {
        x[i] = b[i];
        for (int j = i + 1; j < n; j++)
        {
          x[i] -= A[i][j] * x[j];
        }
        x[i] /= A[i][i];
      }
    }

    //printf("result\n");
    /*for (int i = 0; i < x.size(); ++i)
    {
      //printf("%f ", x[i]);
    }*/
    //printf("\n");
    
    return x;
  }

  TrilinearCoefficients solveLinearSystem(
        const std::vector<SurfacePoint>& points,
        const VoxelBounds& voxel_bounds)
  {
        
    if (points.size() < 8)
    {
      return TrilinearCoefficients();
    }
    
    int n_points = points.size();
    std::vector<std::vector<float>> matrix(n_points, std::vector<float>(8));
    std::vector<float> rhs(n_points);
    
    for (int i = 0; i < n_points; ++i)
    {
      LiteMath::float3 voxel_pos = worldToVoxel(points[i].position, voxel_bounds);
      float x = voxel_pos.x, y = voxel_pos.y, z = voxel_pos.z;
      
      matrix[i][0] = (1-x) * (1-y) * (1-z); // A
      matrix[i][1] = (1-x) * (1-y) * z;     // B
      matrix[i][2] = (1-x) * y * (1-z);     // C
      matrix[i][3] = (1-x) * y * z;         // D
      matrix[i][4] = x * (1-y) * (1-z);     // E
      matrix[i][5] = x * (1-y) * z;         // F
      matrix[i][6] = x * y * (1-z);         // G
      matrix[i][7] = x * y * z;             // H
      
      rhs[i] = points[i].distance;
    }
    
    std::vector<std::vector<float>> AtA(8, std::vector<float>(8, 0));
    std::vector<float> Atb(8, 0);
    
    if (matrix.size() == 8)
    {
      AtA = matrix;
      Atb = rhs;
    }
    else
    {
      for (int i = 0; i < 8; ++i)
      {
        for (int j = 0; j < 8; ++j)
        {
          for (int k = 0; k < n_points; ++k)
          {
            AtA[i][j] += matrix[k][i] * matrix[k][j];
          }
        }
        for (int k = 0; k < n_points; ++k)
        {
          Atb[i] += matrix[k][i] * rhs[k];
        }
      }
    }

    std::vector<float> coeffs = solveLinearSystemGaussian(AtA, Atb);
    
    TrilinearCoefficients result;
    if (coeffs.size() == 8)
    {
      result.A = coeffs[0];
      result.B = coeffs[1];
      result.C = coeffs[2];
      result.D = coeffs[3];
      result.E = coeffs[4];
      result.F = coeffs[5];
      result.G = coeffs[6];
      result.H = coeffs[7];
    }
    
    return result;
  }

  TrilinearCoefficients ransacFit(
        const std::vector<SurfacePoint>& points,
        const VoxelBounds& voxel_bounds,
        int max_iterations = 1000,
        float inlier_threshold = 0.001f,
        float min_iters = 0.1f,
        float enough_points = 0.85f,
        bool is_first_point_necessary = false)
  {
    if (points.size() < 8)
    {
      return TrilinearCoefficients();
    }
    
    TrilinearCoefficients best_coeffs;
    int best_inliers = 0;

    // for (int i = 0; i < points.size(); ++i)
    // {
    //   printf("%d (%f %f %f | %f)\n", i, points[i].position.x, points[i].position.y, points[i].position.z, points[i].distance);
    // }

    inlier_threshold = LiteMath::length(voxel_bounds.max_corner - voxel_bounds.min_corner) / 25.0f;
    
    for (int iter = 0; iter < max_iterations; ++iter)
    {
      std::vector<SurfacePoint> sample_points;
      std::vector<int> selected_indices;
      if (is_first_point_necessary) sample_points.push_back(points[0]);
      
      while (sample_points.size() < 8)
      {
        int idx = randomInt(points.size());
        if (std::find(selected_indices.begin(), selected_indices.end(), idx) == selected_indices.end())
        {
          sample_points.push_back(points[idx]);
          selected_indices.push_back(idx);
        }
      }
      
      TrilinearCoefficients coeffs = solveLinearSystem(sample_points, voxel_bounds);
      
      int inlier_count = 0;
      for (const auto& point : points)
      {
        LiteMath::float3 voxel_pos = worldToVoxel(point.position, voxel_bounds);
        float predicted_distance = coeffs.evaluate(voxel_pos.x, voxel_pos.y, voxel_pos.z);
        
        if (std::abs(predicted_distance - point.distance) < inlier_threshold)
        {
          inlier_count++;
        }
      }
      
      if (inlier_count > best_inliers)
      {
        best_inliers = inlier_count;
        best_coeffs = coeffs;
        if (best_inliers == points.size()) break;
        if ((float)best_inliers / (float)points.size() > enough_points && (float)iter / max_iterations > min_iters) break;
      }
    }
    
    if (best_inliers > 8)
    {
      //printf("%d / %d = %f\n", best_inliers, (int)points.size(), (float)best_inliers / (float)points.size());
      std::vector<SurfacePoint> inlier_points;
      for (const auto& point : points)
      {
        LiteMath::float3 voxel_pos = worldToVoxel(point.position, voxel_bounds);
        float predicted_distance = best_coeffs.evaluate(voxel_pos.x, voxel_pos.y, voxel_pos.z);
        
        if (std::abs(predicted_distance - point.distance) < inlier_threshold)
        {
          inlier_points.push_back(point);
        }
      }
      
      best_coeffs = solveLinearSystem(inlier_points, voxel_bounds);
    }
    
    return best_coeffs;
  }
  
  void gradient_descent(TrilinearCoefficients& coeffs, 
                        const std::vector<SurfacePoint>& points, 
                        const VoxelBounds& voxel_bounds,
                        float step = 0.01f,
                        int max_iterations = 100)
  {
    float real_step = step * LiteMath::length(voxel_bounds.max_corner - voxel_bounds.min_corner);
    float prev_loss = 0.0f;
    TrilinearCoefficients tmp_coeffs = coeffs;
    for (int i = 0; i < max_iterations; ++i)
    {
      float loss = 0.0f, dloss[8] = {0.0f}, tmp[8];
      for (auto &point : points)
      {
        float3 vox_point = (point.position - voxel_bounds.min_corner) / (voxel_bounds.max_corner - voxel_bounds.min_corner);
        float func = tmp_coeffs.evaluate(vox_point.x, vox_point.y, vox_point.z);
        loss += (func - point.distance) * (func - point.distance);
        tmp_coeffs.coeff_gradient(tmp, vox_point.x, vox_point.y, vox_point.z);
        for (int j = 0; j < 8; ++j)
        {
          dloss[j] += 2 * tmp[j] * (func - point.distance);
        }
      }
      if (loss <= prev_loss)
      {
        coeffs = tmp_coeffs;
        tmp_coeffs.A -= dloss[0] * real_step;
        tmp_coeffs.B -= dloss[1] * real_step;
        tmp_coeffs.C -= dloss[2] * real_step;
        tmp_coeffs.D -= dloss[3] * real_step;
        tmp_coeffs.E -= dloss[4] * real_step;
        tmp_coeffs.F -= dloss[5] * real_step;
        tmp_coeffs.G -= dloss[6] * real_step;
        tmp_coeffs.H -= dloss[7] * real_step;
      }
      else
      {
        real_step /= 2.0f;
        tmp_coeffs.A = coeffs.A - dloss[0] * real_step;
        tmp_coeffs.B = coeffs.B - dloss[1] * real_step;
        tmp_coeffs.C = coeffs.C - dloss[2] * real_step;
        tmp_coeffs.D = coeffs.D - dloss[3] * real_step;
        tmp_coeffs.E = coeffs.E - dloss[4] * real_step;
        tmp_coeffs.F = coeffs.F - dloss[5] * real_step;
        tmp_coeffs.G = coeffs.G - dloss[6] * real_step;
        tmp_coeffs.H = coeffs.H - dloss[7] * real_step;
      }
      //printf("iter: %d loss: %f\n grad (%f %f %f %f %f %f %f %f)\n coeffs: [%f %f %f %f %f %f %f %f]\n", i, loss, 
      //  dloss[0], dloss[1], dloss[2], dloss[3], dloss[4], dloss[5], dloss[6], dloss[7], 
      //  coeffs.A, coeffs.B, coeffs.C, coeffs.D, coeffs.E, coeffs.F, coeffs.G, coeffs.H);
    }
  }

  void gradient_descent_with_momentum(TrilinearCoefficients& coeffs,
                                      const std::vector<SurfacePoint>& points,
                                      const VoxelBounds& voxel_bounds,
                                      float step = 0.01f,
                                      int max_iterations = 100,
                                      float momentum = 0.9f,
                                      float decay_rate = 0.95f)
  {
    float real_step = step * LiteMath::length(voxel_bounds.max_corner - voxel_bounds.min_corner);
    TrilinearCoefficients tmp_coeffs = coeffs;
    float velocity[8] = {0.0f};

    for (int i = 0; i < max_iterations; ++i)
    {
      float loss = 0.0f, dloss[8] = {0.0f}, tmp[8];
      for (auto &point : points)
      {
        float3 vox_point = (point.position - voxel_bounds.min_corner) / (voxel_bounds.max_corner - voxel_bounds.min_corner);
        float func = tmp_coeffs.evaluate(vox_point.x, vox_point.y, vox_point.z);
        loss += (func - point.distance) * (func - point.distance);
        tmp_coeffs.coeff_gradient(tmp, vox_point.x, vox_point.y, vox_point.z);
        for (int j = 0; j < 8; ++j)
        {
          dloss[j] += 2 * tmp[j] * (func - point.distance);
        }
      }

      for (int j = 0; j < 8; ++j)
      {
        velocity[j] = momentum * velocity[j] + real_step * dloss[j];
      }

      tmp_coeffs.A -= velocity[0];
      tmp_coeffs.B -= velocity[1];
      tmp_coeffs.C -= velocity[2];
      tmp_coeffs.D -= velocity[3];
      tmp_coeffs.E -= velocity[4];
      tmp_coeffs.F -= velocity[5];
      tmp_coeffs.G -= velocity[6];
      tmp_coeffs.H -= velocity[7];

      coeffs = tmp_coeffs;

      real_step *= decay_rate;
    }
  }

  void correctSigns(TrilinearCoefficients& coeffs, 
                  const std::vector<SurfacePoint>& points,
                  const VoxelBounds& voxel_bounds)
  {
    
    if (points.empty()) return;
    
    int incorrect_orientations = 0;
    int total_valid_points = 0;
    
    for (const auto& point : points)
    {
      float normal_length = LiteMath::length(point.normal);
      if (normal_length < 1e-6f) continue;
      
      LiteMath::float3 voxel_pos = worldToVoxel(point.position, voxel_bounds);
      
      if (voxel_pos.x < 0.0f || voxel_pos.x > 1.0f ||
          voxel_pos.y < 0.0f || voxel_pos.y > 1.0f ||
          voxel_pos.z < 0.0f || voxel_pos.z > 1.0f)
      {
        continue;
      }
      
      LiteMath::float3 function_gradient = computeTrilinearGradient(coeffs, voxel_pos.x, voxel_pos.y, voxel_pos.z);
      
      float gradient_length = LiteMath::length(function_gradient);
      if (gradient_length < 1e-6f) continue;
      
      LiteMath::float3 normalized_gradient = function_gradient / gradient_length;
      LiteMath::float3 normalized_surface_normal = point.normal / normal_length;
      
      float dot_product = LiteMath::dot(normalized_gradient, normalized_surface_normal);
      
      if (dot_product < -0.1f)
      {
        incorrect_orientations++;
      }
      
      total_valid_points++;
    }
    
    if (total_valid_points > 0 && 
        incorrect_orientations > total_valid_points * 0.6f)
    {
        
      coeffs.A = -coeffs.A;
      coeffs.B = -coeffs.B;
      coeffs.C = -coeffs.C;
      coeffs.D = -coeffs.D;
      coeffs.E = -coeffs.E;
      coeffs.F = -coeffs.F;
      coeffs.G = -coeffs.G;
      coeffs.H = -coeffs.H;
      
    }
  }

  void SDF_by_point_cloud(
    const cmesh4::SimpleMesh& mesh,
    const SparseOctreeSettings settings,
    const uint32_t* triangle_ids,
    uint32_t triangle_count,
    const LiteMath::float3& voxel_min,
    const LiteMath::float3& voxel_max,
    sdf_converter::MeshDistFunc &dist_func,
    float *res)
  {
    
    VoxelBounds bounds;
    bounds.min_corner = voxel_min;
    bounds.max_corner = voxel_max;
    std::vector<SurfacePoint> point_cloud = generatePointCloud(
        mesh, triangle_ids, triangle_count, bounds, dist_func, settings.points_count);
    
    if (point_cloud.empty()) return;
    
    TrilinearCoefficients coeffs = ransacFit(point_cloud, bounds, 
                                             settings.max_iterations, 
                                             settings.inlier_threshold, 
                                             settings.minimum_iterations,
                                             settings.enough_points_percent);

    //printf("tris: %d - %f + %d\n", triangle_count, coeffs.H, (int)point_cloud.size());
    // std::vector<SurfacePoint> point_corner;
    // SurfacePoint p;
    // p.distance = coeffs.A - 0.01;
    // p.position = {0.0, 0.0, 0.0};
    // p.position = bounds.min_corner + p.position * (bounds.max_corner - bounds.min_corner);
    // point_corner.push_back(p);
    // p.distance = coeffs.B - 0.01;
    // p.position = {0.0, 0.0, 1.0};
    // p.position = bounds.min_corner + p.position * (bounds.max_corner - bounds.min_corner);
    // point_corner.push_back(p);
    // p.distance = coeffs.C - 0.01;
    // p.position = {0.0, 1.0, 0.0};
    // p.position = bounds.min_corner + p.position * (bounds.max_corner - bounds.min_corner);
    // point_corner.push_back(p);
    // p.distance = coeffs.D - 0.01;
    // p.position = {0.0, 1.0, 1.0};
    // p.position = bounds.min_corner + p.position * (bounds.max_corner - bounds.min_corner);
    // point_corner.push_back(p);
    // p.distance = coeffs.E - 0.01;
    // p.position = {1.0, 0.0, 0.0};
    // p.position = bounds.min_corner + p.position * (bounds.max_corner - bounds.min_corner);
    // point_corner.push_back(p);
    // p.distance = coeffs.F - 0.01;
    // p.position = {1.0, 0.0, 1.0};
    // p.position = bounds.min_corner + p.position * (bounds.max_corner - bounds.min_corner);
    // point_corner.push_back(p);
    // p.distance = coeffs.G - 0.01;
    // p.position = {1.0, 1.0, 0.0};
    // p.position = bounds.min_corner + p.position * (bounds.max_corner - bounds.min_corner);
    // point_corner.push_back(p);
    // p.distance = coeffs.H - 0.01;
    // p.position = {1.0, 1.0, 1.0};
    // p.position = bounds.min_corner + p.position * (bounds.max_corner - bounds.min_corner);
    // point_corner.push_back(p);
    gradient_descent_with_momentum(coeffs, point_cloud, bounds, 
                                   settings.learning_rate, settings.gradient_steps, settings.momentum, settings.decay_rate);
    
    //correctSigns(coeffs, point_cloud, bounds);

    res[0] = coeffs.A;
    res[1] = coeffs.B;
    res[2] = coeffs.C;
    res[3] = coeffs.D;
    res[4] = coeffs.E;
    res[5] = coeffs.F;
    res[6] = coeffs.G;
    res[7] = coeffs.H;
  }

};