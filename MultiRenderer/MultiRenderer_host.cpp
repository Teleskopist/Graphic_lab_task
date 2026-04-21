#include <cfloat>
#include <cstring>
#include <sstream>
#include <fstream>

#include "MultiRenderer.h"
#include "LiteScene/hydraxml.h"
#include "utils/mesh/mesh.h"
#include "utils/common/timer.h"
#include "utils/mesh/mesh.h"
#include "utils/mesh/mesh_bvh.h"
#include "utils/mesh/mesh_internal.h"
#include "utils/mesh/augmented_mesh.h"
#include "core/scene_extension.h"
#include "LiteScene/scene.h"
#include "utils/common/strings.h"
#include "sdf/simple/sdf_simple.h"
#include "sdf/multi/sdf_multi.h"
#include "sdf/scom/scom.h"
#include "sdf/scom2/scom.h"

using LiteMath::float2;
using LiteMath::float3;
using LiteMath::float4;

using LiteMath::perspectiveMatrix;
using LiteMath::lookAt;
using LiteMath::inverse4x4;

MultiRenderer::MultiRenderer(uint32_t AS_type, uint32_t maxPrimitives) 
{ 
  m_maxPrimitives = maxPrimitives;
  if (AS_type == AS_TYPE_COMMON)
    SetAccelStruct(CreateSceneRT("BVH2Common", "cbvh_embree2", "SuperTreeletAlignedMerged4"));
  else
    SetAccelStruct(nullptr);
  m_preset = getDefaultPreset();
  m_highlight = getEmptyHighlightPreset();

  m_textures.resize(MULTI_RENDER_MAX_TEXTURES);

  LiteImage::Image2D<float4> texture = LiteImage::Image2D<float4>(16, 16, float4(0,1,1,1)); //LiteImage::LoadImage<float4>("scenes/porcelain.jpg");
  for (int i=0;i<MULTI_RENDER_MAX_TEXTURES;i++)
    AddTexture(texture);
  active_textures_count = 1;

  MultiRendererMaterial mat = LiteScene::get_default_material();
  mat.base_color = float4(1,1,1,1);
  mat.type = MULTI_RENDER_MATERIAL_TYPE_COLORED;
  m_materials.push_back(mat);
  m_matIdbyPrimId.push_back(DEFAULT_MATERIAL);

  m_seed = rand();

  m_lights = {create_direct_light(float3(1,1,1), float3(2.0f/3.0f)), create_ambient_light(float3(0.0, 0.0, 0.0)),
              create_ambient_light(float3(0.25, 0.25, 0.25))};

  m_backgroundColor = float4(0.0f, 0.0f, 0.0f, 1.0f);

  setCuttingPlane(create_disabled_plane());

  std::vector<float4> CTF_0 = {float4(0.1, 0.1, 0.1, 0), float4(1, 1, 1, 1)};
  std::vector<float4> CTF_1 = {float4(0, 0, 1, 0), float4(1, 1, 1, 0.5), float4(1, 0, 0, 1)};
  AddColorTransferFunction(CTF_0);
  AddColorTransferFunction(CTF_1);
}

void MultiRenderer::setCuttingPlane(Plane plane)
{
  m_cuttingPlane = plane;
  auto bvhrt = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
  if (bvhrt)
    bvhrt->m_cuttingPlane = plane;
}

void MultiRenderer::SetViewport(int a_xStart, int a_yStart, int a_width, int a_height, bool enable_color_buffer)
{
  m_width  = a_width;
  m_height = a_height;
  m_packedXY_width  = PACK_XY_BLOCK_SIZE*((a_width + PACK_XY_BLOCK_SIZE - 1) / PACK_XY_BLOCK_SIZE);
  m_packedXY_height = PACK_XY_BLOCK_SIZE*((a_height + PACK_XY_BLOCK_SIZE - 1) / PACK_XY_BLOCK_SIZE);;
  
  m_packedXY.resize(m_packedXY_width*m_packedXY_height);
  m_gBuffer.resize(m_width*m_height);
  m_transparencyBuffer.resize(m_width*m_height);
  if (enable_color_buffer)
    m_colorBuffer.resize(m_width*m_height);
  else
    m_colorBuffer.resize(1);
}

void MultiRenderer::SetViewport(int a_xStart, int a_yStart, int a_width, int a_height)
{
  SetViewport(a_xStart, a_yStart, a_width, a_height, true);
}

bool MultiRenderer::LoadScene(const char* a_scenePath)
{
  LiteScene::HydraScene scene;
  scene.load(a_scenePath);
  return LoadScene(scene);
}

void MultiRenderer::UpdateCamera(const LiteMath::float4x4& worldView, const LiteMath::float4x4& proj)
{
  m_proj = proj;
  m_worldView = worldView;
  m_projInv      = inverse4x4(proj);
  m_worldViewInv = inverse4x4(worldView);
}

size_t MultiRenderer::getSceneSize()
{
  auto bvhrt = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
  if (!bvhrt)
  {
    printf("[MultiRenderer::getSceneSize]Underlying accel struct is not BVHRT\n");
    return 0;
  }
  size_t BVHRT_size = bvhrt->get_model_size(0);
  size_t self_size = 0;

  self_size += sizeof(float4)*m_vertices.size();
  self_size += sizeof(float4)*m_normals.size();
  self_size += sizeof(uint32_t)*m_indices.size();
  self_size += sizeof(uint2)*m_geomOffsets.size();
  self_size += sizeof(int)*m_allIntChannels.size();
  self_size += sizeof(float)*m_allFloatChannels.size();
  self_size += sizeof(uint32_t)*m_allCompressedChannels.size();
  self_size += sizeof(uint4)*m_allChannelOffsets.size();
  self_size += sizeof(uint32_t)*m_allChannelRenderInfo.size();
  self_size += sizeof(ColorTranferFunctionInfo)*m_allCTFs.size();
  self_size += sizeof(float4)*m_allCTFPoints.size();
  self_size += sizeof(MultiRendererMaterial)*m_materials.size();
  self_size += sizeof(uint32_t)*m_matIdbyPrimId.size();
  self_size += sizeof(uint2)*m_matIdOffsets.size();
  self_size += sizeof(Light)*m_lights.size();
  self_size += sizeof(LiteMath::float4x4)*m_instanceTransformInvTransposed.size();

  return self_size + BVHRT_size;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

const char* MultiRenderer::Name() const
{
  return m_pAccelStruct->Name();
}

void MultiRenderer::GetExecutionTime(const char* a_funcName, float a_out[4])
{
  auto p = timeDataByName.find(a_funcName);
  if(p == timeDataByName.end())
    return;
  a_out[0] = p->second;
}

bool MultiRenderer::hasCAEData() const
{
  return (m_CAEPointChannelsInfo.size() + m_CAEVoxelChannelsInfo.size()) > 0;
}

std::vector<DataChannelInfo> MultiRenderer::getCAEPointChannelInfo() const
{
  return m_CAEPointChannelsInfo;
}

std::vector<DataChannelInfo> MultiRenderer::getCAEVoxelChannelInfo() const
{
  return m_CAEVoxelChannelsInfo;
}

void MultiRenderer::SetScene(const cmesh4::SimpleMesh &scene)
{
  assert(!m_sceneSet); //SetScene must be called only once for renderer
  m_sceneSet = true;

  SetPreset(m_preset);
  m_pAccelStruct->ClearGeom();
  unsigned geomId = m_pAccelStruct->AddGeom_Triangles3f((const float*)scene.vPos4f.data(), scene.vPos4f.size(), scene.indices.data(), scene.indices.size(), BUILD_HIGH, sizeof(float)*4);                                                          
  add_mesh_internal(scene, geomId);
  m_pAccelStruct->ClearScene();
  AddInstance(geomId, LiteMath::float4x4());
  m_pAccelStruct->CommitScene();
}

void MultiRenderer::SetScene(const cmesh4::AugmentedMesh &scene)
{
  assert(!m_sceneSet); //SetScene must be called only once for renderer
  m_sceneSet = true;

  SetPreset(m_preset);
  m_pAccelStruct->ClearGeom();
  unsigned geomId = m_pAccelStruct->AddGeom_Triangles3f((const float*)scene.vertices.data(), scene.vertices.size(), scene.indices.data(), scene.indices.size(), BUILD_HIGH, sizeof(float)*3);                                                          
  add_augmented_mesh_internal(scene, geomId);
  m_pAccelStruct->ClearScene();
  AddInstance(geomId, LiteMath::float4x4());
  m_pAccelStruct->CommitScene();
}

void MultiRenderer::SetScene(SdfGridView scene)
{
  assert(!m_sceneSet); //SetScene must be called only once for renderer
  m_sceneSet = true;
  
  BVHRT *bvhrt = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
  if (!bvhrt)
  {
    printf("only BVHRT supports SdfGrid\n");
    return;
  }

  SetPreset(m_preset);
  m_pAccelStruct->ClearGeom();
  auto geomId = bvhrt->AddGeom_SdfGrid(scene, m_pAccelStruct.get());
  m_pAccelStruct->ClearScene();
  AddInstance(geomId, LiteMath::float4x4());
  m_pAccelStruct->CommitScene();
}

void MultiRenderer::SetScene(SdfFrameOctreeView scene)
{ 
  assert(!m_sceneSet); //SetScene must be called only once for renderer
  m_sceneSet = true;

  BVHRT *bvhrt = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
  if (!bvhrt)
  {
    printf("only BVHRT supports SdfFrameOctree\n");
    return;
  }

  SetPreset(m_preset);
  m_pAccelStruct->ClearGeom();
  auto geomId = bvhrt->AddGeom_SdfFrameOctree(scene, m_pAccelStruct.get());
  m_pAccelStruct->ClearScene();
  AddInstance(geomId, LiteMath::float4x4());
  m_pAccelStruct->CommitScene();
}

void MultiRenderer::SetScene(SdfSVSView scene)
{
  assert(!m_sceneSet); //SetScene must be called only once for renderer
  m_sceneSet = true;

  BVHRT *bvhrt = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
  if (!bvhrt)
  {
    printf("only BVHRT supports SdfSVS\n");
    return;
  }

  SetPreset(m_preset);
  m_pAccelStruct->ClearGeom();
  auto geomId = bvhrt->AddGeom_SdfSVS(scene, m_pAccelStruct.get());
  m_pAccelStruct->ClearScene();
  AddInstance(geomId, LiteMath::float4x4());
  m_pAccelStruct->CommitScene();
}


void MultiRenderer::SetScene(SdfSBSView scene)
{
  assert(!m_sceneSet); //SetScene must be called only once for renderer
  m_sceneSet = true;

  BVHRT *bvhrt = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
  if (!bvhrt)
  {
    printf("only BVHRT supports SdfSBS\n");
    return;
  }

  SetPreset(m_preset);
  m_pAccelStruct->ClearGeom();
  auto geomId = bvhrt->AddGeom_SdfSBS(scene, m_pAccelStruct.get());
  m_pAccelStruct->ClearScene();
  AddInstance(geomId, LiteMath::float4x4());
  m_pAccelStruct->CommitScene();
}

void MultiRenderer::SetScene(SdfFrameOctreeTexView scene)
{
  assert(!m_sceneSet); //SetScene must be called only once for renderer
  m_sceneSet = true;

  BVHRT *bvhrt = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
  if (!bvhrt)
  {
    printf("only BVHRT supports SdfFrameOctreeTex\n");
    return;
  }

  SetPreset(m_preset);
  m_pAccelStruct->ClearGeom();
  auto geomId = bvhrt->AddGeom_SdfFrameOctreeTex(scene, m_pAccelStruct.get());
  add_SdfFrameOctreeTex_internal(scene, geomId);
  m_pAccelStruct->ClearScene();
  AddInstance(geomId, LiteMath::float4x4());
  m_pAccelStruct->CommitScene();
}

void MultiRenderer::SetScene(GraphicsPrimView scene)
{
  assert(!m_sceneSet); //SetScene must be called only once for renderer
  m_sceneSet = true;
  
  BVHRT *bvhrt = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
  if (!bvhrt)
  {
    printf("only BVHRT supports Graphics primitives\n");
    return;
  }

  SetPreset(m_preset);
  m_pAccelStruct->ClearGeom();
  auto geomId = bvhrt->AddGeom_GraphicsPrim(scene, m_pAccelStruct.get());
  m_pAccelStruct->ClearScene();
  AddInstance(geomId, LiteMath::float4x4());
  m_pAccelStruct->CommitScene();
}

void MultiRenderer::SetScene(const std::vector<GraphicsPrim>& scene)
{
  assert(!m_sceneSet); //SetScene must be called only once for renderer
  m_sceneSet = true;
  
  BVHRT *bvhrt = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
  if (!bvhrt)
  {
    printf("only BVHRT supports Graphics primitives\n");
    return;
  }

  SetPreset(m_preset);

  m_pAccelStruct->ClearGeom();
  m_pAccelStruct->ClearScene();

  for (auto& prim: scene)
  {
    auto geomId = bvhrt->AddGeom_GraphicsPrim(prim, m_pAccelStruct.get());
    AddInstance(geomId, LiteMath::float4x4());
  }

  m_pAccelStruct->CommitScene();
}

void MultiRenderer::SetScene(COctreeV3View scene, unsigned bvh_level)
{
  assert(!m_sceneSet); //SetScene must be called only once for renderer
  m_sceneSet = true;
  
  BVHRT *bvhrt = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));

  SetPreset(m_preset);
  m_pAccelStruct->ClearGeom();
  uint32_t geomId = 0;

  if (bvhrt)
    geomId = bvhrt->AddGeom_COctreeV3(scene, bvh_level, m_pAccelStruct.get());
  else
  {
    printf("only BVHRT supports Compact Octree v3\n");
    return;
  }
  m_pAccelStruct->ClearScene();
  AddInstance(geomId, LiteMath::float4x4());
  m_pAccelStruct->CommitScene();
}

void MultiRenderer::SetScene(SdfMultiOctreeView scene)
{
  assert(!m_sceneSet); //SetScene must be called only once for renderer
  m_sceneSet = true;
  
  BVHRT *bvhrt = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
  if (!bvhrt)
  {
    printf("only BVHRT supports Sdf Multi Octree\n");
    return;
  }

  SetPreset(m_preset);
  m_pAccelStruct->ClearGeom();
  auto geomId = bvhrt->AddGeom_SdfMultiOctree(scene, m_pAccelStruct.get());
  m_pAccelStruct->ClearScene();
  AddInstance(geomId, LiteMath::float4x4());
  m_pAccelStruct->CommitScene();
}

void MultiRenderer::SetScene(const SdfMultiOctreeAugmented &scene)
{
  assert(!m_sceneSet); //SetScene must be called only once for renderer
  m_sceneSet = true;
  
  BVHRT *bvhrt = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
  if (!bvhrt)
  {
    printf("only BVHRT supports Sdf Multi Octree\n");
    return;
  }

  SetPreset(m_preset);
  m_pAccelStruct->ClearGeom();
  auto geomId = bvhrt->AddGeom_SdfMultiOctree(scene.octree, m_pAccelStruct.get());
  m_pAccelStruct->ClearScene();
  AddInstance(geomId, LiteMath::float4x4());
  m_pAccelStruct->CommitScene();
  add_augmented_sdf_internal(scene, geomId);
}

void MultiRenderer::SetScene(const SdfDAG &scene)
{
  assert(!m_sceneSet); //SetScene must be called only once for renderer
  m_sceneSet = true;
  
  BVHRT *bvhrt = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
  if (!bvhrt)
  {
    printf("only BVHRT supports Sdf DAG\n");
    return;
  }

  SetPreset(m_preset);
  m_pAccelStruct->ClearGeom();
  auto geomId = bvhrt->AddGeom_SdfDAG(scene, m_pAccelStruct.get());
  m_pAccelStruct->ClearScene();
  AddInstance(geomId, LiteMath::float4x4());
  m_pAccelStruct->CommitScene();
  add_augmented_sdf_internal(scene, geomId);
}

void MultiRenderer::SetScene(const SCom2Tree &scene)
{
  assert(!m_sceneSet); //SetScene must be called only once for renderer
  m_sceneSet = true;
  
  BVHRT *bvhrt = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
  if (!bvhrt)
  {
    printf("only BVHRT supports Sdf DAG\n");
    return;
  }

  SetPreset(m_preset);
  m_pAccelStruct->ClearGeom();
  auto geomId = bvhrt->AddGeom_SCom2(scene, m_pAccelStruct.get());
  m_pAccelStruct->ClearScene();
  AddInstance(geomId, LiteMath::float4x4());
  m_pAccelStruct->CommitScene();
  add_augmented_sdf_internal(scene, geomId);
}

void MultiRenderer::SetPreset(const MultiRenderPreset& a_preset)
{
  m_preset = a_preset;
  auto *bvhrt = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
  if (bvhrt)
    bvhrt->SetPreset(a_preset);
}

void MultiRenderer::SetHighlightPreset(const HighlightPreset &a_highlight_preset)
{
  m_highlight = a_highlight_preset;
}

MultiRenderPreset MultiRenderer::GetPreset()
{
  return m_preset;
}

void MultiRenderer::Render(uint32_t* imageData, uint32_t a_width, uint32_t a_height, 
                           const LiteMath::float4x4& a_worldView, const LiteMath::float4x4& a_proj,
                           MultiRenderPreset preset, int a_passNum)
{
  SetViewport(0,0, a_width, a_height);
  UpdateCamera(a_worldView, a_proj);
  SetPreset(preset);
  CommitDeviceData();
  Clear(a_width, a_height);
  Render(imageData, a_width, a_height, a_passNum); 
}

void MultiRenderer::RenderFloat(float4* imageData, uint32_t a_width, uint32_t a_height, 
                                const LiteMath::float4x4& a_worldView, const LiteMath::float4x4& a_proj,
                                MultiRenderPreset preset, int a_passNum)
{
  SetViewport(0,0, a_width, a_height);
  UpdateCamera(a_worldView, a_proj);
  SetPreset(preset);
  CommitDeviceData();
  Clear(a_width, a_height);
  RenderFloat(imageData, a_width, a_height, a_passNum); 
}

void MultiRenderer::add_mesh_internal(const cmesh4::SimpleMesh &scene, uint32_t geomId)
{
  if(geomId & CRT_GEOM_MASK_AABB_BIT) // TEMP SOLUTION !!!
    geomId = (geomId & 0x7fffffff);   // TEMP SOLUTION !!!

  m_geomOffsets.resize(geomId + 1, uint2(0,0));
  m_geomOffsets[geomId].x = m_indices.size();
  m_geomOffsets[geomId].y = m_vertices.size();
  m_indices.insert(m_indices.end(), scene.indices.begin(), scene.indices.end());

  unsigned sz = scene.vPos4f.size();
  m_vertices.resize(m_vertices.size() + sz);
  m_normals.resize(m_normals.size() + sz);

  for (unsigned i = 0; i < sz; ++i)
  {
    m_vertices[m_geomOffsets[geomId].y + i] = scene.vPos4f[i];
    m_vertices[m_geomOffsets[geomId].y + i].w = scene.vTexCoord2f[i].x;

    m_normals[m_geomOffsets[geomId].y + i] = scene.vNorm4f[i];
    m_normals[m_geomOffsets[geomId].y + i].w = scene.vTexCoord2f[i].y;
  }

  //add material if it was not explicitly set before
  if (geomId >= m_matIdOffsets.size())
  {
    m_matIdOffsets.resize(geomId + 1, uint2(0,1));
    
    //no material, set default
    if (scene.matIndices.empty())
    {
      m_matIdbyPrimId.push_back(DEFAULT_MATERIAL);
      m_matIdOffsets[geomId] = uint2(m_matIdbyPrimId.size()-1, 1);
    }
    else
    {
      m_matIdbyPrimId.insert(m_matIdbyPrimId.end(), scene.matIndices.begin(), scene.matIndices.end());
      m_matIdOffsets[geomId] = uint2(m_matIdbyPrimId.size()-scene.matIndices.size(), scene.matIndices.size());
    }
  }
}

void MultiRenderer::add_channel(const DataChannel &channel, uint32_t channel_type, uint32_t expected_elements_num)
{
  assert(channel.num_components > 0);
  if (channel.type == DataChannel::Type::INT)
    assert(channel.data_i.size() == channel.num_components * expected_elements_num || expected_elements_num == INVALID_IDX);
  else if (channel.type == DataChannel::Type::FLOAT)
    assert(channel.data_f.size() == channel.num_components * expected_elements_num || expected_elements_num == INVALID_IDX);
  else if (channel.type == DataChannel::Type::FP8)
    assert(channel.data_fp8.size() == channel.num_components * expected_elements_num || expected_elements_num == INVALID_IDX);
  else
    assert(false);

  ChannelRenderInfo info;
  info.components = 0;
  info.data_type = 0;
  info.max_val = 0.0f;
  info.min_val = 0.0f;
  info.offset = 0;
  info.type = 0;

  if (channel.type == DataChannel::Type::INT)
  {
    float max_val = channel.max_val;
    float min_val = channel.min_val;
    if (channel.max_val == DataChannel::LIMIT_UNDEFINED || channel.min_val == DataChannel::LIMIT_UNDEFINED)
    {
      max_val = channel.data_i[0];
      min_val = channel.data_i[0];

      for (const auto &val : channel.data_i)
      {
        max_val = std::max(max_val, (float)val);
        min_val = std::min(min_val, (float)val);
      }
    }

    info.components = channel.num_components;
    info.data_type = CHANNEL_DATA_TYPE_INT;
    info.max_val = max_val;
    info.min_val = min_val;
    info.offset = m_allIntChannels.size();
    info.type = channel_type;

    m_allIntChannels.insert(m_allIntChannels.end(), channel.data_i.begin(), channel.data_i.end());
  }
  else if (channel.type == DataChannel::Type::FLOAT)
  {
    float max_val = channel.max_val;
    float min_val = channel.min_val;
    if (channel.max_val == DataChannel::LIMIT_UNDEFINED || channel.min_val == DataChannel::LIMIT_UNDEFINED)
    { 
      max_val = channel.data_f[0];
      min_val = channel.data_f[0];

      for (const auto &val : channel.data_f)
      {
        max_val = std::max(max_val, (float)val);
        min_val = std::min(min_val, (float)val);
      }
    }

    info.components = channel.num_components;
    info.data_type = CHANNEL_DATA_TYPE_FLOAT;
    info.max_val = max_val;
    info.min_val = min_val;
    info.offset = m_allFloatChannels.size();
    info.type = channel_type;

    m_allFloatChannels.insert(m_allFloatChannels.end(), channel.data_f.begin(), channel.data_f.end());
  }
  else if (channel.type == DataChannel::Type::FP8)
  {
    info.components = channel.num_components;
    info.data_type = CHANNEL_DATA_TYPE_FP8;
    info.max_val = 1.0f;                              // compressed channels are always [0,1], data is aready normalized
    info.min_val = 0.0f;                              // and rescaled when mesh is created
    info.offset = 4 * m_allCompressedChannels.size(); // offset in values, and we have 4 values per uint32
    info.type = channel_type;

    uint32_t prev_size = m_allCompressedChannels.size();
    uint32_t size_uint = (channel.data_fp8.size() + 3) / 4;
    m_allCompressedChannels.resize(prev_size + size_uint);
    memcpy(&m_allCompressedChannels[prev_size], channel.data_fp8.data(), channel.data_fp8.size());
  }

  m_allChannelRenderInfo.push_back(info);
}

void MultiRenderer::add_augmented_sdf_internal(const SdfMultiOctreeAugmented& sdf, unsigned geomId)
{
#ifndef DISABLE_AUGMENTED_GEOMETRY
  m_allChannelOffsets.resize(geomId + 1, uint4(HAS_NO_CHANNELS, 0, 0, 0));
  m_allChannelOffsets[geomId] = uint4(m_allIntChannels.size(), 
                                      m_allFloatChannels.size(), 
                                      m_allCompressedChannels.size(), 
                                      m_allChannelRenderInfo.size());
  for (const auto &channel : sdf.point_channels)
    add_channel(channel, CHANNEL_TYPE_VERTEX, 8*sdf.octree.nodes.size());
  for (const auto &channel : sdf.voxel_channels)
    add_channel(channel, CHANNEL_TYPE_FACE, sdf.octree.nodes.size());

  bool is_CAE = true;
  if (is_CAE)
  {
    for (auto &ch : sdf.point_channels)
      m_CAEPointChannelsInfo.push_back(get_info(ch));
    for (auto &ch : sdf.voxel_channels)
      m_CAEVoxelChannelsInfo.push_back(get_info(ch));   
  }
#else
  printf("Support for augmented SDFs is disabled. Channel data will be ignored.\n");
#endif
}

void MultiRenderer::add_augmented_sdf_internal(const SdfDAG& sdf, unsigned geomId)
{
#ifndef DISABLE_AUGMENTED_GEOMETRY
  if (sdf.header.mat_id_off >= 0)
  {
    assert(sdf.voxel_channels.size() > sdf.header.mat_id_off);
    assert(sdf.voxel_channels[sdf.header.mat_id_off].type == DataChannel::Type::INT);
    static_assert(sizeof(int) == sizeof(unsigned));
    const unsigned *matIds = (const unsigned *)sdf.voxel_channels[sdf.header.mat_id_off].data_i.data();
    unsigned matIdsCount = sdf.voxel_channels[sdf.header.mat_id_off].data_i.size();
    m_matIdOffsets.resize(geomId + 1, uint2(0,1));
    m_matIdOffsets[geomId] = uint2(m_matIdbyPrimId.size(), matIdsCount);
    m_matIdbyPrimId.insert(m_matIdbyPrimId.end(), matIds, matIds + matIdsCount);
  }
  
  m_allChannelOffsets.resize(geomId + 1, uint4(HAS_NO_CHANNELS, 0, 0, 0));
  m_allChannelOffsets[geomId] = uint4(m_allIntChannels.size(), 
                                      m_allFloatChannels.size(), 
                                      m_allCompressedChannels.size(), 
                                      m_allChannelRenderInfo.size());

  uint32_t maxChOffset = 0;
  for (const auto &node : sdf.nodes)
    maxChOffset = std::max(maxChOffset, node.channels_edge.child_offset);

  for (const auto &channel : sdf.point_channels)
    add_channel(channel, CHANNEL_TYPE_VERTEX, INVALID_IDX);
  for (const auto &channel : sdf.voxel_channels)
    add_channel(channel, CHANNEL_TYPE_FACE, INVALID_IDX);

  bool only_tc = (sdf.header.tex_id_off == -1 && sdf.point_channels.size() == 0) ||
                  (sdf.header.tex_id_off >= 0  && sdf.point_channels.size() == 1);
  bool only_pc = (sdf.header.mat_id_off == -1 && sdf.voxel_channels.size() == 0) ||
                  (sdf.header.mat_id_off >= 0  && sdf.voxel_channels.size() == 1);
  bool is_CAE = !(only_tc && only_pc);
  if (is_CAE)
  {
    for (auto &ch : sdf.point_channels)
      m_CAEPointChannelsInfo.push_back(get_info(ch));
    for (auto &ch : sdf.voxel_channels)
      m_CAEVoxelChannelsInfo.push_back(get_info(ch));   
  }
#else
  printf("Support for augmented SDFs is disabled. Channel data will be ignored.\n");
#endif
}

void MultiRenderer::add_augmented_sdf_internal(const SCom2Tree& sdf, unsigned geomId)
{
#ifndef DISABLE_AUGMENTED_GEOMETRY
  if (sdf.header.mat_id_off >= 0)
  {
    assert(sdf.voxel_channels.size() > sdf.header.mat_id_off);
    assert(sdf.voxel_channels[sdf.header.mat_id_off].type == DataChannel::Type::INT);
    static_assert(sizeof(int) == sizeof(unsigned));
    const unsigned *matIds = (const unsigned *)sdf.voxel_channels[sdf.header.mat_id_off].data_i.data();
    unsigned matIdsCount = sdf.voxel_channels[sdf.header.mat_id_off].data_i.size();
    m_matIdOffsets.resize(geomId + 1, uint2(0,1));
    m_matIdOffsets[geomId] = uint2(m_matIdbyPrimId.size(), matIdsCount);
    m_matIdbyPrimId.insert(m_matIdbyPrimId.end(), matIds, matIds + matIdsCount);
  }
  
  m_allChannelOffsets.resize(geomId + 1, uint4(HAS_NO_CHANNELS, 0, 0, 0));
  m_allChannelOffsets[geomId] = uint4(m_allIntChannels.size(), 
                                      m_allFloatChannels.size(), 
                                      m_allCompressedChannels.size(), 
                                      m_allChannelRenderInfo.size());

  for (const auto &channel : sdf.point_channels)
    add_channel(channel, CHANNEL_TYPE_VERTEX, INVALID_IDX);
  for (const auto &channel : sdf.voxel_channels)
    add_channel(channel, CHANNEL_TYPE_FACE, INVALID_IDX);

  bool only_tc = (sdf.header.tex_id_off == -1 && sdf.point_channels.size() == 0) ||
                  (sdf.header.tex_id_off >= 0  && sdf.point_channels.size() == 1);
  bool only_pc = (sdf.header.mat_id_off == -1 && sdf.voxel_channels.size() == 0) ||
                  (sdf.header.mat_id_off >= 0  && sdf.voxel_channels.size() == 1);
  bool is_CAE = !(only_tc && only_pc);
  if (is_CAE)
  {
    for (auto &ch : sdf.point_channels)
      m_CAEPointChannelsInfo.push_back(get_info(ch));
    for (auto &ch : sdf.voxel_channels)
      m_CAEVoxelChannelsInfo.push_back(get_info(ch));   
  }
#else
  printf("Support for augmented SDFs is disabled. Channel data will be ignored.\n");
#endif
}

void MultiRenderer::add_augmented_mesh_internal(const cmesh4::AugmentedMesh &scene, uint32_t geomId)
{
  assert(scene.vertices.size() > 0);
  assert(scene.normals.size() == scene.vertices.size());
  assert(scene.indices.size() > 0);

  //set basic data, the same as for simple mesh
  if(geomId & CRT_GEOM_MASK_AABB_BIT) // TEMP SOLUTION !!!
    geomId = (geomId & 0x7fffffff);   // TEMP SOLUTION !!!

  m_geomOffsets.resize(geomId + 1, uint2(0,0));
  m_geomOffsets[geomId].x = m_indices.size();
  m_geomOffsets[geomId].y = m_vertices.size();
  m_indices.insert(m_indices.end(), scene.indices.begin(), scene.indices.end());

  unsigned sz = scene.vertices.size();
  m_vertices.resize(m_vertices.size() + sz);
  m_normals.resize(m_normals.size() + sz);

  //set texture coordinates to (0,0)
  //TODO: if there is tex_coords/tc/texture_coordinates channels, we should probably use it
  for (unsigned i = 0; i < sz; ++i)
  {
    m_vertices[m_geomOffsets[geomId].y + i] = to_float4(scene.vertices[i], 0.0f);
    m_normals[m_geomOffsets[geomId].y + i] = to_float4(scene.normals[i], 0.0f);
  }

  //add material if it was not explicitly set before
  if (geomId >= m_matIdOffsets.size())
  {
    m_matIdOffsets.resize(geomId + 1, uint2(0,1));
    
    //no material, set default
    //TODO: if there is material/mat_id channel, we should probably use it
    m_matIdbyPrimId.push_back(DEFAULT_MATERIAL);
    m_matIdOffsets[geomId] = uint2(m_matIdbyPrimId.size()-1, 1);
  }

//filling channels
#ifndef DISABLE_AUGMENTED_GEOMETRY
  m_allChannelOffsets.resize(geomId + 1, uint4(HAS_NO_CHANNELS, 0, 0, 0));
  m_allChannelOffsets[geomId] = uint4(m_allIntChannels.size(), 
                                      m_allFloatChannels.size(), 
                                      m_allCompressedChannels.size(), 
                                      m_allChannelRenderInfo.size());

  for (const auto &channel : scene.vertex_channels)
    add_channel(channel, CHANNEL_TYPE_VERTEX, scene.vertices.size());
  for (const auto &channel : scene.face_channels)
    add_channel(channel, CHANNEL_TYPE_FACE, scene.indices.size()/3);

  bool is_CAE = true;
  if (is_CAE)
  {
    for (auto &ch : scene.vertex_channels)
      m_CAEPointChannelsInfo.push_back(get_info(ch));
    for (auto &ch : scene.face_channels)
      m_CAEVoxelChannelsInfo.push_back(get_info(ch));   
  }
#else
  printf("Support for augmented meshes is disabled. Channel data will be ignored.\n");
#endif
}

void MultiRenderer::add_SdfFrameOctreeTex_internal(SdfFrameOctreeTexView scene, unsigned geomId)
{
  if(geomId & CRT_GEOM_MASK_AABB_BIT) // TEMP SOLUTION !!!
    geomId = (geomId & 0x7fffffff);   // TEMP SOLUTION !!!
  //add material if it was not explicitly set before
  if (geomId >= m_matIdOffsets.size())
  {
    m_matIdOffsets.resize(geomId + 1, uint2(0,1));
    unsigned start_idx = m_matIdbyPrimId.size();
    
    m_matIdbyPrimId.resize(start_idx + scene.size);
    for (unsigned i = 0; i < scene.size; ++i)
      m_matIdbyPrimId[start_idx + i] = scene.nodes[i].material_id;
    
    m_matIdOffsets[geomId] = uint2(start_idx, scene.size);
  }
}

uint32_t MultiRenderer::AddTexture(const Image2D<LiteMath::float4> &image)
{
  assert(active_textures_count < m_textures.size());
  std::shared_ptr<Image2D<LiteMath::float4>> pTexture1 = std::make_shared<Image2D<LiteMath::float4>>(image.width(), image.height(), image.data());
  Sampler sampler;
  sampler.filter   = Sampler::Filter::LINEAR; 
  sampler.addressU = Sampler::AddressMode::CLAMP;
  sampler.addressV = Sampler::AddressMode::CLAMP;
  m_textures[active_textures_count] = MakeCombinedTexture2D(pTexture1, sampler);
  active_textures_count++;

  return active_textures_count - 1;
}
uint32_t MultiRenderer::AddMaterial(const MultiRendererMaterial &material)
{
  if (m_sceneHasLoadedMaterials)
  {
    m_materials.push_back(material);
  }
  else
  {
    m_materials[0] = material;
    m_sceneHasLoadedMaterials = true;
  }
  return m_materials.size() - 1;
}
void MultiRenderer::SetMaterial(uint32_t matId, uint32_t geomId)
{
  m_matIdbyPrimId.push_back(matId);
  if (geomId >= m_matIdOffsets.size())
    m_matIdOffsets.resize(geomId + 1, uint2(0,1));
  m_matIdOffsets[geomId] = uint2(m_matIdbyPrimId.size()-1, 1);
}

void MultiRenderer::SetLights(const std::vector<Light>& lights)
{
  for (int i = 0; i < std::min(m_lights.size(), lights.size()); i++)
    m_lights[i] = lights[i];
  Update_m_lights(0, m_lights.size());
}

uint32_t MultiRenderer::AddInstance(uint32_t a_geomId, const LiteMath::float4x4& a_matrix)
{
  m_instanceTransformInvTransposed.push_back(transpose(inverse4x4(a_matrix)));
  return m_pAccelStruct->AddInstance(a_geomId, a_matrix);
}

void MultiRenderer::UpdateInstance(uint32_t a_instanceId, const float4x4 &a_matrix)
{
  if (a_instanceId < m_instanceTransformInvTransposed.size())
  {
    m_instanceTransformInvTransposed[a_instanceId] = transpose(inverse4x4(a_matrix));
    m_pAccelStruct->UpdateInstance(a_instanceId, a_matrix);
  }
}

bool MultiRenderer::LoadScene(LiteScene::HydraScene &scene)
{
  if (scene.geometries.size() == 0)
  {
    printf("[MultiRenderer::LoadScene] No models in scene\n");
    return false;
  }
  if (scene.cameras.size() > 0 && scene.cameras.find(0) == scene.cameras.end())
  {
    printf("[MultiRenderer::LoadScene] Default camera not found\n");
    return false;
  }
  if (scene.scenes.size() == 0)
  {
    printf("[MultiRenderer::LoadScene] No instanced scenes in HydraScene\n");
    return false;
  }
  if (scene.scenes.find(0) == scene.scenes.end())
  {
    printf("[MultiRenderer::LoadScene] Default instanced scene not found\n");
    return false;
  }
  if (scene.scenes[0].instances.size() == 0)
  {
    printf("[MultiRenderer::LoadScene] No instances in scene\n");
    return false;
  }

  BVHRT *bvhrt = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
  if (!bvhrt)
  {
    printf("[MultiRenderer::LoadScene] Only BVHRT currently support loading from HydraScene\n");
    return false;
  }

  GetAccelStruct()->ClearGeom();
  GetAccelStruct()->ClearScene();

  // load textures
  std::map<uint32_t, uint32_t> tex_remap;
  for (auto& tex_pair : scene.textures)
  {
    auto path = tex_pair.second.get_info().path;
    auto extension = split(path, '.').back();
    LiteImage::Image2D<float4> texture;
    if (extension == "jpg" || extension == "png" || extension == "jpeg" || extension == "bmp")
      texture = LiteImage::LoadImage<float4>(path.c_str());
    else if (extension == "image4ub")
      texture = LiteImage::Image2D<float4>(1, 1, float4(0,0,0,0)); //TODO: load image4ub
    else
    {
      printf("[MultiRenderer::LoadScene] Unsupported texture format: %s\n", extension.c_str());
      texture = LiteImage::Image2D<float4>(1, 1, float4(0,0,0,0));
    }
    uint32_t id = AddTexture(texture);
    tex_remap[tex_pair.first] = id;
  }

  // load materials
  std::map<uint32_t, uint32_t> mat_remap;
  mat_remap[(uint32_t)-1] = 0;
  for (auto& mat_pair : scene.materials)
  {
    MultiRendererMaterial m_mat = load_material(mat_pair.second, tex_remap);
    uint32_t matId = AddMaterial(m_mat);
    mat_remap[mat_pair.first] = matId;
  }

  std::map<uint32_t, uint32_t> isCustom;
  std::map<uint32_t, uint32_t> remap;

  //load geometries
  for (auto& geom_pair : scene.geometries)
  {
    bool isMesh = false;
    bool hasMatIdsInsideGeometry = false;
    unsigned geomId = INVALID_IDX;

    if (geom_pair.second->type_name == LiteScene::MeshGeometry::get_type_name())
    {
      cast_or_create(LiteScene::MeshGeometry, mesh, scene.metadata, geom_pair.second);
      cmesh4::fix_missing(mesh->mesh);
      cmesh4::recalculate_vertex_normals(mesh->mesh);

      for (auto &m : mesh->mesh.matIndices)
      {
        auto it = mat_remap.find(m);
        if (it == mat_remap.end()) // broken mat id
          m = 0;                   // set to default material
        else
          m = it->second;
      }

      geomId = m_pAccelStruct->AddGeom_Triangles3f((const float *)mesh->mesh.vPos4f.data(), mesh->mesh.vPos4f.size(),
                                                   mesh->mesh.indices.data(), mesh->mesh.indices.size(), BUILD_HIGH, 
                                                   sizeof(float) * 4);
      add_mesh_internal(mesh->mesh, geomId);
      hasMatIdsInsideGeometry = true;
      isMesh = true;
    }
    else if (geom_pair.second->type_name == LiteScene::AugmentedMeshGeometry::get_type_name())
    {
      cast_or_create(LiteScene::AugmentedMeshGeometry, a_mesh, scene.metadata, geom_pair.second);
      geomId = m_pAccelStruct->AddGeom_Triangles3f((const float *)a_mesh->model.vertices.data(), a_mesh->model.vertices.size(),
                                                   a_mesh->model.indices.data(), a_mesh->model.indices.size(), BUILD_HIGH, sizeof(float) * 3);
      add_augmented_mesh_internal(a_mesh->model, geomId); 
      hasMatIdsInsideGeometry = true;
      isMesh = true;
    }
    else if (geom_pair.second->type_name == LiteScene::SDFGridGeometry::get_type_name())
    {
      cast_or_create(LiteScene::SDFGridGeometry, grid, scene.metadata, geom_pair.second);
      geomId = bvhrt->AddGeom_SdfGrid(grid->model, m_pAccelStruct.get()) & GEOM_ID_MASK;
    }
    else if (geom_pair.second->type_name == LiteScene::SDFFrameOctreeGeometry::get_type_name())
    {
      cast_or_create(LiteScene::SDFFrameOctreeGeometry, frame_octree, scene.metadata, geom_pair.second);
      geomId = bvhrt->AddGeom_SdfFrameOctree(frame_octree->model, m_pAccelStruct.get()) & GEOM_ID_MASK;
    }
    else if (geom_pair.second->type_name == LiteScene::SDFSVSGeometry::get_type_name())
    {
      cast_or_create(LiteScene::SDFSVSGeometry, svs, scene.metadata, geom_pair.second);
      geomId = bvhrt->AddGeom_SdfSVS(svs->model, m_pAccelStruct.get()) & GEOM_ID_MASK;
    }
    else if (geom_pair.second->type_name == LiteScene::SDFSBSGeometry::get_type_name())
    {
      cast_or_create(LiteScene::SDFSBSGeometry, sbs, scene.metadata, geom_pair.second);
      geomId = bvhrt->AddGeom_SdfSBS(sbs->model, m_pAccelStruct.get()) & GEOM_ID_MASK;
    }
    else if (geom_pair.second->type_name == LiteScene::SDFCOctreeGeometry::get_type_name())
    {
      cast_or_create(LiteScene::SDFCOctreeGeometry, coctree, scene.metadata, geom_pair.second);
      geomId = bvhrt->AddGeom_COctreeV3(coctree->model, bvhrt->preferredBVHLevel, m_pAccelStruct.get()) & GEOM_ID_MASK;
    }
    else if (geom_pair.second->type_name == LiteScene::SDFMultiOctreeGeometry::get_type_name())
    {
      cast_or_create(LiteScene::SDFMultiOctreeGeometry, multi_octree, scene.metadata, geom_pair.second);
      geomId = bvhrt->AddGeom_SdfMultiOctree(multi_octree->model, m_pAccelStruct.get()) & GEOM_ID_MASK;
    }
    else if (geom_pair.second->type_name == LiteScene::SDFAugmentedMultiOctreeGeometry::get_type_name())
    {
      cast_or_create(LiteScene::SDFAugmentedMultiOctreeGeometry, multi_octree_a, scene.metadata, geom_pair.second);
      geomId = bvhrt->AddGeom_SdfMultiOctree(multi_octree_a->model.octree, m_pAccelStruct.get()) & GEOM_ID_MASK;
      add_augmented_sdf_internal(multi_octree_a->model, geomId);
    }
    else if (geom_pair.second->type_name == LiteScene::SDFDAGGeometry::get_type_name())
    {
      cast_or_create(LiteScene::SDFDAGGeometry, dag_a, scene.metadata, geom_pair.second);
      geomId = bvhrt->AddGeom_SdfDAG(dag_a->model, m_pAccelStruct.get()) & GEOM_ID_MASK;
      add_augmented_sdf_internal(dag_a->model, geomId);
      hasMatIdsInsideGeometry = dag_a->model.header.mat_id_off >= 0;
    }
    else if (geom_pair.second->type_name == LiteScene::SCom2Geometry::get_type_name())
    {
      cast_or_create(LiteScene::SCom2Geometry, scom2, scene.metadata, geom_pair.second);
      geomId = bvhrt->AddGeom_SCom2(scom2->model, m_pAccelStruct.get()) & GEOM_ID_MASK;
      add_augmented_sdf_internal(scom2->model, geomId);
      hasMatIdsInsideGeometry = scom2->model.header.mat_id_off >= 0;
    }
    else
    {
      printf("[MultiRenderer::LoadScene] Unknown geometry type %s\n", geom_pair.second->type_name.c_str());
      continue;
    }

    geomId = geomId & GEOM_ID_MASK;
    remap[geom_pair.first] = geomId;
    isCustom[geomId] = !isMesh;

    if (!hasMatIdsInsideGeometry)
    {
      uint32_t matId = geom_pair.second->custom_data.attribute(L"mat_id").as_uint(0);
      auto it = mat_remap.find(matId);
      if (it == mat_remap.end()) // broken mat id
        matId = 0;               // set to default material
      else
        matId = it->second;
      
      SetMaterial(matId, geomId);
    }
  }

  if (scene.cameras.size() > 0)
  {
    //set first camera
    auto &cam = scene.cameras[0];
    float aspect   = float(m_width) / float(m_height);
    auto proj      = perspectiveMatrix(cam.fov, aspect, cam.nearPlane, cam.farPlane);
    auto worldView = lookAt(float3(cam.pos), float3(cam.lookAt), float3(cam.up));
    UpdateCamera(worldView, proj);
  }
  else
  {
    //some default camera
    float aspect   = float(m_width) / float(m_height);
    auto proj      = perspectiveMatrix(60, aspect, 0.01f, 100.0f);
    auto worldView = lookAt(float3(0,0,3), float3(0,0,0), float3(0,1,0));
    UpdateCamera(worldView, proj);
  }

  //load instances
  for (auto& inst_pair : scene.scenes[0].instances)
  {
    int geomId = inst_pair.second.mesh_id;
    auto remap_it = remap.find(geomId);
    if (remap_it == remap.end())
      printf("[MultiRenderer::LoadScene] Instance remaps to unknown geometry with id=%u\n", geomId);
    else
      AddInstance(remap[geomId] + isCustom[geomId] * CRT_GEOM_MASK_AABB_BIT, inst_pair.second.matrix);
  }

  m_pAccelStruct->CommitScene();

  return true;
}

uint32_t MultiRenderer::AddColorTransferFunction(const std::vector<float4> &ctf)
{
#ifndef DISABLE_AUGMENTED_GEOMETRY
  assert(ctf.size() >= 2);

  m_allCTFs.emplace_back();
  m_allCTFs.back().sample_points_offset = m_allCTFPoints.size();
  m_allCTFs.back().sample_points_count = ctf.size();
  m_allCTFs.back().sample_steps = log2(ctf.size());

  m_allCTFPoints.insert(m_allCTFPoints.end(), ctf.begin(), ctf.end());

  return m_allCTFs.size()-1;
#endif
  return 0;
}

#if defined(USE_GPU)
  #include "vk_context.h"
  #include "MultiRenderer_slang_comp.h"
  using MultiRenderer_GPU = MultiRenderer_slang_comp;
  std::shared_ptr<MultiRenderer> CreateMultiRenderer_slang_comp(uint32_t AS_type, uint32_t maxPrimitives, 
                                                         vk_utils::VulkanContext a_ctx, size_t a_maxThreadsGenerated);
#endif
#if defined(USE_GPU_RQ)
  #include "MultiRenderer_slang_comp_rq.h"
  std::shared_ptr<MultiRenderer> CreateMultiRenderer_slang_comp_rq(uint32_t AS_type, uint32_t maxPrimitives, 
                                                            vk_utils::VulkanContext a_ctx, size_t a_maxThreadsGenerated);
#endif

std::shared_ptr<MultiRenderer> CreateMultiRenderer(uint32_t device, uint32_t AS_type, uint32_t maxPrimitives, 
                                                   uint32_t preferredDeviceId) 
{
  static bool downgrade_warning_required = true;

  #if defined(USE_GPU)
  vk_utils::VulkanContext context;
  if (device != DEVICE_CPU) {
  //create context with maximum supported features (i.e. with HW-RTX support even if we want comp shaders)
  std::vector<const char*> requiredExtensions;
  VkPhysicalDeviceFeatures2 deviceFeatures;
  #if defined(USE_GPU_RQ)
    deviceFeatures = MultiRenderer_slang_comp_rq::ListRequiredDeviceFeatures(requiredExtensions);
  #else
    deviceFeatures = MultiRenderer_GPU::ListRequiredDeviceFeatures(requiredExtensions);
  #endif

  //globalContextInit will initialize context on first use
  context = vk_utils::globalContextInit(requiredExtensions, false, preferredDeviceId, &deviceFeatures);
  }
#endif
  uint32_t preferred_device, actual_device;
  
  //choose preferred device
  if (device == DEVICE_GPU)
  {
    preferred_device = DEVICE_GPU_COMP;  
  }
  else
  {
    preferred_device = device;
  }

  //choose actual device
  //TODO: check for HW support, not only compilation flags
  if (preferred_device == DEVICE_GPU_RQ) //GPU_RQ -> GPU -> CPU
  {
    #if defined(USE_GPU_RQ)
      actual_device = DEVICE_GPU_RQ;
    #elif defined(USE_GPU)
      actual_device = DEVICE_GPU_COMP;
    #else
      actual_device = DEVICE_CPU;
    #endif
  }
  else if (preferred_device == DEVICE_GPU_COMP) //GPU -> CPU
  {
    #if defined(USE_GPU)
      actual_device = DEVICE_GPU_COMP;
    #else
      actual_device = DEVICE_CPU;
    #endif
  }
  else if (preferred_device == DEVICE_CPU)
  {
    actual_device = DEVICE_CPU;
  }
  else 
  {
    printf("CreateMultiRenderer::Unknown preferred render device %d\n", preferred_device);
    actual_device = DEVICE_CPU;
  }

  //issue warning only one time, no reason for more spam
  if (actual_device != preferred_device && downgrade_warning_required)
  {
    printf("CreateMultiRenderer::Downgrading render device from %s to %s\n", 
           RenderDevice_to_string(preferred_device).c_str(), RenderDevice_to_string(actual_device).c_str());
    downgrade_warning_required = false;
  }

  //create MultiRenderer according to actual device and AS_type
  if (actual_device == DEVICE_CPU)
  {
    return std::shared_ptr<MultiRenderer>(new MultiRenderer(AS_type, maxPrimitives));
  }
#if defined(USE_GPU)
  else if (actual_device == DEVICE_GPU_COMP)
  {
    if (AS_type == AS_TYPE_COMMON)
      return CreateMultiRenderer_slang_comp(AS_type, maxPrimitives, context, 256);
  }
#endif
#if defined(USE_GPU_RQ)
  else if (actual_device == DEVICE_GPU_RQ)
  {
    if (AS_type == AS_TYPE_COMMON)
      return CreateMultiRenderer_slang_comp_rq(AS_type, maxPrimitives, context, 256);
  }
#endif
  else
  {
    printf("CreateMultiRenderer::Unsupported combination of render device: %s and AS_type: %s\n", 
           RenderDevice_to_string(actual_device).c_str(), RenderASType_to_string(AS_type).c_str());
    return nullptr;
  }
  return nullptr;
}
std::string RenderDevice_to_string(unsigned /*enum RenderDevice*/ device)
{
  switch (device)
  {
  case DEVICE_CPU:
    return "CPU";
  case DEVICE_GPU:
    return "GPU";
  case DEVICE_GPU_COMP:
    return "GPU_COMP";
  case DEVICE_GPU_RQ:
    return "GPU_RQ";
  default:
    return "DEVICE_UNKNOWN";
  }
}

unsigned  RenderDevice_from_string(const std::string &name)
{
  if (name == "CPU")
    return DEVICE_CPU;
  else if (name == "GPU")
    return DEVICE_GPU;
  else if (name == "GPU_COMP")
    return DEVICE_GPU_COMP;
  else if (name == "GPU_RQ")
    return DEVICE_GPU_RQ;
  else
  {
    printf("Unknown render device %s\n", name.c_str());
    return DEVICE_CPU;
  }
}

std::string RenderASType_to_string(unsigned /*enum RenderASType*/ AS_type)
{
  switch (AS_type)
  {
  case AS_TYPE_COMMON:
    return "AS_TYPE_COMMON";
  default:
    return "AS_TYPE_UNKNOWN";
  }
}

unsigned  RenderASType_from_string(const std::string &name)
{
  if (name == "AS_TYPE_COMMON")
    return AS_TYPE_COMMON;
  else
  {
    printf("Unknown AS type %s\n", name.c_str());
    return AS_TYPE_COMMON;
  }
}
