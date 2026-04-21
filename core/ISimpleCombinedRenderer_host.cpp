#include "core/ISimpleCombinedRenderer.h"
#include "utils/raytracing/render_common.h"

ISimpleCombinedRenderer::ISimpleCombinedRenderer()
{
  m_spp = 1;
  m_seed = 0;
  m_backgroundColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
  m_cuttingPlane = create_disabled_plane();
  m_AAFrameNum = 0;
  m_lights = {create_direct_light(float3(1,1,1), float3(2.0f/3.0f)), create_ambient_light(float3(0.0, 0.0, 0.0)),
              create_ambient_light(float3(0.25, 0.25, 0.25))};
}

void ISimpleCombinedRenderer::SetViewport(int a_xStart, int a_yStart, int a_width, int a_height)
{
  m_width  = a_width;
  m_height = a_height;
  m_packedXY_width  = PACK_XY_BLOCK_SIZE*((a_width + PACK_XY_BLOCK_SIZE - 1) / PACK_XY_BLOCK_SIZE);
  m_packedXY_height = PACK_XY_BLOCK_SIZE*((a_height + PACK_XY_BLOCK_SIZE - 1) / PACK_XY_BLOCK_SIZE);;
  
  m_packedXY.resize(m_packedXY_width*m_packedXY_height);
  m_colorBuffer.resize(m_width*m_height);
}

void ISimpleCombinedRenderer::UpdateCamera(const LiteMath::float4x4& a_worldView, const LiteMath::float4x4& a_proj)
{
  m_proj = a_proj;
  m_worldView = a_worldView;
  m_projInv      = inverse4x4(a_proj);
  m_worldViewInv = inverse4x4(a_worldView);
}

void ISimpleCombinedRenderer::SetLights(const std::vector<Light>& lights)
{
  for (int i = 0; i < std::min(m_lights.size(), lights.size()); i++)
    m_lights[i] = lights[i];
  Update_m_lights(0, m_lights.size());
}

void ISimpleCombinedRenderer::SetFrameNum(uint32_t num)
{
  m_AAFrameNum = num;
}