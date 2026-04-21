#include "sparse_octree_unstructured_grid.h"
#include "utils/vtk/parser.h"
#include <unordered_set>

namespace sdf_converter
{
  using vtk::LegacyCell;
  using vtk::PackedCell;
  using vtk::FullCell;
  using vtk::UnstructuredGrid;

  const float4 calculate_tetrahedron_barycentric(const float3 &v0,
                                                 const float3 &v1,
                                                 const float3 &v2,
                                                 const float3 &v3,
                                                 const float3 &p0)
  {
    float3 e1 = v1 - v0;
    float3 e2 = v2 - v0;
    float3 e3 = v3 - v0;
    float3 p_prime = p0 - v0;

    float3 cross_e2_e3 = cross(e2, e3);
    float detM = dot(e1, cross_e2_e3);

    if (std::abs(detM) < 1e-9f)
      return float4(1.0f, 0.0f, 0.0f, 0.0f);

    float invDetM = 1.0f / detM;

    float3 cross_p_e3 = cross(p_prime, e3);
    float3 cross_e2_p = cross(e2, p_prime);

    float lambda1 = dot(p_prime, cross_e2_e3) * invDetM;
    float lambda2 = dot(e1, cross_p_e3) * invDetM;
    float lambda3 = dot(e1, cross_e2_p) * invDetM;
    float lambda0 = 1.0f - lambda1 - lambda2 - lambda3;

    return float4(lambda0, lambda1, lambda2, lambda3);
  }

  struct FaceData
  {
    std::unordered_set<int> vertex_indices;
    float distance_face;
  };

  template <bool force_positive>
  std::vector<float> computeWachspressCoordinates(float3 p, const std::vector<float3> &points,
                                                  const std::vector<std::vector<int>> &indices)
  {
    std::vector<FaceData> faces_data;

    // Precompute plane data and distance for each face
    for (const auto &face : indices)
    {
      if (face.size() < 3)
        continue; // invalid face, skip

      FaceData fd;
      fd.vertex_indices.insert(face.begin(), face.end());

      // Get three points to compute the plane
      const float3 &p0 = points[face[0]];
      const float3 &p1 = points[face[1]];
      const float3 &p2 = points[face[2]];

      // Compute vectors
      float3 v1 = p1 - p0;
      float3 v2 = p2 - p0;

      // Compute cross product for normal
      float3 normal = normalize(cross(v1, v2));
      normal.x = v1.y * v2.z - v1.z * v2.y;
      normal.y = v1.z * v2.x - v1.x * v2.z;
      normal.z = v1.x * v2.y - v1.y * v2.x;

      // Compute plane's d
      float d = -(normal.x * p0.x + normal.y * p0.y + normal.z * p0.z);

      // Compute signed distance of p to the plane
      float signed_distance = normal.x * p.x + normal.y * p.y + normal.z * p.z + d;

      if constexpr (force_positive)
      {

        // Ensure normal points outward (signed_distance should be negative if p is inside)
        if (signed_distance > 0)
        {
          normal.x = -normal.x;
          normal.y = -normal.y;
          normal.z = -normal.z;
          d = -d;
          signed_distance = -signed_distance;
        }
      }

      fd.distance_face = -signed_distance; // since signed_distance is negative, this is positive
      faces_data.push_back(fd);
    }

    // Compute weights for each vertex
    std::vector<float> weights(points.size(), 1.0f);
    for (size_t i = 0; i < points.size(); ++i)
    {
      for (const auto &fd : faces_data)
      {
        if (fd.vertex_indices.find(i) == fd.vertex_indices.end())
        {
          weights[i] *= fd.distance_face;
        }
      }
    }

    // Normalize the weights to sum to 1
    float sum = 0.0f;
    for (float w : weights)
    {
      sum += w;
    }

    std::vector<float> barycentric;
    barycentric.reserve(weights.size());
    for (float w : weights)
    {
      barycentric.push_back(w / sum);
    }

    return barycentric;
  }

  bool check_point_in_tetrahedron(const float3 &v0_,
                                        const float3 &v1_,
                                        const float3 &v2_,
                                        const float3 &v3_,
                                        const float3 &p0_)
  {
    std::vector<float3> points = {v0_, v1_, v2_, v3_};
    std::vector<std::vector<int>> indices(4);
    indices[0] = {0, 1, 2};
    indices[1] = {0, 3, 1};
    indices[2] = {0, 2, 3};
    indices[3] = {1, 3, 2};
    std::vector<float> bc1 = computeWachspressCoordinates<false>(p0_, points, indices);
    return bc1[0] >= 0 && bc1[1] >= 0 && bc1[2] >= 0 && bc1[3] >= 0;

    float4 barycentricCoord = calculate_tetrahedron_barycentric(v0_, v1_, v2_, v3_, p0_);
    return barycentricCoord.x >= 0 && barycentricCoord.y >= 0 && barycentricCoord.z >= 0 && barycentricCoord.w >= 0;
  }

  #define GET_INDEX(poly_id, point_id) fc.indices[fc.polygons_offsets[poly_id] + point_id]
  #define GET_POINT(poly_id, point_id) grid.points[GET_INDEX(poly_id, point_id)]

  bool check_point_in_wedge(const vtk::UnstructuredGrid &grid, const float3 &pos, const FullCell &fc)
  {
    float3 a = GET_POINT(0, 0);
    float3 b = GET_POINT(0, 1);
    float3 c = GET_POINT(0, 2);
    float3 d = GET_POINT(1, 0);
    float3 e = GET_POINT(1, 2);
    float3 f = GET_POINT(1, 1);
    std::vector<float3> points = {a, b, c, d, e, f};
    std::vector<std::vector<int>> indices(5);
    indices[0] = {0, 1, 2};
    indices[1] = {3, 5, 4};
    indices[2] = {1, 4, 5, 2};
    indices[3] = {0, 2, 5, 3};
    indices[4] = {0, 3, 4, 1};
    std::vector<float> res = computeWachspressCoordinates<false>(pos, points, indices);

    return res[0] >= 0 && res[1] >= 0 && res[2] >= 0 && res[3] >= 0 && res[4] >= 0 && res[5] >= 0;
  }

  bool check_point_in_pyramid(const vtk::UnstructuredGrid &grid, const float3 &pos, const FullCell &fc)
  {
    float3 a = GET_POINT(0, 0);
    float3 b = GET_POINT(0, 1);
    float3 c = GET_POINT(0, 2);
    float3 d = GET_POINT(0, 3);
    float3 e = GET_POINT(1, 1);
    std::vector<float3> points = {a, b, c, d, e};
    std::vector<std::vector<int>> indices(5);
    indices[0] = {0, 1, 2, 3};
    indices[1] = {0, 4, 1};
    indices[2] = {1, 4, 2};
    indices[3] = {2, 4, 3};
    indices[4] = {3, 4, 0};
    std::vector<float> res = computeWachspressCoordinates<true>(pos, points, indices);

    return res[0] >= 0 && res[1] >= 0 && res[2] >= 0 && res[3] >= 0 && res[4] >= 0;
  }


  bool UGCellInBBoxFunc::in_bbox(const vtk::UnstructuredGrid &grid, unsigned t_i, const float3 &bbox_min, const float3 &bbox_max)
  {
    FullCell fc;
    vtk::unpack_cell(grid.p_cells[t_i], grid.p_indices.data(), &fc);

    switch (grid.p_cells[t_i].type)
    {
      case vtk::VTK_TRIANGLE:
      {
        float3 a = GET_POINT(0, 0);
        float3 b = GET_POINT(0, 1);
        float3 c = GET_POINT(0, 2);
        return contains(a, bbox_min, bbox_max) || contains(b, bbox_min, bbox_max) || contains(c, bbox_min, bbox_max) ||
               cmesh4::triangle_aabb_intersect(a, b, c, 0.5f*(bbox_min + bbox_max), 0.5f*(bbox_max - bbox_min));
      }
      case vtk::VTK_TRIANGLE_STRIP:
      {
        printf("[UGCellInBBoxFunc::in_bbox] not implemented for vtk::VTK_TRIANGLE_STRIP\n");
        return false;
      }
      case vtk::VTK_POLYGON:
      {
        printf("[UGCellInBBoxFunc::in_bbox] not implemented for vtk::VTK_POLYGON\n");
        return false;
      }
      case vtk::VTK_PIXEL:
      {
        printf("[UGCellInBBoxFunc::in_bbox] not implemented for vtk::VTK_PIXEL\n");
        return false;
      }
      case vtk::VTK_QUAD:
      {
        float3 a = GET_POINT(0, 0);
        float3 b = GET_POINT(0, 1);
        float3 c = GET_POINT(0, 2);
        float3 d = GET_POINT(0, 3);
        return contains(a, bbox_min, bbox_max) || contains(b, bbox_min, bbox_max) || 
               contains(c, bbox_min, bbox_max) || contains(d, bbox_min, bbox_max) ||
               cmesh4::triangle_aabb_intersect(a, b, c, 0.5f*(bbox_min + bbox_max), 0.5f*(bbox_max - bbox_min)) ||
               cmesh4::triangle_aabb_intersect(b, c, d, 0.5f*(bbox_min + bbox_max), 0.5f*(bbox_max - bbox_min));
      }
      case vtk::VTK_TETRA:
      {
        //TODO, FIXME: fill return false if bbox in fully inside the tetrahedron
        float3 a = GET_POINT(0, 0);
        float3 b = GET_POINT(0, 1);
        float3 c = GET_POINT(0, 2);
        float3 d = GET_POINT(1, 1);
        bool points_inside = contains(a, bbox_min, bbox_max) || contains(b, bbox_min, bbox_max) || 
                             contains(c, bbox_min, bbox_max) || contains(d, bbox_min, bbox_max);
        if (points_inside)
          return true;
        for (int i=0; i<4; i++)
        {
          a = GET_POINT(i, 0);
          b = GET_POINT(i, 1);
          c = GET_POINT(i, 2);
          if (cmesh4::triangle_aabb_intersect(a, b, c, 0.5f*(bbox_min + bbox_max), 0.5f*(bbox_max - bbox_min)))
            return true;
        }
        return false;
      }
      case vtk::VTK_VOXEL:
      case vtk::VTK_HEXAHEDRON:
      {
        float3 a = GET_POINT(0, 0);
        float3 b = GET_POINT(4, 3);
        float3 v_min = min(a, b);
        float3 v_max = max(a, b);

        return (v_min.x <= bbox_max.x) &&
               (v_max.x >= bbox_min.x) &&
               (v_min.y <= bbox_max.y) &&
               (v_max.y >= bbox_min.y) &&
               (v_min.z <= bbox_max.z) &&
               (v_max.z >= bbox_min.z);        
      }
      case vtk::VTK_WEDGE:
      {
        float3 a = GET_POINT(0, 0);
        float3 b = GET_POINT(0, 1);
        float3 c = GET_POINT(0, 2);
        float3 d = GET_POINT(1, 0);
        float3 e = GET_POINT(1, 2);
        float3 f = GET_POINT(1, 1);
        std::vector<float3> points = {a, b, c, d, e, f};
        float3 v_min = a, v_max = a;
        for (int i=0; i<6; i++)
        {
          v_min = min(v_min, points[i]);
          v_max = max(v_max, points[i]);
        }
        return (v_min.x <= bbox_max.x) &&
               (v_max.x >= bbox_min.x) &&
               (v_min.y <= bbox_max.y) &&
               (v_max.y >= bbox_min.y) &&
               (v_min.z <= bbox_max.z) &&
               (v_max.z >= bbox_min.z);         
      }
      case vtk::VTK_PYRAMID:
      {
        float3 a = GET_POINT(0, 0);
        float3 b = GET_POINT(0, 1);
        float3 c = GET_POINT(0, 2);
        float3 d = GET_POINT(0, 3);
        float3 e = GET_POINT(1, 1);
        std::vector<float3> points = {a, b, c, d, e};
        float3 v_min = a, v_max = a;
        for (int i=0; i<5; i++)
        {
          v_min = min(v_min, points[i]);
          v_max = max(v_max, points[i]);
        }
        return (v_min.x <= bbox_max.x) &&
               (v_max.x >= bbox_min.x) &&
               (v_min.y <= bbox_max.y) &&
               (v_max.y >= bbox_min.y) &&
               (v_min.z <= bbox_max.z) &&
               (v_max.z >= bbox_min.z);
        return false;        
      }
      case vtk::VTK_PENTAGONAL_PRISM:
      {
        printf("[UGCellInBBoxFunc::in_bbox] not implemented for vtk::VTK_PENTAGONAL_PRISM\n");
        return false;        
      }
      case vtk::VTK_HEXAGONAL_PRISM:
      {
        printf("[UGCellInBBoxFunc::in_bbox] not implemented for vtk::VTK_HEXAGONAL_PRISM\n");
        return false;        
      }
      default:
        printf("[UGCellInBBoxFunc::in_bbox] not implemented for cell type %d\n", (int)grid.p_cells[t_i].type);
        return false;
    }
  }

  //find distance to polygon by calculating distance to each triangle
  void distance_to_polygon_tri(const vtk::UnstructuredGrid &grid, const float3 &pos, 
                               const FullCell &fc, uint32_t p_idx, 
                               uint32_t &closest_idx, float &closest_dist_sq, float3 &closest_point)
  {
    uint32_t off = fc.polygons_offsets[p_idx];
    uint32_t count = fc.polygons_offsets[p_idx+1] - fc.polygons_offsets[p_idx];
    for (int i=2;i<count;i++)
    {
      float3 a = GET_POINT(p_idx, 0);
      float3 b = GET_POINT(p_idx, i-1);
      float3 c = GET_POINT(p_idx, i);
      float3 p = cmesh4::closest_point_triangle(pos, a, b, c);
      float dist_sq = dot(p - pos, p - pos);
      if (dist_sq < closest_dist_sq)
      {
        closest_dist_sq = dist_sq;
        closest_point = p;
        closest_idx = p_idx;
      }
    }
  }

  bool in_cell(const vtk::UnstructuredGrid &grid, const float3 &pos, const FullCell &fc)
  {
    if (!vtk::is_3d_cell(fc.type))
      return false;
  
    switch (fc.type)
    {
      case vtk::VTK_TETRA:
      {
        float3 a = GET_POINT(0, 0);
        float3 b = GET_POINT(0, 1);
        float3 c = GET_POINT(0, 2);
        float3 d = GET_POINT(1, 1);

        return check_point_in_tetrahedron(a, b, c, d, pos);
      }
      case vtk::VTK_VOXEL:
      case vtk::VTK_HEXAHEDRON:
      {
        float3 a = GET_POINT(0, 0);
        float3 b = GET_POINT(4, 3);
        float3 v_min = min(a, b);
        float3 v_max = max(a, b);
        return (v_min.x <= pos.x) &&
               (v_max.x >= pos.x) &&
               (v_min.y <= pos.y) &&
               (v_max.y >= pos.y) &&
               (v_min.z <= pos.z) &&
               (v_max.z >= pos.z);     
      }
      case vtk::VTK_WEDGE:
      {
        return check_point_in_wedge(grid, pos, fc);       
      }
      case vtk::VTK_PYRAMID:
      {
        return check_point_in_pyramid(grid, pos, fc);  
      }
      case vtk::VTK_PENTAGONAL_PRISM:
      {
        printf("[in_cell] not implemented for vtk::VTK_PENTAGONAL_PRISM\n");
        return false;        
      }
      case vtk::VTK_HEXAGONAL_PRISM:
      {
        printf("[in_cell] not implemented for vtk::VTK_HEXAGONAL_PRISM\n");
        return false;        
      }
      default:
        printf("[in_cell] not implemented for cell type %d\n", (int)fc.type);
        return false;
    }
  }

float sdBox( float3 p, float3 b )
{
  float3 q = abs(p) - b;
  return length(max(q,float3(0,0,0))) + min(max(q.x,max(q.y,q.z)),0.0f);
}

  PointAttributes UGCellDistFunc::calculate(const vtk::UnstructuredGrid &grid, const uint32_t *p_ids, uint32_t p_ids_count, float3 pos)
  {
    PointAttributes po;

    float3 closest_point = float3(1e9, 1e9, 1e9);
    float closest_dist_sq = 1e6f;
    uint32_t closest_prim_id = 0;
    uint32_t forced_closest_prim_id = 0;
    float sign = 1.0f;
    
    float3 p;
    float d;
    uint32_t poly_id;
    FullCell fc;
    for (int i=0; i<p_ids_count; i++)
    {
      uint32_t prim_id = p_ids[i];

      vtk::unpack_cell(grid.p_cells[prim_id], grid.p_indices.data(), &fc);
      if (in_cell(grid, pos, fc))
      {
        sign = -1.0f;
        forced_closest_prim_id = prim_id;
      }

      if (grid.p_cells[prim_id].border_bits)
      {
        d = 1e9;
        p = float3(1e9, 1e9, 1e9);
        poly_id = 0;
        for (int j=0; j<fc.polygons_count; j++)
        {
          if (grid.p_cells[prim_id].border_bits & (1 << j))
            distance_to_polygon_tri(grid, pos, fc, j, poly_id, d, p);
        }
        if (d < closest_dist_sq)
        {
          closest_dist_sq = d;
          closest_point = p;
          closest_prim_id = prim_id;
        }
      }
    }

    bool is_3D = vtk::is_3d_cell(grid.p_cells[closest_prim_id].type);

    po.closest_point = is_3D ? pos : closest_point; //for 3D cell closest point is the point itself
    po.closest_primitive_id = forced_closest_prim_id > 0 ? forced_closest_prim_id : closest_prim_id;
    po.distance = sign*sqrtf(closest_dist_sq);
    po.mat = 0;
    po.tex_coord = float2(0, 0);

    return po;
  }

  template <typename T>
  T get_value(const vtk::DataArray &arr, uint32_t idx)
  {
    switch (arr.values.type)
    {
      case vtk::DataArray::Type::Int8:
        return (T)arr.values.i8[idx];
      case vtk::DataArray::Type::UInt8:
        return (T)arr.values.u8[idx];
      case vtk::DataArray::Type::Int16:
        return (T)arr.values.i16[idx];
      case vtk::DataArray::Type::UInt16:
        return (T)arr.values.u16[idx];
      case vtk::DataArray::Type::Int32:
        return (T)arr.values.i32[idx];
      case vtk::DataArray::Type::UInt32:
        return (T)arr.values.u32[idx];
      case vtk::DataArray::Type::Int64:
        return (T)arr.values.i64[idx];
      case vtk::DataArray::Type::UInt64:
        return (T)arr.values.u64[idx];
      case vtk::DataArray::Type::Float32:
        return (T)arr.values.f32[idx];
      case vtk::DataArray::Type::Float64:
        return (T)arr.values.f64[idx];
      default:
        return 0;
    }
  }

  // point attributes interpolation quotient
  struct PAIQ
  {
    float weight = 0.0f;
    uint32_t point_id = 0;
  };

  void calculate_PAIQs_triangle(const vtk::UnstructuredGrid &grid, const PointAttributes &pa, const FullCell &fc,
                                PAIQ *paiqs)
  {
    float3 p0 = GET_POINT(0, 0);
    float3 p1 = GET_POINT(0, 1);
    float3 p2 = GET_POINT(0, 2);

    float3 barycentric = cmesh4::barycentric(pa.closest_point, p0, p1, p2);
    
    paiqs[0] = {barycentric.x, GET_INDEX(0, 0)};
    paiqs[1] = {barycentric.y, GET_INDEX(0, 1)};
    paiqs[2] = {barycentric.z, GET_INDEX(0, 2)};
  }

  void calculate_PAIQs_polygon(const vtk::UnstructuredGrid &grid, const PointAttributes &pa, const FullCell &fc,
                               PAIQ *paiqs)
  {
    printf("[calculate_PAIQs_polygon] not implemented\n");
  }

  void calculate_PAIQs_tetrahedron(const vtk::UnstructuredGrid &grid, const PointAttributes &pa, const FullCell &fc,
                                   PAIQ *paiqs)
  {
    float3 a = GET_POINT(0, 0);
    float3 b = GET_POINT(0, 1);
    float3 c = GET_POINT(0, 2);
    float3 d = GET_POINT(1, 1);
    std::vector<float3> points = {a, b, c, d};
    std::vector<std::vector<int>> indices(4);
    indices[0] = {0, 1, 2};
    indices[1] = {0, 3, 1};
    indices[2] = {0, 2, 3};
    indices[3] = {1, 3, 2};
    std::vector<float> res = computeWachspressCoordinates<true>(pa.closest_point, points, indices);

    paiqs[0] = {res[0], GET_INDEX(0, 0)};
    paiqs[1] = {res[1], GET_INDEX(0, 1)};
    paiqs[2] = {res[2], GET_INDEX(0, 2)};
    paiqs[3] = {res[3], GET_INDEX(1, 1)};
  }

  void calculate_PAIQs_wedge(const vtk::UnstructuredGrid &grid, const PointAttributes &pa, const FullCell &fc,
    PAIQ *paiqs)
  {
    float3 a = GET_POINT(0, 0);
    float3 b = GET_POINT(0, 1);
    float3 c = GET_POINT(0, 2);
    float3 d = GET_POINT(1, 0);
    float3 e = GET_POINT(1, 2);
    float3 f = GET_POINT(1, 1);
    std::vector<float3> points = {a, b, c, d, e, f};
    std::vector<std::vector<int>> indices(5);
    indices[0] = {0, 2, 1};
    indices[1] = {3, 4, 5};
    indices[2] = {1, 2, 5, 4};
    indices[3] = {0, 3, 5, 2};
    indices[4] = {0, 1, 4, 3};
    std::vector<float> res = computeWachspressCoordinates<true>(pa.closest_point, points, indices);

    paiqs[0] = {res[0], GET_INDEX(0, 0)};
    paiqs[1] = {res[1], GET_INDEX(0, 1)};
    paiqs[2] = {res[2], GET_INDEX(0, 2)};
    paiqs[3] = {res[3], GET_INDEX(1, 0)};
    paiqs[4] = {res[4], GET_INDEX(1, 2)};
    paiqs[5] = {res[5], GET_INDEX(1, 1)};
  }

  void calculate_PAIQs_pyramid(const vtk::UnstructuredGrid &grid, const PointAttributes &pa, const FullCell &fc,
                               PAIQ *paiqs)
  {
    float3 a = GET_POINT(0, 0);
    float3 b = GET_POINT(0, 1);
    float3 c = GET_POINT(0, 2);
    float3 d = GET_POINT(0, 3);
    float3 e = GET_POINT(1, 1);
    std::vector<float3> points = {a, b, c, d, e};
    std::vector<std::vector<int>> indices(5);
    indices[0] = {0, 1, 2, 3};
    indices[1] = {0, 4, 1};
    indices[2] = {1, 4, 2};
    indices[3] = {2, 4, 3};
    indices[4] = {3, 4, 0};
    std::vector<float> res = computeWachspressCoordinates<true>(pa.closest_point, points, indices);

    paiqs[0] = {res[0], GET_INDEX(0, 0)};
    paiqs[1] = {res[1], GET_INDEX(0, 1)};
    paiqs[2] = {res[2], GET_INDEX(0, 2)};
    paiqs[3] = {res[3], GET_INDEX(0, 3)};
    paiqs[4] = {res[4], GET_INDEX(1, 1)};
  }

  void calculate_PAIQs_cube(const vtk::UnstructuredGrid &grid, const PointAttributes &pa, const FullCell &fc,
                            PAIQ *paiqs)
  {
    float3 a = GET_POINT(0, 0);
    float3 b = GET_POINT(4, 3);
    const float3 min_pos = min(a, b);
    const float3 max_pos = max(a, b);
    const float3 size = max_pos - min_pos;
    const float3 center = min_pos + size/2;
    const float3 lerp_q = clamp((pa.closest_point - min_pos)/size, float3(0, 0, 0), float3(1, 1, 1));

    uint32_t points[8] = {
      GET_INDEX(0, 0),
      GET_INDEX(0, 3),
      GET_INDEX(0, 1),
      GET_INDEX(0, 2),
      GET_INDEX(1, 0),
      GET_INDEX(1, 1),
      GET_INDEX(1, 3),
      GET_INDEX(1, 2),
    };

    for (int i = 0; i < 8; i++)
    {
      float3 p = grid.points[points[i]];
      float3 c = p - center;
      paiqs[i].point_id = points[i];
      paiqs[i].weight = (c.x > 0 ? lerp_q.x : (1-lerp_q.x)) * (c.y > 0 ? lerp_q.y : (1-lerp_q.y)) * (c.z > 0 ? lerp_q.z : (1-lerp_q.z));
    }

    // float3 code;
    // code =  paiqs[0] = {(1-lerp_q.x)*(1-lerp_q.y)*(1-lerp_q.z), closest_cell.polygons[0].points[0]};
    // paiqs[1] = {(  lerp_q.x)*(1-lerp_q.y)*(1-lerp_q.z), closest_cell.polygons[0].points[3]};
    // paiqs[2] = {(1-lerp_q.x)*(  lerp_q.y)*(1-lerp_q.z), closest_cell.polygons[0].points[1]};
    // paiqs[3] = {(  lerp_q.x)*(  lerp_q.y)*(1-lerp_q.z), closest_cell.polygons[0].points[2]};
    // paiqs[4] = {(1-lerp_q.x)*(1-lerp_q.y)*(  lerp_q.z), closest_cell.polygons[1].points[0]};
    // paiqs[5] = {(  lerp_q.x)*(1-lerp_q.y)*(  lerp_q.z), closest_cell.polygons[1].points[1]};
    // paiqs[6] = {(1-lerp_q.x)*(  lerp_q.y)*(  lerp_q.z), closest_cell.polygons[1].points[3]};
    // paiqs[7] = {(  lerp_q.x)*(  lerp_q.y)*(  lerp_q.z), closest_cell.polygons[1].points[2]};
  }


  void calculate_PAIQs_generalized_barycentric(const vtk::UnstructuredGrid &grid, const PointAttributes &pa, const FullCell &fc,
                                               PAIQ *paiqs)
  {
    printf("[calculate_PAIQs_generalized_barycentric] not implemented\n");
  }

  void calculate_PAIQs(const vtk::UnstructuredGrid &grid, const PointAttributes &pa, const FullCell &closest_cell,
                       PAIQ *paiqs, unsigned *num_paiqs)
  {
    *num_paiqs = 0;
    switch (closest_cell.type)
    {
      case vtk::VTK_TRIANGLE:
        *num_paiqs = 3;
        calculate_PAIQs_triangle(grid, pa, closest_cell, paiqs);
        break;
      case vtk::VTK_POLYGON:
        calculate_PAIQs_polygon(grid, pa, closest_cell, paiqs);
        break;
      case vtk::VTK_TETRA:
        *num_paiqs = 4;
        calculate_PAIQs_tetrahedron(grid, pa, closest_cell, paiqs);
        break;
      case vtk::VTK_HEXAHEDRON:
        *num_paiqs = 8;
        calculate_PAIQs_cube(grid, pa, closest_cell, paiqs);
        break;
      case vtk::VTK_WEDGE:
        *num_paiqs = 6;
        calculate_PAIQs_wedge(grid, pa, closest_cell, paiqs);
        break;
      case vtk::VTK_PYRAMID:
        *num_paiqs = 5;
        calculate_PAIQs_pyramid(grid, pa, closest_cell, paiqs);
        break;
      default:
        calculate_PAIQs_generalized_barycentric(grid, pa, closest_cell, paiqs);
        break;
    }
  }

  void fill_data_channels_node(const vtk::UnstructuredGrid &grid, GlobalOctree &octree, unsigned v_size, unsigned size,
                               const std::vector<sdf_converter::PointAttributes> &attributes, unsigned node_idx)
  {
    // fill voxel channels with values from cell_data_arrays from center-ish point
    {
      unsigned idx = v_size * v_size * (size / 2) + v_size * (size / 2) + size / 2;
      unsigned in_id = attributes[idx].closest_primitive_id;
      unsigned out_id = node_idx;

      for (unsigned ch_id = 0; ch_id < grid.cell_data_arrays.size(); ch_id++)
      {
        unsigned c_num = octree.voxel_channels[ch_id].num_components;
        for (unsigned c_id = 0; c_id < c_num; c_id++)
        {
          if (octree.voxel_channels[ch_id].type == DataChannel::Type::FLOAT)
            octree.voxel_channels[ch_id].data_f[out_id * c_num + c_id] = get_value<float>(grid.cell_data_arrays[ch_id], in_id * c_num + c_id);
          else if (octree.voxel_channels[ch_id].type == DataChannel::Type::INT)
            octree.voxel_channels[ch_id].data_i[out_id * c_num + c_id] = get_value<int>(grid.cell_data_arrays[ch_id], in_id * c_num + c_id);
        }
      }
    }

    FullCell cell;
    for (int j = 0; j < 8; ++j)
    {
      unsigned idx = v_size * v_size * (j >> 2) + v_size * ((j >> 1) & 1) + (j & 1);
      unsigned t_i = attributes[idx].closest_primitive_id;
      vtk::unpack_cell(grid.p_cells[t_i], grid.p_indices.data(), &cell);
      unsigned paiq_num = 0;
      PAIQ paiqs[12]; //there are no more that 12 points in supported cells
      calculate_PAIQs(grid, attributes[idx], cell, paiqs, &paiq_num);
      if (paiq_num == 0)
        continue;

      unsigned out_id = 8 * node_idx + j;
      for (unsigned ch_id = 0; ch_id < octree.point_channels.size(); ch_id++)
      {
        unsigned c_num = octree.point_channels[ch_id].num_components;
        for (unsigned c_id = 0; c_id < c_num; c_id++)
        {
          octree.point_channels[ch_id].data_f[out_id * c_num + c_id] = 0;
          for (unsigned k = 0; k < paiq_num; k++)
          {
            //if (std::abs(paiqs[k].weight) > 2.0f)
            //  printf("PAIQ weight %f\n", paiqs[k].weight);
            paiqs[k].weight = std::min(std::max(paiqs[k].weight, -0.5f), 1.5f);
            float value = get_value<float>(grid.point_data_arrays[ch_id], paiqs[k].point_id * c_num + c_id);
            octree.point_channels[ch_id].data_f[out_id * c_num + c_id] += value * paiqs[k].weight;
          }
          // float v = octree.point_channels[ch_id].data_f[out_id * c_num + c_id];
          // float real_v = 0.5f*length(attributes[idx].closest_point);
          // if (real_v < 0.5f*0.9)
          // {
          //   printf("v real_v %f %f\n", v, real_v);
          //   printf("pos = %f %f %f\n", attributes[idx].closest_point.x, attributes[idx].closest_point.y, attributes[idx].closest_point.z);
          //   for (int k = 0; k < paiq_num; k++)
          //   {
          //     float value = get_value<float>(grid.point_data_arrays[ch_id], paiqs[k].point_id * c_num + c_id);
          //     float3 pnt = grid.points[paiqs[k].point_id];
          //     printf("pnt %f %f %f, q= %f v = %f\n", pnt.x, pnt.y, pnt.z, paiqs[k].weight, value);
          //   }
          //   printf("\n");
          // }
        }
      }
    }
  }

  void init_data_channels(const vtk::UnstructuredGrid &grid, GlobalOctree &octree)
  {
    octree.point_channels.resize(grid.point_data_arrays.size());
    octree.voxel_channels.resize(grid.cell_data_arrays.size());

    for (int i=0; i<grid.point_data_arrays.size(); i++)
    {
      //only float data type is supported for point channel
      octree.point_channels[i].type = DataChannel::Type::FLOAT;
      octree.point_channels[i].num_components = grid.point_data_arrays[i].num_components;
      octree.point_channels[i].name = grid.point_data_arrays[i].name;
      vtk::calculate_data_array_limits(grid.point_data_arrays[i], &(octree.point_channels[i].min_val), &(octree.point_channels[i].max_val));
    }

    for (int i=0; i<grid.cell_data_arrays.size(); i++)
    {
      switch (grid.cell_data_arrays[i].values.type)
      {
        case vtk::DataArray::Type::Float32:
        case vtk::DataArray::Type::Float64:
          octree.voxel_channels[i].type = DataChannel::Type::FLOAT;
          break;
        default:
          octree.voxel_channels[i].type = DataChannel::Type::INT;
          break;
      }
      octree.voxel_channels[i].num_components = grid.cell_data_arrays[i].num_components;
      octree.voxel_channels[i].name = grid.cell_data_arrays[i].name;
      vtk::calculate_data_array_limits(grid.cell_data_arrays[i], &(octree.voxel_channels[i].min_val), &(octree.voxel_channels[i].max_val));
    }    
  }
}