#pragma once
#include <cstdint>
#include <memory>

#include "utils/common/plane.h"
#include "core/ISceneObject.h"

#ifdef USE_GPU
#include "vk_utils.h"
#include "vk_copy.h"
#endif

namespace LiteScene { struct HydraScene; }

//enum LightType
static constexpr unsigned LIGHT_TYPE_DIRECT   = 0;
static constexpr unsigned LIGHT_TYPE_POINT    = 1;
static constexpr unsigned LIGHT_TYPE_AMBIENT  = 2;

//enum MultiRendererMaterialType
static constexpr unsigned MULTI_RENDER_MATERIAL_TYPE_COLORED  = 0;
static constexpr unsigned MULTI_RENDER_MATERIAL_TYPE_TEXTURED = 1;

static constexpr unsigned DEFAULT_MATERIAL = 0u;
static constexpr unsigned DEFAULT_TEXTURE = 0u;

static constexpr unsigned MULTI_RENDER_MAX_TEXTURES = 256;
static constexpr unsigned PACK_XY_BLOCK_SIZE = 8;

static constexpr unsigned HAS_NO_CHANNELS = 0xFFFFFFFF;

struct MultiRendererMaterial
{
  unsigned type;
  unsigned texId; // valid if type == MULTI_RENDER_MATERIAL_TYPE_TEXTURED 
  float metallic;
  float roughness;
  float4 base_color; // valid if type == MULTI_RENDER_MATERIAL_TYPE_COLORED
};

struct HydraSceneProperties
{
  uint32_t num_primitives;
};

//enum ChannelDataType
static constexpr unsigned CHANNEL_DATA_TYPE_INT    = 0;
static constexpr unsigned CHANNEL_DATA_TYPE_FLOAT  = 1;
static constexpr unsigned CHANNEL_DATA_TYPE_FP8    = 2;

//enum ChannelType
static constexpr unsigned CHANNEL_TYPE_VERTEX  = 0;
static constexpr unsigned CHANNEL_TYPE_FACE    = 1;

struct ChannelRenderInfo
{
  uint32_t components;// number of components in the channel
  uint32_t offset;    // offset in vector where the actual data for channel is stored
  uint32_t type;      // enum ChannelType
  uint32_t data_type; // enum ChannelDataType
  float min_val;
  float max_val;
};

struct ColorTranferFunctionInfo
{
  uint32_t sample_points_offset;
  uint32_t sample_points_count;
  uint32_t sample_steps;
};

struct Light
{
  float3 space; // position or direction
  unsigned type;// enum LightType
  float3 color; // intensity included
  unsigned _pad;
};

static Light create_direct_light(float3 dir, float3 color)
{
  return {normalize(dir), LIGHT_TYPE_DIRECT, color, 0};
}

static Light create_point_light(float3 pos, float3 color)
{
  return {pos, LIGHT_TYPE_POINT, color, 0};
}

static Light create_ambient_light(float3 color)
{
  return {float3(0,0,0), LIGHT_TYPE_AMBIENT, color, 0};
}

struct IRenderer
{
  IRenderer(){ }
  virtual ~IRenderer(){}

  // basic set of functions required by renderer
  virtual const char* Name() const = 0;
  virtual bool LoadScene(const char* a_scenePath) = 0;
  virtual bool LoadScene(LiteScene::HydraScene &scene) = 0;
  virtual void Clear (uint32_t a_width, uint32_t a_height) = 0;
  virtual void Render(uint32_t* imageData, uint32_t a_width, uint32_t a_height, int a_passNum = 1) = 0;
  virtual void RenderFloat(LiteMath::float4* imageData, uint32_t a_width, uint32_t a_height, int a_passNum = 1) = 0;
  virtual void SetViewport(int a_xStart, int a_yStart, int a_width, int a_height) = 0;
  virtual void SetAccelStruct(std::shared_ptr<ISceneObject> a_customAccelStruct) = 0;
  virtual ISceneObject *GetAccelStruct() = 0;
  
  // optional functions
  virtual void GetExecutionTime(const char* a_funcName, float a_out[4]){}; // will be overriden in generated class
  virtual void UpdateCamera(const LiteMath::float4x4& a_worldView, const LiteMath::float4x4& a_proj) {}
  virtual void SetLights(const std::vector<Light>& lights) {}
  virtual void SetFrameNum(uint32_t a_num) {}
  virtual uint32_t AddInstance(uint32_t a_geomId, const LiteMath::float4x4& a_matrix) { return 0; }
  virtual void UpdateInstances(const std::vector<uint32_t> &instanceIds, const std::vector<LiteMath::float4x4> &matrices) {}
  virtual size_t getSceneSize() { return 0; };
  virtual void initEyeRay(uint32_t x, uint32_t y, uint32_t sample_id, LiteMath::float4* rayPosAndNear, LiteMath::float4* rayDirAndFar) {}
  virtual void setCuttingPlane(Plane plane) {}
  virtual void setBackgroundColor(const LiteMath::float4& color) {}
  virtual void nextFrame(float dt) {} //if renderer supports animation, call one per frame. Time in seconds
  virtual void SetHighlightPreset(const HighlightPreset &a_highlight_preset) { }

  // functions to be overriden in GPU implementation
  virtual void CommitDeviceData() {}                                     // will be overriden in generated class
  virtual void UpdateMembersPlainData() {}                               // will be overriden in generated class, optional function
  virtual void UpdateMembersVectorData() {}                              // will be overriden in generated class, optional function
  virtual void UpdateMembersTexureData() {}                              // will be overriden in generated class, optional function
#ifdef USE_GPU
  virtual void UpdatePlainMembers(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine) {
    printf("UpdatePlainMembers was called on renderer without GPU implementation. It's likely a bug\n");
  }
  virtual void RenderCmd(VkCommandBuffer a_commandBuffer, uint32_t *out_color, uint32_t w, uint32_t h, int a_passNum = 1) {
    printf("RenderCmd was called on renderer without GPU implementation. It's likely a bug\n");
  }
  virtual void RenderFloatCmd(VkCommandBuffer a_commandBuffer, float4 *out_color, uint32_t w, uint32_t h, int a_passNum = 1) {
    printf("RenderFloatCmd was called on renderer without GPU implementation. It's likely a bug\n");
  } 
  virtual void SetVulkanInOutFor_Render(VkBuffer out_colorBuffer, size_t out_colorOffset, uint32_t dummyArgument = 0) {
    printf("SetVulkanInOutFor_Render was called on renderer without GPU implementation. It's likely a bug\n");
  }
  virtual void SetVulkanInOutFor_RenderFloat(VkBuffer out_colorBuffer, size_t out_colorOffset, uint32_t dummyArgument = 0) {
    printf("SetVulkanInOutFor_RenderFloat was called on renderer without GPU implementation. It's likely a bug\n");
  }
#endif

protected:

  IRenderer(const IRenderer& rhs) {}
  IRenderer& operator=(const IRenderer& rhs) { return *this;}
  
  virtual uint32_t GetGeomNum() const  { return 0; };
  virtual uint32_t GetInstNum() const  { return 0; };
  virtual const LiteMath::float4* GetGeomBoxes() const { return nullptr; };
};