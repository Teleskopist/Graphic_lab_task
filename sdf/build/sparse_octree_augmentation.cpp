#include "sparse_octree_builder.h"
#include "sparse_octree_common.h"
#include <map>

namespace sdf_converter
{
  void init_data_channels(const std::vector<DataChannel> &point_channels, const std::vector<DataChannel> &primitive_channels, 
                          GlobalOctree &octree)
  {
    octree.point_channels.resize(point_channels.size());
    octree.voxel_channels.resize(primitive_channels.size());

    for (int i=0; i<point_channels.size(); i++)
    {
      assert(point_channels[i].type == DataChannel::Type::FLOAT);

      octree.point_channels[i].num_components = point_channels[i].num_components;
      octree.point_channels[i].type = point_channels[i].type;
      octree.point_channels[i].name = point_channels[i].name;
      octree.point_channels[i].min_val = point_channels[i].min_val;
      octree.point_channels[i].max_val = point_channels[i].max_val;
    }

    for (int i=0; i<primitive_channels.size(); i++)
    {
      assert(primitive_channels[i].type == DataChannel::Type::FLOAT ||
             primitive_channels[i].type == DataChannel::Type::INT);

      octree.voxel_channels[i].num_components = primitive_channels[i].num_components;
      octree.voxel_channels[i].type = primitive_channels[i].type;
      octree.voxel_channels[i].name = primitive_channels[i].name;
      octree.voxel_channels[i].min_val = primitive_channels[i].min_val;
      octree.voxel_channels[i].max_val = primitive_channels[i].max_val;
    }
  }

  void copy_data_channels(const GlobalOctree &from, GlobalOctree &to)
  {
    to.point_channels.resize(from.point_channels.size());
    to.voxel_channels.resize(from.voxel_channels.size());

    to.header.tc_channel_id = from.header.tc_channel_id;
    to.header.mat_channel_id = from.header.mat_channel_id;

    for (int i=0; i<to.point_channels.size(); i++)
    {
      to.point_channels[i].num_components = from.point_channels[i].num_components;
      to.point_channels[i].type = from.point_channels[i].type;
      to.point_channels[i].name = from.point_channels[i].name;
      to.point_channels[i].min_val = from.point_channels[i].min_val;
      to.point_channels[i].max_val = from.point_channels[i].max_val;
    }

    for (int i=0; i<to.voxel_channels.size(); i++)
    {
      to.voxel_channels[i].num_components = from.voxel_channels[i].num_components;
      to.voxel_channels[i].type = from.voxel_channels[i].type;
      to.voxel_channels[i].name = from.voxel_channels[i].name;
      to.voxel_channels[i].min_val = from.voxel_channels[i].min_val;
      to.voxel_channels[i].max_val = from.voxel_channels[i].max_val;
    }
  }

  void resize_data_channels(GlobalOctree &octree, int prim_count)
  {
    for (int i=0; i<octree.point_channels.size(); i++)
      octree.point_channels[i].data_f.resize(octree.point_channels[i].data_f.size() + 8 * prim_count * octree.point_channels[i].num_components);
    
    for (int i=0; i<octree.voxel_channels.size(); i++)
    {
      if (octree.voxel_channels[i].type == DataChannel::Type::INT)
        octree.voxel_channels[i].data_i.resize(octree.voxel_channels[i].data_i.size() + prim_count * octree.voxel_channels[i].num_components);
      else
        octree.voxel_channels[i].data_f.resize(octree.voxel_channels[i].data_f.size() + prim_count * octree.voxel_channels[i].num_components);
    }
  }

  void reserve_data_channels(GlobalOctree &octree, int prim_count)
  {
    for (int i=0; i<octree.point_channels.size(); i++)
      octree.point_channels[i].data_f.reserve(octree.point_channels[i].data_f.size() + 8 * prim_count * octree.point_channels[i].num_components);
    
    for (int i=0; i<octree.voxel_channels.size(); i++)
    {
      if (octree.voxel_channels[i].type == DataChannel::Type::INT)
        octree.voxel_channels[i].data_i.reserve(octree.voxel_channels[i].data_i.size() + prim_count * octree.voxel_channels[i].num_components);
      else
        octree.voxel_channels[i].data_f.reserve(octree.voxel_channels[i].data_f.size() + prim_count * octree.voxel_channels[i].num_components);
    }
  }

  void fill_data_channels_node(const cmesh4::AugmentedMesh &a_mesh, GlobalOctree &octree, unsigned v_size, unsigned size,
                               const std::vector<sdf_converter::PointAttributes> &attributes, unsigned node_idx)
  {
    // fill voxel channels with values from face channels from center-ish point
    {
      unsigned idx = v_size * v_size * (size / 2) + v_size * (size / 2) + size / 2;
      unsigned in_id = attributes[idx].closest_primitive_id;
      unsigned out_id = node_idx;

      for (unsigned ch_id = 0; ch_id < a_mesh.face_channels.size(); ch_id++)
      {
        unsigned c_num = octree.voxel_channels[ch_id].num_components;
        for (unsigned c_id = 0; c_id < c_num; c_id++)
        {
          if (a_mesh.face_channels[ch_id].type == DataChannel::Type::FLOAT)
            octree.voxel_channels[ch_id].data_f[out_id * c_num + c_id] = a_mesh.face_channels[ch_id].data_f[in_id * c_num + c_id];
          else if (a_mesh.face_channels[ch_id].type == DataChannel::Type::INT)
            octree.voxel_channels[ch_id].data_i[out_id * c_num + c_id] = a_mesh.face_channels[ch_id].data_i[in_id * c_num + c_id];
        }
      }
    }

    for (int j = 0; j < 8; ++j)
    {
      unsigned idx = v_size * v_size * (j >> 2) + v_size * ((j >> 1) & 1) + (j & 1);
      unsigned t_i = attributes[idx].closest_primitive_id;

      unsigned idA = a_mesh.indices[3 * t_i + 0];
      unsigned idB = a_mesh.indices[3 * t_i + 1];
      unsigned idC = a_mesh.indices[3 * t_i + 2];

      float3 a = a_mesh.vertices[idA];
      float3 b = a_mesh.vertices[idB];
      float3 c = a_mesh.vertices[idC];
      float3 bc = cmesh4::barycentric(attributes[idx].closest_point, a, b, c);

      unsigned out_id = 8 * node_idx + j;
      for (unsigned ch_id = 0; ch_id < a_mesh.vertex_channels.size(); ch_id++)
      {
        unsigned c_num = octree.point_channels[ch_id].num_components;
        for (unsigned c_id = 0; c_id < c_num; c_id++)
        {
          float v1 = a_mesh.vertex_channels[ch_id].data_f[idA * c_num + c_id];
          float v2 = a_mesh.vertex_channels[ch_id].data_f[idB * c_num + c_id];
          float v3 = a_mesh.vertex_channels[ch_id].data_f[idC * c_num + c_id];
          octree.point_channels[ch_id].data_f[out_id * c_num + c_id] = bc.x * v1 + bc.y * v2 + bc.z * v3;
        }
      }
    }
  }

  void recursive_voxel_channels_merging(GlobalOctree &octree, uint32_t idx)
  {
    if ((idx & INVALID_IDX) == 0 && octree.nodes[idx].offset != 0)
    {
      unsigned children_cnt = std::pow(2, octree.header.dim);
      for (int i = 0; i < children_cnt; ++i)
      {
        recursive_voxel_channels_merging(octree, octree.nodes[idx].offset + i);
      }
      for (int i = 0; i < octree.voxel_channels.size(); ++i)
      {
        if (octree.voxel_channels[i].type == DataChannel::Type::INT)
        {
          unsigned n_comp = octree.voxel_channels[i].num_components;
          for (int j = 0; j < n_comp; ++j)
          {
            std::map<int, int> channel_data;
            for (int k = 0; k < children_cnt; ++k)
            {
              if ((octree.nodes[idx].offset & INVALID_IDX) == 0 && octree.nodes[idx].offset != 0 && octree.nodes[octree.nodes[idx].offset + k].bricks_count != 0)
              {
                octree.voxel_channels[i].data_i[(octree.nodes[idx].offset + k) * n_comp + j];
                if (channel_data.find(octree.voxel_channels[i].data_i[(octree.nodes[idx].offset + k) * n_comp + j]) != channel_data.end())
                {
                  channel_data[octree.voxel_channels[i].data_i[(octree.nodes[idx].offset + k) * n_comp + j]] += 1;
                }
                else
                {
                  channel_data[octree.voxel_channels[i].data_i[(octree.nodes[idx].offset + k) * n_comp + j]] = 1;
                }
              }
            }
            int cnt = 0;
            for (auto channel : channel_data)
            {
              if (cnt < channel.second)
              {
                cnt = channel.second;
                octree.voxel_channels[i].data_i[idx * n_comp + j] = channel.first;
              }
            }
          }
        }
      }
    }
  }

  void eliminate_empty_nodes_from_data_channels(const GlobalOctree &in_octree, GlobalOctree &out_octree,
                                                const std::vector<unsigned> &idx_remap)
  {
    unsigned old_size = in_octree.nodes.size();
    unsigned new_size = out_octree.nodes.size();

    assert(idx_remap.size() == old_size);

    out_octree.header.tc_channel_id = in_octree.header.tc_channel_id;
    out_octree.header.mat_channel_id = in_octree.header.mat_channel_id;
    out_octree.point_channels.resize(in_octree.point_channels.size());
    for (unsigned i = 0; i < in_octree.point_channels.size(); ++i)
    {
      out_octree.point_channels[i].name = in_octree.point_channels[i].name;
      out_octree.point_channels[i].type = in_octree.point_channels[i].type;
      out_octree.point_channels[i].num_components = in_octree.point_channels[i].num_components;
      out_octree.point_channels[i].min_val = in_octree.point_channels[i].min_val;
      out_octree.point_channels[i].max_val = in_octree.point_channels[i].max_val;
      if (in_octree.point_channels[i].type == DataChannel::Type::FLOAT)
        out_octree.point_channels[i].data_f.resize(8 * new_size * in_octree.point_channels[i].num_components);
      else
        assert(false);
    }

    out_octree.voxel_channels.resize(in_octree.voxel_channels.size());
    for (unsigned i = 0; i < in_octree.voxel_channels.size(); ++i)
    {
      out_octree.voxel_channels[i].name = in_octree.voxel_channels[i].name;
      out_octree.voxel_channels[i].type = in_octree.voxel_channels[i].type;
      out_octree.voxel_channels[i].num_components = in_octree.voxel_channels[i].num_components;
      out_octree.voxel_channels[i].min_val = in_octree.voxel_channels[i].min_val;
      out_octree.voxel_channels[i].max_val = in_octree.voxel_channels[i].max_val;
      if (in_octree.voxel_channels[i].type == DataChannel::Type::FLOAT)
        out_octree.voxel_channels[i].data_f.resize(new_size * in_octree.voxel_channels[i].num_components);
      else if (in_octree.voxel_channels[i].type == DataChannel::Type::INT)
        out_octree.voxel_channels[i].data_i.resize(new_size * in_octree.voxel_channels[i].num_components);
      else
        assert(false);
    }

    for (unsigned i = 0; i < old_size; ++i)
    {
      if (idx_remap[i] == 0xFFFFFFFFu)
        continue;
      for (unsigned ch = 0; ch < out_octree.voxel_channels.size(); ++ch)
      {
        unsigned n_comp = out_octree.voxel_channels[ch].num_components;
        unsigned new_idx = idx_remap[i];

        if (out_octree.voxel_channels[ch].type == DataChannel::Type::FLOAT)
        {
          for (unsigned c = 0; c < n_comp; c++)
            out_octree.voxel_channels[ch].data_f[new_idx * n_comp + c] = in_octree.voxel_channels[ch].data_f[i * n_comp + c];
        }
        else if (out_octree.voxel_channels[ch].type == DataChannel::Type::INT)
        {
          for (unsigned c = 0; c < n_comp; c++)
            out_octree.voxel_channels[ch].data_i[new_idx * n_comp + c] = in_octree.voxel_channels[ch].data_i[i * n_comp + c];
        }
      }

      for (unsigned ch = 0; ch < out_octree.point_channels.size(); ++ch)
      {
        unsigned n_comp = 8 * out_octree.point_channels[ch].num_components;
        unsigned new_idx = idx_remap[i];
        for (unsigned c = 0; c < n_comp; c++)
          out_octree.point_channels[ch].data_f[new_idx * n_comp + c] = in_octree.point_channels[ch].data_f[i * n_comp + c];
      }
    }
  }

  float parent_child_distance_channels(const GlobalOctree &octree, uint32_t node_id, 
                                       uint32_t ch_node_id, uint32_t child_n)
  {
    //if there are point integer values or other strange cases, 
    //we assume that the nodes are incompatible
    for (auto &ch : octree.point_channels)
    {
      if (ch.type != DataChannel::Type::FLOAT)
        return INCOMPARABLE_NODES_DISTANCE;
    }

    for (auto &ch : octree.voxel_channels)
    {
      if (ch.type != DataChannel::Type::FLOAT && ch.type != DataChannel::Type::INT)
        return INCOMPARABLE_NODES_DISTANCE;
    }

    //first, test if all int values match, assume only voxel values can be integer
    for (auto &ch : octree.voxel_channels)
    {
      if (ch.type != DataChannel::Type::INT)
        continue;
      for (uint32_t c_id=0; c_id<ch.num_components; c_id++)
      {
        int v1 = ch.data_i[ch.num_components*node_id + c_id];
        int v2 = ch.data_i[ch.num_components*ch_node_id + c_id];
        if (v1 != v2)
          return INCOMPARABLE_NODES_DISTANCE;
      }
    }

    //find (relative) error in float channels
    float total_error = 0.0f;

    //first, voxel channels
    for (auto &ch : octree.voxel_channels)
    {
      if (ch.type != DataChannel::Type::FLOAT)
        continue;
      for (uint32_t c_id=0; c_id<ch.num_components; c_id++)
      {
        float v1 = ch.data_f[ch.num_components*node_id + c_id];
        float v2 = ch.data_f[ch.num_components*ch_node_id + c_id];
        total_error += std::abs(v1 - v2);
      }
    }

    //then, point channels
    for (auto &ch : octree.point_channels)
    {
      if (ch.type != DataChannel::Type::FLOAT)
        continue;
      for (int i=0;i<8;i++)
      {
        uint3 q1 = uint3(i >> 2, (i >> 1) & 1, i & 1);
        uint3 q2 = uint3(child_n >> 2, (child_n >> 1) & 1, child_n & 1);
        float3 q = float3(q1+q2)*0.5f;
        for (uint32_t c_id=0; c_id<ch.num_components; c_id++)
        {
          const float *data = ch.data_f.data() + 8*ch.num_components*node_id + c_id;
          float v1 = (1-q.x)*(1-q.y)*(1-q.z)*data[0*ch.num_components] + 
                     (1-q.x)*(1-q.y)*(  q.z)*data[1*ch.num_components] +
                     (1-q.x)*(  q.y)*(1-q.z)*data[2*ch.num_components] +
                     (1-q.x)*(  q.y)*(  q.z)*data[3*ch.num_components] +
                     (  q.x)*(1-q.y)*(1-q.z)*data[4*ch.num_components] +
                     (  q.x)*(1-q.y)*(  q.z)*data[5*ch.num_components] + 
                     (  q.x)*(  q.y)*(1-q.z)*data[6*ch.num_components] +
                     (  q.x)*(  q.y)*(  q.z)*data[7*ch.num_components];
          float v2 = ch.data_f[ch.num_components*(8*ch_node_id + i) + c_id];
          total_error += 0.125f*std::abs((v1 - v2)/(ch.max_val-ch.min_val+1e-9f));
        }
      }
    }

    return total_error;
  }
}