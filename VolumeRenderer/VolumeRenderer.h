#pragma once

#include "LiteMath.h"
#include "core/ISceneObject.h"
#include "core/IRenderer.h"
#include "sdf/scom2/scom_shared.h"
#include "utils/raytracing/intersection_common.h"
#include "core/render_settings.h"

struct SdfDAG;
struct SCom2Tree;

//enum VolumeTraversalMode
static constexpr unsigned VOLUME_TRAVERSAL_MODE_BRESENHAM      = 0;
static constexpr unsigned VOLUME_TRAVERSAL_MODE_DDA            = 1; 
static constexpr unsigned VOLUME_TRAVERSAL_MODE_DDA_BRANCHLESS = 2;
static constexpr unsigned VOLUME_TRAVERSAL_MODE_VRT_SS         = 3; //Volume Ray Tracing Single Scattering

//enum VolumeType
static constexpr unsigned VOLUME_TYPE_LINEAR_DENSITY = 0;
static constexpr unsigned VOLUME_TYPE_SVRASTER_RF    = 1;

static constexpr unsigned SCENE_EXTENT_USER_PARAM_ID = 0;
static constexpr unsigned SCENE_CENTER_X_USER_PARAM_ID = 1;
static constexpr unsigned SCENE_CENTER_Y_USER_PARAM_ID = 2;
static constexpr unsigned SCENE_CENTER_Z_USER_PARAM_ID = 3;

struct VolumeRenderPreset
{
  uint32_t traversal_mode; // VolumeTraversalMode
  uint32_t volume_type; // VolumeType
  uint32_t spp;
  uint32_t voxel_traversal_steps;
  uint32_t ignore_timestamps;
  uint32_t use_lighting;
  float alpha_thr;
  float animation_speed;
};

static VolumeRenderPreset getDefaultVolumeRenderPreset()
{
  VolumeRenderPreset preset;
  preset.traversal_mode = VOLUME_TRAVERSAL_MODE_BRESENHAM;
  preset.volume_type = VOLUME_TYPE_LINEAR_DENSITY;
  preset.voxel_traversal_steps = 1;
  preset.spp = 1;
  preset.alpha_thr = 0.01f;
  preset.animation_speed = 0.1f;
  preset.ignore_timestamps = 0;
  preset.use_lighting = 0;
  return preset;
}

class VolumeRenderer : public IRenderer, public ISceneObject
{
public:
  VolumeRenderer();

  // functions implementing IRenderer interface
  virtual const char *Name() const override;
  virtual bool LoadScene(const char *scenePath) override;
  virtual bool LoadScene(LiteScene::HydraScene &scene) override;
  virtual void Clear(uint32_t width, uint32_t height) override;
  virtual void Render(uint32_t *imageData [[size("width*height")]], uint32_t width, uint32_t height, int passNum = 1) override;
  virtual void RenderFloat(float4 *imageData [[size("width*height")]], uint32_t width, uint32_t height, int passNum = 1) override;
  virtual void SetViewport(int xStart, int yStart, int width, int height) override;
  virtual void SetAccelStruct(std::shared_ptr<ISceneObject> customAccelStruct) override;
  virtual ISceneObject *GetAccelStruct() override;
  
  virtual void CommitDeviceData() override {}
  virtual void GetExecutionTime(const char *funcName, float out[4]) override;
  virtual void UpdateCamera(const float4x4 &worldView, const float4x4 &proj) override;
  virtual void SetLights(const std::vector<Light> &lights) override;
  virtual void SetFrameNum(uint32_t num) override;
  virtual size_t getSceneSize() override;

  virtual void setCuttingPlane(Plane plane) override { m_cuttingPlane = plane; }
  virtual void setBackgroundColor(const LiteMath::float4& color) override { m_backgroundColor = color; }
  virtual void initEyeRay(uint32_t x, uint32_t y, uint32_t sample_id, LiteMath::float4* rayPosAndNear, LiteMath::float4* rayDirAndFar) override;

  // functions implementing ISceneObject interface
  virtual void ClearGeom() override;
  virtual void ClearScene() override;
  virtual void CommitScene(uint32_t options = BUILD_HIGH) override;
  virtual uint32_t AddInstance(uint32_t geomId, const float4x4 &matrix) override;
  virtual void     UpdateInstance(uint32_t instanceId, const float4x4& matrix) override;

  virtual bool    RayQuery_AnyHit(float4 posAndNear, float4 dirAndFar) override;
  virtual CRT_Hit RayQuery_NearestHit(float4 posAndNear, float4 dirAndFar) override;
  
  //unsupported stuff
  virtual uint32_t AddInstanceMotion(uint32_t geomId, const float4x4* matrices, uint32_t matrixNumber) override
  {
    return INVALID_IDX;
  }
  virtual uint32_t AddGeom_Triangles3f(const float* vpos3f, size_t vertNumber, const uint32_t* triIndices, size_t indNumber,
                                       uint32_t flags = BUILD_HIGH, size_t vByteStride = sizeof(float)*3) override
  {
    return INVALID_IDX;
  }
  virtual void UpdateGeom_Triangles3f(uint32_t geomId, const float* vpos3f, size_t vertNumber, const uint32_t* triIndices, size_t indNumber,
                                      uint32_t flags = BUILD_HIGH, size_t vByteStride = sizeof(float)*3) override
  {

  }
  virtual CRT_Hit RayQuery_NearestHitMotion(float4 posAndNear, float4 dirAndFar, float time) override
  {
    return CRT_Hit();
  }
  virtual bool    RayQuery_AnyHitMotion(float4 posAndNear, float4 dirAndFar, float time = 0.0f) override
  {
    return false;
  }

  virtual void nextFrame(float dt) override;
  virtual void UpdateMembersPlainData() override {}                               // will be overriden in generated class

  //helper functions for loading geometry
  //
#ifndef KERNEL_SLICER 
  // loads <color, density> regular grid of size*size*size
  void LoadGrid(uint32_t size, const float *grid, const std::vector<float> &timestamps);
  void LoadGrid(uint32_t size, const float4 *grid, const std::vector<float> &timestamps);
  void LoadDAG(const SdfDAG &dag, std::vector<float> timestamps = { 0.0f });
  void LoadSCom(const SCom2Tree &dag, std::vector<float> timestamps = { 0.0f });
#endif
  void SetPreset(const VolumeRenderPreset &preset) { m_preset = preset; }
  VolumeRenderPreset GetPreset() { return m_preset; }
  void SetBaseColor(float4 color) { m_baseColor = color; }

protected:
  //kernels
  //
  //fill m_packedXY with pixel ids
  virtual void kernel2D_PackXY(uint32_t w, uint32_t h);
  //convert accumulated color to LDR (8 bit per channel). Simple linear conversion
  virtual void kernel1D_Tonemap(uint32_t count, uint32_t sample_count, uint32_t* out_color);
  //divide accumulated color by number of samples, color remains in HDR
  virtual void kernel1D_AverageColor(uint32_t count, uint32_t sample_count, LiteMath::float4* out_color);

  virtual void kernel1D_RaymarchGrid_Bresenham(uint32_t count, uint32_t sample_id, float4* out_color);
  virtual void kernel1D_RaymarchGrid_DDA(uint32_t count, uint32_t sample_id, float4* out_color);
  virtual void kernel1D_RaymarchGrid_DDA_Branchless(uint32_t count, uint32_t sample_id, float4* out_color);
  virtual void kernel1D_RaytraceGrid_SS(uint32_t count, uint32_t sample_id, float4* out_color);

  virtual void kernel1D_RaymarchDAG(uint32_t count, uint32_t sample_id, float4* out_color);
  virtual void kernel1D_RaymarchDAG4D(uint32_t count, uint32_t sample_id, float4* out_color);
  virtual void kernel1D_RaymarchSCom(uint32_t count, uint32_t sample_id, float4* out_color);
  virtual void kernel1D_RaymarchSCom4D(uint32_t count, uint32_t sample_id, float4* out_color);

  //helper functions for rendering
  //
  float4 RaymarchGrid_Bresenham(float4 posAndNear, float4 dirAndFar);
  float4 RaymarchGrid_DDA(float4 posAndNear, float4 dirAndFar);
  void RaymarchGrid_DDA_SS_shadow(float4 posAndNear, float4 dirAndFar, float *out_T);
  float4 RaymarchGrid_DDA_Branchless(float4 posAndNear, float4 dirAndFar);
  float4 RaymarchGrid_SS(float4 posAndNear, float4 dirAndFar, uint32_t x, uint32_t y);
  void RaytraceGrid_SS(float4 posAndNear, float4 dirAndFar, uint32_t x, uint32_t y, uint32_t bounce, bool is_shadow, float3* out_C, float3* out_T);
  void RaytraceGrid_SS_shadow(float4 posAndNear, float4 dirAndFar, uint32_t x, uint32_t y, uint32_t bounce, bool is_shadow, float3* out_C, float3* out_T);

  float4 RaymarchDAG(float4 posAndNear, float4 dirAndFar);
  float4 RaymarchDAG4D(float4 posAndNear, float4 dirAndFar);
  float4 RaymarchSCom(float4 posAndNear, float4 dirAndFar);
  float4 RaymarchSCom4D(float4 posAndNear, float4 dirAndFar);
  void RaytraceSCom_SS_shadow(float4 posAndNear, float4 dirAndFar, float* out_T);
  void RaytraceSCom4D_SS_shadow(float4 posAndNear, float4 dirAndFar, float* out_T);

  float4 Sample(float3 p);
  float SampleD(float3 p, uint32_t frame);
  float4 SampleDC(float3 p, uint32_t frame);
  float3 computeColorFromSH(float3 vox_c, float3 ro, uint32_t idx);

  uint32_t m_width;
  uint32_t m_height;
  uint32_t m_packedXY_width;
  uint32_t m_packedXY_height;
  uint32_t m_seed;
  uint32_t m_AAFrameNum;
  uint32_t m_vox_cnt;
  uint32_t m_sz;
  Plane m_cuttingPlane;
  VolumeRenderPreset m_preset;

  float4x4 m_proj;
  float4x4 m_worldView;
  float4x4 m_projInv;
  float4x4 m_worldViewInv;
  float4   m_backgroundColor;
  float4   m_baseColor;

  uint32_t m_grid_type;
  float m_time;
  uint32_t frame0;
  uint32_t frame1;
  float m_dframe;

  // render targets, all arrays have the same size (m_width * m_height)
  std::vector<uint32_t>  m_packedXY;
  std::vector<float4>    m_colorBuffer; //color after resolve, used when RGBA8 output is required

  // scene data
  std::vector<float>  m_timestamps;
  std::vector<float>  m_grid;
  std::vector<float4> m_colored_grid;
  
#if !defined(DISABLE_SDF_DAG) || !defined(DISABLE_SCOM2)
  // has 3*48 rot modifiers - for distance values, for voxels, for child nodes
  static constexpr uint32_t ROT_COUNT = 48;
  std::vector<int4>     m_SdfCompactOctreeRotModifiers;
  std::vector<float4x4> m_SdfCompactOctreeRotTransforms;
  std::vector<float4x4> m_DAGInverseTransforms;
  std::vector<uint32_t> m_RotAddTable;

  static constexpr uint32_t SH_COMPONENT_COUNT = (3+1)*(3+1);
  std::vector<float4> m_SphericalHarmonics;
#endif

#ifndef DISABLE_SCOM2
  SCom2Header m_header;
  std::vector<uint32_t> m_SComNodes;
  std::vector<uint32_t> m_SComValues;
#endif

#ifndef DISABLE_SDF_DAG
  std::vector<SdfDAGHeader> m_SdfDAGHeaders;
  std::vector<SdfDAGNode> m_SdfDAGNodes;
  std::vector<SdfDAGChildEdge> m_SdfDAGChildEdges;
  std::vector<SdfDAGDataEdge> m_SdfDAGDataEdges;
  std::vector<float> m_SdfDAGDistances;
  std::vector<uint32_t> m_SdfDAGTranspositions;
  uint32_t m_SdfDAGBrickTranspositionOffset = 0;
  uint32_t m_SdfDAGTransformCount = 0;
#endif
};

#ifndef KERNEL_SLICER 
  std::shared_ptr<VolumeRenderer> CreateVolumeRenderer(unsigned /*enum RenderDevice*/ device);

  struct Block;
  void load_from_blk(VolumeRenderPreset &settings, const Block *block);
  void save_to_blk(const VolumeRenderPreset &settings, Block *block);
#endif