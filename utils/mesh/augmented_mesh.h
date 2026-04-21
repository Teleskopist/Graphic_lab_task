#pragma once
#include "utils/mesh/mesh.h"
#include "utils/common/data_channel.h"
#include "utils/misc/scene_common.h"

namespace cmesh4
{
  using LiteMath::float4;
  using LiteMath::float3;
  using LiteMath::uint3;

  /* Data structure to store mesh with arbitrary attributes
     There are two kinds of attributes:
     - vertex attributes
     - face attributes
    Each attribute is a k-component vector, either float or int
    doubles and long ints are not supported, but can be trivially 
    added if necessary.
    Vertex attributes are interpolated, so they are usually float.
    Face attributes are not interpolated.
    Each array should either be empty or have the length k*N, where
    N is the number of vertices or faces.
  */
  struct AugmentedMesh
  {
    std::vector<float3>  vertices;
    std::vector<float3>  normals;
    std::vector<DataChannel> vertex_channels;

    std::vector<uint32_t> indices;
    std::vector<DataChannel>  face_channels;
  };

  uint64_t calculate_bytesize(const AugmentedMesh &mesh);
  ModelInfo get_info_augmented_mesh(const AugmentedMesh &mesh);
  void recalculate_vertex_normals(AugmentedMesh &mesh);
  LiteMath::float4x4 rescale_mesh(AugmentedMesh &mesh, float3 min_pos, float3 max_pos);
  void save_augmented_mesh(const AugmentedMesh &mesh, const char *filename);
  void load_augmented_mesh(AugmentedMesh &mesh, const char *filename);
}