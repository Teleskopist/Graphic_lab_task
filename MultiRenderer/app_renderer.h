#pragma once
#include "MultiRenderer/MultiRenderer_slang_comp.h"

using MultiRenderer_GPU = MultiRenderer_slang_comp;

class MultiRendererApp : public MultiRenderer_GPU
{
public:

  MultiRendererApp(uint32_t AS_type, uint32_t maxPrimitives) : 
    MultiRenderer_GPU(AS_type, maxPrimitives)
  {

  }

  void UpdateVisibilityBuffer() override;
  virtual void UpdateInstances(const std::vector<uint32_t> &instanceIds, const std::vector<LiteMath::float4x4> &matrices) override;
};

static std::shared_ptr<IRenderer> CreateMultiRendererApp(uint32_t AS_type, uint32_t maxPrimitives, 
                                                         vk_utils::VulkanContext a_ctx, size_t a_maxThreadsGenerated)
{
  auto pObj = std::make_shared<MultiRendererApp>(AS_type, maxPrimitives);
  pObj->SetVulkanContext(a_ctx);
  pObj->InitVulkanObjects(a_ctx.device, a_ctx.physicalDevice, a_maxThreadsGenerated);
  return pObj;
}