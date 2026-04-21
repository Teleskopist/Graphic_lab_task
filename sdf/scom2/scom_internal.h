#pragma once
#include "scom_builder.h"
#include "scom_utils.h"
#include "clustering/clustering.h"
#include "clustering/symmetric_groups.h"
#include "sdf/build/sparse_octree_common.h"
#include "utils/common/int_pow.h"

using sdf_converter::MergeInData;
using sdf_converter::MergeTmpData;

namespace scom2
{
  // Each dataset of bricks or branches contais different nodes, some are incompatible
  // with each other (i.e. different number of surfaces or from different levels).
  // We split the dataset into these sub-datasets, each of them can be clustered separately, but
  // no match between sub-datasets. Difference in any field of sub-dataset descriptor prevents
  // two sub-datasets from being clustered together.
  struct SubDatasetDescriptor
  {
    uint32_t dist_count_per_surface = 0; // number of distance values per surface, i.e. brick_size^3, affects data point size
    uint32_t surface_count = 0;          // number of surfaces (bricks), affects data point size
    uint32_t channels_count = 0;         // number of channel values (only float/FP8 here), affects data point size 
    uint32_t level = 0;                  // level of the node in the octree, different levels must be clustered with
                                         // different thresholds
    uint32_t brick_type = 0;             // type of the brick (BrickInfo::Type) - affects how the data point is encoded
    uint32_t int_channels_idx = 0;       // unique id for set of integer channels values (e.g. material Id). It is reasonable 
                                         // to think that different values of integer channels mean different things and 
                                         // clustering them makes little sense. Using hash here to avoid variable descriptor size
    uint32_t bin_hash = 0;               // to increase clustering speed, binning may be used, in this case points from the
                                         // separate bins should be put into different dataset
  };

  static uint32_t get_desc_size(const SubDatasetDescriptor &desc)
  {
    return desc.surface_count * desc.dist_count_per_surface + desc.channels_count;
  }

  struct SubDatasetHasher
  {
    std::size_t operator()(const SubDatasetDescriptor &desc) const
    {
      constexpr uint32_t D_SIZE = sizeof(SubDatasetDescriptor) / sizeof(uint32_t);
      uint32_t code[D_SIZE] = {desc.dist_count_per_surface, desc.surface_count, desc.channels_count, 
                               desc.level, desc.brick_type, desc.int_channels_idx, desc.bin_hash};
      std::size_t hash = D_SIZE;
      for(uint32_t i = 0; i < D_SIZE; i++) 
      {
        uint32_t x = code[i];
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        hash ^= x + 0x9e3779b9 + (hash << 6) + (hash >> 2);
      }
      return hash; 
    }
  };

  struct SubDatasetEqual
  {
    bool operator()(const SubDatasetDescriptor &lhs, const SubDatasetDescriptor &rhs) const
    {
      return lhs.dist_count_per_surface == rhs.dist_count_per_surface && 
             lhs.channels_count == rhs.channels_count &&
             lhs.surface_count == rhs.surface_count && 
             lhs.level == rhs.level && 
             lhs.brick_type == rhs.brick_type;
    }
  };
  template <typename T>
  using SubDatasetMap = std::unordered_map<SubDatasetDescriptor, T, SubDatasetHasher, SubDatasetEqual>;

  static BrickInfo::Type get_surface_type(const SdfDAG *dag, uint32_t node_id, uint32_t data_edge_id,
                                          InternalBrickUse internal_brick_use)
  {
    uint32_t v_size = get_v_size(dag->header);
    uint32_t v_count = get_v_count(dag->header);
    uint32_t val_off = dag->data_edges[data_edge_id].data_offset;

    float min_val = 1;
    float max_val = -1;
    float sum = 0.0f;
    for (uint32_t j = 0; j < v_count; ++j)
    {
      float v = dag->distances[val_off + j];
      sum += v;
      if (v < min_val)
        min_val = v;
      if (v > max_val)
        max_val = v;
    }

    BrickInfo::Type type;
    if (min_val <= 0.0f && max_val >= 0.0f)
    {
      if (DAG_extract_is_surface(dag->nodes[node_id].voxel_count_flags))
        type = BrickInfo::Type::SURFACE;
      else
        type = BrickInfo::Type::VOLUME;
    }
    else
    {
      if (max_val < 0.0f || DAG_extract_is_inside(dag->nodes[node_id].voxel_count_flags))
        type = BrickInfo::Type::INTERNAL;
      else
        type = BrickInfo::Type::EMPTY;
    }

    if (type == BrickInfo::Type::INTERNAL)
    {
      if (internal_brick_use == InternalBrickUse::DROP)
        type = BrickInfo::Type::EMPTY;
      else if (internal_brick_use == InternalBrickUse::MERGE_ALL)
        type = BrickInfo::Type::INTERNAL;
      else if (internal_brick_use == InternalBrickUse::PROPER_CLUSTERING)
        type = BrickInfo::Type::VOLUME;
    }

    return type;
  }

  static BrickInfo::Type merge_types(BrickInfo::Type a, BrickInfo::Type b)
  {
    return (BrickInfo::Type)(std::max<uint8_t>(uint8_t(a), uint8_t(b)));
  }

  template <bool multiple_components>
  static void upsample_brick(const float* in, float* out, uint32_t v_size_in, 
                             uint32_t mult, uint32_t num_components = 1)
  {
    uint32_t num_comp = 1;
    if constexpr(multiple_components)
      num_comp = num_components;
    uint32_t ivsz = v_size_in;
    uint32_t ovsz = (v_size_in-1)*mult + 1;
    float is = 1.0f/mult;
    for (uint32_t x1=0; x1<v_size_in-1; x1++)
    {
      for (uint32_t y1=0; y1<v_size_in-1; y1++)
      {
        for (uint32_t z1=0; z1<v_size_in-1; z1++)
        {
          for (uint32_t x2=(x1>0?1:0); x2<=mult; x2++)
          {
            for (uint32_t y2=(y1>0?1:0); y2<=mult; y2++)
            {
              for (uint32_t z2=(z1>0?1:0); z2<=mult; z2++)
              {
                uint32_t out_idx = num_comp*(ovsz*ovsz*(x1*mult+x2) + ovsz*(y1*mult+y2) + (z1*mult+z2));
                uint32_t  in_idx = num_comp*(ivsz*ivsz*x1 + ivsz*y1 + z1);
                float3 q = float3(is*x2,is*y2,is*z2);
                float qs[8] = {(1-q.x)*(1-q.y)*(1-q.z),(1-q.x)*(1-q.y)*q.z,
                               (1-q.x)*q.y*(1-q.z),(1-q.x)*q.y*q.z,
                                  q.x*(1-q.y)*(1-q.z),    q.x*(1-q.y)*q.z,
                                  q.x*q.y*(1-q.z),        q.x*q.y*q.z};
                uint32_t idx[8] = {in_idx, in_idx + num_comp, in_idx + num_comp*ivsz,
                                   in_idx + num_comp*(ivsz+1), in_idx + num_comp*(ivsz*ivsz),
                                   in_idx + num_comp*(ivsz*ivsz + 1), in_idx + num_comp*(ivsz*ivsz + ivsz),
                                   in_idx + num_comp*(ivsz*ivsz + ivsz + 1)};
                for (uint32_t c=0;c<num_comp;c++)
                {
                  out[out_idx+c] = 0.0f;
                  for (uint32_t i=0;i<8;i++)
                    out[out_idx+c] += qs[i]*in[idx[i]+c];
                }
              }             
            }            
          }
        }      
      }      
    }
  }

  template <typename T>
  static void upsample_voxel_grid(const T* in, T* out, uint32_t size_in, uint32_t mult, uint32_t num_comp)
  {
    uint32_t si = size_in;
    uint32_t so = mult*size_in;
    for (uint32_t i=0;i<so;i++)
    {
      for (uint32_t j=0;j<so;j++)
      {
        for (uint32_t k=0;k<so;k++)
        {
          uint3 p = uint3(i/mult,j/mult,k/mult);
          for (uint32_t c=0; c<num_comp; c++)
            out[num_comp*(i*so*so + j*so + k) + c] = in[num_comp*(p.x*si*si + p.y*si + p.z) + c];
        }
      }
    }
  }

  // calculate sensitivity for given level. 1.0f for base level, 
  // higher for higher levels, as any changes in higher levels are more visible 
  float get_level_sensitivity(uint32_t level, uint32_t base_level, float sensitivity_pow);

  // for similarity compression, both bricks and branches, we compress blocks from
  // different levels separately, because we should use different thresholds
  // (some change in large nodes are more visible than in small ones)
  // so we need to get the base level for which the similarity threshold value
  // is applied. Here we use the level with the most active nodes
  uint32_t get_base_level(const SdfDAG &dag);
}