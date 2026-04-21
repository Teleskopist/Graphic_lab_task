#pragma once

#include <cstdint>
#include <memory>
#include <array>
#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>

#include "LiteMath.h"
#include "core/ISceneObject.h"
#include "core/IRenderer.h"
#include "core/ICAERenderer.h"
#include "Image2d.h"
#include "BVH2Common.h"
#include "sdf/simple/sdf_simple_shared.h"
#include "sdf/multi/sdf_multi_shared.h"
#include "sdf/scom/scom_shared.h"

namespace LiteScene
{
  struct HydraScene;
}

namespace cmesh4
{
  struct SimpleMesh;
  struct AugmentedMesh;
}

using LiteMath::uint;
using LiteImage::Image2D;
using LiteImage::Sampler;
using LiteImage::ICombinedImageSampler;
using std::abs;

struct SparseOctreeSettings;
struct RBezierView;
struct ColorTransferFunction;
struct DataChannel;
struct SdfMultiOctreeAugmented;
struct SdfDAG;
struct SdfGridView;
struct SdfFrameOctreeView;
struct SdfSVSView;
struct SdfSBSView;
struct SdfFrameOctreeTexView;
struct COctreeV3View;
struct SdfMultiOctreeView;

class MultiRenderer : public IRenderer, public ICAERenderer
{
public:
  MultiRenderer(uint32_t AS_type = AS_TYPE_COMMON, uint32_t maxPrimitives = 10'000'000);

  // functions implementing IRenderer interface
  virtual const char *Name() const override;
  virtual bool LoadScene(const char *a_scenePath) override;
  virtual bool LoadScene(LiteScene::HydraScene &scene) override;
  virtual void Clear(uint32_t a_width, uint32_t a_height) override;
  virtual void Render(uint32_t *imageData [[size("a_width*a_height")]], uint32_t a_width, uint32_t a_height, int a_passNum = 1) override;
  virtual void RenderFloat(LiteMath::float4 *imageData [[size("a_width*a_height")]], uint32_t a_width, uint32_t a_height, int a_passNum = 1) override;
  virtual void SetViewport(int a_xStart, int a_yStart, int a_width, int a_height) override;
  virtual void SetAccelStruct(std::shared_ptr<ISceneObject> a_customAccelStruct) override { m_pAccelStruct = a_customAccelStruct; }
  virtual ISceneObject *GetAccelStruct() override { return m_pAccelStruct.get(); }
  
  virtual void CommitDeviceData() override {}
  virtual void GetExecutionTime(const char *a_funcName, float a_out[4]) override;
  virtual void UpdateCamera(const LiteMath::float4x4 &a_worldView, const LiteMath::float4x4 &a_proj) override;
  virtual void SetLights(const std::vector<Light> &lights) override;
  virtual void SetFrameNum(uint32_t a_num) override { m_AAFrameNum = a_num; }
  virtual uint32_t AddInstance(uint32_t a_geomId, const LiteMath::float4x4 &a_matrix) override;
  virtual size_t getSceneSize() override;

  // functions implementing ICAERenderer interface
  virtual bool hasCAEData() const override;
  virtual std::vector<DataChannelInfo> getCAEPointChannelInfo() const override;
  virtual std::vector<DataChannelInfo> getCAEVoxelChannelInfo() const override;

  //a bunch of functions extending IRenderer to make working with MultiRenderer easier
#ifndef KERNEL_SLICER 
  void SetScene(const cmesh4::SimpleMesh &scene);
  void SetScene(const cmesh4::AugmentedMesh &scene);
  void SetScene(SdfGridView scene);
  void SetScene(SdfFrameOctreeView scene);
  void SetScene(SdfSVSView scene);
  void SetScene(SdfSBSView scene);
  void SetScene(SdfFrameOctreeTexView scene);
  void SetScene(GraphicsPrimView scene);
  void SetScene(const std::vector<GraphicsPrim>& scene);
  void SetScene(COctreeV3View scene, unsigned bvh_level = 0);
  void SetScene(SdfMultiOctreeView scene);
  void SetScene(const SdfMultiOctreeAugmented &scene);
  void SetScene(const SdfDAG &scene);
  void SetScene(const SCom2Tree &scene);
#endif

  uint32_t AddColorTransferFunction(const std::vector<float4> &ctf);
  void SetPreset(const MultiRenderPreset& a_preset);
  virtual void SetHighlightPreset(const HighlightPreset &a_highlight_preset) override;
  MultiRenderPreset GetPreset();

  virtual void UpdateMembersPlainData() override {} // will be overriden in generated class, optional function
  virtual void UpdateVisibilityBuffer() { }
  
  //required by slicer!
  virtual void SceneRestrictions(uint32_t a_restrictions[4]) const
  {
    uint32_t maxMeshes            = 1024;
    uint32_t maxTotalVertices     = m_maxPrimitives;
    uint32_t maxTotalPrimitives   = m_maxPrimitives;
    uint32_t maxPrimitivesPerMesh = m_maxPrimitives;

    a_restrictions[0] = maxMeshes;
    a_restrictions[1] = maxTotalVertices;
    a_restrictions[2] = maxTotalPrimitives;
    a_restrictions[3] = maxPrimitivesPerMesh;
  }

  void SetViewport(int a_xStart, int a_yStart, int a_width, int a_height, bool enable_color_buffer);
  uint32_t AddTexture(const Image2D<float4> &image);
  uint32_t AddMaterial(const MultiRendererMaterial &material);
  void     SetMaterial(uint32_t matId, uint32_t geomId);

  //checks if m_materials has any transparent materials and allow transparency  
  void CheckForTransparency();

  void UpdateInstance(uint32_t a_instanceId, const float4x4 &a_matrix);
  
#ifndef KERNEL_SLICER 
  void add_mesh_internal(const cmesh4::SimpleMesh& mesh, unsigned geomId);
  void add_channel(const DataChannel &channel, uint32_t channel_type, uint32_t expected_elements_num);
  void add_augmented_sdf_internal(const SdfMultiOctreeAugmented& sdf, unsigned geomId);
  void add_augmented_sdf_internal(const SdfDAG& sdf, unsigned geomId);
  void add_augmented_sdf_internal(const SCom2Tree& sdf, unsigned geomId);
  void add_augmented_mesh_internal(const cmesh4::AugmentedMesh& mesh, unsigned geomId);
  void add_SdfFrameOctreeTex_internal(SdfFrameOctreeTexView scene, unsigned geomId);
#endif

  LiteMath::float4x4 getProj() { return m_proj; }
  LiteMath::float4x4 getWorldView() { return m_worldView; }

  const std::vector<MultiRendererMaterial>& getMaterials() { return m_materials; }
  const std::vector<std::shared_ptr<ICombinedImageSampler>> &getTextures() { return m_textures; }

  void setSeed(uint32_t seed) { m_seed = seed; }
  uint32_t getSeed() const { return m_seed; }
  virtual void setCuttingPlane(Plane plane) override;
  virtual void setBackgroundColor(const LiteMath::float4& color) override { m_backgroundColor = color; }

  virtual void initEyeRay(uint32_t x, uint32_t y, uint32_t sample_id, LiteMath::float4* rayPosAndNear, LiteMath::float4* rayDirAndFar) override;

  void Render(uint32_t* imageData, uint32_t a_width, uint32_t a_height, 
              const LiteMath::float4x4& a_worldView, const LiteMath::float4x4& a_proj,
              MultiRenderPreset preset = getDefaultPreset(), int a_passNum = 1);
  void RenderFloat(LiteMath::float4* imageData, uint32_t a_width, uint32_t a_height, 
                   const LiteMath::float4x4& a_worldView, const LiteMath::float4x4& a_proj,
                   MultiRenderPreset preset = getDefaultPreset(), int a_passNum = 1);
protected:

  //Fill m_packedXY with ray indices to cast rays by tiles
  //called once after frame resize (SetViewport)
  virtual void kernel2D_PackXY(uint32_t w, uint32_t h);
  void PackXY(uint32_t tidX, uint32_t tidY);

  //cast primary rays, fill gbuffer with intersection data (CRT_Hit)
  virtual void kernel1D_FillGbuffer(uint32_t count, uint32_t sample_id);
  virtual void kernel1D_FillGbufferWithTransparency(uint32_t count, uint32_t sample_id);
  float4 AnyHit(CRT_Hit* hit, float4 rayPosAndNear, float4 rayDirAndFar);

  //resolve gbuffer with common modes (e.g. LAMBERT or PBR)
  virtual void kernel1D_ResolveCommon(uint32_t count, uint32_t sample_id, uint32_t a_render_mode, LiteMath::float4* out_color);
  virtual void kernel1D_ResolveCommonWithTransparency(uint32_t count, uint32_t sample_id, uint32_t render_mode, LiteMath::float4* out_color);
  float4 ResolveCommon(uint32_t tidX, uint32_t render_mode);
  //add transparent object after resolve
  float4 ResolveTransparency(uint32_t tidX, float4 color);

  //resolve gbuffer with debug render modes (e.g. MASK, DEPTH, NORMAL)
  virtual void kernel1D_ResolveDebug(uint32_t count, uint32_t sample_id, uint32_t render_mode, LiteMath::float4* out_color);

  //resolve gbuffer with render modes that use color transfer functions (CTFs), e.g. MULTI_RENDER_MODE_CHANNEL
  virtual void kernel1D_ResolveCTF(uint32_t count, uint32_t sample_id, uint32_t render_mode, LiteMath::float4* out_color);
  float4 ResolveCTF(uint32_t tidX, uint32_t a_render_mode);

  //resolve gbuffer with drawing outline
  virtual void kernel1D_ResolveOutline(uint32_t count, uint32_t sample_id, uint32_t render_mode, float4* out_color);
  float4 ResolveOutline(uint32_t tidX, uint32_t render_mode);
  
  //convert accumulated color to LDR (8 bit per channel). Simple linear conversion
  virtual void kernel1D_Tonemap(uint32_t count, uint32_t sample_count, uint32_t* out_color);
  uint32_t Tonemap(uint32_t tidX, uint32_t a_sample_count);

  //divide accumulated color by number of samples, color remains in HDR
  virtual void kernel1D_AverageColor(uint32_t count, uint32_t sample_count, LiteMath::float4* out_color);
  float4 AverageColor(uint32_t tidX, uint32_t a_sample_count);

  virtual void Update_m_lights(size_t a_start, size_t a_size) {};

  float4 applyHighlight(CRT_Hit hit);
  void loadNormalTcFromGbuffer(CRT_Hit hit, float3 ray_dir, float3 *norm, float2 *tc);
  float3 calculateChannelColor(CRT_Hit hit, uint32_t tidX);
  float3 calculateChannelColorSDF(CRT_Hit hit, uint32_t tidX);
  float3 calculateDirectChannelColorSDF(CRT_Hit hit, uint32_t tidX);
  float loadFromFloatChannel(uint32_t offset);
  float loadFromFP8Channel(uint32_t offset);

  uint32_t m_width;
  uint32_t m_height;
  uint32_t m_packedXY_width;
  uint32_t m_packedXY_height;
  MultiRenderPreset m_preset;
  HighlightPreset m_highlight;
  uint32_t m_seed;
  uint32_t m_AAFrameNum = 0;
  Plane m_cuttingPlane;

  LiteMath::float4x4 m_proj;
  LiteMath::float4x4 m_worldView;
  LiteMath::float4x4 m_projInv;
  LiteMath::float4x4 m_worldViewInv;

  float4 m_backgroundColor;

  std::shared_ptr<ISceneObject>  m_pAccelStruct;

  //all arrays have the same size (m_width * m_height)
  std::vector<uint32_t>  m_packedXY;
  std::vector<CRT_Hit>   m_gBuffer;
  std::vector<float4>    m_transparencyBuffer; //information for blending in resolve
  std::vector<float4>    m_colorBuffer; //color after resolve, used when RGBA8 output is required

  //duplicating data for meshes if we want to visualize them with textures
#ifndef DISABLE_MESH
  std::vector<float4> m_vertices; //.w is tc.x
  std::vector<float4> m_normals;  //.w is tc.y
  std::vector<uint32_t> m_indices;
  std::vector<uint2> m_geomOffsets;
#endif

#ifndef DISABLE_AUGMENTED_GEOMETRY
  std::vector<int>   m_allIntChannels;
  std::vector<float> m_allFloatChannels;
  std::vector<uint32_t> m_allCompressedChannels;
  std::vector<ChannelRenderInfo> m_allChannelRenderInfo;
  std::vector<uint4> m_allChannelOffsets; //for every geometry, starts of it's part in 
                                          //(m_allIntChannels, m_allFloatChannels, m_allCompressedChannels, m_allChannelRenderInfo)

  std::vector<ColorTranferFunctionInfo> m_allCTFs;
  std::vector<float4> m_allCTFPoints; //point is (color.rgb, treshold)
#endif

  //materials and textures if at least one textured type is enabled
//#if !defined(DISABLE_MESH_TEX) || !defined(DISABLE_SDF_TEX)
  std::vector<MultiRendererMaterial> m_materials;
  std::vector< std::shared_ptr<ICombinedImageSampler> > m_textures;
  std::vector<uint32_t> m_matIdbyPrimId;
  std::vector<uint2> m_matIdOffsets; //for every geometry, start and size of it's part of m_matIdbyPrimId
  unsigned active_textures_count = 0;

  bool m_sceneHasTransparency = false;
  bool m_sceneHasLoadedMaterials = false;

  std::vector<Light> m_lights;
//#endif
  std::vector<LiteMath::float4x4> m_instanceTransformInvTransposed;

  // color palette to select color for objects based on mesh/instance id
  static constexpr uint32_t palette_size = 20;
  static constexpr uint32_t m_palette[palette_size] = {
    0xffe6194b, 0xff3cb44b, 0xffffe119, 0xff0082c8,
    0xfff58231, 0xff911eb4, 0xff46f0f0, 0xfff032e6,
    0xffd2f53c, 0xfffabebe, 0xff008080, 0xffe6beff,
    0xffaa6e28, 0xfffffac8, 0xff800000, 0xffaaffc3,
    0xff808000, 0xffffd8b1, 0xff000080, 0xff808080
  };

  static constexpr uint32_t m_lod_palette[palette_size] = {
    0xff000000, 0xff000040, 0xff000080, 0xff0000c0,
    0xff0040ff, 0xff0080ff, 0xff00c0ff, 0xff00ffff,
    0xff00ffc0, 0xff00ff80, 0xff00ff40, 0xff00ff00,
    0xff00c000, 0xff008000, 0xff004000, 0xff000000,
    0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
  };

  //statistics
  std::unordered_map<std::string, float> timeDataByName;
  uint64_t m_totalTris         = 0;
  uint64_t m_totalTrisVisiable = 0;

  uint32_t m_maxPrimitives; //required in constructor to allocate enough memory in Vulkan
  bool m_sceneSet = false;
  
#ifndef KERNEL_SLICER
  std::vector<DataChannelInfo> m_CAEPointChannelsInfo;
  std::vector<DataChannelInfo> m_CAEVoxelChannelsInfo;
#endif
};

#ifndef KERNEL_SLICER 

std::string RenderDevice_to_string(unsigned /*enum RenderDevice*/ device);
unsigned  RenderDevice_from_string(const std::string &name);

std::string RenderASType_to_string(unsigned /*enum RenderASType*/ AS_type);
unsigned  RenderASType_from_string(const std::string &name);

std::shared_ptr<MultiRenderer> CreateMultiRenderer(uint32_t /*enum RenderDevice*/ device, 
                                                   uint32_t /*enum RenderASType*/ AS_type = AS_TYPE_COMMON, 
                                                   uint32_t maxPrimitives = 10'000'000,
                                                   uint32_t preferredDeviceId = (uint32_t)-1);
#endif
