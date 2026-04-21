#include <vector>
#include <memory>
#include <limits>
#include <cassert>
#include <chrono>
#include <array>

#include "vk_copy.h"
#include "vk_context.h"
#include "vk_images.h"

#include "MultiRenderer_slang_comp.h"

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

void MultiRenderer_slang_comp::UpdatePlainMembers(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine)
{
  const size_t maxAllowedSize = std::numeric_limits<uint32_t>::max();
  auto pUnderlyingImpl = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
  m_uboData.m_projInv = m_projInv;
  m_uboData.m_worldViewInv = m_worldViewInv;
  m_uboData.m_cuttingPlane = m_cuttingPlane;
  m_uboData.m_pAccelStruct_m_cuttingPlane = pUnderlyingImpl->m_cuttingPlane;
  m_uboData.m_backgroundColor = m_backgroundColor;
  m_uboData.m_pAccelStruct_coctree_v3_header = pUnderlyingImpl->coctree_v3_header;
  m_uboData.m_pAccelStruct_m_preset = pUnderlyingImpl->m_preset;
  m_uboData.m_preset = m_preset;
  m_uboData.m_highlight = m_highlight;
  m_uboData.m_AAFrameNum = m_AAFrameNum;
  m_uboData.m_height = m_height;
  m_uboData.m_pAccelStruct_m_SdfDAGBrickTranspositionOffset = pUnderlyingImpl->m_SdfDAGBrickTranspositionOffset;
  m_uboData.m_pAccelStruct_m_SdfDAGTransformCount = pUnderlyingImpl->m_SdfDAGTransformCount;
  m_uboData.m_packedXY_height = m_packedXY_height;
  m_uboData.m_packedXY_width = m_packedXY_width;
  m_uboData.m_width = m_width;
  m_uboData.m_pAccelStruct_support_fast_scom_traversal = pUnderlyingImpl->support_fast_scom_traversal;
  m_uboData.m_sceneHasTransparency = m_sceneHasTransparency;
  m_uboData.m_allCTFPoints_size     = uint32_t( m_allCTFPoints.size() );     assert( m_allCTFPoints.size() < maxAllowedSize );
  m_uboData.m_allCTFPoints_capacity = uint32_t( m_allCTFPoints.capacity() ); assert( m_allCTFPoints.capacity() < maxAllowedSize );
  m_uboData.m_allCTFs_size     = uint32_t( m_allCTFs.size() );     assert( m_allCTFs.size() < maxAllowedSize );
  m_uboData.m_allCTFs_capacity = uint32_t( m_allCTFs.capacity() ); assert( m_allCTFs.capacity() < maxAllowedSize );
  m_uboData.m_allChannelOffsets_size     = uint32_t( m_allChannelOffsets.size() );     assert( m_allChannelOffsets.size() < maxAllowedSize );
  m_uboData.m_allChannelOffsets_capacity = uint32_t( m_allChannelOffsets.capacity() ); assert( m_allChannelOffsets.capacity() < maxAllowedSize );
  m_uboData.m_allChannelRenderInfo_size     = uint32_t( m_allChannelRenderInfo.size() );     assert( m_allChannelRenderInfo.size() < maxAllowedSize );
  m_uboData.m_allChannelRenderInfo_capacity = uint32_t( m_allChannelRenderInfo.capacity() ); assert( m_allChannelRenderInfo.capacity() < maxAllowedSize );
  m_uboData.m_allCompressedChannels_size     = uint32_t( m_allCompressedChannels.size() );     assert( m_allCompressedChannels.size() < maxAllowedSize );
  m_uboData.m_allCompressedChannels_capacity = uint32_t( m_allCompressedChannels.capacity() ); assert( m_allCompressedChannels.capacity() < maxAllowedSize );
  m_uboData.m_allFloatChannels_size     = uint32_t( m_allFloatChannels.size() );     assert( m_allFloatChannels.size() < maxAllowedSize );
  m_uboData.m_allFloatChannels_capacity = uint32_t( m_allFloatChannels.capacity() ); assert( m_allFloatChannels.capacity() < maxAllowedSize );
  m_uboData.m_allIntChannels_size     = uint32_t( m_allIntChannels.size() );     assert( m_allIntChannels.size() < maxAllowedSize );
  m_uboData.m_allIntChannels_capacity = uint32_t( m_allIntChannels.capacity() ); assert( m_allIntChannels.capacity() < maxAllowedSize );
  m_uboData.m_colorBuffer_size     = uint32_t( m_colorBuffer.size() );     assert( m_colorBuffer.size() < maxAllowedSize );
  m_uboData.m_colorBuffer_capacity = uint32_t( m_colorBuffer.capacity() ); assert( m_colorBuffer.capacity() < maxAllowedSize );
  m_uboData.m_gBuffer_size     = uint32_t( m_gBuffer.size() );     assert( m_gBuffer.size() < maxAllowedSize );
  m_uboData.m_gBuffer_capacity = uint32_t( m_gBuffer.capacity() ); assert( m_gBuffer.capacity() < maxAllowedSize );
  m_uboData.m_geomOffsets_size     = uint32_t( m_geomOffsets.size() );     assert( m_geomOffsets.size() < maxAllowedSize );
  m_uboData.m_geomOffsets_capacity = uint32_t( m_geomOffsets.capacity() ); assert( m_geomOffsets.capacity() < maxAllowedSize );
  m_uboData.m_indices_size     = uint32_t( m_indices.size() );     assert( m_indices.size() < maxAllowedSize );
  m_uboData.m_indices_capacity = uint32_t( m_indices.capacity() ); assert( m_indices.capacity() < maxAllowedSize );
  m_uboData.m_instanceTransformInvTransposed_size     = uint32_t( m_instanceTransformInvTransposed.size() );     assert( m_instanceTransformInvTransposed.size() < maxAllowedSize );
  m_uboData.m_instanceTransformInvTransposed_capacity = uint32_t( m_instanceTransformInvTransposed.capacity() ); assert( m_instanceTransformInvTransposed.capacity() < maxAllowedSize );
  m_uboData.m_lights_size     = uint32_t( m_lights.size() );     assert( m_lights.size() < maxAllowedSize );
  m_uboData.m_lights_capacity = uint32_t( m_lights.capacity() ); assert( m_lights.capacity() < maxAllowedSize );
  m_uboData.m_matIdOffsets_size     = uint32_t( m_matIdOffsets.size() );     assert( m_matIdOffsets.size() < maxAllowedSize );
  m_uboData.m_matIdOffsets_capacity = uint32_t( m_matIdOffsets.capacity() ); assert( m_matIdOffsets.capacity() < maxAllowedSize );
  m_uboData.m_matIdbyPrimId_size     = uint32_t( m_matIdbyPrimId.size() );     assert( m_matIdbyPrimId.size() < maxAllowedSize );
  m_uboData.m_matIdbyPrimId_capacity = uint32_t( m_matIdbyPrimId.capacity() ); assert( m_matIdbyPrimId.capacity() < maxAllowedSize );
  m_uboData.m_materials_size     = uint32_t( m_materials.size() );     assert( m_materials.size() < maxAllowedSize );
  m_uboData.m_materials_capacity = uint32_t( m_materials.capacity() ); assert( m_materials.capacity() < maxAllowedSize );
  m_uboData.m_normals_size     = uint32_t( m_normals.size() );     assert( m_normals.size() < maxAllowedSize );
  m_uboData.m_normals_capacity = uint32_t( m_normals.capacity() ); assert( m_normals.capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_GraphicsPrimHeaders_size     = uint32_t( m_pAccelStruct_m_GraphicsPrimHeaders->size() );     assert( m_pAccelStruct_m_GraphicsPrimHeaders->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_GraphicsPrimHeaders_capacity = uint32_t( m_pAccelStruct_m_GraphicsPrimHeaders->capacity() ); assert( m_pAccelStruct_m_GraphicsPrimHeaders->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_GraphicsPrimPoints_size     = uint32_t( m_pAccelStruct_m_GraphicsPrimPoints->size() );     assert( m_pAccelStruct_m_GraphicsPrimPoints->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_GraphicsPrimPoints_capacity = uint32_t( m_pAccelStruct_m_GraphicsPrimPoints->capacity() ); assert( m_pAccelStruct_m_GraphicsPrimPoints->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_RotAddTable_size     = uint32_t( m_pAccelStruct_m_RotAddTable->size() );     assert( m_pAccelStruct_m_RotAddTable->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_RotAddTable_capacity = uint32_t( m_pAccelStruct_m_RotAddTable->capacity() ); assert( m_pAccelStruct_m_RotAddTable->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SCom2Data_size     = uint32_t( m_pAccelStruct_m_SCom2Data->size() );     assert( m_pAccelStruct_m_SCom2Data->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SCom2Data_capacity = uint32_t( m_pAccelStruct_m_SCom2Data->capacity() ); assert( m_pAccelStruct_m_SCom2Data->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SCom2Headers_size     = uint32_t( m_pAccelStruct_m_SCom2Headers->size() );     assert( m_pAccelStruct_m_SCom2Headers->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SCom2Headers_capacity = uint32_t( m_pAccelStruct_m_SCom2Headers->capacity() ); assert( m_pAccelStruct_m_SCom2Headers->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfCompactOctreeRotModifiers_size     = uint32_t( m_pAccelStruct_m_SdfCompactOctreeRotModifiers->size() );     assert( m_pAccelStruct_m_SdfCompactOctreeRotModifiers->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfCompactOctreeRotModifiers_capacity = uint32_t( m_pAccelStruct_m_SdfCompactOctreeRotModifiers->capacity() ); assert( m_pAccelStruct_m_SdfCompactOctreeRotModifiers->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfCompactOctreeRotTransforms_size     = uint32_t( m_pAccelStruct_m_SdfCompactOctreeRotTransforms->size() );     assert( m_pAccelStruct_m_SdfCompactOctreeRotTransforms->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfCompactOctreeRotTransforms_capacity = uint32_t( m_pAccelStruct_m_SdfCompactOctreeRotTransforms->capacity() ); assert( m_pAccelStruct_m_SdfCompactOctreeRotTransforms->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfCompactOctreeV3Data_size     = uint32_t( m_pAccelStruct_m_SdfCompactOctreeV3Data->size() );     assert( m_pAccelStruct_m_SdfCompactOctreeV3Data->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfCompactOctreeV3Data_capacity = uint32_t( m_pAccelStruct_m_SdfCompactOctreeV3Data->capacity() ); assert( m_pAccelStruct_m_SdfCompactOctreeV3Data->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfDAGChildEdges_size     = uint32_t( m_pAccelStruct_m_SdfDAGChildEdges->size() );     assert( m_pAccelStruct_m_SdfDAGChildEdges->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfDAGChildEdges_capacity = uint32_t( m_pAccelStruct_m_SdfDAGChildEdges->capacity() ); assert( m_pAccelStruct_m_SdfDAGChildEdges->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfDAGDataEdges_size     = uint32_t( m_pAccelStruct_m_SdfDAGDataEdges->size() );     assert( m_pAccelStruct_m_SdfDAGDataEdges->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfDAGDataEdges_capacity = uint32_t( m_pAccelStruct_m_SdfDAGDataEdges->capacity() ); assert( m_pAccelStruct_m_SdfDAGDataEdges->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfDAGDistances_size     = uint32_t( m_pAccelStruct_m_SdfDAGDistances->size() );     assert( m_pAccelStruct_m_SdfDAGDistances->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfDAGDistances_capacity = uint32_t( m_pAccelStruct_m_SdfDAGDistances->capacity() ); assert( m_pAccelStruct_m_SdfDAGDistances->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfDAGHeaders_size     = uint32_t( m_pAccelStruct_m_SdfDAGHeaders->size() );     assert( m_pAccelStruct_m_SdfDAGHeaders->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfDAGHeaders_capacity = uint32_t( m_pAccelStruct_m_SdfDAGHeaders->capacity() ); assert( m_pAccelStruct_m_SdfDAGHeaders->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfDAGNodes_size     = uint32_t( m_pAccelStruct_m_SdfDAGNodes->size() );     assert( m_pAccelStruct_m_SdfDAGNodes->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfDAGNodes_capacity = uint32_t( m_pAccelStruct_m_SdfDAGNodes->capacity() ); assert( m_pAccelStruct_m_SdfDAGNodes->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfDAGTranspositions_size     = uint32_t( m_pAccelStruct_m_SdfDAGTranspositions->size() );     assert( m_pAccelStruct_m_SdfDAGTranspositions->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfDAGTranspositions_capacity = uint32_t( m_pAccelStruct_m_SdfDAGTranspositions->capacity() ); assert( m_pAccelStruct_m_SdfDAGTranspositions->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfFrameOctreeNodes_size     = uint32_t( m_pAccelStruct_m_SdfFrameOctreeNodes->size() );     assert( m_pAccelStruct_m_SdfFrameOctreeNodes->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfFrameOctreeNodes_capacity = uint32_t( m_pAccelStruct_m_SdfFrameOctreeNodes->capacity() ); assert( m_pAccelStruct_m_SdfFrameOctreeNodes->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfFrameOctreeRoots_size     = uint32_t( m_pAccelStruct_m_SdfFrameOctreeRoots->size() );     assert( m_pAccelStruct_m_SdfFrameOctreeRoots->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfFrameOctreeRoots_capacity = uint32_t( m_pAccelStruct_m_SdfFrameOctreeRoots->capacity() ); assert( m_pAccelStruct_m_SdfFrameOctreeRoots->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfFrameOctreeTexNodes_size     = uint32_t( m_pAccelStruct_m_SdfFrameOctreeTexNodes->size() );     assert( m_pAccelStruct_m_SdfFrameOctreeTexNodes->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfFrameOctreeTexNodes_capacity = uint32_t( m_pAccelStruct_m_SdfFrameOctreeTexNodes->capacity() ); assert( m_pAccelStruct_m_SdfFrameOctreeTexNodes->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfFrameOctreeTexRoots_size     = uint32_t( m_pAccelStruct_m_SdfFrameOctreeTexRoots->size() );     assert( m_pAccelStruct_m_SdfFrameOctreeTexRoots->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfFrameOctreeTexRoots_capacity = uint32_t( m_pAccelStruct_m_SdfFrameOctreeTexRoots->capacity() ); assert( m_pAccelStruct_m_SdfFrameOctreeTexRoots->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfGridData_size     = uint32_t( m_pAccelStruct_m_SdfGridData->size() );     assert( m_pAccelStruct_m_SdfGridData->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfGridData_capacity = uint32_t( m_pAccelStruct_m_SdfGridData->capacity() ); assert( m_pAccelStruct_m_SdfGridData->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfGridOffsets_size     = uint32_t( m_pAccelStruct_m_SdfGridOffsets->size() );     assert( m_pAccelStruct_m_SdfGridOffsets->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfGridOffsets_capacity = uint32_t( m_pAccelStruct_m_SdfGridOffsets->capacity() ); assert( m_pAccelStruct_m_SdfGridOffsets->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfGridSizes_size     = uint32_t( m_pAccelStruct_m_SdfGridSizes->size() );     assert( m_pAccelStruct_m_SdfGridSizes->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfGridSizes_capacity = uint32_t( m_pAccelStruct_m_SdfGridSizes->capacity() ); assert( m_pAccelStruct_m_SdfGridSizes->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfMultiOctreeNodes_size     = uint32_t( m_pAccelStruct_m_SdfMultiOctreeNodes->size() );     assert( m_pAccelStruct_m_SdfMultiOctreeNodes->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfMultiOctreeNodes_capacity = uint32_t( m_pAccelStruct_m_SdfMultiOctreeNodes->capacity() ); assert( m_pAccelStruct_m_SdfMultiOctreeNodes->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfMultiOctreeValues_size     = uint32_t( m_pAccelStruct_m_SdfMultiOctreeValues->size() );     assert( m_pAccelStruct_m_SdfMultiOctreeValues->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfMultiOctreeValues_capacity = uint32_t( m_pAccelStruct_m_SdfMultiOctreeValues->capacity() ); assert( m_pAccelStruct_m_SdfMultiOctreeValues->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfSBSData_size     = uint32_t( m_pAccelStruct_m_SdfSBSData->size() );     assert( m_pAccelStruct_m_SdfSBSData->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfSBSData_capacity = uint32_t( m_pAccelStruct_m_SdfSBSData->capacity() ); assert( m_pAccelStruct_m_SdfSBSData->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfSBSDataF_size     = uint32_t( m_pAccelStruct_m_SdfSBSDataF->size() );     assert( m_pAccelStruct_m_SdfSBSDataF->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfSBSDataF_capacity = uint32_t( m_pAccelStruct_m_SdfSBSDataF->capacity() ); assert( m_pAccelStruct_m_SdfSBSDataF->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfSBSHeaders_size     = uint32_t( m_pAccelStruct_m_SdfSBSHeaders->size() );     assert( m_pAccelStruct_m_SdfSBSHeaders->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfSBSHeaders_capacity = uint32_t( m_pAccelStruct_m_SdfSBSHeaders->capacity() ); assert( m_pAccelStruct_m_SdfSBSHeaders->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfSBSNodes_size     = uint32_t( m_pAccelStruct_m_SdfSBSNodes->size() );     assert( m_pAccelStruct_m_SdfSBSNodes->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfSBSNodes_capacity = uint32_t( m_pAccelStruct_m_SdfSBSNodes->capacity() ); assert( m_pAccelStruct_m_SdfSBSNodes->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfSBSRoots_size     = uint32_t( m_pAccelStruct_m_SdfSBSRoots->size() );     assert( m_pAccelStruct_m_SdfSBSRoots->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfSBSRoots_capacity = uint32_t( m_pAccelStruct_m_SdfSBSRoots->capacity() ); assert( m_pAccelStruct_m_SdfSBSRoots->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfSVSNodes_size     = uint32_t( m_pAccelStruct_m_SdfSVSNodes->size() );     assert( m_pAccelStruct_m_SdfSVSNodes->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfSVSNodes_capacity = uint32_t( m_pAccelStruct_m_SdfSVSNodes->capacity() ); assert( m_pAccelStruct_m_SdfSVSNodes->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfSVSRoots_size     = uint32_t( m_pAccelStruct_m_SdfSVSRoots->size() );     assert( m_pAccelStruct_m_SdfSVSRoots->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_SdfSVSRoots_capacity = uint32_t( m_pAccelStruct_m_SdfSVSRoots->capacity() ); assert( m_pAccelStruct_m_SdfSVSRoots->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_allNodePairs_size     = uint32_t( m_pAccelStruct_m_allNodePairs->size() );     assert( m_pAccelStruct_m_allNodePairs->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_allNodePairs_capacity = uint32_t( m_pAccelStruct_m_allNodePairs->capacity() ); assert( m_pAccelStruct_m_allNodePairs->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_geomData_size     = uint32_t( m_pAccelStruct_m_geomData->size() );     assert( m_pAccelStruct_m_geomData->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_geomData_capacity = uint32_t( m_pAccelStruct_m_geomData->capacity() ); assert( m_pAccelStruct_m_geomData->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_geomTags_size     = uint32_t( m_pAccelStruct_m_geomTags->size() );     assert( m_pAccelStruct_m_geomTags->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_geomTags_capacity = uint32_t( m_pAccelStruct_m_geomTags->capacity() ); assert( m_pAccelStruct_m_geomTags->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_indices_size     = uint32_t( m_pAccelStruct_m_indices->size() );     assert( m_pAccelStruct_m_indices->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_indices_capacity = uint32_t( m_pAccelStruct_m_indices->capacity() ); assert( m_pAccelStruct_m_indices->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_instanceData_size     = uint32_t( m_pAccelStruct_m_instanceData->size() );     assert( m_pAccelStruct_m_instanceData->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_instanceData_capacity = uint32_t( m_pAccelStruct_m_instanceData->capacity() ); assert( m_pAccelStruct_m_instanceData->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_nodesTLAS_size     = uint32_t( m_pAccelStruct_m_nodesTLAS->size() );     assert( m_pAccelStruct_m_nodesTLAS->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_nodesTLAS_capacity = uint32_t( m_pAccelStruct_m_nodesTLAS->capacity() ); assert( m_pAccelStruct_m_nodesTLAS->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_origNodes_size     = uint32_t( m_pAccelStruct_m_origNodes->size() );     assert( m_pAccelStruct_m_origNodes->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_origNodes_capacity = uint32_t( m_pAccelStruct_m_origNodes->capacity() ); assert( m_pAccelStruct_m_origNodes->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_primIdCount_size     = uint32_t( m_pAccelStruct_m_primIdCount->size() );     assert( m_pAccelStruct_m_primIdCount->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_primIdCount_capacity = uint32_t( m_pAccelStruct_m_primIdCount->capacity() ); assert( m_pAccelStruct_m_primIdCount->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_primIndices_size     = uint32_t( m_pAccelStruct_m_primIndices->size() );     assert( m_pAccelStruct_m_primIndices->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_primIndices_capacity = uint32_t( m_pAccelStruct_m_primIndices->capacity() ); assert( m_pAccelStruct_m_primIndices->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_vertNorm_size     = uint32_t( m_pAccelStruct_m_vertNorm->size() );     assert( m_pAccelStruct_m_vertNorm->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_vertNorm_capacity = uint32_t( m_pAccelStruct_m_vertNorm->capacity() ); assert( m_pAccelStruct_m_vertNorm->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_vertPos_size     = uint32_t( m_pAccelStruct_m_vertPos->size() );     assert( m_pAccelStruct_m_vertPos->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_m_vertPos_capacity = uint32_t( m_pAccelStruct_m_vertPos->capacity() ); assert( m_pAccelStruct_m_vertPos->capacity() < maxAllowedSize );
  m_uboData.m_pAccelStruct_startEnd_size     = uint32_t( m_pAccelStruct_startEnd->size() );     assert( m_pAccelStruct_startEnd->size() < maxAllowedSize );
  m_uboData.m_pAccelStruct_startEnd_capacity = uint32_t( m_pAccelStruct_startEnd->capacity() ); assert( m_pAccelStruct_startEnd->capacity() < maxAllowedSize );
  m_uboData.m_packedXY_size     = uint32_t( m_packedXY.size() );     assert( m_packedXY.size() < maxAllowedSize );
  m_uboData.m_packedXY_capacity = uint32_t( m_packedXY.capacity() ); assert( m_packedXY.capacity() < maxAllowedSize );
  m_uboData.m_transparencyBuffer_size     = uint32_t( m_transparencyBuffer.size() );     assert( m_transparencyBuffer.size() < maxAllowedSize );
  m_uboData.m_transparencyBuffer_capacity = uint32_t( m_transparencyBuffer.capacity() ); assert( m_transparencyBuffer.capacity() < maxAllowedSize );
  m_uboData.m_vertices_size     = uint32_t( m_vertices.size() );     assert( m_vertices.size() < maxAllowedSize );
  m_uboData.m_vertices_capacity = uint32_t( m_vertices.capacity() ); assert( m_vertices.capacity() < maxAllowedSize );
  a_pCopyEngine->UpdateBuffer(m_classDataBuffer, 0, &m_uboData, sizeof(m_uboData));
}

void MultiRenderer_slang_comp::ReadPlainMembers(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine)
{
  a_pCopyEngine->ReadBuffer(m_classDataBuffer, 0, &m_uboData, sizeof(m_uboData));
  auto pUnderlyingImpl = dynamic_cast<BVHRT*>(m_pAccelStruct->UnderlyingImpl(0));
  m_projInv = m_uboData.m_projInv;
  m_worldViewInv = m_uboData.m_worldViewInv;
  m_cuttingPlane = m_uboData.m_cuttingPlane;
  pUnderlyingImpl->m_cuttingPlane = m_uboData.m_pAccelStruct_m_cuttingPlane;
  m_backgroundColor = m_uboData.m_backgroundColor;
  pUnderlyingImpl->coctree_v3_header = m_uboData.m_pAccelStruct_coctree_v3_header;
  pUnderlyingImpl->m_preset = m_uboData.m_pAccelStruct_m_preset;
  m_preset = m_uboData.m_preset;
  m_highlight = m_uboData.m_highlight;
  m_AAFrameNum = m_uboData.m_AAFrameNum;
  m_height = m_uboData.m_height;
  pUnderlyingImpl->m_SdfDAGBrickTranspositionOffset = m_uboData.m_pAccelStruct_m_SdfDAGBrickTranspositionOffset;
  pUnderlyingImpl->m_SdfDAGTransformCount = m_uboData.m_pAccelStruct_m_SdfDAGTransformCount;
  m_packedXY_height = m_uboData.m_packedXY_height;
  m_packedXY_width = m_uboData.m_packedXY_width;
  m_width = m_uboData.m_width;
  pUnderlyingImpl->support_fast_scom_traversal = m_uboData.m_pAccelStruct_support_fast_scom_traversal;
  m_sceneHasTransparency = m_uboData.m_sceneHasTransparency;
  m_allCTFPoints.resize(m_uboData.m_allCTFPoints_size);
  m_allCTFs.resize(m_uboData.m_allCTFs_size);
  m_allChannelOffsets.resize(m_uboData.m_allChannelOffsets_size);
  m_allChannelRenderInfo.resize(m_uboData.m_allChannelRenderInfo_size);
  m_allCompressedChannels.resize(m_uboData.m_allCompressedChannels_size);
  m_allFloatChannels.resize(m_uboData.m_allFloatChannels_size);
  m_allIntChannels.resize(m_uboData.m_allIntChannels_size);
  m_colorBuffer.resize(m_uboData.m_colorBuffer_size);
  m_gBuffer.resize(m_uboData.m_gBuffer_size);
  m_geomOffsets.resize(m_uboData.m_geomOffsets_size);
  m_indices.resize(m_uboData.m_indices_size);
  m_instanceTransformInvTransposed.resize(m_uboData.m_instanceTransformInvTransposed_size);
  m_lights.resize(m_uboData.m_lights_size);
  m_matIdOffsets.resize(m_uboData.m_matIdOffsets_size);
  m_matIdbyPrimId.resize(m_uboData.m_matIdbyPrimId_size);
  m_materials.resize(m_uboData.m_materials_size);
  m_normals.resize(m_uboData.m_normals_size);
  m_pAccelStruct_m_GraphicsPrimHeaders->resize(m_uboData.m_pAccelStruct_m_GraphicsPrimHeaders_size);
  m_pAccelStruct_m_GraphicsPrimPoints->resize(m_uboData.m_pAccelStruct_m_GraphicsPrimPoints_size);
  m_pAccelStruct_m_RotAddTable->resize(m_uboData.m_pAccelStruct_m_RotAddTable_size);
  m_pAccelStruct_m_SCom2Data->resize(m_uboData.m_pAccelStruct_m_SCom2Data_size);
  m_pAccelStruct_m_SCom2Headers->resize(m_uboData.m_pAccelStruct_m_SCom2Headers_size);
  m_pAccelStruct_m_SdfCompactOctreeRotModifiers->resize(m_uboData.m_pAccelStruct_m_SdfCompactOctreeRotModifiers_size);
  m_pAccelStruct_m_SdfCompactOctreeRotTransforms->resize(m_uboData.m_pAccelStruct_m_SdfCompactOctreeRotTransforms_size);
  m_pAccelStruct_m_SdfCompactOctreeV3Data->resize(m_uboData.m_pAccelStruct_m_SdfCompactOctreeV3Data_size);
  m_pAccelStruct_m_SdfDAGChildEdges->resize(m_uboData.m_pAccelStruct_m_SdfDAGChildEdges_size);
  m_pAccelStruct_m_SdfDAGDataEdges->resize(m_uboData.m_pAccelStruct_m_SdfDAGDataEdges_size);
  m_pAccelStruct_m_SdfDAGDistances->resize(m_uboData.m_pAccelStruct_m_SdfDAGDistances_size);
  m_pAccelStruct_m_SdfDAGHeaders->resize(m_uboData.m_pAccelStruct_m_SdfDAGHeaders_size);
  m_pAccelStruct_m_SdfDAGNodes->resize(m_uboData.m_pAccelStruct_m_SdfDAGNodes_size);
  m_pAccelStruct_m_SdfDAGTranspositions->resize(m_uboData.m_pAccelStruct_m_SdfDAGTranspositions_size);
  m_pAccelStruct_m_SdfFrameOctreeNodes->resize(m_uboData.m_pAccelStruct_m_SdfFrameOctreeNodes_size);
  m_pAccelStruct_m_SdfFrameOctreeRoots->resize(m_uboData.m_pAccelStruct_m_SdfFrameOctreeRoots_size);
  m_pAccelStruct_m_SdfFrameOctreeTexNodes->resize(m_uboData.m_pAccelStruct_m_SdfFrameOctreeTexNodes_size);
  m_pAccelStruct_m_SdfFrameOctreeTexRoots->resize(m_uboData.m_pAccelStruct_m_SdfFrameOctreeTexRoots_size);
  m_pAccelStruct_m_SdfGridData->resize(m_uboData.m_pAccelStruct_m_SdfGridData_size);
  m_pAccelStruct_m_SdfGridOffsets->resize(m_uboData.m_pAccelStruct_m_SdfGridOffsets_size);
  m_pAccelStruct_m_SdfGridSizes->resize(m_uboData.m_pAccelStruct_m_SdfGridSizes_size);
  m_pAccelStruct_m_SdfMultiOctreeNodes->resize(m_uboData.m_pAccelStruct_m_SdfMultiOctreeNodes_size);
  m_pAccelStruct_m_SdfMultiOctreeValues->resize(m_uboData.m_pAccelStruct_m_SdfMultiOctreeValues_size);
  m_pAccelStruct_m_SdfSBSData->resize(m_uboData.m_pAccelStruct_m_SdfSBSData_size);
  m_pAccelStruct_m_SdfSBSDataF->resize(m_uboData.m_pAccelStruct_m_SdfSBSDataF_size);
  m_pAccelStruct_m_SdfSBSHeaders->resize(m_uboData.m_pAccelStruct_m_SdfSBSHeaders_size);
  m_pAccelStruct_m_SdfSBSNodes->resize(m_uboData.m_pAccelStruct_m_SdfSBSNodes_size);
  m_pAccelStruct_m_SdfSBSRoots->resize(m_uboData.m_pAccelStruct_m_SdfSBSRoots_size);
  m_pAccelStruct_m_SdfSVSNodes->resize(m_uboData.m_pAccelStruct_m_SdfSVSNodes_size);
  m_pAccelStruct_m_SdfSVSRoots->resize(m_uboData.m_pAccelStruct_m_SdfSVSRoots_size);
  m_pAccelStruct_m_allNodePairs->resize(m_uboData.m_pAccelStruct_m_allNodePairs_size);
  m_pAccelStruct_m_geomData->resize(m_uboData.m_pAccelStruct_m_geomData_size);
  m_pAccelStruct_m_geomTags->resize(m_uboData.m_pAccelStruct_m_geomTags_size);
  m_pAccelStruct_m_indices->resize(m_uboData.m_pAccelStruct_m_indices_size);
  m_pAccelStruct_m_instanceData->resize(m_uboData.m_pAccelStruct_m_instanceData_size);
  m_pAccelStruct_m_nodesTLAS->resize(m_uboData.m_pAccelStruct_m_nodesTLAS_size);
  m_pAccelStruct_m_origNodes->resize(m_uboData.m_pAccelStruct_m_origNodes_size);
  m_pAccelStruct_m_primIdCount->resize(m_uboData.m_pAccelStruct_m_primIdCount_size);
  m_pAccelStruct_m_primIndices->resize(m_uboData.m_pAccelStruct_m_primIndices_size);
  m_pAccelStruct_m_vertNorm->resize(m_uboData.m_pAccelStruct_m_vertNorm_size);
  m_pAccelStruct_m_vertPos->resize(m_uboData.m_pAccelStruct_m_vertPos_size);
  m_pAccelStruct_startEnd->resize(m_uboData.m_pAccelStruct_startEnd_size);
  m_packedXY.resize(m_uboData.m_packedXY_size);
  m_transparencyBuffer.resize(m_uboData.m_transparencyBuffer_size);
  m_vertices.resize(m_uboData.m_vertices_size);
}

void MultiRenderer_slang_comp::UpdateVectorMembers(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine)
{
  if(m_allCTFPoints.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_allCTFPointsBuffer, 0, m_allCTFPoints.data(), m_allCTFPoints.size()*sizeof(float4) );
  if(m_allCTFs.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_allCTFsBuffer, 0, m_allCTFs.data(), m_allCTFs.size()*sizeof(ColorTranferFunctionInfo) );
  if(m_allChannelOffsets.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_allChannelOffsetsBuffer, 0, m_allChannelOffsets.data(), m_allChannelOffsets.size()*sizeof(uint4) );
  if(m_allChannelRenderInfo.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_allChannelRenderInfoBuffer, 0, m_allChannelRenderInfo.data(), m_allChannelRenderInfo.size()*sizeof(ChannelRenderInfo) );
  if(m_allCompressedChannels.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_allCompressedChannelsBuffer, 0, m_allCompressedChannels.data(), m_allCompressedChannels.size()*sizeof(unsigned int) );
  if(m_allFloatChannels.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_allFloatChannelsBuffer, 0, m_allFloatChannels.data(), m_allFloatChannels.size()*sizeof(float) );
  if(m_allIntChannels.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_allIntChannelsBuffer, 0, m_allIntChannels.data(), m_allIntChannels.size()*sizeof(int) );
  if(m_colorBuffer.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_colorBufferBuffer, 0, m_colorBuffer.data(), m_colorBuffer.size()*sizeof(float4) );
  if(m_gBuffer.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_gBufferBuffer, 0, m_gBuffer.data(), m_gBuffer.size()*sizeof(CRT_Hit) );
  if(m_geomOffsets.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_geomOffsetsBuffer, 0, m_geomOffsets.data(), m_geomOffsets.size()*sizeof(uint2) );
  if(m_indices.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_indicesBuffer, 0, m_indices.data(), m_indices.size()*sizeof(unsigned int) );
  if(m_instanceTransformInvTransposed.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_instanceTransformInvTransposedBuffer, 0, m_instanceTransformInvTransposed.data(), m_instanceTransformInvTransposed.size()*sizeof(float4x4) );
  if(m_lights.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_lightsBuffer, 0, m_lights.data(), m_lights.size()*sizeof(Light) );
  if(m_matIdOffsets.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_matIdOffsetsBuffer, 0, m_matIdOffsets.data(), m_matIdOffsets.size()*sizeof(uint2) );
  if(m_matIdbyPrimId.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_matIdbyPrimIdBuffer, 0, m_matIdbyPrimId.data(), m_matIdbyPrimId.size()*sizeof(unsigned int) );
  if(m_materials.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_materialsBuffer, 0, m_materials.data(), m_materials.size()*sizeof(MultiRendererMaterial) );
  if(m_normals.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_normalsBuffer, 0, m_normals.data(), m_normals.size()*sizeof(float4) );
  if(m_pAccelStruct_m_GraphicsPrimHeaders->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_GraphicsPrimHeadersBuffer, 0, m_pAccelStruct_m_GraphicsPrimHeaders->data(), m_pAccelStruct_m_GraphicsPrimHeaders->size()*sizeof(GraphicsPrimHeader) );
  if(m_pAccelStruct_m_GraphicsPrimPoints->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_GraphicsPrimPointsBuffer, 0, m_pAccelStruct_m_GraphicsPrimPoints->data(), m_pAccelStruct_m_GraphicsPrimPoints->size()*sizeof(float4) );
  if(m_pAccelStruct_m_RotAddTable->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_RotAddTableBuffer, 0, m_pAccelStruct_m_RotAddTable->data(), m_pAccelStruct_m_RotAddTable->size()*sizeof(unsigned int) );
  if(m_pAccelStruct_m_SCom2Data->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SCom2DataBuffer, 0, m_pAccelStruct_m_SCom2Data->data(), m_pAccelStruct_m_SCom2Data->size()*sizeof(unsigned int) );
  if(m_pAccelStruct_m_SCom2Headers->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SCom2HeadersBuffer, 0, m_pAccelStruct_m_SCom2Headers->data(), m_pAccelStruct_m_SCom2Headers->size()*sizeof(SCom2Header) );
  if(m_pAccelStruct_m_SdfCompactOctreeRotModifiers->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotModifiersBuffer, 0, m_pAccelStruct_m_SdfCompactOctreeRotModifiers->data(), m_pAccelStruct_m_SdfCompactOctreeRotModifiers->size()*sizeof(int4) );
  if(m_pAccelStruct_m_SdfCompactOctreeRotTransforms->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfCompactOctreeRotTransformsBuffer, 0, m_pAccelStruct_m_SdfCompactOctreeRotTransforms->data(), m_pAccelStruct_m_SdfCompactOctreeRotTransforms->size()*sizeof(float4x4) );
  if(m_pAccelStruct_m_SdfCompactOctreeV3Data->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfCompactOctreeV3DataBuffer, 0, m_pAccelStruct_m_SdfCompactOctreeV3Data->data(), m_pAccelStruct_m_SdfCompactOctreeV3Data->size()*sizeof(unsigned int) );
  if(m_pAccelStruct_m_SdfDAGChildEdges->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfDAGChildEdgesBuffer, 0, m_pAccelStruct_m_SdfDAGChildEdges->data(), m_pAccelStruct_m_SdfDAGChildEdges->size()*sizeof(SdfDAGChildEdge) );
  if(m_pAccelStruct_m_SdfDAGDataEdges->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfDAGDataEdgesBuffer, 0, m_pAccelStruct_m_SdfDAGDataEdges->data(), m_pAccelStruct_m_SdfDAGDataEdges->size()*sizeof(SdfDAGDataEdge) );
  if(m_pAccelStruct_m_SdfDAGDistances->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfDAGDistancesBuffer, 0, m_pAccelStruct_m_SdfDAGDistances->data(), m_pAccelStruct_m_SdfDAGDistances->size()*sizeof(float) );
  if(m_pAccelStruct_m_SdfDAGHeaders->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfDAGHeadersBuffer, 0, m_pAccelStruct_m_SdfDAGHeaders->data(), m_pAccelStruct_m_SdfDAGHeaders->size()*sizeof(SdfDAGHeader) );
  if(m_pAccelStruct_m_SdfDAGNodes->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfDAGNodesBuffer, 0, m_pAccelStruct_m_SdfDAGNodes->data(), m_pAccelStruct_m_SdfDAGNodes->size()*sizeof(SdfDAGNode) );
  if(m_pAccelStruct_m_SdfDAGTranspositions->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfDAGTranspositionsBuffer, 0, m_pAccelStruct_m_SdfDAGTranspositions->data(), m_pAccelStruct_m_SdfDAGTranspositions->size()*sizeof(unsigned int) );
  if(m_pAccelStruct_m_SdfFrameOctreeNodes->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfFrameOctreeNodesBuffer, 0, m_pAccelStruct_m_SdfFrameOctreeNodes->data(), m_pAccelStruct_m_SdfFrameOctreeNodes->size()*sizeof(SdfFrameOctreeNode) );
  if(m_pAccelStruct_m_SdfFrameOctreeRoots->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfFrameOctreeRootsBuffer, 0, m_pAccelStruct_m_SdfFrameOctreeRoots->data(), m_pAccelStruct_m_SdfFrameOctreeRoots->size()*sizeof(unsigned int) );
  if(m_pAccelStruct_m_SdfFrameOctreeTexNodes->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexNodesBuffer, 0, m_pAccelStruct_m_SdfFrameOctreeTexNodes->data(), m_pAccelStruct_m_SdfFrameOctreeTexNodes->size()*sizeof(SdfFrameOctreeTexNode) );
  if(m_pAccelStruct_m_SdfFrameOctreeTexRoots->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfFrameOctreeTexRootsBuffer, 0, m_pAccelStruct_m_SdfFrameOctreeTexRoots->data(), m_pAccelStruct_m_SdfFrameOctreeTexRoots->size()*sizeof(unsigned int) );
  if(m_pAccelStruct_m_SdfGridData->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfGridDataBuffer, 0, m_pAccelStruct_m_SdfGridData->data(), m_pAccelStruct_m_SdfGridData->size()*sizeof(float) );
  if(m_pAccelStruct_m_SdfGridOffsets->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfGridOffsetsBuffer, 0, m_pAccelStruct_m_SdfGridOffsets->data(), m_pAccelStruct_m_SdfGridOffsets->size()*sizeof(unsigned int) );
  if(m_pAccelStruct_m_SdfGridSizes->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfGridSizesBuffer, 0, m_pAccelStruct_m_SdfGridSizes->data(), m_pAccelStruct_m_SdfGridSizes->size()*sizeof(uint3) );
  if(m_pAccelStruct_m_SdfMultiOctreeNodes->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfMultiOctreeNodesBuffer, 0, m_pAccelStruct_m_SdfMultiOctreeNodes->data(), m_pAccelStruct_m_SdfMultiOctreeNodes->size()*sizeof(SdfMultiOctreeNode) );
  if(m_pAccelStruct_m_SdfMultiOctreeValues->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfMultiOctreeValuesBuffer, 0, m_pAccelStruct_m_SdfMultiOctreeValues->data(), m_pAccelStruct_m_SdfMultiOctreeValues->size()*sizeof(float) );
  if(m_pAccelStruct_m_SdfSBSData->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfSBSDataBuffer, 0, m_pAccelStruct_m_SdfSBSData->data(), m_pAccelStruct_m_SdfSBSData->size()*sizeof(unsigned int) );
  if(m_pAccelStruct_m_SdfSBSDataF->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfSBSDataFBuffer, 0, m_pAccelStruct_m_SdfSBSDataF->data(), m_pAccelStruct_m_SdfSBSDataF->size()*sizeof(float) );
  if(m_pAccelStruct_m_SdfSBSHeaders->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfSBSHeadersBuffer, 0, m_pAccelStruct_m_SdfSBSHeaders->data(), m_pAccelStruct_m_SdfSBSHeaders->size()*sizeof(SdfSBSHeader) );
  if(m_pAccelStruct_m_SdfSBSNodes->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfSBSNodesBuffer, 0, m_pAccelStruct_m_SdfSBSNodes->data(), m_pAccelStruct_m_SdfSBSNodes->size()*sizeof(SdfSBSNode) );
  if(m_pAccelStruct_m_SdfSBSRoots->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfSBSRootsBuffer, 0, m_pAccelStruct_m_SdfSBSRoots->data(), m_pAccelStruct_m_SdfSBSRoots->size()*sizeof(unsigned int) );
  if(m_pAccelStruct_m_SdfSVSNodes->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfSVSNodesBuffer, 0, m_pAccelStruct_m_SdfSVSNodes->data(), m_pAccelStruct_m_SdfSVSNodes->size()*sizeof(SdfSVSNode) );
  if(m_pAccelStruct_m_SdfSVSRoots->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_SdfSVSRootsBuffer, 0, m_pAccelStruct_m_SdfSVSRoots->data(), m_pAccelStruct_m_SdfSVSRoots->size()*sizeof(unsigned int) );
  if(m_pAccelStruct_m_allNodePairs->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_allNodePairsBuffer, 0, m_pAccelStruct_m_allNodePairs->data(), m_pAccelStruct_m_allNodePairs->size()*sizeof(BVHNodePair) );
  if(m_pAccelStruct_m_geomData->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_geomDataBuffer, 0, m_pAccelStruct_m_geomData->data(), m_pAccelStruct_m_geomData->size()*sizeof(GeomData) );
  if(m_pAccelStruct_m_geomTags->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_geomTagsBuffer, 0, m_pAccelStruct_m_geomTags->data(), m_pAccelStruct_m_geomTags->size()*sizeof(unsigned int) );
  if(m_pAccelStruct_m_indices->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_indicesBuffer, 0, m_pAccelStruct_m_indices->data(), m_pAccelStruct_m_indices->size()*sizeof(unsigned int) );
  if(m_pAccelStruct_m_instanceData->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_instanceDataBuffer, 0, m_pAccelStruct_m_instanceData->data(), m_pAccelStruct_m_instanceData->size()*sizeof(InstanceData) );
  if(m_pAccelStruct_m_nodesTLAS->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_nodesTLASBuffer, 0, m_pAccelStruct_m_nodesTLAS->data(), m_pAccelStruct_m_nodesTLAS->size()*sizeof(BVHNode) );
  if(m_pAccelStruct_m_origNodes->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_origNodesBuffer, 0, m_pAccelStruct_m_origNodes->data(), m_pAccelStruct_m_origNodes->size()*sizeof(BVHNode) );
  if(m_pAccelStruct_m_primIdCount->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_primIdCountBuffer, 0, m_pAccelStruct_m_primIdCount->data(), m_pAccelStruct_m_primIdCount->size()*sizeof(unsigned int) );
  if(m_pAccelStruct_m_primIndices->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_primIndicesBuffer, 0, m_pAccelStruct_m_primIndices->data(), m_pAccelStruct_m_primIndices->size()*sizeof(unsigned int) );
  if(m_pAccelStruct_m_vertNorm->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_vertNormBuffer, 0, m_pAccelStruct_m_vertNorm->data(), m_pAccelStruct_m_vertNorm->size()*sizeof(float4) );
  if(m_pAccelStruct_m_vertPos->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_m_vertPosBuffer, 0, m_pAccelStruct_m_vertPos->data(), m_pAccelStruct_m_vertPos->size()*sizeof(float4) );
  if(m_pAccelStruct_startEnd->size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_pAccelStruct_startEndBuffer, 0, m_pAccelStruct_startEnd->data(), m_pAccelStruct_startEnd->size()*sizeof(uint2) );
  if(m_packedXY.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_packedXYBuffer, 0, m_packedXY.data(), m_packedXY.size()*sizeof(unsigned int) );
  if(m_transparencyBuffer.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_transparencyBufferBuffer, 0, m_transparencyBuffer.data(), m_transparencyBuffer.size()*sizeof(float4) );
  if(m_vertices.size() > 0)
    a_pCopyEngine->UpdateBuffer(m_vdata.m_verticesBuffer, 0, m_vertices.data(), m_vertices.size()*sizeof(float4) );
}

void MultiRenderer_slang_comp::Update_m_lights(size_t a_first, size_t a_size)
{
  if(m_lights.size() != 0 && m_pLastCopyHelper != nullptr)
    m_pLastCopyHelper->UpdateBuffer(m_vdata.m_lightsBuffer, a_first*sizeof(struct Light), m_lights.data() + a_first, a_size*sizeof(struct Light) );
}

void MultiRenderer_slang_comp::UpdateTextureMembers(std::shared_ptr<vk_utils::ICopyEngine> a_pCopyEngine)
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
    throw std::runtime_error("MultiRenderer_slang_comp::UpdateTextureMembers: failed to begin command buffer!");
  vkCmdPipelineBarrier(cmdBuff,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,nullptr,0,nullptr,uint32_t(barriers.size()),barriers.data());
  vkEndCommandBuffer(cmdBuff);

  vk_utils::executeCommandBufferNow(cmdBuff, transferQueue, m_device);
}

void MultiRenderer_slang_comp::ResolveDebugCmd(uint32_t count, uint32_t sample_id, uint32_t render_mode, LiteMath::float4* out_color)
{
  uint32_t blockSizeX = 256;
  uint32_t blockSizeY = 1;
  uint32_t blockSizeZ = 1;

  struct KernelArgsPC
  {
    uint32_t m_sample_id;
    uint32_t m_render_mode;
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
  pcData.m_render_mode = render_mode;
  vkCmdPushConstants(m_currCmdBuffer, ResolveDebugLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(KernelArgsPC), &pcData);
  
  vkCmdBindPipeline(m_currCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ResolveDebugPipeline);
  vkCmdDispatch    (m_currCmdBuffer, (sizeX + blockSizeX - 1) / blockSizeX, (sizeY + blockSizeY - 1) / blockSizeY, (sizeZ + blockSizeZ - 1) / blockSizeZ);
}

void MultiRenderer_slang_comp::FillGbufferCmd(uint32_t count, uint32_t sample_id)
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
  vkCmdPushConstants(m_currCmdBuffer, FillGbufferLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(KernelArgsPC), &pcData);
  
  vkCmdBindPipeline(m_currCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, FillGbufferPipeline);
  vkCmdDispatch    (m_currCmdBuffer, (sizeX + blockSizeX - 1) / blockSizeX, (sizeY + blockSizeY - 1) / blockSizeY, (sizeZ + blockSizeZ - 1) / blockSizeZ);
}

void MultiRenderer_slang_comp::FillGbufferWithTransparencyCmd(uint32_t count, uint32_t sample_id)
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
  vkCmdPushConstants(m_currCmdBuffer, FillGbufferWithTransparencyLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(KernelArgsPC), &pcData);
  
  vkCmdBindPipeline(m_currCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, FillGbufferWithTransparencyPipeline);
  vkCmdDispatch    (m_currCmdBuffer, (sizeX + blockSizeX - 1) / blockSizeX, (sizeY + blockSizeY - 1) / blockSizeY, (sizeZ + blockSizeZ - 1) / blockSizeZ);
}

void MultiRenderer_slang_comp::ResolveOutlineCmd(uint32_t count, uint32_t sample_id, uint32_t render_mode, float4* out_color)
{
  uint32_t blockSizeX = 256;
  uint32_t blockSizeY = 1;
  uint32_t blockSizeZ = 1;

  struct KernelArgsPC
  {
    uint32_t m_sample_id;
    uint32_t m_render_mode;
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
  pcData.m_render_mode = render_mode;
  vkCmdPushConstants(m_currCmdBuffer, ResolveOutlineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(KernelArgsPC), &pcData);
  
  vkCmdBindPipeline(m_currCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ResolveOutlinePipeline);
  vkCmdDispatch    (m_currCmdBuffer, (sizeX + blockSizeX - 1) / blockSizeX, (sizeY + blockSizeY - 1) / blockSizeY, (sizeZ + blockSizeZ - 1) / blockSizeZ);
}

void MultiRenderer_slang_comp::AverageColorCmd(uint32_t count, uint32_t sample_count, float4* out_color)
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

void MultiRenderer_slang_comp::TonemapCmd(uint32_t count, uint32_t sample_count, uint32_t* out_color)
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

void MultiRenderer_slang_comp::PackXYCmd(uint32_t w, uint32_t h)
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

void MultiRenderer_slang_comp::ResolveCommonCmd(uint32_t count, uint32_t sample_id, uint32_t render_mode, LiteMath::float4* out_color)
{
  uint32_t blockSizeX = 256;
  uint32_t blockSizeY = 1;
  uint32_t blockSizeZ = 1;

  struct KernelArgsPC
  {
    uint32_t m_sample_id;
    uint32_t m_render_mode;
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
  pcData.m_render_mode = render_mode;
  vkCmdPushConstants(m_currCmdBuffer, ResolveCommonLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(KernelArgsPC), &pcData);
  
  vkCmdBindPipeline(m_currCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ResolveCommonPipeline);
  vkCmdDispatch    (m_currCmdBuffer, (sizeX + blockSizeX - 1) / blockSizeX, (sizeY + blockSizeY - 1) / blockSizeY, (sizeZ + blockSizeZ - 1) / blockSizeZ);
}

void MultiRenderer_slang_comp::ResolveCommonWithTransparencyCmd(uint32_t count, uint32_t sample_id, uint32_t render_mode, LiteMath::float4* out_color)
{
  uint32_t blockSizeX = 256;
  uint32_t blockSizeY = 1;
  uint32_t blockSizeZ = 1;

  struct KernelArgsPC
  {
    uint32_t m_sample_id;
    uint32_t m_render_mode;
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
  pcData.m_render_mode = render_mode;
  vkCmdPushConstants(m_currCmdBuffer, ResolveCommonWithTransparencyLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(KernelArgsPC), &pcData);
  
  vkCmdBindPipeline(m_currCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ResolveCommonWithTransparencyPipeline);
  vkCmdDispatch    (m_currCmdBuffer, (sizeX + blockSizeX - 1) / blockSizeX, (sizeY + blockSizeY - 1) / blockSizeY, (sizeZ + blockSizeZ - 1) / blockSizeZ);
}

void MultiRenderer_slang_comp::ResolveCTFCmd(uint32_t count, uint32_t sample_id, uint32_t render_mode, LiteMath::float4* out_color)
{
  uint32_t blockSizeX = 256;
  uint32_t blockSizeY = 1;
  uint32_t blockSizeZ = 1;

  struct KernelArgsPC
  {
    uint32_t m_sample_id;
    uint32_t m_render_mode;
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
  pcData.m_render_mode = render_mode;
  vkCmdPushConstants(m_currCmdBuffer, ResolveCTFLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(KernelArgsPC), &pcData);
  
  vkCmdBindPipeline(m_currCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ResolveCTFPipeline);
  vkCmdDispatch    (m_currCmdBuffer, (sizeX + blockSizeX - 1) / blockSizeX, (sizeY + blockSizeY - 1) / blockSizeY, (sizeZ + blockSizeZ - 1) / blockSizeZ);
}


void MultiRenderer_slang_comp::copyKernelFloatCmd(uint32_t length)
{
  uint32_t blockSizeX = MEMCPY_BLOCK_SIZE;

  vkCmdBindPipeline(m_currCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, copyKernelFloatPipeline);
  vkCmdPushConstants(m_currCmdBuffer, copyKernelFloatLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &length);
  vkCmdDispatch(m_currCmdBuffer, (length + blockSizeX - 1) / blockSizeX, 1, 1);
}

void MultiRenderer_slang_comp::matMulTransposeCmd(uint32_t A_offset, uint32_t B_offset, uint32_t C_offset, uint32_t A_col_len, uint32_t B_col_len, uint32_t A_row_len)
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

VkBufferMemoryBarrier MultiRenderer_slang_comp::BarrierForClearFlags(VkBuffer a_buffer)
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

VkBufferMemoryBarrier MultiRenderer_slang_comp::BarrierForSingleBuffer(VkBuffer a_buffer)
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

void MultiRenderer_slang_comp::BarriersForSeveralBuffers(VkBuffer* a_inBuffers, VkBufferMemoryBarrier* a_outBarriers, uint32_t a_buffersNum)
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

void MultiRenderer_slang_comp::RenderFloatCmd(VkCommandBuffer a_commandBuffer, float4* a_outColor, uint32_t a_width, uint32_t a_height, int a_passNum)
{
  VkPipelineStageFlagBits prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  m_currCmdBuffer = a_commandBuffer;
  VkMemoryBarrier memoryBarrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT };
    assert(m_width == a_width);
  assert(m_height == a_height);

  auto before = std::chrono::high_resolution_clock::now();

  uint32_t samples = a_passNum*m_preset.spp;
  CheckForTransparency();
  for (uint32_t sample = 0; sample < samples; sample++)
  {
    switch (m_preset.render_mode)
    {
    case MULTI_RENDER_MODE_DIFFUSE:
    case MULTI_RENDER_MODE_LAMBERT:
    case MULTI_RENDER_MODE_PHONG:
    case MULTI_RENDER_MODE_PBR:
      if (m_sceneHasTransparency)
      {
        {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, FillGbufferWithTransparencyLayout, 0, 1, &m_allGeneratedDS[0], 0, nullptr);
  FillGbufferWithTransparencyCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
        {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ResolveCommonWithTransparencyLayout, 0, 1, &m_allGeneratedDS[1], 0, nullptr);
  ResolveCommonWithTransparencyCmd(m_width * m_height, sample, m_preset.render_mode, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      }
      else
      {
        {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, FillGbufferLayout, 0, 1, &m_allGeneratedDS[2], 0, nullptr);
  FillGbufferCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
        {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ResolveCommonLayout, 0, 1, &m_allGeneratedDS[3], 0, nullptr);
  ResolveCommonCmd(m_width * m_height, sample, m_preset.render_mode, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      }
      break;
    case MULTI_RENDER_MODE_LAMBERT_NO_TEX:
    case MULTI_RENDER_MODE_PHONG_NO_TEX:
    case MULTI_RENDER_MODE_PBR_NO_TEX:
    case MULTI_RENDER_MODE_GEOM:
    case MULTI_RENDER_MODE_LOD:
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, FillGbufferLayout, 0, 1, &m_allGeneratedDS[2], 0, nullptr);
  FillGbufferCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ResolveCommonLayout, 0, 1, &m_allGeneratedDS[4], 0, nullptr);
  ResolveCommonCmd(m_width*m_height, sample, m_preset.render_mode, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      break;
    case MULTI_RENDER_MODE_CHANNEL:
    case MULTI_RENDER_MODE_CHANNEL_LAMBERT:
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, FillGbufferLayout, 0, 1, &m_allGeneratedDS[2], 0, nullptr);
  FillGbufferCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ResolveCTFLayout, 0, 1, &m_allGeneratedDS[5], 0, nullptr);
  ResolveCTFCmd(m_width*m_height, sample, m_preset.render_mode, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      break;
    case MULTI_RENDER_MODE_OUTLINE_BORDER:
    case MULTI_RENDER_MODE_OUTLINE_PRIM:
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, FillGbufferLayout, 0, 1, &m_allGeneratedDS[2], 0, nullptr);
  FillGbufferCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ResolveOutlineLayout, 0, 1, &m_allGeneratedDS[6], 0, nullptr);
  ResolveOutlineCmd(m_width*m_height, sample, m_preset.render_mode, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      break;
    default:
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, FillGbufferLayout, 0, 1, &m_allGeneratedDS[2], 0, nullptr);
  FillGbufferCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ResolveDebugLayout, 0, 1, &m_allGeneratedDS[7], 0, nullptr);
  ResolveDebugCmd(m_width*m_height, sample, m_preset.render_mode, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      break;
    }
  }
  {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, AverageColorLayout, 0, 1, &m_allGeneratedDS[8], 0, nullptr);
  AverageColorCmd(m_width*m_height, samples, a_outColor);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};

  auto after = std::chrono::high_resolution_clock::now();
  timeDataByName["RenderFloat"] = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count()/1000.0f;

}

void MultiRenderer_slang_comp::RenderCmd(VkCommandBuffer a_commandBuffer, uint32_t* a_outColor, uint32_t a_width, uint32_t a_height, int a_passNum)
{
  VkPipelineStageFlagBits prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  m_currCmdBuffer = a_commandBuffer;
  VkMemoryBarrier memoryBarrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT };
    assert(m_width == a_width);
  assert(m_height == a_height);

  auto before = std::chrono::high_resolution_clock::now();

  uint32_t samples = a_passNum*m_preset.spp;
  CheckForTransparency();
  for (uint32_t sample = 0; sample < samples; sample++)
  {
    switch (m_preset.render_mode)
    {
    case MULTI_RENDER_MODE_DIFFUSE:
    case MULTI_RENDER_MODE_LAMBERT:
    case MULTI_RENDER_MODE_PHONG:
    case MULTI_RENDER_MODE_PBR:
      if (m_sceneHasTransparency)
      {
        {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, FillGbufferWithTransparencyLayout, 0, 1, &m_allGeneratedDS[9], 0, nullptr);
  FillGbufferWithTransparencyCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
        {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ResolveCommonWithTransparencyLayout, 0, 1, &m_allGeneratedDS[10], 0, nullptr);
  ResolveCommonWithTransparencyCmd(m_width * m_height, sample, m_preset.render_mode, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      }
      else
      {
        {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, FillGbufferLayout, 0, 1, &m_allGeneratedDS[11], 0, nullptr);
  FillGbufferCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
        {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ResolveCommonLayout, 0, 1, &m_allGeneratedDS[12], 0, nullptr);
  ResolveCommonCmd(m_width * m_height, sample, m_preset.render_mode, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      }
      break;
    case MULTI_RENDER_MODE_LAMBERT_NO_TEX:
    case MULTI_RENDER_MODE_PHONG_NO_TEX:
    case MULTI_RENDER_MODE_PBR_NO_TEX:
    case MULTI_RENDER_MODE_GEOM:
    case MULTI_RENDER_MODE_LOD:
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, FillGbufferLayout, 0, 1, &m_allGeneratedDS[11], 0, nullptr);
  FillGbufferCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ResolveCommonLayout, 0, 1, &m_allGeneratedDS[13], 0, nullptr);
  ResolveCommonCmd(m_width*m_height, sample, m_preset.render_mode, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      break;
    case MULTI_RENDER_MODE_CHANNEL:
    case MULTI_RENDER_MODE_CHANNEL_LAMBERT:
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, FillGbufferLayout, 0, 1, &m_allGeneratedDS[11], 0, nullptr);
  FillGbufferCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ResolveCTFLayout, 0, 1, &m_allGeneratedDS[14], 0, nullptr);
  ResolveCTFCmd(m_width*m_height, sample, m_preset.render_mode, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      break;
    case MULTI_RENDER_MODE_OUTLINE_BORDER:
    case MULTI_RENDER_MODE_OUTLINE_PRIM:
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, FillGbufferLayout, 0, 1, &m_allGeneratedDS[11], 0, nullptr);
  FillGbufferCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ResolveOutlineLayout, 0, 1, &m_allGeneratedDS[15], 0, nullptr);
  ResolveOutlineCmd(m_width*m_height, sample, m_preset.render_mode, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      break;
    default:
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, FillGbufferLayout, 0, 1, &m_allGeneratedDS[11], 0, nullptr);
  FillGbufferCmd(m_packedXY_width * m_packedXY_height, sample % m_preset.spp);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ResolveDebugLayout, 0, 1, &m_allGeneratedDS[16], 0, nullptr);
  ResolveDebugCmd(m_width*m_height, sample, m_preset.render_mode, m_colorBuffer.data());
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};
      break;
    }
  }
  {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, TonemapLayout, 0, 1, &m_allGeneratedDS[17], 0, nullptr);
  TonemapCmd(m_width*m_height, samples, a_outColor);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};

  auto after = std::chrono::high_resolution_clock::now();
  timeDataByName["Render"] = std::chrono::duration_cast<std::chrono::microseconds>(after - before).count()/1000.0f;

}

void MultiRenderer_slang_comp::ClearCmd(VkCommandBuffer a_commandBuffer, uint32_t a_width, uint32_t a_height)
{
  VkPipelineStageFlagBits prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  m_currCmdBuffer = a_commandBuffer;
  VkMemoryBarrier memoryBarrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT };
    {vkCmdBindDescriptorSets(a_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, PackXYLayout, 0, 1, &m_allGeneratedDS[18], 0, nullptr);
  PackXYCmd(a_width, a_height);
  vkCmdPipelineBarrier(m_currCmdBuffer, prevStageBits, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
  prevStageBits = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;};

}



void MultiRenderer_slang_comp::RenderFloat(float4* a_outColor, uint32_t a_width, uint32_t a_height, int a_passNum)
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
  VkBuffer a_outColorGPU = vk_utils::createBuffer(device, a_width*a_height*sizeof(float4 ), outFlags);
  buffers.push_back(a_outColorGPU);

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
  this->SetVulkanInOutFor_RenderFloat(a_outColorGPU, 0, 0);

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
    RenderFloatCmd(commandBuffer, a_outColor, a_width, a_height, a_passNum);
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
  pCopyHelper->ReadBuffer(a_outColorGPU, 0, a_outColor, a_width*a_height*sizeof(float4 ));
  this->ReadPlainMembers(pCopyHelper);
  afterCopy2 = std::chrono::high_resolution_clock::now();
  m_exTimeRenderFloat.msCopyFromGPU = std::chrono::duration_cast<std::chrono::microseconds>(afterCopy2 - beforeCopy2).count()/1000.f;

  // (6) free resources
  //
  vkDestroyBuffer(device, a_outColorGPU, nullptr);
  if(buffersMem != VK_NULL_HANDLE)
    vkFreeMemory(device, buffersMem, nullptr);
  if(imagesMem != VK_NULL_HANDLE)
    vkFreeMemory(device, imagesMem, nullptr);

  m_exTimeRenderFloat.msAPIOverhead += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - afterCopy2).count()/1000.f;
}

void MultiRenderer_slang_comp::Render(uint32_t* a_outColor, uint32_t a_width, uint32_t a_height, int a_passNum)
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
  VkBuffer a_outColorGPU = vk_utils::createBuffer(device, a_width*a_height*sizeof(uint32_t ), outFlags);
  buffers.push_back(a_outColorGPU);

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
  this->SetVulkanInOutFor_Render(a_outColorGPU, 0, 0);

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
    RenderCmd(commandBuffer, a_outColor, a_width, a_height, a_passNum);
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
  pCopyHelper->ReadBuffer(a_outColorGPU, 0, a_outColor, a_width*a_height*sizeof(uint32_t ));
  this->ReadPlainMembers(pCopyHelper);
  afterCopy2 = std::chrono::high_resolution_clock::now();
  m_exTimeRender.msCopyFromGPU = std::chrono::duration_cast<std::chrono::microseconds>(afterCopy2 - beforeCopy2).count()/1000.f;

  // (6) free resources
  //
  vkDestroyBuffer(device, a_outColorGPU, nullptr);
  if(buffersMem != VK_NULL_HANDLE)
    vkFreeMemory(device, buffersMem, nullptr);
  if(imagesMem != VK_NULL_HANDLE)
    vkFreeMemory(device, imagesMem, nullptr);

  m_exTimeRender.msAPIOverhead += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - afterCopy2).count()/1000.f;
}

void MultiRenderer_slang_comp::Clear(uint32_t a_width, uint32_t a_height)
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
    ClearCmd(commandBuffer, a_width, a_height);
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
  this->ReadPlainMembers(pCopyHelper);
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



void MultiRenderer_slang_comp::GetExecutionTime(const char* a_funcName, float a_out[4])
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


