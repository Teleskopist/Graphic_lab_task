#include <vector>
#include <memory>
#include <limits>
#include <cassert>
#include <chrono>
#include <array>

#include "vk_copy.h"
#include "vk_context.h"
#include "vk_images.h"

#include "VoxelRenderer_slang.h"

static uint32_t ComputeReductionSteps(uint32_t whole_size, uint32_t wg_size)
{
  uint32_t steps = 0;
  while (whole_size > 1)
  {
    steps++;
    whole_size = (whole_size + wg_size - 1) / wg_size;
  }
  return steps;
}

constexpr uint32_t KGEN_REDUCTION_LAST_STEP    = 16;
template<typename T> inline size_t ReduceAddInit(std::vector<T>& a_vec, size_t a_targetSize) { return a_vec.size(); } 
template<typename T> inline void   ReduceAddComplete(std::vector<T>& a_vec) { }

void VoxelRenderer_slang::UpdatePlainMembers(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine)
{
  const size_t maxAllowedSize = std::numeric_limits<uint32_t>::max();
  m_uboData.m_projInv = m_projInv;
  m_uboData.m_worldViewInv = m_worldViewInv;
  m_uboData.m_cuttingPlane = m_cuttingPlane;
  m_uboData.m_backgroundColor = m_backgroundColor;
  m_uboData.m_header = m_header;
  m_uboData.m_preset = m_preset;
  m_uboData.m_AAFrameNum = m_AAFrameNum;
  m_uboData.m_DAG_max_level_size = m_DAG_max_level_size;
  m_uboData.m_SVO_max_level_size = m_SVO_max_level_size;
  m_uboData.m_data_type = m_data_type;
  m_uboData.m_height = m_height;
  m_uboData.m_packedXY_height = m_packedXY_height;
  m_uboData.m_packedXY_width = m_packedXY_width;
  m_uboData.m_seed = m_seed;
  m_uboData.m_width = m_width;
  m_uboData.m_RotAddTable_size     = uint32_t( m_RotAddTable.size() );     assert( m_RotAddTable.size() < maxAllowedSize );
  m_uboData.m_RotAddTable_capacity = uint32_t( m_RotAddTable.capacity() ); assert( m_RotAddTable.capacity() < maxAllowedSize );
  m_uboData.m_SComNodes_size     = uint32_t( m_SComNodes.size() );     assert( m_SComNodes.size() < maxAllowedSize );
  m_uboData.m_SComNodes_capacity = uint32_t( m_SComNodes.capacity() ); assert( m_SComNodes.capacity() < maxAllowedSize );
  m_uboData.m_SVO_size     = uint32_t( m_SVO.size() );     assert( m_SVO.size() < maxAllowedSize );
  m_uboData.m_SVO_capacity = uint32_t( m_SVO.capacity() ); assert( m_SVO.capacity() < maxAllowedSize );
  m_uboData.m_SdfCompactOctreeRotModifiers_size     = uint32_t( m_SdfCompactOctreeRotModifiers.size() );     assert( m_SdfCompactOctreeRotModifiers.size() < maxAllowedSize );
  m_uboData.m_SdfCompactOctreeRotModifiers_capacity = uint32_t( m_SdfCompactOctreeRotModifiers.capacity() ); assert( m_SdfCompactOctreeRotModifiers.capacity() < maxAllowedSize );
  m_uboData.m_SdfDAGBlockIds_size     = uint32_t( m_SdfDAGBlockIds.size() );     assert( m_SdfDAGBlockIds.size() < maxAllowedSize );
  m_uboData.m_SdfDAGBlockIds_capacity = uint32_t( m_SdfDAGBlockIds.capacity() ); assert( m_SdfDAGBlockIds.capacity() < maxAllowedSize );
  m_uboData.m_SdfDAGChildEdges_size     = uint32_t( m_SdfDAGChildEdges.size() );     assert( m_SdfDAGChildEdges.size() < maxAllowedSize );
  m_uboData.m_SdfDAGChildEdges_capacity = uint32_t( m_SdfDAGChildEdges.capacity() ); assert( m_SdfDAGChildEdges.capacity() < maxAllowedSize );
  m_uboData.m_SdfDAGDataEdges_size     = uint32_t( m_SdfDAGDataEdges.size() );     assert( m_SdfDAGDataEdges.size() < maxAllowedSize );
  m_uboData.m_SdfDAGDataEdges_capacity = uint32_t( m_SdfDAGDataEdges.capacity() ); assert( m_SdfDAGDataEdges.capacity() < maxAllowedSize );
  m_uboData.m_SdfDAGDistances_size     = uint32_t( m_SdfDAGDistances.size() );     assert( m_SdfDAGDistances.size() < maxAllowedSize );
  m_uboData.m_SdfDAGDistances_capacity = uint32_t( m_SdfDAGDistances.capacity() ); assert( m_SdfDAGDistances.capacity() < maxAllowedSize );
  m_uboData.m_SdfDAGHeaders_size     = uint32_t( m_SdfDAGHeaders.size() );     assert( m_SdfDAGHeaders.size() < maxAllowedSize );
  m_uboData.m_SdfDAGHeaders_capacity = uint32_t( m_SdfDAGHeaders.capacity() ); assert( m_SdfDAGHeaders.capacity() < maxAllowedSize );
  m_uboData.m_SdfDAGNodes_size     = uint32_t( m_SdfDAGNodes.size() );     assert( m_SdfDAGNodes.size() < maxAllowedSize );
  m_uboData.m_SdfDAGNodes_capacity = uint32_t( m_SdfDAGNodes.capacity() ); assert( m_SdfDAGNodes.capacity() < maxAllowedSize );
  m_uboData.m_blocks_size     = uint32_t( m_blocks.size() );     assert( m_blocks.size() < maxAllowedSize );
  m_uboData.m_blocks_capacity = uint32_t( m_blocks.capacity() ); assert( m_blocks.capacity() < maxAllowedSize );
  m_uboData.m_colorBuffer_size     = uint32_t( m_colorBuffer.size() );     assert( m_colorBuffer.size() < maxAllowedSize );
  m_uboData.m_colorBuffer_capacity = uint32_t( m_colorBuffer.capacity() ); assert( m_colorBuffer.capacity() < maxAllowedSize );
  m_uboData.m_lights_size     = uint32_t( m_lights.size() );     assert( m_lights.size() < maxAllowedSize );
  m_uboData.m_lights_capacity = uint32_t( m_lights.capacity() ); assert( m_lights.capacity() < maxAllowedSize );
  m_uboData.m_packedXY_size     = uint32_t( m_packedXY.size() );     assert( m_packedXY.size() < maxAllowedSize );
  m_uboData.m_packedXY_capacity = uint32_t( m_packedXY.capacity() ); assert( m_packedXY.capacity() < maxAllowedSize );
  a_pCopyEngine->UpdateBuffer(m_classDataBuffer, 0, &m_uboData, sizeof(m_uboData));
}

void VoxelRenderer_slang::ReadPlainMembers(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine)
{
  a_pCopyEngine->ReadBuffer(m_classDataBuffer, 0, &m_uboData, sizeof(m_uboData));
  m_projInv = m_uboData.m_projInv;
  m_worldViewInv = m_uboData.m_worldViewInv;
  m_cuttingPlane = m_uboData.m_cuttingPlane;
  m_backgroundColor = m_uboData.m_backgroundColor;
  m_header = m_uboData.m_header;
  m_preset = m_uboData.m_preset;
  m_AAFrameNum = m_uboData.m_AAFrameNum;
  m_DAG_max_level_size = m_uboData.m_DAG_max_level_size;
  m_SVO_max_level_size = m_uboData.m_SVO_max_level_size;
  m_data_type = m_uboData.m_data_type;
  m_height = m_uboData.m_height;
  m_packedXY_height = m_uboData.m_packedXY_height;
  m_packedXY_width = m_uboData.m_packedXY_width;
  m_seed = m_uboData.m_seed;
  m_width = m_uboData.m_width;
  m_RotAddTable.resize(m_uboData.m_RotAddTable_size);
  m_SComNodes.resize(m_uboData.m_SComNodes_size);
  m_SVO.resize(m_uboData.m_SVO_size);
  m_SdfCompactOctreeRotModifiers.resize(m_uboData.m_SdfCompactOctreeRotModifiers_size);
  m_SdfDAGBlockIds.resize(m_uboData.m_SdfDAGBlockIds_size);
  m_SdfDAGChildEdges.resize(m_uboData.m_SdfDAGChildEdges_size);
  m_SdfDAGDataEdges.resize(m_uboData.m_SdfDAGDataEdges_size);
  m_SdfDAGDistances.resize(m_uboData.m_SdfDAGDistances_size);
  m_SdfDAGHeaders.resize(m_uboData.m_SdfDAGHeaders_size);
  m_SdfDAGNodes.resize(m_uboData.m_SdfDAGNodes_size);
  m_blocks.resize(m_uboData.m_blocks_size);
  m_colorBuffer.resize(m_uboData.m_colorBuffer_size);
  m_lights.resize(m_uboData.m_lights_size);
  m_packedXY.resize(m_uboData.m_packedXY_size);
}

void VoxelRenderer_slang::UpdateVectorMembers(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine)
{
  if(m_RotAddTable.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_RotAddTableBuffer, 0, m_RotAddTable.data(), m_RotAddTable.size()*sizeof(unsigned int) );
  if(m_SComNodes.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_SComNodesBuffer, 0, m_SComNodes.data(), m_SComNodes.size()*sizeof(unsigned int) );
  if(m_SVO.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_SVOBuffer, 0, m_SVO.data(), m_SVO.size()*sizeof(unsigned int) );
  if(m_SdfCompactOctreeRotModifiers.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_SdfCompactOctreeRotModifiersBuffer, 0, m_SdfCompactOctreeRotModifiers.data(), m_SdfCompactOctreeRotModifiers.size()*sizeof(int4) );
  if(m_SdfDAGBlockIds.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_SdfDAGBlockIdsBuffer, 0, m_SdfDAGBlockIds.data(), m_SdfDAGBlockIds.size()*sizeof(int) );
  if(m_SdfDAGChildEdges.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_SdfDAGChildEdgesBuffer, 0, m_SdfDAGChildEdges.data(), m_SdfDAGChildEdges.size()*sizeof(SdfDAGChildEdge) );
  if(m_SdfDAGDataEdges.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_SdfDAGDataEdgesBuffer, 0, m_SdfDAGDataEdges.data(), m_SdfDAGDataEdges.size()*sizeof(SdfDAGDataEdge) );
  if(m_SdfDAGDistances.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_SdfDAGDistancesBuffer, 0, m_SdfDAGDistances.data(), m_SdfDAGDistances.size()*sizeof(float) );
  if(m_SdfDAGHeaders.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_SdfDAGHeadersBuffer, 0, m_SdfDAGHeaders.data(), m_SdfDAGHeaders.size()*sizeof(SdfDAGHeader) );
  if(m_SdfDAGNodes.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_SdfDAGNodesBuffer, 0, m_SdfDAGNodes.data(), m_SdfDAGNodes.size()*sizeof(SdfDAGNode) );
  if(m_blocks.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_blocksBuffer, 0, m_blocks.data(), m_blocks.size()*sizeof(BlockMetadata) );
  if(m_colorBuffer.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_colorBufferBuffer, 0, m_colorBuffer.data(), m_colorBuffer.size()*sizeof(float4) );
  if(m_lights.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_lightsBuffer, 0, m_lights.data(), m_lights.size()*sizeof(Light) );
  if(m_packedXY.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_packedXYBuffer, 0, m_packedXY.data(), m_packedXY.size()*sizeof(unsigned int) );
}

void VoxelRenderer_slang::Update_m_lights(size_t a_first, size_t a_size)
{
  if(m_lights.size() != 0 && m_pLastCopyHelper != nullptr)
    m_pLastCopyHelper->UpdateBuffer(m_vdata.m_lightsBuffer, a_first*sizeof(struct Light), m_lights.data() + a_first, a_size*sizeof(struct Light) );
}

void VoxelRenderer_slang::UpdateTextureMembers(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine)
{
  for(int i=0;i<m_vdata.m_texturesArrayTexture.size();i++)
    a_pCopyEngine->UpdateImage(m_vdata.m_texturesArrayTexture[i], m_textures[i]->data(), m_textures[i]->width(), m_textures[i]->height(), m_textures[i]->bpp(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  std::array<VkImageMemoryBarrier, 0> barriers;


  VkCommandBuffer cmdBuff       = a_pCopyEngine->CmdBuffer();
  VkQueue         transferQueue = a_pCopyEngine->TransferQueue();

  vkResetCommandBuffer(cmdBuff, 0);
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  if (vkBeginCommandBuffer(cmdBuff, &beginInfo) != VK_SUCCESS)
    throw std::runtime_error("VoxelRenderer_slang::UpdateTextureMembers: failed to begin command buffer!");
  vkCmdPipelineBarrier(cmdBuff,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,nullptr,0,nullptr,uint32_t(barriers.size()),barriers.data());
  vkEndCommandBuffer(cmdBuff);

  vk_utils::executeCommandBufferNow(cmdBuff, transferQueue, m_device);
}

void VoxelRenderer_slang::PackXYCmd(uint32_t w, uint32_t h)
{
  uint32_t blockSizeX = 32;
  uint32_t blockSizeY = 8;
  uint32_t blockSizeZ = 1;

  struct KernelArgsPC
  {
    uint32_t m_sizeX;
    uint32_t m_sizeY;
    uint32_t m_sizeZ;
    uint32_t m_tFlags;
  } pcData;

  uint32_t sizeX  = uint32_t(h);
  uint32_t sizeY  = uint32_t(w);
  uint32_t sizeZ  = uint32_t(1);

  pcData.m_sizeX  = h;
  pcData.m_sizeY  = w;
  pcData.m_sizeZ  = 1;
  pcData.m_tFlags = m_currThreadFlags;
  vkCmdPushConstants(m_currCmdBuffer, PackXYLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(KernelArgsPC), &pcData);
  
  vkCmdBindPipeline(m_currCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, PackXYPipeline);
  vkCmdDispatch    (m_currCmdBuffer, (sizeX + blockSizeX - 1) / blockSizeX, (sizeY + blockSizeY - 1) / blockSizeY, (sizeZ + blockSizeZ - 1) / blockSizeZ);
}

void VoxelRenderer_slang::AverageColorCmd(uint32_t count, uint32_t sample_count, LiteMath::float4* out_color)
{
  uint32_t blockSizeX = 256;
  uint32_t blockSizeY = 1;
  uint32_t blockSizeZ = 1;

  struct KernelArgsPC
  {
    uint32_t m_sample_count;
    uint32_t m_sizeX;
    uint32_t m_sizeY;
    uint32_t m_sizeZ;
    uint32_t m_tFlags;
  } pcData;

  uint32_t sizeX  = uint32_t(count);
  uint32_t sizeY  = uint32_t(1);
  uint32_t sizeZ  = uint32_t(1);

  pcData.m_sizeX  = count;
  pcData.m_sizeY  = 1;
  pcData.m_sizeZ  = 1;
  pcData.m_tFlags = m_currThreadFlags;
  pcData.m_sample_count = sample_count;
  vkCmdPushConstants(m_currCmdBuffer, AverageColorLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(KernelArgsPC), &pcData);
  
  vkCmdBindPipeline(m_currCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, AverageColorPipeline);
  vkCmdDispatch    (m_currCmdBuffer, (sizeX + blockSizeX - 1) / blockSizeX, (sizeY + blockSizeY - 1) / blockSizeY, (sizeZ + blockSizeZ - 1) / blockSizeZ);
}

void VoxelRenderer_slang::IntersectSVOFastCmd(uint32_t count, uint32_t sample_id, float4* out_color)
{
  uint32_t blockSizeX = 256;
  uint32_t blockSizeY = 1;
  uint32_t blockSizeZ = 1;

  struct KernelArgsPC
  {
    uint32_t m_sample_id;
    uint32_t m_sizeX;
    uint32_t m_sizeY;
    uint32_t m_sizeZ;
    uint32_t m_tFlags;
  } pcData;

  uint32_t sizeX  = uint32_t(count);
  uint32_t sizeY  = uint32_t(1);
  uint32_t sizeZ  = uint32_t(1);

  pcData.m_sizeX  = count;
  pcData.m_sizeY  = 1;
  pcData.m_sizeZ  = 1;
  pcData.m_tFlags = m_currThreadFlags;
  pcData.m_sample_id = sample_id;
  vkCmdPushConstants(m_currCmdBuffer, IntersectSVOFastLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(KernelArgsPC), &pcData);
  
  vkCmdBindPipeline(m_currCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, IntersectSVOFastPipeline);
  vkCmdDispatch    (m_currCmdBuffer, (sizeX + blockSizeX - 1) / blockSizeX, (sizeY + blockSizeY - 1) / blockSizeY, (sizeZ + blockSizeZ - 1) / blockSizeZ);
}

void VoxelRenderer_slang::IntersectDAGCmd(uint32_t count, uint32_t sample_id, float4* out_color)
{
  uint32_t blockSizeX = 256;
  uint32_t blockSizeY = 1;
  uint32_t blockSizeZ = 1;

  struct KernelArgsPC
  {
    uint32_t m_sample_id;
    uint32_t m_sizeX;
    uint32_t m_sizeY;
    uint32_t m_sizeZ;
    uint32_t m_tFlags;
  } pcData;

  uint32_t sizeX  = uint32_t(count);
  uint32_t sizeY  = uint32_t(1);
  uint32_t sizeZ  = uint32_t(1);

  pcData.m_sizeX  = count;
  pcData.m_sizeY  = 1;
  pcData.m_sizeZ  = 1;
  pcData.m_tFlags = m_currThreadFlags;
  pcData.m_sample_id = sample_id;
  vkCmdPushConstants(m_currCmdBuffer, IntersectDAGLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(KernelArgsPC), &pcData);
  
  vkCmdBindPipeline(m_currCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, IntersectDAGPipeline);
  vkCmdDispatch    (m_currCmdBuffer, (sizeX + blockSizeX - 1) / blockSizeX, (sizeY + blockSizeY - 1) / blockSizeY, (sizeZ + blockSizeZ - 1) / blockSizeZ);
}

void VoxelRenderer_slang::TonemapCmd(uint32_t count, uint32_t sample_count, uint32_t* out_color)
{
  uint32_t blockSizeX = 256;
  uint32_t blockSizeY = 1;
  uint32_t blockSizeZ = 1;

  struct KernelArgsPC
  {
    uint32_t m_sample_count;
    uint32_t m_sizeX;
    uint32_t m_sizeY;
    uint32_t m_sizeZ;
    uint32_t m_tFlags;
  } pcData;

  uint32_t sizeX  = uint32_t(count);
  uint32_t sizeY  = uint32_t(1);
  uint32_t sizeZ  = uint32_t(1);

  pcData.m_sizeX  = count;
  pcData.m_sizeY  = 1;
  pcData.m_sizeZ  = 1;
  pcData.m_tFlags = m_currThreadFlags;
  pcData.m_sample_count = sample_count;
  vkCmdPushConstants(m_currCmdBuffer, TonemapLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(KernelArgsPC), &pcData);
  
  vkCmdBindPipeline(m_currCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, TonemapPipeline);
  vkCmdDispatch    (m_currCmdBuffer, (sizeX + blockSizeX - 1) / blockSizeX, (sizeY + blockSizeY - 1) / blockSizeY, (sizeZ + blockSizeZ - 1) / blockSizeZ);
}

void VoxelRenderer_slang::IntersectSComCmd(uint32_t count, uint32_t sample_id, float4* out_color)
{
  uint32_t blockSizeX = 256;
  uint32_t blockSizeY = 1;
  uint32_t blockSizeZ = 1;

  struct KernelArgsPC
  {
    uint32_t m_sample_id;
    uint32_t m_sizeX;
    uint32_t m_sizeY;
    uint32_t m_sizeZ;
    uint32_t m_tFlags;
  } pcData;

  uint32_t sizeX  = uint32_t(count);
  uint32_t sizeY  = uint32_t(1);
  uint32_t sizeZ  = uint32_t(1);

  pcData.m_sizeX  = count;
  pcData.m_sizeY  = 1;
  pcData.m_sizeZ  = 1;
  pcData.m_tFlags = m_currThreadFlags;
  pcData.m_sample_id = sample_id;
  vkCmdPushConstants(m_currCmdBuffer, IntersectSComLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(KernelArgsPC), &pcData);
  
  vkCmdBindPipeline(m_currCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, IntersectSComPipeline);
  vkCmdDispatch    (m_currCmdBuffer, (sizeX + blockSizeX - 1) / blockSizeX, (sizeY + blockSizeY - 1) / blockSizeY, (sizeZ + blockSizeZ - 1) / blockSizeZ);
}

void VoxelRenderer_slang::IntersectSVOCmd(uint32_t count, uint32_t sample_id, float4* out_color)
{
  uint32_t blockSizeX = 256;
  uint32_t blockSizeY = 1;
  uint32_t blockSizeZ = 1;

  struct KernelArgsPC
  {
    uint32_t m_sample_id;
    uint32_t m_sizeX;
    uint32_t m_sizeY;
    uint32_t m_sizeZ;
    uint32_t m_tFlags;
  } pcData;

  uint32_t sizeX  = uint32_t(count);
  uint32_t sizeY  = uint32_t(1);
  uint32_t sizeZ  = uint32_t(1);

  pcData.m_sizeX  = count;
  pcData.m_sizeY  = 1;
  pcData.m_sizeZ  = 1;
  pcData.m_tFlags = m_currThreadFlags;
  pcData.m_sample_id = sample_id;
  vkCmdPushConstants(m_currCmdBuffer, IntersectSVOLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(KernelArgsPC), &pcData);
  
  vkCmdBindPipeline(m_currCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, IntersectSVOPipeline);
  vkCmdDispatch    (m_currCmdBuffer, (sizeX + blockSizeX - 1) / blockSizeX, (sizeY + blockSizeY - 1) / blockSizeY, (sizeZ + blockSizeZ - 1) / blockSizeZ);
}


void VoxelRenderer_slang::copyKernelFloatCmd(uint32_t length)
{
  uint32_t blockSizeX = MEMCPY_BLOCK_SIZE;

  vkCmdBindPipeline(m_currCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, copyKernelFloatPipeline);
  vkCmdPushConstants(m_currCmdBuffer, copyKernelFloatLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &length);
  vkCmdDispatch(m_currCmdBuffer, (length + blockSizeX - 1) / blockSizeX, 1, 1);
}

void VoxelRenderer_slang::matMulTransposeCmd(uint32_t A_offset, uint32_t B_offset, uint32_t C_offset, uint32_t A_col_len, uint32_t B_col_len, uint32_t A_row_len)
{
  const uint32_t blockSizeX = 8;
  const uint32_t blockSizeY = 8;

  vkCmdBindPipeline(m_currCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, matMulTransposePipeline);
  struct KernelArgsPC
  {
    uint32_t m_A_row_len;
    uint32_t m_sizeX;
    uint32_t m_sizeY;
    uint32_t m_A_offset;
    uint32_t m_B_offset;
    uint32_t m_C_offset;
  } pcData;
  pcData.m_A_row_len = A_row_len;
  pcData.m_sizeX = B_col_len;
  pcData.m_sizeY = A_col_len;
  pcData.m_A_offset = A_offset;
  pcData.m_B_offset = B_offset;
  pcData.m_C_offset = C_offset;
  vkCmdPushConstants(m_currCmdBuffer, matMulTransposeLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pcData), &pcData);
  vkCmdDispatch(m_currCmdBuffer, (B_col_len + blockSizeX - 1) / blockSizeX, (A_col_len + blockSizeY - 1) / blockSizeY, 1);
}

VkBufferMemoryBarrier VoxelRenderer_slang::BarrierForClearFlags(VkBuffer a_buffer)
{
  VkBufferMemoryBarrier bar = {};
  bar.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bar.pNext               = NULL;
  bar.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
  bar.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
  bar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bar.buffer              = a_buffer;
  bar.offset              = 0;
  bar.size                = VK_WHOLE_SIZE;
  return bar;
}

VkBufferMemoryBarrier VoxelRenderer_slang::BarrierForSingleBuffer(VkBuffer a_buffer)
{
  VkBufferMemoryBarrier bar = {};
  bar.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bar.pNext               = NULL;
  bar.srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
  bar.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
  bar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bar.buffer              = a_buffer;
  bar.offset              = 0;
  bar.size                = VK_WHOLE_SIZE;
  return bar;
}

void VoxelRenderer_slang::BarriersForSeveralBuffers(VkBuffer* a_inBuffers, VkBufferMemoryBarrier* a_outBarriers, uint32_t a_buffersNum)
{
  for(uint32_t i=0; i<a_buffersNum;i++)
  {
    a_outBarriers[i].sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    a_outBarriers[i].pNext               = NULL;
    a_outBarriers[i].srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
    a_outBarriers[i].dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
    a_outBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    a_outBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    a_outBarriers[i].buffer              = a_inBuffers[i];
    a_outBarriers[i].offset              = 0;
    a_outBarriers[i].size                = VK_WHOLE_SIZE;
  }
}

void VoxelRenderer_slang::RenderFloatCmd(VkCommandBuffer a_commandBuffer, float4 *imageData, uint32_t width, uint32_t height, int passNum)
{
  VkPipelineStageFlagBits prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  m_currCmdBuffer = a_commandBuffer;
  VkMemoryBarrier memoryBarrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT };
    assert(m_width == width);
  assert(m_height == height);

  uint32_t samples = passNum*m_preset.spp;
  for (uint32_t sample = 0; sample < samples; sample++)
  {
    if (m_data_type == TYPE_SDF_DAG)
    {
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, IntersectDAGLayout, 0, 1, &m_allGeneratedDS[0], 0, nullptr);
  IntersectDAGCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
    }
    else if (m_data_type == TYPE_SCOM2)
    {
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, IntersectSComLayout, 0, 1, &m_allGeneratedDS[1], 0, nullptr);
  IntersectSComCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
    }
    else if (m_data_type == TYPE_SPARSE_VOXEL_OCTREE)
    {
      if (m_preset.octree_traversal_mode == OCTREE_TRAVERSAL_MODE_DEFAULT)
      {
        {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, IntersectSVOLayout, 0, 1, &m_allGeneratedDS[2], 0, nullptr);
  IntersectSVOCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      }
      else if (m_preset.octree_traversal_mode == OCTREE_TRAVERSAL_MODE_FAST)
      {
        {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, IntersectSVOFastLayout, 0, 1, &m_allGeneratedDS[2], 0, nullptr);
  IntersectSVOFastCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      }
    }
  }
  {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, AverageColorLayout, 0, 1, &m_allGeneratedDS[3], 0, nullptr);
  AverageColorCmd(m_width*m_height, samples, imageData);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};

}

void VoxelRenderer_slang::RenderCmd(VkCommandBuffer a_commandBuffer, uint32_t *imageData, uint32_t width, uint32_t height, int passNum)
{
  VkPipelineStageFlagBits prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  m_currCmdBuffer = a_commandBuffer;
  VkMemoryBarrier memoryBarrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT };
    assert(m_width == width);
  assert(m_height == height);

  uint32_t samples = passNum*m_preset.spp;
  for (uint32_t sample = 0; sample < samples; sample++)
  {
    if (m_data_type == TYPE_SDF_DAG)
    {
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, IntersectDAGLayout, 0, 1, &m_allGeneratedDS[4], 0, nullptr);
  IntersectDAGCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
    }
    else if (m_data_type == TYPE_SCOM2)
    {
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, IntersectSComLayout, 0, 1, &m_allGeneratedDS[5], 0, nullptr);
  IntersectSComCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
    }
    else if (m_data_type == TYPE_SPARSE_VOXEL_OCTREE)
    {
      if (m_preset.octree_traversal_mode == OCTREE_TRAVERSAL_MODE_DEFAULT)
      {
        {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, IntersectSVOLayout, 0, 1, &m_allGeneratedDS[6], 0, nullptr);
  IntersectSVOCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      }
      else if (m_preset.octree_traversal_mode == OCTREE_TRAVERSAL_MODE_FAST)
      {
        {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, IntersectSVOFastLayout, 0, 1, &m_allGeneratedDS[6], 0, nullptr);
  IntersectSVOFastCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      }
    }
  }
  {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, TonemapLayout, 0, 1, &m_allGeneratedDS[7], 0, nullptr);
  TonemapCmd(m_width*m_height, samples, imageData);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};

}

void VoxelRenderer_slang::ClearCmd(VkCommandBuffer a_commandBuffer, uint32_t width, uint32_t height)
{
  VkPipelineStageFlagBits prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  m_currCmdBuffer = a_commandBuffer;
  VkMemoryBarrier memoryBarrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT };
    {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, PackXYLayout, 0, 1, &m_allGeneratedDS[8], 0, nullptr);
  PackXYCmd(width, height);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};

}



void VoxelRenderer_slang::RenderFloat(float4 *imageData, uint32_t width, uint32_t height, int passNum)
{
  // (1) get global Vulkan context objects
  //
  VkInstance       instance       = m_ctx.instance;
  VkPhysicalDevice physicalDevice = m_ctx.physicalDevice;
  VkDevice         device         = m_ctx.device;
  VkCommandPool    commandPool    = m_ctx.commandPool;
  VkQueue          computeQueue   = m_ctx.computeQueue;
  VkQueue          transferQueue  = m_ctx.transferQueue;
  auto             pCopyHelper    = m_ctx.pCopyHelper;
  auto             pAllocatorSpec = m_ctx.pAllocatorSpecial;

  // (2) create GPU objects
  //
  auto outFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  if(RenderFloat_local.needToClearOutput)
    outFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  std::vector<VkBuffer> buffers;
  std::vector<VkImage>  images2;
  std::vector<vk_utils::VulkanImageMem*> images;
  auto beforeCreateObjects = std::chrono::high_resolution_clock::now();
  VkBuffer imageDataGPU = vk_utils::createBuffer(device, width*height*sizeof(float4 ), outFlags);
  buffers.push_back(imageDataGPU);

  VkDeviceMemory buffersMem = VK_NULL_HANDLE; // vk_utils::allocateAndBindWithPadding(device, physicalDevice, buffers);
  VkDeviceMemory imagesMem  = VK_NULL_HANDLE; // vk_utils::allocateAndBindWithPadding(device, physicalDevice, std::vector<VkBuffer>(), images);

  vk_utils::MemAllocInfo tempMemoryAllocInfo;
  tempMemoryAllocInfo.memUsage = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // TODO, select depending on device and sample/application (???)
  if(buffers.size() != 0)
    pAllocatorSpec->Allocate(tempMemoryAllocInfo, buffers);
  if(images.size() != 0)
  {
    pAllocatorSpec->Allocate(tempMemoryAllocInfo, images2);
    for(auto imgMem : images)
    {
      VkImageViewCreateInfo imageView{};
      imageView.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
      imageView.image    = imgMem->image;
      imageView.format   = imgMem->format;
      imageView.subresourceRange = {};
      imageView.subresourceRange.aspectMask     = imgMem->aspectMask;
      imageView.subresourceRange.baseMipLevel   = 0;
      imageView.subresourceRange.levelCount     = imgMem->mipLvls;
      imageView.subresourceRange.baseArrayLayer = 0;
      imageView.subresourceRange.layerCount     = 1;
      VK_CHECK_RESULT(vkCreateImageView(device, &imageView, nullptr, &imgMem->view));
    }
  }

  auto afterCreateObjects = std::chrono::high_resolution_clock::now();
  m_exTimeRenderFloat.msAPIOverhead = std::chrono::duration_cast<std::chrono::microseconds>(afterCreateObjects - beforeCreateObjects).count()/1000.f;

  auto afterCopy2 = std::chrono::high_resolution_clock::now(); // just declare it here, replace value later

  auto afterInitBuffers = std::chrono::high_resolution_clock::now();
  m_exTimeRenderFloat.msAPIOverhead += std::chrono::duration_cast<std::chrono::microseconds>(afterInitBuffers - afterCreateObjects).count()/1000.f;

  auto beforeSetInOut = std::chrono::high_resolution_clock::now();
  this->SetVulkanInOutFor_RenderFloat(imageDataGPU, 0, 0);

  // (3) copy input data to GPU
  //
  auto beforeCopy = std::chrono::high_resolution_clock::now();
  m_exTimeRenderFloat.msAPIOverhead += std::chrono::duration_cast<std::chrono::microseconds>(beforeCopy - beforeSetInOut).count()/1000.f;
  auto afterCopy = std::chrono::high_resolution_clock::now();
  m_exTimeRenderFloat.msCopyToGPU = std::chrono::duration_cast<std::chrono::microseconds>(afterCopy - beforeCopy).count()/1000.f;
  //
  m_exTimeRenderFloat.msExecuteOnGPU = 0;

  // (4) now execute algorithm on GPU
  //
  {
    VkCommandBuffer commandBuffer = vk_utils::createCommandBuffer(device, commandPool);
    VkCommandBufferBeginInfo beginCommandBufferInfo = {};
    beginCommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginCommandBufferInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginCommandBufferInfo);
    RenderFloatCmd(commandBuffer, imageData, width, height, passNum);
    vkEndCommandBuffer(commandBuffer);
    auto start = std::chrono::high_resolution_clock::now();
    vk_utils::executeCommandBufferNow(commandBuffer, computeQueue, device);
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    auto stop = std::chrono::high_resolution_clock::now();
    m_exTimeRenderFloat.msExecuteOnGPU += std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count()/1000.f;
  }

  // (5) copy output data to CPU
  //
  auto beforeCopy2 = std::chrono::high_resolution_clock::now();
  pCopyHelper->ReadBuffer(imageDataGPU, 0, imageData, width*height*sizeof(float4 ));
  //this->ReadPlainMembers(pCopyHelper);
  afterCopy2 = std::chrono::high_resolution_clock::now();
  m_exTimeRenderFloat.msCopyFromGPU = std::chrono::duration_cast<std::chrono::microseconds>(afterCopy2 - beforeCopy2).count()/1000.f;

  // (6) free resources
  //
  vkDestroyBuffer(device, imageDataGPU, nullptr);
  if(buffersMem != VK_NULL_HANDLE)
    vkFreeMemory(device, buffersMem, nullptr);
  if(imagesMem != VK_NULL_HANDLE)
    vkFreeMemory(device, imagesMem, nullptr);

  m_exTimeRenderFloat.msAPIOverhead += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - afterCopy2).count()/1000.f;
}

void VoxelRenderer_slang::Render(uint32_t *imageData, uint32_t width, uint32_t height, int passNum)
{
  // (1) get global Vulkan context objects
  //
  VkInstance       instance       = m_ctx.instance;
  VkPhysicalDevice physicalDevice = m_ctx.physicalDevice;
  VkDevice         device         = m_ctx.device;
  VkCommandPool    commandPool    = m_ctx.commandPool;
  VkQueue          computeQueue   = m_ctx.computeQueue;
  VkQueue          transferQueue  = m_ctx.transferQueue;
  auto             pCopyHelper    = m_ctx.pCopyHelper;
  auto             pAllocatorSpec = m_ctx.pAllocatorSpecial;

  // (2) create GPU objects
  //
  auto outFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  if(Render_local.needToClearOutput)
    outFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  std::vector<VkBuffer> buffers;
  std::vector<VkImage>  images2;
  std::vector<vk_utils::VulkanImageMem*> images;
  auto beforeCreateObjects = std::chrono::high_resolution_clock::now();
  VkBuffer imageDataGPU = vk_utils::createBuffer(device, width*height*sizeof(uint32_t ), outFlags);
  buffers.push_back(imageDataGPU);

  VkDeviceMemory buffersMem = VK_NULL_HANDLE; // vk_utils::allocateAndBindWithPadding(device, physicalDevice, buffers);
  VkDeviceMemory imagesMem  = VK_NULL_HANDLE; // vk_utils::allocateAndBindWithPadding(device, physicalDevice, std::vector<VkBuffer>(), images);

  vk_utils::MemAllocInfo tempMemoryAllocInfo;
  tempMemoryAllocInfo.memUsage = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // TODO, select depending on device and sample/application (???)
  if(buffers.size() != 0)
    pAllocatorSpec->Allocate(tempMemoryAllocInfo, buffers);
  if(images.size() != 0)
  {
    pAllocatorSpec->Allocate(tempMemoryAllocInfo, images2);
    for(auto imgMem : images)
    {
      VkImageViewCreateInfo imageView{};
      imageView.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
      imageView.image    = imgMem->image;
      imageView.format   = imgMem->format;
      imageView.subresourceRange = {};
      imageView.subresourceRange.aspectMask     = imgMem->aspectMask;
      imageView.subresourceRange.baseMipLevel   = 0;
      imageView.subresourceRange.levelCount     = imgMem->mipLvls;
      imageView.subresourceRange.baseArrayLayer = 0;
      imageView.subresourceRange.layerCount     = 1;
      VK_CHECK_RESULT(vkCreateImageView(device, &imageView, nullptr, &imgMem->view));
    }
  }

  auto afterCreateObjects = std::chrono::high_resolution_clock::now();
  m_exTimeRender.msAPIOverhead = std::chrono::duration_cast<std::chrono::microseconds>(afterCreateObjects - beforeCreateObjects).count()/1000.f;

  auto afterCopy2 = std::chrono::high_resolution_clock::now(); // just declare it here, replace value later

  auto afterInitBuffers = std::chrono::high_resolution_clock::now();
  m_exTimeRender.msAPIOverhead += std::chrono::duration_cast<std::chrono::microseconds>(afterInitBuffers - afterCreateObjects).count()/1000.f;

  auto beforeSetInOut = std::chrono::high_resolution_clock::now();
  this->SetVulkanInOutFor_Render(imageDataGPU, 0, 0);

  // (3) copy input data to GPU
  //
  auto beforeCopy = std::chrono::high_resolution_clock::now();
  m_exTimeRender.msAPIOverhead += std::chrono::duration_cast<std::chrono::microseconds>(beforeCopy - beforeSetInOut).count()/1000.f;
  auto afterCopy = std::chrono::high_resolution_clock::now();
  m_exTimeRender.msCopyToGPU = std::chrono::duration_cast<std::chrono::microseconds>(afterCopy - beforeCopy).count()/1000.f;
  //
  m_exTimeRender.msExecuteOnGPU = 0;

  // (4) now execute algorithm on GPU
  //
  {
    VkCommandBuffer commandBuffer = vk_utils::createCommandBuffer(device, commandPool);
    VkCommandBufferBeginInfo beginCommandBufferInfo = {};
    beginCommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginCommandBufferInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginCommandBufferInfo);
    RenderCmd(commandBuffer, imageData, width, height, passNum);
    vkEndCommandBuffer(commandBuffer);
    auto start = std::chrono::high_resolution_clock::now();
    vk_utils::executeCommandBufferNow(commandBuffer, computeQueue, device);
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    auto stop = std::chrono::high_resolution_clock::now();
    m_exTimeRender.msExecuteOnGPU += std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count()/1000.f;
  }

  // (5) copy output data to CPU
  //
  auto beforeCopy2 = std::chrono::high_resolution_clock::now();
  pCopyHelper->ReadBuffer(imageDataGPU, 0, imageData, width*height*sizeof(uint32_t ));
  //this->ReadPlainMembers(pCopyHelper);
  afterCopy2 = std::chrono::high_resolution_clock::now();
  m_exTimeRender.msCopyFromGPU = std::chrono::duration_cast<std::chrono::microseconds>(afterCopy2 - beforeCopy2).count()/1000.f;

  // (6) free resources
  //
  vkDestroyBuffer(device, imageDataGPU, nullptr);
  if(buffersMem != VK_NULL_HANDLE)
    vkFreeMemory(device, buffersMem, nullptr);
  if(imagesMem != VK_NULL_HANDLE)
    vkFreeMemory(device, imagesMem, nullptr);

  m_exTimeRender.msAPIOverhead += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - afterCopy2).count()/1000.f;
}

void VoxelRenderer_slang::Clear(uint32_t width, uint32_t height)
{
  // (1) get global Vulkan context objects
  //
  VkInstance       instance       = m_ctx.instance;
  VkPhysicalDevice physicalDevice = m_ctx.physicalDevice;
  VkDevice         device         = m_ctx.device;
  VkCommandPool    commandPool    = m_ctx.commandPool;
  VkQueue          computeQueue   = m_ctx.computeQueue;
  VkQueue          transferQueue  = m_ctx.transferQueue;
  auto             pCopyHelper    = m_ctx.pCopyHelper;
  auto             pAllocatorSpec = m_ctx.pAllocatorSpecial;

  // (2) create GPU objects
  //
  auto outFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  if(Clear_local.needToClearOutput)
    outFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  std::vector<VkBuffer> buffers;
  std::vector<VkImage>  images2;
  std::vector<vk_utils::VulkanImageMem*> images;
  auto beforeCreateObjects = std::chrono::high_resolution_clock::now();

  VkDeviceMemory buffersMem = VK_NULL_HANDLE; // vk_utils::allocateAndBindWithPadding(device, physicalDevice, buffers);
  VkDeviceMemory imagesMem  = VK_NULL_HANDLE; // vk_utils::allocateAndBindWithPadding(device, physicalDevice, std::vector<VkBuffer>(), images);

  vk_utils::MemAllocInfo tempMemoryAllocInfo;
  tempMemoryAllocInfo.memUsage = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // TODO, select depending on device and sample/application (???)
  if(buffers.size() != 0)
    pAllocatorSpec->Allocate(tempMemoryAllocInfo, buffers);
  if(images.size() != 0)
  {
    pAllocatorSpec->Allocate(tempMemoryAllocInfo, images2);
    for(auto imgMem : images)
    {
      VkImageViewCreateInfo imageView{};
      imageView.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
      imageView.image    = imgMem->image;
      imageView.format   = imgMem->format;
      imageView.subresourceRange = {};
      imageView.subresourceRange.aspectMask     = imgMem->aspectMask;
      imageView.subresourceRange.baseMipLevel   = 0;
      imageView.subresourceRange.levelCount     = imgMem->mipLvls;
      imageView.subresourceRange.baseArrayLayer = 0;
      imageView.subresourceRange.layerCount     = 1;
      VK_CHECK_RESULT(vkCreateImageView(device, &imageView, nullptr, &imgMem->view));
    }
  }

  auto afterCreateObjects = std::chrono::high_resolution_clock::now();
  m_exTimeClear.msAPIOverhead = std::chrono::duration_cast<std::chrono::microseconds>(afterCreateObjects - beforeCreateObjects).count()/1000.f;

  auto afterCopy2 = std::chrono::high_resolution_clock::now(); // just declare it here, replace value later

  auto afterInitBuffers = std::chrono::high_resolution_clock::now();
  m_exTimeClear.msAPIOverhead += std::chrono::duration_cast<std::chrono::microseconds>(afterInitBuffers - afterCreateObjects).count()/1000.f;

  auto beforeSetInOut = std::chrono::high_resolution_clock::now();
  this->SetVulkanInOutFor_Clear();

  // (3) copy input data to GPU
  //
  auto beforeCopy = std::chrono::high_resolution_clock::now();
  m_exTimeClear.msAPIOverhead += std::chrono::duration_cast<std::chrono::microseconds>(beforeCopy - beforeSetInOut).count()/1000.f;
  auto afterCopy = std::chrono::high_resolution_clock::now();
  m_exTimeClear.msCopyToGPU = std::chrono::duration_cast<std::chrono::microseconds>(afterCopy - beforeCopy).count()/1000.f;
  //
  m_exTimeClear.msExecuteOnGPU = 0;

  // (4) now execute algorithm on GPU
  //
  {
    VkCommandBuffer commandBuffer = vk_utils::createCommandBuffer(device, commandPool);
    VkCommandBufferBeginInfo beginCommandBufferInfo = {};
    beginCommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginCommandBufferInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginCommandBufferInfo);
    ClearCmd(commandBuffer, width, height);
    vkEndCommandBuffer(commandBuffer);
    auto start = std::chrono::high_resolution_clock::now();
    vk_utils::executeCommandBufferNow(commandBuffer, computeQueue, device);
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    auto stop = std::chrono::high_resolution_clock::now();
    m_exTimeClear.msExecuteOnGPU += std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count()/1000.f;
  }

  // (5) copy output data to CPU
  //
  auto beforeCopy2 = std::chrono::high_resolution_clock::now();
  //this->ReadPlainMembers(pCopyHelper);
  afterCopy2 = std::chrono::high_resolution_clock::now();
  m_exTimeClear.msCopyFromGPU = std::chrono::duration_cast<std::chrono::microseconds>(afterCopy2 - beforeCopy2).count()/1000.f;

  // (6) free resources
  //
  if(buffersMem != VK_NULL_HANDLE)
    vkFreeMemory(device, buffersMem, nullptr);
  if(imagesMem != VK_NULL_HANDLE)
    vkFreeMemory(device, imagesMem, nullptr);

  m_exTimeClear.msAPIOverhead += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - afterCopy2).count()/1000.f;
}



void VoxelRenderer_slang::GetExecutionTime(const char* a_funcName, float a_out[4])
{
  vk_utils::ExecTime res = {};
  if(std::string(a_funcName) == "RenderFloat" || std::string(a_funcName) == "RenderFloatBlock")
    res = m_exTimeRenderFloat;
  if(std::string(a_funcName) == "Render" || std::string(a_funcName) == "RenderBlock")
    res = m_exTimeRender;
  if(std::string(a_funcName) == "Clear" || std::string(a_funcName) == "ClearBlock")
    res = m_exTimeClear;
  a_out[0] = res.msExecuteOnGPU;
  a_out[1] = res.msCopyToGPU;
  a_out[2] = res.msCopyFromGPU;
  a_out[3] = res.msAPIOverhead;
}


