#pragma once
#include "LiteMath.h"

using LiteMath::float3;

struct Plane
{
  float3 normal;
  uint32_t is_active;
  float3 pos;
  uint32_t _pad;
};

static Plane create_disabled_plane()
{
  Plane p;
  p.is_active = 0;
  p.normal = float3(0,0,-1);
  p.pos = float3(0,0,0);
  p._pad = 0;
  return p;
}