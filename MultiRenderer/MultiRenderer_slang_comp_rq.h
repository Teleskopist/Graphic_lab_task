#pragma once

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <array>

#include "vk_pipeline.h"
#include "vk_buffers.h"
#include "vk_utils.h"
#include "vk_copy.h"
#include "vk_context.h"
#include "Image2d.h"
using LiteImage::Image2D;
using LiteImage::Sampler;
using namespace LiteMath;
#include "MultiRenderer.h"


/////////////////////////////////////////////////////////////////////////////////////////// UBO

#include "LiteMath.h"
#ifndef CUDA_MATH
using   LiteMath::uint;
typedef LiteMath::float4x4 mat4;
typedef LiteMath::float2   vec2;
typedef LiteMath::float3   vec3;
typedef LiteMath::float4   vec4;
typedef LiteMath::int2     ivec2;
typedef LiteMath::int3     ivec3;
typedef LiteMath::int4     ivec4;
typedef LiteMath::uint2    uvec2;
typedef LiteMath::uint3    uvec3;
typedef LiteMath::uint4    uvec4;
#else
//typedef float4x4 mat4;
typedef float2   vec2;
typedef float3   vec3;
typedef float4   vec4;
typedef int2     ivec2;
typedef int3     ivec3;
typedef int4     ivec4;
typedef uint2    uvec2;
typedef uint3    uvec3;
typedef uint4    uvec4;
#endif

struct MultiRenderer_slang_comp_rq_UBO_Data
{
  float4x4 m_projInv;
  float4x4 m_worldViewInv;
  Plane m_cuttingPlane;
  Plane m_pAccelStruct_m_cuttingPlane;
  float4 m_backgroundColor;
  COctreeV3Header m_pAccelStruct_coctree_v3_header;
  MultiRenderPreset m_pAccelStruct_m_preset;
  MultiRenderPreset m_preset;
  HighlightPreset m_highlight;
  uint m_AAFrameNum;
  uint m_height;
  uint m_pAccelStruct_m_SdfDAGBrickTranspositionOffset;
  uint m_pAccelStruct_m_SdfDAGTransformCount;
  uint m_packedXY_height;
  uint m_packedXY_width;
  uint m_width;
  uint m_allCTFPoints_capacity;
  uint m_allCTFPoints_size;
  uint m_allCTFs_capacity;
  uint m_allCTFs_size;
  uint m_allChannelOffsets_capacity;
  uint m_allChannelOffsets_size;
  uint m_allChannelRenderInfo_capacity;
  uint m_allChannelRenderInfo_size;
  uint m_allCompressedChannels_capacity;
  uint m_allCompressedChannels_size;
  uint m_allFloatChannels_capacity;
  uint m_allFloatChannels_size;
  uint m_allIntChannels_capacity;
  uint m_allIntChannels_size;
  uint m_colorBuffer_capacity;
  uint m_colorBuffer_size;
  uint m_gBuffer_capacity;
  uint m_gBuffer_size;
  uint m_geomOffsets_capacity;
  uint m_geomOffsets_size;
  uint m_indices_capacity;
  uint m_indices_size;
  uint m_instanceTransformInvTransposed_capacity;
  uint m_instanceTransformInvTransposed_size;
  uint m_lights_capacity;
  uint m_lights_size;
  uint m_matIdOffsets_capacity;
  uint m_matIdOffsets_size;
  uint m_matIdbyPrimId_capacity;
  uint m_matIdbyPrimId_size;
  uint m_materials_capacity;
  uint m_materials_size;
  uint m_normals_capacity;
  uint m_normals_size;
  uint m_pAccelStruct_capacity;
  uint m_pAccelStruct_m_GraphicsPrimHeaders_capacity;
  uint m_pAccelStruct_m_GraphicsPrimHeaders_size;
  uint m_pAccelStruct_m_GraphicsPrimPoints_capacity;
  uint m_pAccelStruct_m_GraphicsPrimPoints_size;
  uint m_pAccelStruct_m_RotAddTable_capacity;
  uint m_pAccelStruct_m_RotAddTable_size;
  uint m_pAccelStruct_m_SCom2Data_capacity;
  uint m_pAccelStruct_m_SCom2Data_size;
  uint m_pAccelStruct_m_SCom2Headers_capacity;
  uint m_pAccelStruct_m_SCom2Headers_size;
  uint m_pAccelStruct_m_SdfCompactOctreeRotModifiers_capacity;
  uint m_pAccelStruct_m_SdfCompactOctreeRotModifiers_size;
  uint m_pAccelStruct_m_SdfCompactOctreeRotTransforms_capacity;
  uint m_pAccelStruct_m_SdfCompactOctreeRotTransforms_size;
  uint m_pAccelStruct_m_SdfCompactOctreeV3Data_capacity;
  uint m_pAccelStruct_m_SdfCompactOctreeV3Data_size;
  uint m_pAccelStruct_m_SdfDAGChildEdges_capacity;
  uint m_pAccelStruct_m_SdfDAGChildEdges_size;
  uint m_pAccelStruct_m_SdfDAGDataEdges_capacity;
  uint m_pAccelStruct_m_SdfDAGDataEdges_size;
  uint m_pAccelStruct_m_SdfDAGDistances_capacity;
  uint m_pAccelStruct_m_SdfDAGDistances_size;
  uint m_pAccelStruct_m_SdfDAGHeaders_capacity;
  uint m_pAccelStruct_m_SdfDAGHeaders_size;
  uint m_pAccelStruct_m_SdfDAGNodes_capacity;
  uint m_pAccelStruct_m_SdfDAGNodes_size;
  uint m_pAccelStruct_m_SdfDAGTranspositions_capacity;
  uint m_pAccelStruct_m_SdfDAGTranspositions_size;
  uint m_pAccelStruct_m_SdfFrameOctreeNodes_capacity;
  uint m_pAccelStruct_m_SdfFrameOctreeNodes_size;
  uint m_pAccelStruct_m_SdfFrameOctreeRoots_capacity;
  uint m_pAccelStruct_m_SdfFrameOctreeRoots_size;
  uint m_pAccelStruct_m_SdfFrameOctreeTexNodes_capacity;
  uint m_pAccelStruct_m_SdfFrameOctreeTexNodes_size;
  uint m_pAccelStruct_m_SdfFrameOctreeTexRoots_capacity;
  uint m_pAccelStruct_m_SdfFrameOctreeTexRoots_size;
  uint m_pAccelStruct_m_SdfGridData_capacity;
  uint m_pAccelStruct_m_SdfGridData_size;
  uint m_pAccelStruct_m_SdfGridOffsets_capacity;
  uint m_pAccelStruct_m_SdfGridOffsets_size;
  uint m_pAccelStruct_m_SdfGridSizes_capacity;
  uint m_pAccelStruct_m_SdfGridSizes_size;
  uint m_pAccelStruct_m_SdfMultiOctreeNodes_capacity;
  uint m_pAccelStruct_m_SdfMultiOctreeNodes_size;
  uint m_pAccelStruct_m_SdfMultiOctreeValues_capacity;
  uint m_pAccelStruct_m_SdfMultiOctreeValues_size;
  uint m_pAccelStruct_m_SdfSBSDataF_capacity;
  uint m_pAccelStruct_m_SdfSBSDataF_size;
  uint m_pAccelStruct_m_SdfSBSData_capacity;
  uint m_pAccelStruct_m_SdfSBSData_size;
  uint m_pAccelStruct_m_SdfSBSHeaders_capacity;
  uint m_pAccelStruct_m_SdfSBSHeaders_size;
  uint m_pAccelStruct_m_SdfSBSNodes_capacity;
  uint m_pAccelStruct_m_SdfSBSNodes_size;
  uint m_pAccelStruct_m_SdfSBSRoots_capacity;
  uint m_pAccelStruct_m_SdfSBSRoots_size;
  uint m_pAccelStruct_m_SdfSVSNodes_capacity;
  uint m_pAccelStruct_m_SdfSVSNodes_size;
  uint m_pAccelStruct_m_SdfSVSRoots_capacity;
  uint m_pAccelStruct_m_SdfSVSRoots_size;
  uint m_pAccelStruct_m_allNodePairs_capacity;
  uint m_pAccelStruct_m_allNodePairs_size;
  uint m_pAccelStruct_m_geomData_capacity;
  uint m_pAccelStruct_m_geomData_size;
  uint m_pAccelStruct_m_geomTags_capacity;
  uint m_pAccelStruct_m_geomTags_size;
  uint m_pAccelStruct_m_indices_capacity;
  uint m_pAccelStruct_m_indices_size;
  uint m_pAccelStruct_m_instanceData_capacity;
  uint m_pAccelStruct_m_instanceData_size;
  uint m_pAccelStruct_m_nodesTLAS_capacity;
  uint m_pAccelStruct_m_nodesTLAS_size;
  uint m_pAccelStruct_m_origNodes_capacity;
  uint m_pAccelStruct_m_origNodes_size;
  uint m_pAccelStruct_m_primIdCount_capacity;
  uint m_pAccelStruct_m_primIdCount_size;
  uint m_pAccelStruct_m_primIndices_capacity;
  uint m_pAccelStruct_m_primIndices_size;
  uint m_pAccelStruct_m_vertNorm_capacity;
  uint m_pAccelStruct_m_vertNorm_size;
  uint m_pAccelStruct_m_vertPos_capacity;
  uint m_pAccelStruct_m_vertPos_size;
  uint m_pAccelStruct_remap_capacity;
  uint m_pAccelStruct_remap_size;
  uint m_pAccelStruct_size;
  uint m_pAccelStruct_startEnd_capacity;
  uint m_pAccelStruct_startEnd_size;
  uint m_packedXY_capacity;
  uint m_packedXY_size;
  uint m_textures_capacity;
  uint m_textures_size;
  uint m_transparencyBuffer_capacity;
  uint m_transparencyBuffer_size;
  uint m_vertices_capacity;
  uint m_vertices_size;
  uint timeDataByName_capacity;
  uint timeDataByName_size;
  bool m_pAccelStruct_support_fast_scom_traversal;
  bool m_sceneHasTransparency;
  uint dummy_last;
};
/////////////////////////////////////////////////////////////////////////////////////////// UBO
class MultiRenderer_slang_comp_rq : public MultiRenderer
{
public:

  MultiRenderer_slang_comp_rq(uint32_t AS_type, uint32_t maxPrimitives) : MultiRenderer(AS_type, maxPrimitives)
  {
    if(m_pAccelStruct == nullptr)
      m_pAccelStruct = std::make_shared<BVHRT>();
  }
  const char* Name() const override { return "MultiRenderer_slang_comp_rq";}
  virtual void InitVulkanObjects(VkDevice a_device, VkPhysicalDevice a_physicalDevice, size_t a_maxThreadsCount);

  virtual void SetVulkanContext(vk_utils::VulkanContext a_ctx) { m_ctx = a_ctx; }
  virtual void SetVulkanInOutFor_RenderFloat(
    VkBuffer a_outColorBuffer,
    size_t   a_outColorOffset,
    uint32_t dummyArgument = 0)
  {
    RenderFloat_local.a_outColorBuffer = a_outColorBuffer;
    RenderFloat_local.a_outColorOffset = a_outColorOffset;
    UpdateAllGeneratedDescriptorSets_RenderFloat();
  }

  virtual void SetVulkanInOutFor_Render(
    VkBuffer a_outColorBuffer,
    size_t   a_outColorOffset,
    uint32_t dummyArgument = 0)
  {
    Render_local.a_outColorBuffer = a_outColorBuffer;
    Render_local.a_outColorOffset = a_outColorOffset;
    UpdateAllGeneratedDescriptorSets_Render();
  }

  virtual void SetVulkanInOutFor_Clear(
    uint32_t dummyArgument = 0)
  {
    UpdateAllGeneratedDescriptorSets_Clear();
  }

  virtual ~MultiRenderer_slang_comp_rq();


virtual void InitDeviceData() ;
virtual void UpdateDeviceData(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine)   {
    UpdateVectorMembers(a_pCopyEngine);
    UpdateTextureMembers(a_pCopyEngine);
    UpdatePlainMembers(a_pCopyEngine);
  }
  void Update_m_lights(size_t a_first, size_t a_size) override;

  virtual void UpdatePrefixPointers()
  {
    auto pUnderlyingImpl = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
    if(pUnderlyingImpl != nullptr)
    {
      m_pAccelStruct_m_GraphicsPrimHeaders = &pUnderlyingImpl->m_GraphicsPrimHeaders;
      m_pAccelStruct_m_GraphicsPrimPoints = &pUnderlyingImpl->m_GraphicsPrimPoints;
      m_pAccelStruct_m_RotAddTable = &pUnderlyingImpl->m_RotAddTable;
      m_pAccelStruct_m_SCom2Data = &pUnderlyingImpl->m_SCom2Data;
      m_pAccelStruct_m_SCom2Headers = &pUnderlyingImpl->m_SCom2Headers;
      m_pAccelStruct_m_SdfCompactOctreeRotModifiers = &pUnderlyingImpl->m_SdfCompactOctreeRotModifiers;
      m_pAccelStruct_m_SdfCompactOctreeRotTransforms = &pUnderlyingImpl->m_SdfCompactOctreeRotTransforms;
      m_pAccelStruct_m_SdfCompactOctreeV3Data = &pUnderlyingImpl->m_SdfCompactOctreeV3Data;
      m_pAccelStruct_m_SdfDAGChildEdges = &pUnderlyingImpl->m_SdfDAGChildEdges;
      m_pAccelStruct_m_SdfDAGDataEdges = &pUnderlyingImpl->m_SdfDAGDataEdges;
      m_pAccelStruct_m_SdfDAGDistances = &pUnderlyingImpl->m_SdfDAGDistances;
      m_pAccelStruct_m_SdfDAGHeaders = &pUnderlyingImpl->m_SdfDAGHeaders;
      m_pAccelStruct_m_SdfDAGNodes = &pUnderlyingImpl->m_SdfDAGNodes;
      m_pAccelStruct_m_SdfDAGTranspositions = &pUnderlyingImpl->m_SdfDAGTranspositions;
      m_pAccelStruct_m_SdfFrameOctreeNodes = &pUnderlyingImpl->m_SdfFrameOctreeNodes;
      m_pAccelStruct_m_SdfFrameOctreeRoots = &pUnderlyingImpl->m_SdfFrameOctreeRoots;
      m_pAccelStruct_m_SdfFrameOctreeTexNodes = &pUnderlyingImpl->m_SdfFrameOctreeTexNodes;
      m_pAccelStruct_m_SdfFrameOctreeTexRoots = &pUnderlyingImpl->m_SdfFrameOctreeTexRoots;
      m_pAccelStruct_m_SdfGridData = &pUnderlyingImpl->m_SdfGridData;
      m_pAccelStruct_m_SdfGridOffsets = &pUnderlyingImpl->m_SdfGridOffsets;
      m_pAccelStruct_m_SdfGridSizes = &pUnderlyingImpl->m_SdfGridSizes;
      m_pAccelStruct_m_SdfMultiOctreeNodes = &pUnderlyingImpl->m_SdfMultiOctreeNodes;
      m_pAccelStruct_m_SdfMultiOctreeValues = &pUnderlyingImpl->m_SdfMultiOctreeValues;
      m_pAccelStruct_m_SdfSBSData = &pUnderlyingImpl->m_SdfSBSData;
      m_pAccelStruct_m_SdfSBSDataF = &pUnderlyingImpl->m_SdfSBSDataF;
      m_pAccelStruct_m_SdfSBSHeaders = &pUnderlyingImpl->m_SdfSBSHeaders;
      m_pAccelStruct_m_SdfSBSNodes = &pUnderlyingImpl->m_SdfSBSNodes;
      m_pAccelStruct_m_SdfSBSRoots = &pUnderlyingImpl->m_SdfSBSRoots;
      m_pAccelStruct_m_SdfSVSNodes = &pUnderlyingImpl->m_SdfSVSNodes;
      m_pAccelStruct_m_SdfSVSRoots = &pUnderlyingImpl->m_SdfSVSRoots;
      m_pAccelStruct_m_allNodePairs = &pUnderlyingImpl->m_allNodePairs;
      m_pAccelStruct_m_geomData = &pUnderlyingImpl->m_geomData;
      m_pAccelStruct_m_geomTags = &pUnderlyingImpl->m_geomTags;
      m_pAccelStruct_m_indices = &pUnderlyingImpl->m_indices;
      m_pAccelStruct_m_instanceData = &pUnderlyingImpl->m_instanceData;
      m_pAccelStruct_m_nodesTLAS = &pUnderlyingImpl->m_nodesTLAS;
      m_pAccelStruct_m_origNodes = &pUnderlyingImpl->m_origNodes;
      m_pAccelStruct_m_primIdCount = &pUnderlyingImpl->m_primIdCount;
      m_pAccelStruct_m_primIndices = &pUnderlyingImpl->m_primIndices;
      m_pAccelStruct_m_vertNorm = &pUnderlyingImpl->m_vertNorm;
      m_pAccelStruct_m_vertPos = &pUnderlyingImpl->m_vertPos;
      m_pAccelStruct_startEnd = &pUnderlyingImpl->startEnd;
    }
  }
  std::shared_ptr<vk_utils::ICopyEngine> m_pLastCopyHelper = nullptr;
  virtual void DeleteDeviceData();
  virtual void CommitDeviceData(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyHelper) // you have to define this virtual function in the original imput class
  {
    UpdatePrefixPointers();
    ReserveEmptyVectors();
    DeleteDeviceData();
    InitDeviceData();
    UpdateDeviceData(a_pCopyHelper);
    m_pLastCopyHelper = a_pCopyHelper;
    m_commitCount++;
  }
  void CommitDeviceData() override { CommitDeviceData(m_ctx.pCopyHelper); }
  void GetExecutionTime(const char* a_funcName, float a_out[4]) override;
  void UpdateMembersPlainData() override { UpdatePlainMembers(m_ctx.pCopyHelper); }
  
  virtual void ReserveEmptyVectors();
  virtual void UpdatePlainMembers(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine);
  virtual void UpdateVectorMembers(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine);
  virtual void UpdateTextureMembers(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine);
  virtual void ReadPlainMembers(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine);
  static VkPhysicalDeviceFeatures2 ListRequiredDeviceFeatures(std::vector<const char*>& deviceExtensions);

  virtual void RenderFloatCmd(VkCommandBuffer a_commandBuffer, float4* a_outColor, uint32_t a_width, uint32_t a_height, int a_passNum);
  virtual void RenderCmd(VkCommandBuffer a_commandBuffer, uint32_t* a_outColor, uint32_t a_width, uint32_t a_height, int a_passNum);
  virtual void ClearCmd(VkCommandBuffer a_commandBuffer, uint32_t a_width, uint32_t a_height);

  void RenderFloat(float4* a_outColor, uint32_t a_width, uint32_t a_height, int a_passNum) override;
  void Render(uint32_t* a_outColor, uint32_t a_width, uint32_t a_height, int a_passNum) override;
  void Clear(uint32_t a_width, uint32_t a_height) override;

  inline vk_utils::ExecTime GetRenderFloatExecutionTime() const { return m_exTimeRenderFloat; }
  inline vk_utils::ExecTime GetRenderExecutionTime() const { return m_exTimeRender; }
  inline vk_utils::ExecTime GetClearExecutionTime() const { return m_exTimeClear; }

  vk_utils::ExecTime m_exTimeRenderFloat;
  vk_utils::ExecTime m_exTimeRender;
  vk_utils::ExecTime m_exTimeClear;

  virtual void copyKernelFloatCmd(uint32_t length);
  virtual void matMulTransposeCmd(uint32_t A_offset, uint32_t B_offset, uint32_t C_offset, uint32_t A_col_len, uint32_t B_col_len, uint32_t A_row_len);

  virtual void ResolveDebugCmd(uint32_t count, uint32_t sample_id, uint32_t render_mode, LiteMath::float4* out_color);
  virtual void FillGbufferCmd(uint32_t count, uint32_t sample_id);
  virtual void FillGbufferWithTransparencyCmd(uint32_t count, uint32_t sample_id);
  virtual void ResolveOutlineCmd(uint32_t count, uint32_t sample_id, uint32_t render_mode, float4* out_color);
  virtual void AverageColorCmd(uint32_t count, uint32_t sample_count, float4* out_color);
  virtual void TonemapCmd(uint32_t count, uint32_t sample_count, uint32_t* out_color);
  virtual void PackXYCmd(uint32_t w, uint32_t h);
  virtual void ResolveCommonCmd(uint32_t count, uint32_t sample_id, uint32_t render_mode, LiteMath::float4* out_color);
  virtual void ResolveCommonWithTransparencyCmd(uint32_t count, uint32_t sample_id, uint32_t render_mode, LiteMath::float4* out_color);
  virtual void ResolveCTFCmd(uint32_t count, uint32_t sample_id, uint32_t render_mode, LiteMath::float4* out_color);

  struct MemLoc
  {
    VkDeviceMemory memObject = VK_NULL_HANDLE;
    size_t         memOffset = 0;
    size_t         allocId   = 0;
  };

  virtual MemLoc AllocAndBind(const std::vector<VkBuffer>& a_buffers, VkMemoryAllocateFlags a_flags = 0); ///< replace this function to apply custom allocator
  virtual MemLoc AllocAndBind(const std::vector<VkImage>& a_image,    VkMemoryAllocateFlags a_flags = 0);    ///< replace this function to apply custom allocator
  virtual void   FreeAllAllocations(std::vector<MemLoc>& a_memLoc);    ///< replace this function to apply custom allocator

protected:

  VkPhysicalDevice           m_physicalDevice = VK_NULL_HANDLE;
  VkDevice                   m_device         = VK_NULL_HANDLE;
  vk_utils::VulkanContext    m_ctx          = {};
  VkCommandBuffer            m_currCmdBuffer   = VK_NULL_HANDLE;
  uint32_t                   m_currThreadFlags = 0;
  std::vector<MemLoc>        m_allMems;
  VkPhysicalDeviceProperties m_devProps;
  size_t                     m_commitCount = 0;

  VkBufferMemoryBarrier BarrierForClearFlags(VkBuffer a_buffer);
  VkBufferMemoryBarrier BarrierForSingleBuffer(VkBuffer a_buffer);
  void BarriersForSeveralBuffers(VkBuffer* a_inBuffers, VkBufferMemoryBarrier* a_outBarriers, uint32_t a_buffersNum);

  virtual void InitHelpers();
  virtual void InitBuffers(size_t a_maxThreadsCount, bool a_tempBuffersOverlay = true);
  virtual void InitKernels(const char* a_filePath);
  virtual void AllocateAllDescriptorSets();

  virtual void UpdateAllGeneratedDescriptorSets_RenderFloat();
  virtual void UpdateAllGeneratedDescriptorSets_Render();
  virtual void UpdateAllGeneratedDescriptorSets_Clear();

  virtual void AssignBuffersToMemory(const std::vector<VkBuffer>& a_buffers, VkDeviceMemory a_mem);

  virtual void AllocMemoryForMemberBuffersAndImages(const std::vector<VkBuffer>& a_buffers, const std::vector<VkImage>& a_image);
  virtual std::string AlterShaderPath(const char* in_shaderPath) { return std::string("MultiRenderer/") + std::string(in_shaderPath); }
  
  

  struct RenderFloat_Data
  {
    VkBuffer a_outColorBuffer = VK_NULL_HANDLE;
    size_t   a_outColorOffset = 0;
    bool needToClearOutput = false;
  } RenderFloat_local;

  struct Render_Data
  {
    VkBuffer a_outColorBuffer = VK_NULL_HANDLE;
    size_t   a_outColorOffset = 0;
    bool needToClearOutput = false;
  } Render_local;

  struct Clear_Data
  {
    bool needToClearOutput = false;
  } Clear_local;



  struct MembersDataGPU
  {
    VkBuffer m_pAccelStruct_remapBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_remapOffset = 0;
    VkBuffer m_allCTFPointsBuffer = VK_NULL_HANDLE;
    size_t   m_allCTFPointsOffset = 0;
    VkBuffer m_allCTFsBuffer = VK_NULL_HANDLE;
    size_t   m_allCTFsOffset = 0;
    VkBuffer m_allChannelOffsetsBuffer = VK_NULL_HANDLE;
    size_t   m_allChannelOffsetsOffset = 0;
    VkBuffer m_allChannelRenderInfoBuffer = VK_NULL_HANDLE;
    size_t   m_allChannelRenderInfoOffset = 0;
    VkBuffer m_allCompressedChannelsBuffer = VK_NULL_HANDLE;
    size_t   m_allCompressedChannelsOffset = 0;
    VkBuffer m_allFloatChannelsBuffer = VK_NULL_HANDLE;
    size_t   m_allFloatChannelsOffset = 0;
    VkBuffer m_allIntChannelsBuffer = VK_NULL_HANDLE;
    size_t   m_allIntChannelsOffset = 0;
    VkBuffer m_colorBufferBuffer = VK_NULL_HANDLE;
    size_t   m_colorBufferOffset = 0;
    VkBuffer m_gBufferBuffer = VK_NULL_HANDLE;
    size_t   m_gBufferOffset = 0;
    VkBuffer m_geomOffsetsBuffer = VK_NULL_HANDLE;
    size_t   m_geomOffsetsOffset = 0;
    VkBuffer m_indicesBuffer = VK_NULL_HANDLE;
    size_t   m_indicesOffset = 0;
    VkBuffer m_instanceTransformInvTransposedBuffer = VK_NULL_HANDLE;
    size_t   m_instanceTransformInvTransposedOffset = 0;
    VkBuffer m_lightsBuffer = VK_NULL_HANDLE;
    size_t   m_lightsOffset = 0;
    VkBuffer m_matIdOffsetsBuffer = VK_NULL_HANDLE;
    size_t   m_matIdOffsetsOffset = 0;
    VkBuffer m_matIdbyPrimIdBuffer = VK_NULL_HANDLE;
    size_t   m_matIdbyPrimIdOffset = 0;
    VkBuffer m_materialsBuffer = VK_NULL_HANDLE;
    size_t   m_materialsOffset = 0;
    VkBuffer m_normalsBuffer = VK_NULL_HANDLE;
    size_t   m_normalsOffset = 0;
    VkBuffer m_pAccelStruct_m_GraphicsPrimHeadersBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_GraphicsPrimHeadersOffset = 0;
    VkBuffer m_pAccelStruct_m_GraphicsPrimPointsBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_GraphicsPrimPointsOffset = 0;
    VkBuffer m_pAccelStruct_m_RotAddTableBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_RotAddTableOffset = 0;
    VkBuffer m_pAccelStruct_m_SCom2DataBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SCom2DataOffset = 0;
    VkBuffer m_pAccelStruct_m_SCom2HeadersBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SCom2HeadersOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfCompactOctreeRotModifiersBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfCompactOctreeRotModifiersOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfCompactOctreeRotTransformsBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfCompactOctreeRotTransformsOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfCompactOctreeV3DataBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfCompactOctreeV3DataOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfDAGChildEdgesBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfDAGChildEdgesOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfDAGDataEdgesBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfDAGDataEdgesOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfDAGDistancesBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfDAGDistancesOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfDAGHeadersBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfDAGHeadersOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfDAGNodesBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfDAGNodesOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfDAGTranspositionsBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfDAGTranspositionsOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfFrameOctreeNodesBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfFrameOctreeNodesOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfFrameOctreeRootsBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfFrameOctreeRootsOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfFrameOctreeTexNodesBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfFrameOctreeTexNodesOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfFrameOctreeTexRootsBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfFrameOctreeTexRootsOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfGridDataBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfGridDataOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfGridOffsetsBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfGridOffsetsOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfGridSizesBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfGridSizesOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfMultiOctreeNodesBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfMultiOctreeNodesOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfMultiOctreeValuesBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfMultiOctreeValuesOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfSBSDataBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfSBSDataOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfSBSDataFBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfSBSDataFOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfSBSHeadersBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfSBSHeadersOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfSBSNodesBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfSBSNodesOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfSBSRootsBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfSBSRootsOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfSVSNodesBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfSVSNodesOffset = 0;
    VkBuffer m_pAccelStruct_m_SdfSVSRootsBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_SdfSVSRootsOffset = 0;
    VkBuffer m_pAccelStruct_m_allNodePairsBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_allNodePairsOffset = 0;
    VkBuffer m_pAccelStruct_m_geomDataBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_geomDataOffset = 0;
    VkBuffer m_pAccelStruct_m_geomTagsBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_geomTagsOffset = 0;
    VkBuffer m_pAccelStruct_m_indicesBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_indicesOffset = 0;
    VkBuffer m_pAccelStruct_m_instanceDataBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_instanceDataOffset = 0;
    VkBuffer m_pAccelStruct_m_nodesTLASBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_nodesTLASOffset = 0;
    VkBuffer m_pAccelStruct_m_origNodesBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_origNodesOffset = 0;
    VkBuffer m_pAccelStruct_m_primIdCountBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_primIdCountOffset = 0;
    VkBuffer m_pAccelStruct_m_primIndicesBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_primIndicesOffset = 0;
    VkBuffer m_pAccelStruct_m_vertNormBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_vertNormOffset = 0;
    VkBuffer m_pAccelStruct_m_vertPosBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_m_vertPosOffset = 0;
    VkBuffer m_pAccelStruct_startEndBuffer = VK_NULL_HANDLE;
    size_t   m_pAccelStruct_startEndOffset = 0;
    VkBuffer m_packedXYBuffer = VK_NULL_HANDLE;
    size_t   m_packedXYOffset = 0;
    VkBuffer m_transparencyBufferBuffer = VK_NULL_HANDLE;
    size_t   m_transparencyBufferOffset = 0;
    VkBuffer m_verticesBuffer = VK_NULL_HANDLE;
    size_t   m_verticesOffset = 0;
    std::vector<VkImage>     m_texturesArrayTexture;
    std::vector<VkImageView> m_texturesArrayView   ;
    std::vector<VkSampler>   m_texturesArraySampler; ///<! samplers for texture arrays are always used
    size_t                   m_texturesArrayMaxSize; ///<! used when texture array size is not known after constructor of base class is finished
  } m_vdata;
  std::vector<uint2> m_pAccelStruct_remap;

  std::vector<GraphicsPrimHeader>* m_pAccelStruct_m_GraphicsPrimHeaders = nullptr;
  std::vector<float4>* m_pAccelStruct_m_GraphicsPrimPoints = nullptr;
  std::vector<uint32_t>* m_pAccelStruct_m_RotAddTable = nullptr;
  std::vector<uint32_t>* m_pAccelStruct_m_SCom2Data = nullptr;
  std::vector<SCom2Header>* m_pAccelStruct_m_SCom2Headers = nullptr;
  std::vector<int4>* m_pAccelStruct_m_SdfCompactOctreeRotModifiers = nullptr;
  std::vector<float4x4>* m_pAccelStruct_m_SdfCompactOctreeRotTransforms = nullptr;
  std::vector<uint32_t>* m_pAccelStruct_m_SdfCompactOctreeV3Data = nullptr;
  std::vector<SdfDAGChildEdge>* m_pAccelStruct_m_SdfDAGChildEdges = nullptr;
  std::vector<SdfDAGDataEdge>* m_pAccelStruct_m_SdfDAGDataEdges = nullptr;
  std::vector<float>* m_pAccelStruct_m_SdfDAGDistances = nullptr;
  std::vector<SdfDAGHeader>* m_pAccelStruct_m_SdfDAGHeaders = nullptr;
  std::vector<SdfDAGNode>* m_pAccelStruct_m_SdfDAGNodes = nullptr;
  std::vector<uint32_t>* m_pAccelStruct_m_SdfDAGTranspositions = nullptr;
  std::vector<SdfFrameOctreeNode>* m_pAccelStruct_m_SdfFrameOctreeNodes = nullptr;
  std::vector<uint32_t>* m_pAccelStruct_m_SdfFrameOctreeRoots = nullptr;
  std::vector<SdfFrameOctreeTexNode>* m_pAccelStruct_m_SdfFrameOctreeTexNodes = nullptr;
  std::vector<uint32_t>* m_pAccelStruct_m_SdfFrameOctreeTexRoots = nullptr;
  std::vector<float>* m_pAccelStruct_m_SdfGridData = nullptr;
  std::vector<uint32_t>* m_pAccelStruct_m_SdfGridOffsets = nullptr;
  std::vector<uint3>* m_pAccelStruct_m_SdfGridSizes = nullptr;
  std::vector<SdfMultiOctreeNode>* m_pAccelStruct_m_SdfMultiOctreeNodes = nullptr;
  std::vector<float>* m_pAccelStruct_m_SdfMultiOctreeValues = nullptr;
  std::vector<uint32_t>* m_pAccelStruct_m_SdfSBSData = nullptr;
  std::vector<float>* m_pAccelStruct_m_SdfSBSDataF = nullptr;
  std::vector<SdfSBSHeader>* m_pAccelStruct_m_SdfSBSHeaders = nullptr;
  std::vector<SdfSBSNode>* m_pAccelStruct_m_SdfSBSNodes = nullptr;
  std::vector<uint32_t>* m_pAccelStruct_m_SdfSBSRoots = nullptr;
  std::vector<SdfSVSNode>* m_pAccelStruct_m_SdfSVSNodes = nullptr;
  std::vector<uint32_t>* m_pAccelStruct_m_SdfSVSRoots = nullptr;
  std::vector<BVHNodePair>* m_pAccelStruct_m_allNodePairs = nullptr;
  std::vector<GeomData>* m_pAccelStruct_m_geomData = nullptr;
  std::vector<uint>* m_pAccelStruct_m_geomTags = nullptr;
  std::vector<uint32_t>* m_pAccelStruct_m_indices = nullptr;
  std::vector<InstanceData>* m_pAccelStruct_m_instanceData = nullptr;
  std::vector<BVHNode>* m_pAccelStruct_m_nodesTLAS = nullptr;
  std::vector<BVHNode>* m_pAccelStruct_m_origNodes = nullptr;
  std::vector<uint32_t>* m_pAccelStruct_m_primIdCount = nullptr;
  std::vector<uint32_t>* m_pAccelStruct_m_primIndices = nullptr;
  std::vector<float4>* m_pAccelStruct_m_vertNorm = nullptr;
  std::vector<float4>* m_pAccelStruct_m_vertPos = nullptr;
  std::vector<uint2>* m_pAccelStruct_startEnd = nullptr;

  VkImage     CreateTexture2D(const int a_width, const int a_height, VkFormat a_format, VkImageUsageFlags a_usage);
  VkSampler   CreateSampler(const Sampler& a_sampler);
  VkImageView CreateView(VkFormat a_format, VkImage a_image);
  struct TexAccessPair
  {
    TexAccessPair() : image(VK_NULL_HANDLE), access(0) {}
    TexAccessPair(VkImage a_image, VkAccessFlags a_access) : image(a_image), access(a_access) {}
    VkImage image;
    VkAccessFlags access;
  };
  void TrackTextureAccess(const std::vector<TexAccessPair>& a_pairs, std::unordered_map<uint64_t, VkAccessFlags>& a_currImageFlags);
  size_t m_maxThreadCount = 0;
  VkBuffer m_classDataBuffer = VK_NULL_HANDLE;

  VkPipelineLayout      ResolveDebugLayout   = VK_NULL_HANDLE;
  VkPipeline            ResolveDebugPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout ResolveDebugDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreateResolveDebugDSLayout();
  virtual void InitKernel_ResolveDebug(const char* a_filePath);
  VkPipelineLayout      FillGbufferLayout   = VK_NULL_HANDLE;
  VkPipeline            FillGbufferPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout FillGbufferDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreateFillGbufferDSLayout();
  virtual void InitKernel_FillGbuffer(const char* a_filePath);
  VkPipelineLayout      FillGbufferWithTransparencyLayout   = VK_NULL_HANDLE;
  VkPipeline            FillGbufferWithTransparencyPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout FillGbufferWithTransparencyDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreateFillGbufferWithTransparencyDSLayout();
  virtual void InitKernel_FillGbufferWithTransparency(const char* a_filePath);
  VkPipelineLayout      ResolveOutlineLayout   = VK_NULL_HANDLE;
  VkPipeline            ResolveOutlinePipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout ResolveOutlineDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreateResolveOutlineDSLayout();
  virtual void InitKernel_ResolveOutline(const char* a_filePath);
  VkPipelineLayout      AverageColorLayout   = VK_NULL_HANDLE;
  VkPipeline            AverageColorPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout AverageColorDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreateAverageColorDSLayout();
  virtual void InitKernel_AverageColor(const char* a_filePath);
  VkPipelineLayout      TonemapLayout   = VK_NULL_HANDLE;
  VkPipeline            TonemapPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout TonemapDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreateTonemapDSLayout();
  virtual void InitKernel_Tonemap(const char* a_filePath);
  VkPipelineLayout      PackXYLayout   = VK_NULL_HANDLE;
  VkPipeline            PackXYPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout PackXYDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreatePackXYDSLayout();
  virtual void InitKernel_PackXY(const char* a_filePath);
  VkPipelineLayout      ResolveCommonLayout   = VK_NULL_HANDLE;
  VkPipeline            ResolveCommonPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout ResolveCommonDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreateResolveCommonDSLayout();
  virtual void InitKernel_ResolveCommon(const char* a_filePath);
  VkPipelineLayout      ResolveCommonWithTransparencyLayout   = VK_NULL_HANDLE;
  VkPipeline            ResolveCommonWithTransparencyPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout ResolveCommonWithTransparencyDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreateResolveCommonWithTransparencyDSLayout();
  virtual void InitKernel_ResolveCommonWithTransparency(const char* a_filePath);
  VkPipelineLayout      ResolveCTFLayout   = VK_NULL_HANDLE;
  VkPipeline            ResolveCTFPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout ResolveCTFDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreateResolveCTFDSLayout();
  virtual void InitKernel_ResolveCTF(const char* a_filePath);
  VkPipelineCache m_pipelineCache = VK_NULL_HANDLE; // if pipeline cache not enabled, will be VK_NULL_HANDLE 
  virtual VkBufferUsageFlags GetAdditionalFlagsForUBO() const;
  virtual uint32_t           GetDefaultMaxTextures() const;

  VkPipelineLayout      copyKernelFloatLayout   = VK_NULL_HANDLE;
  VkPipeline            copyKernelFloatPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout copyKernelFloatDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreatecopyKernelFloatDSLayout();

  VkPipelineLayout      matMulTransposeLayout   = VK_NULL_HANDLE;
  VkPipeline            matMulTransposePipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout matMulTransposeDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreatematMulTransposeDSLayout();

  VkDescriptorPool m_dsPool = VK_NULL_HANDLE;
  VkDescriptorSet  m_allGeneratedDS[19];

  MultiRenderer_slang_comp_rq_UBO_Data m_uboData;

  constexpr static uint32_t MEMCPY_BLOCK_SIZE = 256;
  constexpr static uint32_t REDUCTION_BLOCK_SIZE = 256;

  virtual void MakeComputePipelineAndLayout(const char* a_shaderPath, const char* a_mainName, const VkSpecializationInfo *a_specInfo, const VkDescriptorSetLayout a_dsLayout,
                                            VkPipelineLayout* pPipelineLayout, VkPipeline* pPipeline);
  virtual void MakeComputePipelineOnly(const char* a_shaderPath, const char* a_mainName, const VkSpecializationInfo *a_specInfo, const VkDescriptorSetLayout a_dsLayout, VkPipelineLayout pipelineLayout,
                                       VkPipeline* pPipeline);

  std::vector<VkPipelineLayout> m_allCreatedPipelineLayouts; ///<! remenber them here to delete later
  std::vector<VkPipeline>       m_allCreatedPipelines;       ///<! remenber them here to delete later
public:

  struct MegaKernelIsEnabled
  {
    bool dummy = 0;
  };

  static MegaKernelIsEnabled  m_megaKernelFlags;
  static MegaKernelIsEnabled& EnabledPipelines() { return m_megaKernelFlags; }
  uint32_t m_subgroupSize = 1; 
};


