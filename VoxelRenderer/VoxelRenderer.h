#pragma once

#include "LiteMath.h"
#include "core/ISceneObject.h"
#include "core/IRenderer.h"
#include "Image2d.h"
#include "sdf/scom2/scom_shared.h"
#include "utils/raytracing/intersection_common.h"
#include "core/render_settings.h"
#include "voxel_octree.h"

struct SdfDAG;
struct SCom2Tree;
using LiteImage::Image2D;
using LiteImage::Sampler;
using LiteImage::ICombinedImageSampler;

static constexpr uint32_t VOXEL_RENDER_MODE_MASK = 0;
static constexpr uint32_t VOXEL_RENDER_MODE_LAMBERT_NO_TEX = 1;
static constexpr uint32_t VOXEL_RENDER_MODE_LAMBERT = 2;
static constexpr uint32_t VOXEL_RENDER_MODE_NORMAL = 3;
static constexpr uint32_t VOXEL_RENDER_MODE_PRIMITIVE = 4;

static constexpr uint32_t BLOCK_SIDE_LEFT   = 0;
static constexpr uint32_t BLOCK_SIDE_RIGHT  = 1;
static constexpr uint32_t BLOCK_SIDE_TOP    = 2;
static constexpr uint32_t BLOCK_SIDE_BOTTOM = 3;
static constexpr uint32_t BLOCK_SIDE_FRONT  = 4;
static constexpr uint32_t BLOCK_SIDE_BACK   = 5;

static constexpr uint32_t OCTREE_TRAVERSAL_MODE_DEFAULT = 0;
static constexpr uint32_t OCTREE_TRAVERSAL_MODE_FAST    = 1;

struct VoxelRenderPreset
{
  uint32_t spp;
  uint32_t render_mode;
  uint32_t octree_traversal_mode;
};

struct BlockMetadata
{
  uint32_t texIds[6];
};

static VoxelRenderPreset getDefaultVoxelRenderPreset()
{
  VoxelRenderPreset preset;
  preset.spp = 1;
  preset.render_mode = VOXEL_RENDER_MODE_LAMBERT;
  preset.octree_traversal_mode = OCTREE_TRAVERSAL_MODE_DEFAULT;
  return preset;
}

class VoxelRenderer : public IRenderer, public ISceneObject
{
public:
  VoxelRenderer();

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

  //helper functions for loading geometry
  //
#ifndef KERNEL_SLICER 
  // loads <color, density> regular grid of size*size*size
  void LoadDAG(const SdfDAG &dag);
  void LoadSCom(const SCom2Tree &dag);
  void LoadSVO(const std::vector<uint32_t> &data, uint32_t max_level_size);
#endif
  void SetPreset(const VoxelRenderPreset &preset) { m_preset = preset; }
  VoxelRenderPreset GetPreset() { return m_preset; }

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
  //intersect ray with Directed Acyclic Graph, for debug purposes only
  virtual void kernel1D_IntersectDAG(uint32_t count, uint32_t sample_id, float4* out_color);
  //intersect ray with Similarity compressed DAG
  virtual void kernel1D_IntersectSCom(uint32_t count, uint32_t sample_id, float4* out_color);
  //intersect ray with Sparse Voxel Octree
  virtual void kernel1D_IntersectSVO(uint32_t count, uint32_t sample_id, float4* out_color);
  //intersect ray with Sparse Voxel Octree
  virtual void kernel1D_IntersectSVOFast(uint32_t count, uint32_t sample_id, float4* out_color);

  //helper functions for rendering
  //
  CRT_Hit IntersectDAG(float4 posAndNear, float4 dirAndFar);
  CRT_Hit IntersectSCom(float4 posAndNear, float4 dirAndFar);
  CRT_Hit IntersectSVO(float4 posAndNear, float4 dirAndFar);
  CRT_Hit IntersectSVOFast(float4 posAndNear, float4 dirAndFar);
  float4  Resolve(const CRT_Hit &hit, float4 posAndNear, float4 dirAndFar);

  uint32_t m_width;
  uint32_t m_height;
  uint32_t m_packedXY_width;
  uint32_t m_packedXY_height;
  uint32_t m_seed;
  uint32_t m_AAFrameNum;
  uint32_t m_vox_cnt;
  uint32_t m_sz;

  float4x4 m_proj;
  float4x4 m_worldView;
  float4x4 m_projInv;
  float4x4 m_worldViewInv;
  float4   m_backgroundColor;

  uint32_t m_data_type;
  Plane m_cuttingPlane;
  VoxelRenderPreset m_preset;

  std::vector<Light> m_lights;
  std::vector<BlockMetadata> m_blocks;
  std::vector<std::shared_ptr<ICombinedImageSampler>> m_textures;

  // render targets, all arrays have the same size (m_width * m_height)
  std::vector<uint32_t>  m_packedXY;
  std::vector<float4>    m_colorBuffer; //color after resolve, used when RGBA8 output is required

  // scene data
  std::vector<uint32_t> m_SVO; //Sparse Voxel Octree
  uint32_t m_SVO_max_level_size; //2^max_level

#if !defined(DISABLE_SDF_DAG) || !defined(DISABLE_SCOM2)
  // has 3*48 rot modifiers - for distance values, for voxels, for child nodes
  static constexpr uint32_t ROT_COUNT = 48;
  std::vector<int4>     m_SdfCompactOctreeRotModifiers;
  std::vector<float4x4> m_SdfCompactOctreeRotTransforms;
  std::vector<uint32_t> m_RotAddTable;
  uint32_t m_DAG_max_level_size = 0;
#endif

#ifndef DISABLE_SCOM2
  SCom2Header m_header;
  std::vector<uint32_t> m_SComNodes;
#endif

#ifndef DISABLE_SDF_DAG
  std::vector<SdfDAGHeader> m_SdfDAGHeaders;
  std::vector<SdfDAGNode> m_SdfDAGNodes;
  std::vector<SdfDAGChildEdge> m_SdfDAGChildEdges;
  std::vector<SdfDAGDataEdge> m_SdfDAGDataEdges;
  std::vector<float> m_SdfDAGDistances;
  std::vector<int> m_SdfDAGBlockIds;
  std::vector<uint32_t> m_SdfDAGTranspositions;
  uint32_t m_SdfDAGBrickTranspositionOffset = 0;
  uint32_t m_SdfDAGTransformCount = 0;
#endif

  // color palette to select color for objects based on mesh/instance id
  static constexpr uint32_t palette_size = 20;
  static constexpr uint32_t m_palette[palette_size] = {
    0xffe6194b, 0xff3cb44b, 0xffffe119, 0xff0082c8,
    0xfff58231, 0xff911eb4, 0xff46f0f0, 0xfff032e6,
    0xffd2f53c, 0xfffabebe, 0xff008080, 0xffe6beff,
    0xffaa6e28, 0xfffffac8, 0xff800000, 0xffaaffc3,
    0xff808000, 0xffffd8b1, 0xff000080, 0xff808080
  };
};

#ifndef KERNEL_SLICER 
  std::shared_ptr<VoxelRenderer> CreateVoxelRenderer(unsigned /*enum RenderDevice*/ device);

  struct Block;
  void load_from_blk(VoxelRenderPreset &settings, const Block *block);
  void save_to_blk(const VoxelRenderPreset &settings, Block *block);
#endif