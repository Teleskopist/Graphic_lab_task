#pragma once
#include "LiteMath/LiteMath.h"
#include "utils/common/matrix.h"
#include <vector>
#include <cstdint>

namespace scom2
{
  using LiteMath::uint2;
  using LiteMath::uint3;
  using LiteMath::uint4;
  using LiteMath::int2;
  using LiteMath::int3;
  using LiteMath::int4;
  using LiteMath::float2;
  using LiteMath::float3;
  using LiteMath::float4;
  using LiteMath::float4x4;

  static constexpr uint32_t INVALID_IDX = 1u<<31u;
  static constexpr uint32_t ROT_COUNT = 48;

  /* With dim = 3, universal rot transforms generator creates the same transformations as the legacy one,
     but in a different order. It is crusial to preserve the legacy order, as rotation id is preserved in
     DAG/SCom2 models saved by old code. This is a remap to preserve this order.
     TODO: after some breaking changes invalidating old models, this can be removed
  */
  static const uint32_t legacy_to_universal_id_remap[ROT_COUNT] = {0, 24, 32, 1, 25, 33, 8, 16, 40, 9, 17, 41, 
                                                                   2, 26, 34, 3, 27, 35, 10, 18, 42, 11, 19, 43, 
                                                                   4, 28, 36, 5, 29, 37, 12, 20, 44, 13, 21, 45, 
                                                                   6, 30, 38, 7, 31, 39, 14, 22, 46, 15, 23, 47};

  //generalized rotations
  void initialize_rot_transforms(int dim, int v_size, std::vector<LiteMath::Matrix<float>> &rot_transforms);

  // initialization function for rotation code arrays/table for rendering SCom Tree (3-dimentional only)
  void initialize_rot_transforms(std::vector<float4x4> &rot_transforms, int v_size);
  void initialize_rot_modifiers(std::vector<int4> &rot_modifiers, int v_size);
}