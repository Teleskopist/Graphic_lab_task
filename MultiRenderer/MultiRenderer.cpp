#include <cfloat>
#include <cstring>
#include <sstream>
#include <chrono>  

#include "MultiRenderer.h"
#include "utils/raytracing/render_common.h"

void MultiRenderer::Clear(uint32_t a_width, uint32_t a_height)
{
  kernel2D_PackXY(a_width, a_height);
}

void MultiRenderer::Render(uint32_t* a_outColor, uint32_t a_width, uint32_t a_height, int a_passNum)
{
  assert(m_width == a_width);
  assert(m_height == a_height);

  auto before = std::chrono::high_resolution_clock::now();

  uint32_t samples = a_passNum*m_preset.spp;
  CheckForTransparency();
  for (uint32_t sample = 0; sample < samples; sample++)
  {
    switch (m_preset.render_mode)
    {
    case MULTI_RENDER_MODE_DIFFUSE:
    case MULTI_RENDER_MODE_LAMBERT:
    case MULTI_RENDER_MODE_PHONG:
    case MULTI_RENDER_MODE_PBR:
      if (m_sceneHasTransparency)
      {
        kernel1D_FillGbufferWithTransparency(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
        kernel1D_ResolveCommonWithTransparency(m_width * m_height, sample, m_preset.render_mode, m_colorBuffer.data());
      }
      else
      {
        kernel1D_FillGbuffer(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
        kernel1D_ResolveCommon(m_width * m_height, sample, m_preset.render_mode, m_colorBuffer.data());
      }
      break;
    case MULTI_RENDER_MODE_LAMBERT_NO_TEX:
    case MULTI_RENDER_MODE_PHONG_NO_TEX:
    case MULTI_RENDER_MODE_PBR_NO_TEX:
    case MULTI_RENDER_MODE_GEOM:
    case MULTI_RENDER_MODE_LOD:
      kernel1D_FillGbuffer(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
      kernel1D_ResolveCommon(m_width*m_height, sample, m_preset.render_mode, m_colorBuffer.data());
      break;
    case MULTI_RENDER_MODE_CHANNEL:
    case MULTI_RENDER_MODE_CHANNEL_LAMBERT:
      kernel1D_FillGbuffer(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
      kernel1D_ResolveCTF(m_width*m_height, sample, m_preset.render_mode, m_colorBuffer.data());
      break;
    case MULTI_RENDER_MODE_OUTLINE_BORDER:
    case MULTI_RENDER_MODE_OUTLINE_PRIM:
      kernel1D_FillGbuffer(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
      kernel1D_ResolveOutline(m_width*m_height, sample, m_preset.render_mode, m_colorBuffer.data());
      break;
    default:
      kernel1D_FillGbuffer(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
      kernel1D_ResolveDebug(m_width*m_height, sample, m_preset.render_mode, m_colorBuffer.data());
      break;
    }
  }
  kernel1D_Tonemap(m_width*m_height, samples, a_outColor);

  auto after = std::chrono::high_resolution_clock::now();
  timeDataByName["Render"] = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count()/1000.0f;
}

void MultiRenderer::RenderFloat(float4* a_outColor, uint32_t a_width, uint32_t a_height, int a_passNum)
{
  assert(m_width == a_width);
  assert(m_height == a_height);

  auto before = std::chrono::high_resolution_clock::now();

  uint32_t samples = a_passNum*m_preset.spp;
  CheckForTransparency();
  for (uint32_t sample = 0; sample < samples; sample++)
  {
    switch (m_preset.render_mode)
    {
    case MULTI_RENDER_MODE_DIFFUSE:
    case MULTI_RENDER_MODE_LAMBERT:
    case MULTI_RENDER_MODE_PHONG:
    case MULTI_RENDER_MODE_PBR:
      if (m_sceneHasTransparency)
      {
        kernel1D_FillGbufferWithTransparency(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
        kernel1D_ResolveCommonWithTransparency(m_width * m_height, sample, m_preset.render_mode, m_colorBuffer.data());
      }
      else
      {
        kernel1D_FillGbuffer(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
        kernel1D_ResolveCommon(m_width * m_height, sample, m_preset.render_mode, m_colorBuffer.data());
      }
      break;
    case MULTI_RENDER_MODE_LAMBERT_NO_TEX:
    case MULTI_RENDER_MODE_PHONG_NO_TEX:
    case MULTI_RENDER_MODE_PBR_NO_TEX:
    case MULTI_RENDER_MODE_GEOM:
    case MULTI_RENDER_MODE_LOD:
      kernel1D_FillGbuffer(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
      kernel1D_ResolveCommon(m_width*m_height, sample, m_preset.render_mode, m_colorBuffer.data());
      break;
    case MULTI_RENDER_MODE_CHANNEL:
    case MULTI_RENDER_MODE_CHANNEL_LAMBERT:
      kernel1D_FillGbuffer(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
      kernel1D_ResolveCTF(m_width*m_height, sample, m_preset.render_mode, m_colorBuffer.data());
      break;
    case MULTI_RENDER_MODE_OUTLINE_BORDER:
    case MULTI_RENDER_MODE_OUTLINE_PRIM:
      kernel1D_FillGbuffer(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
      kernel1D_ResolveOutline(m_width*m_height, sample, m_preset.render_mode, m_colorBuffer.data());
      break;
    default:
      kernel1D_FillGbuffer(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
      kernel1D_ResolveDebug(m_width*m_height, sample, m_preset.render_mode, m_colorBuffer.data());
      break;
    }
  }
  kernel1D_AverageColor(m_width*m_height, samples, a_outColor);

  auto after = std::chrono::high_resolution_clock::now();
  timeDataByName["RenderFloat"] = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count()/1000.0f;
}

void MultiRenderer::kernel2D_PackXY(uint32_t w, uint32_t h)
{
  #pragma omp parallel for
  for(uint32_t y=0;y<h;y++)
  {
    for(uint32_t x=0;x<w;x++)
    {
      PackXY(x, y);
    }
  }
}

void MultiRenderer::initEyeRay(uint32_t x, uint32_t y, uint32_t a_sample_id, float4* rayPosAndNear, float4* rayDirAndFar)
{
  float2 HamiltonSequence[16] = {
    float2(0.500000, 0.333333),
    float2(0.250000, 0.666667),
    float2(0.750000, 0.111111),
    float2(0.125000, 0.444444),
    float2(0.625000, 0.777778),
    float2(0.375000, 0.222222),
    float2(0.875000, 0.555556),
    float2(0.062500, 0.888889),
    float2(0.562500, 0.037037),
    float2(0.312500, 0.370370),
    float2(0.812500, 0.703704),
    float2(0.187500, 0.148148),
    float2(0.687500, 0.481481),
    float2(0.437500, 0.814815),
    float2(0.937500, 0.259259),
    float2(0.031250, 0.592593)
  };

  uint32_t spp_sqrt = uint32_t(sqrt(m_preset.spp));
  float i_spp_sqrt = 1.0f/spp_sqrt;
  float2 d = (m_AAFrameNum >= 2 || m_preset.ray_gen_mode == RAY_GEN_MODE_RANDOM) ? 
              /*rand2(m_seed, x, y, a_sample_id + m_AAFrameNum + m_seed % 2)*/HamiltonSequence[m_AAFrameNum % 16] : i_spp_sqrt*float2(a_sample_id/spp_sqrt+0.5, a_sample_id%spp_sqrt+0.5);


  float3 rayDir = EyeRayDirNormalized((float(x)+d.x)/float(m_packedXY_width), (float(y)+d.y)/float(m_packedXY_height), m_projInv);
  float3 rayPos = float3(0,0,0);

  transform_ray3f(m_worldViewInv, 
                  &rayPos, &rayDir);
  
  float rayNear = 1e-9f;
  float rayFar  =  1e9f;
  float sgn = sign(dot(rayPos - m_cuttingPlane.pos, m_cuttingPlane.normal));
  float denom = m_cuttingPlane.is_active > 0 ? dot(m_cuttingPlane.normal, rayDir) : 0.0f;
  rayNear = sgn*std::abs(denom) < -1e-6f ? dot(m_cuttingPlane.pos - rayPos, m_cuttingPlane.normal) / denom : rayNear;
  rayFar  = sgn*std::abs(denom) >  1e-6f ? dot(m_cuttingPlane.pos - rayPos, m_cuttingPlane.normal) / denom : rayFar;
  *rayPosAndNear = to_float4(rayPos, rayNear < 0 ? rayFar : rayNear);
  *rayDirAndFar  = to_float4(rayDir, rayFar < 0 ?    1e9f :  rayFar);
}

void MultiRenderer::kernel1D_FillGbuffer(uint32_t count, uint32_t sample_id)
{
#ifdef NDEBUG
  //#pragma omp parallel for
#endif
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    const uint XY = m_packedXY[tidX];
    uint x  = (XY & 0x0000FFFF);
    uint y  = (XY & 0xFFFF0000) >> 16;
  
    float4 rayPosAndNear, rayDirAndFar;
    initEyeRay(x, y, sample_id, &rayPosAndNear, &rayDirAndFar);
    CRT_Hit hit = m_pAccelStruct->RayQuery_NearestHit(rayPosAndNear, rayDirAndFar);
  
    m_gBuffer[y*m_width + x] = hit;
  }
}

void MultiRenderer::kernel1D_FillGbufferWithTransparency(uint32_t count, uint32_t sample_id)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    const uint XY = m_packedXY[tidX];
    uint x  = (XY & 0x0000FFFF);
    uint y  = (XY & 0xFFFF0000) >> 16;
  
    float4 rayPosAndNear, rayDirAndFar;
    initEyeRay(x, y, sample_id, &rayPosAndNear, &rayDirAndFar);
  
    CRT_Hit hit;
    m_transparencyBuffer[y*m_width + x] = AnyHit(&hit, rayPosAndNear, rayDirAndFar);
  
    m_gBuffer[y*m_width + x] = hit;
  }
}

float4 MultiRenderer::AnyHit(CRT_Hit* hit, float4 rayPosAndNear, float4 rayDirAndFar)
{
  float3 transparencyColor = float3(0,0,0);
  float alpha = 1.0f;
  float4 color = float4(0,0,0,0);
  do {
    transparencyColor += alpha * color.w * to_float3(color);
    alpha *= (1.0f - color.w);
    *hit = m_pAccelStruct->RayQuery_NearestHit(rayPosAndNear, rayDirAndFar);
    unsigned type = hit->geomId >> SH_TYPE;
    unsigned geomId = hit->geomId & GEOM_ID_MASK;
    if (hit->primId == 0xFFFFFFFF) // no hit
      color = m_backgroundColor;
    else if (type != TYPE_SDF_SBS_COL && type != TYPE_GRAPHICS_PRIM)
    {
      unsigned matId = m_matIdbyPrimId[m_matIdOffsets[geomId].x + hit->primId % m_matIdOffsets[geomId].y];
      color = m_materials[matId].type == MULTI_RENDER_MATERIAL_TYPE_COLORED ? 
                        m_materials[matId].base_color : float4(0,0,0,1);
    }
    float3 rayPos = to_float3(rayPosAndNear) + to_float3(rayDirAndFar) * hit->t;
    rayPosAndNear = to_float4(rayPos, rayPosAndNear.w);
  } while (alpha > 0.01f);
  return to_float4(transparencyColor, alpha);
}

void MultiRenderer::CheckForTransparency()
{
  m_sceneHasTransparency = false;
  for (const auto& mat: m_materials)
  {
    if (mat.type == MULTI_RENDER_MATERIAL_TYPE_COLORED && (mat.base_color).w < 1.0f)
    {
      m_sceneHasTransparency = true;
      break;
    }
  }
}

void MultiRenderer::PackXY(uint tidX, uint tidY)
{
  const uint offset   = SuperBlockIndex2DOpt(tidX, tidY, m_packedXY_width);
  m_packedXY[offset] = ((tidY << 16) & 0xFFFF0000) | (tidX & 0x0000FFFF);
}
