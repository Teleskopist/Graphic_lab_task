#include "augmented_mesh.h"
#include <fstream>

namespace cmesh4
{
  void recalculate_vertex_normals(AugmentedMesh &mesh)
  {
    mesh.normals.resize(mesh.vertices.size());
    std::vector<float4> vec_norm_count(mesh.vertices.size());

    for (int i=0;i<mesh.indices.size();i+=3)
    {
      unsigned idx0 = mesh.indices[i+0];
      unsigned idx1 = mesh.indices[i+1];
      unsigned idx2 = mesh.indices[i+2];

      float3 &p0 = mesh.vertices[idx0]; 
      float3 &p1 = mesh.vertices[idx1];
      float3 &p2 = mesh.vertices[idx2];

      float4 triangle_normal = to_float4(normalize(cross(p1 - p0, p2 - p0)), 1.0f);

      vec_norm_count[idx0] += triangle_normal;
      vec_norm_count[idx1] += triangle_normal;
      vec_norm_count[idx2] += triangle_normal;
    }

    for (int i=0;i<mesh.normals.size();i++)
      mesh.normals[i] = to_float3(vec_norm_count[i]) / vec_norm_count[i].w;
  }

  void get_bbox(const AugmentedMesh &mesh, float3 *min_pos, float3 *max_pos)
  {
    *min_pos = float3(1e9,1e9,1e9);
    *max_pos = float3(-1e9,-1e9,-1e9);

    for (const float3 &p : mesh.vertices)
    {
      *min_pos = min(*min_pos, p);
      *max_pos = max(*max_pos, p);
    }
  }

  LiteMath::float4x4 rescale_mesh(AugmentedMesh &mesh, float3 min_pos, float3 max_pos)
  {
    assert(mesh.vertices.size() >= 3);

    float3 mesh_min, mesh_max;
    get_bbox(mesh, &mesh_min, &mesh_max);

    float3 mesh_size = mesh_max - mesh_min;
    float3 target_size = max_pos - min_pos;
    float3 scale3 = target_size/mesh_size;
    float scale = std::min(scale3.x, std::min(scale3.y, scale3.z));

    //changing poditions, .w coord is preserved
    for (float3 &p : mesh.vertices)
      p = min_pos + scale*(p - mesh_min);

    //it is only move and rescale, so now changes to normals are required

    LiteMath::float4x4 trans = translate4x4(min_pos)*scale4x4(float3(scale))*translate4x4(-mesh_min);
    return trans;
  }

  void save_augmented_mesh(const AugmentedMesh &mesh, const char *filename)
  {
    std::ofstream fs(filename, std::ios::binary);

    unsigned v_count = mesh.vertices.size();
    unsigned i_count = mesh.indices.size();
    unsigned n_count = mesh.normals.size();
    unsigned vc_count = mesh.vertex_channels.size();
    unsigned fc_count = mesh.face_channels.size();

    fs.write((const char *)&v_count, sizeof(v_count));
    fs.write((const char *)&i_count, sizeof(i_count));
    fs.write((const char *)&n_count, sizeof(n_count));
    fs.write((const char *)&vc_count, sizeof(vc_count));
    fs.write((const char *)&fc_count, sizeof(fc_count));

    fs.write((const char *)mesh.vertices.data(), v_count * sizeof(float3));
    fs.write((const char *)mesh.indices.data(), i_count * sizeof(unsigned));
    fs.write((const char *)mesh.normals.data(), n_count * sizeof(float3));

    for (unsigned i=0;i<vc_count;i++)
      save_data_channel(fs, mesh.vertex_channels[i]);

    for (unsigned i=0;i<fc_count;i++)
      save_data_channel(fs, mesh.face_channels[i]);

    fs.flush();
    fs.close();
  }

  void load_augmented_mesh(AugmentedMesh &mesh, const char *filename)
  {
    std::ifstream fs(filename, std::ios::binary);

    unsigned v_count = 0;
    unsigned i_count = 0;
    unsigned n_count = 0;
    unsigned vc_count = 0;
    unsigned fc_count = 0;

    fs.read((char *)&v_count, sizeof(v_count));
    fs.read((char *)&i_count, sizeof(i_count));
    fs.read((char *)&n_count, sizeof(n_count));
    fs.read((char *)&vc_count, sizeof(vc_count));
    fs.read((char *)&fc_count, sizeof(fc_count));

    mesh.vertices.resize(v_count);
    mesh.indices.resize(i_count);
    mesh.normals.resize(n_count);
    mesh.vertex_channels.resize(vc_count);
    mesh.face_channels.resize(fc_count);

    fs.read((char *)mesh.vertices.data(), v_count * sizeof(float3));
    fs.read((char *)mesh.indices.data(), i_count * sizeof(unsigned));
    fs.read((char *)mesh.normals.data(), n_count * sizeof(float3));

    for (unsigned i=0;i<vc_count;i++)
      load_data_channel(fs, mesh.vertex_channels[i]);

    for (unsigned i=0;i<fc_count;i++)
      load_data_channel(fs, mesh.face_channels[i]);

    fs.close();
  }

  uint64_t calculate_bytesize(const AugmentedMesh &mesh)
  {
    uint64_t size_vector_sizes = 5 + 4*(mesh.face_channels.size() + mesh.vertex_channels.size());
    uint64_t size_vertices = mesh.vertices.size() * sizeof(float3);
    uint64_t size_indices = mesh.indices.size() * sizeof(unsigned);
    uint64_t size_normals = mesh.normals.size() * sizeof(float3);
    uint64_t size_channels = 0;

    for (const DataChannel &channel : mesh.face_channels)
      size_channels += calculate_bytesize(channel);

    for (const DataChannel &channel : mesh.vertex_channels)
      size_channels += calculate_bytesize(channel);
    
    return size_vector_sizes + size_vertices + size_indices + size_normals + size_channels;
  }

  ModelInfo get_info_augmented_mesh(const AugmentedMesh &mesh)
  {
    ModelInfo info;
    info.bytesize = calculate_bytesize(mesh);
    info.num_primitives = mesh.indices.size()/3;
    info.name = "augmented_mesh";
    return info;
  }
}