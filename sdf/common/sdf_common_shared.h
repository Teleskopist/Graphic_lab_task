#pragma once

#include "LiteMath.h"
#include <vector>
#include <string>
#include <memory>
#include <cassert>

using LiteMath::float2;
using LiteMath::float3;
using LiteMath::float4;
using LiteMath::uint2;
using LiteMath::uint3;
using LiteMath::uint4;
using LiteMath::int2;
using LiteMath::int3;
using LiteMath::int4;
using LiteMath::float4x4;
using LiteMath::float3x3;
using LiteMath::cross;
using LiteMath::dot;
using LiteMath::length;
using LiteMath::normalize;
using LiteMath::to_float4;
using LiteMath::to_float3;
using LiteMath::max;
using LiteMath::min;
using LiteMath::bitCount;

static constexpr unsigned INVALID_IDX = 1u<<31u;

//structs for octrees traversal
struct OTStackElement
{
  uint32_t nodeId;
  uint32_t info;
  uint2 p_size;
};

struct ExtStackElement
{
  uint32_t linksOff;
  uint32_t info;
  uint32_t transform;
  uint2 p_size;
};

//voxel position (i,j,k) to linear index
static unsigned SBS_v_to_i(float i, float j, float k, unsigned v_size, unsigned pad)
{
  return ((uint32_t)i+pad)*v_size*v_size + ((uint32_t)j+pad)*v_size + ((uint32_t)k+pad);
}
