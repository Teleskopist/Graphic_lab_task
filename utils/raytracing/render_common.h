#pragma once
#include "LiteMath.h"

using LiteMath::M_TWOPI, LiteMath::M_PI, LiteMath::uint3, LiteMath::fract, LiteMath::clamp, LiteMath::lerp, LiteMath::float2, LiteMath::float3x3, LiteMath::to_float3, LiteMath::float3, LiteMath::float4, LiteMath::float4x4;

// orthographic projection with right-handed coordinate system to unit cube [-1,1]^3
static inline float4x4 orthoRH_NO(float left, float right, float bottom, float top, float zNear, float zFar)
{
  float4x4 Result;
  Result(0, 0) = 2.0f / (right - left);
  Result(1, 1) = 2.0f / (top - bottom);
  Result(2, 2) = -2.0f / (zFar - zNear);
  Result(0, 3) = -(right + left) / (right - left);
  Result(1, 3) = -(top + bottom) / (top - bottom);
  Result(2, 3) = -(zFar + zNear) / (zFar - zNear);
  return Result;
}

// orthographic projection
static inline float4x4 ortho(float left, float right, float bottom, float top, float zNear, float zFar)
{
  return orthoRH_NO(left, right, bottom, top, zNear, zFar);
}

static inline float3 EyeRayDirNormalized(float x, float y, float4x4 a_mViewProjInv)
{
  float4 pos = float4(2.0f*x - 1.0f, -2.0f*y + 1.0f, 1.0f, 1.0f );
  pos = a_mViewProjInv * pos;
  pos /= pos.w;
  return normalize(to_float3(pos));
}

static inline float3 mymul3x3(float4x4 m, float3 v)
{ 
  return to_float3(m*to_float4(v, 0.0f));
}

static inline float maxcomp(float3 v) { return std::max(v.x, std::max(v.y, v.z)); }

static inline float3 mymul4x3(float4x4 m, float3 v)
{
  return to_float3(m*to_float4(v, 1.0f));
}

static inline void transform_ray3f(float4x4 a_mWorldViewInv, 
                                   float3* ray_pos, float3* ray_dir) 
{
  float3 pos  = mymul4x3(a_mWorldViewInv, (*ray_pos));
  float3 pos2 = mymul4x3(a_mWorldViewInv, ((*ray_pos) + 100.0f*(*ray_dir)));

  float3 diff = pos2 - pos;

  (*ray_pos)  = pos;
  (*ray_dir)  = normalize(diff);
}

static inline void CoordinateSystem(float3 v1, float3* v2, float3* v3)
{
  float invLen = 1.0f;

  if (std::abs(v1.x) > std::abs(v1.y))
  {
    invLen = 1.0f / std::sqrt(v1.x*v1.x + v1.z*v1.z);
    (*v2)  = float3((-1.0f) * v1.z * invLen, 0.0f, v1.x * invLen);
  }
  else
  {
    invLen = 1.0f / sqrt(v1.y * v1.y + v1.z * v1.z);
    (*v2)  = float3(0.0f, v1.z * invLen, (-1.0f) * v1.y * invLen);
  }

  (*v3) = cross(v1, (*v2));
}

static inline float3 MapSampleToCosineDistribution(float r1, float r2, float3 direction, float3 hit_norm, float power)
{
  if(power >= 1e6f)
    return direction;

  const float sin_phi = sin(M_TWOPI * r1);
  const float cos_phi = cos(M_TWOPI * r1);

  //sincos(2.0f*r1*3.141592654f, &sin_phi, &cos_phi);

  const float cos_theta = pow(1.0f - r2, 1.0f / (power + 1.0f));
  const float sin_theta = sqrt(1.0f - cos_theta*cos_theta);

  float3 deviation;
  deviation.x = sin_theta*cos_phi;
  deviation.y = sin_theta*sin_phi;
  deviation.z = cos_theta;

  float3 ny = direction, nx, nz;
  CoordinateSystem(ny, &nx, &nz);

  {
    float3 temp = ny;
    ny = nz;
    nz = temp;
  }

  float3 res = nx*deviation.x + ny*deviation.y + nz*deviation.z;

  float invSign = dot(direction, hit_norm) > 0.0f ? 1.0f : -1.0f;

  if (invSign*dot(res, hit_norm) < 0.0f) // reflected ray is below surface #CHECK_THIS
  {
    res = (-1.0f)*nx*deviation.x + ny*deviation.y - nz*deviation.z;
    //belowSurface = true;
  }

  return res;
}

constexpr float GEPSILON = 2e-5f ;
constexpr float DEPSILON = 1e-20f;
static inline float epsilonOfPos(float3 hitPos) { return std::max(std::max(std::abs(hitPos.x), std::max(std::abs(hitPos.y), std::abs(hitPos.z))), 2.0f*GEPSILON)*GEPSILON; }

/**
\brief offset reflected ray position by epsilon;
\param  a_hitPos      - world space position on surface
\param  a_surfaceNorm - surface normal at a_hitPos
\param  a_sampleDir   - ray direction in which we are going to trace reflected ray
\return offseted ray position
*/
static inline float3 OffsRayPos(const float3 a_hitPos, const float3 a_surfaceNorm, const float3 a_sampleDir)
{
  const float signOfNormal2 = dot(a_sampleDir, a_surfaceNorm) < 0.0f ? -1.0f : 1.0f;
  const float offsetEps     = epsilonOfPos(a_hitPos);
  return a_hitPos + signOfNormal2 * offsetEps * a_surfaceNorm;
}

static inline unsigned int encodeNormal(float3 n)
{
  const int x = (int)(n.x*32767.0f);
  const int y = (int)(n.y*32767.0f);

  const unsigned int sign = (n.z >= 0) ? 0 : 1;
  const unsigned int sx   = ((unsigned int)(x & 0xfffe) | sign);
  const unsigned int sy   = ((unsigned int)(y & 0xffff) << 16);

  return (sx | sy);
}

static inline float3 decodeNormal(unsigned int a_data)
{  
  const unsigned int a_enc_x = (a_data  & 0x0000FFFF);
  const unsigned int a_enc_y = ((a_data & 0xFFFF0000) >> 16);
  const float sign           = ((a_enc_x & 0x0001) != 0) ? -1.0f : 1.0f;

  const float x = ((short)(a_enc_x & 0xfffe))*(1.0f / 32767.0f);
  const float y = ((short)(a_enc_y & 0xffff))*(1.0f / 32767.0f);
  const float z = sign*std::sqrt(std::max(1.0f - x*x - y*y, 0.0f));

  return float3(x, y, z);
}


static inline uint32_t RealColorToUint32(float4 real_color)
{
  float  r = real_color[0]*255.0f;
  float  g = real_color[1]*255.0f;
  float  b = real_color[2]*255.0f;
  float  a = real_color[3]*255.0f;

  uint32_t red   = uint32_t(r);
  uint32_t green = uint32_t(g);
  uint32_t blue  = uint32_t(b);
  uint32_t alpha = uint32_t(a);

  return red | (green << 8) | (blue << 16) | (alpha << 24);
}

static inline float2 mulRows2x4(const float4 row0, const float4 row1, float2 v)
{
  float2 res;
  res.x = row0.x*v.x + row0.y*v.y + row0.w;
  res.y = row1.x*v.x + row1.y*v.y + row1.w;
  return res;
}

static inline float clamp1f(float u, float a, float b) { return std::min(std::max(a, u), b); }

static inline float3x3& operator += (float3x3& a, const float3x3 b) 
{ 
  a.m_col[0] += b.m_col[0];
  a.m_col[1] += b.m_col[1];
  a.m_col[2] += b.m_col[2]; 
  return a; 
}

static inline uint32_t BlockIndex2D(uint32_t tidX, uint32_t tidY, uint32_t a_width)
{
  const uint32_t inBlockIdX = tidX % 4; // 4x4 blocks
  const uint32_t inBlockIdY = tidY % 4; // 4x4 blocks
 
  const uint32_t localIndex = inBlockIdY*4 + inBlockIdX;
  const uint32_t wBlocks    = a_width/4;

  const uint32_t blockX     = tidX/4;
  const uint32_t blockY     = tidY/4;
  const uint32_t offset     = (blockX + blockY*wBlocks)*4*4 + localIndex;
  return offset;
}

static inline uint32_t SuperBlockIndex2D(uint32_t tidX, uint32_t tidY, uint32_t a_width)
{
  const uint32_t inBlockIdX = tidX % 4; // 4x4 blocks
  const uint32_t inBlockIdY = tidY % 4; // 4x4 blocks
  const uint32_t localIndex = inBlockIdY*4 + inBlockIdX;
  const uint32_t wBlocks    = a_width/4;
  const uint32_t blockX     = tidX/4;
  const uint32_t blockY     = tidY/4;
  
  const uint32_t inHBlockIdX = blockX % 2; // 2x2 SuperBlocks
  const uint32_t inHBlockIdY = blockY % 2; // 2x2 SuperBlocks
  const uint32_t localIndexH = inHBlockIdY*2 + inHBlockIdX;
  const uint32_t wBlocksH    = wBlocks/2;
  const uint32_t blockHX     = blockX/2;
  const uint32_t blockHY     = blockY/2;

  return (blockHX + blockHY*wBlocksH)*8*8 + localIndexH*4*4 + localIndex;
}

static inline uint32_t SuperBlockIndex2DOpt(uint32_t tidX, uint32_t tidY, uint32_t a_width)
{
  const uint32_t inBlockIdX = tidX & 0x00000003; // 4x4 blocks
  const uint32_t inBlockIdY = tidY & 0x00000003; // 4x4 blocks
  const uint32_t localIndex = inBlockIdY*4 + inBlockIdX;
  const uint32_t wBlocks    = a_width >> 2;
  const uint32_t blockX     = tidX    >> 2;
  const uint32_t blockY     = tidY    >> 2;
  
  const uint32_t inHBlockIdX = blockX & 0x00000001; // 2x2 SuperBlocks
  const uint32_t inHBlockIdY = blockY & 0x00000001; // 2x2 SuperBlocks
  const uint32_t localIndexH = inHBlockIdY*2 + inHBlockIdX;
  const uint32_t wBlocksH    = wBlocks >> 1;
  const uint32_t blockHX     = blockX  >> 1;
  const uint32_t blockHY     = blockY  >> 1;

  return (blockHX + blockHY*wBlocksH)*64 + localIndexH*16 + localIndex;
}

static uint32_t encode_RGBA8(float4 c)
{
  uint4 col = uint4(255 * clamp(c, float4(0,0,0,0), float4(1,1,1,1)));
  return (col.w<<24) | (col.z<<16) | (col.y<<8) | col.x;
}

static float4 decode_RGBA8(uint32_t c)
{
  uint4 col = uint4(c & 0xFF, (c>>8) & 0xFF, (c>>16) & 0xFF, (c>>24) & 0xFF);
  return float4(col.x * (1.0f/255.0f), col.y * (1.0f/255.0f), col.z * (1.0f/255.0f), col.w * (1.0f/255.0f));
}

// Octahedral Normal Vectors (ONV) decoding https://jcgt.org/published/0003/02/01/
static float3 decode_normal(float2 e)
{
  float3 v = float3(e.x, e.y, 1.0f - std::abs(e.x) - std::abs(e.y));
  if (v.z < 0) 
  {
    float vx = v.x;
    v.x = (1.0f - std::abs(v.y)) * ((v.x >= 0.0f) ? +1.0f : -1.0f);
    v.y = (1.0f - std::abs( vx)) * ((v.y >= 0.0f) ? +1.0f : -1.0f);
  }
  return normalize(v);
}

// http://www.jcgt.org/published/0009/03/02/
static float3 rand3(uint32_t seed, uint32_t x, uint32_t y, uint32_t iter)
{
  x = x + 1233u*(iter+seed) % 171u;
  y = y + 453u*(iter+seed) % 765u;
  uint3 v = uint3(x, y, x ^ y);

  v = v * 1664525u + 1013904223u;

  v.x += v.y * v.z;
  v.y += v.z * v.x;
  v.z += v.x * v.y;

  v.x ^= v.x >> 16u;
  v.y ^= v.y >> 16u;
  v.z ^= v.z >> 16u;

  v.x += v.y * v.z;
  v.y += v.z * v.x;
  v.z += v.x * v.y;

  return float3(v) * (1.0f/float(0xffffffffu));
}

static float2 rand2(uint32_t seed, uint32_t x, uint32_t y, uint32_t iter)
{
  float3 r3 = rand3(seed, x, y, iter);
  return float2(r3.x, r3.y);
}