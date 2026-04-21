#include "procedural_meshes.h"
#include "mesh_internal.h"
#include <random>

namespace cmesh4
{
  using LiteMath::float4;
  using LiteMath::float3;
  using LiteMath::float2;
  using LiteMath::uint3;

  struct RawVertex
  {
    RawVertex() = default;
    explicit RawVertex(float3 a_pos)
    {
      pos = a_pos;
      tc = float2(0,0);      
    }
    RawVertex(float x, float y, float z)
    {
      pos = float3(x,y,z);
    }
    RawVertex(float3 a_pos, float2 a_tc)
    {
      pos = a_pos;
      tc = a_tc;
    }
    float3 pos;
    float2 tc;
  };

  static void set_normals_around_center(const std::vector<RawVertex>& a_vertices, std::vector<uint3>& a_triangles, float3 center = float3(0,0,0))
  {
    for (uint32_t i = 0; i < a_triangles.size(); i++)
    {
      uint3 tri = a_triangles[i];
      float3 a = a_vertices[tri.x].pos;
      float3 b = a_vertices[tri.y].pos;
      float3 c = a_vertices[tri.z].pos;
      float3 normal = LiteMath::cross(b - a, c - a);
      if (LiteMath::dot(normal, a - center) < 0)
      {
        unsigned t = tri.z;
        tri.z = tri.y;
        tri.y = t;
      }
      a_triangles[i] = tri;
    }
  }


  static SimpleMesh create_mesh(const std::vector<RawVertex>& a_vertices, std::vector<uint3>& a_triangles,
                                const std::vector<unsigned>& a_materials,
                                VerticesType a_verticesType, NormalsType a_normalsType, bool fix_normals = false, float3 center = float3(0,0,0))  
  {
    std::vector<unsigned> materials = a_materials;
    assert(a_triangles.size() == materials.size() || materials.size() == 0);
    
    if (materials.size() == 0)
      materials.resize(a_triangles.size(), 0);

    // define proper vertex normals and tangents
    std::vector<float3> normals(a_vertices.size(), float3(0,0,0));
    std::vector<float3> tangents(a_vertices.size(), float3(0,0,0));
    std::vector<float> counts(a_vertices.size(), 0);
    for (uint32_t i = 0; i < a_triangles.size(); i++)
    {
      uint3 tri = a_triangles[i];
      float3 a = a_vertices[tri.x].pos;
      float3 b = a_vertices[tri.y].pos;
      float3 c = a_vertices[tri.z].pos;
      float3 normal = LiteMath::normalize(LiteMath::cross(b - a, c - a));
      //printf("triangle %u: a: %f %f %f b: %f %f %f c: %f %f %f normal: %f %f %f\n", i, a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z, normal.x, normal.y, normal.z);

      normals[tri.x] += normal;
      normals[tri.y] += normal;
      normals[tri.z] += normal;
      tangents[tri.x] += LiteMath::normalize(b - a);
      tangents[tri.y] += LiteMath::normalize(b - a);
      tangents[tri.z] += LiteMath::normalize(b - a);
      counts[tri.x]++;
      counts[tri.y]++;
      counts[tri.z]++;
    }
    for (uint32_t i = 0; i < normals.size(); i++)
    {
      if (counts[i] > 0)
      {
        normals[i] /= counts[i];
        tangents[i] /= counts[i];
      }
      else
      {
        normals[i] = float3(0,0,1);
        tangents[i] = float3(1,0,0);
        printf("WARNING: vertex %d is not used by any triangle\n", i);
      }
    }

    SimpleMesh mesh;
    if (a_verticesType == VerticesType::SHARED)
    {
      mesh.vPos4f.resize(a_vertices.size());
      mesh.vNorm4f.resize(a_vertices.size());
      mesh.vTang4f.resize(a_vertices.size());
      mesh.vTexCoord2f.resize(a_vertices.size());

      mesh.indices.resize(a_triangles.size() * 3);
      mesh.matIndices.resize(a_triangles.size());

      for (uint32_t i = 0; i < a_vertices.size(); i++)
      {
        mesh.vPos4f[i] = LiteMath::to_float4(a_vertices[i].pos, 1);
        mesh.vNorm4f[i] = LiteMath::to_float4(normals[i], 0);
        mesh.vTang4f[i] = LiteMath::to_float4(tangents[i], 0);
        mesh.vTexCoord2f[i] = a_vertices[i].tc;
      }

      for (uint32_t i = 0; i < a_triangles.size(); i++)
      {
        mesh.indices[i * 3 + 0] = a_triangles[i].x;
        mesh.indices[i * 3 + 1] = a_triangles[i].y;
        mesh.indices[i * 3 + 2] = a_triangles[i].z;
        mesh.matIndices[i] = materials[i];
      }
    }
    else
    {
      mesh.vPos4f.resize(a_triangles.size() * 3);
      mesh.vNorm4f.resize(a_triangles.size() * 3);
      mesh.vTang4f.resize(a_triangles.size() * 3);
      mesh.vTexCoord2f.resize(a_triangles.size() * 3);

      mesh.indices.resize(a_triangles.size() * 3);
      mesh.matIndices.resize(a_triangles.size());

      for (uint32_t i = 0; i < a_triangles.size(); i++)
      {
        uint3 tri = a_triangles[i];
        float3 a = a_vertices[tri.x].pos;
        float3 b = a_vertices[tri.y].pos;
        float3 c = a_vertices[tri.z].pos;
        float3 geom_normal = LiteMath::normalize(LiteMath::cross(b - a, c - a));
        float3 geom_tangent = LiteMath::normalize(b - a);

        mesh.vPos4f[i * 3 + 0] = LiteMath::to_float4(a, 1);
        mesh.vPos4f[i * 3 + 1] = LiteMath::to_float4(b, 1);
        mesh.vPos4f[i * 3 + 2] = LiteMath::to_float4(c, 1);

        mesh.vNorm4f[i * 3 + 0] = LiteMath::to_float4(a_normalsType == NormalsType::VERTEX ? normals[tri.x] : geom_normal, 0);
        mesh.vNorm4f[i * 3 + 1] = LiteMath::to_float4(a_normalsType == NormalsType::VERTEX ? normals[tri.y] : geom_normal, 0);
        mesh.vNorm4f[i * 3 + 2] = LiteMath::to_float4(a_normalsType == NormalsType::VERTEX ? normals[tri.z] : geom_normal, 0);

        mesh.vTang4f[i * 3 + 0] = LiteMath::to_float4(a_normalsType == NormalsType::VERTEX ? tangents[tri.x] : geom_tangent, 0);
        mesh.vTang4f[i * 3 + 1] = LiteMath::to_float4(a_normalsType == NormalsType::VERTEX ? tangents[tri.y] : geom_tangent, 0);
        mesh.vTang4f[i * 3 + 2] = LiteMath::to_float4(a_normalsType == NormalsType::VERTEX ? tangents[tri.z] : geom_tangent, 0);

        mesh.vTexCoord2f[i * 3 + 0] = a_vertices[tri.x].tc;
        mesh.vTexCoord2f[i * 3 + 1] = a_vertices[tri.y].tc;
        mesh.vTexCoord2f[i * 3 + 2] = a_vertices[tri.z].tc;

        mesh.indices[i * 3 + 0] = i * 3 + 0;
        mesh.indices[i * 3 + 1] = i * 3 + 1;
        mesh.indices[i * 3 + 2] = i * 3 + 2;

        mesh.matIndices[i] = materials[i];
      }
    }

    cmesh4::compress_close_vertices(mesh, 1e-6f, false);

    if (fix_normals)
      cmesh4::fix_normals(mesh, false);

    for (uint32_t i = 0; i < a_triangles.size(); i++)
    {
      float3 a = LiteMath::to_float3(mesh.vPos4f[mesh.indices[3 * i + 0]]);
      float3 b = LiteMath::to_float3(mesh.vPos4f[mesh.indices[3 * i + 1]]);
      float3 c = LiteMath::to_float3(mesh.vPos4f[mesh.indices[3 * i + 2]]);
      float3 n1 = LiteMath::cross(b - a, c - a);
      float3 n2 = (1.0f / 3.0f) * (LiteMath::to_float3(mesh.vNorm4f[mesh.indices[3 * i + 0]]) +
                                   LiteMath::to_float3(mesh.vNorm4f[mesh.indices[3 * i + 1]]) +
                                   LiteMath::to_float3(mesh.vNorm4f[mesh.indices[3 * i + 2]]));
      if (LiteMath::dot(n1, n2) < 0)
      {
        unsigned tmp = mesh.indices[3 * i + 1];
        mesh.indices[3 * i + 1] = mesh.indices[3 * i + 2];
        mesh.indices[3 * i + 2] = tmp;
      }
    }

    return mesh;
  }

  SimpleMesh create_cube(float3 scale, bool six_materials, VerticesType a_verticesType, NormalsType a_normalsType)
  {
    float3 c(0,0,0);

    std::vector<RawVertex> vertices = {
      RawVertex(c + float3(-1,-1,-1) * scale, float2(0,0)),
      RawVertex(c + float3(1,-1,-1) * scale, float2(1,0)),
      RawVertex(c + float3(1,1,-1) * scale, float2(1,1)),
      RawVertex(c + float3(-1,1,-1) * scale, float2(0,1)),

      RawVertex(c + float3(-1,-1,1) * scale, float2(0,0)),
      RawVertex(c + float3(1,-1,1) * scale, float2(1,0)),
      RawVertex(c + float3(1,1,1) * scale, float2(1,1)),
      RawVertex(c + float3(-1,1,1) * scale, float2(0,1)),

      RawVertex(c + float3(-1,-1,-1) * scale, float2(0,0)),
      RawVertex(c + float3(-1,1,-1) * scale, float2(1,0)),
      RawVertex(c + float3(-1,1,1) * scale, float2(1,1)),
      RawVertex(c + float3(-1,-1,1) * scale, float2(0,1)),

      RawVertex(c + float3(1,-1,-1) * scale, float2(0,0)),
      RawVertex(c + float3(1,1,-1) * scale, float2(1,0)),
      RawVertex(c + float3(1,1,1) * scale, float2(1,1)),
      RawVertex(c + float3(1,-1,1) * scale, float2(0,1)),

      RawVertex(c + float3(-1,-1,-1) * scale, float2(0,0)),
      RawVertex(c + float3(1,-1,-1) * scale, float2(1,0)),
      RawVertex(c + float3(1,-1,1) * scale, float2(1,1)),
      RawVertex(c + float3(-1,-1,1) * scale, float2(0,1)),

      RawVertex(c + float3(-1,1,-1) * scale, float2(0,0)),
      RawVertex(c + float3(1,1,-1) * scale, float2(1,0)),
      RawVertex(c + float3(1,1,1) * scale, float2(1,1)),
      RawVertex(c + float3(-1,1,1) * scale, float2(0,1)),
    };

    std::vector<uint3> triangles = {
      uint3(0,1,2),
      uint3(0,2,3),

      uint3(4,5,6),
      uint3(4,6,7),

      uint3(8,9,10),
      uint3(8,10,11),

      uint3(12,13,14),
      uint3(12,14,15),

      uint3(16,17,18),
      uint3(16,18,19),
      
      uint3(20,21,22),
      uint3(20,22,23),
    };

    std::vector<uint32_t> materials;
    if (six_materials)
    {
      materials = {1,1, 2,2, 3,3, 4,4, 5,5, 6,6};
    }
    else
    {
      materials = {0,0,0,0,0,0,0,0,0,0,0,0};
    }

    set_normals_around_center(vertices, triangles);
    return create_mesh(vertices, triangles, materials, a_verticesType, a_normalsType, true, c);
  }

  SimpleMesh create_box(float3 scale, float thickness, VerticesType a_verticesType, NormalsType a_normalsType)
  {
    float3 c(0,0,0);

    float d = thickness;
    std::vector<RawVertex> vertices = {
      RawVertex(c + float3(-1,-1,-1) * scale),
      RawVertex(c + float3(1,-1,-1) * scale),
      RawVertex(c + float3(1,1,-1) * scale),
      RawVertex(c + float3(-1,1,-1) * scale),

      RawVertex(c + float3(-1,-1,1) * scale),
      RawVertex(c + float3(1,-1,1) * scale),
      RawVertex(c + float3(1,1,1) * scale),
      RawVertex(c + float3(-1,1,1) * scale),


      RawVertex(c + float3(-1,-1,-1) * scale + float3(0,-d,-d)),
      RawVertex(c + float3(1,-1,-1) * scale + float3(0,-d,-d)),
      RawVertex(c + float3(1,1,-1) * scale + float3(0,d,-d)),
      RawVertex(c + float3(-1,1,-1) * scale + float3(0,d,-d)),

      RawVertex(c + float3(-1,-1,1) * scale + float3(0,-d,d)),
      RawVertex(c + float3(1,-1,1) * scale + float3(0,-d,d)),
      RawVertex(c + float3(1,1,1) * scale + float3(0,d,d)),
      RawVertex(c + float3(-1,1,1) * scale + float3(0,d,d)),
    };

    std::vector<uint3> triangles = {
      uint3(0,1,2),
      uint3(0,2,3),

      uint3(4,5,6),
      uint3(4,6,7),

      uint3(0,5,1),
      uint3(0,4,5),

      uint3(2,6,3),
      uint3(6,7,3),

      uint3(8,9,10),
      uint3(8,10,11),

      uint3(12,13,14),
      uint3(12,14,15),

      uint3(8,13,9),
      uint3(8,12,13),

      uint3(10,14,11),
      uint3(14,15,11),

      uint3(1,2,10),
      uint3(1,10,9),

      uint3(5,1,9),
      uint3(5,9,13),

      uint3(6,5,13),
      uint3(6,13,14),

      uint3(2,6,14),
      uint3(2,14,10),

      uint3(3,0,8),
      uint3(3,8,11),

      uint3(0,4,12),
      uint3(0,12,8),

      uint3(4,7,15),
      uint3(4,15,12),

      uint3(7,3,11),
      uint3(7,11,15)
    };

    std::vector<uint32_t> materials(triangles.size(), 0);

    return create_mesh(vertices, triangles, materials, a_verticesType, a_normalsType, true, c);
  }

  SimpleMesh create_cylinder(float3 scale, unsigned c_segments, unsigned h_segments, 
                                     VerticesType a_verticesType, NormalsType a_normalsType)
  {
    float3 c(0,0,0);
    float d = scale.y;
    float r = std::min(scale.x, scale.z);
    float delta = 0.0;

    std::vector<RawVertex> vertices;
    std::vector<uint3> triangles;
    unsigned prev_start = 0;
    for (int i=0;i<=h_segments;i++)
    {
      unsigned start = vertices.size();

      for (int j=0;j<c_segments;j++)
      {
        float angle = float(j) / float(c_segments) * 2.0f * LiteMath::M_PI;
        float x = r*std::cos(angle);
        float y = d*(-1 + 2*float(i) / float(h_segments));
        float z = r*std::sin(angle);

        vertices.push_back(RawVertex(c + float3(x,y,z), float2(angle/(2*LiteMath::M_PI),0.25 + 0.5*float(i) / float(h_segments))));
      }

      if (i == 0 || i == h_segments)
      {
        //smooth border from side to cap
        for (int j=0;j<c_segments;j++)
          vertices.push_back(RawVertex(vertices[start + j].pos*float3(1-delta,1+delta,1-delta), vertices[start + j].tc + float2(0,delta*(i-0.5))));
        for (int j=1;j<=c_segments;j++)
        {
          //triangles.push_back(uint3(start + j - 1, start + j%c_segments, start+c_segments + j - 1));
          //triangles.push_back(uint3(start + j%c_segments, start+c_segments + j%c_segments, start+c_segments + j - 1));
        }

        if (i == 0)
          vertices.push_back(RawVertex(c + float3(0,-d*(1+delta),0), float2(0.5,0)));
        else
          vertices.push_back(RawVertex(c + float3(0,d*(1+delta),0), float2(0.5,1.0)));
      
        for (int j=1;j<=c_segments;j++)
        {
          triangles.push_back(uint3(start + c_segments + j - 1, start + c_segments + j%c_segments, vertices.size()-1));
        }
      }

      if (i > 0)
      {
        for (int j=1;j<=c_segments;j++)
        {
          triangles.push_back(uint3(prev_start + j - 1, prev_start + j%c_segments, start + j - 1));
          triangles.push_back(uint3(prev_start + j%c_segments, start + j%c_segments, start + j - 1));
        }
      }
      prev_start = start;
    }


    std::vector<uint32_t> materials(triangles.size(), 0);
    return create_mesh(vertices, triangles, materials, a_verticesType, a_normalsType, true, c);
  }

  SimpleMesh create_tube(float r_outer, float d, unsigned c_segments, unsigned h_segments, 
                                     VerticesType a_verticesType, NormalsType a_normalsType)
  {
    float3 c(0,0,0);

    std::vector<RawVertex> vertices;
    std::vector<uint3> triangles;
    unsigned prev_start = 0;
    for (int i=0;i<=h_segments;i++)
    {
      unsigned start = vertices.size();

      for (int j=0;j<c_segments;j++)
      {
        float angle = float(j) / float(c_segments) * 2.0f * LiteMath::M_PI;
        float x = r_outer*std::cos(angle);
        float y = -1 + 2*float(i) / float(h_segments);
        float z = r_outer*std::sin(angle);

        vertices.push_back(RawVertex(c + float3(x,y,z)));
      }

      for (int j=0;j<c_segments;j++)
      {
        float angle = float(j) / float(c_segments) * 2.0f * LiteMath::M_PI;
        float x = (r_outer-d)*std::cos(angle);
        float y = -1 + 2*float(i) / float(h_segments);
        float z = (r_outer-d)*std::sin(angle);

        vertices.push_back(RawVertex(c + float3(x,y,z)));
      }

      if (i == 0 || i == h_segments)
      {
        for (int j=1;j<=c_segments;j++)
        {
          triangles.push_back(uint3(start + j - 1, start + j%c_segments, start + c_segments + j - 1));
          triangles.push_back(uint3(start + j%c_segments, start + c_segments + j%c_segments, start + c_segments + j - 1));
        }
      }

      if (i > 0)
      {
        for (int j=1;j<=c_segments;j++)
        {
          triangles.push_back(uint3(prev_start + j - 1, prev_start + j%c_segments, start + j - 1));
          triangles.push_back(uint3(prev_start + j%c_segments, start + j%c_segments, start + j - 1));

          triangles.push_back(uint3(prev_start + c_segments + j - 1, prev_start + c_segments + j%c_segments, start + c_segments + j - 1));
          triangles.push_back(uint3(prev_start + c_segments + j%c_segments, start + c_segments + j%c_segments, start + c_segments + j - 1));
        }
      }
      prev_start = start;
    }


    std::vector<uint32_t> materials(triangles.size(), 0);
    return create_mesh(vertices, triangles, materials, a_verticesType, a_normalsType, true, c);
  }

  SimpleMesh create_bent_tube(float r_outer, float d, float3 line[], unsigned line_size, unsigned c_segments, unsigned h_segments, 
                                     VerticesType a_verticesType, NormalsType a_normalsType)
  {
    float3 c(0,0,0);

    std::vector<RawVertex> vertices;
    std::vector<uint3> triangles;
    unsigned prev_start = 0;
    for (int l=1; l<line_size; l++)
    {
      float3 frag_dir = line[l] - line[l-1];
      for (int i=0;i<=h_segments;i++)
      {
        if (l != 1 && i == 0)
          continue;
        unsigned start = vertices.size();
        float3 offset = frag_dir * float(i) / float(h_segments);

        for (int j=0;j<c_segments;j++)
        {
          float angle = float(j) / float(c_segments) * 2.0f * LiteMath::M_PI;
          float x = line[l-1].x + offset.x + r_outer*std::cos(angle);
          float y = line[l-1].y + offset.y;
          float z = line[l-1].z + offset.z + r_outer*std::sin(angle);

          vertices.push_back(RawVertex(c + float3(x,y,z)));
        }

        for (int j=0;j<c_segments;j++)
        {
          float angle = float(j) / float(c_segments) * 2.0f * LiteMath::M_PI;
          float x = line[l-1].x + offset.x + (r_outer-d)*std::cos(angle);
          float y = line[l-1].y + offset.y;
          float z = line[l-1].z + offset.z + (r_outer-d)*std::sin(angle);

          vertices.push_back(RawVertex(c + float3(x,y,z)));
        }

        if ((l == 1 && i == 0) || (l == line_size - 1 && i == h_segments))
        {
          for (int j=1;j<=c_segments;j++)
          {
            triangles.push_back(uint3(start + j - 1, start + j%c_segments, start + c_segments + j - 1));
            triangles.push_back(uint3(start + j%c_segments, start + c_segments + j%c_segments, start + c_segments + j - 1));
          }
        }

        if (i > 0)
        {
          for (int j=1;j<=c_segments;j++)
          {
            triangles.push_back(uint3(prev_start + j - 1, prev_start + j%c_segments, start + j - 1));
            triangles.push_back(uint3(prev_start + j%c_segments, start + j%c_segments, start + j - 1));

            triangles.push_back(uint3(prev_start + c_segments + j - 1, prev_start + c_segments + j%c_segments, start + c_segments + j - 1));
            triangles.push_back(uint3(prev_start + c_segments + j%c_segments, start + c_segments + j%c_segments, start + c_segments + j - 1));
          }
        }
        prev_start = start;
      }
    }

    std::vector<uint32_t> materials(triangles.size(), 0);
    return create_mesh(vertices, triangles, materials, a_verticesType, a_normalsType, true, c);
  }

  SimpleMesh create_plane(VerticesType a_verticesType, NormalsType a_normalsType)
  {
    std::vector<RawVertex> vertices = {RawVertex(-1,0,-1), RawVertex(-1,0, 1), RawVertex( 1,0,-1), RawVertex( 1,0, 1)};
    std::vector<uint3> triangles = {uint3(0,1,2), uint3(2,1,3)};
    std::vector<uint32_t> materials(triangles.size(), 0);
    return create_mesh(vertices, triangles, materials, a_verticesType, a_normalsType);
  }

  SimpleMesh create_two_planes(float thickness, VerticesType a_verticesType, NormalsType a_normalsType)
  {
    float d = 0.5f*thickness;
    std::vector<RawVertex> vertices = {RawVertex(-1,-d,-1), RawVertex(-1,-d, 1), RawVertex( 1,-d,-1), RawVertex( 1,-d, 1),
                                       RawVertex(-1, d,-1), RawVertex(-1, d, 1), RawVertex( 1, d,-1), RawVertex( 1, d, 1)};
    std::vector<uint3> triangles = {uint3(0,2,1), uint3(2,3,1), uint3(4,5,6), uint3(6,5,7)};
    std::vector<uint32_t> materials(triangles.size(), 0);
    return create_mesh(vertices, triangles, materials, a_verticesType, a_normalsType);
  }

  SimpleMesh create_thin_list(float thickness, VerticesType a_verticesType, NormalsType a_normalsType)
  {
    float d = 0.5f*thickness;
    std::vector<RawVertex> vertices = {RawVertex(-1,-d,-1), RawVertex(-1,-d, 1), RawVertex( 1,-d,-1), RawVertex( 1,-d, 1),
                                       RawVertex(-1, d,-1), RawVertex(-1, d, 1), RawVertex( 1, d,-1), RawVertex( 1, d, 1)};
    std::vector<uint3> triangles = {uint3(0,2,1), uint3(2,3,1), uint3(4,5,6), uint3(6,5,7),
                                    uint3(0,4,6), uint3(6,2,0), uint3(0,1,4), uint3(4,1,5),
                                    uint3(7,5,1), uint3(3,7,1), uint3(2,6,7), uint3(3,2,7)};
    std::vector<uint32_t> materials(triangles.size(), 0);
    return create_mesh(vertices, triangles, materials, a_verticesType, a_normalsType);
  }

  SimpleMesh create_dented_list(float d1, float d2, float l1, float l2, VerticesType a_verticesType, NormalsType a_normalsType)
  {
    float d = 0.5f*d1;
    std::vector<RawVertex> vertices = {RawVertex( 1,-d,-1), RawVertex( 1,-d, 1), RawVertex(-1,-d,-1), RawVertex(-1,-d, 1), 
                                       RawVertex( 1, d,-1), RawVertex( 1, d, 1), RawVertex(-1, d,-1), RawVertex(-1, d, 1)};
    std::vector<uint3> triangles = {uint3(0,2,1), uint3(2,3,1),
                                    uint3(0,1,4), uint3(4,1,5),
                                    uint3(2,6,7), uint3(3,2,7)};
    unsigned dents = std::floor((2.0f - l2) / (l1 + l2));
    
    for (int i=0;i<dents;i++)
    {
      unsigned s = vertices.size();
      vertices.insert(vertices.end(), {RawVertex(-1+l2+i*(l1+l2),d,-1), RawVertex(-1+l2+i*(l1+l2),d,1),
                                       RawVertex(-1+l2+i*(l1+l2),d-d2,-1), RawVertex(-1+l2+i*(l1+l2),d-d2,1),
                                       RawVertex(-1+l2+i*(l1+l2),-d,-1), RawVertex(-1+l2+i*(l1+l2),-d,1),
                                       RawVertex(-1+(i+1)*(l1+l2),-d,-1), RawVertex(-1+(i+1)*(l1+l2),-d,1),
                                       RawVertex(-1+(i+1)*(l1+l2),d-d2,-1), RawVertex(-1+(i+1)*(l1+l2),d-d2,1),
                                       RawVertex(-1+(i+1)*(l1+l2),d,-1), RawVertex(-1+(i+1)*(l1+l2),d,1)
                                      });
      triangles.insert(triangles.end(), {uint3(s-2,s+1,s-1), uint3(s-2,s,s+1),
                                         uint3(s,s+3,s+1), uint3(s,s+2,s+3),
                                         uint3(s+2,s+9,s+3), uint3(s+2,s+8,s+9),
                                         uint3(s+8,s+11,s+9), uint3(s+8,s+10,s+11),

                                        uint3(s-6,s+4,s), uint3(s-6,s,s-2),
                                        uint3(s-5,s-1,s+1), uint3(s-5,s+1,s+5),
                                        uint3(s+4,s+8,s+2), uint3(s+4,s+6,s+8),
                                        uint3(s+5,s+3,s+9), uint3(s+5,s+9,s+7)
                                        });
    }
    unsigned s = vertices.size();
    triangles.insert(triangles.end(), {uint3(s-2,5,s-1), uint3(s-2,4,5),
                                        uint3(s-6,0,4), uint3(s-6,4,s-2),
                                        uint3(s-5,s-1,5), uint3(s-5,5,1),
                                      });
    std::vector<uint32_t> materials(triangles.size(), 0);
    return create_mesh(vertices, triangles, materials, a_verticesType, a_normalsType);
  }

  SimpleMesh create_n_lists(unsigned n, float d1, float d2, VerticesType a_verticesType, NormalsType a_normalsType)
  {
    std::vector<RawVertex> vertices;
    std::vector<uint3> triangles;
    float d = 0.5f*d1;
    for (int i=0;i<n;i++)
    {
      float offs = (d1 + d2) * (float(n-1)/2 - i);
      unsigned s = vertices.size();
      vertices.insert(vertices.end(), {RawVertex(-1, offs - d, -1), RawVertex(-1, offs - d, 1), RawVertex(1, offs - d, -1), RawVertex(1, offs - d, 1),
                                       RawVertex(-1, offs + d, -1), RawVertex(-1, offs + d, 1), RawVertex(1, offs + d, -1), RawVertex(1, offs + d, 1)});
      triangles.insert(triangles.end(), {uint3(s,s+2,s+1), uint3(s+2,s+3,s+1), uint3(s+4,s+5,s+6), uint3(s+6,s+5,s+7),
                                      uint3(s+0,s+4,s+6), uint3(s+6,s+2,s+0), uint3(s+0,s+1,s+4), uint3(s+4,s+1,s+5),
                                      uint3(s+7,s+5,s+1), uint3(s+3,s+7,s+1), uint3(s+2,s+6,s+7), uint3(s+3,s+2,s+7)});
    }
    std::vector<uint32_t> materials(triangles.size(), 0);
    return create_mesh(vertices, triangles, materials, a_verticesType, a_normalsType);
  }

  SimpleMesh create_variably_thin_list(float d1, float d2, VerticesType a_verticesType, NormalsType a_normalsType)
  {
    std::vector<RawVertex> vertices = {RawVertex(-1,-d1/2,-1), RawVertex(-1,-d1/2, 1), RawVertex( 1,-d2/2,-1), RawVertex( 1,-d2/2, 1),
                                       RawVertex(-1, d1/2,-1), RawVertex(-1, d1/2, 1), RawVertex( 1, d2/2,-1), RawVertex( 1, d2/2, 1)};
    std::vector<uint3> triangles = {uint3(0,2,1), uint3(2,3,1), uint3(4,5,6), uint3(6,5,7),
                                    uint3(0,4,6), uint3(6,2,0), uint3(0,1,4), uint3(4,1,5),
                                    uint3(7,5,1), uint3(3,7,1), uint3(2,6,7), uint3(3,2,7)};
    std::vector<uint32_t> materials(triangles.size(), 0);
    return create_mesh(vertices, triangles, materials, a_verticesType, a_normalsType);
  }

  SimpleMesh create_sin_list(float thickness, int tiles, float wavelength, float amplitude, bool two_dim, 
                             VerticesType a_verticesType, NormalsType a_normalsType)
  {
    float d = 0.5f*thickness;
    float tile_size = 2.0f/tiles;
    int l_off = (tiles+1)*(tiles+1);
    std::vector<RawVertex> vertices;
    std::vector<uint3> triangles;

    vertices.resize(2*l_off);
    for (int h = 0; h < 2; h++)
    {
      for (int y=0;y<=tiles;y++)
      {
        for (int x=0;x<=tiles;x++)
        {
          float f;
          if (two_dim)
            f = std::sin(float(x) / float(tiles) * 4 * LiteMath::M_PI / wavelength) * std::sin(float(y) / float(tiles) * 4 * LiteMath::M_PI / wavelength);
          else
            f = std::sin(float(x) / float(tiles) * 4 * LiteMath::M_PI / wavelength);
          vertices[h*l_off + y*(tiles+1) + x].pos = float3(-1 + (2.0f*x)/tiles, d*(2*h - 1) + amplitude*f, -1 + (2.0f*y)/tiles);
        }        
      }
    }

    triangles.resize(2*2*tiles*tiles);
    for (int y=0;y<tiles;y++)
    {
      for (int x=0;x<tiles;x++)
      {
        triangles[2*(0*tiles*tiles + y*tiles + x) + 0] = uint3(y*(tiles+1) + x, (y+1)*(tiles+1) + x, y*(tiles+1) + x+1);
        triangles[2*(0*tiles*tiles + y*tiles + x) + 1] = uint3((y+1)*(tiles+1) + x, (y+1)*(tiles+1) + x+1, y*(tiles+1) + x+1);
      }        
    }
    for (int y=0;y<tiles;y++)
    {
      for (int x=0;x<tiles;x++)
      {
        triangles[2*(1*tiles*tiles + y*tiles + x) + 0] = uint3(l_off + y*(tiles+1) + x, l_off + y*(tiles+1) + x+1, l_off + (y+1)*(tiles+1) + x);
        triangles[2*(1*tiles*tiles + y*tiles + x) + 1] = uint3(l_off + (y+1)*(tiles+1) + x, l_off + y*(tiles+1) + x+1, l_off + (y+1)*(tiles+1) + x+1);
      }        
    }

    for (int x=0;x<tiles;x++)
    {
      triangles.push_back(uint3(x, x+1, l_off+x));
      triangles.push_back(uint3(x+1, l_off+x+1, l_off+x));

      triangles.push_back(uint3(tiles*(tiles+1)+x, tiles*(tiles+1)+l_off+x, tiles*(tiles+1)+x+1));
      triangles.push_back(uint3(tiles*(tiles+1)+x+1, tiles*(tiles+1)+l_off+x, tiles*(tiles+1)+l_off+x+1));
    }

    for (int y=0;y<tiles;y++)
    {
      triangles.push_back(uint3(y*(tiles+1), l_off+y*(tiles+1), (y+1)*(tiles+1)));
      triangles.push_back(uint3(l_off+y*(tiles+1), l_off+(y+1)*(tiles+1), (y+1)*(tiles+1)));

      triangles.push_back(uint3(y*(tiles+1)+tiles, (y+1)*(tiles+1)+tiles, l_off+y*(tiles+1)+tiles));
      triangles.push_back(uint3(l_off+y*(tiles+1)+tiles, (y+1)*(tiles+1)+tiles, l_off+(y+1)*(tiles+1)+tiles));
    }

    std::vector<uint32_t> materials(triangles.size(), 0);
    printf("tt %d %d\n", (int)vertices.size(), (int)triangles.size());
    return create_mesh(vertices, triangles, materials, a_verticesType, a_normalsType);
  }

  SimpleMesh create_two_tiled_planes(float thickness, int tiles, VerticesType a_verticesType, NormalsType a_normalsType)
  {
    float d = 0.5f*thickness;
    float tile_size = 2.0f/tiles;
    int l_off = (tiles+1)*(tiles+1);
    std::vector<RawVertex> vertices;
    std::vector<uint3> triangles;

    vertices.resize(2*l_off);
    for (int h = 0; h < 2; h++)
    {
      for (int y=0;y<=tiles;y++)
      {
        for (int x=0;x<=tiles;x++)
        {
          vertices[h*l_off + y*(tiles+1) + x].pos = float3(-1 + (2.0f*x)/tiles, d*(2*h - 1), -1 + (2.0f*y)/tiles);
        }        
      }
    }

    triangles.resize(2*2*tiles*tiles);
    for (int y=0;y<tiles;y++)
    {
      for (int x=0;x<tiles;x++)
      {
        triangles[2*(0*tiles*tiles + y*tiles + x) + 0] = uint3(y*(tiles+1) + x, (y+1)*(tiles+1) + x, y*(tiles+1) + x+1);
        triangles[2*(0*tiles*tiles + y*tiles + x) + 1] = uint3((y+1)*(tiles+1) + x, (y+1)*(tiles+1) + x+1, y*(tiles+1) + x+1);
      }        
    }
    for (int y=0;y<tiles;y++)
    {
      for (int x=0;x<tiles;x++)
      {
        triangles[2*(1*tiles*tiles + y*tiles + x) + 0] = uint3(l_off + y*(tiles+1) + x, l_off + y*(tiles+1) + x+1, l_off + (y+1)*(tiles+1) + x);
        triangles[2*(1*tiles*tiles + y*tiles + x) + 1] = uint3(l_off + (y+1)*(tiles+1) + x, l_off + y*(tiles+1) + x+1, l_off + (y+1)*(tiles+1) + x+1);
      }        
    }
    std::vector<uint32_t> materials(triangles.size(), 0);
    printf("tt %d %d\n", (int)vertices.size(), (int)triangles.size());
    return create_mesh(vertices, triangles, materials, a_verticesType, a_normalsType);
  }

  float cubicInterpolate(float p[4], float x) 
  {
    return p[1] + 0.5f * x*(p[2] - p[0] + x*(2.0f*p[0] - 5.0f*p[1] + 4.0f*p[2] - p[3] + x*(3.0f*(p[1] - p[2]) + p[3] - p[0])));
  }

  SimpleMesh create_random_terrain(uint32_t seed, int heightmap_resolution, int tiles, float amplitude, bool is_two_sided, float thickness,
                                   VerticesType a_verticesType, NormalsType a_normalsType)
  {
    std::vector<RawVertex> vertices;
    std::vector<uint3> triangles;

    // Create random heightmap with normal distribution
    std::default_random_engine generator(seed);
    std::normal_distribution<float> distribution(0.0f, amplitude);

    uint32_t hr = heightmap_resolution;
    std::vector<float> heightmap(hr*hr, 0.0f);
    for (int i = 0; i < heightmap_resolution; ++i) {
        for (int j = 0; j < heightmap_resolution; ++j) {
            heightmap[i*hr + j] = distribution(generator);
        }
    }

    // Function to safely get heightmap value with clamping
    auto getHeight = [&](int x, int y) -> float {
        x = std::max(0, std::min(x, heightmap_resolution - 1));
        y = std::max(0, std::min(y, heightmap_resolution - 1));
        return heightmap[x*hr + y];
    };

    // Bicubic sampling function for heightmap
    auto sampleHeight = [&](float x, float y) -> float {
        int ix = (int)std::floor(x);
        int iy = (int)std::floor(y);
        float fx = x - ix;
        float fy = y - iy;

        float patch[4][4];
        for (int m = -1; m <= 2; ++m) {
            for (int n = -1; n <= 2; ++n) {
                patch[m+1][n+1] = getHeight(ix + m, iy + n);
            }
        }
        
        float arr[4];
        for (int i = 0; i < 4; i++)
            arr[i] = cubicInterpolate(patch[i], fy);
        return cubicInterpolate(arr, fx);
    };

    // Create grid of points for the top layer
    std::vector<RawVertex> top_vertices;
    for (int i = 0; i < tiles; ++i) {
        for (int j = 0; j < tiles; ++j) {
            float x = (float)i / (tiles - 1);
            float y = (float)j / (tiles - 1);
            // Map grid coordinates to heightmap coordinates
            float hm_x = x * (heightmap_resolution - 1);
            float hm_y = y * (heightmap_resolution - 1);
            float height = sampleHeight(hm_x, hm_y);
            top_vertices.push_back(RawVertex{float3(x, height, y)});
        }
    }

    // If two-sided, create bottom vertices by offsetting the top vertices
    std::vector<RawVertex> bottom_vertices;
    if (is_two_sided) {
        for (const auto& vertex : top_vertices) {
            bottom_vertices.push_back(RawVertex{float3(vertex.pos.x, vertex.pos.y - thickness, vertex.pos.z)});
        }
    }

    // Combine top and bottom vertices
    vertices.insert(vertices.end(), top_vertices.begin(), top_vertices.end());
    if (is_two_sided) {
        vertices.insert(vertices.end(), bottom_vertices.begin(), bottom_vertices.end());
    }

    // Create triangles for the top layer
    for (int i = 0; i < tiles - 1; ++i) {
        for (int j = 0; j < tiles - 1; ++j) {
            uint32_t idx0 = i * tiles + j;
            uint32_t idx1 = (i + 1) * tiles + j;
            uint32_t idx2 = (i + 1) * tiles + (j + 1);
            uint32_t idx3 = i * tiles + (j + 1);

            // Two triangles per quad
            triangles.push_back(uint3{idx0, idx2, idx1});
            triangles.push_back(uint3{idx0, idx3, idx2});
        }
    }

    // If two-sided, create triangles for the bottom layer (in reverse order)
    if (is_two_sided) {
        uint32_t offset = top_vertices.size();
        for (int i = 0; i < tiles - 1; ++i) {
            for (int j = 0; j < tiles - 1; ++j) {
                uint32_t idx0 = offset + i * tiles + j;
                uint32_t idx1 = offset + (i + 1) * tiles + j;
                uint32_t idx2 = offset + (i + 1) * tiles + (j + 1);
                uint32_t idx3 = offset + i * tiles + (j + 1);

                // Two triangles per quad (reversed)
                triangles.push_back(uint3{idx0, idx1, idx2});
                triangles.push_back(uint3{idx0, idx2, idx3});
            }
      }

      for (int i=0;i<tiles-1;i++)
      {
        int j = 0;
        uint32_t idx0 = i * tiles + j;
        uint32_t idx1 = (i + 1) * tiles + j;
        uint32_t idx2 = offset + i * tiles + j;
        uint32_t idx3 = offset + (i + 1) * tiles + j;
        triangles.push_back(uint3{idx0, idx3, idx2});
        triangles.push_back(uint3{idx0, idx1, idx3});
      }
      for (int i=0;i<tiles-1;i++)
      {
        int j = tiles-1;
        uint32_t idx0 = i * tiles + j;
        uint32_t idx1 = (i + 1) * tiles + j;
        uint32_t idx2 = offset + i * tiles + j;
        uint32_t idx3 = offset + (i + 1) * tiles + j;
        triangles.push_back(uint3{idx0, idx2, idx3});
        triangles.push_back(uint3{idx0, idx3, idx1});
      }
      for (int j=0;j<tiles-1;j++)
      {
        int i = 0;
        uint32_t idx0 = i * tiles + j;
        uint32_t idx1 = i * tiles + j + 1;
        uint32_t idx2 = offset + i * tiles + j;
        uint32_t idx3 = offset + i * tiles + j + 1;
        triangles.push_back(uint3{idx0, idx2, idx3});
        triangles.push_back(uint3{idx0, idx3, idx1});
      }
      for (int j=0;j<tiles-1;j++)
      {
        int i = tiles-1;
        uint32_t idx0 = i * tiles + j;
        uint32_t idx1 = i * tiles + j + 1;
        uint32_t idx2 = offset + i * tiles + j;
        uint32_t idx3 = offset + i * tiles + j + 1;
        triangles.push_back(uint3{idx0, idx3, idx2});
        triangles.push_back(uint3{idx0, idx1, idx3});
      }
    }

    std::vector<uint32_t> materials(triangles.size(), 0);
    return create_mesh(vertices, triangles, materials, a_verticesType, a_normalsType);
  }

  SimpleMesh create_sphere(unsigned segments, VerticesType a_verticesType, NormalsType a_normalsType)
  {
    // Ensure minimum segments requirement
    if (segments < 4)
      segments = 4;

    float3 center(0, 0, 0);
    float radius = 1.0f;

    std::vector<RawVertex> vertices;
    std::vector<uint3> triangles;

    // Create vertices
    // Top pole vertex
    vertices.push_back(RawVertex(center + float3(0, radius, 0), float2(0.5f, 0.0f)));

    // Create rings of vertices from top to bottom
    for (unsigned i = 1; i < segments; i++)
    {
      float theta = float(i) / float(segments) * LiteMath::M_PI; // Vertical angle from top (0) to bottom (PI)
      float sin_theta = std::sin(theta);
      float cos_theta = std::cos(theta);
      float v = float(i) / float(segments); // Texture coordinate V

      for (unsigned j = 0; j < segments; j++)
      {
        float phi = float(j) / float(segments) * 2.0f * LiteMath::M_PI; // Horizontal angle
        float sin_phi = std::sin(phi);
        float cos_phi = std::cos(phi);
        float u = float(j) / float(segments); // Texture coordinate U

        // Spherical coordinates to Cartesian
        float x = radius * sin_theta * cos_phi;
        float y = radius * cos_theta;
        float z = radius * sin_theta * sin_phi;

        vertices.push_back(RawVertex(center + float3(x, y, z), float2(u, v)));
      }
    }

    // Bottom pole vertex
    vertices.push_back(RawVertex(center + float3(0, -radius, 0), float2(0.5f, 1.0f)));

    // Create triangles
    // Top cap (connecting to top pole)
    unsigned top_pole = 0;
    unsigned first_ring_start = 1;
    for (unsigned j = 0; j < segments; j++)
    {
      unsigned next = (j + 1) % segments;
      triangles.push_back(uint3(top_pole, first_ring_start + next, first_ring_start + j));
    }

    // Middle rings
    for (unsigned i = 0; i < segments - 2; i++)
    {
      unsigned current_ring_start = 1 + i * segments;
      unsigned next_ring_start = 1 + (i + 1) * segments;

      for (unsigned j = 0; j < segments; j++)
      {
        unsigned next = (j + 1) % segments;

        // Two triangles per quad
        triangles.push_back(uint3(current_ring_start + j, current_ring_start + next, next_ring_start + j));
        triangles.push_back(uint3(current_ring_start + next, next_ring_start + next, next_ring_start + j));
      }
    }

    // Bottom cap (connecting to bottom pole)
    unsigned bottom_pole = vertices.size() - 1;
    unsigned last_ring_start = 1 + (segments - 2) * segments;
    for (unsigned j = 0; j < segments; j++)
    {
      unsigned next = (j + 1) % segments;
      triangles.push_back(uint3(bottom_pole, last_ring_start + j, last_ring_start + next));
    }

    // Create materials (all triangles use material 0)
    std::vector<uint32_t> materials(triangles.size(), 0);

    return create_mesh(vertices, triangles, materials, a_verticesType, a_normalsType, true, center);
  }

  static void orient_triangle_around_center(const std::vector<RawVertex>& a_vertices, uint3 &tri, float3 center)
  {
    float3 a = a_vertices[tri.x].pos;
    float3 b = a_vertices[tri.y].pos;
    float3 c = a_vertices[tri.z].pos;
    float3 normal = LiteMath::cross(b - a, c - a);
    if (LiteMath::dot(normal, a - center) < 0)
    {
      unsigned t = tri.z;
      tri.z = tri.y;
      tri.y = t;
    }
  }

  static void build_orthonormal_basis(const float3 &n, float3 &out_u, float3 &out_v)
  {
    float3 axis = (fabs(n.x) > fabs(n.y)) ? float3(0,1,0) : float3(1,0,0);
    out_u = LiteMath::normalize(LiteMath::cross(axis, n));
    out_v = LiteMath::normalize(LiteMath::cross(n, out_u));
  }

  static void append_tetrahedron(std::vector<RawVertex> &vertices, std::vector<uint3> &triangles,
                                 const float3 &p0, const float3 &p1, const float3 &p2, const float3 &p3)
  {
    unsigned base_idx = (unsigned)vertices.size();
    vertices.emplace_back(p0);
    vertices.emplace_back(p1);
    vertices.emplace_back(p2);
    vertices.emplace_back(p3);

    uint3 t0 = uint3{base_idx + 0, base_idx + 1, base_idx + 2};
    uint3 t1 = uint3{base_idx + 0, base_idx + 2, base_idx + 3};
    uint3 t2 = uint3{base_idx + 0, base_idx + 3, base_idx + 1};
    uint3 t3 = uint3{base_idx + 1, base_idx + 3, base_idx + 2};

    float3 centroid = (p0 + p1 + p2 + p3) * 0.25f;
    orient_triangle_around_center(vertices, t0, centroid);
    orient_triangle_around_center(vertices, t1, centroid);
    orient_triangle_around_center(vertices, t2, centroid);
    orient_triangle_around_center(vertices, t3, centroid);

    triangles.push_back(t0);
    triangles.push_back(t1);
    triangles.push_back(t2);
    triangles.push_back(t3);
  }

  static SimpleMesh create_from_point_cloud_internal(const float3 *points, int num_points, float point_size,
                                                    const float3 *normals, bool use_normals, float normal_length)
  {
    if (!points || num_points <= 0 || point_size <= 0.0f)
      return SimpleMesh();

    std::vector<RawVertex> vertices;
    std::vector<uint3> triangles;
    vertices.reserve(num_points * 4);
    triangles.reserve(num_points * 4);

    const float base_scale = point_size * 0.5f;
    const float3 base_positions[4] = {
      float3( 1,  1,  1) * base_scale,
      float3( 1, -1, -1) * base_scale,
      float3(-1,  1, -1) * base_scale,
      float3(-1, -1,  1) * base_scale
    };

    for (int i = 0; i < num_points; ++i)
    {
      const float3 p = points[i];
      if (!use_normals)
      {
        append_tetrahedron(vertices, triangles,
                           p + base_positions[0], p + base_positions[1],
                           p + base_positions[2], p + base_positions[3]);
      }
      else
      {
        float3 n = normals[i];
        float nlen = LiteMath::length(n);
        if (nlen < 1e-9f)
          n = float3(0, 1, 0);
        else
          n = n / nlen;

        float3 u, v;
        build_orthonormal_basis(n, u, v);
        float3 base0 = p + u * point_size;
        float3 base1 = p + (-0.5f * u + 0.8660254f * v) * point_size;
        float3 base2 = p + (-0.5f * u - 0.8660254f * v) * point_size;
        float3 tip   = p + n * normal_length;

        unsigned base_idx = (unsigned)vertices.size();
        vertices.emplace_back(base0);
        vertices.emplace_back(base1);
        vertices.emplace_back(base2);
        vertices.emplace_back(tip);

        uint3 t0 = uint3{base_idx + 0, base_idx + 2, base_idx + 1};
        uint3 t1 = uint3{base_idx + 0, base_idx + 1, base_idx + 3};
        uint3 t2 = uint3{base_idx + 1, base_idx + 2, base_idx + 3};
        uint3 t3 = uint3{base_idx + 2, base_idx + 0, base_idx + 3};

        float3 centroid = (base0 + base1 + base2 + tip) * 0.25f;
        orient_triangle_around_center(vertices, t0, centroid);
        orient_triangle_around_center(vertices, t1, centroid);
        orient_triangle_around_center(vertices, t2, centroid);
        orient_triangle_around_center(vertices, t3, centroid);

        triangles.push_back(t0);
        triangles.push_back(t1);
        triangles.push_back(t2);
        triangles.push_back(t3);
      }
    }

    std::vector<unsigned> materials(triangles.size(), 0);
    return create_mesh(vertices, triangles, materials, VerticesType::UNIQUE, NormalsType::GEOMETRY, false);
  }

  SimpleMesh create_from_point_cloud(const float3 *points, int num_points, float point_size)
  {
    return create_from_point_cloud_internal(points, num_points, point_size, nullptr, false, 0.0f);
  }

  SimpleMesh create_from_point_normal_cloud(const float3 *points, const float3 *normals, int num_points,
                                            float point_size, float normal_length)
  {
    if (!normals)
      return create_from_point_cloud(points, num_points, point_size);
    return create_from_point_cloud_internal(points, num_points, point_size, normals, true, normal_length);
  }

  SimpleMesh create_procedural_mesh_single(const Block &settings)
  {
    //common parameters for mesh generator
    std::string model    = settings.get_string("model");
    bool shared_vertices = settings.get_bool("shared_vertices", false);
    bool  vertex_normals = settings.get_bool("vertex_normals", false);
    float3 scale = settings.get_vec3("scale", float3(1,1,1));
    float rotate_x = settings.get_double("rotate_x", 0.0f);
    float rotate_y = settings.get_double("rotate_y", 0.0f);
    float rotate_z = settings.get_double("rotate_z", 0.0f);
    float4x4 transform = settings.get_mat4("transform", float4x4());

    VerticesType vt = shared_vertices ? VerticesType::SHARED : VerticesType::UNIQUE;
    NormalsType  nt = shared_vertices && vertex_normals ? NormalsType::VERTEX : NormalsType::GEOMETRY;

    SimpleMesh mesh;

    if (model == "cube")
    {
      bool six_materials = settings.get_bool("six_materials", false);

      mesh = create_cube(float3(1,1,1), six_materials, vt, nt);
    }
    else if (model == "cylinder")
    {
      int c_segments = settings.get_int("c_segments", 16);
      int h_segments = settings.get_int("h_segments", 16);

      mesh = create_cylinder(float3(1,1,1), c_segments, h_segments, vt, nt);
    }
    else if (model == "plane")
    {
      mesh = create_plane(vt, nt);
    }
    else if (model == "two_planes")
    {
      float thickness = settings.get_double("thickness", 0.01);
      mesh = create_two_planes(thickness, vt, nt);
    }
    else if (model == "two_tiled_planes")
    {
      float thickness = settings.get_double("thickness", 0.01);
      int tiles = settings.get_int("tiles", 4);
      mesh = create_two_tiled_planes(thickness, tiles, vt, nt);
    }
    else if (model == "box")
    {
      float thickness = settings.get_double("thickness", 0.01);
      mesh = create_box(float3(1,1,1), thickness, vt, nt);
    }
    else if (model == "tube")
    {
      float r_outer = settings.get_double("r_outer", 0.4);
      float d = settings.get_double("d", 0.001);
      unsigned c_segments = settings.get_int("c_segments", 5);
      unsigned h_segments = settings.get_int("h_segments", 7);
      mesh = create_tube(r_outer, d, c_segments, h_segments, vt, nt);
    }
    else if (model == "bent_tube")//SO LONG?
    {
      float r_outer = settings.get_double("r_outer", 0.4);
      float d = settings.get_double("d", 0.001);
      unsigned c_segments = settings.get_int("c_segments", 5);
      unsigned h_segments = settings.get_int("h_segments", 7);
      std::vector<float> points_x, points_y, points_z;
      settings.get_arr("points_x", points_x);
      settings.get_arr("points_y", points_y);
      settings.get_arr("points_z", points_z);
      assert(points_x.size() == points_y.size() && points_x.size() == points_z.size());
      std::vector<float3> line;
      for (int i=0;i<points_x.size();i++)
        line.push_back(float3(points_x[i], points_y[i], points_z[i]));
      mesh = create_bent_tube(r_outer, d, line.data(), line.size(), c_segments, h_segments, vt, nt);
    }
    else if (model == "thin_list")
    {
      float thickness = settings.get_double("thickness", 0.01);
      mesh = create_thin_list(thickness, vt, nt);
    }
    else if (model == "n_lists")
    {
      int n = settings.get_int("n", 5);
      float d1 = settings.get_double("d1", 0.001);
      float d2 = settings.get_double("d2", 0.005);
      mesh = create_n_lists(n, d1, d2, vt, nt);
    }
    else if (model == "variably_thin_list")
    {
      float d1 = settings.get_double("d1", 0.001);
      float d2 = settings.get_double("d2", 0.005);
      mesh = create_variably_thin_list(d1, d2, vt, nt);
    }
    else if (model == "sin_list")
    {
      float thickness = settings.get_double("thickness", 0.001);
      float wavelength = settings.get_double("wavelength", 10);
      float amplitude = settings.get_double("amplitude", 1);
      bool two_dim = settings.get_bool("two_dim", true);
      int tiles = settings.get_int("tiles", 100);
      mesh = create_sin_list(thickness, tiles, wavelength, amplitude, two_dim, vt, nt);
    }
    else if (model == "random_terrain")
    {
      int seed = settings.get_int("seed", 0);
      int heightmap_resolution = settings.get_int("heightmap_resolution", 16);
      int tiles = settings.get_int("tiles", 128);
      float amplitude = settings.get_double("amplitude", 0.01);
      bool two_sided = settings.get_bool("two_sided", false);
      float thickness = settings.get_double("thickness", 0.0001);
      mesh = create_random_terrain(seed, heightmap_resolution, tiles, amplitude, two_sided, thickness, vt, nt);
    }
    else if (model == "sphere")
    {
      int segments = settings.get_int("segments", 16);
      mesh = create_sphere(segments, vt, nt);
    }
    else
    {
      printf("unknown model name = %s\n", model.c_str());
    }

    transform_mesh(mesh, LiteMath::scale4x4(scale));
    transform_mesh(mesh, LiteMath::rotate4x4X(rotate_x)*LiteMath::rotate4x4Y(rotate_y)*LiteMath::rotate4x4Z(rotate_z));
    transform_mesh(mesh, transform);
    return mesh;
  }

  SimpleMesh create_procedural_mesh(const Block &settings)
  {
    std::string generator_name = settings.get_string("generator", "mesh");
    if (generator_name == "composite")
    {
      std::vector<SimpleMesh> meshes;
      for (int i=0; i<settings.size(); i++)
      {
        Block *b = settings.get_block(i);
        if (b)
          meshes.push_back(create_procedural_mesh(*b));
      }
      
      return MergeMeshes(meshes);
    }

    if (generator_name == "mesh")
    {
      return create_procedural_mesh_single(settings);
    }

    printf("[create_procedural_mesh]Unknown generator %s for mesh. Should be \"mesh\" or \"composite\"\n",
           generator_name.c_str());
    return SimpleMesh();
  }

  SimpleMesh create_procedural_mesh(const char *blk_file)
  {
    Block blk;
    load_block_from_file(blk_file, blk);
    return create_procedural_mesh(blk);
  }
}