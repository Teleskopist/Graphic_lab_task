#include "VolumeRenderer.h"
#include "utils/raytracing/render_common.h"

void VolumeRenderer::Clear(uint32_t width, uint32_t height)
{
  kernel2D_PackXY(width, height);
}

void VolumeRenderer::Render(uint32_t *imageData, uint32_t width, uint32_t height, int passNum)
{
  assert(m_width == width);
  assert(m_height == height);

  uint32_t samples = passNum*m_preset.spp;
  for (uint32_t sample = 0; sample < samples; sample++)
  {
    if (m_grid_type == TYPE_SDF_DAG)
    {
      if (m_SdfDAGHeaders[0].dim == 3)
      {
        kernel1D_RaymarchDAG(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
      }
      else if (m_SdfDAGHeaders[0].dim == 4)
      {
        kernel1D_RaymarchDAG4D(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
      }
    }
    else if (m_grid_type == TYPE_SCOM2)
    {
      if (m_header.dimension == 3)
      {
        kernel1D_RaymarchSCom(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
      }
      else if (m_header.dimension == 4)
      {
        kernel1D_RaymarchSCom4D(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
      }
    }
    else
    {
    switch (m_preset.traversal_mode)
      {
        case VOLUME_TRAVERSAL_MODE_BRESENHAM:
          kernel1D_RaymarchGrid_Bresenham(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
          break;
        case VOLUME_TRAVERSAL_MODE_DDA:
          kernel1D_RaymarchGrid_DDA(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
          break;
        case VOLUME_TRAVERSAL_MODE_DDA_BRANCHLESS:
          kernel1D_RaymarchGrid_DDA_Branchless(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
          break;
        case VOLUME_TRAVERSAL_MODE_VRT_SS:
          kernel1D_RaytraceGrid_SS(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
          break;
        default:
          break;
      }
    }
  }
  kernel1D_Tonemap(m_width*m_height, samples, imageData);
}

void VolumeRenderer::RenderFloat(float4 *imageData, uint32_t width, uint32_t height, int passNum)
{
  assert(m_width == width);
  assert(m_height == height);

  uint32_t samples = passNum*m_preset.spp;
  for (uint32_t sample = 0; sample < samples; sample++)
  {
    if (m_grid_type == TYPE_SDF_DAG)
    {
      if (m_SdfDAGHeaders[0].dim == 3)
      {
        kernel1D_RaymarchDAG(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
      }
      else if (m_SdfDAGHeaders[0].dim == 4)
      {
        kernel1D_RaymarchDAG4D(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
      }
    }
    else if (m_grid_type == TYPE_SCOM2)
    {
      if (m_header.dimension == 3)
      {
        kernel1D_RaymarchSCom(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
      }
      else if (m_header.dimension == 4)
      {
        kernel1D_RaymarchSCom4D(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
      }
    }
    else
    {
    switch (m_preset.traversal_mode)
      {
        case VOLUME_TRAVERSAL_MODE_BRESENHAM:
          kernel1D_RaymarchGrid_Bresenham(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
          break;
        case VOLUME_TRAVERSAL_MODE_DDA:
          kernel1D_RaymarchGrid_DDA(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
          break;
        case VOLUME_TRAVERSAL_MODE_DDA_BRANCHLESS:
          kernel1D_RaymarchGrid_DDA_Branchless(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
          break;
        case VOLUME_TRAVERSAL_MODE_VRT_SS:
          kernel1D_RaytraceGrid_SS(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
          break;
        default:
          break;
      }
    }
  }
  kernel1D_AverageColor(m_width*m_height, samples, imageData);
}

void VolumeRenderer::kernel2D_PackXY(uint32_t w, uint32_t h)
{
  #pragma omp parallel for
  for(uint32_t y=0;y<h;y++)
  {
    for(uint32_t x=0;x<w;x++)
    {
      const uint offset   = SuperBlockIndex2DOpt(x, y, m_packedXY_width);
      m_packedXY[offset] = ((y << 16) & 0xFFFF0000) | (x & 0x0000FFFF);
    }
  }
}

void VolumeRenderer::kernel1D_Tonemap(uint32_t count, uint32_t sample_count, uint32_t* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    float4 color_f = m_colorBuffer[tidX] / float(sample_count);
    uint4 col = uint4(255 * clamp(color_f, float4(0,0,0,0), float4(1,1,1,1)));
    uint32_t color_rgba8 = (col.w<<24) | (col.z<<16) | (col.y<<8) | col.x;
    out_color[tidX] = color_rgba8;
  }
}

void VolumeRenderer::kernel1D_AverageColor(uint32_t count, uint32_t sample_count, LiteMath::float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
    out_color[tidX] = m_colorBuffer[tidX] / float(sample_count);
}

void VolumeRenderer::kernel1D_RaymarchGrid_Bresenham(uint32_t count, uint32_t sample_id, float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    const uint XY = m_packedXY[tidX];
    uint x  = (XY & 0x0000FFFF);
    uint y  = (XY & 0xFFFF0000) >> 16;
  
    float4 rayPos, rayDir;
    initEyeRay(x, y, sample_id, &rayPos, &rayDir);
    float4 res = RaymarchGrid_Bresenham(rayPos, rayDir);
    m_colorBuffer[y*m_width + x] = sample_id == 0 ? res : m_colorBuffer[y*m_width + x] + res;
  }
}

void VolumeRenderer::kernel1D_RaymarchGrid_DDA(uint32_t count, uint32_t sample_id, float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    const uint XY = m_packedXY[tidX];
    uint x  = (XY & 0x0000FFFF);
    uint y  = (XY & 0xFFFF0000) >> 16;
  
    float4 rayPos, rayDir;
    initEyeRay(x, y, sample_id, &rayPos, &rayDir);
    float4 res = RaymarchGrid_DDA(rayPos, rayDir);
    m_colorBuffer[y*m_width + x] = sample_id == 0 ? res : m_colorBuffer[y*m_width + x] + res;
  }
}

void VolumeRenderer::kernel1D_RaymarchGrid_DDA_Branchless(uint32_t count, uint32_t sample_id, float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    const uint XY = m_packedXY[tidX];
    uint x  = (XY & 0x0000FFFF);
    uint y  = (XY & 0xFFFF0000) >> 16;
  
    float4 rayPos, rayDir;
    initEyeRay(x, y, sample_id, &rayPos, &rayDir);
    float4 res = RaymarchGrid_DDA_Branchless(rayPos, rayDir);
    m_colorBuffer[y*m_width + x] = sample_id == 0 ? res : m_colorBuffer[y*m_width + x] + res;
  }
}

void VolumeRenderer::kernel1D_RaytraceGrid_SS(uint32_t count, uint32_t sample_id, float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    const uint XY = m_packedXY[tidX];
    uint x  = (XY & 0x0000FFFF);
    uint y  = (XY & 0xFFFF0000) >> 16;
  
    float4 rayPos, rayDir;
    initEyeRay(x, y, sample_id, &rayPos, &rayDir);
    float4 res = RaymarchGrid_SS(rayPos, rayDir, x, y);
    m_colorBuffer[y*m_width + x] = sample_id == 0 ? res : m_colorBuffer[y*m_width + x] + res;
  }
}

void VolumeRenderer::kernel1D_RaymarchDAG(uint32_t count, uint32_t sample_id, float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    const uint XY = m_packedXY[tidX];
    uint x  = (XY & 0x0000FFFF);
    uint y  = (XY & 0xFFFF0000) >> 16;
  
    float4 rayPos, rayDir;
    initEyeRay(x, y, sample_id, &rayPos, &rayDir);
    float4 res = RaymarchDAG(rayPos, rayDir);
    m_colorBuffer[y*m_width + x] = sample_id == 0 ? res : m_colorBuffer[y*m_width + x] + res;
  }
}

void VolumeRenderer::kernel1D_RaymarchDAG4D(uint32_t count, uint32_t sample_id, float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    const uint XY = m_packedXY[tidX];
    uint x  = (XY & 0x0000FFFF);
    uint y  = (XY & 0xFFFF0000) >> 16;
  
    float4 rayPos, rayDir;
    initEyeRay(x, y, sample_id, &rayPos, &rayDir);
    float4 res = RaymarchDAG4D(rayPos, rayDir);
    m_colorBuffer[y*m_width + x] = sample_id == 0 ? res : m_colorBuffer[y*m_width + x] + res;
  }
}

void VolumeRenderer::kernel1D_RaymarchSCom(uint32_t count, uint32_t sample_id, float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    const uint XY = m_packedXY[tidX];
    uint x  = (XY & 0x0000FFFF);
    uint y  = (XY & 0xFFFF0000) >> 16;
  
    float4 rayPos, rayDir;
    initEyeRay(x, y, sample_id, &rayPos, &rayDir);
    float4 res = RaymarchSCom(rayPos, rayDir);
    m_colorBuffer[y*m_width + x] = sample_id == 0 ? res : m_colorBuffer[y*m_width + x] + res;
  }
}

void VolumeRenderer::kernel1D_RaymarchSCom4D(uint32_t count, uint32_t sample_id, float4* out_color)
{
  //#pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    const uint XY = m_packedXY[tidX];
    uint x  = (XY & 0x0000FFFF);
    uint y  = (XY & 0xFFFF0000) >> 16;
  
    float4 rayPos, rayDir;
    initEyeRay(x, y, sample_id, &rayPos, &rayDir);
    float4 res = RaymarchSCom4D(rayPos, rayDir);
    m_colorBuffer[y*m_width + x] = sample_id == 0 ? res : m_colorBuffer[y*m_width + x] + res;
  }
}

void VolumeRenderer::initEyeRay(uint32_t x, uint32_t y, uint32_t s_id, 
                                LiteMath::float4* rayPosAndNear, LiteMath::float4* rayDirAndFar)
{
  uint32_t spp_sqrt = uint32_t(sqrt(m_preset.spp));
  float i_spp_sqrt = 1.0f/spp_sqrt;
  float2 d = (m_AAFrameNum >= 2) ? rand2(m_seed, x, y, s_id + m_AAFrameNum + m_seed % 2) : 
                                   i_spp_sqrt*float2(s_id/spp_sqrt+0.5, s_id%spp_sqrt+0.5);

  float3 rayDir = EyeRayDirNormalized((float(x)+d.x)/float(m_width), (float(y)+d.y)/float(m_height), m_projInv);
  float3 rayPos = float3(0,0,0);

  transform_ray3f(m_worldViewInv, 
                  &rayPos, &rayDir);
  
  float rayNear = 1e-5f;
  float rayFar  =  1e5f;
  float sgn = sign(dot(rayPos - m_cuttingPlane.pos, m_cuttingPlane.normal));
  float denom = m_cuttingPlane.is_active > 0 ? dot(m_cuttingPlane.normal, rayDir) : 0.0f;
  rayNear = sgn*std::abs(denom) < -1e-6f ? dot(m_cuttingPlane.pos - rayPos, m_cuttingPlane.normal) / denom : rayNear;
  rayFar  = sgn*std::abs(denom) >  1e-6f ? dot(m_cuttingPlane.pos - rayPos, m_cuttingPlane.normal) / denom : rayFar;
  *rayPosAndNear = to_float4(rayPos, rayNear < 0 ? rayFar : rayNear);
  *rayDirAndFar  = to_float4(rayDir, rayFar < 0 ?    1e9f :  rayFar);
}

float VolumeRenderer::SampleD(float3 p, uint32_t a_frame_id)
{
  uint32_t off = a_frame_id*m_vox_cnt;
  float3 fp = 0.5f*float(m_sz - 1) * (p + 1.0f);
  uint3 ip = uint3(fp);
  uint3 ip0 = clamp(ip, uint3(0u), uint3(m_sz - 1));
  uint3 ip1 = clamp(ip + uint3(1u), uint3(0u), uint3(m_sz - 1));
  float3 dp = fp - float3(ip);
  
  return (1.0f-dp.z) * (1.0f-dp.y) * (1.0f-dp.x) * m_grid[off+ip0.x*m_sz*m_sz + ip0.y*m_sz + ip0.z] +
         (     dp.z) * (1.0f-dp.y) * (1.0f-dp.x) * m_grid[off+ip0.x*m_sz*m_sz + ip0.y*m_sz + ip1.z] +
         (1.0f-dp.z) * (     dp.y) * (1.0f-dp.x) * m_grid[off+ip0.x*m_sz*m_sz + ip1.y*m_sz + ip0.z] +
         (     dp.z) * (     dp.y) * (1.0f-dp.x) * m_grid[off+ip0.x*m_sz*m_sz + ip1.y*m_sz + ip1.z] +
         (1.0f-dp.z) * (1.0f-dp.y) * (     dp.x) * m_grid[off+ip1.x*m_sz*m_sz + ip0.y*m_sz + ip0.z] +
         (     dp.z) * (1.0f-dp.y) * (     dp.x) * m_grid[off+ip1.x*m_sz*m_sz + ip0.y*m_sz + ip1.z] +
         (1.0f-dp.z) * (     dp.y) * (     dp.x) * m_grid[off+ip1.x*m_sz*m_sz + ip1.y*m_sz + ip0.z] +
         (     dp.z) * (     dp.y) * (     dp.x) * m_grid[off+ip1.x*m_sz*m_sz + ip1.y*m_sz + ip1.z];
}


float4 VolumeRenderer::SampleDC(float3 p, uint32_t a_frame_id)
{
  uint32_t off = a_frame_id*m_vox_cnt;
  float3 fp = 0.5f*float(m_sz - 1) * (p + 1.0f);
  uint3 ip = uint3(fp);
  uint3 ip0 = clamp(ip, uint3(0u), uint3(m_sz - 1));
  uint3 ip1 = clamp(ip + uint3(1u), uint3(0u), uint3(m_sz - 1));
  float3 dp = fp - float3(ip);
  
  return (1.0f-dp.z) * (1.0f-dp.y) * (1.0f-dp.x) * m_colored_grid[off+ip0.x*m_sz*m_sz + ip0.y*m_sz + ip0.z] +
         (     dp.z) * (1.0f-dp.y) * (1.0f-dp.x) * m_colored_grid[off+ip0.x*m_sz*m_sz + ip0.y*m_sz + ip1.z] +
         (1.0f-dp.z) * (     dp.y) * (1.0f-dp.x) * m_colored_grid[off+ip0.x*m_sz*m_sz + ip1.y*m_sz + ip0.z] +
         (     dp.z) * (     dp.y) * (1.0f-dp.x) * m_colored_grid[off+ip0.x*m_sz*m_sz + ip1.y*m_sz + ip1.z] +
         (1.0f-dp.z) * (1.0f-dp.y) * (     dp.x) * m_colored_grid[off+ip1.x*m_sz*m_sz + ip0.y*m_sz + ip0.z] +
         (     dp.z) * (1.0f-dp.y) * (     dp.x) * m_colored_grid[off+ip1.x*m_sz*m_sz + ip0.y*m_sz + ip1.z] +
         (1.0f-dp.z) * (     dp.y) * (     dp.x) * m_colored_grid[off+ip1.x*m_sz*m_sz + ip1.y*m_sz + ip0.z] +
         (     dp.z) * (     dp.y) * (     dp.x) * m_colored_grid[off+ip1.x*m_sz*m_sz + ip1.y*m_sz + ip1.z];
}

float4 VolumeRenderer::Sample(float3 pos)
{
  float4 sample0 = m_grid_type == TYPE_DENSITY_GRID ? 
                   float4(m_baseColor.x, m_baseColor.y, m_baseColor.z, SampleD(pos, frame0)) : 
                   SampleDC(pos, frame0);
  float4 sample1 = frame0 == frame1 ? sample0 : (m_grid_type == TYPE_DENSITY_GRID ? 
                                                 float4(m_baseColor.x, m_baseColor.y, m_baseColor.z, SampleD(pos, frame1)) : 
                                                 SampleDC(pos, frame1));
  return (1.0f - m_dframe) * sample0 + m_dframe * sample1;
}

float3 VolumeRenderer::computeColorFromSH(float3 vox_c, float3 ro, uint32_t idx)
{
  if (m_SphericalHarmonics.size() == 0) return float3(1,0,1);

  const float3 dir = normalize(vox_c - ro);
  float4 result = 0.28209479177387814f * m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 0];

  float x = dir.x;
  float y = dir.y;
  float z = dir.z;
  result = result + 0.4886025119029199f * (-y * m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 1]
                                           +z * m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 2] 
                                           -x * m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 3]);

  // printf("v = %f %f %f, q= %f %f %f\n", x, y, z, 
  //   m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 1].x, 
  //   m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 2].y, 
  //   m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 3].z);

  float xx = x * x, yy = y * y, zz = z * z;
  float xy = x * y, yz = y * z, xz = x * z;
  result = result +
           (1.0925484305920792f)  * xy *                    m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 4] +
           (-1.0925484305920792f) * yz *                    m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 5] +
           (0.31539156525252005f) * (2.0f * zz - xx - yy) * m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 6] +
           (-1.0925484305920792f) * xz *                    m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 7] +
           0.5462742152960396f    * (xx - yy) *             m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 8];

  result = result +
           (-0.5900435899266435f) * y * (3.0f * xx - yy) *                    m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 9] +
           (2.890611442640554f)   * xy * z *                                  m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 10] +
           (-0.4570457994644658f) * y * (4.0f * zz - xx - yy) *               m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 11] +
           (0.3731763325901154f)  * z * (2.0f * zz - 3.0f * xx - 3.0f * yy) * m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 12] +
           (-0.4570457994644658f) * x * (4.0f * zz - xx - yy) *               m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 13] +
           (1.445305721320277f)   * z * (xx - yy) *                           m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 14] +
           (-0.5900435899266435f) * x * (xx - 3.0f * yy) *                    m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 15];
  //printf("result = %f, %f, %f\n", result.x, result.y, result.z);
  // RGB colors is shifted and clamped to obtain non-negative values.
  result = max(result + 0.5f, float4(0.0f));
  //printf("result = %f, %f, %f\n", result.x, result.y, result.z);
  return to_float3(result);
}

float4 VolumeRenderer::RaymarchGrid_Bresenham(float4 rayPos, float4 rayDir)
{
  // Normalize rayDir
  const float3 dir = to_float3(rayDir);
  const float3 dir_inv = SafeInverse(dir);

  float step = 1.0f / m_sz; // half voxel size for smooth interpolation

  float2 tNearFar = RayBoxIntersection2(to_float3(rayPos), SafeInverse(dir), float3(-1.0f, -1.0f, -1.0f), float3(1.0f, 1.0f, 1.0f));
  tNearFar.x = std::max(tNearFar.x, rayPos.w);
  tNearFar.y = std::min(tNearFar.y, rayDir.w);
  float alpha = 1.0f;
  float t = tNearFar.x;
  float3 pos = to_float3(rayPos) + t * dir;
  float3 accumulatedColor = float3(0.0f);

  while (t < tNearFar.y && alpha > m_preset.alpha_thr)
  {
    float4 sample = Sample(pos);
    float density = sample.w;

    // Standard volume raymarching blend (front-to-back compositing)
    // Here, we interpret density as alpha per step
    float localAlpha = 1.0f - std::exp(-density * step);
    accumulatedColor += alpha * localAlpha * to_float3(sample);
    alpha *= 1.0f - localAlpha; // composite

    t += step;
    pos = to_float3(rayPos) + t * dir;
  }

  return to_float4(accumulatedColor + alpha*to_float3(m_backgroundColor), 1);
}

float4 VolumeRenderer::RaymarchGrid_SS(float4 posAndNear, float4 dirAndFar, uint32_t x, uint32_t y)
{
  float3 C = float3(0.0f);
  float3 T = float3(1.0f);
  RaytraceGrid_SS(posAndNear, dirAndFar, 0, 0, 0, false, &C, &T);
  return to_float4(C + T*to_float3(m_backgroundColor), 1);
}

void VolumeRenderer::RaytraceGrid_SS(float4 rayPos, float4 rayDir, uint32_t x, uint32_t y, uint32_t bounce, bool is_shadow, 
                                     float3* out_C, float3* out_T)
{
  float3 m_LightDir = normalize(float3(1,1,1));
  float3 m_LightColor = float3(1,1,1);

  // Normalize rayDir
  const float3 dir = to_float3(rayDir);
  const float3 dir_inv = SafeInverse(dir);

  float step = 1.0f / float(m_preset.voxel_traversal_steps*m_sz); // half voxel size for smooth interpolation

  float2 tNearFar = RayBoxIntersection2(to_float3(rayPos), SafeInverse(dir), float3(-1.0f, -1.0f, -1.0f), float3(1.0f, 1.0f, 1.0f));
  tNearFar.x = std::max(tNearFar.x, rayPos.w);
  tNearFar.y = std::min(tNearFar.y, rayDir.w);
  float t = tNearFar.x + (is_shadow ? 0.5f*step : rand2(m_seed, x, y, m_AAFrameNum).x*step);
  float3 pos = to_float3(rayPos) + t * dir;

  float3 T = float3(1.0f);
  float3 C = float3(0.0f);

  while (t < tNearFar.y && T.x > m_preset.alpha_thr)
  {
    float3 pos = to_float3(rayPos) + t * dir;
    float4 sample = Sample(pos);
    float density = sample.w;
    float3 sampleT = float3(exp(-1.0f*density * step));

    float3 light_attenuation = float3(1.0f);
    if (m_preset.use_lighting == 1 && density > 3.0f*m_preset.alpha_thr)
    {
      float3 lightC;
      RaytraceGrid_SS_shadow(to_float4(pos, 0.0001f), to_float4(m_LightDir, rayDir.w), x, y, bounce+1, true, &lightC, &light_attenuation);
    }

    C += T * (1-sampleT) * m_LightColor * light_attenuation * to_float3(sample);
    T *= sampleT; // composite

    t += step;
  }

  *out_C = C;
  *out_T = T;
}

void VolumeRenderer::RaytraceGrid_SS_shadow(float4 rayPos, float4 rayDir, uint32_t x, uint32_t y, uint32_t bounce, bool is_shadow, 
                                            float3* out_C, float3* out_T)
{
  float3 m_LightDir = normalize(float3(1,1,1));
  float3 m_LightColor = float3(1,1,1);

  // Normalize rayDir
  const float3 dir = to_float3(rayDir);
  const float3 dir_inv = SafeInverse(dir);

  float step = 1.0f / m_sz; // half voxel size for smooth interpolation

  float2 tNearFar = RayBoxIntersection2(to_float3(rayPos), SafeInverse(dir), float3(-1.0f, -1.0f, -1.0f), float3(1.0f, 1.0f, 1.0f));
  tNearFar.x = std::max(tNearFar.x, rayPos.w);
  tNearFar.y = std::min(tNearFar.y, rayDir.w);
  float t = tNearFar.x + (is_shadow ? 0.0f*step : rand2(m_seed, x, y, m_AAFrameNum).x*step);
  float3 pos = to_float3(rayPos) + t * dir;

  float3 T = float3(1.0f);
  float3 C = float3(0.0f);

  while (t < tNearFar.y && T.x > m_preset.alpha_thr)
  {
    float3 pos = to_float3(rayPos) + t * dir;
    float4 sample = Sample(pos);
    float density = sample.w;
    float3 sampleT = float3(exp(-1.0f*density * step));

    float3 light_attenuation = float3(1.0f);
    // if (density > m_preset.alpha_thr && is_shadow == false)
    // {
    //   float3 lightC;
    //   RaytraceGrid_SS_shadow(rayPos, rayDir, x, y, bounce+1, true, &C, &light_attenuation);
    // }

    C += T * (1-sampleT) * m_LightColor * light_attenuation * to_float3(sample);
    T *= sampleT; // composite


    t += step;
  }

  *out_C = C;
  *out_T = T;
}

float4 VolumeRenderer::RaymarchGrid_DDA(float4 rayPos, float4 rayDir)
{
  float3 m_LightDir = normalize(float3(1,1,1));
  float3 m_LightColor = float3(1,1,1);

  float3 dir = to_float3(rayDir);
  float3 dir_inv = SafeInverse(dir);

  // Compute intersection with the bounding box of the volume (unit cube)
  float2 tNearFar = RayBoxIntersection2(to_float3(rayPos), dir_inv, float3(-1.0f), float3(1.0f));
  tNearFar.x = std::max(tNearFar.x, rayPos.w);
  tNearFar.y = std::min(tNearFar.y, rayDir.w);

  if (tNearFar.x > tNearFar.y)
    return m_backgroundColor; // No intersection

  // DDA setup
  float3 pos = to_float3(rayPos) + tNearFar.x * dir;
  float3 voxel = floor(pos * (m_sz - 1));
  float3 deltaT = abs(dir_inv) / (m_sz - 1);

  float3 step;
  float3 nextT;
  for (int i = 0; i < 3; ++i)
  {
    if (dir[i] > 0)
    {
      step[i] = 1;
      nextT[i] = (max(-1.0f,(voxel[i] + 1) / (m_sz - 1)) - rayPos[i]) / dir[i];
    }
    else
    {
      step[i] = -1;
      nextT[i] = (min(1.0f,(voxel[i]) / (m_sz - 1)) - rayPos[i]) / dir[i];
    }
    if (dir[i] == 0)
      nextT[i] = 1e9f;
  }

  float alpha = 1.0f;
  float3 accumulatedColor = float3(0.0f);
  float t = tNearFar.x;

  while (t < tNearFar.y && alpha > m_preset.alpha_thr)
  {
    int minAxis = 0;
    if (nextT[1] < nextT[minAxis])
      minAxis = 1;
    if (nextT[2] < nextT[minAxis])
      minAxis = 2;
    // Sample and compositing
    float4 sample = Sample(to_float3(rayPos) + (t + nextT[minAxis]) * 0.5f * dir);
    float density = sample.w;
    float localAlpha = 1.0f - std::exp(-density * (nextT[minAxis] - t));

    float light_attenuation = 1.0f;
    if (m_preset.use_lighting == 1 && density > 3.0f*m_preset.alpha_thr)
    {
      RaymarchGrid_DDA_SS_shadow(to_float4(to_float3(rayPos) + (t + nextT[minAxis]) * 0.5f * dir, 0.0001f), to_float4(m_LightDir, rayDir.w), &light_attenuation);
    }
    accumulatedColor += alpha * localAlpha * light_attenuation * to_float3(sample);
    alpha *= 1.0f - localAlpha;

    // Step to next voxel boundary

    t = nextT[minAxis];
    pos = to_float3(rayPos) + nextT[minAxis] * dir;
    voxel[minAxis] += step[minAxis];
    nextT[minAxis] += deltaT[minAxis];
  }

  return to_float4(alpha * to_float3(m_backgroundColor) + accumulatedColor, 1);
}

void VolumeRenderer::RaymarchGrid_DDA_SS_shadow(float4 rayPos, float4 rayDir, float *out_T)
{
  *out_T = 1.0f;
  
  float3 m_LightDir = normalize(float3(1,1,1));
  float3 m_LightColor = float3(1,1,1);

  float3 dir = to_float3(rayDir);
  float3 dir_inv = SafeInverse(dir);

  // Compute intersection with the bounding box of the volume (unit cube)
  float2 tNearFar = RayBoxIntersection2(to_float3(rayPos), dir_inv, float3(-1.0f), float3(1.0f));
  tNearFar.x = std::max(tNearFar.x, rayPos.w);
  tNearFar.y = std::min(tNearFar.y, rayDir.w);

  if (tNearFar.x > tNearFar.y)
    return; // No intersection

  // DDA setup
  float3 pos = to_float3(rayPos) + tNearFar.x * dir;
  float3 voxel = floor(pos * (m_sz - 1));
  float3 deltaT = abs(dir_inv) / (m_sz - 1);

  float3 step;
  float3 nextT;
  for (int i = 0; i < 3; ++i)
  {
    if (dir[i] > 0)
    {
      step[i] = 1;
      nextT[i] = (max(-1.0f,(voxel[i] + 1) / (m_sz - 1)) - rayPos[i]) / dir[i];
    }
    else
    {
      step[i] = -1;
      nextT[i] = (min(1.0f,(voxel[i]) / (m_sz - 1)) - rayPos[i]) / dir[i];
    }
    if (dir[i] == 0)
      nextT[i] = 1e9f;
  }

  float alpha = 1.0f;
  float3 accumulatedColor = float3(0.0f);
  float t = tNearFar.x;

  while (t < tNearFar.y && alpha > m_preset.alpha_thr)
  {
    int minAxis = 0;
    if (nextT[1] < nextT[minAxis])
      minAxis = 1;
    if (nextT[2] < nextT[minAxis])
      minAxis = 2;
    // Sample and compositing
    float4 sample = Sample(to_float3(rayPos) + (t + nextT[minAxis]) * 0.5f * dir);
    float density = sample.w;
    float localAlpha = 1.0f - std::exp(-density * (nextT[minAxis] - t));

    float light_attenuation = 1.0f;
    // if (m_preset.use_lighting == 1 && density > 3.0f*m_preset.alpha_thr)
    // {
    //   RaymarchGrid_DDA_SS_shadow(to_float4(to_float3(rayPos) + (t + nextT[minAxis]) * 0.5f * dir, 0.0001f), to_float4(m_LightDir, rayDir.w), &light_attenuation);
    // }
    accumulatedColor += alpha * localAlpha * light_attenuation * to_float3(sample);
    alpha *= 1.0f - localAlpha;

    // Step to next voxel boundary

    t = nextT[minAxis];
    pos = to_float3(rayPos) + nextT[minAxis] * dir;
    voxel[minAxis] += step[minAxis];
    nextT[minAxis] += deltaT[minAxis];
  }

  *out_T = alpha;
}

float4 VolumeRenderer::RaymarchGrid_DDA_Branchless(float4 rayPos, float4 rayDir)
{
  float3 dir = to_float3(rayDir);
  float3 dir_inv = SafeInverse(dir);

  // Compute intersection with the bounding box of the volume (unit cube)
  float2 tNearFar = RayBoxIntersection2(to_float3(rayPos), dir_inv, float3(-1.0f), float3(1.0f));
  tNearFar.x = std::max(tNearFar.x, rayPos.w);
  tNearFar.y = std::min(tNearFar.y, rayDir.w);

  if (tNearFar.x > tNearFar.y)
    return m_backgroundColor; // No intersection

  // DDA setup
  float3 pos = to_float3(rayPos) + tNearFar.x * dir;
  float3 voxel = floor(pos * m_sz);
  float3 deltaT = abs(dir_inv) / m_sz;

  // Compute step direction (+1 or -1) using sign function (no branch)
  float3 step = sign(dir);

  // Compute nextT for each axis, the distance to next voxel boundary along ray
  float3 voxel_boundary = (voxel + max(step, float3(0.0f))) / m_sz;
  float3 nextT = (voxel_boundary - pos) * dir_inv;

  float alpha = 1.0f;
  float3 accumulatedColor = float3(0.0f);
  float t = tNearFar.x;

  // Main loop: branchless voxel traversal
  while (t < tNearFar.y && alpha > m_preset.alpha_thr)
  {
    // Sample and compositing
    float4 sample = Sample(pos);
    float density = sample.w;
    float localAlpha = 1.0f - std::exp(-density * (1.0f / m_sz));
    accumulatedColor += alpha * localAlpha * to_float3(sample);
    alpha *= 1.0f - localAlpha;

    // Select axis with minimum nextT without branching:
    // Compute mask for minimum axis: masking to 0 or 1
    bool minX = (nextT.x <= nextT.y) && (nextT.x <= nextT.z);
    bool minY = (nextT.y < nextT.x) && (nextT.y <= nextT.z);
    bool minZ = !minX && !minY;
    int minAxis = minX ? 0 : (minY ? 1 : 2);

    t += nextT[minAxis];
    pos += nextT[minAxis] * dir;
    voxel[minAxis] += step[minAxis];
    nextT[minAxis] = deltaT[minAxis];
  }

  return alpha * m_backgroundColor + to_float4(accumulatedColor, 1);
}

float eval_dist_trilinear(const float values[8], float3 dp)
{
  return (1-dp.x)*(1-dp.y)*(1-dp.z)*values[0] + 
         (1-dp.x)*(1-dp.y)*(  dp.z)*values[1] + 
         (1-dp.x)*(  dp.y)*(1-dp.z)*values[2] + 
         (1-dp.x)*(  dp.y)*(  dp.z)*values[3] + 
         (  dp.x)*(1-dp.y)*(1-dp.z)*values[4] + 
         (  dp.x)*(1-dp.y)*(  dp.z)*values[5] + 
         (  dp.x)*(  dp.y)*(1-dp.z)*values[6] + 
         (  dp.x)*(  dp.y)*(  dp.z)*values[7];
}

static float exp_linear_11(float x)
{
  return (x > 1.1f) ? x : std::exp(0.909090909091f * x - 0.904689820196f);
}

struct OTDeepStackElement
{
  uint32_t nodeId;
  uint32_t info;
  uint32_t i;
  uint32_t j;
  uint32_t k;
  uint32_t size;
};

float4 VolumeRenderer::RaymarchDAG(float4 posAndNear, float4 dirAndFar)
{
#ifndef DISABLE_SDF_DAG
  //It is a naive version that iterates all child nodes, instead of traversing it in octree-like fashion
  //but DAG is a mostly for debugging similarity compression, so it does not really matter
  //Still, we should optimize it if performance testing of octree/64-tree is considered

  float3 ray_pos = to_float3(posAndNear);
  const float3 ray_dir = to_float3(dirAndFar);
  const float tNear = posAndNear.w;
  const float tFar = dirAndFar.w;
  const float3 ray_dir_inv = SafeInverse(ray_dir);
  OTDeepStackElement stack[48];

  int top = 0;
  int buf_top = 0;
  uint3 p;
  float3 p_f, t0, t1, tm;
  int currNode;
  uint32_t nodeId;
  uint32_t level_sz;
  float d, level_sz_f, sz_inv;
  float2 fNearFar;
  float values[8];
  SdfDAGHeader header = m_SdfDAGHeaders[0];//m_SdfDAGHeaders[m_geomData[geomId].offset.y];
  if (m_preset.volume_type == VOLUME_TYPE_SVRASTER_RF)
  {
    float3 scene_center;
    scene_center.x = header.user_params[SCENE_CENTER_X_USER_PARAM_ID];
    scene_center.y = header.user_params[SCENE_CENTER_Y_USER_PARAM_ID];
    scene_center.z = header.user_params[SCENE_CENTER_Z_USER_PARAM_ID];
    ray_pos = (ray_pos - scene_center) * 2.0f / header.user_params[SCENE_EXTENT_USER_PARAM_ID];
  }

  stack[top].nodeId = 0;//m_geomData[geomId].offset.x;
  stack[top].info = 0; //info here is cumulative rotation id of the node
  stack[top].i = 0;
  stack[top].j = 0;
  stack[top].k = 0;
  stack[top].size = 1;

  float alpha = 1.0f;
  float3 accumulatedColor = float3(0.0f);

  while (top >= 0 && alpha > m_preset.alpha_thr)
  {
    OTDeepStackElement curElem = stack[top];
    top--;

    SdfDAGNode node = m_SdfDAGNodes[curElem.nodeId];
    level_sz = curElem.size;
    level_sz_f = float(level_sz);
    p = uint3(curElem.i, curElem.j, curElem.k);
    
    sz_inv = 2.0f/level_sz_f;
    float3 brick_min_pos = float3(-1,-1,-1) + sz_inv*float3(p);
    float3 brick_max_pos = brick_min_pos + sz_inv*float3(1,1,1);
    float3 brick_size = brick_max_pos - brick_min_pos;

    float2 brick_fNearFar = RayBoxIntersection2(ray_pos, ray_dir_inv, brick_min_pos, brick_max_pos);
    brick_fNearFar.x = std::max(tNear, brick_fNearFar.x);

#ifdef ON_CPU
    //printf("[%d %d %d %d] node %d rot %d\n", p.x, p.y, p.z, level_sz, curElem.nodeId, curElem.info);
#endif
    if (brick_fNearFar.x >= brick_fNearFar.y)
    {
      continue;
    }

    bool is_leaf = node.children_edges_offset == 0;
    // if (!is_leaf)
    // {
    //   float t = m_preset.fixed_lod > 0 ? 1.0f : brick_fNearFar.x;
    //   float target_lod_size = (std::pow(2.0f, m_preset.level_of_detail) - 0.01f) / t;
    //   is_leaf = float(level_sz) > target_lod_size;
    // }

    if (is_leaf)
    {
      //intersect with grid inside leaf
      if (DAG_node_is_full(node.voxel_count_flags) && node.data_edges_offset > 0)
      {
        //intersect leaf
        float sz = level_sz_f;
        d = 2.0f/(sz*header.brick_size);
        bool is_surface = DAG_extract_is_surface(node.voxel_count_flags);
        uint32_t surface_count = DAG_extract_count(node.voxel_count_flags);
        uint32_t primId = node.channels_edge.child_offset; // primId is used to access channel data in Resolve pass
                                                           // so we must put channels offset here to apply sim. comp. to channels

        while (brick_fNearFar.x < brick_fNearFar.y && alpha > m_preset.alpha_thr)
        {
          uint32_t v_size = header.brick_size + 2*header.brick_pad + 1;
          float3 hit_pos = ray_pos + brick_fNearFar.x*ray_dir;
          float3 local_pos = (hit_pos - brick_min_pos) * (0.5f*sz*header.brick_size);
          float3 voxelPos = floor(clamp(local_pos, 1e-6f, header.brick_size-1e-6f));

          float3 min_pos = brick_min_pos + d*voxelPos;
          float3 max_pos = min_pos + d*float3(1,1,1);
          float3 size = max_pos - min_pos;

          uint32_t s_id = 0;
          while (s_id < surface_count && alpha > m_preset.alpha_thr)
          {
            uint32_t offset = m_SdfDAGDataEdges[node.data_edges_offset + s_id].data_offset;
            int4 voxelPosI = int4(int(voxelPos.x), int(voxelPos.y), int(voxelPos.z), 1);
            uint32_t rotIdx = m_RotAddTable[curElem.info*ROT_COUNT + m_SdfDAGDataEdges[node.data_edges_offset + s_id].rotation_id];
            float add = m_SdfDAGDataEdges[node.data_edges_offset + s_id].add;

            for (int i=0;i<8;i++)
            {
              int4 pI = voxelPosI + int4((i & 4) >> 2, (i & 2) >> 1, i & 1, 0);
              uint32_t vId0 = uint32_t(dot(m_SdfCompactOctreeRotModifiers[rotIdx], pI));
              float val = m_SdfDAGDistances[offset + vId0] + add;
              values[i] = val;
            }

            fNearFar = RayBoxIntersection2(ray_pos, SafeInverse(ray_dir), min_pos, max_pos);
            if (fNearFar.x < fNearFar.y)    
            {
              float3 start_pos = ray_pos + fNearFar.x*ray_dir;
              float3 end_pos   = ray_pos + fNearFar.y*ray_dir;
              float vox_sz = (0.5f*sz*header.brick_size);
              float3 start_q = vox_sz * (start_pos - min_pos);
              float3 end_q = vox_sz * (end_pos - min_pos);
          
              // Sample and compositing
              float i_steps = 1.0f/float(m_preset.voxel_traversal_steps);
              for (int smp = 0; smp < m_preset.voxel_traversal_steps; smp++)
              {
                float smp_q = i_steps*(smp + 0.5f);
                float density = -1.0f*eval_dist_trilinear(values, start_q * (1.0f - smp_q) + end_q * smp_q);
                float3 sample_color = to_float3(m_baseColor);
                if (m_preset.volume_type == VOLUME_TYPE_SVRASTER_RF)
                {
                  density = 100.0f*header.user_params[SCENE_EXTENT_USER_PARAM_ID]*exp_linear_11(density);
                  float4 v = to_float4(ray_pos - 0.5f*(start_pos + end_pos), 0.0f);
                  uint32_t chRotIdx = m_RotAddTable[curElem.info*ROT_COUNT + node.channels_edge.rotation_id];
                  v = m_DAGInverseTransforms[chRotIdx] * v;

                  sample_color = computeColorFromSH(float3(0,0,0), to_float3(v), node.channels_edge.child_offset);
                }
                density = std::max(density, 0.0f);
                float localAlpha = 1.0f - std::exp(-density * i_steps * length(end_pos - start_pos));
                accumulatedColor += alpha * localAlpha * sample_color;
                alpha *= 1.0f - localAlpha;
              }
            }
            s_id++;
          }
          brick_fNearFar.x += std::max(0.0f, fNearFar.y-brick_fNearFar.x) + 1e-5f;
        }
      }
    }
    else if (brick_fNearFar.x < brick_fNearFar.y)
    {
      //traverse children and add to stack (in reverse order)
      int v_sz = header.node_grid_size;
      float t_neg = (brick_fNearFar.y + tNear + 1);
      float3 neg_pos = ray_pos + t_neg * ray_dir;
      float3 neg_dir = -ray_dir;
      float3 neg_dir_inv = -ray_dir_inv;
      float2 neg_brick_fNearFar = float2(t_neg - brick_fNearFar.y, t_neg - brick_fNearFar.x);

      while (neg_brick_fNearFar.x < neg_brick_fNearFar.y)
      {
        d = 2.0f/(level_sz_f*v_sz);
        float3 hit_pos = neg_pos + neg_brick_fNearFar.x*neg_dir;
        float3 local_pos = (hit_pos - brick_min_pos) * (0.5f*level_sz_f*v_sz);
        float3 voxelPos = floor(clamp(local_pos, 1e-6f, v_sz-1e-6f));
        uint32_t voxId = uint32_t(dot(m_SdfCompactOctreeRotModifiers[2*ROT_COUNT + curElem.info], 
                                      int4((int)voxelPos.x, (int)voxelPos.y, (int)voxelPos.z, 1)));

        float3 min_pos = brick_min_pos + d*voxelPos;
        float3 max_pos = min_pos + d*float3(1,1,1);
        float3 size = max_pos - min_pos;

        fNearFar = RayBoxIntersection2(neg_pos, neg_dir_inv, min_pos, max_pos);
        SdfDAGChildEdge edge = m_SdfDAGChildEdges[node.children_edges_offset + voxId];

        //if we itersected with this node bbox and this node is valid
        if (tNear < fNearFar.x && edge.child_offset > 0)    
        {
          SdfDAGNode child_node = m_SdfDAGNodes[edge.child_offset];
          top++;
          stack[top].nodeId = edge.child_offset;
          stack[top].info = m_RotAddTable[curElem.info*ROT_COUNT + edge.rotation_id];
          stack[top].i = curElem.i*v_sz + uint(voxelPos.x);
          stack[top].j = curElem.j*v_sz + uint(voxelPos.y);
          stack[top].k = curElem.k*v_sz + uint(voxelPos.z);
          stack[top].size = curElem.size*v_sz;
        }

        neg_brick_fNearFar.x += std::max(0.0f, fNearFar.y-neg_brick_fNearFar.x) + 0.01f*d;
      }
    }
  }
  return alpha * m_backgroundColor + to_float4(accumulatedColor, 1);
#endif
}

float4 VolumeRenderer::RaymarchSCom(float4 posAndNear, float4 dirAndFar)
{
  float3 m_LightDir = normalize(float3(1,1,1));
  float3 m_LightColor = float3(1,1,1);

  if (m_preset.volume_type == VOLUME_TYPE_SVRASTER_RF)
  {
    float3 scene_center;
    scene_center.x = m_header.user_params[SCENE_CENTER_X_USER_PARAM_ID];
    scene_center.y = m_header.user_params[SCENE_CENTER_Y_USER_PARAM_ID];
    scene_center.z = m_header.user_params[SCENE_CENTER_Z_USER_PARAM_ID];
    posAndNear.x = (posAndNear.x - scene_center.x) * 2.0f / m_header.user_params[SCENE_EXTENT_USER_PARAM_ID];
    posAndNear.y = (posAndNear.y - scene_center.y) * 2.0f / m_header.user_params[SCENE_EXTENT_USER_PARAM_ID];
    posAndNear.z = (posAndNear.z - scene_center.z) * 2.0f / m_header.user_params[SCENE_EXTENT_USER_PARAM_ID];
  }
#ifndef DISABLE_SCOM2
  float2 boxNearFar = RayBoxIntersection2(to_float3(posAndNear), SafeInverse(to_float3(dirAndFar)), float3(-1,-1,-1), float3(1,1,1));
  if (boxNearFar.x > boxNearFar.y) 
    return m_backgroundColor;

  float3 pos_ray_pos = to_float3(posAndNear);
  float3 pos_ray_dir = to_float3(dirAndFar);

  //assume octree is box [-1,1]^3
  uint32_t a = 0;
  if (dirAndFar.x < 0)
  {
    pos_ray_pos.x *= -1;
    pos_ray_dir.x *= -1;
    a |= 4;
  }

  if (dirAndFar.y < 0)
  {
    pos_ray_pos.y *= -1;
    pos_ray_dir.y *= -1;
    a |= 2;
  }

  if (dirAndFar.z < 0)
  {
    pos_ray_pos.z *= -1;
    pos_ray_dir.z *= -1;
    a |= 1;
  }

  pos_ray_pos *= -1;
  const float3 pos_ray_dir_inv = SafeInverse(pos_ray_dir);
  const float3 _t0 = pos_ray_pos * pos_ray_dir_inv - pos_ray_dir_inv;
  const float3 _t1 = pos_ray_pos * pos_ray_dir_inv + pos_ray_dir_inv;
  const float3 _l = _t1 - _t0;

  ExtStackElement stack[16];

  int top = 0;
  float d = 1.0f;
  float3 t0 = _t0;
  float3 tm = t0 + 0.5f*_l;

  ExtStackElement cur;
  {
    uint32_t node = m_SComNodes[m_header.root_node_off];
    uint32_t currNode = first_node(t0, tm);

    cur.linksOff = m_header.root_node_off + m_header.links_offset + ((node & 0xFF) == SCOM2_CHILD_EMPTY ? 0 : 1+m_header.has_channels);
    cur.info = currNode | (node & 0xFFFFFF00u);
    cur.transform = 0;
    cur.p_size = uint2(0,1);
  }

  stack[0].linksOff = 0xFFFFFFFFu;
  stack[0].info = 0;
  stack[0].transform = 0;
  stack[0].p_size = uint2(0,0);

  float alpha = 1.0f;
  float3 accumulatedColor = float3(0.0f);

  while (top >= 0 && alpha > m_preset.alpha_thr)
  {
    uint32_t currNode = cur.info & 0x7;
    uint32_t rawChildNode = currNode ^ a; 
    uint3 voxelPos  = uint3((rawChildNode & 4) >> 2, (rawChildNode & 2) >> 1, rawChildNode & 1);
    uint32_t childN = uint32_t(dot(m_SdfCompactOctreeRotModifiers[2 * ROT_COUNT + cur.transform], // child node number, from 0 to 8
                                   int4((int)voxelPos.x, (int)voxelPos.y, (int)voxelPos.z, 1)));
    uint32_t childLink = cur.linksOff + bitCount((cur.info >> 24) & ((1u << childN) - 1));
    uint32_t childHasData = cur.info & (1u << (24+childN));
    uint32_t childIsLeaf  = cur.info & (1u << (8+2*childN));

    d = 1.0f / float(cur.p_size.y & 0xFFFF);
    float3 pf2 = float3(float(cur.p_size.x >> 16) + (((currNode & 4) > 0) ? 1.0f : 0.5f),
                        float(cur.p_size.x & 0xFFFF) + (((currNode & 2) > 0) ? 1.0f : 0.5f),
                        float(cur.p_size.y >> 16) + (((currNode & 1) > 0) ? 1.0f : 0.5f));
    float3 tl = _t0 + d * pf2 * _l;

    //this child is empty, go to the next child of this node
    if (std::min(std::min(tl.x, tl.y), tl.z) < posAndNear.w || childHasData == 0)
    {
      uint32_t nextChildOfCurNode = new_node(currNode, tl.x, tl.y, tl.z);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.info = nextChildOfCurNode  | (cur.info & 0xFFFFFF00u);
    }
    else if (childIsLeaf > 0) // we intersected with leaf, fill the hit data and return
    {
      uint32_t level_sz = 2*(cur.p_size.y & 0xFFFF);
      uint3 p = 2*uint3(cur.p_size.x >> 16, cur.p_size.x & 0xFFFF, cur.p_size.y >> 16) + 
            uint3((currNode & 4) >> 2, (currNode & 2) >> 1, currNode & 1);
      d = 1.0f / float(level_sz);
      float3 p_f = float3(p);
      t0 = _t0 + d * p_f * _l;
      float3 t1 = _t0 + d * (p_f + 1.0f) * _l;

      uint32_t p_mask = level_sz - 1;
      uint3 real_p = uint3(((a & 4) > 0) ? (~p.x & p_mask) : p.x,
                           ((a & 2) > 0) ? (~p.y & p_mask) : p.y,
                           ((a & 1) > 0) ? (~p.z & p_mask) : p.z);

      float tmin = std::max(std::max(posAndNear.w, t0.x), std::max(t0.y, t0.z));
      float tmax = std::min(std::min(dirAndFar.w,  t1.x), std::min(t1.y, t1.z));
      float3 start_pos = to_float3(posAndNear) + tmin * to_float3(dirAndFar);
      float3 end_pos = to_float3(posAndNear) + tmax * to_float3(dirAndFar);
      float3 min_pos = float3(-1, -1, -1) + 2.0f * d * float3(real_p.x, real_p.y, real_p.z);
      float3 start_q = 0.5f * float(level_sz) * (start_pos - min_pos);
      float3 end_q = 0.5f * float(level_sz) * (end_pos - min_pos);
      //printf("start q %f %f %f, end q %f %f %f\n", start_q.x, start_q.y, start_q.z, end_q.x, end_q.y, end_q.z);

          SdfDAGDataEdge de = unpack_data_edge(m_header, m_header.max_val, m_SComNodes[childLink], m_SComNodes[childLink + m_header.child_offset_off]);
          uint32_t offset = m_header.bricks_arr_offset + m_header.bricks_step * de.data_offset;
          uint32_t rotIdx = m_RotAddTable[cur.transform * ROT_COUNT + de.rotation_id];
          float values[8];
          for (int i = 0; i < 8; i++)
          {
            int4 pI = int4((i & 4) >> 2, (i & 2) >> 1, i & 1, 1);
            uint32_t vId0 = uint32_t(dot(m_SdfCompactOctreeRotModifiers[rotIdx], pI));
            uint32_t p_val = m_SComValues[offset + vId0 / m_header.values_per_uint];
            uint32_t p_off = (vId0 % m_header.values_per_uint) * m_header.bits_per_value;
            values[i] = m_header.max_val * (2.0f * ((p_val >> p_off) & m_header.value_mask) / float(m_header.value_mask) - 1) + de.add;
          }

      // Sample and compositing
      float density = -1.0f * eval_dist_trilinear(values, 0.5f * (start_q + end_q));
      float3 sample_color = to_float3(m_baseColor);
      if (m_preset.volume_type == VOLUME_TYPE_SVRASTER_RF)
      {
        density = 100.0f * m_header.user_params[SCENE_EXTENT_USER_PARAM_ID] * exp_linear_11(density);
        //channel data is stored after bodes/surfaces list and only for leaves
        // the second bitcount extracts number of leaves before the current child to find proper child offset
        uint32_t channelLink = cur.linksOff + bitCount((cur.info >> 24) & 0xFF) + bitCount((cur.info >> 8) & (0x5555 >> (16-2*childN)));
        SdfDAGChildEdge ce = unpack_child_edge(m_header, m_SComNodes[channelLink], m_SComNodes[channelLink + m_header.child_offset_off]);
        float4 v = to_float4(to_float3(posAndNear)  - 0.5f*(start_pos + end_pos), 0.0f);
        uint32_t chRotIdx = m_RotAddTable[cur.transform*ROT_COUNT + ce.rotation_id];
        v = m_DAGInverseTransforms[chRotIdx] * v;

        sample_color = computeColorFromSH(float3(0,0,0), to_float3(v), ce.child_offset);
      }

      float step = length(end_pos - start_pos);
      float sampleAlpha = exp(-1.0f*density * step);
      
      float lightAtten = 1.0f;
      if (m_preset.use_lighting == 1 && density > 3.0f*m_preset.alpha_thr)
      {
        float3 pos = 0.5f * (start_pos + end_pos);
        RaytraceSCom_SS_shadow(to_float4(pos, 0.0001f), to_float4(m_LightDir, 1000.0f), &lightAtten);
      }

      accumulatedColor += alpha * (1.0f - sampleAlpha) * lightAtten * m_LightColor * sample_color;
      alpha *= sampleAlpha;

      uint32_t nextChildOfCurNode = new_node(currNode, tl.x, tl.y, tl.z);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.info = nextChildOfCurNode  | (cur.info & 0xFFFFFF00u);
    }
    else // this child is a node, move to it
    {
      uint32_t nextChildOfCurNode = new_node(currNode, tl.x, tl.y, tl.z);

      if (nextChildOfCurNode < 8)  // if it is not a last child, leave the current node and place the child in the stack
      {
        cur.info = nextChildOfCurNode | (cur.info & 0xFFFFFF00u);
        stack[++top] = cur;
      }

      d = 0.5f / float(cur.p_size.y & 0xFFFF);
      float3 p_f = float3(2*(cur.p_size.x >> 16) + ((currNode & 4) >> 2),
                          2*(cur.p_size.x & 0xFFFF) + ((currNode & 2) >> 1),
                          2*(cur.p_size.y >> 16) + (currNode & 1));
      t0 = _t0 + d * p_f * _l;
      tm = t0 + 0.5f*d*_l;

      SdfDAGChildEdge ce = unpack_child_edge(m_header, m_SComNodes[childLink], m_SComNodes[childLink + m_header.child_offset_off]);
      uint32_t node = m_SComNodes[ce.child_offset];
      uint32_t nextNode = first_node(t0, tm);

      cur.linksOff = ce.child_offset + m_header.links_offset + ((node & 0xFF) == SCOM2_CHILD_EMPTY ? 0 : 1+m_header.has_channels);
      cur.transform = m_RotAddTable[cur.transform * ROT_COUNT + ce.rotation_id];
      cur.info = nextNode | (node & 0xFFFFFF00u);
      cur.p_size = (cur.p_size << 1) | uint2(((currNode & 4) << (16 - 2)) | ((currNode & 2) >> 1), (currNode & 1) << 16);
    }
  }

  return to_float4(alpha * to_float3(m_backgroundColor) + accumulatedColor, 1);
#else
  return float4(1,0,0,1);
#endif
}

struct OTStackElement4D
{
  uint4 p;
  uint32_t nodeId;
  uint32_t info;
  uint32_t size;
  uint32_t transform;
};

float4 VolumeRenderer::RaymarchDAG4D(float4 posAndNear, float4 dirAndFar)
{
#ifndef DISABLE_SDF_DAG
  //It is a naive version that iterates all child nodes, instead of traversing it in octree-like fashion
  //but DAG is a mostly for debugging similarity compression, so it does not really matter
  //Still, we should optimize it if performance testing of octree/64-tree is considered

  const float3 ray_pos = to_float3(posAndNear);
  const float3 ray_dir = to_float3(dirAndFar);
  const float tNear = posAndNear.w;
  const float tFar = dirAndFar.w;
  const float time = (float(frame0) + m_dframe) / float(m_timestamps.size() - 1);
  const float3 ray_dir_inv = SafeInverse(ray_dir);
  OTStackElement4D stack[32];

  int top = 0;
  int buf_top = 0;
  uint3 p;
  float4 p_f;
  float3 t0, t1, tm;
  int currNode;
  uint32_t nodeId;
  uint32_t level_sz;
  float d, level_sz_f, sz_inv;
  float2 fNearFar;
  float values0[8];
  float values1[8];
  SdfDAGHeader header = m_SdfDAGHeaders[0];//m_SdfDAGHeaders[m_geomData[geomId].offset.y];

  stack[top].nodeId = 0;//m_geomData[geomId].offset.x;
  stack[top].info = 0; 
  stack[top].transform = 0; //cumulative rotation id of the node
  stack[top].p = uint4(0,0,0,0);
  stack[top].size = 1;

  float alpha = 1.0f;
  float3 accumulatedColor = float3(0.0f);

  while (top >= 0 && alpha > m_preset.alpha_thr)
  {
    OTStackElement4D curElem = stack[top];
    top--;

    SdfDAGNode node = m_SdfDAGNodes[curElem.nodeId];
    level_sz = curElem.size;
    level_sz_f = float(level_sz);
    p = to_uint3(curElem.p);
    
    sz_inv = 2.0f/level_sz_f;
    float3 brick_min_pos = float3(-1,-1,-1) + sz_inv*float3(p);
    float3 brick_max_pos = brick_min_pos + sz_inv*float3(1,1,1);
    float3 brick_size = brick_max_pos - brick_min_pos;

    float time_min = float(curElem.p.w) / level_sz_f;
    float time_max = time_min + 1.0f/level_sz_f;
    float time_size = time_max - time_min;
    // printf("[%d %d %d %d][%d] node %d rot %d\n", p.x, p.y, p.z, curElem.p.w, level_sz, curElem.nodeId, curElem.transform);
    // printf("%d %f %f %f %f\n", voxel_w0, voxel_w0_f, time, time_min, time_max);

    float2 brick_fNearFar = RayBoxIntersection2(ray_pos, ray_dir_inv, brick_min_pos, brick_max_pos);
    brick_fNearFar.x = std::max(tNear, brick_fNearFar.x);

    if (brick_fNearFar.x >= brick_fNearFar.y)
    {
      continue;
    }

    bool is_leaf = node.children_edges_offset == 0;
    // if (!is_leaf)
    // {
    //   float t = m_preset.fixed_lod > 0 ? 1.0f : brick_fNearFar.x;
    //   float target_lod_size = (std::pow(2.0f, m_preset.level_of_detail) - 0.01f) / t;
    //   is_leaf = float(level_sz) > target_lod_size;
    // }

    if (is_leaf)
    {
      //intersect with grid inside leaf
      if (DAG_node_is_full(node.voxel_count_flags) && node.data_edges_offset > 0)
      {
        float voxel_w0_f =  float(header.brick_size)*(time - time_min)/time_size;                                            
        uint32_t voxel_w0 = uint32_t(voxel_w0_f);
        float voxel_dw = voxel_w0_f - float(voxel_w0);

        //intersect leaf
        float sz = level_sz_f;
        d = 2.0f/(sz*header.brick_size);
        bool is_surface = DAG_extract_is_surface(node.voxel_count_flags);
        uint32_t surface_count = DAG_extract_count(node.voxel_count_flags);
        uint32_t primId = node.channels_edge.child_offset; // primId is used to access channel data in Resolve pass
                                                           // so we must put channels offset here to apply sim. comp. to channels

        while (brick_fNearFar.x < brick_fNearFar.y && alpha > m_preset.alpha_thr)
        {
          uint32_t v_size = header.brick_size + 2*header.brick_pad + 1;
          float3 hit_pos = ray_pos + brick_fNearFar.x*ray_dir;
          float3 local_pos = (hit_pos - brick_min_pos) * (0.5f*sz*header.brick_size);
          float3 voxelPos = floor(clamp(local_pos, 1e-6f, header.brick_size-1e-6f));

          float3 min_pos = brick_min_pos + d*voxelPos;
          float3 max_pos = min_pos + d*float3(1,1,1);
          float3 size = max_pos - min_pos;

          uint32_t s_id = 0;
          while (s_id < surface_count && alpha > m_preset.alpha_thr)
          {
            uint32_t offset = m_SdfDAGDataEdges[node.data_edges_offset + s_id].data_offset;
            int4 voxelPosI = int4(int(voxelPos.x), int(voxelPos.y), int(voxelPos.z), voxel_w0);
            uint32_t rotIdx = m_RotAddTable[curElem.transform*m_SdfDAGTransformCount + 
                                            m_SdfDAGDataEdges[node.data_edges_offset + s_id].rotation_id];
            float add = m_SdfDAGDataEdges[node.data_edges_offset + s_id].add;
            for (int i=0;i<16;i++)
            {
              int4 pI = voxelPosI + int4((i & 4) >> 2, (i & 2) >> 1, i & 1, (i & 8) >> 3);
              int idx = pI.w*int(v_size*v_size*v_size) + pI.x*int(v_size*v_size) + pI.y*int(v_size) + pI.z;
              uint32_t vId0 = m_SdfDAGTranspositions[rotIdx*v_size*v_size*v_size*v_size + idx];
              float val = m_SdfDAGDistances[offset + vId0] + add;
              if (i < 8)
                values0[i] = val;
              else 
                values1[i-8] = val;
              //printf("vId0 = %d, offset = %d add = %f, val = %f\n", vId0, offset, add, values[i]);
            }

            fNearFar = RayBoxIntersection2(ray_pos, SafeInverse(ray_dir), min_pos, max_pos);
            if (fNearFar.x < fNearFar.y)    
            {
              float3 start_pos = ray_pos + fNearFar.x*ray_dir;
              float3 end_pos   = ray_pos + fNearFar.y*ray_dir;
              float vox_sz = (0.5f*sz*header.brick_size);
              float3 start_q = vox_sz * (start_pos - min_pos);
              float3 end_q = vox_sz * (end_pos - min_pos);
          
              // Sample and compositing
              float3 sample_color = float3(1,1,1);
              float s0 = eval_dist_trilinear(values0, 0.5f*(start_q + end_q));
              float s1 = eval_dist_trilinear(values1, 0.5f*(start_q + end_q));
              float density = -1.0f*((1-voxel_dw)*s0 + voxel_dw*s1);
              float localAlpha = 1.0f - std::exp(-density * length(end_pos - start_pos));
              accumulatedColor += alpha * localAlpha * sample_color;
              alpha *= 1.0f - localAlpha;
            }
            s_id++;
          }
          brick_fNearFar.x += std::max(0.0f, fNearFar.y-brick_fNearFar.x) + 1e-5f;
        }
      }
    }
    else if (brick_fNearFar.x < brick_fNearFar.y)
    {
      //traverse children and add to stack (in reverse order)
      int v_sz = header.node_grid_size;
      float t_neg = (brick_fNearFar.y + tNear + 1);
      float3 neg_pos = ray_pos + t_neg * ray_dir;
      float3 neg_dir = -ray_dir;
      float3 neg_dir_inv = -ray_dir_inv;
      float2 neg_brick_fNearFar = float2(t_neg - brick_fNearFar.y, t_neg - brick_fNearFar.x);

      float voxel_w0_f =  float(header.node_grid_size)*(time - time_min)/time_size;                                            
      uint32_t voxel_w0 = uint32_t(voxel_w0_f);

      while (neg_brick_fNearFar.x < neg_brick_fNearFar.y)
      {
        d = 2.0f/(level_sz_f*v_sz);
        float3 hit_pos = neg_pos + neg_brick_fNearFar.x*neg_dir;
        float3 local_pos = (hit_pos - brick_min_pos) * (0.5f*level_sz_f*v_sz);
        float3 voxelPos = floor(clamp(local_pos, 1e-6f, v_sz-1e-6f));
        int4 voxelPosTime = int4((int)voxelPos.x, (int)voxelPos.y, (int)voxelPos.z, voxel_w0);
        int idx = v_sz*v_sz*v_sz*voxelPosTime.w + v_sz*v_sz*voxelPosTime.x + v_sz*voxelPosTime.y + voxelPosTime.z;
        uint32_t voxId = m_SdfDAGTranspositions[m_SdfDAGBrickTranspositionOffset + curElem.transform*v_sz*v_sz*v_sz*v_sz + idx];

        float3 min_pos = brick_min_pos + d*voxelPos;
        float3 max_pos = min_pos + d*float3(1,1,1);
        float3 size = max_pos - min_pos;

        fNearFar = RayBoxIntersection2(neg_pos, neg_dir_inv, min_pos, max_pos);
        SdfDAGChildEdge edge = m_SdfDAGChildEdges[node.children_edges_offset + voxId];

        //if we itersected with this node bbox and this node is valid
        if (tNear < fNearFar.x && edge.child_offset > 0)    
        {
          SdfDAGNode child_node = m_SdfDAGNodes[edge.child_offset];
          top++;
          stack[top].nodeId = edge.child_offset;
          stack[top].transform = m_RotAddTable[curElem.transform*m_SdfDAGTransformCount + edge.rotation_id];
          stack[top].p = v_sz*curElem.p + uint4(voxelPosTime);
          stack[top].size = 2*curElem.size;
        }

        neg_brick_fNearFar.x += std::max(0.0f, fNearFar.y-neg_brick_fNearFar.x) + 0.01f*d;
      }
    }
  }
  return alpha * m_backgroundColor + to_float4(accumulatedColor, 1);
#endif
}

struct ExtStackElement4D
{
  uint4 p;
  uint32_t size;
  uint32_t linksOff;
  uint32_t currNode;
  uint32_t transform;

  uint32_t chActive;
  uint32_t chTypes;
  uint32_t _pad0;
  uint32_t _pad1;
};


float4 VolumeRenderer::RaymarchSCom4D(float4 posAndNear, float4 dirAndFar)
{
  float3 m_LightDir = normalize(float3(1,1,1));
  float3 m_LightColor = float3(1,1,1);

#ifndef DISABLE_SCOM2
  float2 boxNearFar = RayBoxIntersection2(to_float3(posAndNear), SafeInverse(to_float3(dirAndFar)), float3(-1,-1,-1), float3(1,1,1));
  if (boxNearFar.x > boxNearFar.y) 
    return m_backgroundColor;

  float3 pos_ray_pos = to_float3(posAndNear);
  float3 pos_ray_dir = to_float3(dirAndFar);
  const float time = (float(frame0) + m_dframe) / float(m_timestamps.size() - 1);

  //assume octree is box [-1,1]^3
  uint32_t a = 0;
  if (dirAndFar.x < 0)
  {
    pos_ray_pos.x *= -1;
    pos_ray_dir.x *= -1;
    a |= 4;
  }

  if (dirAndFar.y < 0)
  {
    pos_ray_pos.y *= -1;
    pos_ray_dir.y *= -1;
    a |= 2;
  }

  if (dirAndFar.z < 0)
  {
    pos_ray_pos.z *= -1;
    pos_ray_dir.z *= -1;
    a |= 1;
  }

  pos_ray_pos *= -1;
  const float3 pos_ray_dir_inv = SafeInverse(pos_ray_dir);
  const float3 _t0 = pos_ray_pos * pos_ray_dir_inv - pos_ray_dir_inv;
  const float3 _t1 = pos_ray_pos * pos_ray_dir_inv + pos_ray_dir_inv;
  const float3 _l = _t1 - _t0;

  ExtStackElement4D stack[16];

  int top = 0;
  float d = 1.0f;
  float3 t0 = _t0;
  float3 tm = t0 + 0.5f*_l;

  ExtStackElement4D cur;
  {
    NodeHeadUnpacked node = unpack_node_head(m_header, m_SComNodes[m_header.root_node_off], m_SComNodes[m_header.root_node_off+m_header.links_offset-1]);
    uint32_t currNode = first_node(t0, tm);

    cur.linksOff = m_header.root_node_off + m_header.links_offset + 
                   (node.base_type == SCOM2_CHILD_EMPTY ? 0 : m_header.uints_per_link*(1+m_header.has_channels));
    cur.currNode = currNode | (time < 0.5f ? 0 : 8);
    cur.chActive = node.children_active;
    cur.chTypes  = node.children_types;
    cur.transform = 0;
    cur.p = uint4(0,0,0,0);
    cur.size = 1;
  }

  stack[0].linksOff = 0xFFFFFFFFu;
  stack[0].currNode = 0;
  stack[0].chActive = 0;
  stack[0].chTypes = 0;
  stack[0].p = uint4(0,0,0,0);
  stack[0].size = 0;

  float alpha = 1.0f;
  float3 accumulatedColor = float3(0.0f);

  while (top >= 0 && alpha > m_preset.alpha_thr)
  {
    uint32_t currNode = cur.currNode & 0xF;
    uint32_t rawChildNode = currNode ^ a; 
    //uint3 voxelPos  = uint3((rawChildNode & 4) >> 2, (rawChildNode & 2) >> 1, rawChildNode & 1);
    //uint32_t childN = uint32_t(dot(m_SdfCompactOctreeRotModifiers[2 * ROT_COUNT + cur.transform], // child node number, from 0 to 8
    //                               int4((int)voxelPos.x, (int)voxelPos.y, (int)voxelPos.z, 1)));
    uint32_t childN = m_SdfDAGTranspositions[m_SdfDAGBrickTranspositionOffset + cur.transform*16 + rawChildNode];
    uint32_t childLink = cur.linksOff + bitCount(cur.chActive & ((1u << childN) - 1))*m_header.uints_per_link;
    uint32_t childHasData = cur.chActive & (1u << (  childN));
    uint32_t childIsLeaf  = cur.chTypes  & (1u << (2*childN));

    //printf("intersect %d (%d) linksOff = %d, chActive = %x, chTypes = %x\n", top, currNode, cur.linksOff, cur.chActive, cur.chTypes);

    d = 1.0f / float(cur.size);
    float3 pf2 = float3(float(cur.p.x) + (((currNode & 4) > 0) ? 1.0f : 0.5f),
                        float(cur.p.y) + (((currNode & 2) > 0) ? 1.0f : 0.5f),
                        float(cur.p.z) + (((currNode & 1) > 0) ? 1.0f : 0.5f));
    float3 tl = _t0 + d * pf2 * _l;

    float time_min = float(cur.p.w) / float(cur.size);
    float time_max = time_min + 1.0f/float(cur.size);
    float time_size = time_max - time_min;
    float voxel_w0_f = (time - time_min) / time_size;

    //printf("top = %d curr node = %d\n",top, currNode);

    //this child is empty, go to the next child of this node
    if (std::min(std::min(tl.x, tl.y), tl.z) < posAndNear.w || childHasData == 0)
    {
      uint32_t nextChildOfCurNode = new_node(currNode & 0x7, tl.x, tl.y, tl.z);
      //printf("1 %d %d\n", currNode, nextChildOfCurNode);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.currNode = nextChildOfCurNode | (voxel_w0_f < 0.5f ? 0 : 8);
      //printf("A %d\n", cur.currNode);
    }
    else if (childIsLeaf > 0) // we intersected with leaf, fill the hit data and return
    {
      //printf("2\n");
      uint32_t level_sz = 2 * (cur.size);
      uint3 p = 2 * uint3(cur.p.x, cur.p.y, cur.p.z) + uint3((currNode & 4) >> 2, (currNode & 2) >> 1, currNode & 1);
      d = 1.0f / float(level_sz);
      float3 p_f = float3(p);
      t0 = _t0 + d * p_f * _l;
      float3 t1 = _t0 + d * (p_f + 1.0f) * _l;

      uint32_t p_mask = level_sz - 1;
      uint3 real_p = uint3(((a & 4) > 0) ? (~p.x & p_mask) : p.x,
                           ((a & 2) > 0) ? (~p.y & p_mask) : p.y,
                           ((a & 1) > 0) ? (~p.z & p_mask) : p.z);

      float tmin = std::max(std::max(posAndNear.w, t0.x), std::max(t0.y, t0.z));
      float tmax = std::min(std::min(dirAndFar.w,  t1.x), std::min(t1.y, t1.z));
      float3 start_pos = to_float3(posAndNear) + tmin * to_float3(dirAndFar);
      float3 end_pos = to_float3(posAndNear) + tmax * to_float3(dirAndFar);
      float3 min_pos = float3(-1, -1, -1) + 2.0f * d * float3(real_p.x, real_p.y, real_p.z);
      float3 start_q = 0.5f * float(level_sz) * (start_pos - min_pos);
      float3 end_q = 0.5f * float(level_sz) * (end_pos - min_pos);
      // printf("start q %f %f %f, end q %f %f %f\n", start_q.x, start_q.y, start_q.z, end_q.x, end_q.y, end_q.z);

      SdfDAGDataEdge de = unpack_data_edge(m_header, m_header.max_val, m_SComNodes[childLink], m_SComNodes[childLink + m_header.child_offset_off]);
      uint32_t offset = m_header.bricks_arr_offset + m_header.bricks_step * de.data_offset;
      uint32_t rotIdx = m_RotAddTable[cur.transform * m_SdfDAGTransformCount + de.rotation_id];
      float values0[8];
      float values1[8];
      for (int i = 0; i < 16; i++)
      {
        int4 pI = int4((i & 4) >> 2, (i & 2) >> 1, i & 1, (i & 8) >> 3);
        int idx = pI.w * 8 + pI.x * 4 + pI.y * 2 + pI.z;
        uint32_t vId0 = m_SdfDAGTranspositions[rotIdx * 16 + idx];
        uint32_t p_val = m_SComValues[offset + vId0 / m_header.values_per_uint];
        uint32_t p_off = (vId0 % m_header.values_per_uint) * m_header.bits_per_value;
        float val = m_header.max_val * (2.0f * ((p_val >> p_off) & m_header.value_mask) / float(m_header.value_mask) - 1) + de.add;
        if (i < 8)
          values0[i] = val;
        else
          values1[i - 8] = val;
        //printf("vId0 = %d, offset = %d add = %f, val = %f\n", vId0, offset, de.add, val);
      }

      float voxel_dw = 2.0f*voxel_w0_f - float(uint32_t(2.0f*voxel_w0_f));

      // Sample and compositing
      float3 sample_color = float3(m_baseColor.x, m_baseColor.y, m_baseColor.z);
      float s0 = eval_dist_trilinear(values0, 0.5f * (start_q + end_q));
      float s1 = eval_dist_trilinear(values1, 0.5f * (start_q + end_q));
      float density = -1.0f * ((1 - voxel_dw) * s0 + voxel_dw * s1);
      float step = length(end_pos - start_pos);
      float sampleAlpha = exp(-1.0f*density * step);
      
      float lightAtten = 1.0f;
      if (m_preset.use_lighting == 1 && density > 3.0f*m_preset.alpha_thr)
      {
        float3 pos = 0.5f * (start_pos + end_pos);
        RaytraceSCom4D_SS_shadow(to_float4(pos, 0.0001f), to_float4(m_LightDir, 1000.0f), &lightAtten);
      }

      accumulatedColor += alpha * (1.0f - sampleAlpha) * lightAtten * m_LightColor * sample_color;
      alpha *= sampleAlpha;

      uint32_t nextChildOfCurNode = new_node(currNode & 0x7, tl.x, tl.y, tl.z);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.currNode = nextChildOfCurNode | (voxel_w0_f < 0.5f ? 0 : 8);
    }
    else // this child is a node, move to it
    {
      //printf("3\n");
      uint32_t nextChildOfCurNode = new_node(currNode & 0x7, tl.x, tl.y, tl.z);

      if (nextChildOfCurNode < 8)  // if it is not a last child, leave the current node and place the child in the stack
      {
        cur.currNode = nextChildOfCurNode | (voxel_w0_f < 0.5f ? 0 : 8);
        stack[++top] = cur;
      }

      d = 0.5f / float(cur.size);
      float3 p_f = float3(2*(cur.p.x) + ((currNode & 4) >> 2),
                          2*(cur.p.y) + ((currNode & 2) >> 1),
                          2*(cur.p.z) + (currNode & 1));
      t0 = _t0 + d * p_f * _l;
      tm = t0 + 0.5f*d*_l;
      float voxel_dw = 2.0f*voxel_w0_f - float(uint32_t(2.0f*voxel_w0_f));

      SdfDAGChildEdge ce = unpack_child_edge(m_header, m_SComNodes[childLink], m_SComNodes[childLink + m_header.child_offset_off]);
      NodeHeadUnpacked node = unpack_node_head(m_header, m_SComNodes[ce.child_offset], m_SComNodes[ce.child_offset+m_header.links_offset-1]);
      uint32_t nextNode = first_node(t0, tm);

      cur.linksOff = ce.child_offset + m_header.links_offset + (node.base_links_end == SCOM2_CHILD_EMPTY ? 0 : m_header.uints_per_link*(1+m_header.has_channels));
      cur.transform = m_RotAddTable[cur.transform * m_SdfDAGTransformCount + ce.rotation_id];
      cur.currNode = nextNode | (voxel_dw < 0.5f ? 0 : 8);
      cur.chActive = node.children_active;
      cur.chTypes = node.children_types;
      cur.size = cur.size << 1;
      cur.p = (cur.p << 1) | uint4((currNode & 4) >> 2, (currNode & 2) >> 1, currNode & 1, (currNode & 8) >> 3);
    }
  }

  return alpha * m_backgroundColor + to_float4(accumulatedColor, 1);
#else
  return float4(1,0,0,1);
#endif
}

void VolumeRenderer::RaytraceSCom4D_SS_shadow(float4 posAndNear, float4 dirAndFar, float* out_T)
{
  *out_T = 1.0f;
#ifndef DISABLE_SCOM2
  float2 boxNearFar = RayBoxIntersection2(to_float3(posAndNear), SafeInverse(to_float3(dirAndFar)), float3(-1,-1,-1), float3(1,1,1));
  if (boxNearFar.x > boxNearFar.y) 
    return;

  float3 pos_ray_pos = to_float3(posAndNear);
  float3 pos_ray_dir = to_float3(dirAndFar);
  const float time = (float(frame0) + m_dframe) / float(m_timestamps.size() - 1);

  //assume octree is box [-1,1]^3
  uint32_t a = 0;
  if (dirAndFar.x < 0)
  {
    pos_ray_pos.x *= -1;
    pos_ray_dir.x *= -1;
    a |= 4;
  }

  if (dirAndFar.y < 0)
  {
    pos_ray_pos.y *= -1;
    pos_ray_dir.y *= -1;
    a |= 2;
  }

  if (dirAndFar.z < 0)
  {
    pos_ray_pos.z *= -1;
    pos_ray_dir.z *= -1;
    a |= 1;
  }

  pos_ray_pos *= -1;
  const float3 pos_ray_dir_inv = SafeInverse(pos_ray_dir);
  const float3 _t0 = pos_ray_pos * pos_ray_dir_inv - pos_ray_dir_inv;
  const float3 _t1 = pos_ray_pos * pos_ray_dir_inv + pos_ray_dir_inv;
  const float3 _l = _t1 - _t0;

  ExtStackElement4D stack[16];

  int top = 0;
  float d = 1.0f;
  float3 t0 = _t0;
  float3 tm = t0 + 0.5f*_l;

  ExtStackElement4D cur;
  {
    NodeHeadUnpacked node = unpack_node_head(m_header, m_SComNodes[m_header.root_node_off], m_SComNodes[m_header.root_node_off+m_header.links_offset-1]);
    uint32_t currNode = first_node(t0, tm);

    cur.linksOff = m_header.root_node_off + m_header.links_offset + 
                   (node.base_type == SCOM2_CHILD_EMPTY ? 0 : m_header.uints_per_link*(1+m_header.has_channels));
    cur.currNode = currNode | (time < 0.5f ? 0 : 8);
    cur.chActive = node.children_active;
    cur.chTypes  = node.children_types;
    cur.transform = 0;
    cur.p = uint4(0,0,0,0);
    cur.size = 1;
  }

  stack[0].linksOff = 0xFFFFFFFFu;
  stack[0].currNode = 0;
  stack[0].chActive = 0;
  stack[0].chTypes = 0;
  stack[0].p = uint4(0,0,0,0);
  stack[0].size = 0;

  float alpha = 1.0f;
  float3 accumulatedColor = float3(0.0f);

  while (top >= 0 && alpha > m_preset.alpha_thr)
  {
    uint32_t currNode = cur.currNode & 0xF;
    uint32_t rawChildNode = currNode ^ a; 
    uint32_t childN = m_SdfDAGTranspositions[m_SdfDAGBrickTranspositionOffset + cur.transform*16 + rawChildNode];
    uint32_t childLink = cur.linksOff + bitCount(cur.chActive & ((1u << childN) - 1))*m_header.uints_per_link;
    uint32_t childHasData = cur.chActive & (1u << (  childN));
    uint32_t childIsLeaf  = cur.chTypes  & (1u << (2*childN));

    d = 1.0f / float(cur.size);
    float3 pf2 = float3(float(cur.p.x) + (((currNode & 4) > 0) ? 1.0f : 0.5f),
                        float(cur.p.y) + (((currNode & 2) > 0) ? 1.0f : 0.5f),
                        float(cur.p.z) + (((currNode & 1) > 0) ? 1.0f : 0.5f));
    float3 tl = _t0 + d * pf2 * _l;

    float time_min = float(cur.p.w) / float(cur.size);
    float time_max = time_min + 1.0f/float(cur.size);
    float time_size = time_max - time_min;
    float voxel_w0_f = (time - time_min) / time_size;

    //this child is empty, go to the next child of this node
    if (std::min(std::min(tl.x, tl.y), tl.z) < posAndNear.w || childHasData == 0)
    {
      uint32_t nextChildOfCurNode = new_node(currNode & 0x7, tl.x, tl.y, tl.z);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.currNode = nextChildOfCurNode | (voxel_w0_f < 0.5f ? 0 : 8);
    }
    else if (childIsLeaf > 0) // we intersected with leaf, fill the hit data and return
    {
      uint32_t level_sz = 2 * (cur.size);
      uint3 p = 2 * uint3(cur.p.x, cur.p.y, cur.p.z) + uint3((currNode & 4) >> 2, (currNode & 2) >> 1, currNode & 1);
      d = 1.0f / float(level_sz);
      float3 p_f = float3(p);
      t0 = _t0 + d * p_f * _l;
      float3 t1 = _t0 + d * (p_f + 1.0f) * _l;

      uint32_t p_mask = level_sz - 1;
      uint3 real_p = uint3(((a & 4) > 0) ? (~p.x & p_mask) : p.x,
                           ((a & 2) > 0) ? (~p.y & p_mask) : p.y,
                           ((a & 1) > 0) ? (~p.z & p_mask) : p.z);

      float tmin = std::max(std::max(posAndNear.w, t0.x), std::max(t0.y, t0.z));
      float tmax = std::min(std::min(dirAndFar.w,  t1.x), std::min(t1.y, t1.z));
      float3 start_pos = to_float3(posAndNear) + tmin * to_float3(dirAndFar);
      float3 end_pos = to_float3(posAndNear) + tmax * to_float3(dirAndFar);
      float3 min_pos = float3(-1, -1, -1) + 2.0f * d * float3(real_p.x, real_p.y, real_p.z);
      float3 start_q = 0.5f * float(level_sz) * (start_pos - min_pos);
      float3 end_q = 0.5f * float(level_sz) * (end_pos - min_pos);
      // printf("start q %f %f %f, end q %f %f %f\n", start_q.x, start_q.y, start_q.z, end_q.x, end_q.y, end_q.z);

      SdfDAGDataEdge de = unpack_data_edge(m_header, m_header.max_val, m_SComNodes[childLink], m_SComNodes[childLink + m_header.child_offset_off]);
      uint32_t offset = m_header.bricks_arr_offset + m_header.bricks_step * de.data_offset;
      uint32_t rotIdx = m_RotAddTable[cur.transform * m_SdfDAGTransformCount + de.rotation_id];
      float values0[8];
      float values1[8];
      for (int i = 0; i < 16; i++)
      {
        int4 pI = int4((i & 4) >> 2, (i & 2) >> 1, i & 1, (i & 8) >> 3);
        int idx = pI.w * 8 + pI.x * 4 + pI.y * 2 + pI.z;
        uint32_t vId0 = m_SdfDAGTranspositions[rotIdx * 16 + idx];
        uint32_t p_val = m_SComValues[offset + vId0 / m_header.values_per_uint];
        uint32_t p_off = (vId0 % m_header.values_per_uint) * m_header.bits_per_value;
        float val = m_header.max_val * (2.0f * ((p_val >> p_off) & m_header.value_mask) / float(m_header.value_mask) - 1) + de.add;
        if (i < 8)
          values0[i] = val;
        else
          values1[i - 8] = val;
      }

      float voxel_dw = 2.0f*voxel_w0_f - float(uint32_t(2.0f*voxel_w0_f));

      // Sample and compositing
      float3 sample_color = float3(m_baseColor.x, m_baseColor.y, m_baseColor.z);
      float s0 = eval_dist_trilinear(values0, 0.5f * (start_q + end_q));
      float s1 = eval_dist_trilinear(values1, 0.5f * (start_q + end_q));
      float density = -1.0f * ((1 - voxel_dw) * s0 + voxel_dw * s1);
      float step = length(end_pos - start_pos);
      float sampleAlpha = exp(-1.0f*density * step);

      accumulatedColor += alpha * (1.0f - sampleAlpha) * sample_color;
      alpha *= sampleAlpha;

      uint32_t nextChildOfCurNode = new_node(currNode & 0x7, tl.x, tl.y, tl.z);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.currNode = nextChildOfCurNode | (voxel_w0_f < 0.5f ? 0 : 8);
    }
    else // this child is a node, move to it
    {
      //printf("3\n");
      uint32_t nextChildOfCurNode = new_node(currNode & 0x7, tl.x, tl.y, tl.z);

      if (nextChildOfCurNode < 8)  // if it is not a last child, leave the current node and place the child in the stack
      {
        cur.currNode = nextChildOfCurNode | (voxel_w0_f < 0.5f ? 0 : 8);
        stack[++top] = cur;
      }

      d = 0.5f / float(cur.size);
      float3 p_f = float3(2*(cur.p.x) + ((currNode & 4) >> 2),
                          2*(cur.p.y) + ((currNode & 2) >> 1),
                          2*(cur.p.z) + (currNode & 1));
      t0 = _t0 + d * p_f * _l;
      tm = t0 + 0.5f*d*_l;
      float voxel_dw = 2.0f*voxel_w0_f - float(uint32_t(2.0f*voxel_w0_f));

      SdfDAGChildEdge ce = unpack_child_edge(m_header, m_SComNodes[childLink], m_SComNodes[childLink + m_header.child_offset_off]);
      NodeHeadUnpacked node = unpack_node_head(m_header, m_SComNodes[ce.child_offset], m_SComNodes[ce.child_offset+m_header.links_offset-1]);
      uint32_t nextNode = first_node(t0, tm);

      cur.linksOff = ce.child_offset + m_header.links_offset + (node.base_links_end == SCOM2_CHILD_EMPTY ? 0 : m_header.uints_per_link*(1+m_header.has_channels));
      cur.transform = m_RotAddTable[cur.transform * m_SdfDAGTransformCount + ce.rotation_id];
      cur.currNode = nextNode | (voxel_dw < 0.5f ? 0 : 8);
      cur.chActive = node.children_active;
      cur.chTypes = node.children_types;
      cur.size = cur.size << 1;
      cur.p = (cur.p << 1) | uint4((currNode & 4) >> 2, (currNode & 2) >> 1, currNode & 1, (currNode & 8) >> 3);
    }
  }

  *out_T = alpha;
#endif  
}

void VolumeRenderer::RaytraceSCom_SS_shadow(float4 posAndNear, float4 dirAndFar, float* out_T)
{
  //printf("\n");
  *out_T = 1.0f;
#ifndef DISABLE_SCOM2
  float2 boxNearFar = RayBoxIntersection2(to_float3(posAndNear), SafeInverse(to_float3(dirAndFar)), float3(-1,-1,-1), float3(1,1,1));
  if (boxNearFar.x > boxNearFar.y) 
    return;

  float3 pos_ray_pos = to_float3(posAndNear);
  float3 pos_ray_dir = to_float3(dirAndFar);

  //assume octree is box [-1,1]^3
  uint32_t a = 0;
  if (dirAndFar.x < 0)
  {
    pos_ray_pos.x *= -1;
    pos_ray_dir.x *= -1;
    a |= 4;
  }

  if (dirAndFar.y < 0)
  {
    pos_ray_pos.y *= -1;
    pos_ray_dir.y *= -1;
    a |= 2;
  }

  if (dirAndFar.z < 0)
  {
    pos_ray_pos.z *= -1;
    pos_ray_dir.z *= -1;
    a |= 1;
  }

  pos_ray_pos *= -1;
  const float3 pos_ray_dir_inv = SafeInverse(pos_ray_dir);
  const float3 _t0 = pos_ray_pos * pos_ray_dir_inv - pos_ray_dir_inv;
  const float3 _t1 = pos_ray_pos * pos_ray_dir_inv + pos_ray_dir_inv;
  const float3 _l = _t1 - _t0;

  ExtStackElement stack[16];

  int top = 0;
  float d = 1.0f;
  float3 t0 = _t0;
  float3 tm = t0 + 0.5f*_l;

  ExtStackElement cur;
  {
    uint32_t node = m_SComNodes[m_header.root_node_off];
    uint32_t currNode = first_node(t0, tm);

    cur.linksOff = m_header.root_node_off + m_header.links_offset + ((node & 0xFF) == SCOM2_CHILD_EMPTY ? 0 : 1+m_header.has_channels);
    cur.info = currNode | (node & 0xFFFFFF00u);
    cur.transform = 0;
    cur.p_size = uint2(0,1);
  }

  stack[0].linksOff = 0xFFFFFFFFu;
  stack[0].info = 0;
  stack[0].transform = 0;
  stack[0].p_size = uint2(0,0);

  float alpha = 1.0f;
  float3 accumulatedColor = float3(0.0f);

  while (top >= 0 && alpha > m_preset.alpha_thr)
  {
    //printf("top: %d %08X\n", top, cur.info);
    uint32_t currNode = cur.info & 0x7;
    uint32_t rawChildNode = currNode ^ a; 
    uint3 voxelPos  = uint3((rawChildNode & 4) >> 2, (rawChildNode & 2) >> 1, rawChildNode & 1);
    uint32_t childN = uint32_t(dot(m_SdfCompactOctreeRotModifiers[2 * ROT_COUNT + cur.transform], // child node number, from 0 to 8
                                   int4((int)voxelPos.x, (int)voxelPos.y, (int)voxelPos.z, 1)));
    uint32_t childLink = cur.linksOff + bitCount((cur.info >> 24) & ((1u << childN) - 1));
    uint32_t childHasData = cur.info & (1u << (24+childN));
    uint32_t childIsLeaf  = cur.info & (1u << (8+2*childN));

    d = 1.0f / float(cur.p_size.y & 0xFFFF);
    float3 pf2 = float3(float(cur.p_size.x >> 16) + (((currNode & 4) > 0) ? 1.0f : 0.5f),
                        float(cur.p_size.x & 0xFFFF) + (((currNode & 2) > 0) ? 1.0f : 0.5f),
                        float(cur.p_size.y >> 16) + (((currNode & 1) > 0) ? 1.0f : 0.5f));
    float3 tl = _t0 + d * pf2 * _l;

    //this child is empty, go to the next child of this node
    if (std::min(std::min(tl.x, tl.y), tl.z) < posAndNear.w || childHasData == 0)
    {
      uint32_t nextChildOfCurNode = new_node(currNode, tl.x, tl.y, tl.z);
      //printf("nextChildOfCurNode = %d %f %f %f -- %f\n", nextChildOfCurNode, tl.x, tl.y, tl.z, posAndNear.w);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.info = nextChildOfCurNode  | (cur.info & 0xFFFFFF00u);
    }
    else if (childIsLeaf > 0) // we intersected with leaf, fill the hit data and return
    {
      uint32_t level_sz = 2*(cur.p_size.y & 0xFFFF);
      uint3 p = 2*uint3(cur.p_size.x >> 16, cur.p_size.x & 0xFFFF, cur.p_size.y >> 16) + 
            uint3((currNode & 4) >> 2, (currNode & 2) >> 1, currNode & 1);
      d = 1.0f / float(level_sz);
      float3 p_f = float3(p);
      t0 = _t0 + d * p_f * _l;
      float3 t1 = _t0 + d * (p_f + 1.0f) * _l;

      uint32_t p_mask = level_sz - 1;
      uint3 real_p = uint3(((a & 4) > 0) ? (~p.x & p_mask) : p.x,
                           ((a & 2) > 0) ? (~p.y & p_mask) : p.y,
                           ((a & 1) > 0) ? (~p.z & p_mask) : p.z);

      float tmin = std::max(std::max(posAndNear.w, t0.x), std::max(t0.y, t0.z));
      float tmax = std::min(std::min(dirAndFar.w,  t1.x), std::min(t1.y, t1.z));
      float3 start_pos = to_float3(posAndNear) + tmin * to_float3(dirAndFar);
      float3 end_pos = to_float3(posAndNear) + tmax * to_float3(dirAndFar);
      float3 min_pos = float3(-1, -1, -1) + 2.0f * d * float3(real_p.x, real_p.y, real_p.z);
      float3 start_q = 0.5f * float(level_sz) * (start_pos - min_pos);
      float3 end_q = 0.5f * float(level_sz) * (end_pos - min_pos);

          SdfDAGDataEdge de = unpack_data_edge(m_header, m_header.max_val, m_SComNodes[childLink], m_SComNodes[childLink + m_header.child_offset_off]);
          uint32_t offset = m_header.bricks_arr_offset + m_header.bricks_step * de.data_offset;
          uint32_t rotIdx = m_RotAddTable[cur.transform * ROT_COUNT + de.rotation_id];
          float values[8];
          for (int i = 0; i < 8; i++)
          {
            int4 pI = int4((i & 4) >> 2, (i & 2) >> 1, i & 1, 1);
            uint32_t vId0 = uint32_t(dot(m_SdfCompactOctreeRotModifiers[rotIdx], pI));
            uint32_t p_val = m_SComValues[offset + vId0 / m_header.values_per_uint];
            uint32_t p_off = (vId0 % m_header.values_per_uint) * m_header.bits_per_value;
            values[i] = m_header.max_val * (2.0f * ((p_val >> p_off) & m_header.value_mask) / float(m_header.value_mask) - 1) + de.add;
          }

      // Sample and compositing
      float density = -1.0f * eval_dist_trilinear(values, 0.5f * (start_q + end_q));
      float3 sample_color = to_float3(m_baseColor);

      float localAlpha = 1.0f - std::exp(-density * length(end_pos - start_pos));
      accumulatedColor += alpha * localAlpha * sample_color;
      alpha *= 1.0f - localAlpha;

      uint32_t nextChildOfCurNode = new_node(currNode, tl.x, tl.y, tl.z);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.info = nextChildOfCurNode  | (cur.info & 0xFFFFFF00u);
    }
    else // this child is a node, move to it
    {
      uint32_t nextChildOfCurNode = new_node(currNode, tl.x, tl.y, tl.z);

      if (nextChildOfCurNode < 8)  // if it is not a last child, leave the current node and place the child in the stack
      {
        cur.info = nextChildOfCurNode | (cur.info & 0xFFFFFF00u);
        stack[++top] = cur;
      }

      d = 0.5f / float(cur.p_size.y & 0xFFFF);
      float3 p_f = float3(2*(cur.p_size.x >> 16) + ((currNode & 4) >> 2),
                          2*(cur.p_size.x & 0xFFFF) + ((currNode & 2) >> 1),
                          2*(cur.p_size.y >> 16) + (currNode & 1));
      t0 = _t0 + d * p_f * _l;
      tm = t0 + 0.5f*d*_l;

      SdfDAGChildEdge ce = unpack_child_edge(m_header, m_SComNodes[childLink], m_SComNodes[childLink + m_header.child_offset_off]);
      uint32_t node = m_SComNodes[ce.child_offset];
      uint32_t nextNode = first_node(t0, tm);

      cur.linksOff = ce.child_offset + m_header.links_offset + ((node & 0xFF) == SCOM2_CHILD_EMPTY ? 0 : 1+m_header.has_channels);
      cur.transform = m_RotAddTable[cur.transform * ROT_COUNT + ce.rotation_id];
      cur.info = nextNode | (node & 0xFFFFFF00u);
      cur.p_size = (cur.p_size << 1) | uint2(((currNode & 4) << (16 - 2)) | ((currNode & 2) >> 1), (currNode & 1) << 16);
    }
  }

  *out_T = alpha;
#endif
}

bool VolumeRenderer::RayQuery_AnyHit(float4 posAndNear, float4 dirAndFar)
{
  return RaymarchGrid_DDA(posAndNear, dirAndFar).w > 0;
}

CRT_Hit VolumeRenderer::RayQuery_NearestHit(float4 posAndNear, float4 dirAndFar)
{
  float4 color_alpha = float4(0,0,0,0);
  if (m_grid_type == TYPE_SDF_DAG)
  {
    if (m_SdfDAGHeaders[0].dim == 3)
      color_alpha = RaymarchDAG(posAndNear, dirAndFar);
    else if (m_SdfDAGHeaders[0].dim == 4)
      color_alpha = RaymarchDAG4D(posAndNear, dirAndFar);
  }
  else if (m_grid_type == TYPE_SCOM2)
  {
    if (m_header.dimension == 3)
      color_alpha = RaymarchSCom(posAndNear, dirAndFar);
    else if (m_header.dimension == 4)
      color_alpha = RaymarchSCom4D(posAndNear, dirAndFar);
  }
  else
  {
    RaymarchGrid_DDA(posAndNear, dirAndFar);
  }

  bool has_hit = (color_alpha.w > 0);
  CRT_Hit hit;
  hit.geomId = has_hit ? 0 : INVALID_IDX;
  hit.instId = has_hit ? 0 : INVALID_IDX;
  hit.primId = has_hit ? 0 : INVALID_IDX;
  hit.t = has_hit ? 1 - color_alpha.w : dirAndFar.w;
  hit.coords[0] = color_alpha.x;
  hit.coords[1] = color_alpha.y;
  hit.coords[2] = color_alpha.z;
  hit.coords[3] = color_alpha.w;
  return hit;
}