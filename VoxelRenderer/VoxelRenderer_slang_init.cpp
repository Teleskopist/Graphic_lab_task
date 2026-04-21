#include <cstdint>
#include <vector>
#include <array>
#include <memory>
#include <limits>
#include <cassert>
#include "vk_copy.h"
#include "vk_context.h"
#include "VoxelRenderer_slang.h"


std::shared_ptr<VoxelRenderer> CreateVoxelRenderer_slang(vk_utils::VulkanContext a_ctx, size_t a_maxThreadsGenerated)
{
  auto pObj = std::make_shared<VoxelRenderer_slang>();
  pObj->SetVulkanContext(a_ctx);
  pObj->InitVulkanObjects(a_ctx.device, a_ctx.physicalDevice, a_maxThreadsGenerated);
  return pObj;
}

vk_utils::VulkanDeviceFeatures VoxelRenderer_slang_ListRequiredDeviceFeatures()
{
  vk_utils::VulkanDeviceFeatures res;
  res.features2 = VoxelRenderer_slang::ListRequiredDeviceFeatures(res.extensionNames);
  res.apiVersion = VK_API_VERSION_1_2;
  return res;
}

void VoxelRenderer_slang::InitVulkanObjects(VkDevice a_device, VkPhysicalDevice a_physicalDevice, size_t a_maxThreadsCount)
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

VkBufferUsageFlags VoxelRenderer_slang::GetAdditionalFlagsForUBO() const
{
  return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
}

uint32_t VoxelRenderer_slang::GetDefaultMaxTextures() const { return 256; }

void VoxelRenderer_slang::MakeComputePipelineAndLayout(const char* a_shaderPath, const char* a_mainName, const VkSpecializationInfo *a_specInfo, const VkDescriptorSetLayout a_dsLayout, VkPipelineLayout* pPipelineLayout, VkPipeline* pPipeline)
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

void VoxelRenderer_slang::MakeComputePipelineOnly(const char* a_shaderPath, const char* a_mainName, const VkSpecializationInfo *a_specInfo, const VkDescriptorSetLayout a_dsLayout, VkPipelineLayout pipelineLayout, VkPipeline* pPipeline)
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

void VoxelRenderer_slang::DeleteDeviceData()
{
  if(m_commitCount == 0)
    return;
  vkDestroyBuffer(m_device, m_classDataBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_RotAddTableBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SComNodesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SVOBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SdfCompactOctreeRotModifiersBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SdfDAGBlockIdsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SdfDAGChildEdgesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SdfDAGDataEdgesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SdfDAGDistancesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SdfDAGHeadersBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SdfDAGNodesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_blocksBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_colorBufferBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_lightsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_packedXYBuffer, nullptr);
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

VoxelRenderer_slang::~VoxelRenderer_slang()
{
  for(size_t i=0;i<m_allCreatedPipelines.size();i++)
    vkDestroyPipeline(m_device, m_allCreatedPipelines[i], nullptr);
  for(size_t i=0;i<m_allCreatedPipelineLayouts.size();i++)
    vkDestroyPipelineLayout(m_device, m_allCreatedPipelineLayouts[i], nullptr);
  vkDestroyDescriptorSetLayout(m_device, PackXYDSLayout, nullptr);
  PackXYDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, AverageColorDSLayout, nullptr);
  AverageColorDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, IntersectSVOFastDSLayout, nullptr);
  IntersectSVOFastDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, IntersectDAGDSLayout, nullptr);
  IntersectDAGDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, TonemapDSLayout, nullptr);
  TonemapDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, IntersectSComDSLayout, nullptr);
  IntersectSComDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, IntersectSVODSLayout, nullptr);
  IntersectSVODSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorPool(m_device, m_dsPool, NULL); m_dsPool = VK_NULL_HANDLE;
  DeleteDeviceData();
}

void VoxelRenderer_slang::InitHelpers()
{
  vkGetPhysicalDeviceProperties(m_physicalDevice, &m_devProps);
}


void VoxelRenderer_slang::InitKernel_PackXY(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang/kernel2D_PackXY.comp.spv");
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

void VoxelRenderer_slang::InitKernel_AverageColor(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang/kernel1D_AverageColor.comp.spv");
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

void VoxelRenderer_slang::InitKernel_IntersectSVOFast(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang/kernel1D_IntersectSVOFast.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  IntersectSVOFastDSLayout = CreateIntersectSVOFastDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, IntersectSVOFastDSLayout, &IntersectSVOFastLayout, &IntersectSVOFastPipeline);
  }
  else
  {
    IntersectSVOFastLayout   = nullptr;
    IntersectSVOFastPipeline = nullptr;
  }
}

void VoxelRenderer_slang::InitKernel_IntersectDAG(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang/kernel1D_IntersectDAG.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  IntersectDAGDSLayout = CreateIntersectDAGDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, IntersectDAGDSLayout, &IntersectDAGLayout, &IntersectDAGPipeline);
  }
  else
  {
    IntersectDAGLayout   = nullptr;
    IntersectDAGPipeline = nullptr;
  }
}

void VoxelRenderer_slang::InitKernel_Tonemap(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang/kernel1D_Tonemap.comp.spv");
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

void VoxelRenderer_slang::InitKernel_IntersectSCom(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang/kernel1D_IntersectSCom.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  IntersectSComDSLayout = CreateIntersectSComDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, IntersectSComDSLayout, &IntersectSComLayout, &IntersectSComPipeline);
  }
  else
  {
    IntersectSComLayout   = nullptr;
    IntersectSComPipeline = nullptr;
  }
}

void VoxelRenderer_slang::InitKernel_IntersectSVO(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang/kernel1D_IntersectSVO.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  IntersectSVODSLayout = CreateIntersectSVODSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, IntersectSVODSLayout, &IntersectSVOLayout, &IntersectSVOPipeline);
  }
  else
  {
    IntersectSVOLayout   = nullptr;
    IntersectSVOPipeline = nullptr;
  }
}


void VoxelRenderer_slang::InitKernels(const char* a_filePath)
{
  InitKernel_PackXY(a_filePath);
  InitKernel_AverageColor(a_filePath);
  InitKernel_IntersectSVOFast(a_filePath);
  InitKernel_IntersectDAG(a_filePath);
  InitKernel_Tonemap(a_filePath);
  InitKernel_IntersectSCom(a_filePath);
  InitKernel_IntersectSVO(a_filePath);
}

void VoxelRenderer_slang::InitBuffers(size_t a_maxThreadsCount, bool a_tempBuffersOverlay)
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

void VoxelRenderer_slang::ReserveEmptyVectors()
{
  if(m_RotAddTable.capacity() == 0)
    m_RotAddTable.reserve(4);
  if(m_SComNodes.capacity() == 0)
    m_SComNodes.reserve(4);
  if(m_SVO.capacity() == 0)
    m_SVO.reserve(4);
  if(m_SdfCompactOctreeRotModifiers.capacity() == 0)
    m_SdfCompactOctreeRotModifiers.reserve(4);
  if(m_SdfDAGBlockIds.capacity() == 0)
    m_SdfDAGBlockIds.reserve(4);
  if(m_SdfDAGChildEdges.capacity() == 0)
    m_SdfDAGChildEdges.reserve(4);
  if(m_SdfDAGDataEdges.capacity() == 0)
    m_SdfDAGDataEdges.reserve(4);
  if(m_SdfDAGDistances.capacity() == 0)
    m_SdfDAGDistances.reserve(4);
  if(m_SdfDAGHeaders.capacity() == 0)
    m_SdfDAGHeaders.reserve(4);
  if(m_SdfDAGNodes.capacity() == 0)
    m_SdfDAGNodes.reserve(4);
  if(m_blocks.capacity() == 0)
    m_blocks.reserve(4);
  if(m_colorBuffer.capacity() == 0)
    m_colorBuffer.reserve(4);
  if(m_lights.capacity() == 0)
    m_lights.reserve(4);
  if(m_packedXY.capacity() == 0)
    m_packedXY.reserve(4);
}

void VoxelRenderer_slang::InitDeviceData()
{
  std::vector<VkBuffer> memberVectorsWithDevAddr;
  std::vector<VkBuffer> memberVectors;
  std::vector<VkImage>  memberTextures;
  m_classDataBuffer = vk_utils::createBuffer(m_device, sizeof(m_uboData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | GetAdditionalFlagsForUBO() | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_classDataBuffer);
  m_vdata.m_RotAddTableBuffer = vk_utils::createBuffer(m_device, m_RotAddTable.capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_RotAddTableBuffer);
  m_vdata.m_SComNodesBuffer = vk_utils::createBuffer(m_device, m_SComNodes.capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_SComNodesBuffer);
  m_vdata.m_SVOBuffer = vk_utils::createBuffer(m_device, m_SVO.capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_SVOBuffer);
  m_vdata.m_SdfCompactOctreeRotModifiersBuffer = vk_utils::createBuffer(m_device, m_SdfCompactOctreeRotModifiers.capacity()*sizeof(int4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_SdfCompactOctreeRotModifiersBuffer);
  m_vdata.m_SdfDAGBlockIdsBuffer = vk_utils::createBuffer(m_device, m_SdfDAGBlockIds.capacity()*sizeof(int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_SdfDAGBlockIdsBuffer);
  m_vdata.m_SdfDAGChildEdgesBuffer = vk_utils::createBuffer(m_device, m_SdfDAGChildEdges.capacity()*sizeof(SdfDAGChildEdge), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_SdfDAGChildEdgesBuffer);
  m_vdata.m_SdfDAGDataEdgesBuffer = vk_utils::createBuffer(m_device, m_SdfDAGDataEdges.capacity()*sizeof(SdfDAGDataEdge), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_SdfDAGDataEdgesBuffer);
  m_vdata.m_SdfDAGDistancesBuffer = vk_utils::createBuffer(m_device, m_SdfDAGDistances.capacity()*sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_SdfDAGDistancesBuffer);
  m_vdata.m_SdfDAGHeadersBuffer = vk_utils::createBuffer(m_device, m_SdfDAGHeaders.capacity()*sizeof(SdfDAGHeader), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_SdfDAGHeadersBuffer);
  m_vdata.m_SdfDAGNodesBuffer = vk_utils::createBuffer(m_device, m_SdfDAGNodes.capacity()*sizeof(SdfDAGNode), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_SdfDAGNodesBuffer);
  m_vdata.m_blocksBuffer = vk_utils::createBuffer(m_device, m_blocks.capacity()*sizeof(BlockMetadata), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_blocksBuffer);
  m_vdata.m_colorBufferBuffer = vk_utils::createBuffer(m_device, m_colorBuffer.capacity()*sizeof(float4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_colorBufferBuffer);
  m_vdata.m_lightsBuffer = vk_utils::createBuffer(m_device, m_lights.capacity()*sizeof(Light), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_lightsBuffer);
  m_vdata.m_packedXYBuffer = vk_utils::createBuffer(m_device, m_packedXY.capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_packedXYBuffer);
  
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



VkImage VoxelRenderer_slang::CreateTexture2D(const int a_width, const int a_height, VkFormat a_format, VkImageUsageFlags a_usage)
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

VkSampler VoxelRenderer_slang::CreateSampler(const Sampler& a_sampler) // TODO: implement this function correctly
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

VkImageView VoxelRenderer_slang::CreateView(VkFormat a_format, VkImage a_image)
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




void VoxelRenderer_slang::AssignBuffersToMemory(const std::vector<VkBuffer>& a_buffers, VkDeviceMemory a_mem)
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
      std::cout << "[VoxelRenderer_slang::AssignBuffersToMemory]: error, input buffers has different 'memReq.memoryTypeBits'" << std::endl;
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

VoxelRenderer_slang::MemLoc VoxelRenderer_slang::AllocAndBind(const std::vector<VkBuffer>& a_buffers, VkMemoryAllocateFlags a_flags)
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

VoxelRenderer_slang::MemLoc VoxelRenderer_slang::AllocAndBind(const std::vector<VkImage>& a_images, VkMemoryAllocateFlags a_flags)
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
        std::cout << "VoxelRenderer_slang::AllocAndBind(textures): memoryTypeBits warning, need to split mem allocation (override me)" << std::endl;
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

void VoxelRenderer_slang::FreeAllAllocations(std::vector<MemLoc>& a_memLoc)
{
  // in general you may check 'mem.allocId' for unique to be sure you dont free mem twice
  // for default implementation this is not needed
  for(auto mem : a_memLoc)
    vkFreeMemory(m_device, mem.memObject, nullptr);
  a_memLoc.resize(0);
}

void VoxelRenderer_slang::AllocMemoryForMemberBuffersAndImages(const std::vector<VkBuffer>& a_buffers, const std::vector<VkImage>& a_images)
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

VkPhysicalDeviceFeatures2 VoxelRenderer_slang::ListRequiredDeviceFeatures(std::vector<const char*>& deviceExtensions)
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

VoxelRenderer_slang::MegaKernelIsEnabled VoxelRenderer_slang::m_megaKernelFlags;

