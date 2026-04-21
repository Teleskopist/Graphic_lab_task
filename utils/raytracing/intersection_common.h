#pragma once

#include "LiteMath.h"
#include "raytrace_common.h"
#include "cbvh.h"

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <memory>

using LiteMath::cross;
using LiteMath::dot;
using LiteMath::float2;
using LiteMath::float3;
using LiteMath::float4;
using LiteMath::float4x4;
using LiteMath::int2;
using LiteMath::inverse4x4;
using LiteMath::normalize;
using LiteMath::sign;
using LiteMath::to_float3;
using LiteMath::uint;
using LiteMath::uint2;
using LiteMath::uint3;
using LiteMath::uint4;
using LiteMath::Box4f;
using LiteMath::as_float32;
using LiteMath::as_uint32;

// common data for each geometry on scene
struct GeomData
{
  float4 boxMin;
  float4 boxMax;

  uint32_t bvhOffset;
  uint32_t type; // enum GeomType
  uint2 offset;

  uint32_t hasChannels;
  uint32_t _pad[3];
};

// common data for each instance on scene
struct InstanceData
{
  float4 boxMin;
  float4 boxMax;
  uint32_t geomId;
  uint32_t _pad[7];
  float4x4 transform;
  float4x4 transformInv;
  float4x4 transformInvTransposed; //for normals
};

static int first_node(float3 t0, float3 tm)
{
uint32_t answer = 0;   // initialize to 00000000
// select the entry plane and set bits
if(t0.x > t0.y){
    if(t0.x > t0.z){ // PLANE YZ
        if(t0.x > tm.y) answer|=2;    // set bit at position 1
        if(t0.x > tm.z) answer|=1;    // set bit at position 0
        return (int) answer;
    }
}
else {
    if(t0.y > t0.z){ // PLANE XZ
        if(tm.x < t0.y) answer|=4;    // set bit at position 2
        if(t0.y > tm.z) answer|=1;    // set bit at position 0
        return (int) answer;
    }
}
// PLANE XY
if(tm.x < t0.z) answer|=4;    // set bit at position 2
if(tm.y < t0.z) answer|=2;    // set bit at position 1
return (int) answer;
}

static int new_node(float txm, int x, float tym, int y, float tzm, int z)
{
  return (txm < tym) ? (txm < tzm ? x : z) : (tym < tzm ? y : z);
}

static int new_node(uint32_t i, float txm, float tym, float tzm)
{
  uint32_t x = 4 + i;
  uint32_t y = 2 + (i&0x5) + ((i&0x2) << 3);
  uint32_t z = 1 + (i&0x6) + ((i&0x1) << 3);
  return (txm < tym) ? (txm < tzm ? x : z) : (tym < tzm ? y : z);
}

//Octahedral Normal Vectors (ONV) encoding https://jcgt.org/published/0003/02/01/
static float2 encode_normal(float3 v)
{
  float2 p = float2(v.x, v.y) * (1.0f / (std::abs(v.x) + std::abs(v.y) + std::abs(v.z)));
  float2 signNotZero = float2((p.x >= 0.0f) ? +1.0f : -1.0f, (p.y >= 0.0f) ? +1.0f : -1.0f);
  return (v.z <= 0.0f) ? ((1.0f - abs(float2(p.y, p.x))) * signNotZero) : p;
}

static float2 project2planes(const float4 &P1, const float4 &P2, const float4 &point)
{
  return float2{ dot(P1, point), dot(P2, point) };
}