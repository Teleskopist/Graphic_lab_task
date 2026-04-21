#pragma once
#include <string>
#include "core/render_settings.h"
#include "core/IRenderer.h"
#include "blk/blk.h"

class Integrator;

struct HydraRenderPreset
{
  unsigned integratorType; //only default one (Integrator::INTEGRATOR_MIS_PT) is supported
  unsigned fbLayer;        //only default one (Integrator::FB_COLOR) is supported
  unsigned spp;
};

HydraRenderPreset getDefaultHydraRenderPreset();

class HydraRenderer : public IRenderer
{
public:
  constexpr static unsigned MAX_WIDTH  = 2048;
  constexpr static unsigned MAX_HEIGHT = 2048;
  constexpr static float GAMMA = 2.4f;

  HydraRenderer(unsigned device = DEVICE_CPU);

  virtual const char* Name() const  override { return "HydraRenderer"; }
  virtual bool LoadScene(const char* a_scenePath) override;
  virtual bool LoadScene(LiteScene::HydraScene &scene) override;

  virtual void Clear (uint32_t a_width, uint32_t a_height) override;
  virtual void Render(uint32_t* imageData, uint32_t a_width, uint32_t a_height, int a_passNum = 1) override;
  virtual void RenderFloat(LiteMath::float4* imageData, uint32_t a_width, uint32_t a_height, int a_passNum = 1) override;
  virtual void SetViewport(int a_xStart, int a_yStart, int a_width, int a_height) override;
  virtual void SetAccelStruct(std::shared_ptr<ISceneObject> a_customAccelStruct) override;
  virtual ISceneObject *GetAccelStruct() override;
  
  virtual void GetExecutionTime(const char* a_funcName, float a_out[4]) override;
  virtual void CommitDeviceData() override;
  virtual void UpdateCamera(const LiteMath::float4x4& a_worldView, const LiteMath::float4x4& a_proj) override;

  void SetPreset(HydraRenderPreset a_preset) { m_preset = a_preset; }
  HydraRenderPreset GetPreset() { return m_preset; }

private:

  void RenderInternal(uint32_t a_width, uint32_t a_height, int a_passNum);

  uint32_t m_width = 0;
  uint32_t m_height = 0;
  std::vector<float> realColor;
  std::shared_ptr<Integrator> m_pImpl = nullptr;
  HydraRenderPreset m_preset = getDefaultHydraRenderPreset();
  unsigned m_device = 0;
};

void save_to_blk(const HydraRenderPreset &settings, Block *block);
void load_from_blk(HydraRenderPreset &settings, const Block *block);