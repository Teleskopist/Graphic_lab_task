#include <cstdint>
#include <vector>
#include <array>
#include <memory>
#include <limits>
#include <cassert>
#include "vk_copy.h"
#include "vk_context.h"
#include "VolumeRenderer_slang.h"


std::shared_ptr<VolumeRenderer> CreateVolumeRenderer_slang(vk_utils::VulkanContext a_ctx, size_t a_maxThreadsGenerated)
{
  auto pObj = std::make_shared<VolumeRenderer_slang>();
  pObj->SetVulkanContext(a_ctx);
  pObj->InitVulkanObjects(a_ctx.device, a_ctx.physicalDevice, a_maxThreadsGenerated);
  return pObj;
}

vk_utils::VulkanDeviceFeatures VolumeRenderer_slang_ListRequiredDeviceFeatures()
{
  vk_utils::VulkanDeviceFeatures res;
  res.features2 = VolumeRenderer_slang::ListRequiredDeviceFeatures(res.extensionNames);
  res.apiVersion = VK_API_VERSION_1_2;
  return res;
}

void VolumeRenderer_slang::InitVulkanObjects(VkDevice a_device, VkPhysicalDevice a_physicalDevice, size_t a_maxThreadsCount)
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

VkBufferUsageFlags VolumeRenderer_slang::GetAdditionalFlagsForUBO() const
{
  return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
}

uint32_t VolumeRenderer_slang::GetDefaultMaxTextures() const { return 256; }

void VolumeRenderer_slang::MakeComputePipelineAndLayout(const char* a_shaderPath, const char* a_mainName, const VkSpecializationInfo *a_specInfo, const VkDescriptorSetLayout a_dsLayout, VkPipelineLayout* pPipelineLayout, VkPipeline* pPipeline)
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

void VolumeRenderer_slang::MakeComputePipelineOnly(const char* a_shaderPath, const char* a_mainName, const VkSpecializationInfo *a_specInfo, const VkDescriptorSetLayout a_dsLayout, VkPipelineLayout pipelineLayout, VkPipeline* pPipeline)
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

void VolumeRenderer_slang::DeleteDeviceData()
{
  if(m_commitCount == 0)
    return;
  vkDestroyBuffer(m_device, m_classDataBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_DAGInverseTransformsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_RotAddTableBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SComNodesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SComValuesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SdfCompactOctreeRotModifiersBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SdfDAGChildEdgesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SdfDAGDataEdgesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SdfDAGDistancesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SdfDAGHeadersBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SdfDAGNodesBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SdfDAGTranspositionsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_SphericalHarmonicsBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_colorBufferBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_colored_gridBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_gridBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_packedXYBuffer, nullptr);
  vkDestroyBuffer(m_device, m_vdata.m_timestampsBuffer, nullptr);
  if(copyKernelFloatDSLayout != VK_NULL_HANDLE)
     vkDestroyDescriptorSetLayout(m_device, copyKernelFloatDSLayout, nullptr);
  FreeAllAllocations(m_allMems);
}

VolumeRenderer_slang::~VolumeRenderer_slang()
{
  for(size_t i=0;i<m_allCreatedPipelines.size();i++)
    vkDestroyPipeline(m_device, m_allCreatedPipelines[i], nullptr);
  for(size_t i=0;i<m_allCreatedPipelineLayouts.size();i++)
    vkDestroyPipelineLayout(m_device, m_allCreatedPipelineLayouts[i], nullptr);
  vkDestroyDescriptorSetLayout(m_device, RaymarchGrid_BresenhamDSLayout, nullptr);
  RaymarchGrid_BresenhamDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, RaymarchGrid_DDADSLayout, nullptr);
  RaymarchGrid_DDADSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, RaymarchGrid_DDA_BranchlessDSLayout, nullptr);
  RaymarchGrid_DDA_BranchlessDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, AverageColorDSLayout, nullptr);
  AverageColorDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, RaymarchSCom4DDSLayout, nullptr);
  RaymarchSCom4DDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, RaytraceGrid_SSDSLayout, nullptr);
  RaytraceGrid_SSDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, RaymarchSComDSLayout, nullptr);
  RaymarchSComDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, TonemapDSLayout, nullptr);
  TonemapDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, RaymarchDAGDSLayout, nullptr);
  RaymarchDAGDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, PackXYDSLayout, nullptr);
  PackXYDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(m_device, RaymarchDAG4DDSLayout, nullptr);
  RaymarchDAG4DDSLayout = VK_NULL_HANDLE;
  vkDestroyDescriptorPool(m_device, m_dsPool, NULL); m_dsPool = VK_NULL_HANDLE;
  DeleteDeviceData();
}

void VolumeRenderer_slang::InitHelpers()
{
  vkGetPhysicalDeviceProperties(m_physicalDevice, &m_devProps);
}


void VolumeRenderer_slang::InitKernel_RaymarchGrid_Bresenham(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang/kernel1D_RaymarchGrid_Bresenham.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  RaymarchGrid_BresenhamDSLayout = CreateRaymarchGrid_BresenhamDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, RaymarchGrid_BresenhamDSLayout, &RaymarchGrid_BresenhamLayout, &RaymarchGrid_BresenhamPipeline);
  }
  else
  {
    RaymarchGrid_BresenhamLayout   = nullptr;
    RaymarchGrid_BresenhamPipeline = nullptr;
  }
}

void VolumeRenderer_slang::InitKernel_RaymarchGrid_DDA(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang/kernel1D_RaymarchGrid_DDA.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  RaymarchGrid_DDADSLayout = CreateRaymarchGrid_DDADSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, RaymarchGrid_DDADSLayout, &RaymarchGrid_DDALayout, &RaymarchGrid_DDAPipeline);
  }
  else
  {
    RaymarchGrid_DDALayout   = nullptr;
    RaymarchGrid_DDAPipeline = nullptr;
  }
}

void VolumeRenderer_slang::InitKernel_RaymarchGrid_DDA_Branchless(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang/kernel1D_RaymarchGrid_DDA_Branchless.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  RaymarchGrid_DDA_BranchlessDSLayout = CreateRaymarchGrid_DDA_BranchlessDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, RaymarchGrid_DDA_BranchlessDSLayout, &RaymarchGrid_DDA_BranchlessLayout, &RaymarchGrid_DDA_BranchlessPipeline);
  }
  else
  {
    RaymarchGrid_DDA_BranchlessLayout   = nullptr;
    RaymarchGrid_DDA_BranchlessPipeline = nullptr;
  }
}

void VolumeRenderer_slang::InitKernel_AverageColor(const char* a_filePath)
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

void VolumeRenderer_slang::InitKernel_RaymarchSCom4D(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang/kernel1D_RaymarchSCom4D.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  RaymarchSCom4DDSLayout = CreateRaymarchSCom4DDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, RaymarchSCom4DDSLayout, &RaymarchSCom4DLayout, &RaymarchSCom4DPipeline);
  }
  else
  {
    RaymarchSCom4DLayout   = nullptr;
    RaymarchSCom4DPipeline = nullptr;
  }
}

void VolumeRenderer_slang::InitKernel_RaytraceGrid_SS(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang/kernel1D_RaytraceGrid_SS.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  RaytraceGrid_SSDSLayout = CreateRaytraceGrid_SSDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, RaytraceGrid_SSDSLayout, &RaytraceGrid_SSLayout, &RaytraceGrid_SSPipeline);
  }
  else
  {
    RaytraceGrid_SSLayout   = nullptr;
    RaytraceGrid_SSPipeline = nullptr;
  }
}

void VolumeRenderer_slang::InitKernel_RaymarchSCom(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang/kernel1D_RaymarchSCom.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  RaymarchSComDSLayout = CreateRaymarchSComDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, RaymarchSComDSLayout, &RaymarchSComLayout, &RaymarchSComPipeline);
  }
  else
  {
    RaymarchSComLayout   = nullptr;
    RaymarchSComPipeline = nullptr;
  }
}

void VolumeRenderer_slang::InitKernel_Tonemap(const char* a_filePath)
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

void VolumeRenderer_slang::InitKernel_RaymarchDAG(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang/kernel1D_RaymarchDAG.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  RaymarchDAGDSLayout = CreateRaymarchDAGDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, RaymarchDAGDSLayout, &RaymarchDAGLayout, &RaymarchDAGPipeline);
  }
  else
  {
    RaymarchDAGLayout   = nullptr;
    RaymarchDAGPipeline = nullptr;
  }
}

void VolumeRenderer_slang::InitKernel_PackXY(const char* a_filePath)
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

void VolumeRenderer_slang::InitKernel_RaymarchDAG4D(const char* a_filePath)
{
  std::string shaderPath = AlterShaderPath("shaders_slang/kernel1D_RaymarchDAG4D.comp.spv");
  const VkSpecializationInfo* kspec = nullptr;
  RaymarchDAG4DDSLayout = CreateRaymarchDAG4DDSLayout();
  if(true)
  {
    MakeComputePipelineAndLayout(shaderPath.c_str(), "main", kspec, RaymarchDAG4DDSLayout, &RaymarchDAG4DLayout, &RaymarchDAG4DPipeline);
  }
  else
  {
    RaymarchDAG4DLayout   = nullptr;
    RaymarchDAG4DPipeline = nullptr;
  }
}


void VolumeRenderer_slang::InitKernels(const char* a_filePath)
{
  InitKernel_RaymarchGrid_Bresenham(a_filePath);
  InitKernel_RaymarchGrid_DDA(a_filePath);
  InitKernel_RaymarchGrid_DDA_Branchless(a_filePath);
  InitKernel_AverageColor(a_filePath);
  InitKernel_RaymarchSCom4D(a_filePath);
  InitKernel_RaytraceGrid_SS(a_filePath);
  InitKernel_RaymarchSCom(a_filePath);
  InitKernel_Tonemap(a_filePath);
  InitKernel_RaymarchDAG(a_filePath);
  InitKernel_PackXY(a_filePath);
  InitKernel_RaymarchDAG4D(a_filePath);
}

void VolumeRenderer_slang::InitBuffers(size_t a_maxThreadsCount, bool a_tempBuffersOverlay)
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

void VolumeRenderer_slang::ReserveEmptyVectors()
{
  if(m_DAGInverseTransforms.capacity() == 0)
    m_DAGInverseTransforms.reserve(4);
  if(m_RotAddTable.capacity() == 0)
    m_RotAddTable.reserve(4);
  if(m_SComNodes.capacity() == 0)
    m_SComNodes.reserve(4);
  if(m_SComValues.capacity() == 0)
    m_SComValues.reserve(4);
  if(m_SdfCompactOctreeRotModifiers.capacity() == 0)
    m_SdfCompactOctreeRotModifiers.reserve(4);
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
  if(m_SdfDAGTranspositions.capacity() == 0)
    m_SdfDAGTranspositions.reserve(4);
  if(m_SphericalHarmonics.capacity() == 0)
    m_SphericalHarmonics.reserve(4);
  if(m_colorBuffer.capacity() == 0)
    m_colorBuffer.reserve(4);
  if(m_colored_grid.capacity() == 0)
    m_colored_grid.reserve(4);
  if(m_grid.capacity() == 0)
    m_grid.reserve(4);
  if(m_packedXY.capacity() == 0)
    m_packedXY.reserve(4);
  if(m_timestamps.capacity() == 0)
    m_timestamps.reserve(4);
}

void VolumeRenderer_slang::InitDeviceData()
{
  std::vector<VkBuffer> memberVectorsWithDevAddr;
  std::vector<VkBuffer> memberVectors;
  std::vector<VkImage>  memberTextures;
  m_classDataBuffer = vk_utils::createBuffer(m_device, sizeof(m_uboData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | GetAdditionalFlagsForUBO() | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_classDataBuffer);
  m_vdata.m_DAGInverseTransformsBuffer = vk_utils::createBuffer(m_device, m_DAGInverseTransforms.capacity()*sizeof(float4x4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_DAGInverseTransformsBuffer);
  m_vdata.m_RotAddTableBuffer = vk_utils::createBuffer(m_device, m_RotAddTable.capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_RotAddTableBuffer);
  m_vdata.m_SComNodesBuffer = vk_utils::createBuffer(m_device, m_SComNodes.capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_SComNodesBuffer);
  m_vdata.m_SComValuesBuffer = vk_utils::createBuffer(m_device, m_SComValues.capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_SComValuesBuffer);
  m_vdata.m_SdfCompactOctreeRotModifiersBuffer = vk_utils::createBuffer(m_device, m_SdfCompactOctreeRotModifiers.capacity()*sizeof(int4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_SdfCompactOctreeRotModifiersBuffer);
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
  m_vdata.m_SdfDAGTranspositionsBuffer = vk_utils::createBuffer(m_device, m_SdfDAGTranspositions.capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_SdfDAGTranspositionsBuffer);
  m_vdata.m_SphericalHarmonicsBuffer = vk_utils::createBuffer(m_device, m_SphericalHarmonics.capacity()*sizeof(float4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_SphericalHarmonicsBuffer);
  m_vdata.m_colorBufferBuffer = vk_utils::createBuffer(m_device, m_colorBuffer.capacity()*sizeof(float4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_colorBufferBuffer);
  m_vdata.m_colored_gridBuffer = vk_utils::createBuffer(m_device, m_colored_grid.capacity()*sizeof(float4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_colored_gridBuffer);
  m_vdata.m_gridBuffer = vk_utils::createBuffer(m_device, m_grid.capacity()*sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_gridBuffer);
  m_vdata.m_packedXYBuffer = vk_utils::createBuffer(m_device, m_packedXY.capacity()*sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_packedXYBuffer);
  m_vdata.m_timestampsBuffer = vk_utils::createBuffer(m_device, m_timestamps.capacity()*sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  memberVectors.push_back(m_vdata.m_timestampsBuffer);
  


  AllocMemoryForMemberBuffersAndImages(memberVectors, memberTextures);
  if(memberVectorsWithDevAddr.size() != 0)
    AllocAndBind(memberVectorsWithDevAddr, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
}




void VolumeRenderer_slang::AssignBuffersToMemory(const std::vector<VkBuffer>& a_buffers, VkDeviceMemory a_mem)
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
      std::cout << "[VolumeRenderer_slang::AssignBuffersToMemory]: error, input buffers has different 'memReq.memoryTypeBits'" << std::endl;
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

VolumeRenderer_slang::MemLoc VolumeRenderer_slang::AllocAndBind(const std::vector<VkBuffer>& a_buffers, VkMemoryAllocateFlags a_flags)
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

VolumeRenderer_slang::MemLoc VolumeRenderer_slang::AllocAndBind(const std::vector<VkImage>& a_images, VkMemoryAllocateFlags a_flags)
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
        std::cout << "VolumeRenderer_slang::AllocAndBind(textures): memoryTypeBits warning, need to split mem allocation (override me)" << std::endl;
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

void VolumeRenderer_slang::FreeAllAllocations(std::vector<MemLoc>& a_memLoc)
{
  // in general you may check 'mem.allocId' for unique to be sure you dont free mem twice
  // for default implementation this is not needed
  for(auto mem : a_memLoc)
    vkFreeMemory(m_device, mem.memObject, nullptr);
  a_memLoc.resize(0);
}

void VolumeRenderer_slang::AllocMemoryForMemberBuffersAndImages(const std::vector<VkBuffer>& a_buffers, const std::vector<VkImage>& a_images)
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

}

VkPhysicalDeviceFeatures2 VolumeRenderer_slang::ListRequiredDeviceFeatures(std::vector<const char*>& deviceExtensions)
{
  static VkPhysicalDeviceFeatures2 features2 = {};
  features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features2.pNext = nullptr;
  features2.features.shaderInt64   = false;
  features2.features.shaderFloat64 = false;
  features2.features.shaderInt16   = false;
  
  void** ppNext = &features2.pNext;
  return features2;
}

VolumeRenderer_slang::MegaKernelIsEnabled VolumeRenderer_slang::m_megaKernelFlags;

