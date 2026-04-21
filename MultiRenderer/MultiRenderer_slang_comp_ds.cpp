#include <vector>
#include <array>
#include <memory>
#include <limits>

#include <cassert>
#include "vk_copy.h"
#include "vk_context.h"

#include "MultiRenderer_slang_comp.h"


void MultiRenderer_slang_comp::AllocateAllDescriptorSets()
{
  // allocate pool
  //
  VkDescriptorPoolSize buffersSize, combinedImageSamSize, imageStorageSize, accelStorageSize, dynamicBuffersSize;
  buffersSize.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  buffersSize.descriptorCount = 327 + 64 ; //

  std::vector<VkDescriptorPoolSize> poolSizes = {buffersSize};
  {
    combinedImageSamSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    combinedImageSamSize.descriptorCount = 8*GetDefaultMaxTextures() + 0;
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
  descriptorPoolCreateInfo.maxSets       = 19 + 2; // add 1 to prevent zero case and one more for internal needs
  descriptorPoolCreateInfo.poolSizeCount = poolSizes.size();
  descriptorPoolCreateInfo.pPoolSizes    = poolSizes.data();

  VK_CHECK_RESULT(vkCreateDescriptorPool(m_device, &descriptorPoolCreateInfo, NULL, &m_dsPool));

  // allocate all descriptor sets
  //
  VkDescriptorSetLayout layouts[19] = {};
  layouts[0] = FillGbufferWithTransparencyDSLayout;
  layouts[1] = ResolveCommonWithTransparencyDSLayout;
  layouts[2] = FillGbufferDSLayout;
  layouts[3] = ResolveCommonDSLayout;
  layouts[4] = ResolveCommonDSLayout;
  layouts[5] = ResolveCTFDSLayout;
  layouts[6] = ResolveOutlineDSLayout;
  layouts[7] = ResolveDebugDSLayout;
  layouts[8] = AverageColorDSLayout;
  layouts[9] = FillGbufferWithTransparencyDSLayout;
  layouts[10] = ResolveCommonWithTransparencyDSLayout;
  layouts[11] = FillGbufferDSLayout;
  layouts[12] = ResolveCommonDSLayout;
  layouts[13] = ResolveCommonDSLayout;
  layouts[14] = ResolveCTFDSLayout;
  layouts[15] = ResolveOutlineDSLayout;
  layouts[16] = ResolveDebugDSLayout;
  layouts[17] = TonemapDSLayout;
  layouts[18] = PackXYDSLayout;

  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
  descriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorSetAllocateInfo.descriptorPool     = m_dsPool;
  descriptorSetAllocateInfo.descriptorSetCount = 19;
  descriptorSetAllocateInfo.pSetLayouts        = layouts;

  auto tmpRes = vkAllocateDescriptorSets(m_device, &descriptorSetAllocateInfo, m_allGeneratedDS);
  VK_CHECK_RESULT(tmpRes);
}

VkDescriptorSetLayout MultiRenderer_slang_comp::CreateResolveDebugDSLayout()
{
  std::array<VkDescriptorSetLayoutBinding, 9+1> dsBindings;
  
  const auto stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // binding for out_color
  dsBindings[0].binding            = 0;
  dsBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[0].descriptorCount    = 1;
  dsBindings[0].stageFlags         = stageFlags;
  dsBindings[0].pImmutableSamplers = nullptr;

  // binding for m_gBuffer
  dsBindings[1].binding            = 1;
  dsBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[1].descriptorCount    = 1;
  dsBindings[1].stageFlags         = stageFlags;
  dsBindings[1].pImmutableSamplers = nullptr;

  // binding for m_geomOffsets
  dsBindings[2].binding            = 2;
  dsBindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[2].descriptorCount    = 1;
  dsBindings[2].stageFlags         = stageFlags;
  dsBindings[2].pImmutableSamplers = nullptr;

  // binding for m_indices
  dsBindings[3].binding            = 3;
  dsBindings[3].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[3].descriptorCount    = 1;
  dsBindings[3].stageFlags         = stageFlags;
  dsBindings[3].pImmutableSamplers = nullptr;

  // binding for m_instanceTransformInvTransposed
  dsBindings[4].binding            = 4;
  dsBindings[4].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[4].descriptorCount    = 1;
  dsBindings[4].stageFlags         = stageFlags;
  dsBindings[4].pImmutableSamplers = nullptr;

  // binding for m_matIdOffsets
  dsBindings[5].binding            = 5;
  dsBindings[5].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[5].descriptorCount    = 1;
  dsBindings[5].stageFlags         = stageFlags;
  dsBindings[5].pImmutableSamplers = nullptr;

  // binding for m_matIdbyPrimId
  dsBindings[6].binding            = 6;
  dsBindings[6].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[6].descriptorCount    = 1;
  dsBindings[6].stageFlags         = stageFlags;
  dsBindings[6].pImmutableSamplers = nullptr;

  // binding for m_normals
  dsBindings[7].binding            = 7;
  dsBindings[7].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[7].descriptorCount    = 1;
  dsBindings[7].stageFlags         = stageFlags;
  dsBindings[7].pImmutableSamplers = nullptr;

  // binding for m_vertices
  dsBindings[8].binding            = 8;
  dsBindings[8].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[8].descriptorCount    = 1;
  dsBindings[8].stageFlags         = stageFlags;
  dsBindings[8].pImmutableSamplers = nullptr;

  dsBindings[9].binding            = 9;
  dsBindings[9].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
  dsBindings[9].descriptorCount    = 1;
  dsBindings[9].stageFlags         = stageFlags;
  dsBindings[9].pImmutableSamplers = nullptr;


  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
  descriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount = uint32_t(dsBindings.size());
  descriptorSetLayoutCreateInfo.pBindings    = dsBindings.data();

  VkDescriptorSetLayout layout = nullptr;
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, NULL, &layout));
  return layout;
}
VkDescriptorSetLayout MultiRenderer_slang_comp::CreateFillGbufferDSLayout()
{
  std::array<VkDescriptorSetLayoutBinding, 44+1> dsBindings;
  
  const auto stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // binding for m_gBuffer
  dsBindings[0].binding            = 0;
  dsBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[0].descriptorCount    = 1;
  dsBindings[0].stageFlags         = stageFlags;
  dsBindings[0].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_GraphicsPrimHeaders
  dsBindings[1].binding            = 1;
  dsBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[1].descriptorCount    = 1;
  dsBindings[1].stageFlags         = stageFlags;
  dsBindings[1].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_GraphicsPrimPoints
  dsBindings[2].binding            = 2;
  dsBindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[2].descriptorCount    = 1;
  dsBindings[2].stageFlags         = stageFlags;
  dsBindings[2].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_RotAddTable
  dsBindings[3].binding            = 3;
  dsBindings[3].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[3].descriptorCount    = 1;
  dsBindings[3].stageFlags         = stageFlags;
  dsBindings[3].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SCom2Data
  dsBindings[4].binding            = 4;
  dsBindings[4].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[4].descriptorCount    = 1;
  dsBindings[4].stageFlags         = stageFlags;
  dsBindings[4].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SCom2Headers
  dsBindings[5].binding            = 5;
  dsBindings[5].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[5].descriptorCount    = 1;
  dsBindings[5].stageFlags         = stageFlags;
  dsBindings[5].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfCompactOctreeRotModifiers
  dsBindings[6].binding            = 6;
  dsBindings[6].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[6].descriptorCount    = 1;
  dsBindings[6].stageFlags         = stageFlags;
  dsBindings[6].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfCompactOctreeRotTransforms
  dsBindings[7].binding            = 7;
  dsBindings[7].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[7].descriptorCount    = 1;
  dsBindings[7].stageFlags         = stageFlags;
  dsBindings[7].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfCompactOctreeV3Data
  dsBindings[8].binding            = 8;
  dsBindings[8].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[8].descriptorCount    = 1;
  dsBindings[8].stageFlags         = stageFlags;
  dsBindings[8].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfDAGChildEdges
  dsBindings[9].binding            = 9;
  dsBindings[9].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[9].descriptorCount    = 1;
  dsBindings[9].stageFlags         = stageFlags;
  dsBindings[9].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfDAGDataEdges
  dsBindings[10].binding            = 10;
  dsBindings[10].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[10].descriptorCount    = 1;
  dsBindings[10].stageFlags         = stageFlags;
  dsBindings[10].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfDAGDistances
  dsBindings[11].binding            = 11;
  dsBindings[11].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[11].descriptorCount    = 1;
  dsBindings[11].stageFlags         = stageFlags;
  dsBindings[11].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfDAGHeaders
  dsBindings[12].binding            = 12;
  dsBindings[12].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[12].descriptorCount    = 1;
  dsBindings[12].stageFlags         = stageFlags;
  dsBindings[12].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfDAGNodes
  dsBindings[13].binding            = 13;
  dsBindings[13].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[13].descriptorCount    = 1;
  dsBindings[13].stageFlags         = stageFlags;
  dsBindings[13].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfDAGTranspositions
  dsBindings[14].binding            = 14;
  dsBindings[14].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[14].descriptorCount    = 1;
  dsBindings[14].stageFlags         = stageFlags;
  dsBindings[14].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfFrameOctreeNodes
  dsBindings[15].binding            = 15;
  dsBindings[15].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[15].descriptorCount    = 1;
  dsBindings[15].stageFlags         = stageFlags;
  dsBindings[15].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfFrameOctreeRoots
  dsBindings[16].binding            = 16;
  dsBindings[16].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[16].descriptorCount    = 1;
  dsBindings[16].stageFlags         = stageFlags;
  dsBindings[16].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfFrameOctreeTexNodes
  dsBindings[17].binding            = 17;
  dsBindings[17].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[17].descriptorCount    = 1;
  dsBindings[17].stageFlags         = stageFlags;
  dsBindings[17].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfFrameOctreeTexRoots
  dsBindings[18].binding            = 18;
  dsBindings[18].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[18].descriptorCount    = 1;
  dsBindings[18].stageFlags         = stageFlags;
  dsBindings[18].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfGridData
  dsBindings[19].binding            = 19;
  dsBindings[19].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[19].descriptorCount    = 1;
  dsBindings[19].stageFlags         = stageFlags;
  dsBindings[19].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfGridOffsets
  dsBindings[20].binding            = 20;
  dsBindings[20].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[20].descriptorCount    = 1;
  dsBindings[20].stageFlags         = stageFlags;
  dsBindings[20].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfGridSizes
  dsBindings[21].binding            = 21;
  dsBindings[21].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[21].descriptorCount    = 1;
  dsBindings[21].stageFlags         = stageFlags;
  dsBindings[21].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfMultiOctreeNodes
  dsBindings[22].binding            = 22;
  dsBindings[22].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[22].descriptorCount    = 1;
  dsBindings[22].stageFlags         = stageFlags;
  dsBindings[22].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfMultiOctreeValues
  dsBindings[23].binding            = 23;
  dsBindings[23].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[23].descriptorCount    = 1;
  dsBindings[23].stageFlags         = stageFlags;
  dsBindings[23].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfSBSData
  dsBindings[24].binding            = 24;
  dsBindings[24].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[24].descriptorCount    = 1;
  dsBindings[24].stageFlags         = stageFlags;
  dsBindings[24].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfSBSDataF
  dsBindings[25].binding            = 25;
  dsBindings[25].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[25].descriptorCount    = 1;
  dsBindings[25].stageFlags         = stageFlags;
  dsBindings[25].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfSBSHeaders
  dsBindings[26].binding            = 26;
  dsBindings[26].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[26].descriptorCount    = 1;
  dsBindings[26].stageFlags         = stageFlags;
  dsBindings[26].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfSBSNodes
  dsBindings[27].binding            = 27;
  dsBindings[27].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[27].descriptorCount    = 1;
  dsBindings[27].stageFlags         = stageFlags;
  dsBindings[27].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfSBSRoots
  dsBindings[28].binding            = 28;
  dsBindings[28].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[28].descriptorCount    = 1;
  dsBindings[28].stageFlags         = stageFlags;
  dsBindings[28].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfSVSNodes
  dsBindings[29].binding            = 29;
  dsBindings[29].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[29].descriptorCount    = 1;
  dsBindings[29].stageFlags         = stageFlags;
  dsBindings[29].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfSVSRoots
  dsBindings[30].binding            = 30;
  dsBindings[30].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[30].descriptorCount    = 1;
  dsBindings[30].stageFlags         = stageFlags;
  dsBindings[30].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_allNodePairs
  dsBindings[31].binding            = 31;
  dsBindings[31].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[31].descriptorCount    = 1;
  dsBindings[31].stageFlags         = stageFlags;
  dsBindings[31].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_geomData
  dsBindings[32].binding            = 32;
  dsBindings[32].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[32].descriptorCount    = 1;
  dsBindings[32].stageFlags         = stageFlags;
  dsBindings[32].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_geomTags
  dsBindings[33].binding            = 33;
  dsBindings[33].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[33].descriptorCount    = 1;
  dsBindings[33].stageFlags         = stageFlags;
  dsBindings[33].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_indices
  dsBindings[34].binding            = 34;
  dsBindings[34].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[34].descriptorCount    = 1;
  dsBindings[34].stageFlags         = stageFlags;
  dsBindings[34].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_instanceData
  dsBindings[35].binding            = 35;
  dsBindings[35].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[35].descriptorCount    = 1;
  dsBindings[35].stageFlags         = stageFlags;
  dsBindings[35].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_nodesTLAS
  dsBindings[36].binding            = 36;
  dsBindings[36].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[36].descriptorCount    = 1;
  dsBindings[36].stageFlags         = stageFlags;
  dsBindings[36].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_origNodes
  dsBindings[37].binding            = 37;
  dsBindings[37].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[37].descriptorCount    = 1;
  dsBindings[37].stageFlags         = stageFlags;
  dsBindings[37].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_primIdCount
  dsBindings[38].binding            = 38;
  dsBindings[38].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[38].descriptorCount    = 1;
  dsBindings[38].stageFlags         = stageFlags;
  dsBindings[38].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_primIndices
  dsBindings[39].binding            = 39;
  dsBindings[39].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[39].descriptorCount    = 1;
  dsBindings[39].stageFlags         = stageFlags;
  dsBindings[39].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_vertNorm
  dsBindings[40].binding            = 40;
  dsBindings[40].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[40].descriptorCount    = 1;
  dsBindings[40].stageFlags         = stageFlags;
  dsBindings[40].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_vertPos
  dsBindings[41].binding            = 41;
  dsBindings[41].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[41].descriptorCount    = 1;
  dsBindings[41].stageFlags         = stageFlags;
  dsBindings[41].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_startEnd
  dsBindings[42].binding            = 42;
  dsBindings[42].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[42].descriptorCount    = 1;
  dsBindings[42].stageFlags         = stageFlags;
  dsBindings[42].pImmutableSamplers = nullptr;

  // binding for m_packedXY
  dsBindings[43].binding            = 43;
  dsBindings[43].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[43].descriptorCount    = 1;
  dsBindings[43].stageFlags         = stageFlags;
  dsBindings[43].pImmutableSamplers = nullptr;

  dsBindings[44].binding            = 44;
  dsBindings[44].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
  dsBindings[44].descriptorCount    = 1;
  dsBindings[44].stageFlags         = stageFlags;
  dsBindings[44].pImmutableSamplers = nullptr;


  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
  descriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount = uint32_t(dsBindings.size());
  descriptorSetLayoutCreateInfo.pBindings    = dsBindings.data();

  VkDescriptorSetLayout layout = nullptr;
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, NULL, &layout));
  return layout;
}
VkDescriptorSetLayout MultiRenderer_slang_comp::CreateFillGbufferWithTransparencyDSLayout()
{
  std::array<VkDescriptorSetLayoutBinding, 48+1> dsBindings;
  
  const auto stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // binding for m_gBuffer
  dsBindings[0].binding            = 0;
  dsBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[0].descriptorCount    = 1;
  dsBindings[0].stageFlags         = stageFlags;
  dsBindings[0].pImmutableSamplers = nullptr;

  // binding for m_matIdOffsets
  dsBindings[1].binding            = 1;
  dsBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[1].descriptorCount    = 1;
  dsBindings[1].stageFlags         = stageFlags;
  dsBindings[1].pImmutableSamplers = nullptr;

  // binding for m_matIdbyPrimId
  dsBindings[2].binding            = 2;
  dsBindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[2].descriptorCount    = 1;
  dsBindings[2].stageFlags         = stageFlags;
  dsBindings[2].pImmutableSamplers = nullptr;

  // binding for m_materials
  dsBindings[3].binding            = 3;
  dsBindings[3].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[3].descriptorCount    = 1;
  dsBindings[3].stageFlags         = stageFlags;
  dsBindings[3].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_GraphicsPrimHeaders
  dsBindings[4].binding            = 4;
  dsBindings[4].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[4].descriptorCount    = 1;
  dsBindings[4].stageFlags         = stageFlags;
  dsBindings[4].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_GraphicsPrimPoints
  dsBindings[5].binding            = 5;
  dsBindings[5].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[5].descriptorCount    = 1;
  dsBindings[5].stageFlags         = stageFlags;
  dsBindings[5].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_RotAddTable
  dsBindings[6].binding            = 6;
  dsBindings[6].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[6].descriptorCount    = 1;
  dsBindings[6].stageFlags         = stageFlags;
  dsBindings[6].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SCom2Data
  dsBindings[7].binding            = 7;
  dsBindings[7].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[7].descriptorCount    = 1;
  dsBindings[7].stageFlags         = stageFlags;
  dsBindings[7].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SCom2Headers
  dsBindings[8].binding            = 8;
  dsBindings[8].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[8].descriptorCount    = 1;
  dsBindings[8].stageFlags         = stageFlags;
  dsBindings[8].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfCompactOctreeRotModifiers
  dsBindings[9].binding            = 9;
  dsBindings[9].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[9].descriptorCount    = 1;
  dsBindings[9].stageFlags         = stageFlags;
  dsBindings[9].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfCompactOctreeRotTransforms
  dsBindings[10].binding            = 10;
  dsBindings[10].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[10].descriptorCount    = 1;
  dsBindings[10].stageFlags         = stageFlags;
  dsBindings[10].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfCompactOctreeV3Data
  dsBindings[11].binding            = 11;
  dsBindings[11].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[11].descriptorCount    = 1;
  dsBindings[11].stageFlags         = stageFlags;
  dsBindings[11].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfDAGChildEdges
  dsBindings[12].binding            = 12;
  dsBindings[12].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[12].descriptorCount    = 1;
  dsBindings[12].stageFlags         = stageFlags;
  dsBindings[12].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfDAGDataEdges
  dsBindings[13].binding            = 13;
  dsBindings[13].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[13].descriptorCount    = 1;
  dsBindings[13].stageFlags         = stageFlags;
  dsBindings[13].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfDAGDistances
  dsBindings[14].binding            = 14;
  dsBindings[14].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[14].descriptorCount    = 1;
  dsBindings[14].stageFlags         = stageFlags;
  dsBindings[14].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfDAGHeaders
  dsBindings[15].binding            = 15;
  dsBindings[15].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[15].descriptorCount    = 1;
  dsBindings[15].stageFlags         = stageFlags;
  dsBindings[15].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfDAGNodes
  dsBindings[16].binding            = 16;
  dsBindings[16].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[16].descriptorCount    = 1;
  dsBindings[16].stageFlags         = stageFlags;
  dsBindings[16].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfDAGTranspositions
  dsBindings[17].binding            = 17;
  dsBindings[17].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[17].descriptorCount    = 1;
  dsBindings[17].stageFlags         = stageFlags;
  dsBindings[17].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfFrameOctreeNodes
  dsBindings[18].binding            = 18;
  dsBindings[18].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[18].descriptorCount    = 1;
  dsBindings[18].stageFlags         = stageFlags;
  dsBindings[18].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfFrameOctreeRoots
  dsBindings[19].binding            = 19;
  dsBindings[19].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[19].descriptorCount    = 1;
  dsBindings[19].stageFlags         = stageFlags;
  dsBindings[19].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfFrameOctreeTexNodes
  dsBindings[20].binding            = 20;
  dsBindings[20].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[20].descriptorCount    = 1;
  dsBindings[20].stageFlags         = stageFlags;
  dsBindings[20].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfFrameOctreeTexRoots
  dsBindings[21].binding            = 21;
  dsBindings[21].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[21].descriptorCount    = 1;
  dsBindings[21].stageFlags         = stageFlags;
  dsBindings[21].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfGridData
  dsBindings[22].binding            = 22;
  dsBindings[22].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[22].descriptorCount    = 1;
  dsBindings[22].stageFlags         = stageFlags;
  dsBindings[22].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfGridOffsets
  dsBindings[23].binding            = 23;
  dsBindings[23].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[23].descriptorCount    = 1;
  dsBindings[23].stageFlags         = stageFlags;
  dsBindings[23].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfGridSizes
  dsBindings[24].binding            = 24;
  dsBindings[24].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[24].descriptorCount    = 1;
  dsBindings[24].stageFlags         = stageFlags;
  dsBindings[24].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfMultiOctreeNodes
  dsBindings[25].binding            = 25;
  dsBindings[25].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[25].descriptorCount    = 1;
  dsBindings[25].stageFlags         = stageFlags;
  dsBindings[25].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfMultiOctreeValues
  dsBindings[26].binding            = 26;
  dsBindings[26].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[26].descriptorCount    = 1;
  dsBindings[26].stageFlags         = stageFlags;
  dsBindings[26].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfSBSData
  dsBindings[27].binding            = 27;
  dsBindings[27].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[27].descriptorCount    = 1;
  dsBindings[27].stageFlags         = stageFlags;
  dsBindings[27].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfSBSDataF
  dsBindings[28].binding            = 28;
  dsBindings[28].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[28].descriptorCount    = 1;
  dsBindings[28].stageFlags         = stageFlags;
  dsBindings[28].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfSBSHeaders
  dsBindings[29].binding            = 29;
  dsBindings[29].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[29].descriptorCount    = 1;
  dsBindings[29].stageFlags         = stageFlags;
  dsBindings[29].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfSBSNodes
  dsBindings[30].binding            = 30;
  dsBindings[30].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[30].descriptorCount    = 1;
  dsBindings[30].stageFlags         = stageFlags;
  dsBindings[30].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfSBSRoots
  dsBindings[31].binding            = 31;
  dsBindings[31].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[31].descriptorCount    = 1;
  dsBindings[31].stageFlags         = stageFlags;
  dsBindings[31].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfSVSNodes
  dsBindings[32].binding            = 32;
  dsBindings[32].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[32].descriptorCount    = 1;
  dsBindings[32].stageFlags         = stageFlags;
  dsBindings[32].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_SdfSVSRoots
  dsBindings[33].binding            = 33;
  dsBindings[33].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[33].descriptorCount    = 1;
  dsBindings[33].stageFlags         = stageFlags;
  dsBindings[33].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_allNodePairs
  dsBindings[34].binding            = 34;
  dsBindings[34].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[34].descriptorCount    = 1;
  dsBindings[34].stageFlags         = stageFlags;
  dsBindings[34].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_geomData
  dsBindings[35].binding            = 35;
  dsBindings[35].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[35].descriptorCount    = 1;
  dsBindings[35].stageFlags         = stageFlags;
  dsBindings[35].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_geomTags
  dsBindings[36].binding            = 36;
  dsBindings[36].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[36].descriptorCount    = 1;
  dsBindings[36].stageFlags         = stageFlags;
  dsBindings[36].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_indices
  dsBindings[37].binding            = 37;
  dsBindings[37].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[37].descriptorCount    = 1;
  dsBindings[37].stageFlags         = stageFlags;
  dsBindings[37].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_instanceData
  dsBindings[38].binding            = 38;
  dsBindings[38].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[38].descriptorCount    = 1;
  dsBindings[38].stageFlags         = stageFlags;
  dsBindings[38].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_nodesTLAS
  dsBindings[39].binding            = 39;
  dsBindings[39].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[39].descriptorCount    = 1;
  dsBindings[39].stageFlags         = stageFlags;
  dsBindings[39].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_origNodes
  dsBindings[40].binding            = 40;
  dsBindings[40].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[40].descriptorCount    = 1;
  dsBindings[40].stageFlags         = stageFlags;
  dsBindings[40].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_primIdCount
  dsBindings[41].binding            = 41;
  dsBindings[41].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[41].descriptorCount    = 1;
  dsBindings[41].stageFlags         = stageFlags;
  dsBindings[41].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_primIndices
  dsBindings[42].binding            = 42;
  dsBindings[42].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[42].descriptorCount    = 1;
  dsBindings[42].stageFlags         = stageFlags;
  dsBindings[42].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_vertNorm
  dsBindings[43].binding            = 43;
  dsBindings[43].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[43].descriptorCount    = 1;
  dsBindings[43].stageFlags         = stageFlags;
  dsBindings[43].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_m_vertPos
  dsBindings[44].binding            = 44;
  dsBindings[44].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[44].descriptorCount    = 1;
  dsBindings[44].stageFlags         = stageFlags;
  dsBindings[44].pImmutableSamplers = nullptr;

  // binding for m_pAccelStruct_startEnd
  dsBindings[45].binding            = 45;
  dsBindings[45].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[45].descriptorCount    = 1;
  dsBindings[45].stageFlags         = stageFlags;
  dsBindings[45].pImmutableSamplers = nullptr;

  // binding for m_packedXY
  dsBindings[46].binding            = 46;
  dsBindings[46].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[46].descriptorCount    = 1;
  dsBindings[46].stageFlags         = stageFlags;
  dsBindings[46].pImmutableSamplers = nullptr;

  // binding for m_transparencyBuffer
  dsBindings[47].binding            = 47;
  dsBindings[47].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[47].descriptorCount    = 1;
  dsBindings[47].stageFlags         = stageFlags;
  dsBindings[47].pImmutableSamplers = nullptr;

  dsBindings[48].binding            = 48;
  dsBindings[48].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
  dsBindings[48].descriptorCount    = 1;
  dsBindings[48].stageFlags         = stageFlags;
  dsBindings[48].pImmutableSamplers = nullptr;


  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
  descriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount = uint32_t(dsBindings.size());
  descriptorSetLayoutCreateInfo.pBindings    = dsBindings.data();

  VkDescriptorSetLayout layout = nullptr;
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, NULL, &layout));
  return layout;
}
VkDescriptorSetLayout MultiRenderer_slang_comp::CreateResolveOutlineDSLayout()
{
  std::array<VkDescriptorSetLayoutBinding, 8+1> dsBindings;
  
  const auto stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // binding for out_color
  dsBindings[0].binding            = 0;
  dsBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[0].descriptorCount    = 1;
  dsBindings[0].stageFlags         = stageFlags;
  dsBindings[0].pImmutableSamplers = nullptr;

  // binding for m_gBuffer
  dsBindings[1].binding            = 1;
  dsBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[1].descriptorCount    = 1;
  dsBindings[1].stageFlags         = stageFlags;
  dsBindings[1].pImmutableSamplers = nullptr;

  // binding for m_geomOffsets
  dsBindings[2].binding            = 2;
  dsBindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[2].descriptorCount    = 1;
  dsBindings[2].stageFlags         = stageFlags;
  dsBindings[2].pImmutableSamplers = nullptr;

  // binding for m_indices
  dsBindings[3].binding            = 3;
  dsBindings[3].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[3].descriptorCount    = 1;
  dsBindings[3].stageFlags         = stageFlags;
  dsBindings[3].pImmutableSamplers = nullptr;

  // binding for m_instanceTransformInvTransposed
  dsBindings[4].binding            = 4;
  dsBindings[4].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[4].descriptorCount    = 1;
  dsBindings[4].stageFlags         = stageFlags;
  dsBindings[4].pImmutableSamplers = nullptr;

  // binding for m_lights
  dsBindings[5].binding            = 5;
  dsBindings[5].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[5].descriptorCount    = 1;
  dsBindings[5].stageFlags         = stageFlags;
  dsBindings[5].pImmutableSamplers = nullptr;

  // binding for m_normals
  dsBindings[6].binding            = 6;
  dsBindings[6].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[6].descriptorCount    = 1;
  dsBindings[6].stageFlags         = stageFlags;
  dsBindings[6].pImmutableSamplers = nullptr;

  // binding for m_vertices
  dsBindings[7].binding            = 7;
  dsBindings[7].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[7].descriptorCount    = 1;
  dsBindings[7].stageFlags         = stageFlags;
  dsBindings[7].pImmutableSamplers = nullptr;

  dsBindings[8].binding            = 8;
  dsBindings[8].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
  dsBindings[8].descriptorCount    = 1;
  dsBindings[8].stageFlags         = stageFlags;
  dsBindings[8].pImmutableSamplers = nullptr;


  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
  descriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount = uint32_t(dsBindings.size());
  descriptorSetLayoutCreateInfo.pBindings    = dsBindings.data();

  VkDescriptorSetLayout layout = nullptr;
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, NULL, &layout));
  return layout;
}
VkDescriptorSetLayout MultiRenderer_slang_comp::CreateAverageColorDSLayout()
{
  std::array<VkDescriptorSetLayoutBinding, 2+1> dsBindings;
  
  const auto stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // binding for out_color
  dsBindings[0].binding            = 0;
  dsBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[0].descriptorCount    = 1;
  dsBindings[0].stageFlags         = stageFlags;
  dsBindings[0].pImmutableSamplers = nullptr;

  // binding for m_colorBuffer
  dsBindings[1].binding            = 1;
  dsBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[1].descriptorCount    = 1;
  dsBindings[1].stageFlags         = stageFlags;
  dsBindings[1].pImmutableSamplers = nullptr;

  dsBindings[2].binding            = 2;
  dsBindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
  dsBindings[2].descriptorCount    = 1;
  dsBindings[2].stageFlags         = stageFlags;
  dsBindings[2].pImmutableSamplers = nullptr;


  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
  descriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount = uint32_t(dsBindings.size());
  descriptorSetLayoutCreateInfo.pBindings    = dsBindings.data();

  VkDescriptorSetLayout layout = nullptr;
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, NULL, &layout));
  return layout;
}
VkDescriptorSetLayout MultiRenderer_slang_comp::CreateTonemapDSLayout()
{
  std::array<VkDescriptorSetLayoutBinding, 2+1> dsBindings;
  
  const auto stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // binding for out_color
  dsBindings[0].binding            = 0;
  dsBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[0].descriptorCount    = 1;
  dsBindings[0].stageFlags         = stageFlags;
  dsBindings[0].pImmutableSamplers = nullptr;

  // binding for m_colorBuffer
  dsBindings[1].binding            = 1;
  dsBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[1].descriptorCount    = 1;
  dsBindings[1].stageFlags         = stageFlags;
  dsBindings[1].pImmutableSamplers = nullptr;

  dsBindings[2].binding            = 2;
  dsBindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
  dsBindings[2].descriptorCount    = 1;
  dsBindings[2].stageFlags         = stageFlags;
  dsBindings[2].pImmutableSamplers = nullptr;


  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
  descriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount = uint32_t(dsBindings.size());
  descriptorSetLayoutCreateInfo.pBindings    = dsBindings.data();

  VkDescriptorSetLayout layout = nullptr;
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, NULL, &layout));
  return layout;
}
VkDescriptorSetLayout MultiRenderer_slang_comp::CreatePackXYDSLayout()
{
  std::array<VkDescriptorSetLayoutBinding, 1+1> dsBindings;
  
  const auto stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // binding for m_packedXY
  dsBindings[0].binding            = 0;
  dsBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[0].descriptorCount    = 1;
  dsBindings[0].stageFlags         = stageFlags;
  dsBindings[0].pImmutableSamplers = nullptr;

  dsBindings[1].binding            = 1;
  dsBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
  dsBindings[1].descriptorCount    = 1;
  dsBindings[1].stageFlags         = stageFlags;
  dsBindings[1].pImmutableSamplers = nullptr;


  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
  descriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount = uint32_t(dsBindings.size());
  descriptorSetLayoutCreateInfo.pBindings    = dsBindings.data();

  VkDescriptorSetLayout layout = nullptr;
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, NULL, &layout));
  return layout;
}
VkDescriptorSetLayout MultiRenderer_slang_comp::CreateResolveCommonDSLayout()
{
  std::array<VkDescriptorSetLayoutBinding, 12+1> dsBindings;
  
  const auto stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // binding for out_color
  dsBindings[0].binding            = 0;
  dsBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[0].descriptorCount    = 1;
  dsBindings[0].stageFlags         = stageFlags;
  dsBindings[0].pImmutableSamplers = nullptr;

  // binding for m_gBuffer
  dsBindings[1].binding            = 1;
  dsBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[1].descriptorCount    = 1;
  dsBindings[1].stageFlags         = stageFlags;
  dsBindings[1].pImmutableSamplers = nullptr;

  // binding for m_geomOffsets
  dsBindings[2].binding            = 2;
  dsBindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[2].descriptorCount    = 1;
  dsBindings[2].stageFlags         = stageFlags;
  dsBindings[2].pImmutableSamplers = nullptr;

  // binding for m_indices
  dsBindings[3].binding            = 3;
  dsBindings[3].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[3].descriptorCount    = 1;
  dsBindings[3].stageFlags         = stageFlags;
  dsBindings[3].pImmutableSamplers = nullptr;

  // binding for m_instanceTransformInvTransposed
  dsBindings[4].binding            = 4;
  dsBindings[4].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[4].descriptorCount    = 1;
  dsBindings[4].stageFlags         = stageFlags;
  dsBindings[4].pImmutableSamplers = nullptr;

  // binding for m_lights
  dsBindings[5].binding            = 5;
  dsBindings[5].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[5].descriptorCount    = 1;
  dsBindings[5].stageFlags         = stageFlags;
  dsBindings[5].pImmutableSamplers = nullptr;

  // binding for m_matIdOffsets
  dsBindings[6].binding            = 6;
  dsBindings[6].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[6].descriptorCount    = 1;
  dsBindings[6].stageFlags         = stageFlags;
  dsBindings[6].pImmutableSamplers = nullptr;

  // binding for m_matIdbyPrimId
  dsBindings[7].binding            = 7;
  dsBindings[7].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[7].descriptorCount    = 1;
  dsBindings[7].stageFlags         = stageFlags;
  dsBindings[7].pImmutableSamplers = nullptr;

  // binding for m_materials
  dsBindings[8].binding            = 8;
  dsBindings[8].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[8].descriptorCount    = 1;
  dsBindings[8].stageFlags         = stageFlags;
  dsBindings[8].pImmutableSamplers = nullptr;

  // binding for m_normals
  dsBindings[9].binding            = 9;
  dsBindings[9].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[9].descriptorCount    = 1;
  dsBindings[9].stageFlags         = stageFlags;
  dsBindings[9].pImmutableSamplers = nullptr;

  // binding for m_textures
  dsBindings[10].binding            = 10;
  dsBindings[10].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  m_vdata.m_texturesArrayMaxSize = m_textures.size();
  if(m_vdata.m_texturesArrayMaxSize == 0)
    m_vdata.m_texturesArrayMaxSize = GetDefaultMaxTextures();
  dsBindings[10].descriptorCount    = m_vdata.m_texturesArrayMaxSize;
  dsBindings[10].stageFlags         = stageFlags;
  dsBindings[10].pImmutableSamplers = nullptr;

  // binding for m_vertices
  dsBindings[11].binding            = 11;
  dsBindings[11].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[11].descriptorCount    = 1;
  dsBindings[11].stageFlags         = stageFlags;
  dsBindings[11].pImmutableSamplers = nullptr;

  dsBindings[12].binding            = 12;
  dsBindings[12].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
  dsBindings[12].descriptorCount    = 1;
  dsBindings[12].stageFlags         = stageFlags;
  dsBindings[12].pImmutableSamplers = nullptr;


  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
  descriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount = uint32_t(dsBindings.size());
  descriptorSetLayoutCreateInfo.pBindings    = dsBindings.data();

  VkDescriptorSetLayout layout = nullptr;
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, NULL, &layout));
  return layout;
}
VkDescriptorSetLayout MultiRenderer_slang_comp::CreateResolveCommonWithTransparencyDSLayout()
{
  std::array<VkDescriptorSetLayoutBinding, 13+1> dsBindings;
  
  const auto stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // binding for out_color
  dsBindings[0].binding            = 0;
  dsBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[0].descriptorCount    = 1;
  dsBindings[0].stageFlags         = stageFlags;
  dsBindings[0].pImmutableSamplers = nullptr;

  // binding for m_gBuffer
  dsBindings[1].binding            = 1;
  dsBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[1].descriptorCount    = 1;
  dsBindings[1].stageFlags         = stageFlags;
  dsBindings[1].pImmutableSamplers = nullptr;

  // binding for m_geomOffsets
  dsBindings[2].binding            = 2;
  dsBindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[2].descriptorCount    = 1;
  dsBindings[2].stageFlags         = stageFlags;
  dsBindings[2].pImmutableSamplers = nullptr;

  // binding for m_indices
  dsBindings[3].binding            = 3;
  dsBindings[3].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[3].descriptorCount    = 1;
  dsBindings[3].stageFlags         = stageFlags;
  dsBindings[3].pImmutableSamplers = nullptr;

  // binding for m_instanceTransformInvTransposed
  dsBindings[4].binding            = 4;
  dsBindings[4].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[4].descriptorCount    = 1;
  dsBindings[4].stageFlags         = stageFlags;
  dsBindings[4].pImmutableSamplers = nullptr;

  // binding for m_lights
  dsBindings[5].binding            = 5;
  dsBindings[5].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[5].descriptorCount    = 1;
  dsBindings[5].stageFlags         = stageFlags;
  dsBindings[5].pImmutableSamplers = nullptr;

  // binding for m_matIdOffsets
  dsBindings[6].binding            = 6;
  dsBindings[6].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[6].descriptorCount    = 1;
  dsBindings[6].stageFlags         = stageFlags;
  dsBindings[6].pImmutableSamplers = nullptr;

  // binding for m_matIdbyPrimId
  dsBindings[7].binding            = 7;
  dsBindings[7].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[7].descriptorCount    = 1;
  dsBindings[7].stageFlags         = stageFlags;
  dsBindings[7].pImmutableSamplers = nullptr;

  // binding for m_materials
  dsBindings[8].binding            = 8;
  dsBindings[8].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[8].descriptorCount    = 1;
  dsBindings[8].stageFlags         = stageFlags;
  dsBindings[8].pImmutableSamplers = nullptr;

  // binding for m_normals
  dsBindings[9].binding            = 9;
  dsBindings[9].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[9].descriptorCount    = 1;
  dsBindings[9].stageFlags         = stageFlags;
  dsBindings[9].pImmutableSamplers = nullptr;

  // binding for m_textures
  dsBindings[10].binding            = 10;
  dsBindings[10].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  m_vdata.m_texturesArrayMaxSize = m_textures.size();
  if(m_vdata.m_texturesArrayMaxSize == 0)
    m_vdata.m_texturesArrayMaxSize = GetDefaultMaxTextures();
  dsBindings[10].descriptorCount    = m_vdata.m_texturesArrayMaxSize;
  dsBindings[10].stageFlags         = stageFlags;
  dsBindings[10].pImmutableSamplers = nullptr;

  // binding for m_transparencyBuffer
  dsBindings[11].binding            = 11;
  dsBindings[11].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[11].descriptorCount    = 1;
  dsBindings[11].stageFlags         = stageFlags;
  dsBindings[11].pImmutableSamplers = nullptr;

  // binding for m_vertices
  dsBindings[12].binding            = 12;
  dsBindings[12].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[12].descriptorCount    = 1;
  dsBindings[12].stageFlags         = stageFlags;
  dsBindings[12].pImmutableSamplers = nullptr;

  dsBindings[13].binding            = 13;
  dsBindings[13].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
  dsBindings[13].descriptorCount    = 1;
  dsBindings[13].stageFlags         = stageFlags;
  dsBindings[13].pImmutableSamplers = nullptr;


  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
  descriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount = uint32_t(dsBindings.size());
  descriptorSetLayoutCreateInfo.pBindings    = dsBindings.data();

  VkDescriptorSetLayout layout = nullptr;
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, NULL, &layout));
  return layout;
}
VkDescriptorSetLayout MultiRenderer_slang_comp::CreateResolveCTFDSLayout()
{
  std::array<VkDescriptorSetLayoutBinding, 19+1> dsBindings;
  
  const auto stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // binding for out_color
  dsBindings[0].binding            = 0;
  dsBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[0].descriptorCount    = 1;
  dsBindings[0].stageFlags         = stageFlags;
  dsBindings[0].pImmutableSamplers = nullptr;

  // binding for m_allCTFPoints
  dsBindings[1].binding            = 1;
  dsBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[1].descriptorCount    = 1;
  dsBindings[1].stageFlags         = stageFlags;
  dsBindings[1].pImmutableSamplers = nullptr;

  // binding for m_allCTFs
  dsBindings[2].binding            = 2;
  dsBindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[2].descriptorCount    = 1;
  dsBindings[2].stageFlags         = stageFlags;
  dsBindings[2].pImmutableSamplers = nullptr;

  // binding for m_allChannelOffsets
  dsBindings[3].binding            = 3;
  dsBindings[3].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[3].descriptorCount    = 1;
  dsBindings[3].stageFlags         = stageFlags;
  dsBindings[3].pImmutableSamplers = nullptr;

  // binding for m_allChannelRenderInfo
  dsBindings[4].binding            = 4;
  dsBindings[4].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[4].descriptorCount    = 1;
  dsBindings[4].stageFlags         = stageFlags;
  dsBindings[4].pImmutableSamplers = nullptr;

  // binding for m_allCompressedChannels
  dsBindings[5].binding            = 5;
  dsBindings[5].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[5].descriptorCount    = 1;
  dsBindings[5].stageFlags         = stageFlags;
  dsBindings[5].pImmutableSamplers = nullptr;

  // binding for m_allFloatChannels
  dsBindings[6].binding            = 6;
  dsBindings[6].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[6].descriptorCount    = 1;
  dsBindings[6].stageFlags         = stageFlags;
  dsBindings[6].pImmutableSamplers = nullptr;

  // binding for m_allIntChannels
  dsBindings[7].binding            = 7;
  dsBindings[7].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[7].descriptorCount    = 1;
  dsBindings[7].stageFlags         = stageFlags;
  dsBindings[7].pImmutableSamplers = nullptr;

  // binding for m_gBuffer
  dsBindings[8].binding            = 8;
  dsBindings[8].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[8].descriptorCount    = 1;
  dsBindings[8].stageFlags         = stageFlags;
  dsBindings[8].pImmutableSamplers = nullptr;

  // binding for m_geomOffsets
  dsBindings[9].binding            = 9;
  dsBindings[9].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[9].descriptorCount    = 1;
  dsBindings[9].stageFlags         = stageFlags;
  dsBindings[9].pImmutableSamplers = nullptr;

  // binding for m_indices
  dsBindings[10].binding            = 10;
  dsBindings[10].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[10].descriptorCount    = 1;
  dsBindings[10].stageFlags         = stageFlags;
  dsBindings[10].pImmutableSamplers = nullptr;

  // binding for m_instanceTransformInvTransposed
  dsBindings[11].binding            = 11;
  dsBindings[11].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[11].descriptorCount    = 1;
  dsBindings[11].stageFlags         = stageFlags;
  dsBindings[11].pImmutableSamplers = nullptr;

  // binding for m_lights
  dsBindings[12].binding            = 12;
  dsBindings[12].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[12].descriptorCount    = 1;
  dsBindings[12].stageFlags         = stageFlags;
  dsBindings[12].pImmutableSamplers = nullptr;

  // binding for m_matIdOffsets
  dsBindings[13].binding            = 13;
  dsBindings[13].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[13].descriptorCount    = 1;
  dsBindings[13].stageFlags         = stageFlags;
  dsBindings[13].pImmutableSamplers = nullptr;

  // binding for m_matIdbyPrimId
  dsBindings[14].binding            = 14;
  dsBindings[14].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[14].descriptorCount    = 1;
  dsBindings[14].stageFlags         = stageFlags;
  dsBindings[14].pImmutableSamplers = nullptr;

  // binding for m_materials
  dsBindings[15].binding            = 15;
  dsBindings[15].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[15].descriptorCount    = 1;
  dsBindings[15].stageFlags         = stageFlags;
  dsBindings[15].pImmutableSamplers = nullptr;

  // binding for m_normals
  dsBindings[16].binding            = 16;
  dsBindings[16].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[16].descriptorCount    = 1;
  dsBindings[16].stageFlags         = stageFlags;
  dsBindings[16].pImmutableSamplers = nullptr;

  // binding for m_textures
  dsBindings[17].binding            = 17;
  dsBindings[17].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  m_vdata.m_texturesArrayMaxSize = m_textures.size();
  if(m_vdata.m_texturesArrayMaxSize == 0)
    m_vdata.m_texturesArrayMaxSize = GetDefaultMaxTextures();
  dsBindings[17].descriptorCount    = m_vdata.m_texturesArrayMaxSize;
  dsBindings[17].stageFlags         = stageFlags;
  dsBindings[17].pImmutableSamplers = nullptr;

  // binding for m_vertices
  dsBindings[18].binding            = 18;
  dsBindings[18].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  dsBindings[18].descriptorCount    = 1;
  dsBindings[18].stageFlags         = stageFlags;
  dsBindings[18].pImmutableSamplers = nullptr;

  dsBindings[19].binding            = 19;
  dsBindings[19].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
  dsBindings[19].descriptorCount    = 1;
  dsBindings[19].stageFlags         = stageFlags;
  dsBindings[19].pImmutableSamplers = nullptr;


  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
  descriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount = uint32_t(dsBindings.size());
  descriptorSetLayoutCreateInfo.pBindings    = dsBindings.data();

  VkDescriptorSetLayout layout = nullptr;
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, NULL, &layout));
  return layout;
}

VkDescriptorSetLayout MultiRenderer_slang_comp::CreatecopyKernelFloatDSLayout()
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

VkDescriptorSetLayout MultiRenderer_slang_comp::CreatematMulTransposeDSLayout()
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

void MultiRenderer_slang_comp::UpdateAllGeneratedDescriptorSets_RenderFloat()
{
  // now create actual bindings
  //
  // descriptor set #0: FillGbufferWithTransparencyCmd (["m_gBuffer","m_matIdOffsets","m_matIdbyPrimId","m_materials","m_pAccelStruct_m_GraphicsPrimHeaders","m_pAccelStruct_m_GraphicsPrimPoints","m_pAccelStruct_m_RotAddTable","m_pAccelStruct_m_SCom2Data","m_pAccelStruct_m_SCom2Headers","m_pAccelStruct_m_SdfCompactOctreeRotModifiers","m_pAccelStruct_m_SdfCompactOctreeRotTransforms","m_pAccelStruct_m_SdfCompactOctreeV3Data","m_pAccelStruct_m_SdfDAGChildEdges","m_pAccelStruct_m_SdfDAGDataEdges","m_pAccelStruct_m_SdfDAGDistances","m_pAccelStruct_m_SdfDAGHeaders","m_pAccelStruct_m_SdfDAGNodes","m_pAccelStruct_m_SdfDAGTranspositions","m_pAccelStruct_m_SdfFrameOctreeNodes","m_pAccelStruct_m_SdfFrameOctreeRoots","m_pAccelStruct_m_SdfFrameOctreeTexNodes","m_pAccelStruct_m_SdfFrameOctreeTexRoots","m_pAccelStruct_m_SdfGridData","m_pAccelStruct_m_SdfGridOffsets","m_pAccelStruct_m_SdfGridSizes","m_pAccelStruct_m_SdfMultiOctreeNodes","m_pAccelStruct_m_SdfMultiOctreeValues","m_pAccelStruct_m_SdfSBSData","m_pAccelStruct_m_SdfSBSDataF","m_pAccelStruct_m_SdfSBSHeaders","m_pAccelStruct_m_SdfSBSNodes","m_pAccelStruct_m_SdfSBSRoots","m_pAccelStruct_m_SdfSVSNodes","m_pAccelStruct_m_SdfSVSRoots","m_pAccelStruct_m_allNodePairs","m_pAccelStruct_m_geomData","m_pAccelStruct_m_geomTags","m_pAccelStruct_m_indices","m_pAccelStruct_m_instanceData","m_pAccelStruct_m_nodesTLAS","m_pAccelStruct_m_origNodes","m_pAccelStruct_m_primIdCount","m_pAccelStruct_m_primIndices","m_pAccelStruct_m_vertNorm","m_pAccelStruct_m_vertPos","m_pAccelStruct_startEnd","m_packedXY","m_transparencyBuffer"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 48 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  48 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   48 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = m_vdata.m_gBufferBuffer;
    descriptorBufferInfo[0].offset = m_vdata.m_gBufferOffset;
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
    descriptorBufferInfo[1].buffer = m_vdata.m_matIdOffsetsBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_matIdOffsetsOffset;
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
    descriptorBufferInfo[2].buffer = m_vdata.m_matIdbyPrimIdBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_matIdbyPrimIdOffset;
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
    descriptorBufferInfo[3].buffer = m_vdata.m_materialsBuffer;
    descriptorBufferInfo[3].offset = m_vdata.m_materialsOffset;
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
    descriptorBufferInfo[4].buffer = m_vdata.m_pAccelStruct_m_GraphicsPrimHeadersBuffer;
    descriptorBufferInfo[4].offset = m_vdata.m_pAccelStruct_m_GraphicsPrimHeadersOffset;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    descriptorBufferInfo[5]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[5].buffer = m_vdata.m_pAccelStruct_m_GraphicsPrimPointsBuffer;
    descriptorBufferInfo[5].offset = m_vdata.m_pAccelStruct_m_GraphicsPrimPointsOffset;
    descriptorBufferInfo[5].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[5]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[5].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[5].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[5].dstBinding       = 5;
    writeDescriptorSet[5].descriptorCount  = 1;
    writeDescriptorSet[5].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[5].pBufferInfo      = &descriptorBufferInfo[5];
    writeDescriptorSet[5].pImageInfo       = nullptr;
    writeDescriptorSet[5].pTexelBufferView = nullptr;

    descriptorBufferInfo[6]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[6].buffer = m_vdata.m_pAccelStruct_m_RotAddTableBuffer;
    descriptorBufferInfo[6].offset = m_vdata.m_pAccelStruct_m_RotAddTableOffset;
    descriptorBufferInfo[6].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[6]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[6].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[6].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[6].dstBinding       = 6;
    writeDescriptorSet[6].descriptorCount  = 1;
    writeDescriptorSet[6].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[6].pBufferInfo      = &descriptorBufferInfo[6];
    writeDescriptorSet[6].pImageInfo       = nullptr;
    writeDescriptorSet[6].pTexelBufferView = nullptr;

    descriptorBufferInfo[7]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[7].buffer = m_vdata.m_pAccelStruct_m_SCom2DataBuffer;
    descriptorBufferInfo[7].offset = m_vdata.m_pAccelStruct_m_SCom2DataOffset;
    descriptorBufferInfo[7].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[7]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[7].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[7].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[7].dstBinding       = 7;
    writeDescriptorSet[7].descriptorCount  = 1;
    writeDescriptorSet[7].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[7].pBufferInfo      = &descriptorBufferInfo[7];
    writeDescriptorSet[7].pImageInfo       = nullptr;
    writeDescriptorSet[7].pTexelBufferView = nullptr;

    descriptorBufferInfo[8]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[8].buffer = m_vdata.m_pAccelStruct_m_SCom2HeadersBuffer;
    descriptorBufferInfo[8].offset = m_vdata.m_pAccelStruct_m_SCom2HeadersOffset;
    descriptorBufferInfo[8].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[8]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[8].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[8].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[8].dstBinding       = 8;
    writeDescriptorSet[8].descriptorCount  = 1;
    writeDescriptorSet[8].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[8].pBufferInfo      = &descriptorBufferInfo[8];
    writeDescriptorSet[8].pImageInfo       = nullptr;
    writeDescriptorSet[8].pTexelBufferView = nullptr;

    descriptorBufferInfo[9]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[9].buffer = m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotModifiersBuffer;
    descriptorBufferInfo[9].offset = m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotModifiersOffset;
    descriptorBufferInfo[9].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[9]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[9].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[9].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[9].dstBinding       = 9;
    writeDescriptorSet[9].descriptorCount  = 1;
    writeDescriptorSet[9].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[9].pBufferInfo      = &descriptorBufferInfo[9];
    writeDescriptorSet[9].pImageInfo       = nullptr;
    writeDescriptorSet[9].pTexelBufferView = nullptr;

    descriptorBufferInfo[10]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[10].buffer = m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotTransformsBuffer;
    descriptorBufferInfo[10].offset = m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotTransformsOffset;
    descriptorBufferInfo[10].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[10]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[10].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[10].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[10].dstBinding       = 10;
    writeDescriptorSet[10].descriptorCount  = 1;
    writeDescriptorSet[10].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[10].pBufferInfo      = &descriptorBufferInfo[10];
    writeDescriptorSet[10].pImageInfo       = nullptr;
    writeDescriptorSet[10].pTexelBufferView = nullptr;

    descriptorBufferInfo[11]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[11].buffer = m_vdata.m_pAccelStruct_m_SdfCompactOctreeV3DataBuffer;
    descriptorBufferInfo[11].offset = m_vdata.m_pAccelStruct_m_SdfCompactOctreeV3DataOffset;
    descriptorBufferInfo[11].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[11]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[11].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[11].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[11].dstBinding       = 11;
    writeDescriptorSet[11].descriptorCount  = 1;
    writeDescriptorSet[11].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[11].pBufferInfo      = &descriptorBufferInfo[11];
    writeDescriptorSet[11].pImageInfo       = nullptr;
    writeDescriptorSet[11].pTexelBufferView = nullptr;

    descriptorBufferInfo[12]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[12].buffer = m_vdata.m_pAccelStruct_m_SdfDAGChildEdgesBuffer;
    descriptorBufferInfo[12].offset = m_vdata.m_pAccelStruct_m_SdfDAGChildEdgesOffset;
    descriptorBufferInfo[12].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[12]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[12].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[12].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[12].dstBinding       = 12;
    writeDescriptorSet[12].descriptorCount  = 1;
    writeDescriptorSet[12].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[12].pBufferInfo      = &descriptorBufferInfo[12];
    writeDescriptorSet[12].pImageInfo       = nullptr;
    writeDescriptorSet[12].pTexelBufferView = nullptr;

    descriptorBufferInfo[13]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[13].buffer = m_vdata.m_pAccelStruct_m_SdfDAGDataEdgesBuffer;
    descriptorBufferInfo[13].offset = m_vdata.m_pAccelStruct_m_SdfDAGDataEdgesOffset;
    descriptorBufferInfo[13].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[13]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[13].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[13].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[13].dstBinding       = 13;
    writeDescriptorSet[13].descriptorCount  = 1;
    writeDescriptorSet[13].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[13].pBufferInfo      = &descriptorBufferInfo[13];
    writeDescriptorSet[13].pImageInfo       = nullptr;
    writeDescriptorSet[13].pTexelBufferView = nullptr;

    descriptorBufferInfo[14]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[14].buffer = m_vdata.m_pAccelStruct_m_SdfDAGDistancesBuffer;
    descriptorBufferInfo[14].offset = m_vdata.m_pAccelStruct_m_SdfDAGDistancesOffset;
    descriptorBufferInfo[14].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[14]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[14].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[14].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[14].dstBinding       = 14;
    writeDescriptorSet[14].descriptorCount  = 1;
    writeDescriptorSet[14].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[14].pBufferInfo      = &descriptorBufferInfo[14];
    writeDescriptorSet[14].pImageInfo       = nullptr;
    writeDescriptorSet[14].pTexelBufferView = nullptr;

    descriptorBufferInfo[15]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[15].buffer = m_vdata.m_pAccelStruct_m_SdfDAGHeadersBuffer;
    descriptorBufferInfo[15].offset = m_vdata.m_pAccelStruct_m_SdfDAGHeadersOffset;
    descriptorBufferInfo[15].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[15]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[15].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[15].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[15].dstBinding       = 15;
    writeDescriptorSet[15].descriptorCount  = 1;
    writeDescriptorSet[15].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[15].pBufferInfo      = &descriptorBufferInfo[15];
    writeDescriptorSet[15].pImageInfo       = nullptr;
    writeDescriptorSet[15].pTexelBufferView = nullptr;

    descriptorBufferInfo[16]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[16].buffer = m_vdata.m_pAccelStruct_m_SdfDAGNodesBuffer;
    descriptorBufferInfo[16].offset = m_vdata.m_pAccelStruct_m_SdfDAGNodesOffset;
    descriptorBufferInfo[16].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[16]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[16].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[16].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[16].dstBinding       = 16;
    writeDescriptorSet[16].descriptorCount  = 1;
    writeDescriptorSet[16].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[16].pBufferInfo      = &descriptorBufferInfo[16];
    writeDescriptorSet[16].pImageInfo       = nullptr;
    writeDescriptorSet[16].pTexelBufferView = nullptr;

    descriptorBufferInfo[17]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[17].buffer = m_vdata.m_pAccelStruct_m_SdfDAGTranspositionsBuffer;
    descriptorBufferInfo[17].offset = m_vdata.m_pAccelStruct_m_SdfDAGTranspositionsOffset;
    descriptorBufferInfo[17].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[17]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[17].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[17].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[17].dstBinding       = 17;
    writeDescriptorSet[17].descriptorCount  = 1;
    writeDescriptorSet[17].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[17].pBufferInfo      = &descriptorBufferInfo[17];
    writeDescriptorSet[17].pImageInfo       = nullptr;
    writeDescriptorSet[17].pTexelBufferView = nullptr;

    descriptorBufferInfo[18]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[18].buffer = m_vdata.m_pAccelStruct_m_SdfFrameOctreeNodesBuffer;
    descriptorBufferInfo[18].offset = m_vdata.m_pAccelStruct_m_SdfFrameOctreeNodesOffset;
    descriptorBufferInfo[18].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[18]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[18].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[18].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[18].dstBinding       = 18;
    writeDescriptorSet[18].descriptorCount  = 1;
    writeDescriptorSet[18].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[18].pBufferInfo      = &descriptorBufferInfo[18];
    writeDescriptorSet[18].pImageInfo       = nullptr;
    writeDescriptorSet[18].pTexelBufferView = nullptr;

    descriptorBufferInfo[19]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[19].buffer = m_vdata.m_pAccelStruct_m_SdfFrameOctreeRootsBuffer;
    descriptorBufferInfo[19].offset = m_vdata.m_pAccelStruct_m_SdfFrameOctreeRootsOffset;
    descriptorBufferInfo[19].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[19]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[19].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[19].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[19].dstBinding       = 19;
    writeDescriptorSet[19].descriptorCount  = 1;
    writeDescriptorSet[19].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[19].pBufferInfo      = &descriptorBufferInfo[19];
    writeDescriptorSet[19].pImageInfo       = nullptr;
    writeDescriptorSet[19].pTexelBufferView = nullptr;

    descriptorBufferInfo[20]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[20].buffer = m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexNodesBuffer;
    descriptorBufferInfo[20].offset = m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexNodesOffset;
    descriptorBufferInfo[20].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[20]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[20].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[20].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[20].dstBinding       = 20;
    writeDescriptorSet[20].descriptorCount  = 1;
    writeDescriptorSet[20].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[20].pBufferInfo      = &descriptorBufferInfo[20];
    writeDescriptorSet[20].pImageInfo       = nullptr;
    writeDescriptorSet[20].pTexelBufferView = nullptr;

    descriptorBufferInfo[21]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[21].buffer = m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexRootsBuffer;
    descriptorBufferInfo[21].offset = m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexRootsOffset;
    descriptorBufferInfo[21].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[21]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[21].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[21].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[21].dstBinding       = 21;
    writeDescriptorSet[21].descriptorCount  = 1;
    writeDescriptorSet[21].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[21].pBufferInfo      = &descriptorBufferInfo[21];
    writeDescriptorSet[21].pImageInfo       = nullptr;
    writeDescriptorSet[21].pTexelBufferView = nullptr;

    descriptorBufferInfo[22]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[22].buffer = m_vdata.m_pAccelStruct_m_SdfGridDataBuffer;
    descriptorBufferInfo[22].offset = m_vdata.m_pAccelStruct_m_SdfGridDataOffset;
    descriptorBufferInfo[22].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[22]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[22].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[22].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[22].dstBinding       = 22;
    writeDescriptorSet[22].descriptorCount  = 1;
    writeDescriptorSet[22].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[22].pBufferInfo      = &descriptorBufferInfo[22];
    writeDescriptorSet[22].pImageInfo       = nullptr;
    writeDescriptorSet[22].pTexelBufferView = nullptr;

    descriptorBufferInfo[23]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[23].buffer = m_vdata.m_pAccelStruct_m_SdfGridOffsetsBuffer;
    descriptorBufferInfo[23].offset = m_vdata.m_pAccelStruct_m_SdfGridOffsetsOffset;
    descriptorBufferInfo[23].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[23]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[23].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[23].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[23].dstBinding       = 23;
    writeDescriptorSet[23].descriptorCount  = 1;
    writeDescriptorSet[23].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[23].pBufferInfo      = &descriptorBufferInfo[23];
    writeDescriptorSet[23].pImageInfo       = nullptr;
    writeDescriptorSet[23].pTexelBufferView = nullptr;

    descriptorBufferInfo[24]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[24].buffer = m_vdata.m_pAccelStruct_m_SdfGridSizesBuffer;
    descriptorBufferInfo[24].offset = m_vdata.m_pAccelStruct_m_SdfGridSizesOffset;
    descriptorBufferInfo[24].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[24]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[24].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[24].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[24].dstBinding       = 24;
    writeDescriptorSet[24].descriptorCount  = 1;
    writeDescriptorSet[24].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[24].pBufferInfo      = &descriptorBufferInfo[24];
    writeDescriptorSet[24].pImageInfo       = nullptr;
    writeDescriptorSet[24].pTexelBufferView = nullptr;

    descriptorBufferInfo[25]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[25].buffer = m_vdata.m_pAccelStruct_m_SdfMultiOctreeNodesBuffer;
    descriptorBufferInfo[25].offset = m_vdata.m_pAccelStruct_m_SdfMultiOctreeNodesOffset;
    descriptorBufferInfo[25].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[25]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[25].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[25].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[25].dstBinding       = 25;
    writeDescriptorSet[25].descriptorCount  = 1;
    writeDescriptorSet[25].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[25].pBufferInfo      = &descriptorBufferInfo[25];
    writeDescriptorSet[25].pImageInfo       = nullptr;
    writeDescriptorSet[25].pTexelBufferView = nullptr;

    descriptorBufferInfo[26]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[26].buffer = m_vdata.m_pAccelStruct_m_SdfMultiOctreeValuesBuffer;
    descriptorBufferInfo[26].offset = m_vdata.m_pAccelStruct_m_SdfMultiOctreeValuesOffset;
    descriptorBufferInfo[26].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[26]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[26].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[26].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[26].dstBinding       = 26;
    writeDescriptorSet[26].descriptorCount  = 1;
    writeDescriptorSet[26].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[26].pBufferInfo      = &descriptorBufferInfo[26];
    writeDescriptorSet[26].pImageInfo       = nullptr;
    writeDescriptorSet[26].pTexelBufferView = nullptr;

    descriptorBufferInfo[27]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[27].buffer = m_vdata.m_pAccelStruct_m_SdfSBSDataBuffer;
    descriptorBufferInfo[27].offset = m_vdata.m_pAccelStruct_m_SdfSBSDataOffset;
    descriptorBufferInfo[27].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[27]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[27].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[27].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[27].dstBinding       = 27;
    writeDescriptorSet[27].descriptorCount  = 1;
    writeDescriptorSet[27].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[27].pBufferInfo      = &descriptorBufferInfo[27];
    writeDescriptorSet[27].pImageInfo       = nullptr;
    writeDescriptorSet[27].pTexelBufferView = nullptr;

    descriptorBufferInfo[28]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[28].buffer = m_vdata.m_pAccelStruct_m_SdfSBSDataFBuffer;
    descriptorBufferInfo[28].offset = m_vdata.m_pAccelStruct_m_SdfSBSDataFOffset;
    descriptorBufferInfo[28].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[28]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[28].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[28].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[28].dstBinding       = 28;
    writeDescriptorSet[28].descriptorCount  = 1;
    writeDescriptorSet[28].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[28].pBufferInfo      = &descriptorBufferInfo[28];
    writeDescriptorSet[28].pImageInfo       = nullptr;
    writeDescriptorSet[28].pTexelBufferView = nullptr;

    descriptorBufferInfo[29]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[29].buffer = m_vdata.m_pAccelStruct_m_SdfSBSHeadersBuffer;
    descriptorBufferInfo[29].offset = m_vdata.m_pAccelStruct_m_SdfSBSHeadersOffset;
    descriptorBufferInfo[29].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[29]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[29].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[29].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[29].dstBinding       = 29;
    writeDescriptorSet[29].descriptorCount  = 1;
    writeDescriptorSet[29].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[29].pBufferInfo      = &descriptorBufferInfo[29];
    writeDescriptorSet[29].pImageInfo       = nullptr;
    writeDescriptorSet[29].pTexelBufferView = nullptr;

    descriptorBufferInfo[30]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[30].buffer = m_vdata.m_pAccelStruct_m_SdfSBSNodesBuffer;
    descriptorBufferInfo[30].offset = m_vdata.m_pAccelStruct_m_SdfSBSNodesOffset;
    descriptorBufferInfo[30].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[30]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[30].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[30].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[30].dstBinding       = 30;
    writeDescriptorSet[30].descriptorCount  = 1;
    writeDescriptorSet[30].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[30].pBufferInfo      = &descriptorBufferInfo[30];
    writeDescriptorSet[30].pImageInfo       = nullptr;
    writeDescriptorSet[30].pTexelBufferView = nullptr;

    descriptorBufferInfo[31]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[31].buffer = m_vdata.m_pAccelStruct_m_SdfSBSRootsBuffer;
    descriptorBufferInfo[31].offset = m_vdata.m_pAccelStruct_m_SdfSBSRootsOffset;
    descriptorBufferInfo[31].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[31]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[31].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[31].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[31].dstBinding       = 31;
    writeDescriptorSet[31].descriptorCount  = 1;
    writeDescriptorSet[31].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[31].pBufferInfo      = &descriptorBufferInfo[31];
    writeDescriptorSet[31].pImageInfo       = nullptr;
    writeDescriptorSet[31].pTexelBufferView = nullptr;

    descriptorBufferInfo[32]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[32].buffer = m_vdata.m_pAccelStruct_m_SdfSVSNodesBuffer;
    descriptorBufferInfo[32].offset = m_vdata.m_pAccelStruct_m_SdfSVSNodesOffset;
    descriptorBufferInfo[32].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[32]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[32].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[32].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[32].dstBinding       = 32;
    writeDescriptorSet[32].descriptorCount  = 1;
    writeDescriptorSet[32].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[32].pBufferInfo      = &descriptorBufferInfo[32];
    writeDescriptorSet[32].pImageInfo       = nullptr;
    writeDescriptorSet[32].pTexelBufferView = nullptr;

    descriptorBufferInfo[33]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[33].buffer = m_vdata.m_pAccelStruct_m_SdfSVSRootsBuffer;
    descriptorBufferInfo[33].offset = m_vdata.m_pAccelStruct_m_SdfSVSRootsOffset;
    descriptorBufferInfo[33].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[33]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[33].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[33].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[33].dstBinding       = 33;
    writeDescriptorSet[33].descriptorCount  = 1;
    writeDescriptorSet[33].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[33].pBufferInfo      = &descriptorBufferInfo[33];
    writeDescriptorSet[33].pImageInfo       = nullptr;
    writeDescriptorSet[33].pTexelBufferView = nullptr;

    descriptorBufferInfo[34]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[34].buffer = m_vdata.m_pAccelStruct_m_allNodePairsBuffer;
    descriptorBufferInfo[34].offset = m_vdata.m_pAccelStruct_m_allNodePairsOffset;
    descriptorBufferInfo[34].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[34]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[34].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[34].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[34].dstBinding       = 34;
    writeDescriptorSet[34].descriptorCount  = 1;
    writeDescriptorSet[34].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[34].pBufferInfo      = &descriptorBufferInfo[34];
    writeDescriptorSet[34].pImageInfo       = nullptr;
    writeDescriptorSet[34].pTexelBufferView = nullptr;

    descriptorBufferInfo[35]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[35].buffer = m_vdata.m_pAccelStruct_m_geomDataBuffer;
    descriptorBufferInfo[35].offset = m_vdata.m_pAccelStruct_m_geomDataOffset;
    descriptorBufferInfo[35].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[35]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[35].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[35].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[35].dstBinding       = 35;
    writeDescriptorSet[35].descriptorCount  = 1;
    writeDescriptorSet[35].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[35].pBufferInfo      = &descriptorBufferInfo[35];
    writeDescriptorSet[35].pImageInfo       = nullptr;
    writeDescriptorSet[35].pTexelBufferView = nullptr;

    descriptorBufferInfo[36]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[36].buffer = m_vdata.m_pAccelStruct_m_geomTagsBuffer;
    descriptorBufferInfo[36].offset = m_vdata.m_pAccelStruct_m_geomTagsOffset;
    descriptorBufferInfo[36].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[36]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[36].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[36].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[36].dstBinding       = 36;
    writeDescriptorSet[36].descriptorCount  = 1;
    writeDescriptorSet[36].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[36].pBufferInfo      = &descriptorBufferInfo[36];
    writeDescriptorSet[36].pImageInfo       = nullptr;
    writeDescriptorSet[36].pTexelBufferView = nullptr;

    descriptorBufferInfo[37]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[37].buffer = m_vdata.m_pAccelStruct_m_indicesBuffer;
    descriptorBufferInfo[37].offset = m_vdata.m_pAccelStruct_m_indicesOffset;
    descriptorBufferInfo[37].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[37]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[37].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[37].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[37].dstBinding       = 37;
    writeDescriptorSet[37].descriptorCount  = 1;
    writeDescriptorSet[37].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[37].pBufferInfo      = &descriptorBufferInfo[37];
    writeDescriptorSet[37].pImageInfo       = nullptr;
    writeDescriptorSet[37].pTexelBufferView = nullptr;

    descriptorBufferInfo[38]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[38].buffer = m_vdata.m_pAccelStruct_m_instanceDataBuffer;
    descriptorBufferInfo[38].offset = m_vdata.m_pAccelStruct_m_instanceDataOffset;
    descriptorBufferInfo[38].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[38]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[38].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[38].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[38].dstBinding       = 38;
    writeDescriptorSet[38].descriptorCount  = 1;
    writeDescriptorSet[38].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[38].pBufferInfo      = &descriptorBufferInfo[38];
    writeDescriptorSet[38].pImageInfo       = nullptr;
    writeDescriptorSet[38].pTexelBufferView = nullptr;

    descriptorBufferInfo[39]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[39].buffer = m_vdata.m_pAccelStruct_m_nodesTLASBuffer;
    descriptorBufferInfo[39].offset = m_vdata.m_pAccelStruct_m_nodesTLASOffset;
    descriptorBufferInfo[39].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[39]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[39].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[39].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[39].dstBinding       = 39;
    writeDescriptorSet[39].descriptorCount  = 1;
    writeDescriptorSet[39].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[39].pBufferInfo      = &descriptorBufferInfo[39];
    writeDescriptorSet[39].pImageInfo       = nullptr;
    writeDescriptorSet[39].pTexelBufferView = nullptr;

    descriptorBufferInfo[40]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[40].buffer = m_vdata.m_pAccelStruct_m_origNodesBuffer;
    descriptorBufferInfo[40].offset = m_vdata.m_pAccelStruct_m_origNodesOffset;
    descriptorBufferInfo[40].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[40]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[40].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[40].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[40].dstBinding       = 40;
    writeDescriptorSet[40].descriptorCount  = 1;
    writeDescriptorSet[40].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[40].pBufferInfo      = &descriptorBufferInfo[40];
    writeDescriptorSet[40].pImageInfo       = nullptr;
    writeDescriptorSet[40].pTexelBufferView = nullptr;

    descriptorBufferInfo[41]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[41].buffer = m_vdata.m_pAccelStruct_m_primIdCountBuffer;
    descriptorBufferInfo[41].offset = m_vdata.m_pAccelStruct_m_primIdCountOffset;
    descriptorBufferInfo[41].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[41]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[41].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[41].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[41].dstBinding       = 41;
    writeDescriptorSet[41].descriptorCount  = 1;
    writeDescriptorSet[41].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[41].pBufferInfo      = &descriptorBufferInfo[41];
    writeDescriptorSet[41].pImageInfo       = nullptr;
    writeDescriptorSet[41].pTexelBufferView = nullptr;

    descriptorBufferInfo[42]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[42].buffer = m_vdata.m_pAccelStruct_m_primIndicesBuffer;
    descriptorBufferInfo[42].offset = m_vdata.m_pAccelStruct_m_primIndicesOffset;
    descriptorBufferInfo[42].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[42]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[42].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[42].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[42].dstBinding       = 42;
    writeDescriptorSet[42].descriptorCount  = 1;
    writeDescriptorSet[42].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[42].pBufferInfo      = &descriptorBufferInfo[42];
    writeDescriptorSet[42].pImageInfo       = nullptr;
    writeDescriptorSet[42].pTexelBufferView = nullptr;

    descriptorBufferInfo[43]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[43].buffer = m_vdata.m_pAccelStruct_m_vertNormBuffer;
    descriptorBufferInfo[43].offset = m_vdata.m_pAccelStruct_m_vertNormOffset;
    descriptorBufferInfo[43].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[43]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[43].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[43].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[43].dstBinding       = 43;
    writeDescriptorSet[43].descriptorCount  = 1;
    writeDescriptorSet[43].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[43].pBufferInfo      = &descriptorBufferInfo[43];
    writeDescriptorSet[43].pImageInfo       = nullptr;
    writeDescriptorSet[43].pTexelBufferView = nullptr;

    descriptorBufferInfo[44]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[44].buffer = m_vdata.m_pAccelStruct_m_vertPosBuffer;
    descriptorBufferInfo[44].offset = m_vdata.m_pAccelStruct_m_vertPosOffset;
    descriptorBufferInfo[44].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[44]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[44].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[44].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[44].dstBinding       = 44;
    writeDescriptorSet[44].descriptorCount  = 1;
    writeDescriptorSet[44].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[44].pBufferInfo      = &descriptorBufferInfo[44];
    writeDescriptorSet[44].pImageInfo       = nullptr;
    writeDescriptorSet[44].pTexelBufferView = nullptr;

    descriptorBufferInfo[45]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[45].buffer = m_vdata.m_pAccelStruct_startEndBuffer;
    descriptorBufferInfo[45].offset = m_vdata.m_pAccelStruct_startEndOffset;
    descriptorBufferInfo[45].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[45]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[45].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[45].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[45].dstBinding       = 45;
    writeDescriptorSet[45].descriptorCount  = 1;
    writeDescriptorSet[45].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[45].pBufferInfo      = &descriptorBufferInfo[45];
    writeDescriptorSet[45].pImageInfo       = nullptr;
    writeDescriptorSet[45].pTexelBufferView = nullptr;

    descriptorBufferInfo[46]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[46].buffer = m_vdata.m_packedXYBuffer;
    descriptorBufferInfo[46].offset = m_vdata.m_packedXYOffset;
    descriptorBufferInfo[46].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[46]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[46].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[46].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[46].dstBinding       = 46;
    writeDescriptorSet[46].descriptorCount  = 1;
    writeDescriptorSet[46].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[46].pBufferInfo      = &descriptorBufferInfo[46];
    writeDescriptorSet[46].pImageInfo       = nullptr;
    writeDescriptorSet[46].pTexelBufferView = nullptr;

    descriptorBufferInfo[47]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[47].buffer = m_vdata.m_transparencyBufferBuffer;
    descriptorBufferInfo[47].offset = m_vdata.m_transparencyBufferOffset;
    descriptorBufferInfo[47].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[47]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[47].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[47].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[47].dstBinding       = 47;
    writeDescriptorSet[47].descriptorCount  = 1;
    writeDescriptorSet[47].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[47].pBufferInfo      = &descriptorBufferInfo[47];
    writeDescriptorSet[47].pImageInfo       = nullptr;
    writeDescriptorSet[47].pTexelBufferView = nullptr;

    descriptorBufferInfo[48]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[48].buffer = m_classDataBuffer;
    descriptorBufferInfo[48].offset = 0;
    descriptorBufferInfo[48].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[48]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[48].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[48].dstSet           = m_allGeneratedDS[0];
    writeDescriptorSet[48].dstBinding       = 48;
    writeDescriptorSet[48].descriptorCount  = 1;
    writeDescriptorSet[48].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[48].pBufferInfo      = &descriptorBufferInfo[48];
    writeDescriptorSet[48].pImageInfo       = nullptr;
    writeDescriptorSet[48].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
  // descriptor set #1: ResolveCommonWithTransparencyCmd (["m_colorBuffer.data()","m_gBuffer","m_geomOffsets","m_indices","m_instanceTransformInvTransposed","m_lights","m_matIdOffsets","m_matIdbyPrimId","m_materials","m_normals","m_textures","m_transparencyBuffer","m_vertices"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 13 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  13 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   13 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = m_vdata.m_colorBufferBuffer;
    descriptorBufferInfo[0].offset = m_vdata.m_colorBufferOffset;
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
    descriptorBufferInfo[1].buffer = m_vdata.m_gBufferBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_gBufferOffset;
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
    descriptorBufferInfo[2].buffer = m_vdata.m_geomOffsetsBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_geomOffsetsOffset;
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
    descriptorBufferInfo[3].buffer = m_vdata.m_indicesBuffer;
    descriptorBufferInfo[3].offset = m_vdata.m_indicesOffset;
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
    descriptorBufferInfo[4].buffer = m_vdata.m_instanceTransformInvTransposedBuffer;
    descriptorBufferInfo[4].offset = m_vdata.m_instanceTransformInvTransposedOffset;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[1];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    descriptorBufferInfo[5]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[5].buffer = m_vdata.m_lightsBuffer;
    descriptorBufferInfo[5].offset = m_vdata.m_lightsOffset;
    descriptorBufferInfo[5].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[5]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[5].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[5].dstSet           = m_allGeneratedDS[1];
    writeDescriptorSet[5].dstBinding       = 5;
    writeDescriptorSet[5].descriptorCount  = 1;
    writeDescriptorSet[5].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[5].pBufferInfo      = &descriptorBufferInfo[5];
    writeDescriptorSet[5].pImageInfo       = nullptr;
    writeDescriptorSet[5].pTexelBufferView = nullptr;

    descriptorBufferInfo[6]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[6].buffer = m_vdata.m_matIdOffsetsBuffer;
    descriptorBufferInfo[6].offset = m_vdata.m_matIdOffsetsOffset;
    descriptorBufferInfo[6].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[6]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[6].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[6].dstSet           = m_allGeneratedDS[1];
    writeDescriptorSet[6].dstBinding       = 6;
    writeDescriptorSet[6].descriptorCount  = 1;
    writeDescriptorSet[6].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[6].pBufferInfo      = &descriptorBufferInfo[6];
    writeDescriptorSet[6].pImageInfo       = nullptr;
    writeDescriptorSet[6].pTexelBufferView = nullptr;

    descriptorBufferInfo[7]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[7].buffer = m_vdata.m_matIdbyPrimIdBuffer;
    descriptorBufferInfo[7].offset = m_vdata.m_matIdbyPrimIdOffset;
    descriptorBufferInfo[7].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[7]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[7].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[7].dstSet           = m_allGeneratedDS[1];
    writeDescriptorSet[7].dstBinding       = 7;
    writeDescriptorSet[7].descriptorCount  = 1;
    writeDescriptorSet[7].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[7].pBufferInfo      = &descriptorBufferInfo[7];
    writeDescriptorSet[7].pImageInfo       = nullptr;
    writeDescriptorSet[7].pTexelBufferView = nullptr;

    descriptorBufferInfo[8]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[8].buffer = m_vdata.m_materialsBuffer;
    descriptorBufferInfo[8].offset = m_vdata.m_materialsOffset;
    descriptorBufferInfo[8].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[8]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[8].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[8].dstSet           = m_allGeneratedDS[1];
    writeDescriptorSet[8].dstBinding       = 8;
    writeDescriptorSet[8].descriptorCount  = 1;
    writeDescriptorSet[8].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[8].pBufferInfo      = &descriptorBufferInfo[8];
    writeDescriptorSet[8].pImageInfo       = nullptr;
    writeDescriptorSet[8].pTexelBufferView = nullptr;

    descriptorBufferInfo[9]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[9].buffer = m_vdata.m_normalsBuffer;
    descriptorBufferInfo[9].offset = m_vdata.m_normalsOffset;
    descriptorBufferInfo[9].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[9]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[9].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[9].dstSet           = m_allGeneratedDS[1];
    writeDescriptorSet[9].dstBinding       = 9;
    writeDescriptorSet[9].descriptorCount  = 1;
    writeDescriptorSet[9].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[9].pBufferInfo      = &descriptorBufferInfo[9];
    writeDescriptorSet[9].pImageInfo       = nullptr;
    writeDescriptorSet[9].pTexelBufferView = nullptr;

    std::vector<VkDescriptorImageInfo> m_texturesInfo(m_vdata.m_texturesArrayMaxSize);
    for(size_t i=0; i<m_vdata.m_texturesArrayMaxSize; i++)
    {
      if(i < m_textures.size())
      {
        m_texturesInfo[i].sampler     = m_vdata.m_texturesArraySampler[i];
        m_texturesInfo[i].imageView   = m_vdata.m_texturesArrayView   [i];
        m_texturesInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
      else
      {
        m_texturesInfo[i].sampler     = m_vdata.m_texturesArraySampler[0];
        m_texturesInfo[i].imageView   = m_vdata.m_texturesArrayView   [0];
        m_texturesInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
    }
    writeDescriptorSet[10]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[10].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[10].dstSet           = m_allGeneratedDS[1];
    writeDescriptorSet[10].dstBinding       = 10;
    writeDescriptorSet[10].descriptorCount  = 1;
    writeDescriptorSet[10].descriptorCount  = m_texturesInfo.size();
    writeDescriptorSet[10].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet[10].pBufferInfo      = nullptr;
    writeDescriptorSet[10].pImageInfo       = m_texturesInfo.data();
    writeDescriptorSet[10].pTexelBufferView = nullptr;

    descriptorBufferInfo[11]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[11].buffer = m_vdata.m_transparencyBufferBuffer;
    descriptorBufferInfo[11].offset = m_vdata.m_transparencyBufferOffset;
    descriptorBufferInfo[11].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[11]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[11].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[11].dstSet           = m_allGeneratedDS[1];
    writeDescriptorSet[11].dstBinding       = 11;
    writeDescriptorSet[11].descriptorCount  = 1;
    writeDescriptorSet[11].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[11].pBufferInfo      = &descriptorBufferInfo[11];
    writeDescriptorSet[11].pImageInfo       = nullptr;
    writeDescriptorSet[11].pTexelBufferView = nullptr;

    descriptorBufferInfo[12]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[12].buffer = m_vdata.m_verticesBuffer;
    descriptorBufferInfo[12].offset = m_vdata.m_verticesOffset;
    descriptorBufferInfo[12].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[12]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[12].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[12].dstSet           = m_allGeneratedDS[1];
    writeDescriptorSet[12].dstBinding       = 12;
    writeDescriptorSet[12].descriptorCount  = 1;
    writeDescriptorSet[12].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[12].pBufferInfo      = &descriptorBufferInfo[12];
    writeDescriptorSet[12].pImageInfo       = nullptr;
    writeDescriptorSet[12].pTexelBufferView = nullptr;

    descriptorBufferInfo[13]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[13].buffer = m_classDataBuffer;
    descriptorBufferInfo[13].offset = 0;
    descriptorBufferInfo[13].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[13]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[13].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[13].dstSet           = m_allGeneratedDS[1];
    writeDescriptorSet[13].dstBinding       = 13;
    writeDescriptorSet[13].descriptorCount  = 1;
    writeDescriptorSet[13].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[13].pBufferInfo      = &descriptorBufferInfo[13];
    writeDescriptorSet[13].pImageInfo       = nullptr;
    writeDescriptorSet[13].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
  // descriptor set #2: FillGbufferCmd (["m_gBuffer","m_pAccelStruct_m_GraphicsPrimHeaders","m_pAccelStruct_m_GraphicsPrimPoints","m_pAccelStruct_m_RotAddTable","m_pAccelStruct_m_SCom2Data","m_pAccelStruct_m_SCom2Headers","m_pAccelStruct_m_SdfCompactOctreeRotModifiers","m_pAccelStruct_m_SdfCompactOctreeRotTransforms","m_pAccelStruct_m_SdfCompactOctreeV3Data","m_pAccelStruct_m_SdfDAGChildEdges","m_pAccelStruct_m_SdfDAGDataEdges","m_pAccelStruct_m_SdfDAGDistances","m_pAccelStruct_m_SdfDAGHeaders","m_pAccelStruct_m_SdfDAGNodes","m_pAccelStruct_m_SdfDAGTranspositions","m_pAccelStruct_m_SdfFrameOctreeNodes","m_pAccelStruct_m_SdfFrameOctreeRoots","m_pAccelStruct_m_SdfFrameOctreeTexNodes","m_pAccelStruct_m_SdfFrameOctreeTexRoots","m_pAccelStruct_m_SdfGridData","m_pAccelStruct_m_SdfGridOffsets","m_pAccelStruct_m_SdfGridSizes","m_pAccelStruct_m_SdfMultiOctreeNodes","m_pAccelStruct_m_SdfMultiOctreeValues","m_pAccelStruct_m_SdfSBSData","m_pAccelStruct_m_SdfSBSDataF","m_pAccelStruct_m_SdfSBSHeaders","m_pAccelStruct_m_SdfSBSNodes","m_pAccelStruct_m_SdfSBSRoots","m_pAccelStruct_m_SdfSVSNodes","m_pAccelStruct_m_SdfSVSRoots","m_pAccelStruct_m_allNodePairs","m_pAccelStruct_m_geomData","m_pAccelStruct_m_geomTags","m_pAccelStruct_m_indices","m_pAccelStruct_m_instanceData","m_pAccelStruct_m_nodesTLAS","m_pAccelStruct_m_origNodes","m_pAccelStruct_m_primIdCount","m_pAccelStruct_m_primIndices","m_pAccelStruct_m_vertNorm","m_pAccelStruct_m_vertPos","m_pAccelStruct_startEnd","m_packedXY"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 44 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  44 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   44 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = m_vdata.m_gBufferBuffer;
    descriptorBufferInfo[0].offset = m_vdata.m_gBufferOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = m_vdata.m_pAccelStruct_m_GraphicsPrimHeadersBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_pAccelStruct_m_GraphicsPrimHeadersOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_vdata.m_pAccelStruct_m_GraphicsPrimPointsBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_pAccelStruct_m_GraphicsPrimPointsOffset;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    descriptorBufferInfo[3]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[3].buffer = m_vdata.m_pAccelStruct_m_RotAddTableBuffer;
    descriptorBufferInfo[3].offset = m_vdata.m_pAccelStruct_m_RotAddTableOffset;
    descriptorBufferInfo[3].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[3]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[3].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[3].dstBinding       = 3;
    writeDescriptorSet[3].descriptorCount  = 1;
    writeDescriptorSet[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[3].pBufferInfo      = &descriptorBufferInfo[3];
    writeDescriptorSet[3].pImageInfo       = nullptr;
    writeDescriptorSet[3].pTexelBufferView = nullptr;

    descriptorBufferInfo[4]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[4].buffer = m_vdata.m_pAccelStruct_m_SCom2DataBuffer;
    descriptorBufferInfo[4].offset = m_vdata.m_pAccelStruct_m_SCom2DataOffset;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    descriptorBufferInfo[5]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[5].buffer = m_vdata.m_pAccelStruct_m_SCom2HeadersBuffer;
    descriptorBufferInfo[5].offset = m_vdata.m_pAccelStruct_m_SCom2HeadersOffset;
    descriptorBufferInfo[5].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[5]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[5].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[5].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[5].dstBinding       = 5;
    writeDescriptorSet[5].descriptorCount  = 1;
    writeDescriptorSet[5].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[5].pBufferInfo      = &descriptorBufferInfo[5];
    writeDescriptorSet[5].pImageInfo       = nullptr;
    writeDescriptorSet[5].pTexelBufferView = nullptr;

    descriptorBufferInfo[6]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[6].buffer = m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotModifiersBuffer;
    descriptorBufferInfo[6].offset = m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotModifiersOffset;
    descriptorBufferInfo[6].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[6]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[6].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[6].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[6].dstBinding       = 6;
    writeDescriptorSet[6].descriptorCount  = 1;
    writeDescriptorSet[6].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[6].pBufferInfo      = &descriptorBufferInfo[6];
    writeDescriptorSet[6].pImageInfo       = nullptr;
    writeDescriptorSet[6].pTexelBufferView = nullptr;

    descriptorBufferInfo[7]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[7].buffer = m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotTransformsBuffer;
    descriptorBufferInfo[7].offset = m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotTransformsOffset;
    descriptorBufferInfo[7].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[7]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[7].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[7].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[7].dstBinding       = 7;
    writeDescriptorSet[7].descriptorCount  = 1;
    writeDescriptorSet[7].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[7].pBufferInfo      = &descriptorBufferInfo[7];
    writeDescriptorSet[7].pImageInfo       = nullptr;
    writeDescriptorSet[7].pTexelBufferView = nullptr;

    descriptorBufferInfo[8]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[8].buffer = m_vdata.m_pAccelStruct_m_SdfCompactOctreeV3DataBuffer;
    descriptorBufferInfo[8].offset = m_vdata.m_pAccelStruct_m_SdfCompactOctreeV3DataOffset;
    descriptorBufferInfo[8].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[8]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[8].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[8].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[8].dstBinding       = 8;
    writeDescriptorSet[8].descriptorCount  = 1;
    writeDescriptorSet[8].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[8].pBufferInfo      = &descriptorBufferInfo[8];
    writeDescriptorSet[8].pImageInfo       = nullptr;
    writeDescriptorSet[8].pTexelBufferView = nullptr;

    descriptorBufferInfo[9]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[9].buffer = m_vdata.m_pAccelStruct_m_SdfDAGChildEdgesBuffer;
    descriptorBufferInfo[9].offset = m_vdata.m_pAccelStruct_m_SdfDAGChildEdgesOffset;
    descriptorBufferInfo[9].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[9]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[9].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[9].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[9].dstBinding       = 9;
    writeDescriptorSet[9].descriptorCount  = 1;
    writeDescriptorSet[9].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[9].pBufferInfo      = &descriptorBufferInfo[9];
    writeDescriptorSet[9].pImageInfo       = nullptr;
    writeDescriptorSet[9].pTexelBufferView = nullptr;

    descriptorBufferInfo[10]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[10].buffer = m_vdata.m_pAccelStruct_m_SdfDAGDataEdgesBuffer;
    descriptorBufferInfo[10].offset = m_vdata.m_pAccelStruct_m_SdfDAGDataEdgesOffset;
    descriptorBufferInfo[10].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[10]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[10].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[10].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[10].dstBinding       = 10;
    writeDescriptorSet[10].descriptorCount  = 1;
    writeDescriptorSet[10].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[10].pBufferInfo      = &descriptorBufferInfo[10];
    writeDescriptorSet[10].pImageInfo       = nullptr;
    writeDescriptorSet[10].pTexelBufferView = nullptr;

    descriptorBufferInfo[11]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[11].buffer = m_vdata.m_pAccelStruct_m_SdfDAGDistancesBuffer;
    descriptorBufferInfo[11].offset = m_vdata.m_pAccelStruct_m_SdfDAGDistancesOffset;
    descriptorBufferInfo[11].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[11]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[11].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[11].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[11].dstBinding       = 11;
    writeDescriptorSet[11].descriptorCount  = 1;
    writeDescriptorSet[11].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[11].pBufferInfo      = &descriptorBufferInfo[11];
    writeDescriptorSet[11].pImageInfo       = nullptr;
    writeDescriptorSet[11].pTexelBufferView = nullptr;

    descriptorBufferInfo[12]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[12].buffer = m_vdata.m_pAccelStruct_m_SdfDAGHeadersBuffer;
    descriptorBufferInfo[12].offset = m_vdata.m_pAccelStruct_m_SdfDAGHeadersOffset;
    descriptorBufferInfo[12].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[12]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[12].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[12].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[12].dstBinding       = 12;
    writeDescriptorSet[12].descriptorCount  = 1;
    writeDescriptorSet[12].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[12].pBufferInfo      = &descriptorBufferInfo[12];
    writeDescriptorSet[12].pImageInfo       = nullptr;
    writeDescriptorSet[12].pTexelBufferView = nullptr;

    descriptorBufferInfo[13]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[13].buffer = m_vdata.m_pAccelStruct_m_SdfDAGNodesBuffer;
    descriptorBufferInfo[13].offset = m_vdata.m_pAccelStruct_m_SdfDAGNodesOffset;
    descriptorBufferInfo[13].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[13]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[13].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[13].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[13].dstBinding       = 13;
    writeDescriptorSet[13].descriptorCount  = 1;
    writeDescriptorSet[13].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[13].pBufferInfo      = &descriptorBufferInfo[13];
    writeDescriptorSet[13].pImageInfo       = nullptr;
    writeDescriptorSet[13].pTexelBufferView = nullptr;

    descriptorBufferInfo[14]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[14].buffer = m_vdata.m_pAccelStruct_m_SdfDAGTranspositionsBuffer;
    descriptorBufferInfo[14].offset = m_vdata.m_pAccelStruct_m_SdfDAGTranspositionsOffset;
    descriptorBufferInfo[14].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[14]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[14].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[14].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[14].dstBinding       = 14;
    writeDescriptorSet[14].descriptorCount  = 1;
    writeDescriptorSet[14].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[14].pBufferInfo      = &descriptorBufferInfo[14];
    writeDescriptorSet[14].pImageInfo       = nullptr;
    writeDescriptorSet[14].pTexelBufferView = nullptr;

    descriptorBufferInfo[15]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[15].buffer = m_vdata.m_pAccelStruct_m_SdfFrameOctreeNodesBuffer;
    descriptorBufferInfo[15].offset = m_vdata.m_pAccelStruct_m_SdfFrameOctreeNodesOffset;
    descriptorBufferInfo[15].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[15]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[15].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[15].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[15].dstBinding       = 15;
    writeDescriptorSet[15].descriptorCount  = 1;
    writeDescriptorSet[15].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[15].pBufferInfo      = &descriptorBufferInfo[15];
    writeDescriptorSet[15].pImageInfo       = nullptr;
    writeDescriptorSet[15].pTexelBufferView = nullptr;

    descriptorBufferInfo[16]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[16].buffer = m_vdata.m_pAccelStruct_m_SdfFrameOctreeRootsBuffer;
    descriptorBufferInfo[16].offset = m_vdata.m_pAccelStruct_m_SdfFrameOctreeRootsOffset;
    descriptorBufferInfo[16].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[16]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[16].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[16].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[16].dstBinding       = 16;
    writeDescriptorSet[16].descriptorCount  = 1;
    writeDescriptorSet[16].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[16].pBufferInfo      = &descriptorBufferInfo[16];
    writeDescriptorSet[16].pImageInfo       = nullptr;
    writeDescriptorSet[16].pTexelBufferView = nullptr;

    descriptorBufferInfo[17]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[17].buffer = m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexNodesBuffer;
    descriptorBufferInfo[17].offset = m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexNodesOffset;
    descriptorBufferInfo[17].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[17]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[17].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[17].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[17].dstBinding       = 17;
    writeDescriptorSet[17].descriptorCount  = 1;
    writeDescriptorSet[17].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[17].pBufferInfo      = &descriptorBufferInfo[17];
    writeDescriptorSet[17].pImageInfo       = nullptr;
    writeDescriptorSet[17].pTexelBufferView = nullptr;

    descriptorBufferInfo[18]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[18].buffer = m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexRootsBuffer;
    descriptorBufferInfo[18].offset = m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexRootsOffset;
    descriptorBufferInfo[18].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[18]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[18].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[18].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[18].dstBinding       = 18;
    writeDescriptorSet[18].descriptorCount  = 1;
    writeDescriptorSet[18].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[18].pBufferInfo      = &descriptorBufferInfo[18];
    writeDescriptorSet[18].pImageInfo       = nullptr;
    writeDescriptorSet[18].pTexelBufferView = nullptr;

    descriptorBufferInfo[19]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[19].buffer = m_vdata.m_pAccelStruct_m_SdfGridDataBuffer;
    descriptorBufferInfo[19].offset = m_vdata.m_pAccelStruct_m_SdfGridDataOffset;
    descriptorBufferInfo[19].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[19]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[19].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[19].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[19].dstBinding       = 19;
    writeDescriptorSet[19].descriptorCount  = 1;
    writeDescriptorSet[19].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[19].pBufferInfo      = &descriptorBufferInfo[19];
    writeDescriptorSet[19].pImageInfo       = nullptr;
    writeDescriptorSet[19].pTexelBufferView = nullptr;

    descriptorBufferInfo[20]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[20].buffer = m_vdata.m_pAccelStruct_m_SdfGridOffsetsBuffer;
    descriptorBufferInfo[20].offset = m_vdata.m_pAccelStruct_m_SdfGridOffsetsOffset;
    descriptorBufferInfo[20].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[20]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[20].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[20].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[20].dstBinding       = 20;
    writeDescriptorSet[20].descriptorCount  = 1;
    writeDescriptorSet[20].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[20].pBufferInfo      = &descriptorBufferInfo[20];
    writeDescriptorSet[20].pImageInfo       = nullptr;
    writeDescriptorSet[20].pTexelBufferView = nullptr;

    descriptorBufferInfo[21]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[21].buffer = m_vdata.m_pAccelStruct_m_SdfGridSizesBuffer;
    descriptorBufferInfo[21].offset = m_vdata.m_pAccelStruct_m_SdfGridSizesOffset;
    descriptorBufferInfo[21].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[21]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[21].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[21].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[21].dstBinding       = 21;
    writeDescriptorSet[21].descriptorCount  = 1;
    writeDescriptorSet[21].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[21].pBufferInfo      = &descriptorBufferInfo[21];
    writeDescriptorSet[21].pImageInfo       = nullptr;
    writeDescriptorSet[21].pTexelBufferView = nullptr;

    descriptorBufferInfo[22]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[22].buffer = m_vdata.m_pAccelStruct_m_SdfMultiOctreeNodesBuffer;
    descriptorBufferInfo[22].offset = m_vdata.m_pAccelStruct_m_SdfMultiOctreeNodesOffset;
    descriptorBufferInfo[22].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[22]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[22].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[22].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[22].dstBinding       = 22;
    writeDescriptorSet[22].descriptorCount  = 1;
    writeDescriptorSet[22].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[22].pBufferInfo      = &descriptorBufferInfo[22];
    writeDescriptorSet[22].pImageInfo       = nullptr;
    writeDescriptorSet[22].pTexelBufferView = nullptr;

    descriptorBufferInfo[23]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[23].buffer = m_vdata.m_pAccelStruct_m_SdfMultiOctreeValuesBuffer;
    descriptorBufferInfo[23].offset = m_vdata.m_pAccelStruct_m_SdfMultiOctreeValuesOffset;
    descriptorBufferInfo[23].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[23]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[23].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[23].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[23].dstBinding       = 23;
    writeDescriptorSet[23].descriptorCount  = 1;
    writeDescriptorSet[23].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[23].pBufferInfo      = &descriptorBufferInfo[23];
    writeDescriptorSet[23].pImageInfo       = nullptr;
    writeDescriptorSet[23].pTexelBufferView = nullptr;

    descriptorBufferInfo[24]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[24].buffer = m_vdata.m_pAccelStruct_m_SdfSBSDataBuffer;
    descriptorBufferInfo[24].offset = m_vdata.m_pAccelStruct_m_SdfSBSDataOffset;
    descriptorBufferInfo[24].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[24]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[24].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[24].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[24].dstBinding       = 24;
    writeDescriptorSet[24].descriptorCount  = 1;
    writeDescriptorSet[24].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[24].pBufferInfo      = &descriptorBufferInfo[24];
    writeDescriptorSet[24].pImageInfo       = nullptr;
    writeDescriptorSet[24].pTexelBufferView = nullptr;

    descriptorBufferInfo[25]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[25].buffer = m_vdata.m_pAccelStruct_m_SdfSBSDataFBuffer;
    descriptorBufferInfo[25].offset = m_vdata.m_pAccelStruct_m_SdfSBSDataFOffset;
    descriptorBufferInfo[25].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[25]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[25].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[25].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[25].dstBinding       = 25;
    writeDescriptorSet[25].descriptorCount  = 1;
    writeDescriptorSet[25].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[25].pBufferInfo      = &descriptorBufferInfo[25];
    writeDescriptorSet[25].pImageInfo       = nullptr;
    writeDescriptorSet[25].pTexelBufferView = nullptr;

    descriptorBufferInfo[26]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[26].buffer = m_vdata.m_pAccelStruct_m_SdfSBSHeadersBuffer;
    descriptorBufferInfo[26].offset = m_vdata.m_pAccelStruct_m_SdfSBSHeadersOffset;
    descriptorBufferInfo[26].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[26]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[26].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[26].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[26].dstBinding       = 26;
    writeDescriptorSet[26].descriptorCount  = 1;
    writeDescriptorSet[26].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[26].pBufferInfo      = &descriptorBufferInfo[26];
    writeDescriptorSet[26].pImageInfo       = nullptr;
    writeDescriptorSet[26].pTexelBufferView = nullptr;

    descriptorBufferInfo[27]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[27].buffer = m_vdata.m_pAccelStruct_m_SdfSBSNodesBuffer;
    descriptorBufferInfo[27].offset = m_vdata.m_pAccelStruct_m_SdfSBSNodesOffset;
    descriptorBufferInfo[27].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[27]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[27].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[27].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[27].dstBinding       = 27;
    writeDescriptorSet[27].descriptorCount  = 1;
    writeDescriptorSet[27].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[27].pBufferInfo      = &descriptorBufferInfo[27];
    writeDescriptorSet[27].pImageInfo       = nullptr;
    writeDescriptorSet[27].pTexelBufferView = nullptr;

    descriptorBufferInfo[28]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[28].buffer = m_vdata.m_pAccelStruct_m_SdfSBSRootsBuffer;
    descriptorBufferInfo[28].offset = m_vdata.m_pAccelStruct_m_SdfSBSRootsOffset;
    descriptorBufferInfo[28].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[28]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[28].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[28].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[28].dstBinding       = 28;
    writeDescriptorSet[28].descriptorCount  = 1;
    writeDescriptorSet[28].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[28].pBufferInfo      = &descriptorBufferInfo[28];
    writeDescriptorSet[28].pImageInfo       = nullptr;
    writeDescriptorSet[28].pTexelBufferView = nullptr;

    descriptorBufferInfo[29]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[29].buffer = m_vdata.m_pAccelStruct_m_SdfSVSNodesBuffer;
    descriptorBufferInfo[29].offset = m_vdata.m_pAccelStruct_m_SdfSVSNodesOffset;
    descriptorBufferInfo[29].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[29]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[29].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[29].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[29].dstBinding       = 29;
    writeDescriptorSet[29].descriptorCount  = 1;
    writeDescriptorSet[29].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[29].pBufferInfo      = &descriptorBufferInfo[29];
    writeDescriptorSet[29].pImageInfo       = nullptr;
    writeDescriptorSet[29].pTexelBufferView = nullptr;

    descriptorBufferInfo[30]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[30].buffer = m_vdata.m_pAccelStruct_m_SdfSVSRootsBuffer;
    descriptorBufferInfo[30].offset = m_vdata.m_pAccelStruct_m_SdfSVSRootsOffset;
    descriptorBufferInfo[30].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[30]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[30].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[30].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[30].dstBinding       = 30;
    writeDescriptorSet[30].descriptorCount  = 1;
    writeDescriptorSet[30].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[30].pBufferInfo      = &descriptorBufferInfo[30];
    writeDescriptorSet[30].pImageInfo       = nullptr;
    writeDescriptorSet[30].pTexelBufferView = nullptr;

    descriptorBufferInfo[31]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[31].buffer = m_vdata.m_pAccelStruct_m_allNodePairsBuffer;
    descriptorBufferInfo[31].offset = m_vdata.m_pAccelStruct_m_allNodePairsOffset;
    descriptorBufferInfo[31].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[31]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[31].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[31].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[31].dstBinding       = 31;
    writeDescriptorSet[31].descriptorCount  = 1;
    writeDescriptorSet[31].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[31].pBufferInfo      = &descriptorBufferInfo[31];
    writeDescriptorSet[31].pImageInfo       = nullptr;
    writeDescriptorSet[31].pTexelBufferView = nullptr;

    descriptorBufferInfo[32]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[32].buffer = m_vdata.m_pAccelStruct_m_geomDataBuffer;
    descriptorBufferInfo[32].offset = m_vdata.m_pAccelStruct_m_geomDataOffset;
    descriptorBufferInfo[32].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[32]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[32].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[32].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[32].dstBinding       = 32;
    writeDescriptorSet[32].descriptorCount  = 1;
    writeDescriptorSet[32].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[32].pBufferInfo      = &descriptorBufferInfo[32];
    writeDescriptorSet[32].pImageInfo       = nullptr;
    writeDescriptorSet[32].pTexelBufferView = nullptr;

    descriptorBufferInfo[33]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[33].buffer = m_vdata.m_pAccelStruct_m_geomTagsBuffer;
    descriptorBufferInfo[33].offset = m_vdata.m_pAccelStruct_m_geomTagsOffset;
    descriptorBufferInfo[33].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[33]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[33].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[33].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[33].dstBinding       = 33;
    writeDescriptorSet[33].descriptorCount  = 1;
    writeDescriptorSet[33].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[33].pBufferInfo      = &descriptorBufferInfo[33];
    writeDescriptorSet[33].pImageInfo       = nullptr;
    writeDescriptorSet[33].pTexelBufferView = nullptr;

    descriptorBufferInfo[34]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[34].buffer = m_vdata.m_pAccelStruct_m_indicesBuffer;
    descriptorBufferInfo[34].offset = m_vdata.m_pAccelStruct_m_indicesOffset;
    descriptorBufferInfo[34].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[34]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[34].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[34].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[34].dstBinding       = 34;
    writeDescriptorSet[34].descriptorCount  = 1;
    writeDescriptorSet[34].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[34].pBufferInfo      = &descriptorBufferInfo[34];
    writeDescriptorSet[34].pImageInfo       = nullptr;
    writeDescriptorSet[34].pTexelBufferView = nullptr;

    descriptorBufferInfo[35]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[35].buffer = m_vdata.m_pAccelStruct_m_instanceDataBuffer;
    descriptorBufferInfo[35].offset = m_vdata.m_pAccelStruct_m_instanceDataOffset;
    descriptorBufferInfo[35].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[35]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[35].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[35].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[35].dstBinding       = 35;
    writeDescriptorSet[35].descriptorCount  = 1;
    writeDescriptorSet[35].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[35].pBufferInfo      = &descriptorBufferInfo[35];
    writeDescriptorSet[35].pImageInfo       = nullptr;
    writeDescriptorSet[35].pTexelBufferView = nullptr;

    descriptorBufferInfo[36]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[36].buffer = m_vdata.m_pAccelStruct_m_nodesTLASBuffer;
    descriptorBufferInfo[36].offset = m_vdata.m_pAccelStruct_m_nodesTLASOffset;
    descriptorBufferInfo[36].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[36]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[36].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[36].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[36].dstBinding       = 36;
    writeDescriptorSet[36].descriptorCount  = 1;
    writeDescriptorSet[36].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[36].pBufferInfo      = &descriptorBufferInfo[36];
    writeDescriptorSet[36].pImageInfo       = nullptr;
    writeDescriptorSet[36].pTexelBufferView = nullptr;

    descriptorBufferInfo[37]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[37].buffer = m_vdata.m_pAccelStruct_m_origNodesBuffer;
    descriptorBufferInfo[37].offset = m_vdata.m_pAccelStruct_m_origNodesOffset;
    descriptorBufferInfo[37].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[37]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[37].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[37].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[37].dstBinding       = 37;
    writeDescriptorSet[37].descriptorCount  = 1;
    writeDescriptorSet[37].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[37].pBufferInfo      = &descriptorBufferInfo[37];
    writeDescriptorSet[37].pImageInfo       = nullptr;
    writeDescriptorSet[37].pTexelBufferView = nullptr;

    descriptorBufferInfo[38]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[38].buffer = m_vdata.m_pAccelStruct_m_primIdCountBuffer;
    descriptorBufferInfo[38].offset = m_vdata.m_pAccelStruct_m_primIdCountOffset;
    descriptorBufferInfo[38].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[38]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[38].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[38].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[38].dstBinding       = 38;
    writeDescriptorSet[38].descriptorCount  = 1;
    writeDescriptorSet[38].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[38].pBufferInfo      = &descriptorBufferInfo[38];
    writeDescriptorSet[38].pImageInfo       = nullptr;
    writeDescriptorSet[38].pTexelBufferView = nullptr;

    descriptorBufferInfo[39]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[39].buffer = m_vdata.m_pAccelStruct_m_primIndicesBuffer;
    descriptorBufferInfo[39].offset = m_vdata.m_pAccelStruct_m_primIndicesOffset;
    descriptorBufferInfo[39].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[39]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[39].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[39].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[39].dstBinding       = 39;
    writeDescriptorSet[39].descriptorCount  = 1;
    writeDescriptorSet[39].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[39].pBufferInfo      = &descriptorBufferInfo[39];
    writeDescriptorSet[39].pImageInfo       = nullptr;
    writeDescriptorSet[39].pTexelBufferView = nullptr;

    descriptorBufferInfo[40]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[40].buffer = m_vdata.m_pAccelStruct_m_vertNormBuffer;
    descriptorBufferInfo[40].offset = m_vdata.m_pAccelStruct_m_vertNormOffset;
    descriptorBufferInfo[40].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[40]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[40].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[40].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[40].dstBinding       = 40;
    writeDescriptorSet[40].descriptorCount  = 1;
    writeDescriptorSet[40].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[40].pBufferInfo      = &descriptorBufferInfo[40];
    writeDescriptorSet[40].pImageInfo       = nullptr;
    writeDescriptorSet[40].pTexelBufferView = nullptr;

    descriptorBufferInfo[41]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[41].buffer = m_vdata.m_pAccelStruct_m_vertPosBuffer;
    descriptorBufferInfo[41].offset = m_vdata.m_pAccelStruct_m_vertPosOffset;
    descriptorBufferInfo[41].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[41]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[41].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[41].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[41].dstBinding       = 41;
    writeDescriptorSet[41].descriptorCount  = 1;
    writeDescriptorSet[41].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[41].pBufferInfo      = &descriptorBufferInfo[41];
    writeDescriptorSet[41].pImageInfo       = nullptr;
    writeDescriptorSet[41].pTexelBufferView = nullptr;

    descriptorBufferInfo[42]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[42].buffer = m_vdata.m_pAccelStruct_startEndBuffer;
    descriptorBufferInfo[42].offset = m_vdata.m_pAccelStruct_startEndOffset;
    descriptorBufferInfo[42].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[42]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[42].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[42].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[42].dstBinding       = 42;
    writeDescriptorSet[42].descriptorCount  = 1;
    writeDescriptorSet[42].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[42].pBufferInfo      = &descriptorBufferInfo[42];
    writeDescriptorSet[42].pImageInfo       = nullptr;
    writeDescriptorSet[42].pTexelBufferView = nullptr;

    descriptorBufferInfo[43]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[43].buffer = m_vdata.m_packedXYBuffer;
    descriptorBufferInfo[43].offset = m_vdata.m_packedXYOffset;
    descriptorBufferInfo[43].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[43]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[43].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[43].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[43].dstBinding       = 43;
    writeDescriptorSet[43].descriptorCount  = 1;
    writeDescriptorSet[43].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[43].pBufferInfo      = &descriptorBufferInfo[43];
    writeDescriptorSet[43].pImageInfo       = nullptr;
    writeDescriptorSet[43].pTexelBufferView = nullptr;

    descriptorBufferInfo[44]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[44].buffer = m_classDataBuffer;
    descriptorBufferInfo[44].offset = 0;
    descriptorBufferInfo[44].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[44]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[44].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[44].dstSet           = m_allGeneratedDS[2];
    writeDescriptorSet[44].dstBinding       = 44;
    writeDescriptorSet[44].descriptorCount  = 1;
    writeDescriptorSet[44].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[44].pBufferInfo      = &descriptorBufferInfo[44];
    writeDescriptorSet[44].pImageInfo       = nullptr;
    writeDescriptorSet[44].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
  // descriptor set #3: ResolveCommonCmd (["m_colorBuffer.data()","m_gBuffer","m_geomOffsets","m_indices","m_instanceTransformInvTransposed","m_lights","m_matIdOffsets","m_matIdbyPrimId","m_materials","m_normals","m_textures","m_vertices"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 12 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  12 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   12 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = m_vdata.m_colorBufferBuffer;
    descriptorBufferInfo[0].offset = m_vdata.m_colorBufferOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[3];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = m_vdata.m_gBufferBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_gBufferOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[3];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_vdata.m_geomOffsetsBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_geomOffsetsOffset;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[3];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    descriptorBufferInfo[3]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[3].buffer = m_vdata.m_indicesBuffer;
    descriptorBufferInfo[3].offset = m_vdata.m_indicesOffset;
    descriptorBufferInfo[3].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[3]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[3].dstSet           = m_allGeneratedDS[3];
    writeDescriptorSet[3].dstBinding       = 3;
    writeDescriptorSet[3].descriptorCount  = 1;
    writeDescriptorSet[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[3].pBufferInfo      = &descriptorBufferInfo[3];
    writeDescriptorSet[3].pImageInfo       = nullptr;
    writeDescriptorSet[3].pTexelBufferView = nullptr;

    descriptorBufferInfo[4]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[4].buffer = m_vdata.m_instanceTransformInvTransposedBuffer;
    descriptorBufferInfo[4].offset = m_vdata.m_instanceTransformInvTransposedOffset;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[3];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    descriptorBufferInfo[5]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[5].buffer = m_vdata.m_lightsBuffer;
    descriptorBufferInfo[5].offset = m_vdata.m_lightsOffset;
    descriptorBufferInfo[5].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[5]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[5].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[5].dstSet           = m_allGeneratedDS[3];
    writeDescriptorSet[5].dstBinding       = 5;
    writeDescriptorSet[5].descriptorCount  = 1;
    writeDescriptorSet[5].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[5].pBufferInfo      = &descriptorBufferInfo[5];
    writeDescriptorSet[5].pImageInfo       = nullptr;
    writeDescriptorSet[5].pTexelBufferView = nullptr;

    descriptorBufferInfo[6]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[6].buffer = m_vdata.m_matIdOffsetsBuffer;
    descriptorBufferInfo[6].offset = m_vdata.m_matIdOffsetsOffset;
    descriptorBufferInfo[6].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[6]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[6].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[6].dstSet           = m_allGeneratedDS[3];
    writeDescriptorSet[6].dstBinding       = 6;
    writeDescriptorSet[6].descriptorCount  = 1;
    writeDescriptorSet[6].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[6].pBufferInfo      = &descriptorBufferInfo[6];
    writeDescriptorSet[6].pImageInfo       = nullptr;
    writeDescriptorSet[6].pTexelBufferView = nullptr;

    descriptorBufferInfo[7]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[7].buffer = m_vdata.m_matIdbyPrimIdBuffer;
    descriptorBufferInfo[7].offset = m_vdata.m_matIdbyPrimIdOffset;
    descriptorBufferInfo[7].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[7]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[7].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[7].dstSet           = m_allGeneratedDS[3];
    writeDescriptorSet[7].dstBinding       = 7;
    writeDescriptorSet[7].descriptorCount  = 1;
    writeDescriptorSet[7].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[7].pBufferInfo      = &descriptorBufferInfo[7];
    writeDescriptorSet[7].pImageInfo       = nullptr;
    writeDescriptorSet[7].pTexelBufferView = nullptr;

    descriptorBufferInfo[8]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[8].buffer = m_vdata.m_materialsBuffer;
    descriptorBufferInfo[8].offset = m_vdata.m_materialsOffset;
    descriptorBufferInfo[8].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[8]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[8].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[8].dstSet           = m_allGeneratedDS[3];
    writeDescriptorSet[8].dstBinding       = 8;
    writeDescriptorSet[8].descriptorCount  = 1;
    writeDescriptorSet[8].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[8].pBufferInfo      = &descriptorBufferInfo[8];
    writeDescriptorSet[8].pImageInfo       = nullptr;
    writeDescriptorSet[8].pTexelBufferView = nullptr;

    descriptorBufferInfo[9]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[9].buffer = m_vdata.m_normalsBuffer;
    descriptorBufferInfo[9].offset = m_vdata.m_normalsOffset;
    descriptorBufferInfo[9].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[9]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[9].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[9].dstSet           = m_allGeneratedDS[3];
    writeDescriptorSet[9].dstBinding       = 9;
    writeDescriptorSet[9].descriptorCount  = 1;
    writeDescriptorSet[9].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[9].pBufferInfo      = &descriptorBufferInfo[9];
    writeDescriptorSet[9].pImageInfo       = nullptr;
    writeDescriptorSet[9].pTexelBufferView = nullptr;

    std::vector<VkDescriptorImageInfo> m_texturesInfo(m_vdata.m_texturesArrayMaxSize);
    for(size_t i=0; i<m_vdata.m_texturesArrayMaxSize; i++)
    {
      if(i < m_textures.size())
      {
        m_texturesInfo[i].sampler     = m_vdata.m_texturesArraySampler[i];
        m_texturesInfo[i].imageView   = m_vdata.m_texturesArrayView   [i];
        m_texturesInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
      else
      {
        m_texturesInfo[i].sampler     = m_vdata.m_texturesArraySampler[0];
        m_texturesInfo[i].imageView   = m_vdata.m_texturesArrayView   [0];
        m_texturesInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
    }
    writeDescriptorSet[10]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[10].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[10].dstSet           = m_allGeneratedDS[3];
    writeDescriptorSet[10].dstBinding       = 10;
    writeDescriptorSet[10].descriptorCount  = 1;
    writeDescriptorSet[10].descriptorCount  = m_texturesInfo.size();
    writeDescriptorSet[10].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet[10].pBufferInfo      = nullptr;
    writeDescriptorSet[10].pImageInfo       = m_texturesInfo.data();
    writeDescriptorSet[10].pTexelBufferView = nullptr;

    descriptorBufferInfo[11]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[11].buffer = m_vdata.m_verticesBuffer;
    descriptorBufferInfo[11].offset = m_vdata.m_verticesOffset;
    descriptorBufferInfo[11].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[11]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[11].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[11].dstSet           = m_allGeneratedDS[3];
    writeDescriptorSet[11].dstBinding       = 11;
    writeDescriptorSet[11].descriptorCount  = 1;
    writeDescriptorSet[11].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[11].pBufferInfo      = &descriptorBufferInfo[11];
    writeDescriptorSet[11].pImageInfo       = nullptr;
    writeDescriptorSet[11].pTexelBufferView = nullptr;

    descriptorBufferInfo[12]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[12].buffer = m_classDataBuffer;
    descriptorBufferInfo[12].offset = 0;
    descriptorBufferInfo[12].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[12]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[12].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[12].dstSet           = m_allGeneratedDS[3];
    writeDescriptorSet[12].dstBinding       = 12;
    writeDescriptorSet[12].descriptorCount  = 1;
    writeDescriptorSet[12].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[12].pBufferInfo      = &descriptorBufferInfo[12];
    writeDescriptorSet[12].pImageInfo       = nullptr;
    writeDescriptorSet[12].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
  // descriptor set #4: ResolveCommonCmd (["m_colorBuffer.data()","m_gBuffer","m_geomOffsets","m_indices","m_instanceTransformInvTransposed","m_lights","m_matIdOffsets","m_matIdbyPrimId","m_materials","m_normals","m_textures","m_vertices"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 12 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  12 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   12 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = m_vdata.m_colorBufferBuffer;
    descriptorBufferInfo[0].offset = m_vdata.m_colorBufferOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[4];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = m_vdata.m_gBufferBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_gBufferOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[4];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_vdata.m_geomOffsetsBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_geomOffsetsOffset;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[4];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    descriptorBufferInfo[3]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[3].buffer = m_vdata.m_indicesBuffer;
    descriptorBufferInfo[3].offset = m_vdata.m_indicesOffset;
    descriptorBufferInfo[3].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[3]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[3].dstSet           = m_allGeneratedDS[4];
    writeDescriptorSet[3].dstBinding       = 3;
    writeDescriptorSet[3].descriptorCount  = 1;
    writeDescriptorSet[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[3].pBufferInfo      = &descriptorBufferInfo[3];
    writeDescriptorSet[3].pImageInfo       = nullptr;
    writeDescriptorSet[3].pTexelBufferView = nullptr;

    descriptorBufferInfo[4]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[4].buffer = m_vdata.m_instanceTransformInvTransposedBuffer;
    descriptorBufferInfo[4].offset = m_vdata.m_instanceTransformInvTransposedOffset;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[4];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    descriptorBufferInfo[5]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[5].buffer = m_vdata.m_lightsBuffer;
    descriptorBufferInfo[5].offset = m_vdata.m_lightsOffset;
    descriptorBufferInfo[5].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[5]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[5].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[5].dstSet           = m_allGeneratedDS[4];
    writeDescriptorSet[5].dstBinding       = 5;
    writeDescriptorSet[5].descriptorCount  = 1;
    writeDescriptorSet[5].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[5].pBufferInfo      = &descriptorBufferInfo[5];
    writeDescriptorSet[5].pImageInfo       = nullptr;
    writeDescriptorSet[5].pTexelBufferView = nullptr;

    descriptorBufferInfo[6]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[6].buffer = m_vdata.m_matIdOffsetsBuffer;
    descriptorBufferInfo[6].offset = m_vdata.m_matIdOffsetsOffset;
    descriptorBufferInfo[6].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[6]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[6].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[6].dstSet           = m_allGeneratedDS[4];
    writeDescriptorSet[6].dstBinding       = 6;
    writeDescriptorSet[6].descriptorCount  = 1;
    writeDescriptorSet[6].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[6].pBufferInfo      = &descriptorBufferInfo[6];
    writeDescriptorSet[6].pImageInfo       = nullptr;
    writeDescriptorSet[6].pTexelBufferView = nullptr;

    descriptorBufferInfo[7]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[7].buffer = m_vdata.m_matIdbyPrimIdBuffer;
    descriptorBufferInfo[7].offset = m_vdata.m_matIdbyPrimIdOffset;
    descriptorBufferInfo[7].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[7]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[7].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[7].dstSet           = m_allGeneratedDS[4];
    writeDescriptorSet[7].dstBinding       = 7;
    writeDescriptorSet[7].descriptorCount  = 1;
    writeDescriptorSet[7].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[7].pBufferInfo      = &descriptorBufferInfo[7];
    writeDescriptorSet[7].pImageInfo       = nullptr;
    writeDescriptorSet[7].pTexelBufferView = nullptr;

    descriptorBufferInfo[8]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[8].buffer = m_vdata.m_materialsBuffer;
    descriptorBufferInfo[8].offset = m_vdata.m_materialsOffset;
    descriptorBufferInfo[8].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[8]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[8].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[8].dstSet           = m_allGeneratedDS[4];
    writeDescriptorSet[8].dstBinding       = 8;
    writeDescriptorSet[8].descriptorCount  = 1;
    writeDescriptorSet[8].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[8].pBufferInfo      = &descriptorBufferInfo[8];
    writeDescriptorSet[8].pImageInfo       = nullptr;
    writeDescriptorSet[8].pTexelBufferView = nullptr;

    descriptorBufferInfo[9]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[9].buffer = m_vdata.m_normalsBuffer;
    descriptorBufferInfo[9].offset = m_vdata.m_normalsOffset;
    descriptorBufferInfo[9].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[9]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[9].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[9].dstSet           = m_allGeneratedDS[4];
    writeDescriptorSet[9].dstBinding       = 9;
    writeDescriptorSet[9].descriptorCount  = 1;
    writeDescriptorSet[9].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[9].pBufferInfo      = &descriptorBufferInfo[9];
    writeDescriptorSet[9].pImageInfo       = nullptr;
    writeDescriptorSet[9].pTexelBufferView = nullptr;

    std::vector<VkDescriptorImageInfo> m_texturesInfo(m_vdata.m_texturesArrayMaxSize);
    for(size_t i=0; i<m_vdata.m_texturesArrayMaxSize; i++)
    {
      if(i < m_textures.size())
      {
        m_texturesInfo[i].sampler     = m_vdata.m_texturesArraySampler[i];
        m_texturesInfo[i].imageView   = m_vdata.m_texturesArrayView   [i];
        m_texturesInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
      else
      {
        m_texturesInfo[i].sampler     = m_vdata.m_texturesArraySampler[0];
        m_texturesInfo[i].imageView   = m_vdata.m_texturesArrayView   [0];
        m_texturesInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
    }
    writeDescriptorSet[10]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[10].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[10].dstSet           = m_allGeneratedDS[4];
    writeDescriptorSet[10].dstBinding       = 10;
    writeDescriptorSet[10].descriptorCount  = 1;
    writeDescriptorSet[10].descriptorCount  = m_texturesInfo.size();
    writeDescriptorSet[10].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet[10].pBufferInfo      = nullptr;
    writeDescriptorSet[10].pImageInfo       = m_texturesInfo.data();
    writeDescriptorSet[10].pTexelBufferView = nullptr;

    descriptorBufferInfo[11]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[11].buffer = m_vdata.m_verticesBuffer;
    descriptorBufferInfo[11].offset = m_vdata.m_verticesOffset;
    descriptorBufferInfo[11].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[11]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[11].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[11].dstSet           = m_allGeneratedDS[4];
    writeDescriptorSet[11].dstBinding       = 11;
    writeDescriptorSet[11].descriptorCount  = 1;
    writeDescriptorSet[11].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[11].pBufferInfo      = &descriptorBufferInfo[11];
    writeDescriptorSet[11].pImageInfo       = nullptr;
    writeDescriptorSet[11].pTexelBufferView = nullptr;

    descriptorBufferInfo[12]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[12].buffer = m_classDataBuffer;
    descriptorBufferInfo[12].offset = 0;
    descriptorBufferInfo[12].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[12]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[12].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[12].dstSet           = m_allGeneratedDS[4];
    writeDescriptorSet[12].dstBinding       = 12;
    writeDescriptorSet[12].descriptorCount  = 1;
    writeDescriptorSet[12].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[12].pBufferInfo      = &descriptorBufferInfo[12];
    writeDescriptorSet[12].pImageInfo       = nullptr;
    writeDescriptorSet[12].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
  // descriptor set #5: ResolveCTFCmd (["m_colorBuffer.data()","m_allCTFPoints","m_allCTFs","m_allChannelOffsets","m_allChannelRenderInfo","m_allCompressedChannels","m_allFloatChannels","m_allIntChannels","m_gBuffer","m_geomOffsets","m_indices","m_instanceTransformInvTransposed","m_lights","m_matIdOffsets","m_matIdbyPrimId","m_materials","m_normals","m_textures","m_vertices"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 19 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  19 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   19 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = m_vdata.m_colorBufferBuffer;
    descriptorBufferInfo[0].offset = m_vdata.m_colorBufferOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = m_vdata.m_allCTFPointsBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_allCTFPointsOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_vdata.m_allCTFsBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_allCTFsOffset;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    descriptorBufferInfo[3]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[3].buffer = m_vdata.m_allChannelOffsetsBuffer;
    descriptorBufferInfo[3].offset = m_vdata.m_allChannelOffsetsOffset;
    descriptorBufferInfo[3].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[3]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[3].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[3].dstBinding       = 3;
    writeDescriptorSet[3].descriptorCount  = 1;
    writeDescriptorSet[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[3].pBufferInfo      = &descriptorBufferInfo[3];
    writeDescriptorSet[3].pImageInfo       = nullptr;
    writeDescriptorSet[3].pTexelBufferView = nullptr;

    descriptorBufferInfo[4]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[4].buffer = m_vdata.m_allChannelRenderInfoBuffer;
    descriptorBufferInfo[4].offset = m_vdata.m_allChannelRenderInfoOffset;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    descriptorBufferInfo[5]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[5].buffer = m_vdata.m_allCompressedChannelsBuffer;
    descriptorBufferInfo[5].offset = m_vdata.m_allCompressedChannelsOffset;
    descriptorBufferInfo[5].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[5]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[5].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[5].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[5].dstBinding       = 5;
    writeDescriptorSet[5].descriptorCount  = 1;
    writeDescriptorSet[5].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[5].pBufferInfo      = &descriptorBufferInfo[5];
    writeDescriptorSet[5].pImageInfo       = nullptr;
    writeDescriptorSet[5].pTexelBufferView = nullptr;

    descriptorBufferInfo[6]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[6].buffer = m_vdata.m_allFloatChannelsBuffer;
    descriptorBufferInfo[6].offset = m_vdata.m_allFloatChannelsOffset;
    descriptorBufferInfo[6].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[6]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[6].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[6].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[6].dstBinding       = 6;
    writeDescriptorSet[6].descriptorCount  = 1;
    writeDescriptorSet[6].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[6].pBufferInfo      = &descriptorBufferInfo[6];
    writeDescriptorSet[6].pImageInfo       = nullptr;
    writeDescriptorSet[6].pTexelBufferView = nullptr;

    descriptorBufferInfo[7]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[7].buffer = m_vdata.m_allIntChannelsBuffer;
    descriptorBufferInfo[7].offset = m_vdata.m_allIntChannelsOffset;
    descriptorBufferInfo[7].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[7]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[7].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[7].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[7].dstBinding       = 7;
    writeDescriptorSet[7].descriptorCount  = 1;
    writeDescriptorSet[7].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[7].pBufferInfo      = &descriptorBufferInfo[7];
    writeDescriptorSet[7].pImageInfo       = nullptr;
    writeDescriptorSet[7].pTexelBufferView = nullptr;

    descriptorBufferInfo[8]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[8].buffer = m_vdata.m_gBufferBuffer;
    descriptorBufferInfo[8].offset = m_vdata.m_gBufferOffset;
    descriptorBufferInfo[8].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[8]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[8].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[8].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[8].dstBinding       = 8;
    writeDescriptorSet[8].descriptorCount  = 1;
    writeDescriptorSet[8].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[8].pBufferInfo      = &descriptorBufferInfo[8];
    writeDescriptorSet[8].pImageInfo       = nullptr;
    writeDescriptorSet[8].pTexelBufferView = nullptr;

    descriptorBufferInfo[9]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[9].buffer = m_vdata.m_geomOffsetsBuffer;
    descriptorBufferInfo[9].offset = m_vdata.m_geomOffsetsOffset;
    descriptorBufferInfo[9].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[9]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[9].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[9].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[9].dstBinding       = 9;
    writeDescriptorSet[9].descriptorCount  = 1;
    writeDescriptorSet[9].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[9].pBufferInfo      = &descriptorBufferInfo[9];
    writeDescriptorSet[9].pImageInfo       = nullptr;
    writeDescriptorSet[9].pTexelBufferView = nullptr;

    descriptorBufferInfo[10]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[10].buffer = m_vdata.m_indicesBuffer;
    descriptorBufferInfo[10].offset = m_vdata.m_indicesOffset;
    descriptorBufferInfo[10].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[10]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[10].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[10].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[10].dstBinding       = 10;
    writeDescriptorSet[10].descriptorCount  = 1;
    writeDescriptorSet[10].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[10].pBufferInfo      = &descriptorBufferInfo[10];
    writeDescriptorSet[10].pImageInfo       = nullptr;
    writeDescriptorSet[10].pTexelBufferView = nullptr;

    descriptorBufferInfo[11]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[11].buffer = m_vdata.m_instanceTransformInvTransposedBuffer;
    descriptorBufferInfo[11].offset = m_vdata.m_instanceTransformInvTransposedOffset;
    descriptorBufferInfo[11].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[11]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[11].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[11].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[11].dstBinding       = 11;
    writeDescriptorSet[11].descriptorCount  = 1;
    writeDescriptorSet[11].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[11].pBufferInfo      = &descriptorBufferInfo[11];
    writeDescriptorSet[11].pImageInfo       = nullptr;
    writeDescriptorSet[11].pTexelBufferView = nullptr;

    descriptorBufferInfo[12]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[12].buffer = m_vdata.m_lightsBuffer;
    descriptorBufferInfo[12].offset = m_vdata.m_lightsOffset;
    descriptorBufferInfo[12].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[12]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[12].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[12].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[12].dstBinding       = 12;
    writeDescriptorSet[12].descriptorCount  = 1;
    writeDescriptorSet[12].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[12].pBufferInfo      = &descriptorBufferInfo[12];
    writeDescriptorSet[12].pImageInfo       = nullptr;
    writeDescriptorSet[12].pTexelBufferView = nullptr;

    descriptorBufferInfo[13]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[13].buffer = m_vdata.m_matIdOffsetsBuffer;
    descriptorBufferInfo[13].offset = m_vdata.m_matIdOffsetsOffset;
    descriptorBufferInfo[13].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[13]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[13].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[13].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[13].dstBinding       = 13;
    writeDescriptorSet[13].descriptorCount  = 1;
    writeDescriptorSet[13].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[13].pBufferInfo      = &descriptorBufferInfo[13];
    writeDescriptorSet[13].pImageInfo       = nullptr;
    writeDescriptorSet[13].pTexelBufferView = nullptr;

    descriptorBufferInfo[14]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[14].buffer = m_vdata.m_matIdbyPrimIdBuffer;
    descriptorBufferInfo[14].offset = m_vdata.m_matIdbyPrimIdOffset;
    descriptorBufferInfo[14].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[14]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[14].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[14].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[14].dstBinding       = 14;
    writeDescriptorSet[14].descriptorCount  = 1;
    writeDescriptorSet[14].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[14].pBufferInfo      = &descriptorBufferInfo[14];
    writeDescriptorSet[14].pImageInfo       = nullptr;
    writeDescriptorSet[14].pTexelBufferView = nullptr;

    descriptorBufferInfo[15]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[15].buffer = m_vdata.m_materialsBuffer;
    descriptorBufferInfo[15].offset = m_vdata.m_materialsOffset;
    descriptorBufferInfo[15].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[15]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[15].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[15].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[15].dstBinding       = 15;
    writeDescriptorSet[15].descriptorCount  = 1;
    writeDescriptorSet[15].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[15].pBufferInfo      = &descriptorBufferInfo[15];
    writeDescriptorSet[15].pImageInfo       = nullptr;
    writeDescriptorSet[15].pTexelBufferView = nullptr;

    descriptorBufferInfo[16]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[16].buffer = m_vdata.m_normalsBuffer;
    descriptorBufferInfo[16].offset = m_vdata.m_normalsOffset;
    descriptorBufferInfo[16].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[16]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[16].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[16].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[16].dstBinding       = 16;
    writeDescriptorSet[16].descriptorCount  = 1;
    writeDescriptorSet[16].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[16].pBufferInfo      = &descriptorBufferInfo[16];
    writeDescriptorSet[16].pImageInfo       = nullptr;
    writeDescriptorSet[16].pTexelBufferView = nullptr;

    std::vector<VkDescriptorImageInfo> m_texturesInfo(m_vdata.m_texturesArrayMaxSize);
    for(size_t i=0; i<m_vdata.m_texturesArrayMaxSize; i++)
    {
      if(i < m_textures.size())
      {
        m_texturesInfo[i].sampler     = m_vdata.m_texturesArraySampler[i];
        m_texturesInfo[i].imageView   = m_vdata.m_texturesArrayView   [i];
        m_texturesInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
      else
      {
        m_texturesInfo[i].sampler     = m_vdata.m_texturesArraySampler[0];
        m_texturesInfo[i].imageView   = m_vdata.m_texturesArrayView   [0];
        m_texturesInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
    }
    writeDescriptorSet[17]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[17].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[17].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[17].dstBinding       = 17;
    writeDescriptorSet[17].descriptorCount  = 1;
    writeDescriptorSet[17].descriptorCount  = m_texturesInfo.size();
    writeDescriptorSet[17].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet[17].pBufferInfo      = nullptr;
    writeDescriptorSet[17].pImageInfo       = m_texturesInfo.data();
    writeDescriptorSet[17].pTexelBufferView = nullptr;

    descriptorBufferInfo[18]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[18].buffer = m_vdata.m_verticesBuffer;
    descriptorBufferInfo[18].offset = m_vdata.m_verticesOffset;
    descriptorBufferInfo[18].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[18]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[18].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[18].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[18].dstBinding       = 18;
    writeDescriptorSet[18].descriptorCount  = 1;
    writeDescriptorSet[18].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[18].pBufferInfo      = &descriptorBufferInfo[18];
    writeDescriptorSet[18].pImageInfo       = nullptr;
    writeDescriptorSet[18].pTexelBufferView = nullptr;

    descriptorBufferInfo[19]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[19].buffer = m_classDataBuffer;
    descriptorBufferInfo[19].offset = 0;
    descriptorBufferInfo[19].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[19]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[19].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[19].dstSet           = m_allGeneratedDS[5];
    writeDescriptorSet[19].dstBinding       = 19;
    writeDescriptorSet[19].descriptorCount  = 1;
    writeDescriptorSet[19].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[19].pBufferInfo      = &descriptorBufferInfo[19];
    writeDescriptorSet[19].pImageInfo       = nullptr;
    writeDescriptorSet[19].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
  // descriptor set #6: ResolveOutlineCmd (["m_colorBuffer.data()","m_gBuffer","m_geomOffsets","m_indices","m_instanceTransformInvTransposed","m_lights","m_normals","m_vertices"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 8 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  8 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   8 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = m_vdata.m_colorBufferBuffer;
    descriptorBufferInfo[0].offset = m_vdata.m_colorBufferOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[6];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = m_vdata.m_gBufferBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_gBufferOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[6];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_vdata.m_geomOffsetsBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_geomOffsetsOffset;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[6];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    descriptorBufferInfo[3]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[3].buffer = m_vdata.m_indicesBuffer;
    descriptorBufferInfo[3].offset = m_vdata.m_indicesOffset;
    descriptorBufferInfo[3].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[3]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[3].dstSet           = m_allGeneratedDS[6];
    writeDescriptorSet[3].dstBinding       = 3;
    writeDescriptorSet[3].descriptorCount  = 1;
    writeDescriptorSet[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[3].pBufferInfo      = &descriptorBufferInfo[3];
    writeDescriptorSet[3].pImageInfo       = nullptr;
    writeDescriptorSet[3].pTexelBufferView = nullptr;

    descriptorBufferInfo[4]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[4].buffer = m_vdata.m_instanceTransformInvTransposedBuffer;
    descriptorBufferInfo[4].offset = m_vdata.m_instanceTransformInvTransposedOffset;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[6];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    descriptorBufferInfo[5]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[5].buffer = m_vdata.m_lightsBuffer;
    descriptorBufferInfo[5].offset = m_vdata.m_lightsOffset;
    descriptorBufferInfo[5].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[5]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[5].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[5].dstSet           = m_allGeneratedDS[6];
    writeDescriptorSet[5].dstBinding       = 5;
    writeDescriptorSet[5].descriptorCount  = 1;
    writeDescriptorSet[5].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[5].pBufferInfo      = &descriptorBufferInfo[5];
    writeDescriptorSet[5].pImageInfo       = nullptr;
    writeDescriptorSet[5].pTexelBufferView = nullptr;

    descriptorBufferInfo[6]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[6].buffer = m_vdata.m_normalsBuffer;
    descriptorBufferInfo[6].offset = m_vdata.m_normalsOffset;
    descriptorBufferInfo[6].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[6]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[6].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[6].dstSet           = m_allGeneratedDS[6];
    writeDescriptorSet[6].dstBinding       = 6;
    writeDescriptorSet[6].descriptorCount  = 1;
    writeDescriptorSet[6].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[6].pBufferInfo      = &descriptorBufferInfo[6];
    writeDescriptorSet[6].pImageInfo       = nullptr;
    writeDescriptorSet[6].pTexelBufferView = nullptr;

    descriptorBufferInfo[7]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[7].buffer = m_vdata.m_verticesBuffer;
    descriptorBufferInfo[7].offset = m_vdata.m_verticesOffset;
    descriptorBufferInfo[7].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[7]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[7].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[7].dstSet           = m_allGeneratedDS[6];
    writeDescriptorSet[7].dstBinding       = 7;
    writeDescriptorSet[7].descriptorCount  = 1;
    writeDescriptorSet[7].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[7].pBufferInfo      = &descriptorBufferInfo[7];
    writeDescriptorSet[7].pImageInfo       = nullptr;
    writeDescriptorSet[7].pTexelBufferView = nullptr;

    descriptorBufferInfo[8]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[8].buffer = m_classDataBuffer;
    descriptorBufferInfo[8].offset = 0;
    descriptorBufferInfo[8].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[8]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[8].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[8].dstSet           = m_allGeneratedDS[6];
    writeDescriptorSet[8].dstBinding       = 8;
    writeDescriptorSet[8].descriptorCount  = 1;
    writeDescriptorSet[8].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[8].pBufferInfo      = &descriptorBufferInfo[8];
    writeDescriptorSet[8].pImageInfo       = nullptr;
    writeDescriptorSet[8].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
  // descriptor set #7: ResolveDebugCmd (["m_colorBuffer.data()","m_gBuffer","m_geomOffsets","m_indices","m_instanceTransformInvTransposed","m_matIdOffsets","m_matIdbyPrimId","m_normals","m_vertices"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 9 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  9 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   9 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = m_vdata.m_colorBufferBuffer;
    descriptorBufferInfo[0].offset = m_vdata.m_colorBufferOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[7];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = m_vdata.m_gBufferBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_gBufferOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[7];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_vdata.m_geomOffsetsBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_geomOffsetsOffset;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[7];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    descriptorBufferInfo[3]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[3].buffer = m_vdata.m_indicesBuffer;
    descriptorBufferInfo[3].offset = m_vdata.m_indicesOffset;
    descriptorBufferInfo[3].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[3]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[3].dstSet           = m_allGeneratedDS[7];
    writeDescriptorSet[3].dstBinding       = 3;
    writeDescriptorSet[3].descriptorCount  = 1;
    writeDescriptorSet[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[3].pBufferInfo      = &descriptorBufferInfo[3];
    writeDescriptorSet[3].pImageInfo       = nullptr;
    writeDescriptorSet[3].pTexelBufferView = nullptr;

    descriptorBufferInfo[4]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[4].buffer = m_vdata.m_instanceTransformInvTransposedBuffer;
    descriptorBufferInfo[4].offset = m_vdata.m_instanceTransformInvTransposedOffset;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[7];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    descriptorBufferInfo[5]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[5].buffer = m_vdata.m_matIdOffsetsBuffer;
    descriptorBufferInfo[5].offset = m_vdata.m_matIdOffsetsOffset;
    descriptorBufferInfo[5].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[5]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[5].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[5].dstSet           = m_allGeneratedDS[7];
    writeDescriptorSet[5].dstBinding       = 5;
    writeDescriptorSet[5].descriptorCount  = 1;
    writeDescriptorSet[5].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[5].pBufferInfo      = &descriptorBufferInfo[5];
    writeDescriptorSet[5].pImageInfo       = nullptr;
    writeDescriptorSet[5].pTexelBufferView = nullptr;

    descriptorBufferInfo[6]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[6].buffer = m_vdata.m_matIdbyPrimIdBuffer;
    descriptorBufferInfo[6].offset = m_vdata.m_matIdbyPrimIdOffset;
    descriptorBufferInfo[6].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[6]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[6].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[6].dstSet           = m_allGeneratedDS[7];
    writeDescriptorSet[6].dstBinding       = 6;
    writeDescriptorSet[6].descriptorCount  = 1;
    writeDescriptorSet[6].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[6].pBufferInfo      = &descriptorBufferInfo[6];
    writeDescriptorSet[6].pImageInfo       = nullptr;
    writeDescriptorSet[6].pTexelBufferView = nullptr;

    descriptorBufferInfo[7]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[7].buffer = m_vdata.m_normalsBuffer;
    descriptorBufferInfo[7].offset = m_vdata.m_normalsOffset;
    descriptorBufferInfo[7].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[7]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[7].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[7].dstSet           = m_allGeneratedDS[7];
    writeDescriptorSet[7].dstBinding       = 7;
    writeDescriptorSet[7].descriptorCount  = 1;
    writeDescriptorSet[7].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[7].pBufferInfo      = &descriptorBufferInfo[7];
    writeDescriptorSet[7].pImageInfo       = nullptr;
    writeDescriptorSet[7].pTexelBufferView = nullptr;

    descriptorBufferInfo[8]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[8].buffer = m_vdata.m_verticesBuffer;
    descriptorBufferInfo[8].offset = m_vdata.m_verticesOffset;
    descriptorBufferInfo[8].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[8]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[8].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[8].dstSet           = m_allGeneratedDS[7];
    writeDescriptorSet[8].dstBinding       = 8;
    writeDescriptorSet[8].descriptorCount  = 1;
    writeDescriptorSet[8].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[8].pBufferInfo      = &descriptorBufferInfo[8];
    writeDescriptorSet[8].pImageInfo       = nullptr;
    writeDescriptorSet[8].pTexelBufferView = nullptr;

    descriptorBufferInfo[9]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[9].buffer = m_classDataBuffer;
    descriptorBufferInfo[9].offset = 0;
    descriptorBufferInfo[9].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[9]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[9].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[9].dstSet           = m_allGeneratedDS[7];
    writeDescriptorSet[9].dstBinding       = 9;
    writeDescriptorSet[9].descriptorCount  = 1;
    writeDescriptorSet[9].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[9].pBufferInfo      = &descriptorBufferInfo[9];
    writeDescriptorSet[9].pImageInfo       = nullptr;
    writeDescriptorSet[9].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
  // descriptor set #8: AverageColorCmd (["a_outColor","m_colorBuffer"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 2 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  2 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   2 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = RenderFloat_local.a_outColorBuffer;
    descriptorBufferInfo[0].offset = RenderFloat_local.a_outColorOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[8];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = m_vdata.m_colorBufferBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_colorBufferOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[8];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_classDataBuffer;
    descriptorBufferInfo[2].offset = 0;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[8];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
}

void MultiRenderer_slang_comp::UpdateAllGeneratedDescriptorSets_Render()
{
  // now create actual bindings
  //
  // descriptor set #9: FillGbufferWithTransparencyCmd (["m_gBuffer","m_matIdOffsets","m_matIdbyPrimId","m_materials","m_pAccelStruct_m_GraphicsPrimHeaders","m_pAccelStruct_m_GraphicsPrimPoints","m_pAccelStruct_m_RotAddTable","m_pAccelStruct_m_SCom2Data","m_pAccelStruct_m_SCom2Headers","m_pAccelStruct_m_SdfCompactOctreeRotModifiers","m_pAccelStruct_m_SdfCompactOctreeRotTransforms","m_pAccelStruct_m_SdfCompactOctreeV3Data","m_pAccelStruct_m_SdfDAGChildEdges","m_pAccelStruct_m_SdfDAGDataEdges","m_pAccelStruct_m_SdfDAGDistances","m_pAccelStruct_m_SdfDAGHeaders","m_pAccelStruct_m_SdfDAGNodes","m_pAccelStruct_m_SdfDAGTranspositions","m_pAccelStruct_m_SdfFrameOctreeNodes","m_pAccelStruct_m_SdfFrameOctreeRoots","m_pAccelStruct_m_SdfFrameOctreeTexNodes","m_pAccelStruct_m_SdfFrameOctreeTexRoots","m_pAccelStruct_m_SdfGridData","m_pAccelStruct_m_SdfGridOffsets","m_pAccelStruct_m_SdfGridSizes","m_pAccelStruct_m_SdfMultiOctreeNodes","m_pAccelStruct_m_SdfMultiOctreeValues","m_pAccelStruct_m_SdfSBSData","m_pAccelStruct_m_SdfSBSDataF","m_pAccelStruct_m_SdfSBSHeaders","m_pAccelStruct_m_SdfSBSNodes","m_pAccelStruct_m_SdfSBSRoots","m_pAccelStruct_m_SdfSVSNodes","m_pAccelStruct_m_SdfSVSRoots","m_pAccelStruct_m_allNodePairs","m_pAccelStruct_m_geomData","m_pAccelStruct_m_geomTags","m_pAccelStruct_m_indices","m_pAccelStruct_m_instanceData","m_pAccelStruct_m_nodesTLAS","m_pAccelStruct_m_origNodes","m_pAccelStruct_m_primIdCount","m_pAccelStruct_m_primIndices","m_pAccelStruct_m_vertNorm","m_pAccelStruct_m_vertPos","m_pAccelStruct_startEnd","m_packedXY","m_transparencyBuffer"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 48 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  48 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   48 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = m_vdata.m_gBufferBuffer;
    descriptorBufferInfo[0].offset = m_vdata.m_gBufferOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = m_vdata.m_matIdOffsetsBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_matIdOffsetsOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_vdata.m_matIdbyPrimIdBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_matIdbyPrimIdOffset;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    descriptorBufferInfo[3]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[3].buffer = m_vdata.m_materialsBuffer;
    descriptorBufferInfo[3].offset = m_vdata.m_materialsOffset;
    descriptorBufferInfo[3].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[3]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[3].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[3].dstBinding       = 3;
    writeDescriptorSet[3].descriptorCount  = 1;
    writeDescriptorSet[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[3].pBufferInfo      = &descriptorBufferInfo[3];
    writeDescriptorSet[3].pImageInfo       = nullptr;
    writeDescriptorSet[3].pTexelBufferView = nullptr;

    descriptorBufferInfo[4]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[4].buffer = m_vdata.m_pAccelStruct_m_GraphicsPrimHeadersBuffer;
    descriptorBufferInfo[4].offset = m_vdata.m_pAccelStruct_m_GraphicsPrimHeadersOffset;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    descriptorBufferInfo[5]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[5].buffer = m_vdata.m_pAccelStruct_m_GraphicsPrimPointsBuffer;
    descriptorBufferInfo[5].offset = m_vdata.m_pAccelStruct_m_GraphicsPrimPointsOffset;
    descriptorBufferInfo[5].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[5]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[5].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[5].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[5].dstBinding       = 5;
    writeDescriptorSet[5].descriptorCount  = 1;
    writeDescriptorSet[5].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[5].pBufferInfo      = &descriptorBufferInfo[5];
    writeDescriptorSet[5].pImageInfo       = nullptr;
    writeDescriptorSet[5].pTexelBufferView = nullptr;

    descriptorBufferInfo[6]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[6].buffer = m_vdata.m_pAccelStruct_m_RotAddTableBuffer;
    descriptorBufferInfo[6].offset = m_vdata.m_pAccelStruct_m_RotAddTableOffset;
    descriptorBufferInfo[6].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[6]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[6].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[6].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[6].dstBinding       = 6;
    writeDescriptorSet[6].descriptorCount  = 1;
    writeDescriptorSet[6].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[6].pBufferInfo      = &descriptorBufferInfo[6];
    writeDescriptorSet[6].pImageInfo       = nullptr;
    writeDescriptorSet[6].pTexelBufferView = nullptr;

    descriptorBufferInfo[7]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[7].buffer = m_vdata.m_pAccelStruct_m_SCom2DataBuffer;
    descriptorBufferInfo[7].offset = m_vdata.m_pAccelStruct_m_SCom2DataOffset;
    descriptorBufferInfo[7].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[7]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[7].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[7].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[7].dstBinding       = 7;
    writeDescriptorSet[7].descriptorCount  = 1;
    writeDescriptorSet[7].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[7].pBufferInfo      = &descriptorBufferInfo[7];
    writeDescriptorSet[7].pImageInfo       = nullptr;
    writeDescriptorSet[7].pTexelBufferView = nullptr;

    descriptorBufferInfo[8]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[8].buffer = m_vdata.m_pAccelStruct_m_SCom2HeadersBuffer;
    descriptorBufferInfo[8].offset = m_vdata.m_pAccelStruct_m_SCom2HeadersOffset;
    descriptorBufferInfo[8].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[8]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[8].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[8].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[8].dstBinding       = 8;
    writeDescriptorSet[8].descriptorCount  = 1;
    writeDescriptorSet[8].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[8].pBufferInfo      = &descriptorBufferInfo[8];
    writeDescriptorSet[8].pImageInfo       = nullptr;
    writeDescriptorSet[8].pTexelBufferView = nullptr;

    descriptorBufferInfo[9]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[9].buffer = m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotModifiersBuffer;
    descriptorBufferInfo[9].offset = m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotModifiersOffset;
    descriptorBufferInfo[9].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[9]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[9].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[9].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[9].dstBinding       = 9;
    writeDescriptorSet[9].descriptorCount  = 1;
    writeDescriptorSet[9].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[9].pBufferInfo      = &descriptorBufferInfo[9];
    writeDescriptorSet[9].pImageInfo       = nullptr;
    writeDescriptorSet[9].pTexelBufferView = nullptr;

    descriptorBufferInfo[10]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[10].buffer = m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotTransformsBuffer;
    descriptorBufferInfo[10].offset = m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotTransformsOffset;
    descriptorBufferInfo[10].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[10]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[10].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[10].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[10].dstBinding       = 10;
    writeDescriptorSet[10].descriptorCount  = 1;
    writeDescriptorSet[10].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[10].pBufferInfo      = &descriptorBufferInfo[10];
    writeDescriptorSet[10].pImageInfo       = nullptr;
    writeDescriptorSet[10].pTexelBufferView = nullptr;

    descriptorBufferInfo[11]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[11].buffer = m_vdata.m_pAccelStruct_m_SdfCompactOctreeV3DataBuffer;
    descriptorBufferInfo[11].offset = m_vdata.m_pAccelStruct_m_SdfCompactOctreeV3DataOffset;
    descriptorBufferInfo[11].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[11]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[11].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[11].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[11].dstBinding       = 11;
    writeDescriptorSet[11].descriptorCount  = 1;
    writeDescriptorSet[11].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[11].pBufferInfo      = &descriptorBufferInfo[11];
    writeDescriptorSet[11].pImageInfo       = nullptr;
    writeDescriptorSet[11].pTexelBufferView = nullptr;

    descriptorBufferInfo[12]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[12].buffer = m_vdata.m_pAccelStruct_m_SdfDAGChildEdgesBuffer;
    descriptorBufferInfo[12].offset = m_vdata.m_pAccelStruct_m_SdfDAGChildEdgesOffset;
    descriptorBufferInfo[12].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[12]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[12].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[12].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[12].dstBinding       = 12;
    writeDescriptorSet[12].descriptorCount  = 1;
    writeDescriptorSet[12].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[12].pBufferInfo      = &descriptorBufferInfo[12];
    writeDescriptorSet[12].pImageInfo       = nullptr;
    writeDescriptorSet[12].pTexelBufferView = nullptr;

    descriptorBufferInfo[13]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[13].buffer = m_vdata.m_pAccelStruct_m_SdfDAGDataEdgesBuffer;
    descriptorBufferInfo[13].offset = m_vdata.m_pAccelStruct_m_SdfDAGDataEdgesOffset;
    descriptorBufferInfo[13].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[13]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[13].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[13].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[13].dstBinding       = 13;
    writeDescriptorSet[13].descriptorCount  = 1;
    writeDescriptorSet[13].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[13].pBufferInfo      = &descriptorBufferInfo[13];
    writeDescriptorSet[13].pImageInfo       = nullptr;
    writeDescriptorSet[13].pTexelBufferView = nullptr;

    descriptorBufferInfo[14]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[14].buffer = m_vdata.m_pAccelStruct_m_SdfDAGDistancesBuffer;
    descriptorBufferInfo[14].offset = m_vdata.m_pAccelStruct_m_SdfDAGDistancesOffset;
    descriptorBufferInfo[14].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[14]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[14].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[14].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[14].dstBinding       = 14;
    writeDescriptorSet[14].descriptorCount  = 1;
    writeDescriptorSet[14].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[14].pBufferInfo      = &descriptorBufferInfo[14];
    writeDescriptorSet[14].pImageInfo       = nullptr;
    writeDescriptorSet[14].pTexelBufferView = nullptr;

    descriptorBufferInfo[15]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[15].buffer = m_vdata.m_pAccelStruct_m_SdfDAGHeadersBuffer;
    descriptorBufferInfo[15].offset = m_vdata.m_pAccelStruct_m_SdfDAGHeadersOffset;
    descriptorBufferInfo[15].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[15]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[15].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[15].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[15].dstBinding       = 15;
    writeDescriptorSet[15].descriptorCount  = 1;
    writeDescriptorSet[15].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[15].pBufferInfo      = &descriptorBufferInfo[15];
    writeDescriptorSet[15].pImageInfo       = nullptr;
    writeDescriptorSet[15].pTexelBufferView = nullptr;

    descriptorBufferInfo[16]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[16].buffer = m_vdata.m_pAccelStruct_m_SdfDAGNodesBuffer;
    descriptorBufferInfo[16].offset = m_vdata.m_pAccelStruct_m_SdfDAGNodesOffset;
    descriptorBufferInfo[16].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[16]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[16].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[16].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[16].dstBinding       = 16;
    writeDescriptorSet[16].descriptorCount  = 1;
    writeDescriptorSet[16].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[16].pBufferInfo      = &descriptorBufferInfo[16];
    writeDescriptorSet[16].pImageInfo       = nullptr;
    writeDescriptorSet[16].pTexelBufferView = nullptr;

    descriptorBufferInfo[17]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[17].buffer = m_vdata.m_pAccelStruct_m_SdfDAGTranspositionsBuffer;
    descriptorBufferInfo[17].offset = m_vdata.m_pAccelStruct_m_SdfDAGTranspositionsOffset;
    descriptorBufferInfo[17].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[17]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[17].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[17].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[17].dstBinding       = 17;
    writeDescriptorSet[17].descriptorCount  = 1;
    writeDescriptorSet[17].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[17].pBufferInfo      = &descriptorBufferInfo[17];
    writeDescriptorSet[17].pImageInfo       = nullptr;
    writeDescriptorSet[17].pTexelBufferView = nullptr;

    descriptorBufferInfo[18]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[18].buffer = m_vdata.m_pAccelStruct_m_SdfFrameOctreeNodesBuffer;
    descriptorBufferInfo[18].offset = m_vdata.m_pAccelStruct_m_SdfFrameOctreeNodesOffset;
    descriptorBufferInfo[18].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[18]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[18].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[18].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[18].dstBinding       = 18;
    writeDescriptorSet[18].descriptorCount  = 1;
    writeDescriptorSet[18].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[18].pBufferInfo      = &descriptorBufferInfo[18];
    writeDescriptorSet[18].pImageInfo       = nullptr;
    writeDescriptorSet[18].pTexelBufferView = nullptr;

    descriptorBufferInfo[19]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[19].buffer = m_vdata.m_pAccelStruct_m_SdfFrameOctreeRootsBuffer;
    descriptorBufferInfo[19].offset = m_vdata.m_pAccelStruct_m_SdfFrameOctreeRootsOffset;
    descriptorBufferInfo[19].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[19]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[19].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[19].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[19].dstBinding       = 19;
    writeDescriptorSet[19].descriptorCount  = 1;
    writeDescriptorSet[19].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[19].pBufferInfo      = &descriptorBufferInfo[19];
    writeDescriptorSet[19].pImageInfo       = nullptr;
    writeDescriptorSet[19].pTexelBufferView = nullptr;

    descriptorBufferInfo[20]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[20].buffer = m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexNodesBuffer;
    descriptorBufferInfo[20].offset = m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexNodesOffset;
    descriptorBufferInfo[20].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[20]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[20].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[20].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[20].dstBinding       = 20;
    writeDescriptorSet[20].descriptorCount  = 1;
    writeDescriptorSet[20].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[20].pBufferInfo      = &descriptorBufferInfo[20];
    writeDescriptorSet[20].pImageInfo       = nullptr;
    writeDescriptorSet[20].pTexelBufferView = nullptr;

    descriptorBufferInfo[21]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[21].buffer = m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexRootsBuffer;
    descriptorBufferInfo[21].offset = m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexRootsOffset;
    descriptorBufferInfo[21].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[21]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[21].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[21].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[21].dstBinding       = 21;
    writeDescriptorSet[21].descriptorCount  = 1;
    writeDescriptorSet[21].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[21].pBufferInfo      = &descriptorBufferInfo[21];
    writeDescriptorSet[21].pImageInfo       = nullptr;
    writeDescriptorSet[21].pTexelBufferView = nullptr;

    descriptorBufferInfo[22]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[22].buffer = m_vdata.m_pAccelStruct_m_SdfGridDataBuffer;
    descriptorBufferInfo[22].offset = m_vdata.m_pAccelStruct_m_SdfGridDataOffset;
    descriptorBufferInfo[22].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[22]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[22].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[22].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[22].dstBinding       = 22;
    writeDescriptorSet[22].descriptorCount  = 1;
    writeDescriptorSet[22].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[22].pBufferInfo      = &descriptorBufferInfo[22];
    writeDescriptorSet[22].pImageInfo       = nullptr;
    writeDescriptorSet[22].pTexelBufferView = nullptr;

    descriptorBufferInfo[23]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[23].buffer = m_vdata.m_pAccelStruct_m_SdfGridOffsetsBuffer;
    descriptorBufferInfo[23].offset = m_vdata.m_pAccelStruct_m_SdfGridOffsetsOffset;
    descriptorBufferInfo[23].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[23]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[23].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[23].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[23].dstBinding       = 23;
    writeDescriptorSet[23].descriptorCount  = 1;
    writeDescriptorSet[23].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[23].pBufferInfo      = &descriptorBufferInfo[23];
    writeDescriptorSet[23].pImageInfo       = nullptr;
    writeDescriptorSet[23].pTexelBufferView = nullptr;

    descriptorBufferInfo[24]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[24].buffer = m_vdata.m_pAccelStruct_m_SdfGridSizesBuffer;
    descriptorBufferInfo[24].offset = m_vdata.m_pAccelStruct_m_SdfGridSizesOffset;
    descriptorBufferInfo[24].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[24]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[24].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[24].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[24].dstBinding       = 24;
    writeDescriptorSet[24].descriptorCount  = 1;
    writeDescriptorSet[24].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[24].pBufferInfo      = &descriptorBufferInfo[24];
    writeDescriptorSet[24].pImageInfo       = nullptr;
    writeDescriptorSet[24].pTexelBufferView = nullptr;

    descriptorBufferInfo[25]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[25].buffer = m_vdata.m_pAccelStruct_m_SdfMultiOctreeNodesBuffer;
    descriptorBufferInfo[25].offset = m_vdata.m_pAccelStruct_m_SdfMultiOctreeNodesOffset;
    descriptorBufferInfo[25].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[25]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[25].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[25].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[25].dstBinding       = 25;
    writeDescriptorSet[25].descriptorCount  = 1;
    writeDescriptorSet[25].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[25].pBufferInfo      = &descriptorBufferInfo[25];
    writeDescriptorSet[25].pImageInfo       = nullptr;
    writeDescriptorSet[25].pTexelBufferView = nullptr;

    descriptorBufferInfo[26]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[26].buffer = m_vdata.m_pAccelStruct_m_SdfMultiOctreeValuesBuffer;
    descriptorBufferInfo[26].offset = m_vdata.m_pAccelStruct_m_SdfMultiOctreeValuesOffset;
    descriptorBufferInfo[26].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[26]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[26].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[26].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[26].dstBinding       = 26;
    writeDescriptorSet[26].descriptorCount  = 1;
    writeDescriptorSet[26].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[26].pBufferInfo      = &descriptorBufferInfo[26];
    writeDescriptorSet[26].pImageInfo       = nullptr;
    writeDescriptorSet[26].pTexelBufferView = nullptr;

    descriptorBufferInfo[27]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[27].buffer = m_vdata.m_pAccelStruct_m_SdfSBSDataBuffer;
    descriptorBufferInfo[27].offset = m_vdata.m_pAccelStruct_m_SdfSBSDataOffset;
    descriptorBufferInfo[27].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[27]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[27].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[27].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[27].dstBinding       = 27;
    writeDescriptorSet[27].descriptorCount  = 1;
    writeDescriptorSet[27].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[27].pBufferInfo      = &descriptorBufferInfo[27];
    writeDescriptorSet[27].pImageInfo       = nullptr;
    writeDescriptorSet[27].pTexelBufferView = nullptr;

    descriptorBufferInfo[28]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[28].buffer = m_vdata.m_pAccelStruct_m_SdfSBSDataFBuffer;
    descriptorBufferInfo[28].offset = m_vdata.m_pAccelStruct_m_SdfSBSDataFOffset;
    descriptorBufferInfo[28].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[28]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[28].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[28].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[28].dstBinding       = 28;
    writeDescriptorSet[28].descriptorCount  = 1;
    writeDescriptorSet[28].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[28].pBufferInfo      = &descriptorBufferInfo[28];
    writeDescriptorSet[28].pImageInfo       = nullptr;
    writeDescriptorSet[28].pTexelBufferView = nullptr;

    descriptorBufferInfo[29]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[29].buffer = m_vdata.m_pAccelStruct_m_SdfSBSHeadersBuffer;
    descriptorBufferInfo[29].offset = m_vdata.m_pAccelStruct_m_SdfSBSHeadersOffset;
    descriptorBufferInfo[29].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[29]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[29].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[29].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[29].dstBinding       = 29;
    writeDescriptorSet[29].descriptorCount  = 1;
    writeDescriptorSet[29].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[29].pBufferInfo      = &descriptorBufferInfo[29];
    writeDescriptorSet[29].pImageInfo       = nullptr;
    writeDescriptorSet[29].pTexelBufferView = nullptr;

    descriptorBufferInfo[30]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[30].buffer = m_vdata.m_pAccelStruct_m_SdfSBSNodesBuffer;
    descriptorBufferInfo[30].offset = m_vdata.m_pAccelStruct_m_SdfSBSNodesOffset;
    descriptorBufferInfo[30].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[30]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[30].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[30].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[30].dstBinding       = 30;
    writeDescriptorSet[30].descriptorCount  = 1;
    writeDescriptorSet[30].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[30].pBufferInfo      = &descriptorBufferInfo[30];
    writeDescriptorSet[30].pImageInfo       = nullptr;
    writeDescriptorSet[30].pTexelBufferView = nullptr;

    descriptorBufferInfo[31]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[31].buffer = m_vdata.m_pAccelStruct_m_SdfSBSRootsBuffer;
    descriptorBufferInfo[31].offset = m_vdata.m_pAccelStruct_m_SdfSBSRootsOffset;
    descriptorBufferInfo[31].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[31]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[31].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[31].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[31].dstBinding       = 31;
    writeDescriptorSet[31].descriptorCount  = 1;
    writeDescriptorSet[31].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[31].pBufferInfo      = &descriptorBufferInfo[31];
    writeDescriptorSet[31].pImageInfo       = nullptr;
    writeDescriptorSet[31].pTexelBufferView = nullptr;

    descriptorBufferInfo[32]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[32].buffer = m_vdata.m_pAccelStruct_m_SdfSVSNodesBuffer;
    descriptorBufferInfo[32].offset = m_vdata.m_pAccelStruct_m_SdfSVSNodesOffset;
    descriptorBufferInfo[32].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[32]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[32].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[32].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[32].dstBinding       = 32;
    writeDescriptorSet[32].descriptorCount  = 1;
    writeDescriptorSet[32].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[32].pBufferInfo      = &descriptorBufferInfo[32];
    writeDescriptorSet[32].pImageInfo       = nullptr;
    writeDescriptorSet[32].pTexelBufferView = nullptr;

    descriptorBufferInfo[33]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[33].buffer = m_vdata.m_pAccelStruct_m_SdfSVSRootsBuffer;
    descriptorBufferInfo[33].offset = m_vdata.m_pAccelStruct_m_SdfSVSRootsOffset;
    descriptorBufferInfo[33].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[33]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[33].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[33].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[33].dstBinding       = 33;
    writeDescriptorSet[33].descriptorCount  = 1;
    writeDescriptorSet[33].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[33].pBufferInfo      = &descriptorBufferInfo[33];
    writeDescriptorSet[33].pImageInfo       = nullptr;
    writeDescriptorSet[33].pTexelBufferView = nullptr;

    descriptorBufferInfo[34]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[34].buffer = m_vdata.m_pAccelStruct_m_allNodePairsBuffer;
    descriptorBufferInfo[34].offset = m_vdata.m_pAccelStruct_m_allNodePairsOffset;
    descriptorBufferInfo[34].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[34]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[34].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[34].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[34].dstBinding       = 34;
    writeDescriptorSet[34].descriptorCount  = 1;
    writeDescriptorSet[34].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[34].pBufferInfo      = &descriptorBufferInfo[34];
    writeDescriptorSet[34].pImageInfo       = nullptr;
    writeDescriptorSet[34].pTexelBufferView = nullptr;

    descriptorBufferInfo[35]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[35].buffer = m_vdata.m_pAccelStruct_m_geomDataBuffer;
    descriptorBufferInfo[35].offset = m_vdata.m_pAccelStruct_m_geomDataOffset;
    descriptorBufferInfo[35].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[35]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[35].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[35].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[35].dstBinding       = 35;
    writeDescriptorSet[35].descriptorCount  = 1;
    writeDescriptorSet[35].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[35].pBufferInfo      = &descriptorBufferInfo[35];
    writeDescriptorSet[35].pImageInfo       = nullptr;
    writeDescriptorSet[35].pTexelBufferView = nullptr;

    descriptorBufferInfo[36]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[36].buffer = m_vdata.m_pAccelStruct_m_geomTagsBuffer;
    descriptorBufferInfo[36].offset = m_vdata.m_pAccelStruct_m_geomTagsOffset;
    descriptorBufferInfo[36].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[36]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[36].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[36].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[36].dstBinding       = 36;
    writeDescriptorSet[36].descriptorCount  = 1;
    writeDescriptorSet[36].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[36].pBufferInfo      = &descriptorBufferInfo[36];
    writeDescriptorSet[36].pImageInfo       = nullptr;
    writeDescriptorSet[36].pTexelBufferView = nullptr;

    descriptorBufferInfo[37]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[37].buffer = m_vdata.m_pAccelStruct_m_indicesBuffer;
    descriptorBufferInfo[37].offset = m_vdata.m_pAccelStruct_m_indicesOffset;
    descriptorBufferInfo[37].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[37]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[37].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[37].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[37].dstBinding       = 37;
    writeDescriptorSet[37].descriptorCount  = 1;
    writeDescriptorSet[37].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[37].pBufferInfo      = &descriptorBufferInfo[37];
    writeDescriptorSet[37].pImageInfo       = nullptr;
    writeDescriptorSet[37].pTexelBufferView = nullptr;

    descriptorBufferInfo[38]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[38].buffer = m_vdata.m_pAccelStruct_m_instanceDataBuffer;
    descriptorBufferInfo[38].offset = m_vdata.m_pAccelStruct_m_instanceDataOffset;
    descriptorBufferInfo[38].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[38]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[38].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[38].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[38].dstBinding       = 38;
    writeDescriptorSet[38].descriptorCount  = 1;
    writeDescriptorSet[38].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[38].pBufferInfo      = &descriptorBufferInfo[38];
    writeDescriptorSet[38].pImageInfo       = nullptr;
    writeDescriptorSet[38].pTexelBufferView = nullptr;

    descriptorBufferInfo[39]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[39].buffer = m_vdata.m_pAccelStruct_m_nodesTLASBuffer;
    descriptorBufferInfo[39].offset = m_vdata.m_pAccelStruct_m_nodesTLASOffset;
    descriptorBufferInfo[39].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[39]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[39].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[39].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[39].dstBinding       = 39;
    writeDescriptorSet[39].descriptorCount  = 1;
    writeDescriptorSet[39].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[39].pBufferInfo      = &descriptorBufferInfo[39];
    writeDescriptorSet[39].pImageInfo       = nullptr;
    writeDescriptorSet[39].pTexelBufferView = nullptr;

    descriptorBufferInfo[40]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[40].buffer = m_vdata.m_pAccelStruct_m_origNodesBuffer;
    descriptorBufferInfo[40].offset = m_vdata.m_pAccelStruct_m_origNodesOffset;
    descriptorBufferInfo[40].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[40]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[40].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[40].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[40].dstBinding       = 40;
    writeDescriptorSet[40].descriptorCount  = 1;
    writeDescriptorSet[40].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[40].pBufferInfo      = &descriptorBufferInfo[40];
    writeDescriptorSet[40].pImageInfo       = nullptr;
    writeDescriptorSet[40].pTexelBufferView = nullptr;

    descriptorBufferInfo[41]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[41].buffer = m_vdata.m_pAccelStruct_m_primIdCountBuffer;
    descriptorBufferInfo[41].offset = m_vdata.m_pAccelStruct_m_primIdCountOffset;
    descriptorBufferInfo[41].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[41]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[41].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[41].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[41].dstBinding       = 41;
    writeDescriptorSet[41].descriptorCount  = 1;
    writeDescriptorSet[41].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[41].pBufferInfo      = &descriptorBufferInfo[41];
    writeDescriptorSet[41].pImageInfo       = nullptr;
    writeDescriptorSet[41].pTexelBufferView = nullptr;

    descriptorBufferInfo[42]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[42].buffer = m_vdata.m_pAccelStruct_m_primIndicesBuffer;
    descriptorBufferInfo[42].offset = m_vdata.m_pAccelStruct_m_primIndicesOffset;
    descriptorBufferInfo[42].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[42]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[42].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[42].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[42].dstBinding       = 42;
    writeDescriptorSet[42].descriptorCount  = 1;
    writeDescriptorSet[42].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[42].pBufferInfo      = &descriptorBufferInfo[42];
    writeDescriptorSet[42].pImageInfo       = nullptr;
    writeDescriptorSet[42].pTexelBufferView = nullptr;

    descriptorBufferInfo[43]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[43].buffer = m_vdata.m_pAccelStruct_m_vertNormBuffer;
    descriptorBufferInfo[43].offset = m_vdata.m_pAccelStruct_m_vertNormOffset;
    descriptorBufferInfo[43].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[43]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[43].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[43].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[43].dstBinding       = 43;
    writeDescriptorSet[43].descriptorCount  = 1;
    writeDescriptorSet[43].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[43].pBufferInfo      = &descriptorBufferInfo[43];
    writeDescriptorSet[43].pImageInfo       = nullptr;
    writeDescriptorSet[43].pTexelBufferView = nullptr;

    descriptorBufferInfo[44]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[44].buffer = m_vdata.m_pAccelStruct_m_vertPosBuffer;
    descriptorBufferInfo[44].offset = m_vdata.m_pAccelStruct_m_vertPosOffset;
    descriptorBufferInfo[44].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[44]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[44].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[44].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[44].dstBinding       = 44;
    writeDescriptorSet[44].descriptorCount  = 1;
    writeDescriptorSet[44].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[44].pBufferInfo      = &descriptorBufferInfo[44];
    writeDescriptorSet[44].pImageInfo       = nullptr;
    writeDescriptorSet[44].pTexelBufferView = nullptr;

    descriptorBufferInfo[45]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[45].buffer = m_vdata.m_pAccelStruct_startEndBuffer;
    descriptorBufferInfo[45].offset = m_vdata.m_pAccelStruct_startEndOffset;
    descriptorBufferInfo[45].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[45]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[45].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[45].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[45].dstBinding       = 45;
    writeDescriptorSet[45].descriptorCount  = 1;
    writeDescriptorSet[45].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[45].pBufferInfo      = &descriptorBufferInfo[45];
    writeDescriptorSet[45].pImageInfo       = nullptr;
    writeDescriptorSet[45].pTexelBufferView = nullptr;

    descriptorBufferInfo[46]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[46].buffer = m_vdata.m_packedXYBuffer;
    descriptorBufferInfo[46].offset = m_vdata.m_packedXYOffset;
    descriptorBufferInfo[46].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[46]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[46].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[46].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[46].dstBinding       = 46;
    writeDescriptorSet[46].descriptorCount  = 1;
    writeDescriptorSet[46].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[46].pBufferInfo      = &descriptorBufferInfo[46];
    writeDescriptorSet[46].pImageInfo       = nullptr;
    writeDescriptorSet[46].pTexelBufferView = nullptr;

    descriptorBufferInfo[47]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[47].buffer = m_vdata.m_transparencyBufferBuffer;
    descriptorBufferInfo[47].offset = m_vdata.m_transparencyBufferOffset;
    descriptorBufferInfo[47].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[47]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[47].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[47].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[47].dstBinding       = 47;
    writeDescriptorSet[47].descriptorCount  = 1;
    writeDescriptorSet[47].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[47].pBufferInfo      = &descriptorBufferInfo[47];
    writeDescriptorSet[47].pImageInfo       = nullptr;
    writeDescriptorSet[47].pTexelBufferView = nullptr;

    descriptorBufferInfo[48]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[48].buffer = m_classDataBuffer;
    descriptorBufferInfo[48].offset = 0;
    descriptorBufferInfo[48].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[48]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[48].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[48].dstSet           = m_allGeneratedDS[9];
    writeDescriptorSet[48].dstBinding       = 48;
    writeDescriptorSet[48].descriptorCount  = 1;
    writeDescriptorSet[48].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[48].pBufferInfo      = &descriptorBufferInfo[48];
    writeDescriptorSet[48].pImageInfo       = nullptr;
    writeDescriptorSet[48].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
  // descriptor set #10: ResolveCommonWithTransparencyCmd (["m_colorBuffer.data()","m_gBuffer","m_geomOffsets","m_indices","m_instanceTransformInvTransposed","m_lights","m_matIdOffsets","m_matIdbyPrimId","m_materials","m_normals","m_textures","m_transparencyBuffer","m_vertices"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 13 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  13 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   13 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = m_vdata.m_colorBufferBuffer;
    descriptorBufferInfo[0].offset = m_vdata.m_colorBufferOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[10];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = m_vdata.m_gBufferBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_gBufferOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[10];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_vdata.m_geomOffsetsBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_geomOffsetsOffset;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[10];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    descriptorBufferInfo[3]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[3].buffer = m_vdata.m_indicesBuffer;
    descriptorBufferInfo[3].offset = m_vdata.m_indicesOffset;
    descriptorBufferInfo[3].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[3]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[3].dstSet           = m_allGeneratedDS[10];
    writeDescriptorSet[3].dstBinding       = 3;
    writeDescriptorSet[3].descriptorCount  = 1;
    writeDescriptorSet[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[3].pBufferInfo      = &descriptorBufferInfo[3];
    writeDescriptorSet[3].pImageInfo       = nullptr;
    writeDescriptorSet[3].pTexelBufferView = nullptr;

    descriptorBufferInfo[4]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[4].buffer = m_vdata.m_instanceTransformInvTransposedBuffer;
    descriptorBufferInfo[4].offset = m_vdata.m_instanceTransformInvTransposedOffset;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[10];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    descriptorBufferInfo[5]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[5].buffer = m_vdata.m_lightsBuffer;
    descriptorBufferInfo[5].offset = m_vdata.m_lightsOffset;
    descriptorBufferInfo[5].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[5]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[5].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[5].dstSet           = m_allGeneratedDS[10];
    writeDescriptorSet[5].dstBinding       = 5;
    writeDescriptorSet[5].descriptorCount  = 1;
    writeDescriptorSet[5].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[5].pBufferInfo      = &descriptorBufferInfo[5];
    writeDescriptorSet[5].pImageInfo       = nullptr;
    writeDescriptorSet[5].pTexelBufferView = nullptr;

    descriptorBufferInfo[6]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[6].buffer = m_vdata.m_matIdOffsetsBuffer;
    descriptorBufferInfo[6].offset = m_vdata.m_matIdOffsetsOffset;
    descriptorBufferInfo[6].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[6]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[6].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[6].dstSet           = m_allGeneratedDS[10];
    writeDescriptorSet[6].dstBinding       = 6;
    writeDescriptorSet[6].descriptorCount  = 1;
    writeDescriptorSet[6].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[6].pBufferInfo      = &descriptorBufferInfo[6];
    writeDescriptorSet[6].pImageInfo       = nullptr;
    writeDescriptorSet[6].pTexelBufferView = nullptr;

    descriptorBufferInfo[7]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[7].buffer = m_vdata.m_matIdbyPrimIdBuffer;
    descriptorBufferInfo[7].offset = m_vdata.m_matIdbyPrimIdOffset;
    descriptorBufferInfo[7].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[7]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[7].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[7].dstSet           = m_allGeneratedDS[10];
    writeDescriptorSet[7].dstBinding       = 7;
    writeDescriptorSet[7].descriptorCount  = 1;
    writeDescriptorSet[7].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[7].pBufferInfo      = &descriptorBufferInfo[7];
    writeDescriptorSet[7].pImageInfo       = nullptr;
    writeDescriptorSet[7].pTexelBufferView = nullptr;

    descriptorBufferInfo[8]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[8].buffer = m_vdata.m_materialsBuffer;
    descriptorBufferInfo[8].offset = m_vdata.m_materialsOffset;
    descriptorBufferInfo[8].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[8]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[8].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[8].dstSet           = m_allGeneratedDS[10];
    writeDescriptorSet[8].dstBinding       = 8;
    writeDescriptorSet[8].descriptorCount  = 1;
    writeDescriptorSet[8].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[8].pBufferInfo      = &descriptorBufferInfo[8];
    writeDescriptorSet[8].pImageInfo       = nullptr;
    writeDescriptorSet[8].pTexelBufferView = nullptr;

    descriptorBufferInfo[9]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[9].buffer = m_vdata.m_normalsBuffer;
    descriptorBufferInfo[9].offset = m_vdata.m_normalsOffset;
    descriptorBufferInfo[9].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[9]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[9].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[9].dstSet           = m_allGeneratedDS[10];
    writeDescriptorSet[9].dstBinding       = 9;
    writeDescriptorSet[9].descriptorCount  = 1;
    writeDescriptorSet[9].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[9].pBufferInfo      = &descriptorBufferInfo[9];
    writeDescriptorSet[9].pImageInfo       = nullptr;
    writeDescriptorSet[9].pTexelBufferView = nullptr;

    std::vector<VkDescriptorImageInfo> m_texturesInfo(m_vdata.m_texturesArrayMaxSize);
    for(size_t i=0; i<m_vdata.m_texturesArrayMaxSize; i++)
    {
      if(i < m_textures.size())
      {
        m_texturesInfo[i].sampler     = m_vdata.m_texturesArraySampler[i];
        m_texturesInfo[i].imageView   = m_vdata.m_texturesArrayView   [i];
        m_texturesInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
      else
      {
        m_texturesInfo[i].sampler     = m_vdata.m_texturesArraySampler[0];
        m_texturesInfo[i].imageView   = m_vdata.m_texturesArrayView   [0];
        m_texturesInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
    }
    writeDescriptorSet[10]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[10].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[10].dstSet           = m_allGeneratedDS[10];
    writeDescriptorSet[10].dstBinding       = 10;
    writeDescriptorSet[10].descriptorCount  = 1;
    writeDescriptorSet[10].descriptorCount  = m_texturesInfo.size();
    writeDescriptorSet[10].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet[10].pBufferInfo      = nullptr;
    writeDescriptorSet[10].pImageInfo       = m_texturesInfo.data();
    writeDescriptorSet[10].pTexelBufferView = nullptr;

    descriptorBufferInfo[11]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[11].buffer = m_vdata.m_transparencyBufferBuffer;
    descriptorBufferInfo[11].offset = m_vdata.m_transparencyBufferOffset;
    descriptorBufferInfo[11].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[11]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[11].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[11].dstSet           = m_allGeneratedDS[10];
    writeDescriptorSet[11].dstBinding       = 11;
    writeDescriptorSet[11].descriptorCount  = 1;
    writeDescriptorSet[11].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[11].pBufferInfo      = &descriptorBufferInfo[11];
    writeDescriptorSet[11].pImageInfo       = nullptr;
    writeDescriptorSet[11].pTexelBufferView = nullptr;

    descriptorBufferInfo[12]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[12].buffer = m_vdata.m_verticesBuffer;
    descriptorBufferInfo[12].offset = m_vdata.m_verticesOffset;
    descriptorBufferInfo[12].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[12]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[12].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[12].dstSet           = m_allGeneratedDS[10];
    writeDescriptorSet[12].dstBinding       = 12;
    writeDescriptorSet[12].descriptorCount  = 1;
    writeDescriptorSet[12].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[12].pBufferInfo      = &descriptorBufferInfo[12];
    writeDescriptorSet[12].pImageInfo       = nullptr;
    writeDescriptorSet[12].pTexelBufferView = nullptr;

    descriptorBufferInfo[13]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[13].buffer = m_classDataBuffer;
    descriptorBufferInfo[13].offset = 0;
    descriptorBufferInfo[13].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[13]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[13].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[13].dstSet           = m_allGeneratedDS[10];
    writeDescriptorSet[13].dstBinding       = 13;
    writeDescriptorSet[13].descriptorCount  = 1;
    writeDescriptorSet[13].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[13].pBufferInfo      = &descriptorBufferInfo[13];
    writeDescriptorSet[13].pImageInfo       = nullptr;
    writeDescriptorSet[13].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
  // descriptor set #11: FillGbufferCmd (["m_gBuffer","m_pAccelStruct_m_GraphicsPrimHeaders","m_pAccelStruct_m_GraphicsPrimPoints","m_pAccelStruct_m_RotAddTable","m_pAccelStruct_m_SCom2Data","m_pAccelStruct_m_SCom2Headers","m_pAccelStruct_m_SdfCompactOctreeRotModifiers","m_pAccelStruct_m_SdfCompactOctreeRotTransforms","m_pAccelStruct_m_SdfCompactOctreeV3Data","m_pAccelStruct_m_SdfDAGChildEdges","m_pAccelStruct_m_SdfDAGDataEdges","m_pAccelStruct_m_SdfDAGDistances","m_pAccelStruct_m_SdfDAGHeaders","m_pAccelStruct_m_SdfDAGNodes","m_pAccelStruct_m_SdfDAGTranspositions","m_pAccelStruct_m_SdfFrameOctreeNodes","m_pAccelStruct_m_SdfFrameOctreeRoots","m_pAccelStruct_m_SdfFrameOctreeTexNodes","m_pAccelStruct_m_SdfFrameOctreeTexRoots","m_pAccelStruct_m_SdfGridData","m_pAccelStruct_m_SdfGridOffsets","m_pAccelStruct_m_SdfGridSizes","m_pAccelStruct_m_SdfMultiOctreeNodes","m_pAccelStruct_m_SdfMultiOctreeValues","m_pAccelStruct_m_SdfSBSData","m_pAccelStruct_m_SdfSBSDataF","m_pAccelStruct_m_SdfSBSHeaders","m_pAccelStruct_m_SdfSBSNodes","m_pAccelStruct_m_SdfSBSRoots","m_pAccelStruct_m_SdfSVSNodes","m_pAccelStruct_m_SdfSVSRoots","m_pAccelStruct_m_allNodePairs","m_pAccelStruct_m_geomData","m_pAccelStruct_m_geomTags","m_pAccelStruct_m_indices","m_pAccelStruct_m_instanceData","m_pAccelStruct_m_nodesTLAS","m_pAccelStruct_m_origNodes","m_pAccelStruct_m_primIdCount","m_pAccelStruct_m_primIndices","m_pAccelStruct_m_vertNorm","m_pAccelStruct_m_vertPos","m_pAccelStruct_startEnd","m_packedXY"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 44 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  44 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   44 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = m_vdata.m_gBufferBuffer;
    descriptorBufferInfo[0].offset = m_vdata.m_gBufferOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = m_vdata.m_pAccelStruct_m_GraphicsPrimHeadersBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_pAccelStruct_m_GraphicsPrimHeadersOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_vdata.m_pAccelStruct_m_GraphicsPrimPointsBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_pAccelStruct_m_GraphicsPrimPointsOffset;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    descriptorBufferInfo[3]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[3].buffer = m_vdata.m_pAccelStruct_m_RotAddTableBuffer;
    descriptorBufferInfo[3].offset = m_vdata.m_pAccelStruct_m_RotAddTableOffset;
    descriptorBufferInfo[3].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[3]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[3].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[3].dstBinding       = 3;
    writeDescriptorSet[3].descriptorCount  = 1;
    writeDescriptorSet[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[3].pBufferInfo      = &descriptorBufferInfo[3];
    writeDescriptorSet[3].pImageInfo       = nullptr;
    writeDescriptorSet[3].pTexelBufferView = nullptr;

    descriptorBufferInfo[4]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[4].buffer = m_vdata.m_pAccelStruct_m_SCom2DataBuffer;
    descriptorBufferInfo[4].offset = m_vdata.m_pAccelStruct_m_SCom2DataOffset;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    descriptorBufferInfo[5]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[5].buffer = m_vdata.m_pAccelStruct_m_SCom2HeadersBuffer;
    descriptorBufferInfo[5].offset = m_vdata.m_pAccelStruct_m_SCom2HeadersOffset;
    descriptorBufferInfo[5].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[5]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[5].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[5].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[5].dstBinding       = 5;
    writeDescriptorSet[5].descriptorCount  = 1;
    writeDescriptorSet[5].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[5].pBufferInfo      = &descriptorBufferInfo[5];
    writeDescriptorSet[5].pImageInfo       = nullptr;
    writeDescriptorSet[5].pTexelBufferView = nullptr;

    descriptorBufferInfo[6]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[6].buffer = m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotModifiersBuffer;
    descriptorBufferInfo[6].offset = m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotModifiersOffset;
    descriptorBufferInfo[6].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[6]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[6].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[6].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[6].dstBinding       = 6;
    writeDescriptorSet[6].descriptorCount  = 1;
    writeDescriptorSet[6].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[6].pBufferInfo      = &descriptorBufferInfo[6];
    writeDescriptorSet[6].pImageInfo       = nullptr;
    writeDescriptorSet[6].pTexelBufferView = nullptr;

    descriptorBufferInfo[7]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[7].buffer = m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotTransformsBuffer;
    descriptorBufferInfo[7].offset = m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotTransformsOffset;
    descriptorBufferInfo[7].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[7]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[7].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[7].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[7].dstBinding       = 7;
    writeDescriptorSet[7].descriptorCount  = 1;
    writeDescriptorSet[7].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[7].pBufferInfo      = &descriptorBufferInfo[7];
    writeDescriptorSet[7].pImageInfo       = nullptr;
    writeDescriptorSet[7].pTexelBufferView = nullptr;

    descriptorBufferInfo[8]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[8].buffer = m_vdata.m_pAccelStruct_m_SdfCompactOctreeV3DataBuffer;
    descriptorBufferInfo[8].offset = m_vdata.m_pAccelStruct_m_SdfCompactOctreeV3DataOffset;
    descriptorBufferInfo[8].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[8]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[8].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[8].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[8].dstBinding       = 8;
    writeDescriptorSet[8].descriptorCount  = 1;
    writeDescriptorSet[8].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[8].pBufferInfo      = &descriptorBufferInfo[8];
    writeDescriptorSet[8].pImageInfo       = nullptr;
    writeDescriptorSet[8].pTexelBufferView = nullptr;

    descriptorBufferInfo[9]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[9].buffer = m_vdata.m_pAccelStruct_m_SdfDAGChildEdgesBuffer;
    descriptorBufferInfo[9].offset = m_vdata.m_pAccelStruct_m_SdfDAGChildEdgesOffset;
    descriptorBufferInfo[9].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[9]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[9].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[9].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[9].dstBinding       = 9;
    writeDescriptorSet[9].descriptorCount  = 1;
    writeDescriptorSet[9].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[9].pBufferInfo      = &descriptorBufferInfo[9];
    writeDescriptorSet[9].pImageInfo       = nullptr;
    writeDescriptorSet[9].pTexelBufferView = nullptr;

    descriptorBufferInfo[10]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[10].buffer = m_vdata.m_pAccelStruct_m_SdfDAGDataEdgesBuffer;
    descriptorBufferInfo[10].offset = m_vdata.m_pAccelStruct_m_SdfDAGDataEdgesOffset;
    descriptorBufferInfo[10].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[10]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[10].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[10].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[10].dstBinding       = 10;
    writeDescriptorSet[10].descriptorCount  = 1;
    writeDescriptorSet[10].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[10].pBufferInfo      = &descriptorBufferInfo[10];
    writeDescriptorSet[10].pImageInfo       = nullptr;
    writeDescriptorSet[10].pTexelBufferView = nullptr;

    descriptorBufferInfo[11]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[11].buffer = m_vdata.m_pAccelStruct_m_SdfDAGDistancesBuffer;
    descriptorBufferInfo[11].offset = m_vdata.m_pAccelStruct_m_SdfDAGDistancesOffset;
    descriptorBufferInfo[11].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[11]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[11].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[11].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[11].dstBinding       = 11;
    writeDescriptorSet[11].descriptorCount  = 1;
    writeDescriptorSet[11].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[11].pBufferInfo      = &descriptorBufferInfo[11];
    writeDescriptorSet[11].pImageInfo       = nullptr;
    writeDescriptorSet[11].pTexelBufferView = nullptr;

    descriptorBufferInfo[12]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[12].buffer = m_vdata.m_pAccelStruct_m_SdfDAGHeadersBuffer;
    descriptorBufferInfo[12].offset = m_vdata.m_pAccelStruct_m_SdfDAGHeadersOffset;
    descriptorBufferInfo[12].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[12]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[12].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[12].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[12].dstBinding       = 12;
    writeDescriptorSet[12].descriptorCount  = 1;
    writeDescriptorSet[12].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[12].pBufferInfo      = &descriptorBufferInfo[12];
    writeDescriptorSet[12].pImageInfo       = nullptr;
    writeDescriptorSet[12].pTexelBufferView = nullptr;

    descriptorBufferInfo[13]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[13].buffer = m_vdata.m_pAccelStruct_m_SdfDAGNodesBuffer;
    descriptorBufferInfo[13].offset = m_vdata.m_pAccelStruct_m_SdfDAGNodesOffset;
    descriptorBufferInfo[13].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[13]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[13].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[13].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[13].dstBinding       = 13;
    writeDescriptorSet[13].descriptorCount  = 1;
    writeDescriptorSet[13].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[13].pBufferInfo      = &descriptorBufferInfo[13];
    writeDescriptorSet[13].pImageInfo       = nullptr;
    writeDescriptorSet[13].pTexelBufferView = nullptr;

    descriptorBufferInfo[14]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[14].buffer = m_vdata.m_pAccelStruct_m_SdfDAGTranspositionsBuffer;
    descriptorBufferInfo[14].offset = m_vdata.m_pAccelStruct_m_SdfDAGTranspositionsOffset;
    descriptorBufferInfo[14].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[14]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[14].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[14].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[14].dstBinding       = 14;
    writeDescriptorSet[14].descriptorCount  = 1;
    writeDescriptorSet[14].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[14].pBufferInfo      = &descriptorBufferInfo[14];
    writeDescriptorSet[14].pImageInfo       = nullptr;
    writeDescriptorSet[14].pTexelBufferView = nullptr;

    descriptorBufferInfo[15]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[15].buffer = m_vdata.m_pAccelStruct_m_SdfFrameOctreeNodesBuffer;
    descriptorBufferInfo[15].offset = m_vdata.m_pAccelStruct_m_SdfFrameOctreeNodesOffset;
    descriptorBufferInfo[15].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[15]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[15].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[15].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[15].dstBinding       = 15;
    writeDescriptorSet[15].descriptorCount  = 1;
    writeDescriptorSet[15].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[15].pBufferInfo      = &descriptorBufferInfo[15];
    writeDescriptorSet[15].pImageInfo       = nullptr;
    writeDescriptorSet[15].pTexelBufferView = nullptr;

    descriptorBufferInfo[16]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[16].buffer = m_vdata.m_pAccelStruct_m_SdfFrameOctreeRootsBuffer;
    descriptorBufferInfo[16].offset = m_vdata.m_pAccelStruct_m_SdfFrameOctreeRootsOffset;
    descriptorBufferInfo[16].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[16]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[16].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[16].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[16].dstBinding       = 16;
    writeDescriptorSet[16].descriptorCount  = 1;
    writeDescriptorSet[16].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[16].pBufferInfo      = &descriptorBufferInfo[16];
    writeDescriptorSet[16].pImageInfo       = nullptr;
    writeDescriptorSet[16].pTexelBufferView = nullptr;

    descriptorBufferInfo[17]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[17].buffer = m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexNodesBuffer;
    descriptorBufferInfo[17].offset = m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexNodesOffset;
    descriptorBufferInfo[17].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[17]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[17].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[17].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[17].dstBinding       = 17;
    writeDescriptorSet[17].descriptorCount  = 1;
    writeDescriptorSet[17].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[17].pBufferInfo      = &descriptorBufferInfo[17];
    writeDescriptorSet[17].pImageInfo       = nullptr;
    writeDescriptorSet[17].pTexelBufferView = nullptr;

    descriptorBufferInfo[18]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[18].buffer = m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexRootsBuffer;
    descriptorBufferInfo[18].offset = m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexRootsOffset;
    descriptorBufferInfo[18].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[18]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[18].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[18].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[18].dstBinding       = 18;
    writeDescriptorSet[18].descriptorCount  = 1;
    writeDescriptorSet[18].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[18].pBufferInfo      = &descriptorBufferInfo[18];
    writeDescriptorSet[18].pImageInfo       = nullptr;
    writeDescriptorSet[18].pTexelBufferView = nullptr;

    descriptorBufferInfo[19]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[19].buffer = m_vdata.m_pAccelStruct_m_SdfGridDataBuffer;
    descriptorBufferInfo[19].offset = m_vdata.m_pAccelStruct_m_SdfGridDataOffset;
    descriptorBufferInfo[19].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[19]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[19].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[19].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[19].dstBinding       = 19;
    writeDescriptorSet[19].descriptorCount  = 1;
    writeDescriptorSet[19].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[19].pBufferInfo      = &descriptorBufferInfo[19];
    writeDescriptorSet[19].pImageInfo       = nullptr;
    writeDescriptorSet[19].pTexelBufferView = nullptr;

    descriptorBufferInfo[20]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[20].buffer = m_vdata.m_pAccelStruct_m_SdfGridOffsetsBuffer;
    descriptorBufferInfo[20].offset = m_vdata.m_pAccelStruct_m_SdfGridOffsetsOffset;
    descriptorBufferInfo[20].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[20]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[20].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[20].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[20].dstBinding       = 20;
    writeDescriptorSet[20].descriptorCount  = 1;
    writeDescriptorSet[20].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[20].pBufferInfo      = &descriptorBufferInfo[20];
    writeDescriptorSet[20].pImageInfo       = nullptr;
    writeDescriptorSet[20].pTexelBufferView = nullptr;

    descriptorBufferInfo[21]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[21].buffer = m_vdata.m_pAccelStruct_m_SdfGridSizesBuffer;
    descriptorBufferInfo[21].offset = m_vdata.m_pAccelStruct_m_SdfGridSizesOffset;
    descriptorBufferInfo[21].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[21]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[21].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[21].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[21].dstBinding       = 21;
    writeDescriptorSet[21].descriptorCount  = 1;
    writeDescriptorSet[21].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[21].pBufferInfo      = &descriptorBufferInfo[21];
    writeDescriptorSet[21].pImageInfo       = nullptr;
    writeDescriptorSet[21].pTexelBufferView = nullptr;

    descriptorBufferInfo[22]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[22].buffer = m_vdata.m_pAccelStruct_m_SdfMultiOctreeNodesBuffer;
    descriptorBufferInfo[22].offset = m_vdata.m_pAccelStruct_m_SdfMultiOctreeNodesOffset;
    descriptorBufferInfo[22].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[22]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[22].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[22].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[22].dstBinding       = 22;
    writeDescriptorSet[22].descriptorCount  = 1;
    writeDescriptorSet[22].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[22].pBufferInfo      = &descriptorBufferInfo[22];
    writeDescriptorSet[22].pImageInfo       = nullptr;
    writeDescriptorSet[22].pTexelBufferView = nullptr;

    descriptorBufferInfo[23]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[23].buffer = m_vdata.m_pAccelStruct_m_SdfMultiOctreeValuesBuffer;
    descriptorBufferInfo[23].offset = m_vdata.m_pAccelStruct_m_SdfMultiOctreeValuesOffset;
    descriptorBufferInfo[23].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[23]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[23].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[23].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[23].dstBinding       = 23;
    writeDescriptorSet[23].descriptorCount  = 1;
    writeDescriptorSet[23].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[23].pBufferInfo      = &descriptorBufferInfo[23];
    writeDescriptorSet[23].pImageInfo       = nullptr;
    writeDescriptorSet[23].pTexelBufferView = nullptr;

    descriptorBufferInfo[24]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[24].buffer = m_vdata.m_pAccelStruct_m_SdfSBSDataBuffer;
    descriptorBufferInfo[24].offset = m_vdata.m_pAccelStruct_m_SdfSBSDataOffset;
    descriptorBufferInfo[24].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[24]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[24].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[24].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[24].dstBinding       = 24;
    writeDescriptorSet[24].descriptorCount  = 1;
    writeDescriptorSet[24].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[24].pBufferInfo      = &descriptorBufferInfo[24];
    writeDescriptorSet[24].pImageInfo       = nullptr;
    writeDescriptorSet[24].pTexelBufferView = nullptr;

    descriptorBufferInfo[25]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[25].buffer = m_vdata.m_pAccelStruct_m_SdfSBSDataFBuffer;
    descriptorBufferInfo[25].offset = m_vdata.m_pAccelStruct_m_SdfSBSDataFOffset;
    descriptorBufferInfo[25].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[25]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[25].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[25].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[25].dstBinding       = 25;
    writeDescriptorSet[25].descriptorCount  = 1;
    writeDescriptorSet[25].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[25].pBufferInfo      = &descriptorBufferInfo[25];
    writeDescriptorSet[25].pImageInfo       = nullptr;
    writeDescriptorSet[25].pTexelBufferView = nullptr;

    descriptorBufferInfo[26]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[26].buffer = m_vdata.m_pAccelStruct_m_SdfSBSHeadersBuffer;
    descriptorBufferInfo[26].offset = m_vdata.m_pAccelStruct_m_SdfSBSHeadersOffset;
    descriptorBufferInfo[26].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[26]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[26].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[26].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[26].dstBinding       = 26;
    writeDescriptorSet[26].descriptorCount  = 1;
    writeDescriptorSet[26].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[26].pBufferInfo      = &descriptorBufferInfo[26];
    writeDescriptorSet[26].pImageInfo       = nullptr;
    writeDescriptorSet[26].pTexelBufferView = nullptr;

    descriptorBufferInfo[27]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[27].buffer = m_vdata.m_pAccelStruct_m_SdfSBSNodesBuffer;
    descriptorBufferInfo[27].offset = m_vdata.m_pAccelStruct_m_SdfSBSNodesOffset;
    descriptorBufferInfo[27].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[27]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[27].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[27].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[27].dstBinding       = 27;
    writeDescriptorSet[27].descriptorCount  = 1;
    writeDescriptorSet[27].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[27].pBufferInfo      = &descriptorBufferInfo[27];
    writeDescriptorSet[27].pImageInfo       = nullptr;
    writeDescriptorSet[27].pTexelBufferView = nullptr;

    descriptorBufferInfo[28]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[28].buffer = m_vdata.m_pAccelStruct_m_SdfSBSRootsBuffer;
    descriptorBufferInfo[28].offset = m_vdata.m_pAccelStruct_m_SdfSBSRootsOffset;
    descriptorBufferInfo[28].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[28]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[28].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[28].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[28].dstBinding       = 28;
    writeDescriptorSet[28].descriptorCount  = 1;
    writeDescriptorSet[28].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[28].pBufferInfo      = &descriptorBufferInfo[28];
    writeDescriptorSet[28].pImageInfo       = nullptr;
    writeDescriptorSet[28].pTexelBufferView = nullptr;

    descriptorBufferInfo[29]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[29].buffer = m_vdata.m_pAccelStruct_m_SdfSVSNodesBuffer;
    descriptorBufferInfo[29].offset = m_vdata.m_pAccelStruct_m_SdfSVSNodesOffset;
    descriptorBufferInfo[29].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[29]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[29].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[29].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[29].dstBinding       = 29;
    writeDescriptorSet[29].descriptorCount  = 1;
    writeDescriptorSet[29].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[29].pBufferInfo      = &descriptorBufferInfo[29];
    writeDescriptorSet[29].pImageInfo       = nullptr;
    writeDescriptorSet[29].pTexelBufferView = nullptr;

    descriptorBufferInfo[30]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[30].buffer = m_vdata.m_pAccelStruct_m_SdfSVSRootsBuffer;
    descriptorBufferInfo[30].offset = m_vdata.m_pAccelStruct_m_SdfSVSRootsOffset;
    descriptorBufferInfo[30].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[30]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[30].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[30].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[30].dstBinding       = 30;
    writeDescriptorSet[30].descriptorCount  = 1;
    writeDescriptorSet[30].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[30].pBufferInfo      = &descriptorBufferInfo[30];
    writeDescriptorSet[30].pImageInfo       = nullptr;
    writeDescriptorSet[30].pTexelBufferView = nullptr;

    descriptorBufferInfo[31]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[31].buffer = m_vdata.m_pAccelStruct_m_allNodePairsBuffer;
    descriptorBufferInfo[31].offset = m_vdata.m_pAccelStruct_m_allNodePairsOffset;
    descriptorBufferInfo[31].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[31]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[31].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[31].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[31].dstBinding       = 31;
    writeDescriptorSet[31].descriptorCount  = 1;
    writeDescriptorSet[31].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[31].pBufferInfo      = &descriptorBufferInfo[31];
    writeDescriptorSet[31].pImageInfo       = nullptr;
    writeDescriptorSet[31].pTexelBufferView = nullptr;

    descriptorBufferInfo[32]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[32].buffer = m_vdata.m_pAccelStruct_m_geomDataBuffer;
    descriptorBufferInfo[32].offset = m_vdata.m_pAccelStruct_m_geomDataOffset;
    descriptorBufferInfo[32].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[32]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[32].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[32].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[32].dstBinding       = 32;
    writeDescriptorSet[32].descriptorCount  = 1;
    writeDescriptorSet[32].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[32].pBufferInfo      = &descriptorBufferInfo[32];
    writeDescriptorSet[32].pImageInfo       = nullptr;
    writeDescriptorSet[32].pTexelBufferView = nullptr;

    descriptorBufferInfo[33]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[33].buffer = m_vdata.m_pAccelStruct_m_geomTagsBuffer;
    descriptorBufferInfo[33].offset = m_vdata.m_pAccelStruct_m_geomTagsOffset;
    descriptorBufferInfo[33].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[33]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[33].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[33].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[33].dstBinding       = 33;
    writeDescriptorSet[33].descriptorCount  = 1;
    writeDescriptorSet[33].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[33].pBufferInfo      = &descriptorBufferInfo[33];
    writeDescriptorSet[33].pImageInfo       = nullptr;
    writeDescriptorSet[33].pTexelBufferView = nullptr;

    descriptorBufferInfo[34]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[34].buffer = m_vdata.m_pAccelStruct_m_indicesBuffer;
    descriptorBufferInfo[34].offset = m_vdata.m_pAccelStruct_m_indicesOffset;
    descriptorBufferInfo[34].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[34]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[34].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[34].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[34].dstBinding       = 34;
    writeDescriptorSet[34].descriptorCount  = 1;
    writeDescriptorSet[34].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[34].pBufferInfo      = &descriptorBufferInfo[34];
    writeDescriptorSet[34].pImageInfo       = nullptr;
    writeDescriptorSet[34].pTexelBufferView = nullptr;

    descriptorBufferInfo[35]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[35].buffer = m_vdata.m_pAccelStruct_m_instanceDataBuffer;
    descriptorBufferInfo[35].offset = m_vdata.m_pAccelStruct_m_instanceDataOffset;
    descriptorBufferInfo[35].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[35]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[35].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[35].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[35].dstBinding       = 35;
    writeDescriptorSet[35].descriptorCount  = 1;
    writeDescriptorSet[35].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[35].pBufferInfo      = &descriptorBufferInfo[35];
    writeDescriptorSet[35].pImageInfo       = nullptr;
    writeDescriptorSet[35].pTexelBufferView = nullptr;

    descriptorBufferInfo[36]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[36].buffer = m_vdata.m_pAccelStruct_m_nodesTLASBuffer;
    descriptorBufferInfo[36].offset = m_vdata.m_pAccelStruct_m_nodesTLASOffset;
    descriptorBufferInfo[36].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[36]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[36].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[36].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[36].dstBinding       = 36;
    writeDescriptorSet[36].descriptorCount  = 1;
    writeDescriptorSet[36].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[36].pBufferInfo      = &descriptorBufferInfo[36];
    writeDescriptorSet[36].pImageInfo       = nullptr;
    writeDescriptorSet[36].pTexelBufferView = nullptr;

    descriptorBufferInfo[37]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[37].buffer = m_vdata.m_pAccelStruct_m_origNodesBuffer;
    descriptorBufferInfo[37].offset = m_vdata.m_pAccelStruct_m_origNodesOffset;
    descriptorBufferInfo[37].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[37]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[37].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[37].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[37].dstBinding       = 37;
    writeDescriptorSet[37].descriptorCount  = 1;
    writeDescriptorSet[37].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[37].pBufferInfo      = &descriptorBufferInfo[37];
    writeDescriptorSet[37].pImageInfo       = nullptr;
    writeDescriptorSet[37].pTexelBufferView = nullptr;

    descriptorBufferInfo[38]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[38].buffer = m_vdata.m_pAccelStruct_m_primIdCountBuffer;
    descriptorBufferInfo[38].offset = m_vdata.m_pAccelStruct_m_primIdCountOffset;
    descriptorBufferInfo[38].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[38]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[38].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[38].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[38].dstBinding       = 38;
    writeDescriptorSet[38].descriptorCount  = 1;
    writeDescriptorSet[38].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[38].pBufferInfo      = &descriptorBufferInfo[38];
    writeDescriptorSet[38].pImageInfo       = nullptr;
    writeDescriptorSet[38].pTexelBufferView = nullptr;

    descriptorBufferInfo[39]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[39].buffer = m_vdata.m_pAccelStruct_m_primIndicesBuffer;
    descriptorBufferInfo[39].offset = m_vdata.m_pAccelStruct_m_primIndicesOffset;
    descriptorBufferInfo[39].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[39]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[39].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[39].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[39].dstBinding       = 39;
    writeDescriptorSet[39].descriptorCount  = 1;
    writeDescriptorSet[39].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[39].pBufferInfo      = &descriptorBufferInfo[39];
    writeDescriptorSet[39].pImageInfo       = nullptr;
    writeDescriptorSet[39].pTexelBufferView = nullptr;

    descriptorBufferInfo[40]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[40].buffer = m_vdata.m_pAccelStruct_m_vertNormBuffer;
    descriptorBufferInfo[40].offset = m_vdata.m_pAccelStruct_m_vertNormOffset;
    descriptorBufferInfo[40].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[40]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[40].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[40].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[40].dstBinding       = 40;
    writeDescriptorSet[40].descriptorCount  = 1;
    writeDescriptorSet[40].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[40].pBufferInfo      = &descriptorBufferInfo[40];
    writeDescriptorSet[40].pImageInfo       = nullptr;
    writeDescriptorSet[40].pTexelBufferView = nullptr;

    descriptorBufferInfo[41]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[41].buffer = m_vdata.m_pAccelStruct_m_vertPosBuffer;
    descriptorBufferInfo[41].offset = m_vdata.m_pAccelStruct_m_vertPosOffset;
    descriptorBufferInfo[41].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[41]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[41].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[41].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[41].dstBinding       = 41;
    writeDescriptorSet[41].descriptorCount  = 1;
    writeDescriptorSet[41].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[41].pBufferInfo      = &descriptorBufferInfo[41];
    writeDescriptorSet[41].pImageInfo       = nullptr;
    writeDescriptorSet[41].pTexelBufferView = nullptr;

    descriptorBufferInfo[42]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[42].buffer = m_vdata.m_pAccelStruct_startEndBuffer;
    descriptorBufferInfo[42].offset = m_vdata.m_pAccelStruct_startEndOffset;
    descriptorBufferInfo[42].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[42]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[42].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[42].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[42].dstBinding       = 42;
    writeDescriptorSet[42].descriptorCount  = 1;
    writeDescriptorSet[42].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[42].pBufferInfo      = &descriptorBufferInfo[42];
    writeDescriptorSet[42].pImageInfo       = nullptr;
    writeDescriptorSet[42].pTexelBufferView = nullptr;

    descriptorBufferInfo[43]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[43].buffer = m_vdata.m_packedXYBuffer;
    descriptorBufferInfo[43].offset = m_vdata.m_packedXYOffset;
    descriptorBufferInfo[43].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[43]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[43].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[43].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[43].dstBinding       = 43;
    writeDescriptorSet[43].descriptorCount  = 1;
    writeDescriptorSet[43].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[43].pBufferInfo      = &descriptorBufferInfo[43];
    writeDescriptorSet[43].pImageInfo       = nullptr;
    writeDescriptorSet[43].pTexelBufferView = nullptr;

    descriptorBufferInfo[44]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[44].buffer = m_classDataBuffer;
    descriptorBufferInfo[44].offset = 0;
    descriptorBufferInfo[44].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[44]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[44].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[44].dstSet           = m_allGeneratedDS[11];
    writeDescriptorSet[44].dstBinding       = 44;
    writeDescriptorSet[44].descriptorCount  = 1;
    writeDescriptorSet[44].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[44].pBufferInfo      = &descriptorBufferInfo[44];
    writeDescriptorSet[44].pImageInfo       = nullptr;
    writeDescriptorSet[44].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
  // descriptor set #12: ResolveCommonCmd (["m_colorBuffer.data()","m_gBuffer","m_geomOffsets","m_indices","m_instanceTransformInvTransposed","m_lights","m_matIdOffsets","m_matIdbyPrimId","m_materials","m_normals","m_textures","m_vertices"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 12 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  12 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   12 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = m_vdata.m_colorBufferBuffer;
    descriptorBufferInfo[0].offset = m_vdata.m_colorBufferOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[12];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = m_vdata.m_gBufferBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_gBufferOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[12];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_vdata.m_geomOffsetsBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_geomOffsetsOffset;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[12];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    descriptorBufferInfo[3]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[3].buffer = m_vdata.m_indicesBuffer;
    descriptorBufferInfo[3].offset = m_vdata.m_indicesOffset;
    descriptorBufferInfo[3].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[3]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[3].dstSet           = m_allGeneratedDS[12];
    writeDescriptorSet[3].dstBinding       = 3;
    writeDescriptorSet[3].descriptorCount  = 1;
    writeDescriptorSet[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[3].pBufferInfo      = &descriptorBufferInfo[3];
    writeDescriptorSet[3].pImageInfo       = nullptr;
    writeDescriptorSet[3].pTexelBufferView = nullptr;

    descriptorBufferInfo[4]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[4].buffer = m_vdata.m_instanceTransformInvTransposedBuffer;
    descriptorBufferInfo[4].offset = m_vdata.m_instanceTransformInvTransposedOffset;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[12];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    descriptorBufferInfo[5]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[5].buffer = m_vdata.m_lightsBuffer;
    descriptorBufferInfo[5].offset = m_vdata.m_lightsOffset;
    descriptorBufferInfo[5].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[5]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[5].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[5].dstSet           = m_allGeneratedDS[12];
    writeDescriptorSet[5].dstBinding       = 5;
    writeDescriptorSet[5].descriptorCount  = 1;
    writeDescriptorSet[5].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[5].pBufferInfo      = &descriptorBufferInfo[5];
    writeDescriptorSet[5].pImageInfo       = nullptr;
    writeDescriptorSet[5].pTexelBufferView = nullptr;

    descriptorBufferInfo[6]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[6].buffer = m_vdata.m_matIdOffsetsBuffer;
    descriptorBufferInfo[6].offset = m_vdata.m_matIdOffsetsOffset;
    descriptorBufferInfo[6].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[6]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[6].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[6].dstSet           = m_allGeneratedDS[12];
    writeDescriptorSet[6].dstBinding       = 6;
    writeDescriptorSet[6].descriptorCount  = 1;
    writeDescriptorSet[6].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[6].pBufferInfo      = &descriptorBufferInfo[6];
    writeDescriptorSet[6].pImageInfo       = nullptr;
    writeDescriptorSet[6].pTexelBufferView = nullptr;

    descriptorBufferInfo[7]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[7].buffer = m_vdata.m_matIdbyPrimIdBuffer;
    descriptorBufferInfo[7].offset = m_vdata.m_matIdbyPrimIdOffset;
    descriptorBufferInfo[7].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[7]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[7].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[7].dstSet           = m_allGeneratedDS[12];
    writeDescriptorSet[7].dstBinding       = 7;
    writeDescriptorSet[7].descriptorCount  = 1;
    writeDescriptorSet[7].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[7].pBufferInfo      = &descriptorBufferInfo[7];
    writeDescriptorSet[7].pImageInfo       = nullptr;
    writeDescriptorSet[7].pTexelBufferView = nullptr;

    descriptorBufferInfo[8]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[8].buffer = m_vdata.m_materialsBuffer;
    descriptorBufferInfo[8].offset = m_vdata.m_materialsOffset;
    descriptorBufferInfo[8].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[8]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[8].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[8].dstSet           = m_allGeneratedDS[12];
    writeDescriptorSet[8].dstBinding       = 8;
    writeDescriptorSet[8].descriptorCount  = 1;
    writeDescriptorSet[8].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[8].pBufferInfo      = &descriptorBufferInfo[8];
    writeDescriptorSet[8].pImageInfo       = nullptr;
    writeDescriptorSet[8].pTexelBufferView = nullptr;

    descriptorBufferInfo[9]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[9].buffer = m_vdata.m_normalsBuffer;
    descriptorBufferInfo[9].offset = m_vdata.m_normalsOffset;
    descriptorBufferInfo[9].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[9]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[9].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[9].dstSet           = m_allGeneratedDS[12];
    writeDescriptorSet[9].dstBinding       = 9;
    writeDescriptorSet[9].descriptorCount  = 1;
    writeDescriptorSet[9].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[9].pBufferInfo      = &descriptorBufferInfo[9];
    writeDescriptorSet[9].pImageInfo       = nullptr;
    writeDescriptorSet[9].pTexelBufferView = nullptr;

    std::vector<VkDescriptorImageInfo> m_texturesInfo(m_vdata.m_texturesArrayMaxSize);
    for(size_t i=0; i<m_vdata.m_texturesArrayMaxSize; i++)
    {
      if(i < m_textures.size())
      {
        m_texturesInfo[i].sampler     = m_vdata.m_texturesArraySampler[i];
        m_texturesInfo[i].imageView   = m_vdata.m_texturesArrayView   [i];
        m_texturesInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
      else
      {
        m_texturesInfo[i].sampler     = m_vdata.m_texturesArraySampler[0];
        m_texturesInfo[i].imageView   = m_vdata.m_texturesArrayView   [0];
        m_texturesInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
    }
    writeDescriptorSet[10]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[10].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[10].dstSet           = m_allGeneratedDS[12];
    writeDescriptorSet[10].dstBinding       = 10;
    writeDescriptorSet[10].descriptorCount  = 1;
    writeDescriptorSet[10].descriptorCount  = m_texturesInfo.size();
    writeDescriptorSet[10].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet[10].pBufferInfo      = nullptr;
    writeDescriptorSet[10].pImageInfo       = m_texturesInfo.data();
    writeDescriptorSet[10].pTexelBufferView = nullptr;

    descriptorBufferInfo[11]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[11].buffer = m_vdata.m_verticesBuffer;
    descriptorBufferInfo[11].offset = m_vdata.m_verticesOffset;
    descriptorBufferInfo[11].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[11]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[11].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[11].dstSet           = m_allGeneratedDS[12];
    writeDescriptorSet[11].dstBinding       = 11;
    writeDescriptorSet[11].descriptorCount  = 1;
    writeDescriptorSet[11].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[11].pBufferInfo      = &descriptorBufferInfo[11];
    writeDescriptorSet[11].pImageInfo       = nullptr;
    writeDescriptorSet[11].pTexelBufferView = nullptr;

    descriptorBufferInfo[12]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[12].buffer = m_classDataBuffer;
    descriptorBufferInfo[12].offset = 0;
    descriptorBufferInfo[12].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[12]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[12].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[12].dstSet           = m_allGeneratedDS[12];
    writeDescriptorSet[12].dstBinding       = 12;
    writeDescriptorSet[12].descriptorCount  = 1;
    writeDescriptorSet[12].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[12].pBufferInfo      = &descriptorBufferInfo[12];
    writeDescriptorSet[12].pImageInfo       = nullptr;
    writeDescriptorSet[12].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
  // descriptor set #13: ResolveCommonCmd (["m_colorBuffer.data()","m_gBuffer","m_geomOffsets","m_indices","m_instanceTransformInvTransposed","m_lights","m_matIdOffsets","m_matIdbyPrimId","m_materials","m_normals","m_textures","m_vertices"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 12 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  12 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   12 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = m_vdata.m_colorBufferBuffer;
    descriptorBufferInfo[0].offset = m_vdata.m_colorBufferOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[13];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = m_vdata.m_gBufferBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_gBufferOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[13];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_vdata.m_geomOffsetsBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_geomOffsetsOffset;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[13];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    descriptorBufferInfo[3]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[3].buffer = m_vdata.m_indicesBuffer;
    descriptorBufferInfo[3].offset = m_vdata.m_indicesOffset;
    descriptorBufferInfo[3].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[3]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[3].dstSet           = m_allGeneratedDS[13];
    writeDescriptorSet[3].dstBinding       = 3;
    writeDescriptorSet[3].descriptorCount  = 1;
    writeDescriptorSet[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[3].pBufferInfo      = &descriptorBufferInfo[3];
    writeDescriptorSet[3].pImageInfo       = nullptr;
    writeDescriptorSet[3].pTexelBufferView = nullptr;

    descriptorBufferInfo[4]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[4].buffer = m_vdata.m_instanceTransformInvTransposedBuffer;
    descriptorBufferInfo[4].offset = m_vdata.m_instanceTransformInvTransposedOffset;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[13];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    descriptorBufferInfo[5]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[5].buffer = m_vdata.m_lightsBuffer;
    descriptorBufferInfo[5].offset = m_vdata.m_lightsOffset;
    descriptorBufferInfo[5].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[5]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[5].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[5].dstSet           = m_allGeneratedDS[13];
    writeDescriptorSet[5].dstBinding       = 5;
    writeDescriptorSet[5].descriptorCount  = 1;
    writeDescriptorSet[5].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[5].pBufferInfo      = &descriptorBufferInfo[5];
    writeDescriptorSet[5].pImageInfo       = nullptr;
    writeDescriptorSet[5].pTexelBufferView = nullptr;

    descriptorBufferInfo[6]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[6].buffer = m_vdata.m_matIdOffsetsBuffer;
    descriptorBufferInfo[6].offset = m_vdata.m_matIdOffsetsOffset;
    descriptorBufferInfo[6].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[6]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[6].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[6].dstSet           = m_allGeneratedDS[13];
    writeDescriptorSet[6].dstBinding       = 6;
    writeDescriptorSet[6].descriptorCount  = 1;
    writeDescriptorSet[6].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[6].pBufferInfo      = &descriptorBufferInfo[6];
    writeDescriptorSet[6].pImageInfo       = nullptr;
    writeDescriptorSet[6].pTexelBufferView = nullptr;

    descriptorBufferInfo[7]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[7].buffer = m_vdata.m_matIdbyPrimIdBuffer;
    descriptorBufferInfo[7].offset = m_vdata.m_matIdbyPrimIdOffset;
    descriptorBufferInfo[7].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[7]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[7].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[7].dstSet           = m_allGeneratedDS[13];
    writeDescriptorSet[7].dstBinding       = 7;
    writeDescriptorSet[7].descriptorCount  = 1;
    writeDescriptorSet[7].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[7].pBufferInfo      = &descriptorBufferInfo[7];
    writeDescriptorSet[7].pImageInfo       = nullptr;
    writeDescriptorSet[7].pTexelBufferView = nullptr;

    descriptorBufferInfo[8]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[8].buffer = m_vdata.m_materialsBuffer;
    descriptorBufferInfo[8].offset = m_vdata.m_materialsOffset;
    descriptorBufferInfo[8].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[8]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[8].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[8].dstSet           = m_allGeneratedDS[13];
    writeDescriptorSet[8].dstBinding       = 8;
    writeDescriptorSet[8].descriptorCount  = 1;
    writeDescriptorSet[8].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[8].pBufferInfo      = &descriptorBufferInfo[8];
    writeDescriptorSet[8].pImageInfo       = nullptr;
    writeDescriptorSet[8].pTexelBufferView = nullptr;

    descriptorBufferInfo[9]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[9].buffer = m_vdata.m_normalsBuffer;
    descriptorBufferInfo[9].offset = m_vdata.m_normalsOffset;
    descriptorBufferInfo[9].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[9]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[9].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[9].dstSet           = m_allGeneratedDS[13];
    writeDescriptorSet[9].dstBinding       = 9;
    writeDescriptorSet[9].descriptorCount  = 1;
    writeDescriptorSet[9].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[9].pBufferInfo      = &descriptorBufferInfo[9];
    writeDescriptorSet[9].pImageInfo       = nullptr;
    writeDescriptorSet[9].pTexelBufferView = nullptr;

    std::vector<VkDescriptorImageInfo> m_texturesInfo(m_vdata.m_texturesArrayMaxSize);
    for(size_t i=0; i<m_vdata.m_texturesArrayMaxSize; i++)
    {
      if(i < m_textures.size())
      {
        m_texturesInfo[i].sampler     = m_vdata.m_texturesArraySampler[i];
        m_texturesInfo[i].imageView   = m_vdata.m_texturesArrayView   [i];
        m_texturesInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
      else
      {
        m_texturesInfo[i].sampler     = m_vdata.m_texturesArraySampler[0];
        m_texturesInfo[i].imageView   = m_vdata.m_texturesArrayView   [0];
        m_texturesInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
    }
    writeDescriptorSet[10]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[10].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[10].dstSet           = m_allGeneratedDS[13];
    writeDescriptorSet[10].dstBinding       = 10;
    writeDescriptorSet[10].descriptorCount  = 1;
    writeDescriptorSet[10].descriptorCount  = m_texturesInfo.size();
    writeDescriptorSet[10].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet[10].pBufferInfo      = nullptr;
    writeDescriptorSet[10].pImageInfo       = m_texturesInfo.data();
    writeDescriptorSet[10].pTexelBufferView = nullptr;

    descriptorBufferInfo[11]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[11].buffer = m_vdata.m_verticesBuffer;
    descriptorBufferInfo[11].offset = m_vdata.m_verticesOffset;
    descriptorBufferInfo[11].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[11]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[11].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[11].dstSet           = m_allGeneratedDS[13];
    writeDescriptorSet[11].dstBinding       = 11;
    writeDescriptorSet[11].descriptorCount  = 1;
    writeDescriptorSet[11].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[11].pBufferInfo      = &descriptorBufferInfo[11];
    writeDescriptorSet[11].pImageInfo       = nullptr;
    writeDescriptorSet[11].pTexelBufferView = nullptr;

    descriptorBufferInfo[12]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[12].buffer = m_classDataBuffer;
    descriptorBufferInfo[12].offset = 0;
    descriptorBufferInfo[12].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[12]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[12].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[12].dstSet           = m_allGeneratedDS[13];
    writeDescriptorSet[12].dstBinding       = 12;
    writeDescriptorSet[12].descriptorCount  = 1;
    writeDescriptorSet[12].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[12].pBufferInfo      = &descriptorBufferInfo[12];
    writeDescriptorSet[12].pImageInfo       = nullptr;
    writeDescriptorSet[12].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
  // descriptor set #14: ResolveCTFCmd (["m_colorBuffer.data()","m_allCTFPoints","m_allCTFs","m_allChannelOffsets","m_allChannelRenderInfo","m_allCompressedChannels","m_allFloatChannels","m_allIntChannels","m_gBuffer","m_geomOffsets","m_indices","m_instanceTransformInvTransposed","m_lights","m_matIdOffsets","m_matIdbyPrimId","m_materials","m_normals","m_textures","m_vertices"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 19 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  19 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   19 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = m_vdata.m_colorBufferBuffer;
    descriptorBufferInfo[0].offset = m_vdata.m_colorBufferOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = m_vdata.m_allCTFPointsBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_allCTFPointsOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_vdata.m_allCTFsBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_allCTFsOffset;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    descriptorBufferInfo[3]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[3].buffer = m_vdata.m_allChannelOffsetsBuffer;
    descriptorBufferInfo[3].offset = m_vdata.m_allChannelOffsetsOffset;
    descriptorBufferInfo[3].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[3]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[3].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[3].dstBinding       = 3;
    writeDescriptorSet[3].descriptorCount  = 1;
    writeDescriptorSet[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[3].pBufferInfo      = &descriptorBufferInfo[3];
    writeDescriptorSet[3].pImageInfo       = nullptr;
    writeDescriptorSet[3].pTexelBufferView = nullptr;

    descriptorBufferInfo[4]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[4].buffer = m_vdata.m_allChannelRenderInfoBuffer;
    descriptorBufferInfo[4].offset = m_vdata.m_allChannelRenderInfoOffset;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    descriptorBufferInfo[5]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[5].buffer = m_vdata.m_allCompressedChannelsBuffer;
    descriptorBufferInfo[5].offset = m_vdata.m_allCompressedChannelsOffset;
    descriptorBufferInfo[5].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[5]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[5].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[5].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[5].dstBinding       = 5;
    writeDescriptorSet[5].descriptorCount  = 1;
    writeDescriptorSet[5].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[5].pBufferInfo      = &descriptorBufferInfo[5];
    writeDescriptorSet[5].pImageInfo       = nullptr;
    writeDescriptorSet[5].pTexelBufferView = nullptr;

    descriptorBufferInfo[6]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[6].buffer = m_vdata.m_allFloatChannelsBuffer;
    descriptorBufferInfo[6].offset = m_vdata.m_allFloatChannelsOffset;
    descriptorBufferInfo[6].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[6]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[6].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[6].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[6].dstBinding       = 6;
    writeDescriptorSet[6].descriptorCount  = 1;
    writeDescriptorSet[6].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[6].pBufferInfo      = &descriptorBufferInfo[6];
    writeDescriptorSet[6].pImageInfo       = nullptr;
    writeDescriptorSet[6].pTexelBufferView = nullptr;

    descriptorBufferInfo[7]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[7].buffer = m_vdata.m_allIntChannelsBuffer;
    descriptorBufferInfo[7].offset = m_vdata.m_allIntChannelsOffset;
    descriptorBufferInfo[7].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[7]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[7].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[7].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[7].dstBinding       = 7;
    writeDescriptorSet[7].descriptorCount  = 1;
    writeDescriptorSet[7].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[7].pBufferInfo      = &descriptorBufferInfo[7];
    writeDescriptorSet[7].pImageInfo       = nullptr;
    writeDescriptorSet[7].pTexelBufferView = nullptr;

    descriptorBufferInfo[8]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[8].buffer = m_vdata.m_gBufferBuffer;
    descriptorBufferInfo[8].offset = m_vdata.m_gBufferOffset;
    descriptorBufferInfo[8].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[8]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[8].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[8].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[8].dstBinding       = 8;
    writeDescriptorSet[8].descriptorCount  = 1;
    writeDescriptorSet[8].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[8].pBufferInfo      = &descriptorBufferInfo[8];
    writeDescriptorSet[8].pImageInfo       = nullptr;
    writeDescriptorSet[8].pTexelBufferView = nullptr;

    descriptorBufferInfo[9]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[9].buffer = m_vdata.m_geomOffsetsBuffer;
    descriptorBufferInfo[9].offset = m_vdata.m_geomOffsetsOffset;
    descriptorBufferInfo[9].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[9]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[9].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[9].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[9].dstBinding       = 9;
    writeDescriptorSet[9].descriptorCount  = 1;
    writeDescriptorSet[9].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[9].pBufferInfo      = &descriptorBufferInfo[9];
    writeDescriptorSet[9].pImageInfo       = nullptr;
    writeDescriptorSet[9].pTexelBufferView = nullptr;

    descriptorBufferInfo[10]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[10].buffer = m_vdata.m_indicesBuffer;
    descriptorBufferInfo[10].offset = m_vdata.m_indicesOffset;
    descriptorBufferInfo[10].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[10]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[10].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[10].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[10].dstBinding       = 10;
    writeDescriptorSet[10].descriptorCount  = 1;
    writeDescriptorSet[10].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[10].pBufferInfo      = &descriptorBufferInfo[10];
    writeDescriptorSet[10].pImageInfo       = nullptr;
    writeDescriptorSet[10].pTexelBufferView = nullptr;

    descriptorBufferInfo[11]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[11].buffer = m_vdata.m_instanceTransformInvTransposedBuffer;
    descriptorBufferInfo[11].offset = m_vdata.m_instanceTransformInvTransposedOffset;
    descriptorBufferInfo[11].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[11]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[11].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[11].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[11].dstBinding       = 11;
    writeDescriptorSet[11].descriptorCount  = 1;
    writeDescriptorSet[11].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[11].pBufferInfo      = &descriptorBufferInfo[11];
    writeDescriptorSet[11].pImageInfo       = nullptr;
    writeDescriptorSet[11].pTexelBufferView = nullptr;

    descriptorBufferInfo[12]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[12].buffer = m_vdata.m_lightsBuffer;
    descriptorBufferInfo[12].offset = m_vdata.m_lightsOffset;
    descriptorBufferInfo[12].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[12]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[12].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[12].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[12].dstBinding       = 12;
    writeDescriptorSet[12].descriptorCount  = 1;
    writeDescriptorSet[12].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[12].pBufferInfo      = &descriptorBufferInfo[12];
    writeDescriptorSet[12].pImageInfo       = nullptr;
    writeDescriptorSet[12].pTexelBufferView = nullptr;

    descriptorBufferInfo[13]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[13].buffer = m_vdata.m_matIdOffsetsBuffer;
    descriptorBufferInfo[13].offset = m_vdata.m_matIdOffsetsOffset;
    descriptorBufferInfo[13].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[13]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[13].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[13].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[13].dstBinding       = 13;
    writeDescriptorSet[13].descriptorCount  = 1;
    writeDescriptorSet[13].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[13].pBufferInfo      = &descriptorBufferInfo[13];
    writeDescriptorSet[13].pImageInfo       = nullptr;
    writeDescriptorSet[13].pTexelBufferView = nullptr;

    descriptorBufferInfo[14]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[14].buffer = m_vdata.m_matIdbyPrimIdBuffer;
    descriptorBufferInfo[14].offset = m_vdata.m_matIdbyPrimIdOffset;
    descriptorBufferInfo[14].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[14]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[14].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[14].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[14].dstBinding       = 14;
    writeDescriptorSet[14].descriptorCount  = 1;
    writeDescriptorSet[14].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[14].pBufferInfo      = &descriptorBufferInfo[14];
    writeDescriptorSet[14].pImageInfo       = nullptr;
    writeDescriptorSet[14].pTexelBufferView = nullptr;

    descriptorBufferInfo[15]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[15].buffer = m_vdata.m_materialsBuffer;
    descriptorBufferInfo[15].offset = m_vdata.m_materialsOffset;
    descriptorBufferInfo[15].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[15]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[15].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[15].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[15].dstBinding       = 15;
    writeDescriptorSet[15].descriptorCount  = 1;
    writeDescriptorSet[15].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[15].pBufferInfo      = &descriptorBufferInfo[15];
    writeDescriptorSet[15].pImageInfo       = nullptr;
    writeDescriptorSet[15].pTexelBufferView = nullptr;

    descriptorBufferInfo[16]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[16].buffer = m_vdata.m_normalsBuffer;
    descriptorBufferInfo[16].offset = m_vdata.m_normalsOffset;
    descriptorBufferInfo[16].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[16]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[16].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[16].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[16].dstBinding       = 16;
    writeDescriptorSet[16].descriptorCount  = 1;
    writeDescriptorSet[16].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[16].pBufferInfo      = &descriptorBufferInfo[16];
    writeDescriptorSet[16].pImageInfo       = nullptr;
    writeDescriptorSet[16].pTexelBufferView = nullptr;

    std::vector<VkDescriptorImageInfo> m_texturesInfo(m_vdata.m_texturesArrayMaxSize);
    for(size_t i=0; i<m_vdata.m_texturesArrayMaxSize; i++)
    {
      if(i < m_textures.size())
      {
        m_texturesInfo[i].sampler     = m_vdata.m_texturesArraySampler[i];
        m_texturesInfo[i].imageView   = m_vdata.m_texturesArrayView   [i];
        m_texturesInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
      else
      {
        m_texturesInfo[i].sampler     = m_vdata.m_texturesArraySampler[0];
        m_texturesInfo[i].imageView   = m_vdata.m_texturesArrayView   [0];
        m_texturesInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }
    }
    writeDescriptorSet[17]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[17].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[17].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[17].dstBinding       = 17;
    writeDescriptorSet[17].descriptorCount  = 1;
    writeDescriptorSet[17].descriptorCount  = m_texturesInfo.size();
    writeDescriptorSet[17].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet[17].pBufferInfo      = nullptr;
    writeDescriptorSet[17].pImageInfo       = m_texturesInfo.data();
    writeDescriptorSet[17].pTexelBufferView = nullptr;

    descriptorBufferInfo[18]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[18].buffer = m_vdata.m_verticesBuffer;
    descriptorBufferInfo[18].offset = m_vdata.m_verticesOffset;
    descriptorBufferInfo[18].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[18]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[18].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[18].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[18].dstBinding       = 18;
    writeDescriptorSet[18].descriptorCount  = 1;
    writeDescriptorSet[18].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[18].pBufferInfo      = &descriptorBufferInfo[18];
    writeDescriptorSet[18].pImageInfo       = nullptr;
    writeDescriptorSet[18].pTexelBufferView = nullptr;

    descriptorBufferInfo[19]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[19].buffer = m_classDataBuffer;
    descriptorBufferInfo[19].offset = 0;
    descriptorBufferInfo[19].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[19]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[19].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[19].dstSet           = m_allGeneratedDS[14];
    writeDescriptorSet[19].dstBinding       = 19;
    writeDescriptorSet[19].descriptorCount  = 1;
    writeDescriptorSet[19].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[19].pBufferInfo      = &descriptorBufferInfo[19];
    writeDescriptorSet[19].pImageInfo       = nullptr;
    writeDescriptorSet[19].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
  // descriptor set #15: ResolveOutlineCmd (["m_colorBuffer.data()","m_gBuffer","m_geomOffsets","m_indices","m_instanceTransformInvTransposed","m_lights","m_normals","m_vertices"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 8 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  8 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   8 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = m_vdata.m_colorBufferBuffer;
    descriptorBufferInfo[0].offset = m_vdata.m_colorBufferOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[15];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = m_vdata.m_gBufferBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_gBufferOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[15];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_vdata.m_geomOffsetsBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_geomOffsetsOffset;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[15];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    descriptorBufferInfo[3]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[3].buffer = m_vdata.m_indicesBuffer;
    descriptorBufferInfo[3].offset = m_vdata.m_indicesOffset;
    descriptorBufferInfo[3].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[3]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[3].dstSet           = m_allGeneratedDS[15];
    writeDescriptorSet[3].dstBinding       = 3;
    writeDescriptorSet[3].descriptorCount  = 1;
    writeDescriptorSet[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[3].pBufferInfo      = &descriptorBufferInfo[3];
    writeDescriptorSet[3].pImageInfo       = nullptr;
    writeDescriptorSet[3].pTexelBufferView = nullptr;

    descriptorBufferInfo[4]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[4].buffer = m_vdata.m_instanceTransformInvTransposedBuffer;
    descriptorBufferInfo[4].offset = m_vdata.m_instanceTransformInvTransposedOffset;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[15];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    descriptorBufferInfo[5]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[5].buffer = m_vdata.m_lightsBuffer;
    descriptorBufferInfo[5].offset = m_vdata.m_lightsOffset;
    descriptorBufferInfo[5].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[5]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[5].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[5].dstSet           = m_allGeneratedDS[15];
    writeDescriptorSet[5].dstBinding       = 5;
    writeDescriptorSet[5].descriptorCount  = 1;
    writeDescriptorSet[5].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[5].pBufferInfo      = &descriptorBufferInfo[5];
    writeDescriptorSet[5].pImageInfo       = nullptr;
    writeDescriptorSet[5].pTexelBufferView = nullptr;

    descriptorBufferInfo[6]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[6].buffer = m_vdata.m_normalsBuffer;
    descriptorBufferInfo[6].offset = m_vdata.m_normalsOffset;
    descriptorBufferInfo[6].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[6]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[6].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[6].dstSet           = m_allGeneratedDS[15];
    writeDescriptorSet[6].dstBinding       = 6;
    writeDescriptorSet[6].descriptorCount  = 1;
    writeDescriptorSet[6].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[6].pBufferInfo      = &descriptorBufferInfo[6];
    writeDescriptorSet[6].pImageInfo       = nullptr;
    writeDescriptorSet[6].pTexelBufferView = nullptr;

    descriptorBufferInfo[7]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[7].buffer = m_vdata.m_verticesBuffer;
    descriptorBufferInfo[7].offset = m_vdata.m_verticesOffset;
    descriptorBufferInfo[7].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[7]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[7].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[7].dstSet           = m_allGeneratedDS[15];
    writeDescriptorSet[7].dstBinding       = 7;
    writeDescriptorSet[7].descriptorCount  = 1;
    writeDescriptorSet[7].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[7].pBufferInfo      = &descriptorBufferInfo[7];
    writeDescriptorSet[7].pImageInfo       = nullptr;
    writeDescriptorSet[7].pTexelBufferView = nullptr;

    descriptorBufferInfo[8]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[8].buffer = m_classDataBuffer;
    descriptorBufferInfo[8].offset = 0;
    descriptorBufferInfo[8].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[8]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[8].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[8].dstSet           = m_allGeneratedDS[15];
    writeDescriptorSet[8].dstBinding       = 8;
    writeDescriptorSet[8].descriptorCount  = 1;
    writeDescriptorSet[8].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[8].pBufferInfo      = &descriptorBufferInfo[8];
    writeDescriptorSet[8].pImageInfo       = nullptr;
    writeDescriptorSet[8].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
  // descriptor set #16: ResolveDebugCmd (["m_colorBuffer.data()","m_gBuffer","m_geomOffsets","m_indices","m_instanceTransformInvTransposed","m_matIdOffsets","m_matIdbyPrimId","m_normals","m_vertices"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 9 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  9 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   9 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = m_vdata.m_colorBufferBuffer;
    descriptorBufferInfo[0].offset = m_vdata.m_colorBufferOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[16];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = m_vdata.m_gBufferBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_gBufferOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[16];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_vdata.m_geomOffsetsBuffer;
    descriptorBufferInfo[2].offset = m_vdata.m_geomOffsetsOffset;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[16];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    descriptorBufferInfo[3]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[3].buffer = m_vdata.m_indicesBuffer;
    descriptorBufferInfo[3].offset = m_vdata.m_indicesOffset;
    descriptorBufferInfo[3].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[3]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[3].dstSet           = m_allGeneratedDS[16];
    writeDescriptorSet[3].dstBinding       = 3;
    writeDescriptorSet[3].descriptorCount  = 1;
    writeDescriptorSet[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[3].pBufferInfo      = &descriptorBufferInfo[3];
    writeDescriptorSet[3].pImageInfo       = nullptr;
    writeDescriptorSet[3].pTexelBufferView = nullptr;

    descriptorBufferInfo[4]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[4].buffer = m_vdata.m_instanceTransformInvTransposedBuffer;
    descriptorBufferInfo[4].offset = m_vdata.m_instanceTransformInvTransposedOffset;
    descriptorBufferInfo[4].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[4]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[4].dstSet           = m_allGeneratedDS[16];
    writeDescriptorSet[4].dstBinding       = 4;
    writeDescriptorSet[4].descriptorCount  = 1;
    writeDescriptorSet[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[4].pBufferInfo      = &descriptorBufferInfo[4];
    writeDescriptorSet[4].pImageInfo       = nullptr;
    writeDescriptorSet[4].pTexelBufferView = nullptr;

    descriptorBufferInfo[5]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[5].buffer = m_vdata.m_matIdOffsetsBuffer;
    descriptorBufferInfo[5].offset = m_vdata.m_matIdOffsetsOffset;
    descriptorBufferInfo[5].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[5]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[5].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[5].dstSet           = m_allGeneratedDS[16];
    writeDescriptorSet[5].dstBinding       = 5;
    writeDescriptorSet[5].descriptorCount  = 1;
    writeDescriptorSet[5].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[5].pBufferInfo      = &descriptorBufferInfo[5];
    writeDescriptorSet[5].pImageInfo       = nullptr;
    writeDescriptorSet[5].pTexelBufferView = nullptr;

    descriptorBufferInfo[6]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[6].buffer = m_vdata.m_matIdbyPrimIdBuffer;
    descriptorBufferInfo[6].offset = m_vdata.m_matIdbyPrimIdOffset;
    descriptorBufferInfo[6].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[6]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[6].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[6].dstSet           = m_allGeneratedDS[16];
    writeDescriptorSet[6].dstBinding       = 6;
    writeDescriptorSet[6].descriptorCount  = 1;
    writeDescriptorSet[6].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[6].pBufferInfo      = &descriptorBufferInfo[6];
    writeDescriptorSet[6].pImageInfo       = nullptr;
    writeDescriptorSet[6].pTexelBufferView = nullptr;

    descriptorBufferInfo[7]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[7].buffer = m_vdata.m_normalsBuffer;
    descriptorBufferInfo[7].offset = m_vdata.m_normalsOffset;
    descriptorBufferInfo[7].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[7]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[7].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[7].dstSet           = m_allGeneratedDS[16];
    writeDescriptorSet[7].dstBinding       = 7;
    writeDescriptorSet[7].descriptorCount  = 1;
    writeDescriptorSet[7].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[7].pBufferInfo      = &descriptorBufferInfo[7];
    writeDescriptorSet[7].pImageInfo       = nullptr;
    writeDescriptorSet[7].pTexelBufferView = nullptr;

    descriptorBufferInfo[8]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[8].buffer = m_vdata.m_verticesBuffer;
    descriptorBufferInfo[8].offset = m_vdata.m_verticesOffset;
    descriptorBufferInfo[8].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[8]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[8].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[8].dstSet           = m_allGeneratedDS[16];
    writeDescriptorSet[8].dstBinding       = 8;
    writeDescriptorSet[8].descriptorCount  = 1;
    writeDescriptorSet[8].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[8].pBufferInfo      = &descriptorBufferInfo[8];
    writeDescriptorSet[8].pImageInfo       = nullptr;
    writeDescriptorSet[8].pTexelBufferView = nullptr;

    descriptorBufferInfo[9]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[9].buffer = m_classDataBuffer;
    descriptorBufferInfo[9].offset = 0;
    descriptorBufferInfo[9].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[9]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[9].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[9].dstSet           = m_allGeneratedDS[16];
    writeDescriptorSet[9].dstBinding       = 9;
    writeDescriptorSet[9].descriptorCount  = 1;
    writeDescriptorSet[9].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[9].pBufferInfo      = &descriptorBufferInfo[9];
    writeDescriptorSet[9].pImageInfo       = nullptr;
    writeDescriptorSet[9].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
  // descriptor set #17: TonemapCmd (["a_outColor","m_colorBuffer"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 2 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  2 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   2 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = Render_local.a_outColorBuffer;
    descriptorBufferInfo[0].offset = Render_local.a_outColorOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[17];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = m_vdata.m_colorBufferBuffer;
    descriptorBufferInfo[1].offset = m_vdata.m_colorBufferOffset;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[17];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    descriptorBufferInfo[2]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[2].buffer = m_classDataBuffer;
    descriptorBufferInfo[2].offset = 0;
    descriptorBufferInfo[2].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[2]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[2].dstSet           = m_allGeneratedDS[17];
    writeDescriptorSet[2].dstBinding       = 2;
    writeDescriptorSet[2].descriptorCount  = 1;
    writeDescriptorSet[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[2].pBufferInfo      = &descriptorBufferInfo[2];
    writeDescriptorSet[2].pImageInfo       = nullptr;
    writeDescriptorSet[2].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
}

void MultiRenderer_slang_comp::UpdateAllGeneratedDescriptorSets_Clear()
{
  // now create actual bindings
  //
  // descriptor set #18: PackXYCmd (["m_packedXY"])
  {
    constexpr uint additionalSize = 1;

    std::array<VkDescriptorBufferInfo, 1 + additionalSize> descriptorBufferInfo;
    std::array<VkDescriptorImageInfo,  1 + additionalSize> descriptorImageInfo;
    std::array<VkWriteDescriptorSet,   1 + additionalSize> writeDescriptorSet;

    descriptorBufferInfo[0]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[0].buffer = m_vdata.m_packedXYBuffer;
    descriptorBufferInfo[0].offset = m_vdata.m_packedXYOffset;
    descriptorBufferInfo[0].range  = VK_WHOLE_SIZE;
    writeDescriptorSet[0]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet           = m_allGeneratedDS[18];
    writeDescriptorSet[0].dstBinding       = 0;
    writeDescriptorSet[0].descriptorCount  = 1;
    writeDescriptorSet[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo      = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo       = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    descriptorBufferInfo[1]        = VkDescriptorBufferInfo{};
    descriptorBufferInfo[1].buffer = m_classDataBuffer;
    descriptorBufferInfo[1].offset = 0;
    descriptorBufferInfo[1].range  = VK_WHOLE_SIZE;

    writeDescriptorSet[1]                  = VkWriteDescriptorSet{};
    writeDescriptorSet[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].dstSet           = m_allGeneratedDS[18];
    writeDescriptorSet[1].dstBinding       = 1;
    writeDescriptorSet[1].descriptorCount  = 1;
    writeDescriptorSet[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    writeDescriptorSet[1].pBufferInfo      = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo       = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_device, uint32_t(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, NULL);
  }
}



