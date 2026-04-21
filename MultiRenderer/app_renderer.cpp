#include "app_renderer.h"

void MultiRendererApp::UpdateInstances(const std::vector<uint32_t> &instanceIds, const std::vector<LiteMath::float4x4> &matrices)
{
#ifdef ONLY_SINGLE_MODEL_SCOM
  printf("[MultiRendererApp::UpdateInstances] Operation not supported, GPU implementation is compiled without instancing support\n");
#else
  for (size_t i = 0; i < instanceIds.size(); i++)
    MultiRenderer_GPU::UpdateInstance(instanceIds[i], matrices[i]);
  GetAccelStruct()->CommitScene();
  m_pLastCopyHelper->UpdateBuffer(m_vdata.m_pAccelStruct_m_instanceDataBuffer, 0,
                            m_pAccelStruct_m_instanceData->data(),
                            m_pAccelStruct_m_instanceData->size() * sizeof(struct InstanceData));
  m_pLastCopyHelper->UpdateBuffer(m_vdata.m_pAccelStruct_m_nodesTLASBuffer, 0,
                            m_pAccelStruct_m_nodesTLAS->data(),
                            m_pAccelStruct_m_nodesTLAS->size() * sizeof(struct BVHNode));
#endif
}

void MultiRendererApp::UpdateVisibilityBuffer()
{
  
}