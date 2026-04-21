#include "core/ISimpleCombinedRenderer.h"
#include "utils/raytracing/render_common.h"

void ISimpleCombinedRenderer::initEyeRay(uint32_t x, uint32_t y, uint32_t s_id, float4* rayPosAndNear, float4* rayDirAndFar)
{
  uint32_t spp_sqrt = uint32_t(std::sqrt(float(m_spp)));
  float i_spp_sqrt = 1.0f/spp_sqrt;
  float2 d = (m_AAFrameNum >= 2) ? rand2(m_seed, x, y, s_id + m_AAFrameNum + m_seed % 2) : 
                                   i_spp_sqrt*float2(s_id/spp_sqrt+0.5, s_id%spp_sqrt+0.5);
  float3 rayDir = EyeRayDirNormalized((float(x)+d.x)/float(m_packedXY_width), (float(y)+d.y)/float(m_packedXY_height), m_projInv);
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

void ISimpleCombinedRenderer::kernel2D_PackXY(uint32_t w, uint32_t h)
{
  #pragma omp parallel for
  for(uint32_t y=0;y<h;y++)
  {
    for(uint32_t x=0;x<w;x++)
    {
      const uint32_t offset   = SuperBlockIndex2DOpt(x, y, m_packedXY_width);
      m_packedXY[offset] = ((y << 16) & 0xFFFF0000) | (x & 0x0000FFFF);
    }
  }
}

void ISimpleCombinedRenderer::kernel1D_Tonemap(uint32_t count, uint32_t sample_count, uint32_t* out_color)
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

void ISimpleCombinedRenderer::kernel1D_AverageColor(uint32_t count, uint32_t sample_count, LiteMath::float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
    out_color[tidX] = m_colorBuffer[tidX] / float(sample_count); 
}