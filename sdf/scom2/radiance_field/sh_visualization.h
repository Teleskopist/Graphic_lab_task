#pragma once

#include "sdf/scom2/radiance_field/sh_compute.h"
#include "sdf/scom2/radiance_field/sh_rotate.h"
#include "utils/mesh/procedural_meshes.h"

static cmesh4::SimpleMesh create_sh_sphere(uint32_t deg, const float3 *sh_coeffs, uint32_t segments = 80)
{
  cmesh4::SimpleMesh mesh = cmesh4::create_sphere(segments);

  uint32_t sh_count = (deg + 1) * (deg + 1);
  for (int i = 0; i < mesh.VerticesNum(); i++)
  {
    float3 val = computeColorFromSH(float3(0.0f), to_float3(mesh.vPos4f[i]), 0, deg, sh_count, sh_coeffs);
    mesh.vTexCoord2f[i] = 1.5f * float2(val.x < 0.0f ? -val.x : 0, val.x > 0.0f ? val.x : 0);
  }

  return mesh;
}

static cmesh4::SimpleMesh create_transformed_sh_sphere(uint32_t deg, const float3 *sh_coeffs, uint32_t segments, float4x4 ray_transform)
{
  cmesh4::SimpleMesh mesh = cmesh4::create_sphere(segments);

  uint32_t sh_count = (deg + 1) * (deg + 1);
  for (int i = 0; i < mesh.VerticesNum(); i++)
  {
    float3 val = computeColorFromSH(float3(0.0f), to_float3(ray_transform * mesh.vPos4f[i]), 0, deg, sh_count, sh_coeffs);
    mesh.vTexCoord2f[i] = 1.5f * float2(val.x < 0.0f ? -val.x : 0, val.x > 0.0f ? val.x : 0);
  }

  return mesh;
}