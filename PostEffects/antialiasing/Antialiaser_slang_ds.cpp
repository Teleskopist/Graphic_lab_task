#include <vector>
#include <array>
#include <memory>
#include <limits>

#include <cassert>
#include "vk_copy.h"
#include "vk_context.h"

#include "Antialiaser_slang.h"


void Antialiaser_slang::AllocateAllDescriptorSets()
{
  // allocate pool
  //
  VkDescriptorPoolSize buffersSize, combinedImageSamSize, imageStorageSize, accelStorageSize, dynamicBuffersSize;
  buffersSize.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  buffersSize.descriptorCount = 8 + 64 ; //

  std::vector<VkDescriptorPoolSize> poolSizes = {buffersSize};
  {
    combinedImageSamSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    combinedImageSamSize.descriptorCount = 0*GetDefaultMaxTextures() + 0;
    imageStorageSize.type                = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imageStorageSize.descriptorCount     = 0;
    accelStorageSize.type                = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    accelStorageSize.descriptorCount     = 0 ;

    if(combinedImageSamSize.descriptorCount > 0)
      poolSizes.push_back(combinedImageSamSize);
    if(imageStorageSize.descriptorCount > 0)
      poolSizes.push_back(imageStorageSize);
    if(accelStorageSize.descriptorCount > 0)
      poolSizes.push_back(accelStorageSize);
    
  }


  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
  descriptorPoolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolCreateInfo.maxSets       = 2 + 2; // add 1 to prevent zero case and one more for internal needs
  descriptorPoolCreateInfo.poolSizeCount = poolSizes.size();
  descriptorPoolCreateInfo.pPoolSizes    = poolSizes.data();

  VK_CHECK_RESULT(vkCreateDescriptorPool(m_device, &descriptorPoolCreateInfo, NULL, &m_dsPool));

  // allocate all descriptor sets
  //
  VkDescriptorSetLayout layouts[2] = {};
  layouts[0] = blendDSLayout;
  layouts[1] = blendDSLayout;

  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
  descriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorSetAllocateInfo.descriptorPool     = m_dsPool;
  descriptorSetAllocateInfo.descriptorSetCount = 2;
  descriptorSetAllocateInfo.pSetLayouts        = layouts;

  auto tmpRes = vkAllocateDescriptorSets(m_device, &descriptorSetAllocateInfo, m_allGeneratedDS);
  VK_CHECK_RESULT(tmpRes);
}

VkDescriptorSetLayout Antialiaser_slang::CreateblendDSLayout()
{
  std::array<VkDescriptorSetLayoutBinding, 4+1> dsBindings;
  
  const auto stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // binding for in_data
  dsBindings[0].binding            = 0;
  dsBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[0].descriptorCount    = 1;
  dsBindings[0].stageFlags         = stageFlags;
  dsBindings[0].pImmutableSamplers = nullptr;

  // binding for out_data
  dsBindings[1].binding            = 1;
  dsBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[1].descriptorCount    = 1;
  dsBindings[1].stageFlags         = stageFlags;
  dsBindings[1].pImmutableSamplers = nullptr;

  // binding for m_AABuffer
  dsBindings[2].binding            = 2;
  dsBindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[2].descriptorCount    = 1;
  dsBindings[2].stageFlags         = stageFlags;
  dsBindings[2].pImmutableSamplers = nullptr;

  // binding for m_AABuffer1
  dsBindings[3].binding            = 3;
  dsBindings[3].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[3].descriptorCount    = 1;
  dsBindings[3].stageFlags         = stageFlags;
  dsBindings[3].pImmutableSamplers = nullptr;

  dsBindings[4].binding            = 4;
  dsBindings[4].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
  dsBindings[4].descriptorCount    = 1;
  dsBindings[4].stageFlags         = stageFlags;
  dsBindings[4].pImmutableSamplers = nullptr;


  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
  descriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount = uint32_t(dsBindings.size());
  descriptorSetLayoutCreateInfo.pBindings    = dsBindings.data();

  VkDescriptorSetLayout layout = nullptr;
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, NULL, &layout));
  return layout;
}

VkDescriptorSetLayout Antialiaser_slang::CreatecopyKernelFloatDSLayout()
{
  std::array<VkDescriptorSetLayoutBinding, 2> dsBindings;

  dsBindings[0].binding            = 0;
  dsBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[0].descriptorCount    = 1;
  dsBindings[0].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
  dsBindings[0].pImmutableSamplers = nullptr;

  dsBindings[1].binding            = 1;
  dsBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[1].descriptorCount    = 1;
  dsBindings[1].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
  dsBindings[1].pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
  descriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount = dsBindings.size();
  descriptorSetLayoutCreateInfo.pBindings    = dsBindings.data();

  VkDescriptorSetLayout layout = nullptr;
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, NULL, &layout));
  return layout;
}

VkDescriptorSetLayout Antialiaser_slang::CreatematMulTransposeDSLayout()
{
  std::array<VkDescriptorSetLayoutBinding, 3> dsBindings;

  dsBindings[0].binding            = 0;
  dsBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[0].descriptorCount    = 1;
  dsBindings[0].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
  dsBindings[0].pImmutableSamplers = nullptr;

  dsBindings[1].binding            = 1;
  dsBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[1].descriptorCount    = 1;
  dsBindings[1].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
  dsBindings[1].pImmutableSamplers = nullptr;

  dsBindings[2].binding            = 2;
  dsBindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[2].descriptorCount    = 1;
  dsBindings[2].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
  dsBindings[2].pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
  descriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount = dsBindings.size();
  descriptorSetLayoutCreateInfo.pBindings    = dsBindings.data();

  VkDescriptorSetLayout layout = nullptr;
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, NULL, &layout));
  return layout;
}

void Antialiaser_slang::UpdateAllGeneratedDescriptorSets_AntialiasAfterDenoiser()
{
  // now create actual bindings
  //
  // descriptor set #0: blendCmd (["in_data","out_data","m_AABuffer","m_AABuffer1"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 4 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  4 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   4 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = AntialiasAfterDenoiser_local.in_dataBuffer;
    descriptorBufferInfo[0].offset = AntialiasAfterDenoiser_local.in_dataOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = AntialiasAfterDenoiser_local.out_dataBuffer;
    descriptorBufferInfo[1].offset = AntialiasAfterDenoiser_local.out_dataOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_vdata.m_AABufferBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_AABufferOffset;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    descriptorBufferInfo[3]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[3].buffer = m_vdata.m_AABuffer1Buffer;
    descriptorBufferInfo[3].offset = m_vdata.m_AABuffer1Offset;
    descriptorBufferInfo[3].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[3]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[3].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[3].dstBinding       = 3;
    writeDescriptorSet[3].descriptorCount  = 1;
    writeDescriptorSet[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[3].pBufferInfo      = &descriptorBufferInfo[3];
    writeDescriptorSet[3].pImageInfo       = nullptr;
    writeDescriptorSet[3].pTexelBufferView = nullptr;

    descriptorBufferInfo[4]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[4].buffer = m_classDataBuffer;
    descriptorBufferInfo[4].offset = 0;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
}

void Antialiaser_slang::UpdateAllGeneratedDescriptorSets_Antialias()
{
  // now create actual bindings
  //
  // descriptor set #1: blendCmd (["in_data","out_data","m_AABuffer","m_AABuffer1"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 4 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  4 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   4 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = Antialias_local.in_dataBuffer;
    descriptorBufferInfo[0].offset = Antialias_local.in_dataOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[1];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = Antialias_local.out_dataBuffer;
    descriptorBufferInfo[1].offset = Antialias_local.out_dataOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[1];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_vdata.m_AABufferBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_AABufferOffset;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[1];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    descriptorBufferInfo[3]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[3].buffer = m_vdata.m_AABuffer1Buffer;
    descriptorBufferInfo[3].offset = m_vdata.m_AABuffer1Offset;
    descriptorBufferInfo[3].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[3]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[3].dstSet           = m_allGeneratedDS[1];
    writeDescriptorSet[3].dstBinding       = 3;
    writeDescriptorSet[3].descriptorCount  = 1;
    writeDescriptorSet[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[3].pBufferInfo      = &descriptorBufferInfo[3];
    writeDescriptorSet[3].pImageInfo       = nullptr;
    writeDescriptorSet[3].pTexelBufferView = nullptr;

    descriptorBufferInfo[4]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[4].buffer = m_classDataBuffer;
    descriptorBufferInfo[4].offset = 0;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[1];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
}



