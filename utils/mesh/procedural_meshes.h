#pragma once
#include "blk/blk.h"
#include "mesh.h"

namespace cmesh4
{
  //these settings do not affect the data structure (it is still SimpleMesh), 
  //they just determine how the mesh is created
  
  enum class VerticesType
  {
    SHARED, //one vertex can belong to multiple triangles (typical for indexed meshes)
    UNIQUE //one vertex can only belong to one triangle ("triangle soup")
  };
  enum class NormalsType
  {
    VERTEX, //default, normals in each vertex is an average of geomtry normals
    GEOMETRY //only for VerticesType::UNIQUE, all vertices in trianle will have the same normal
  };

  //functions to create some meshes proved to be useful
  //all meshes are created with center in (0,0,0)
  //scale means different things, but reducing in all directions will always result in a smaller mesh
  SimpleMesh create_cube(LiteMath::float3 scale, bool six_materials, VerticesType a_verticesType = VerticesType::SHARED, NormalsType a_normalsType = NormalsType::VERTEX);
  SimpleMesh create_cylinder(LiteMath::float3 scale, unsigned c_segments = 16, unsigned h_segments = 16, VerticesType a_verticesType = VerticesType::SHARED, NormalsType a_normalsType = NormalsType::VERTEX);
  SimpleMesh create_box(float3 scale, float thickness, VerticesType a_verticesType = VerticesType::SHARED, NormalsType a_normalsType = NormalsType::VERTEX);
  SimpleMesh create_tube(float r_outer, float d, unsigned c_segments, unsigned h_segments, 
                                     VerticesType a_verticesType = VerticesType::SHARED, NormalsType a_normalsType = NormalsType::VERTEX);
  SimpleMesh create_bent_tube(float r_outer, float d, float3 line[], unsigned line_size, unsigned c_segments, unsigned h_segments, 
                                     VerticesType a_verticesType = VerticesType::SHARED, NormalsType a_normalsType = NormalsType::VERTEX);
  SimpleMesh create_thin_list(float thickness, VerticesType a_verticesType = VerticesType::SHARED, NormalsType a_normalsType = NormalsType::VERTEX);
  SimpleMesh create_n_lists(unsigned n, float d1, float d2, VerticesType a_verticesType = VerticesType::SHARED, NormalsType a_normalsType = NormalsType::VERTEX);
  SimpleMesh create_variably_thin_list(float d1, float d2, VerticesType a_verticesType = VerticesType::SHARED, NormalsType a_normalsType = NormalsType::VERTEX);
  SimpleMesh create_sin_list(float thickness, int tiles, float wavelength, float amplitude, bool two_dim, VerticesType a_verticesType = VerticesType::SHARED, NormalsType a_normalsType = NormalsType::VERTEX);
  SimpleMesh create_dented_list(float d1, float d2, float l1, float l2, VerticesType a_verticesType = VerticesType::SHARED, NormalsType a_normalsType = NormalsType::VERTEX);
  SimpleMesh create_sphere(unsigned segments, VerticesType a_verticesType = VerticesType::SHARED, NormalsType a_normalsType = NormalsType::VERTEX);

  // creates a mesh from point cloud with a small (point_size) tetrahedron for each point
  SimpleMesh create_from_point_cloud(const float3 *points, int num_points, float point_size = 0.01f);

  // creates a mesh from point cloud with a small tetrahedron for each point elongated
  // along the normals of each point (point_size base, normal_length length)
  SimpleMesh create_from_point_normal_cloud(const float3 *points, const float3 *normals, int num_points, 
                                            float point_size = 0.01f, float normal_length = 0.05f);

  //unified interface to create procedural meshes from blk
  SimpleMesh create_procedural_mesh(const Block &settings);
  SimpleMesh create_procedural_mesh(const char *blk_file);
}