#include "renderer.h"

class Integrator
{
  int a;
};

HydraRenderPreset getDefaultHydraRenderPreset()
{
  HydraRenderPreset preset;
  preset.integratorType = 0;
  preset.fbLayer = 0;
  preset.spp = 1;

  return preset;
}

HydraRenderer::HydraRenderer(unsigned device)
{
  printf("[HydraRenderer] Hydra is disabled. Rebuild project with -DUSE_HYDRA=ON to use this function\n");
}

bool HydraRenderer::LoadScene(const char* a_scenePath)
{
  printf("[HydraRenderer] Hydra is disabled. Rebuild project with -DUSE_HYDRA=ON to use this function\n");
  return false;
}

void HydraRenderer::Clear(uint32_t a_width, uint32_t a_height)
{
  printf("[HydraRenderer] Hydra is disabled. Rebuild project with -DUSE_HYDRA=ON to use this function\n");
}

void HydraRenderer::Render(uint32_t* imageData, uint32_t a_width, uint32_t a_height, int a_passNum)
{
  printf("[HydraRenderer] Hydra is disabled. Rebuild project with -DUSE_HYDRA=ON to use this function\n");
}

void HydraRenderer::SetViewport(int a_xStart, int a_yStart, int a_width, int a_height)
{
  printf("[HydraRenderer] Hydra is disabled. Rebuild project with -DUSE_HYDRA=ON to use this function\n");
}

void HydraRenderer::SetAccelStruct(std::shared_ptr<ISceneObject> a_customAccelStruct)
{
  printf("[HydraRenderer] Hydra is disabled. Rebuild project with -DUSE_HYDRA=ON to use this function\n");
}

ISceneObject *HydraRenderer::GetAccelStruct()
{
  printf("[HydraRenderer] Hydra is disabled. Rebuild project with -DUSE_HYDRA=ON to use this function\n");
  return nullptr;
}

void HydraRenderer::GetExecutionTime(const char* a_funcName, float a_out[4])
{
  printf("[HydraRenderer] Hydra is disabled. Rebuild project with -DUSE_HYDRA=ON to use this function\n");
}

void HydraRenderer::CommitDeviceData()
{
  printf("[HydraRenderer] Hydra is disabled. Rebuild project with -DUSE_HYDRA=ON to use this function\n");
}

void HydraRenderer::UpdateCamera(const LiteMath::float4x4& a_worldView, const LiteMath::float4x4& a_proj)
{
  printf("[HydraRenderer] Hydra is disabled. Rebuild project with -DUSE_HYDRA=ON to use this function\n");
}

bool HydraRenderer::LoadScene(LiteScene::HydraScene &scene)
{
  printf("[HydraRenderer] Hydra is disabled. Rebuild project with -DUSE_HYDRA=ON to use this function\n");
  return false;
}

void HydraRenderer::RenderFloat(LiteMath::float4* imageData, uint32_t a_width, uint32_t a_height, int a_passNum)
{
  printf("[HydraRenderer] Hydra is disabled. Rebuild project with -DUSE_HYDRA=ON to use this function\n");
}

void save_to_blk(const HydraRenderPreset &settings, Block *block)
{
  block->set_int("spp", settings.spp);
  block->set_int("fbLayer", settings.fbLayer);
  block->set_int("integratorType", settings.integratorType);
}

void load_from_blk(HydraRenderPreset &settings, const Block *block)
{
  settings = getDefaultHydraRenderPreset();
  settings.spp = block->get_int("spp", settings.spp);
  settings.fbLayer = block->get_int("fbLayer", settings.fbLayer);
  settings.integratorType = block->get_int("integratorType", settings.integratorType);
}