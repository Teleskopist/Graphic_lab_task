#pragma once

#include "LiteMath.h"
#include <vector>
#include <sstream>
#include <cassert>

namespace cmesh4
{
  using LiteMath::float4;
  using LiteMath::float3;
  using LiteMath::float2;
  using LiteMath::uint3;

  struct Header
  {
    enum GEOM_FLAGS {
      HAS_TANGENT    = 1,
      UNUSED2        = 2,
      UNUSED4        = 4,
      HAS_NO_NORMALS = 8
    };

    uint64_t fileSizeInBytes;
    uint32_t verticesNum;
    uint32_t indicesNum;
    uint32_t materialsNum;
    uint32_t flags;
  };

  Header LoadHeader(std::istream &str);

  // very simple utility mesh representation for working with geometry on the CPU in C++
  //
  struct SimpleMesh
  {
    static const uint64_t POINTS_IN_TRIANGLE = 3;
    SimpleMesh(){}
    SimpleMesh(size_t a_vertNum, size_t a_indNum) { Resize(a_vertNum, a_indNum); }
    SimpleMesh(const SimpleMesh &other) = default;
    SimpleMesh(SimpleMesh &&other) = default;
    SimpleMesh &operator=(const SimpleMesh &other) = default;
    SimpleMesh &operator=(SimpleMesh &&other) = default;

    inline size_t VerticesNum()  const { return vPos4f.size(); }
    inline size_t IndicesNum()   const { return indices.size();  }
    inline size_t TrianglesNum() const { return IndicesNum() / POINTS_IN_TRIANGLE;  }
    inline void   Resize(size_t a_vertNum, size_t a_indNum)
    {
      vPos4f.resize(a_vertNum);
      vNorm4f.resize(a_vertNum);
      vTang4f.resize(a_vertNum);
      vTexCoord2f.resize(a_vertNum);
      indices.resize(a_indNum);
      matIndices.resize(a_indNum/3); 
      assert(a_indNum%3 == 0); // PLEASE NOTE THAT CURRENT IMPLEMENTATION ASSUMES ONLY TRIANGLE MESHES!
    };

    inline size_t SizeInBytes() const
    {
      return vPos4f.size()*sizeof(float)*4  +
             vNorm4f.size()*sizeof(float)*4 +
             vTang4f.size()*sizeof(float)*4 +
             vTexCoord2f.size()*sizeof(float)*2 +
             indices.size()*sizeof(int) +
             matIndices.size()*sizeof(int);
    }

    float GetAvgTriArea() const;
    float GetAvgTriPerimeter() const;

    void ApplyMatrix(const LiteMath::float4x4& m);

    std::vector<LiteMath::float4> vPos4f;      // #TODO: put aligned allocator here
    std::vector<LiteMath::float4> vNorm4f;     // #TODO: put aligned allocator here
    std::vector<LiteMath::float4> vTang4f;     // #TODO: put aligned allocator here
    std::vector<float2>           vTexCoord2f; // 
    std::vector<unsigned int>     indices;     // size = 3*TrianglesNum() for triangle mesh, 4*TrianglesNum() for quad mesh
    std::vector<unsigned int>     matIndices;  // size = 1*TrianglesNum()
  };

  SimpleMesh LoadMeshFromVSGF(const char* a_fileName);
  void       SaveMeshToVSGF  (const char* a_fileName, const SimpleMesh& a_mesh);
  SimpleMesh CreateQuad(const int a_sizeX, const int a_sizeY, const float a_size);

  struct PreprocessSettings
  {
    bool rescale = true; // should we rescale mesh to [min_p, max_p]^3
    float3 min_p = float3(-0.999f);
    float3 max_p = float3( 0.999f);

    bool recalculate_normals = true;
    bool verbose = false;

    bool compress_close_vertices = true;
    bool force_merge_distinct_normals = true;
    float close_vertices_thr = 1e-7f;

    bool align_to_axes = false;
    float align_to_axes_tolerance = 0.001f;
  };

  SimpleMesh preprocess_mesh(const SimpleMesh &mesh, PreprocessSettings settings, LiteMath::float4x4 *out_transform = nullptr);
  std::vector<SimpleMesh> preprocess_mesh_parts(const std::vector<SimpleMesh> &parts, PreprocessSettings settings, 
                                                bool recreate_parts, LiteMath::float4x4 *out_transform = nullptr);

  bool intersect_segment_triangle(const float3 &p, const float3 &q, 
                                  const float3 &a, const float3 &b, const float3 &c, float &offset);

  float3 closest_point_triangle(const float3 &p, const float3 &a, const float3 &b, const float3 &c);
  float3 barycentric(const float3 &p, const float3 &a, const float3 &b, const float3 &c);
  void get_bbox(const cmesh4::SimpleMesh &mesh, float3 *min_pos, float3 *max_pos);
  bool triangle_aabb_intersect(const float3 &a, const float3 &b, const float3 &c, 
                               const float3 &aabb_center, const float3 &aabb_half_size);

  //rescales mesh with constant scale to fit it inside the given box and returns transform that does it
  LiteMath::float4x4 rescale_mesh(cmesh4::SimpleMesh &mesh, float3 min_pos, float3 max_pos);
  void transform_mesh(cmesh4::SimpleMesh &mesh, LiteMath::float4x4 transform);
  void recalculate_vertex_normals(cmesh4::SimpleMesh &mesh);
  void set_mat_id(cmesh4::SimpleMesh &mesh, int mat_id);

  /**
  fast check if the mesh is baseline valid, if returns false, it is actually broken and should not be used
  if verbose = true, it will print what exactly is wrong
  */
  bool check_is_valid(const cmesh4::SimpleMesh &mesh, bool verbose = false);
  
  //checks for mesh defects and issues, notifies about them and tries to fix (e.g. wrong normal direction) 
  //it also rescales mesh so that the AABB is fit inside [-1,1]^3, and returns transform that does it
  LiteMath::float4x4 normalize_mesh(cmesh4::SimpleMesh &mesh, bool verbose = false);

  cmesh4::SimpleMesh LoadMesh(const char* a_fileName, bool apply_basic_transforms = true, bool verbose = false,
                              LiteMath::float4x4 *out_transform = nullptr, bool rescale = true);

  cmesh4::SimpleMesh MergeMeshes(const std::vector<cmesh4::SimpleMesh> &meshes);
  std::vector<cmesh4::SimpleMesh> separate_components(const cmesh4::SimpleMesh& mesh);

  void align_mesh_to_axes(SimpleMesh& mesh, float tolerance = 0.001f);

  void SaveMeshToObj(const char* a_fileName, const cmesh4::SimpleMesh &mesh);
}