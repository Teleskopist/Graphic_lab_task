#pragma once
#include "core/IRenderer.h"
#include <string>

using LiteMath::float2;
using LiteMath::float3;
using LiteMath::float4;
using LiteMath::float4x4;
using LiteMath::uint2;
using LiteMath::uint3;
using LiteMath::uint4;
using LiteMath::int2;
using LiteMath::int3;
using LiteMath::int4;
using LiteMath::sign;

class ISimpleCombinedRenderer : public IRenderer
{
public:
  ISimpleCombinedRenderer();
  void setShaderPath(const char* a_path) {m_shaderPath = std::string(a_path); }
  void SetPreset(uint32_t spp) { m_spp = spp; }

  // basic set of functions required by renderer
  virtual void SetAccelStruct(std::shared_ptr<ISceneObject> a_customAccelStruct) override { };
  virtual ISceneObject *GetAccelStruct() override { return nullptr; }
  virtual void GetExecutionTime(const char *funcName, float out[4]) override {}
  virtual std::string GetResourcesRootDir() {return m_shaderPath; }

  virtual void initEyeRay(uint32_t x, uint32_t y, uint32_t s_id, float4* rayPosAndNear, float4* rayDirAndFar) override;
  virtual void SetViewport(int a_xStart, int a_yStart, int a_width, int a_height) override;
  virtual void UpdateCamera(const LiteMath::float4x4& a_worldView, const LiteMath::float4x4& a_proj) override;
  virtual void SetLights(const std::vector<Light>& lights) override;
  virtual void SetFrameNum(uint32_t num) override;

protected:
  //updating data
  //
  virtual void Update_m_lights(size_t a_start, size_t a_size) {};

  //kernels
  //
  //fill m_packedXY with pixel ids
  virtual void kernel2D_PackXY(uint32_t w, uint32_t h);
  //convert accumulated color to LDR (8 bit per channel). Simple linear conversion
  virtual void kernel1D_Tonemap(uint32_t count, uint32_t sample_count, uint32_t* out_color);
  //divide accumulated color by number of samples, color remains in HDR
  virtual void kernel1D_AverageColor(uint32_t count, uint32_t sample_count, LiteMath::float4* out_color);

  uint32_t m_width;
  uint32_t m_height;
  uint32_t m_packedXY_width;
  uint32_t m_packedXY_height;
  uint32_t m_seed;
  uint32_t m_AAFrameNum;
  uint32_t m_spp;

  float4x4 m_proj;
  float4x4 m_worldView;
  float4x4 m_projInv;
  float4x4 m_worldViewInv;
  float4   m_backgroundColor;

  Plane m_cuttingPlane;

  std::vector<Light> m_lights;
  std::string m_shaderPath = ".";
  
  // render targets, all arrays have the same size (m_width * m_height)
  std::vector<uint32_t>  m_packedXY;
  std::vector<float4>    m_colorBuffer; //color after resolve, used when RGBA8 output is required
};