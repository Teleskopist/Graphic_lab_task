#include <cstdint>
#include <vector>
#include <array>
#include <memory>
#include <limits>
#include <cassert>
#include "vk_copy.h"
#include "vk_context.h"
#include "MultiRenderer_slang_comp.h"


std::shared_ptr<MultiRenderer> CreateMultiRenderer_slang_comp(uint32_t AS_type, uint32_t maxPrimitives, vk_utils::VulkanContext a_ctx, size_t a_maxThreadsGenerated)
{
  auto pObj = std::make_shared<MultiRenderer_slang_comp>(AS_type, maxPrimitives);
  pObj->SetVulkanContext(a_ctx);
  pObj->InitVulkanObjects(a_ctx.device, a_ctx.physicalDevice, a_maxThreadsGenerated);
  return pObj;
}

vk_utils::VulkanDeviceFeatures MultiRenderer_slang_comp_ListRequiredDeviceFeatures()
{
  vk_utils::VulkanDeviceFeatures res;
  res.features2 = MultiRenderer_slang_comp::ListRequiredDeviceFeatures(res.extensionNames);
  res.apiVersion = VK_API_VERSION_1_2;
  return res;
}

void MultiRenderer_slang_comp::InitVulkanObjects(VkDevice a_device, VkPhysicalDevice a_physicalDevice, size_t a_maxThreadsCount)
{
  m_physicalDevice = a_physicalDevice;
  m_device         = a_device;
  m_allCreatedPipelineLayouts.reserve(256);
  m_allCreatedPipelines.reserve(256);
  InitHelpers();
  InitBuffers(a_maxThreadsCount, true);
  InitKernels(".spv");
  AllocateAllDescriptorSets();
  // get timestampPeriod from device props
  //
  VkPhysicalDeviceProperties2 physicalDeviceProperties{};
  VkPhysicalDeviceSubgroupProperties  subgroupProperties{};
  subgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
  subgroupProperties.pNext = nullptr;
  physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  physicalDeviceProperties.pNext = &subgroupProperties;
  vkGetPhysicalDeviceProperties2(m_physicalDevice, &physicalDeviceProperties);
  m_subgroupSize    = subgroupProperties.subgroupSize;
}

static uint32_t ComputeReductionAuxBufferElements(uint32_t whole_size, uint32_t wg_size)
{
  uint32_t sizeTotal = 0;
  while (whole_size > 1)
  {
    whole_size  = (whole_size + wg_size - 1) / wg_size;
    sizeTotal  += std::max<uint32_t>(whole_size, 1);
  }
  return sizeTotal;
}

VkBufferUsageFlags MultiRenderer_slang_comp::GetAdditionalFlagsForUBO() const
{
  return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
}

uint32_t MultiRenderer_slang_comp::GetDefaultMaxTextures() const { return 256; }

void MultiRenderer_slang_comp::MakeComputePipelineAndLayout(const char* a_shaderPath, const char* a_mainName, const VkSpecializationInfo *a_specInfo, const VkDescriptorSetLayout a_dsLayout, VkPipelineLayout* pPipelineLayout, VkPipeline* pPipeline)
{
  VkPipelineShaderStageCreateInfo shaderStageInfo = {};
  shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;

  auto shaderCode   = vk_utils::readSPVFile(a_shaderPath);
  auto shaderModule = vk_utils::createShaderModule(m_device, shaderCode);

  shaderStageInfo.module              = shaderModule;
  shaderStageInfo.pName               = a_mainName;
  shaderStageInfo.pSpecializationInfo = a_specInfo;

  VkPushConstantRange pcRange = {};
  pcRange.stageFlags = shaderStageInfo.stage;
  pcRange.offset     = 0;
  pcRange.size       = 128; // at least 128 bytes for push constants for all Vulkan implementations

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges    = &pcRange;
  pipelineLayoutInfo.pSetLayouts            = &a_dsLayout;
  pipelineLayoutInfo.setLayoutCount         = 1;

  VkResult res = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, pPipelineLayout);
  if(res != VK_SUCCESS)
  {
    std::string errMsg = vk_utils::errorString(res);
    std::cout << "[ShaderError]: vkCreatePipelineLayout have failed for '" << a_shaderPath << "' with '" << errMsg.c_str() << "'" << std::endl;
  }
  else
    m_allCreatedPipelineLayouts.push_back(*pPipelineLayout);

  VkComputePipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.flags              = 0;
  pipelineInfo.stage              = shaderStageInfo;
  pipelineInfo.layout             = (*pPipelineLayout);
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  res = vkCreateComputePipelines(m_device, m_pipelineCache, 1, &pipelineInfo, nullptr, pPipeline);
  if(res != VK_SUCCESS)
  {
    std::string errMsg = vk_utils::errorString(res);
    std::cout << "[ShaderError]: vkCreateComputePipelines have failed for '" << a_shaderPath << "' with '" << errMsg.c_str() << "'" << std::endl;
  }
  else
    m_allCreatedPipelines.push_back(*pPipeline);

  if (shaderModule != VK_NULL_HANDLE)
    vkDestroyShaderModule(m_device, shaderModule, VK_NULL_HANDLE);
}

void MultiRenderer_slang_comp::MakeComputePipelineOnly(const char* a_shaderPath, const char* a_mainName, const VkSpecializationInfo *a_specInfo, const VkDescriptorSetLayout a_dsLayout, VkPipelineLayout pipelineLayout, VkPipeline* pPipeline)
{
  VkPipelineShaderStageCreateInfo shaderStageInfo = {};
  shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;

  auto shaderCode   = vk_utils::readSPVFile(a_shaderPath);
  auto shaderModule = vk_utils::createShaderModule(m_device, shaderCode);

  shaderStageInfo.module              = shaderModule;
  shaderStageInfo.pName               = a_mainName;
  shaderStageInfo.pSpecializationInfo = a_specInfo;

  VkComputePipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.flags              = 0;
  pipelineInfo.stage              = shaderStageInfo;
  pipelineInfo.layout             = pipelineLayout;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  VkResult res = vkCreateComputePipelines(m_device, m_pipelineCache, 1, &pipelineInfo, nullptr, pPipeline);
  if(res != VK_SUCCESS)
  {
    std::string errMsg = vk_utils::errorString(res);
    std::cout << "[ShaderError]: vkCreateComputePipelines have failed for '" << a_shaderPath << "' with '" << errMsg.c_str() << "'" << std::endl;
  }
  else
    m_allCreatedPipelines.push_back(*pPipeline);

  if (shaderModule != VK_NULL_HANDLE)
    vkDestroyShaderModule(m_device, shaderModule, VK_NULL_HANDLE);
}

void MultiRenderer_slang_comp::DeleteDeviceData()
{
  if(m_commitCount == 0)
    return;
  vkDestroyBuffer(m_device, m_classDataBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_allCTFPointsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_allCTFsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_allChannelOffsetsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_allChannelRenderInfoBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_allCompressedChannelsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_allFloatChannelsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_allIntChannelsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_colorBufferBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_gBufferBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_geomOffsetsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_indicesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_instanceTransformInvTransposedBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_lightsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_matIdOffsetsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_matIdbyPrimIdBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_materialsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_normalsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_GraphicsPrimHeadersBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_GraphicsPrimPointsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_RotAddTableBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SCom2DataBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SCom2HeadersBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotModifiersBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotTransformsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfCompactOctreeV3DataBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfDAGChildEdgesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfDAGDataEdgesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfDAGDistancesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfDAGHeadersBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfDAGNodesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfDAGTranspositionsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfFrameOctreeNodesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfFrameOctreeRootsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexNodesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexRootsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfGridDataBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfGridOffsetsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfGridSizesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfMultiOctreeNodesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfMultiOctreeValuesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfSBSDataBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfSBSDataFBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfSBSHeadersBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfSBSNodesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfSBSRootsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfSVSNodesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_SdfSVSRootsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_allNodePairsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_geomDataBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_geomTagsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_indicesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_instanceDataBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_nodesTLASBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_origNodesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_primIdCountBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_primIndicesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_vertNormBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_m_vertPosBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_pAccelStruct_startEndBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_packedXYBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_transparencyBufferBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_verticesBuffer, nullptr);
  for(auto obj : m_vdata.m_texturesArrayTexture)
    vkDestroyImage(m_device, obj, nullptr);
  for(auto obj : m_vdata.m_texturesArrayView)
    vkDestroyImageView(m_device, obj, nullptr);
  for(auto obj : m_vdata.m_texturesArraySampler)
  vkDestroySampler(m_device, obj, nullptr);
  if(copyKernelFloatDSLayout != VK_NULL_HANDLE)
     vkDestroyDescriptorSetLayout(m_device, copyKernelFloatDSLayout, nullptr);
  FreeAllAllocations(m_allMems);
}

MultiRenderer_slang_comp::~MultiRenderer_slang_comp()
{
  for(size_t i=0;i<m_allCreatedPipelines.size();i++)
    vkDestroyPipeline(m_device, m_allCreatedPipelines[i], nullptr);
  for(size_t i=0;i<m_allCreatedPipelineLayouts.size();i++)
    vkDestroyPipelineLayout(m_device, m_allCreatedPipelineLayouts[i], nullptr);
  vkDestroyDescriptorSetLayout(m_device, ResolveDebugDSLayout, nullptr);
  ResolveDebugDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, FillGbufferDSLayout, nullptr);
  FillGbufferDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, FillGbufferWithTransparencyDSLayout, nullptr);
  FillGbufferWithTransparencyDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, ResolveOutlineDSLayout, nullptr);
  ResolveOutlineDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, AverageColorDSLayout, nullptr);
  AverageColorDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, TonemapDSLayout, nullptr);
  TonemapDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, PackXYDSLayout, nullptr);
  PackXYDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, ResolveCommonDSLayout, nullptr);
  ResolveCommonDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, ResolveCommonWithTransparencyDSLayout, nullptr);
  ResolveCommonWithTransparencyDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, ResolveCTFDSLayout, nullptr);
  ResolveCTFDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorPool(m_device, m_dsPool, NULL); m_dsPool = VK_NULL_HANDLE;
  DeleteDeviceData();
}

void MultiRenderer_slang_comp::InitHelpers()
{
  vkGetPhysicalDeviceProperties(m_physicalDevice, &m_devProps);
}


void MultiRenderer_slang_comp::InitKernel_ResolveDebug(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang_comp/kernel1D_ResolveDebug.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  ResolveDebugDSLayout = CreateResolveDebugDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, ResolveDebugDSLayout, &ResolveDebugLayout, &ResolveDebugPipeline);
  }
  else
  {
    ResolveDebugLayout   = nullptr;
    ResolveDebugPipeline = nullptr;
  }
}

void MultiRenderer_slang_comp::InitKernel_FillGbuffer(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang_comp/kernel1D_FillGbuffer.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  FillGbufferDSLayout = CreateFillGbufferDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, FillGbufferDSLayout, &FillGbufferLayout, &FillGbufferPipeline);
  }
  else
  {
    FillGbufferLayout   = nullptr;
    FillGbufferPipeline = nullptr;
  }
}

void MultiRenderer_slang_comp::InitKernel_FillGbufferWithTransparency(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang_comp/kernel1D_FillGbufferWithTransparency.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  FillGbufferWithTransparencyDSLayout = CreateFillGbufferWithTransparencyDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, FillGbufferWithTransparencyDSLayout, &FillGbufferWithTransparencyLayout, &FillGbufferWithTransparencyPipeline);
  }
  else
  {
    FillGbufferWithTransparencyLayout   = nullptr;
    FillGbufferWithTransparencyPipeline = nullptr;
  }
}

void MultiRenderer_slang_comp::InitKernel_ResolveOutline(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang_comp/kernel1D_ResolveOutline.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  ResolveOutlineDSLayout = CreateResolveOutlineDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, ResolveOutlineDSLayout, &ResolveOutlineLayout, &ResolveOutlinePipeline);
  }
  else
  {
    ResolveOutlineLayout   = nullptr;
    ResolveOutlinePipeline = nullptr;
  }
}

void MultiRenderer_slang_comp::InitKernel_AverageColor(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang_comp/kernel1D_AverageColor.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  AverageColorDSLayout = CreateAverageColorDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, AverageColorDSLayout, &AverageColorLayout, &AverageColorPipeline);
  }
  else
  {
    AverageColorLayout   = nullptr;
    AverageColorPipeline = nullptr;
  }
}

void MultiRenderer_slang_comp::InitKernel_Tonemap(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang_comp/kernel1D_Tonemap.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  TonemapDSLayout = CreateTonemapDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, TonemapDSLayout, &TonemapLayout, &TonemapPipeline);
  }
  else
  {
    TonemapLayout   = nullptr;
    TonemapPipeline = nullptr;
  }
}

void MultiRenderer_slang_comp::InitKernel_PackXY(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang_comp/kernel2D_PackXY.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  PackXYDSLayout = CreatePackXYDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, PackXYDSLayout, &PackXYLayout, &PackXYPipeline);
  }
  else
  {
    PackXYLayout   = nullptr;
    PackXYPipeline = nullptr;
  }
}

void MultiRenderer_slang_comp::InitKernel_ResolveCommon(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang_comp/kernel1D_ResolveCommon.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  ResolveCommonDSLayout = CreateResolveCommonDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, ResolveCommonDSLayout, &ResolveCommonLayout, &ResolveCommonPipeline);
  }
  else
  {
    ResolveCommonLayout   = nullptr;
    ResolveCommonPipeline = nullptr;
  }
}

void MultiRenderer_slang_comp::InitKernel_ResolveCommonWithTransparency(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang_comp/kernel1D_ResolveCommonWithTransparency.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  ResolveCommonWithTransparencyDSLayout = CreateResolveCommonWithTransparencyDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, ResolveCommonWithTransparencyDSLayout, &ResolveCommonWithTransparencyLayout, &ResolveCommonWithTransparencyPipeline);
  }
  else
  {
    ResolveCommonWithTransparencyLayout   = nullptr;
    ResolveCommonWithTransparencyPipeline = nullptr;
  }
}

void MultiRenderer_slang_comp::InitKernel_ResolveCTF(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang_comp/kernel1D_ResolveCTF.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  ResolveCTFDSLayout = CreateResolveCTFDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, ResolveCTFDSLayout, &ResolveCTFLayout, &ResolveCTFPipeline);
  }
  else
  {
    ResolveCTFLayout   = nullptr;
    ResolveCTFPipeline = nullptr;
  }
}


void MultiRenderer_slang_comp::InitKernels(const char* a_filePath)
{
  InitKernel_ResolveDebug(a_filePath);
  InitKernel_FillGbuffer(a_filePath);
  InitKernel_FillGbufferWithTransparency(a_filePath);
  InitKernel_ResolveOutline(a_filePath);
  InitKernel_AverageColor(a_filePath);
  InitKernel_Tonemap(a_filePath);
  InitKernel_PackXY(a_filePath);
  InitKernel_ResolveCommon(a_filePath);
  InitKernel_ResolveCommonWithTransparency(a_filePath);
  InitKernel_ResolveCTF(a_filePath);
}

void MultiRenderer_slang_comp::InitBuffers(size_t a_maxThreadsCount, bool a_tempBuffersOverlay)
{
  ReserveEmptyVectors();

  m_maxThreadCount = a_maxThreadsCount;
  std::vector<VkBuffer> allBuffers;
  allBuffers.reserve(64);

  struct BufferReqPair
  {
    BufferReqPair() {  }
    BufferReqPair(VkBuffer a_buff, VkDevice a_dev) : buf(a_buff) { vkGetBufferMemoryRequirements(a_dev, a_buff, &req); }
    VkBuffer             buf = VK_NULL_HANDLE;
    VkMemoryRequirements req = {};
  };

  struct LocalBuffers
  {
    std::vector<BufferReqPair> bufs;
    size_t                     size = 0;
    std::vector<VkBuffer>      bufsClean;
  };

  std::vector<LocalBuffers> groups;
  groups.reserve(16);


  size_t largestIndex = 0;
  size_t largestSize  = 0;
  for(size_t i=0;i<groups.size();i++)
  {
    if(groups[i].size > largestSize)
    {
      largestIndex = i;
      largestSize  = groups[i].size;
    }
    groups[i].bufsClean.resize(groups[i].bufs.size());
    for(size_t j=0;j<groups[i].bufsClean.size();j++)
      groups[i].bufsClean[j] = groups[i].bufs[j].buf;
  }
  auto& allBuffersRef = allBuffers;

  auto internalBuffersMem = AllocAndBind(allBuffersRef);
  if(a_tempBuffersOverlay)
  {
    for(size_t i=0;i<groups.size();i++)
      if(i != largestIndex)
        AssignBuffersToMemory(groups[i].bufsClean, internalBuffersMem.memObject);
  }
}

void MultiRenderer_slang_comp::ReserveEmptyVectors()
{
  if(m_allCTFPoints.capacity() == 0)
    m_allCTFPoints.reserve(4);
  if(m_allCTFs.capacity() == 0)
    m_allCTFs.reserve(4);
  if(m_allChannelOffsets.capacity() == 0)
    m_allChannelOffsets.reserve(4);
  if(m_allChannelRenderInfo.capacity() == 0)
    m_allChannelRenderInfo.reserve(4);
  if(m_allCompressedChannels.capacity() == 0)
    m_allCompressedChannels.reserve(4);
  if(m_allFloatChannels.capacity() == 0)
    m_allFloatChannels.reserve(4);
  if(m_allIntChannels.capacity() == 0)
    m_allIntChannels.reserve(4);
  if(m_colorBuffer.capacity() == 0)
    m_colorBuffer.reserve(4);
  if(m_gBuffer.capacity() == 0)
    m_gBuffer.reserve(4);
  if(m_geomOffsets.capacity() == 0)
    m_geomOffsets.reserve(4);
  if(m_indices.capacity() == 0)
    m_indices.reserve(4);
  if(m_instanceTransformInvTransposed.capacity() == 0)
    m_instanceTransformInvTransposed.reserve(4);
  if(m_lights.capacity() == 0)
    m_lights.reserve(4);
  if(m_matIdOffsets.capacity() == 0)
    m_matIdOffsets.reserve(4);
  if(m_matIdbyPrimId.capacity() == 0)
    m_matIdbyPrimId.reserve(4);
  if(m_materials.capacity() == 0)
    m_materials.reserve(4);
  if(m_normals.capacity() == 0)
    m_normals.reserve(4);
  if(m_pAccelStruct_m_GraphicsPrimHeaders != nullptr && m_pAccelStruct_m_GraphicsPrimHeaders->capacity() == 0)
    m_pAccelStruct_m_GraphicsPrimHeaders->reserve(4);
  if(m_pAccelStruct_m_GraphicsPrimPoints != nullptr && m_pAccelStruct_m_GraphicsPrimPoints->capacity() == 0)
    m_pAccelStruct_m_GraphicsPrimPoints->reserve(4);
  if(m_pAccelStruct_m_RotAddTable != nullptr && m_pAccelStruct_m_RotAddTable->capacity() == 0)
    m_pAccelStruct_m_RotAddTable->reserve(4);
  if(m_pAccelStruct_m_SCom2Data != nullptr && m_pAccelStruct_m_SCom2Data->capacity() == 0)
    m_pAccelStruct_m_SCom2Data->reserve(4);
  if(m_pAccelStruct_m_SCom2Headers != nullptr && m_pAccelStruct_m_SCom2Headers->capacity() == 0)
    m_pAccelStruct_m_SCom2Headers->reserve(4);
  if(m_pAccelStruct_m_SdfCompactOctreeRotModifiers != nullptr && m_pAccelStruct_m_SdfCompactOctreeRotModifiers->capacity() == 0)
    m_pAccelStruct_m_SdfCompactOctreeRotModifiers->reserve(4);
  if(m_pAccelStruct_m_SdfCompactOctreeRotTransforms != nullptr && m_pAccelStruct_m_SdfCompactOctreeRotTransforms->capacity() == 0)
    m_pAccelStruct_m_SdfCompactOctreeRotTransforms->reserve(4);
  if(m_pAccelStruct_m_SdfCompactOctreeV3Data != nullptr && m_pAccelStruct_m_SdfCompactOctreeV3Data->capacity() == 0)
    m_pAccelStruct_m_SdfCompactOctreeV3Data->reserve(4);
  if(m_pAccelStruct_m_SdfDAGChildEdges != nullptr && m_pAccelStruct_m_SdfDAGChildEdges->capacity() == 0)
    m_pAccelStruct_m_SdfDAGChildEdges->reserve(4);
  if(m_pAccelStruct_m_SdfDAGDataEdges != nullptr && m_pAccelStruct_m_SdfDAGDataEdges->capacity() == 0)
    m_pAccelStruct_m_SdfDAGDataEdges->reserve(4);
  if(m_pAccelStruct_m_SdfDAGDistances != nullptr && m_pAccelStruct_m_SdfDAGDistances->capacity() == 0)
    m_pAccelStruct_m_SdfDAGDistances->reserve(4);
  if(m_pAccelStruct_m_SdfDAGHeaders != nullptr && m_pAccelStruct_m_SdfDAGHeaders->capacity() == 0)
    m_pAccelStruct_m_SdfDAGHeaders->reserve(4);
  if(m_pAccelStruct_m_SdfDAGNodes != nullptr && m_pAccelStruct_m_SdfDAGNodes->capacity() == 0)
    m_pAccelStruct_m_SdfDAGNodes->reserve(4);
  if(m_pAccelStruct_m_SdfDAGTranspositions != nullptr && m_pAccelStruct_m_SdfDAGTranspositions->capacity() == 0)
    m_pAccelStruct_m_SdfDAGTranspositions->reserve(4);
  if(m_pAccelStruct_m_SdfFrameOctreeNodes != nullptr && m_pAccelStruct_m_SdfFrameOctreeNodes->capacity() == 0)
    m_pAccelStruct_m_SdfFrameOctreeNodes->reserve(4);
  if(m_pAccelStruct_m_SdfFrameOctreeRoots != nullptr && m_pAccelStruct_m_SdfFrameOctreeRoots->capacity() == 0)
    m_pAccelStruct_m_SdfFrameOctreeRoots->reserve(4);
  if(m_pAccelStruct_m_SdfFrameOctreeTexNodes != nullptr && m_pAccelStruct_m_SdfFrameOctreeTexNodes->capacity() == 0)
    m_pAccelStruct_m_SdfFrameOctreeTexNodes->reserve(4);
  if(m_pAccelStruct_m_SdfFrameOctreeTexRoots != nullptr && m_pAccelStruct_m_SdfFrameOctreeTexRoots->capacity() == 0)
    m_pAccelStruct_m_SdfFrameOctreeTexRoots->reserve(4);
  if(m_pAccelStruct_m_SdfGridData != nullptr && m_pAccelStruct_m_SdfGridData->capacity() == 0)
    m_pAccelStruct_m_SdfGridData->reserve(4);
  if(m_pAccelStruct_m_SdfGridOffsets != nullptr && m_pAccelStruct_m_SdfGridOffsets->capacity() == 0)
    m_pAccelStruct_m_SdfGridOffsets->reserve(4);
  if(m_pAccelStruct_m_SdfGridSizes != nullptr && m_pAccelStruct_m_SdfGridSizes->capacity() == 0)
    m_pAccelStruct_m_SdfGridSizes->reserve(4);
  if(m_pAccelStruct_m_SdfMultiOctreeNodes != nullptr && m_pAccelStruct_m_SdfMultiOctreeNodes->capacity() == 0)
    m_pAccelStruct_m_SdfMultiOctreeNodes->reserve(4);
  if(m_pAccelStruct_m_SdfMultiOctreeValues != nullptr && m_pAccelStruct_m_SdfMultiOctreeValues->capacity() == 0)
    m_pAccelStruct_m_SdfMultiOctreeValues->reserve(4);
  if(m_pAccelStruct_m_SdfSBSData != nullptr && m_pAccelStruct_m_SdfSBSData->capacity() == 0)
    m_pAccelStruct_m_SdfSBSData->reserve(4);
  if(m_pAccelStruct_m_SdfSBSDataF != nullptr && m_pAccelStruct_m_SdfSBSDataF->capacity() == 0)
    m_pAccelStruct_m_SdfSBSDataF->reserve(4);
  if(m_pAccelStruct_m_SdfSBSHeaders != nullptr && m_pAccelStruct_m_SdfSBSHeaders->capacity() == 0)
    m_pAccelStruct_m_SdfSBSHeaders->reserve(4);
  if(m_pAccelStruct_m_SdfSBSNodes != nullptr && m_pAccelStruct_m_SdfSBSNodes->capacity() == 0)
    m_pAccelStruct_m_SdfSBSNodes->reserve(4);
  if(m_pAccelStruct_m_SdfSBSRoots != nullptr && m_pAccelStruct_m_SdfSBSRoots->capacity() == 0)
    m_pAccelStruct_m_SdfSBSRoots->reserve(4);
  if(m_pAccelStruct_m_SdfSVSNodes != nullptr && m_pAccelStruct_m_SdfSVSNodes->capacity() == 0)
    m_pAccelStruct_m_SdfSVSNodes->reserve(4);
  if(m_pAccelStruct_m_SdfSVSRoots != nullptr && m_pAccelStruct_m_SdfSVSRoots->capacity() == 0)
    m_pAccelStruct_m_SdfSVSRoots->reserve(4);
  if(m_pAccelStruct_m_allNodePairs != nullptr && m_pAccelStruct_m_allNodePairs->capacity() == 0)
    m_pAccelStruct_m_allNodePairs->reserve(4);
  if(m_pAccelStruct_m_geomData != nullptr && m_pAccelStruct_m_geomData->capacity() == 0)
    m_pAccelStruct_m_geomData->reserve(4);
  if(m_pAccelStruct_m_geomTags != nullptr && m_pAccelStruct_m_geomTags->capacity() == 0)
    m_pAccelStruct_m_geomTags->reserve(4);
  if(m_pAccelStruct_m_indices != nullptr && m_pAccelStruct_m_indices->capacity() == 0)
    m_pAccelStruct_m_indices->reserve(4);
  if(m_pAccelStruct_m_instanceData != nullptr && m_pAccelStruct_m_instanceData->capacity() == 0)
    m_pAccelStruct_m_instanceData->reserve(4);
  if(m_pAccelStruct_m_nodesTLAS != nullptr && m_pAccelStruct_m_nodesTLAS->capacity() == 0)
    m_pAccelStruct_m_nodesTLAS->reserve(4);
  if(m_pAccelStruct_m_origNodes != nullptr && m_pAccelStruct_m_origNodes->capacity() == 0)
    m_pAccelStruct_m_origNodes->reserve(4);
  if(m_pAccelStruct_m_primIdCount != nullptr && m_pAccelStruct_m_primIdCount->capacity() == 0)
    m_pAccelStruct_m_primIdCount->reserve(4);
  if(m_pAccelStruct_m_primIndices != nullptr && m_pAccelStruct_m_primIndices->capacity() == 0)
    m_pAccelStruct_m_primIndices->reserve(4);
  if(m_pAccelStruct_m_vertNorm != nullptr && m_pAccelStruct_m_vertNorm->capacity() == 0)
    m_pAccelStruct_m_vertNorm->reserve(4);
  if(m_pAccelStruct_m_vertPos != nullptr && m_pAccelStruct_m_vertPos->capacity() == 0)
    m_pAccelStruct_m_vertPos->reserve(4);
  if(m_pAccelStruct_startEnd != nullptr && m_pAccelStruct_startEnd->capacity() == 0)
    m_pAccelStruct_startEnd->reserve(4);
  if(m_packedXY.capacity() == 0)
    m_packedXY.reserve(4);
  if(m_transparencyBuffer.capacity() == 0)
    m_transparencyBuffer.reserve(4);
  if(m_vertices.capacity() == 0)
    m_vertices.reserve(4);
}

void MultiRenderer_slang_comp::InitDeviceData()
{
  std::vector<VkBuffer> memberVectorsWithDevAddr;
  std::vector<VkBuffer> memberVectors;
  std::vector<VkImage>  memberTextures;
  m_classDataBuffer = vk_utils::createBuffer(m_device, sizeof(m_uboData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | GetAdditionalFlagsForUBO() | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_classDataBuffer);
  m_vdata.m_allCTFPointsBuffer = vk_utils::createBuffer(m_device, m_allCTFPoints.capacity()*sizeof(float4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_allCTFPointsBuffer);
  m_vdata.m_allCTFsBuffer = vk_utils::createBuffer(m_device, m_allCTFs.capacity()*sizeof(ColorTranferFunctionInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_allCTFsBuffer);
  m_vdata.m_allChannelOffsetsBuffer = vk_utils::createBuffer(m_device, m_allChannelOffsets.capacity()*sizeof(uint4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_allChannelOffsetsBuffer);
  m_vdata.m_allChannelRenderInfoBuffer = vk_utils::createBuffer(m_device, m_allChannelRenderInfo.capacity()*sizeof(ChannelRenderInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_allChannelRenderInfoBuffer);
  m_vdata.m_allCompressedChannelsBuffer = vk_utils::createBuffer(m_device, m_allCompressedChannels.capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_allCompressedChannelsBuffer);
  m_vdata.m_allFloatChannelsBuffer = vk_utils::createBuffer(m_device, m_allFloatChannels.capacity()*sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_allFloatChannelsBuffer);
  m_vdata.m_allIntChannelsBuffer = vk_utils::createBuffer(m_device, m_allIntChannels.capacity()*sizeof(int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_allIntChannelsBuffer);
  m_vdata.m_colorBufferBuffer = vk_utils::createBuffer(m_device, m_colorBuffer.capacity()*sizeof(float4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_colorBufferBuffer);
  m_vdata.m_gBufferBuffer = vk_utils::createBuffer(m_device, m_gBuffer.capacity()*sizeof(CRT_Hit), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_gBufferBuffer);
  m_vdata.m_geomOffsetsBuffer = vk_utils::createBuffer(m_device, m_geomOffsets.capacity()*sizeof(uint2), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_geomOffsetsBuffer);
  m_vdata.m_indicesBuffer = vk_utils::createBuffer(m_device, m_indices.capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_indicesBuffer);
  m_vdata.m_instanceTransformInvTransposedBuffer = vk_utils::createBuffer(m_device, m_instanceTransformInvTransposed.capacity()*sizeof(float4x4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_instanceTransformInvTransposedBuffer);
  m_vdata.m_lightsBuffer = vk_utils::createBuffer(m_device, m_lights.capacity()*sizeof(Light), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_lightsBuffer);
  m_vdata.m_matIdOffsetsBuffer = vk_utils::createBuffer(m_device, m_matIdOffsets.capacity()*sizeof(uint2), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_matIdOffsetsBuffer);
  m_vdata.m_matIdbyPrimIdBuffer = vk_utils::createBuffer(m_device, m_matIdbyPrimId.capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_matIdbyPrimIdBuffer);
  m_vdata.m_materialsBuffer = vk_utils::createBuffer(m_device, m_materials.capacity()*sizeof(MultiRendererMaterial), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_materialsBuffer);
  m_vdata.m_normalsBuffer = vk_utils::createBuffer(m_device, m_normals.capacity()*sizeof(float4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_normalsBuffer);
  m_vdata.m_pAccelStruct_m_GraphicsPrimHeadersBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_GraphicsPrimHeaders->capacity()*sizeof(GraphicsPrimHeader), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_GraphicsPrimHeadersBuffer);
  m_vdata.m_pAccelStruct_m_GraphicsPrimPointsBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_GraphicsPrimPoints->capacity()*sizeof(float4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_GraphicsPrimPointsBuffer);
  m_vdata.m_pAccelStruct_m_RotAddTableBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_RotAddTable->capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_RotAddTableBuffer);
  m_vdata.m_pAccelStruct_m_SCom2DataBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SCom2Data->capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SCom2DataBuffer);
  m_vdata.m_pAccelStruct_m_SCom2HeadersBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SCom2Headers->capacity()*sizeof(SCom2Header), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SCom2HeadersBuffer);
  m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotModifiersBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfCompactOctreeRotModifiers->capacity()*sizeof(int4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotModifiersBuffer);
  m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotTransformsBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfCompactOctreeRotTransforms->capacity()*sizeof(float4x4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotTransformsBuffer);
  m_vdata.m_pAccelStruct_m_SdfCompactOctreeV3DataBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfCompactOctreeV3Data->capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfCompactOctreeV3DataBuffer);
  m_vdata.m_pAccelStruct_m_SdfDAGChildEdgesBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfDAGChildEdges->capacity()*sizeof(SdfDAGChildEdge), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfDAGChildEdgesBuffer);
  m_vdata.m_pAccelStruct_m_SdfDAGDataEdgesBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfDAGDataEdges->capacity()*sizeof(SdfDAGDataEdge), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfDAGDataEdgesBuffer);
  m_vdata.m_pAccelStruct_m_SdfDAGDistancesBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfDAGDistances->capacity()*sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfDAGDistancesBuffer);
  m_vdata.m_pAccelStruct_m_SdfDAGHeadersBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfDAGHeaders->capacity()*sizeof(SdfDAGHeader), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfDAGHeadersBuffer);
  m_vdata.m_pAccelStruct_m_SdfDAGNodesBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfDAGNodes->capacity()*sizeof(SdfDAGNode), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfDAGNodesBuffer);
  m_vdata.m_pAccelStruct_m_SdfDAGTranspositionsBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfDAGTranspositions->capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfDAGTranspositionsBuffer);
  m_vdata.m_pAccelStruct_m_SdfFrameOctreeNodesBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfFrameOctreeNodes->capacity()*sizeof(SdfFrameOctreeNode), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfFrameOctreeNodesBuffer);
  m_vdata.m_pAccelStruct_m_SdfFrameOctreeRootsBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfFrameOctreeRoots->capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfFrameOctreeRootsBuffer);
  m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexNodesBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfFrameOctreeTexNodes->capacity()*sizeof(SdfFrameOctreeTexNode), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexNodesBuffer);
  m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexRootsBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfFrameOctreeTexRoots->capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexRootsBuffer);
  m_vdata.m_pAccelStruct_m_SdfGridDataBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfGridData->capacity()*sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfGridDataBuffer);
  m_vdata.m_pAccelStruct_m_SdfGridOffsetsBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfGridOffsets->capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfGridOffsetsBuffer);
  m_vdata.m_pAccelStruct_m_SdfGridSizesBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfGridSizes->capacity()*sizeof(uint3), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfGridSizesBuffer);
  m_vdata.m_pAccelStruct_m_SdfMultiOctreeNodesBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfMultiOctreeNodes->capacity()*sizeof(SdfMultiOctreeNode), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfMultiOctreeNodesBuffer);
  m_vdata.m_pAccelStruct_m_SdfMultiOctreeValuesBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfMultiOctreeValues->capacity()*sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfMultiOctreeValuesBuffer);
  m_vdata.m_pAccelStruct_m_SdfSBSDataBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfSBSData->capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfSBSDataBuffer);
  m_vdata.m_pAccelStruct_m_SdfSBSDataFBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfSBSDataF->capacity()*sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfSBSDataFBuffer);
  m_vdata.m_pAccelStruct_m_SdfSBSHeadersBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfSBSHeaders->capacity()*sizeof(SdfSBSHeader), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfSBSHeadersBuffer);
  m_vdata.m_pAccelStruct_m_SdfSBSNodesBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfSBSNodes->capacity()*sizeof(SdfSBSNode), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfSBSNodesBuffer);
  m_vdata.m_pAccelStruct_m_SdfSBSRootsBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfSBSRoots->capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfSBSRootsBuffer);
  m_vdata.m_pAccelStruct_m_SdfSVSNodesBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfSVSNodes->capacity()*sizeof(SdfSVSNode), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfSVSNodesBuffer);
  m_vdata.m_pAccelStruct_m_SdfSVSRootsBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_SdfSVSRoots->capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_SdfSVSRootsBuffer);
  m_vdata.m_pAccelStruct_m_allNodePairsBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_allNodePairs->capacity()*sizeof(BVHNodePair), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_allNodePairsBuffer);
  m_vdata.m_pAccelStruct_m_geomDataBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_geomData->capacity()*sizeof(GeomData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_geomDataBuffer);
  m_vdata.m_pAccelStruct_m_geomTagsBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_geomTags->capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_geomTagsBuffer);
  m_vdata.m_pAccelStruct_m_indicesBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_indices->capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_indicesBuffer);
  m_vdata.m_pAccelStruct_m_instanceDataBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_instanceData->capacity()*sizeof(InstanceData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_instanceDataBuffer);
  m_vdata.m_pAccelStruct_m_nodesTLASBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_nodesTLAS->capacity()*sizeof(BVHNode), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_nodesTLASBuffer);
  m_vdata.m_pAccelStruct_m_origNodesBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_origNodes->capacity()*sizeof(BVHNode), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_origNodesBuffer);
  m_vdata.m_pAccelStruct_m_primIdCountBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_primIdCount->capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_primIdCountBuffer);
  m_vdata.m_pAccelStruct_m_primIndicesBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_primIndices->capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_primIndicesBuffer);
  m_vdata.m_pAccelStruct_m_vertNormBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_vertNorm->capacity()*sizeof(float4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_vertNormBuffer);
  m_vdata.m_pAccelStruct_m_vertPosBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_m_vertPos->capacity()*sizeof(float4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_m_vertPosBuffer);
  m_vdata.m_pAccelStruct_startEndBuffer = vk_utils::createBuffer(m_device, m_pAccelStruct_startEnd->capacity()*sizeof(uint2), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_pAccelStruct_startEndBuffer);
  m_vdata.m_packedXYBuffer = vk_utils::createBuffer(m_device, m_packedXY.capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_packedXYBuffer);
  m_vdata.m_transparencyBufferBuffer = vk_utils::createBuffer(m_device, m_transparencyBuffer.capacity()*sizeof(float4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_transparencyBufferBuffer);
  m_vdata.m_verticesBuffer = vk_utils::createBuffer(m_device, m_vertices.capacity()*sizeof(float4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_verticesBuffer);
  
  m_vdata.m_texturesArrayTexture.resize(0);
  m_vdata.m_texturesArrayView.resize(0);
  m_vdata.m_texturesArraySampler.resize(0);
  m_vdata.m_texturesArrayTexture.reserve(64);
  m_vdata.m_texturesArrayView.reserve(64);
  m_vdata.m_texturesArraySampler.reserve(64);
  for(auto imageObj : m_textures)
  {
    auto tex = CreateTexture2D(imageObj->width(), imageObj->height(), VkFormat(imageObj->format()), VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    auto sam = CreateSampler(imageObj->sampler());
    m_vdata.m_texturesArrayTexture.push_back(tex);
    m_vdata.m_texturesArrayView.push_back(VK_NULL_HANDLE);
    m_vdata.m_texturesArraySampler.push_back(sam);
    memberTextures.push_back(tex);
  }


  AllocMemoryForMemberBuffersAndImages(memberVectors, memberTextures);
  if(memberVectorsWithDevAddr.size() != 0)
    AllocAndBind(memberVectorsWithDevAddr, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
}



VkImage MultiRenderer_slang_comp::CreateTexture2D(const int a_width, const int a_height, VkFormat a_format, VkImageUsageFlags a_usage)
{
  VkImage result = VK_NULL_HANDLE;
  VkImageCreateInfo imgCreateInfo = {};
  imgCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imgCreateInfo.pNext         = nullptr;
  imgCreateInfo.flags         = 0; // not sure about this ...
  imgCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
  imgCreateInfo.format        = a_format;
  imgCreateInfo.extent        = VkExtent3D{uint32_t(a_width), uint32_t(a_height), 1};
  imgCreateInfo.mipLevels     = 1;
  imgCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
  imgCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
  imgCreateInfo.usage         = a_usage;
  imgCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
  imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgCreateInfo.arrayLayers   = 1;
  VK_CHECK_RESULT(vkCreateImage(m_device, &imgCreateInfo, nullptr, &result));
  return result;
}

VkSampler MultiRenderer_slang_comp::CreateSampler(const Sampler& a_sampler) // TODO: implement this function correctly
{
  VkSampler result = VK_NULL_HANDLE;
  VkSamplerCreateInfo samplerInfo = {};
  samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.pNext        = nullptr;
  samplerInfo.flags        = 0;
  samplerInfo.magFilter    = VkFilter(int(a_sampler.filter));
  samplerInfo.minFilter    = VkFilter(int(a_sampler.filter));
  samplerInfo.mipmapMode   = (samplerInfo.magFilter == VK_FILTER_LINEAR ) ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
  samplerInfo.addressModeU = VkSamplerAddressMode(int(a_sampler.addressU));
  samplerInfo.addressModeV = VkSamplerAddressMode(int(a_sampler.addressV));
  samplerInfo.addressModeW = VkSamplerAddressMode(int(a_sampler.addressW));
  samplerInfo.mipLodBias   = a_sampler.mipLODBias;
  samplerInfo.compareOp    = VK_COMPARE_OP_NEVER;
  samplerInfo.minLod           = a_sampler.minLOD;
  samplerInfo.maxLod           = a_sampler.maxLOD;
  samplerInfo.maxAnisotropy    = a_sampler.maxAnisotropy;
  samplerInfo.anisotropyEnable = (a_sampler.maxAnisotropy > 1) ? VK_TRUE : VK_FALSE;
  samplerInfo.borderColor      = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  VK_CHECK_RESULT(vkCreateSampler(m_device, &samplerInfo, nullptr, &result));
  return result;
}

VkImageView MultiRenderer_slang_comp::CreateView(VkFormat a_format, VkImage a_image)
{
  VkImageView result = VK_NULL_HANDLE;
  VkImageViewCreateInfo createInfo{};
  createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.image    = a_image;
  createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  createInfo.format   = a_format;

  if(a_format == VK_FORMAT_R32_SFLOAT || a_format == VK_FORMAT_R8_UNORM  || a_format == VK_FORMAT_R8_SNORM ||
     a_format == VK_FORMAT_R16_SFLOAT || a_format == VK_FORMAT_R16_UNORM || a_format == VK_FORMAT_R16_SNORM)
  {
    createInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_R;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_R;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_R;
  }
  else
  {
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  }

  createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  createInfo.subresourceRange.baseMipLevel   = 0;
  createInfo.subresourceRange.levelCount     = 1;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.layerCount     = 1;

  VK_CHECK_RESULT(vkCreateImageView(m_device, &createInfo, nullptr, &result));
  return result;
}




void MultiRenderer_slang_comp::AssignBuffersToMemory(const std::vector<VkBuffer>& a_buffers, VkDeviceMemory a_mem)
{
  if(a_buffers.size() == 0 || a_mem == VK_NULL_HANDLE)
    return;

  std::vector<VkMemoryRequirements> memInfos(a_buffers.size());
  for(size_t i=0;i<memInfos.size();i++)
  {
    if(a_buffers[i] != VK_NULL_HANDLE)
      vkGetBufferMemoryRequirements(m_device, a_buffers[i], &memInfos[i]);
    else
    {
      memInfos[i] = memInfos[0];
      memInfos[i].size = 0;
    }
  }

  for(size_t i=1;i<memInfos.size();i++)
  {
    if(memInfos[i].memoryTypeBits != memInfos[0].memoryTypeBits)
    {
      std::cout << "[MultiRenderer_slang_comp::AssignBuffersToMemory]: error, input buffers has different 'memReq.memoryTypeBits'" << std::endl;
      return;
    }
  }

  auto offsets = vk_utils::calculateMemOffsets(memInfos);
  for (size_t i = 0; i < memInfos.size(); i++)
  {
    if(a_buffers[i] != VK_NULL_HANDLE)
      vkBindBufferMemory(m_device, a_buffers[i], a_mem, offsets[i]);
  }
}

MultiRenderer_slang_comp::MemLoc MultiRenderer_slang_comp::AllocAndBind(const std::vector<VkBuffer>& a_buffers, VkMemoryAllocateFlags a_flags)
{
  MemLoc currLoc;
  if(a_buffers.size() > 0)
  {
    currLoc.memObject = vk_utils::allocateAndBindWithPadding(m_device, m_physicalDevice, a_buffers, a_flags);
    currLoc.allocId   = m_allMems.size();
    m_allMems.push_back(currLoc);
  }
  return currLoc;
}

MultiRenderer_slang_comp::MemLoc MultiRenderer_slang_comp::AllocAndBind(const std::vector<VkImage>& a_images, VkMemoryAllocateFlags a_flags)
{
  MemLoc currLoc;
  if(a_images.size() > 0)
  {
    std::vector<VkMemoryRequirements> reqs(a_images.size());
    for(size_t i=0; i<reqs.size(); i++)
      vkGetImageMemoryRequirements(m_device, a_images[i], &reqs[i]);

    for(size_t i=0; i<reqs.size(); i++)
    {
      if(reqs[i].memoryTypeBits != reqs[0].memoryTypeBits)
      {
        std::cout << "MultiRenderer_slang_comp::AllocAndBind(textures): memoryTypeBits warning, need to split mem allocation (override me)" << std::endl;
        break;
      }
    }

    auto offsets  = vk_utils::calculateMemOffsets(reqs);
    auto memTotal = offsets[offsets.size() - 1];

    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.pNext           = nullptr;
    allocateInfo.allocationSize  = memTotal;
    allocateInfo.memoryTypeIndex = vk_utils::findMemoryType(reqs[0].memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_physicalDevice);
    VK_CHECK_RESULT(vkAllocateMemory(m_device, &allocateInfo, NULL, &currLoc.memObject));

    for(size_t i=0;i<a_images.size();i++) {
      VK_CHECK_RESULT(vkBindImageMemory(m_device, a_images[i], currLoc.memObject, offsets[i]));
    }

    currLoc.allocId = m_allMems.size();
    m_allMems.push_back(currLoc);
  }
  return currLoc;
}

void MultiRenderer_slang_comp::FreeAllAllocations(std::vector<MemLoc>& a_memLoc)
{
  // in general you may check 'mem.allocId' for unique to be sure you dont free mem twice
  // for default implementation this is not needed
  for(auto mem : a_memLoc)
    vkFreeMemory(m_device, mem.memObject, nullptr);
  a_memLoc.resize(0);
}

void MultiRenderer_slang_comp::AllocMemoryForMemberBuffersAndImages(const std::vector<VkBuffer>& a_buffers, const std::vector<VkImage>& a_images)
{
  std::vector<VkMemoryRequirements> bufMemReqs(a_buffers.size()); // we must check that all buffers have same memoryTypeBits;
  for(size_t i = 0; i < a_buffers.size(); ++i)                    // if not, split to multiple allocations
  {
    if(a_buffers[i] != VK_NULL_HANDLE)
      vkGetBufferMemoryRequirements(m_device, a_buffers[i], &bufMemReqs[i]);
    else
    {
      bufMemReqs[i] = bufMemReqs[0];
      bufMemReqs[i].size = 0;
    }
  }

  bool needSplit = false;
  for(size_t i = 1; i < bufMemReqs.size(); ++i)
  {
    if(bufMemReqs[i].memoryTypeBits != bufMemReqs[0].memoryTypeBits)
    {
      needSplit = true;
      break;
    }
  }

  if(needSplit)
  {
    std::unordered_map<uint32_t, std::vector<uint32_t> > bufferSets;
    for(uint32_t j = 0; j < uint32_t(bufMemReqs.size()); ++j)
    {
      uint32_t key = uint32_t(bufMemReqs[j].memoryTypeBits);
      bufferSets[key].push_back(j);
    }

    for(const auto& buffGroup : bufferSets)
    {
      std::vector<VkBuffer> currGroup;
      for(auto id : buffGroup.second)
        currGroup.push_back(a_buffers[id]);
      AllocAndBind(currGroup);
    }
  }
  else
    AllocAndBind(a_buffers);

  std::vector<VkFormat>             formats;  formats.reserve(0);
  std::vector<VkImageView*>         views;    views.reserve(0);
  std::vector<VkImage>              textures; textures.reserve(0);
  VkMemoryRequirements memoryRequirements;

  for(size_t i=0;i< m_vdata.m_texturesArrayTexture.size(); i++)
  {
    formats.push_back (VkFormat(m_textures[i]->format()));
    views.push_back   (&m_vdata.m_texturesArrayView[i]);
    textures.push_back(m_vdata.m_texturesArrayTexture[i]);
  }

  AllocAndBind(textures);
  for(size_t i=0;i<textures.size();i++)
  {
    VkImageViewCreateInfo imageViewInfo = {};
    imageViewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.flags                           = 0;
    imageViewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format                          = formats[i];
    if(imageViewInfo.format == VK_FORMAT_R32_SFLOAT || imageViewInfo.format == VK_FORMAT_R8_UNORM  || imageViewInfo.format == VK_FORMAT_R8_SNORM ||
       imageViewInfo.format == VK_FORMAT_R16_SFLOAT || imageViewInfo.format == VK_FORMAT_R16_UNORM || imageViewInfo.format == VK_FORMAT_R16_SNORM)
      imageViewInfo.components                    = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R }; 
    else
      imageViewInfo.components                    = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    imageViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewInfo.subresourceRange.baseMipLevel   = 0;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount     = 1;
    imageViewInfo.subresourceRange.levelCount     = 1;
    imageViewInfo.image                           = textures[i];     // The view will be based on the texture's image
    VK_CHECK_RESULT(vkCreateImageView(m_device, &imageViewInfo, nullptr, views[i]));
  }
}

VkPhysicalDeviceFeatures2 MultiRenderer_slang_comp::ListRequiredDeviceFeatures(std::vector<const char*>& deviceExtensions)
{
  static VkPhysicalDeviceFeatures2 features2 = {};
  features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features2.pNext = nullptr;
  features2.features.shaderInt64   = false;
  features2.features.shaderFloat64 = false;
  features2.features.shaderInt16   = false;
  
  void** ppNext = &features2.pNext;
  static VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures = {};
  indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
  indexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
  indexingFeatures.runtimeDescriptorArray                    = VK_TRUE;
  (*ppNext) = &indexingFeatures; ppNext = &indexingFeatures.pNext;
  deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

  return features2;
}

MultiRenderer_slang_comp::MegaKernelIsEnabled MultiRenderer_slang_comp::m_megaKernelFlags;

