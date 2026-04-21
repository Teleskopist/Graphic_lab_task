#pragma once
#include "LiteMath.h"

using LiteMath::float3;
using LiteMath::normalize;
using LiteMath::max;

static constexpr uint32_t SH_MAX_DEGREE = 3;
static constexpr uint32_t SH_MAX_COEFFS = (SH_MAX_DEGREE + 1) * (SH_MAX_DEGREE + 1);

static const float SH_C0 = 0.28209479177387814f;
static const float SH_C1 = 0.4886025119029199f;
static const float SH_C2[] = {
    1.0925484305920792f,
    -1.0925484305920792f,
    0.31539156525252005f,
    -1.0925484305920792f,
    0.5462742152960396f};
static const float SH_C3[] = {
    -0.5900435899266435f,
    2.890611442640554f,
    -0.4570457994644658f,
    0.3731763325901154f,
    -0.4570457994644658f,
    1.445305721320277f,
    -0.5900435899266435f};

// Forward method for converting the input spherical harmonics
// coefficients of each Gaussian to a simple RGB color.
static float3 computeColorFromSH(float3 vox_c, float3 ro, uint32_t idx,  
                                 uint32_t deg, uint32_t sh_stride, const float3 *shs)
{
  // The implementation is loosely based on code for
  // "Differentiable Point-Based Radiance Fields for
  // Efficient View Synthesis" by Zhang et al. (2022)
  const float3 dir = normalize(vox_c - ro);

  const float3 *sh = shs + idx * sh_stride;
  float3 result = SH_C0 * sh[0];

  if (deg > 0)
  {
    float x = dir.x;
    float y = dir.y;
    float z = dir.z;
    result = result - SH_C1 * y * sh[1] + SH_C1 * z * sh[2] - SH_C1 * x * sh[3];

    if (deg > 1)
    {
      float xx = x * x, yy = y * y, zz = z * z;
      float xy = x * y, yz = y * z, xz = x * z;
      result = result +
               SH_C2[0] * xy * sh[4] +
               SH_C2[1] * yz * sh[5] +
               SH_C2[2] * (2.0f * zz - xx - yy) * sh[6] +
               SH_C2[3] * xz * sh[7] +
               SH_C2[4] * (xx - yy) * sh[8];

      if (deg > 2)
      {
        result = result +
                 SH_C3[0] * y * (3.0f * xx - yy) * sh[9] +
                 SH_C3[1] * xy * z * sh[10] +
                 SH_C3[2] * y * (4.0f * zz - xx - yy) * sh[11] +
                 SH_C3[3] * z * (2.0f * zz - 3.0f * xx - 3.0f * yy) * sh[12] +
                 SH_C3[4] * x * (4.0f * zz - xx - yy) * sh[13] +
                 SH_C3[5] * z * (xx - yy) * sh[14] +
                 SH_C3[6] * x * (xx - 3.0f * yy) * sh[15];
      }
    }
  }

  // RGB colors can be shifted and clamped to non-negative values.
  //result = max(result + 0.5f, float3(0.0f));
  
  return result;
}