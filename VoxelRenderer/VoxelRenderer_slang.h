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
#include "VoxelRenderer.h"


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

struct VoxelRenderer_slang_UBO_Data
{
  float4x4 m_projInv;
  float4x4 m_worldViewInv;
  Plane m_cuttingPlane;
  float4 m_backgroundColor;
  SCom2Header m_header;
  VoxelRenderPreset m_preset;
  uint m_AAFrameNum;
  uint m_DAG_max_level_size;
  uint m_SVO_max_level_size;
  uint m_data_type;
  uint m_height;
  uint m_packedXY_height;
  uint m_packedXY_width;
  uint m_seed;
  uint m_width;
  uint m_RotAddTable_capacity;
  uint m_RotAddTable_size;
  uint m_SComNodes_capacity;
  uint m_SComNodes_size;
  uint m_SVO_capacity;
  uint m_SVO_size;
  uint m_SdfCompactOctreeRotModifiers_capacity;
  uint m_SdfCompactOctreeRotModifiers_size;
  uint m_SdfDAGBlockIds_capacity;
  uint m_SdfDAGBlockIds_size;
  uint m_SdfDAGChildEdges_capacity;
  uint m_SdfDAGChildEdges_size;
  uint m_SdfDAGDataEdges_capacity;
  uint m_SdfDAGDataEdges_size;
  uint m_SdfDAGDistances_capacity;
  uint m_SdfDAGDistances_size;
  uint m_SdfDAGHeaders_capacity;
  uint m_SdfDAGHeaders_size;
  uint m_SdfDAGNodes_capacity;
  uint m_SdfDAGNodes_size;
  uint m_blocks_capacity;
  uint m_blocks_size;
  uint m_colorBuffer_capacity;
  uint m_colorBuffer_size;
  uint m_lights_capacity;
  uint m_lights_size;
  uint m_packedXY_capacity;
  uint m_packedXY_size;
  uint m_textures_capacity;
  uint m_textures_size;
  uint dummy_last;
};
/////////////////////////////////////////////////////////////////////////////////////////// UBO
class VoxelRenderer_slang : public VoxelRenderer
{
public:

  VoxelRenderer_slang()
  {
  }
  const char* Name() const override { return "VoxelRenderer_slang";}
  virtual void InitVulkanObjects(VkDevice a_device, VkPhysicalDevice a_physicalDevice, size_t a_maxThreadsCount);

  virtual void SetVulkanContext(vk_utils::VulkanContext a_ctx) { m_ctx = a_ctx; }
  virtual void SetVulkanInOutFor_RenderFloat(
    VkBuffer imageDataBuffer,
    size_t   imageDataOffset,
    uint32_t dummyArgument = 0)
  {
    RenderFloat_local.imageDataBuffer = imageDataBuffer;
    RenderFloat_local.imageDataOffset = imageDataOffset;
    UpdateAllGeneratedDescriptorSets_RenderFloat();
  }

  virtual void SetVulkanInOutFor_Render(
    VkBuffer imageDataBuffer,
    size_t   imageDataOffset,
    uint32_t dummyArgument = 0)
  {
    Render_local.imageDataBuffer = imageDataBuffer;
    Render_local.imageDataOffset = imageDataOffset;
    UpdateAllGeneratedDescriptorSets_Render();
  }

  virtual void SetVulkanInOutFor_Clear(
    uint32_t dummyArgument = 0)
  {
    UpdateAllGeneratedDescriptorSets_Clear();
  }

  virtual ~VoxelRenderer_slang();


virtual void InitDeviceData() ;
virtual void UpdateDeviceData(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine)   {
    UpdateVectorMembers(a_pCopyEngine);
    UpdateTextureMembers(a_pCopyEngine);
    UpdatePlainMembers(a_pCopyEngine);
  }
  void Update_m_lights(size_t a_first, size_t a_size) override;

  std::shared_ptr<vk_utils::ICopyEngine> m_pLastCopyHelper = nullptr;
  virtual void DeleteDeviceData();
  virtual void CommitDeviceData(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyHelper) // you have to define this virtual function in the original imput class
  {
    ReserveEmptyVectors();
    DeleteDeviceData();
    InitDeviceData();
    UpdateDeviceData(a_pCopyHelper);
    m_pLastCopyHelper = a_pCopyHelper;
    m_commitCount++;
  }
  void CommitDeviceData() override { CommitDeviceData(m_ctx.pCopyHelper); }
  void GetExecutionTime(const char* a_funcName, float a_out[4]) override;
  
  virtual void ReserveEmptyVectors();
  virtual void UpdatePlainMembers(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine);
  virtual void UpdateVectorMembers(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine);
  virtual void UpdateTextureMembers(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine);
  virtual void ReadPlainMembers(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine);
  static VkPhysicalDeviceFeatures2 ListRequiredDeviceFeatures(std::vector<const char*>& deviceExtensions);

  virtual void RenderFloatCmd(VkCommandBuffer a_commandBuffer, float4 *imageData, uint32_t width, uint32_t height, int passNum);
  virtual void RenderCmd(VkCommandBuffer a_commandBuffer, uint32_t *imageData, uint32_t width, uint32_t height, int passNum);
  virtual void ClearCmd(VkCommandBuffer a_commandBuffer, uint32_t width, uint32_t height);

  void RenderFloat(float4 *imageData, uint32_t width, uint32_t height, int passNum) override;
  void Render(uint32_t *imageData, uint32_t width, uint32_t height, int passNum) override;
  void Clear(uint32_t width, uint32_t height) override;

  inline vk_utils::ExecTime GetRenderFloatExecutionTime() const { return m_exTimeRenderFloat; }
  inline vk_utils::ExecTime GetRenderExecutionTime() const { return m_exTimeRender; }
  inline vk_utils::ExecTime GetClearExecutionTime() const { return m_exTimeClear; }

  vk_utils::ExecTime m_exTimeRenderFloat;
  vk_utils::ExecTime m_exTimeRender;
  vk_utils::ExecTime m_exTimeClear;

  virtual void copyKernelFloatCmd(uint32_t length);
  virtual void matMulTransposeCmd(uint32_t A_offset, uint32_t B_offset, uint32_t C_offset, uint32_t A_col_len, uint32_t B_col_len, uint32_t A_row_len);

  virtual void PackXYCmd(uint32_t w, uint32_t h);
  virtual void AverageColorCmd(uint32_t count, uint32_t sample_count, LiteMath::float4* out_color);
  virtual void IntersectSVOFastCmd(uint32_t count, uint32_t sample_id, float4* out_color);
  virtual void IntersectDAGCmd(uint32_t count, uint32_t sample_id, float4* out_color);
  virtual void TonemapCmd(uint32_t count, uint32_t sample_count, uint32_t* out_color);
  virtual void IntersectSComCmd(uint32_t count, uint32_t sample_id, float4* out_color);
  virtual void IntersectSVOCmd(uint32_t count, uint32_t sample_id, float4* out_color);

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
  virtual std::string AlterShaderPath(const char* in_shaderPath) { return std::string("VoxelRenderer/") + std::string(in_shaderPath); }
  
  

  struct RenderFloat_Data
  {
    VkBuffer imageDataBuffer = VK_NULL_HANDLE;
    size_t   imageDataOffset = 0;
    bool needToClearOutput = false;
  } RenderFloat_local;

  struct Render_Data
  {
    VkBuffer imageDataBuffer = VK_NULL_HANDLE;
    size_t   imageDataOffset = 0;
    bool needToClearOutput = false;
  } Render_local;

  struct Clear_Data
  {
    bool needToClearOutput = false;
  } Clear_local;



  struct MembersDataGPU
  {
    VkBuffer m_RotAddTableBuffer = VK_NULL_HANDLE;
    size_t   m_RotAddTableOffset = 0;
    VkBuffer m_SComNodesBuffer = VK_NULL_HANDLE;
    size_t   m_SComNodesOffset = 0;
    VkBuffer m_SVOBuffer = VK_NULL_HANDLE;
    size_t   m_SVOOffset = 0;
    VkBuffer m_SdfCompactOctreeRotModifiersBuffer = VK_NULL_HANDLE;
    size_t   m_SdfCompactOctreeRotModifiersOffset = 0;
    VkBuffer m_SdfDAGBlockIdsBuffer = VK_NULL_HANDLE;
    size_t   m_SdfDAGBlockIdsOffset = 0;
    VkBuffer m_SdfDAGChildEdgesBuffer = VK_NULL_HANDLE;
    size_t   m_SdfDAGChildEdgesOffset = 0;
    VkBuffer m_SdfDAGDataEdgesBuffer = VK_NULL_HANDLE;
    size_t   m_SdfDAGDataEdgesOffset = 0;
    VkBuffer m_SdfDAGDistancesBuffer = VK_NULL_HANDLE;
    size_t   m_SdfDAGDistancesOffset = 0;
    VkBuffer m_SdfDAGHeadersBuffer = VK_NULL_HANDLE;
    size_t   m_SdfDAGHeadersOffset = 0;
    VkBuffer m_SdfDAGNodesBuffer = VK_NULL_HANDLE;
    size_t   m_SdfDAGNodesOffset = 0;
    VkBuffer m_blocksBuffer = VK_NULL_HANDLE;
    size_t   m_blocksOffset = 0;
    VkBuffer m_colorBufferBuffer = VK_NULL_HANDLE;
    size_t   m_colorBufferOffset = 0;
    VkBuffer m_lightsBuffer = VK_NULL_HANDLE;
    size_t   m_lightsOffset = 0;
    VkBuffer m_packedXYBuffer = VK_NULL_HANDLE;
    size_t   m_packedXYOffset = 0;
    std::vector<VkImage>     m_texturesArrayTexture;
    std::vector<VkImageView> m_texturesArrayView   ;
    std::vector<VkSampler>   m_texturesArraySampler; ///<! samplers for texture arrays are always used
    size_t                   m_texturesArrayMaxSize; ///<! used when texture array size is not known after constructor of base class is finished
  } m_vdata;


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

  VkPipelineLayout      PackXYLayout   = VK_NULL_HANDLE;
  VkPipeline            PackXYPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout PackXYDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreatePackXYDSLayout();
  virtual void InitKernel_PackXY(const char* a_filePath);
  VkPipelineLayout      AverageColorLayout   = VK_NULL_HANDLE;
  VkPipeline            AverageColorPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout AverageColorDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreateAverageColorDSLayout();
  virtual void InitKernel_AverageColor(const char* a_filePath);
  VkPipelineLayout      IntersectSVOFastLayout   = VK_NULL_HANDLE;
  VkPipeline            IntersectSVOFastPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout IntersectSVOFastDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreateIntersectSVOFastDSLayout();
  virtual void InitKernel_IntersectSVOFast(const char* a_filePath);
  VkPipelineLayout      IntersectDAGLayout   = VK_NULL_HANDLE;
  VkPipeline            IntersectDAGPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout IntersectDAGDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreateIntersectDAGDSLayout();
  virtual void InitKernel_IntersectDAG(const char* a_filePath);
  VkPipelineLayout      TonemapLayout   = VK_NULL_HANDLE;
  VkPipeline            TonemapPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout TonemapDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreateTonemapDSLayout();
  virtual void InitKernel_Tonemap(const char* a_filePath);
  VkPipelineLayout      IntersectSComLayout   = VK_NULL_HANDLE;
  VkPipeline            IntersectSComPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout IntersectSComDSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreateIntersectSComDSLayout();
  virtual void InitKernel_IntersectSCom(const char* a_filePath);
  VkPipelineLayout      IntersectSVOLayout   = VK_NULL_HANDLE;
  VkPipeline            IntersectSVOPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout IntersectSVODSLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout CreateIntersectSVODSLayout();
  virtual void InitKernel_IntersectSVO(const char* a_filePath);
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
  VkDescriptorSet  m_allGeneratedDS[9];

  VoxelRenderer_slang_UBO_Data m_uboData;

  constexpr static uint32_t MEMCPY_BLOCK_SIZE = 256;
  constexpr static uint32_t REDUCTION_BLOCK_SIZE = 256;

  virtual void SceneRestrictions(uint32_t a_restrictions[4]) const
  {
    uint32_t maxMeshes            = 1024;
    uint32_t maxTotalVertices     = 1'000'000;
    uint32_t maxTotalPrimitives   = 1'000'000;
    uint32_t maxPrimitivesPerMesh = 200'000;

    a_restrictions[0] = maxMeshes;
    a_restrictions[1] = maxTotalVertices;
    a_restrictions[2] = maxTotalPrimitives;
    a_restrictions[3] = maxPrimitivesPerMesh;
  }
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


