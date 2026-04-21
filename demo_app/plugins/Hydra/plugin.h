#pragma once
#include "HydraRenderer/renderer.h"
#include "demo_app/IRenderPlugin.h"
//#include "HydraCore3/Integrator_glsl_litert.h"
namespace demo_app
{
  class HydraPlugin : public IRenderPlugin
  {
  public:
    virtual VkPhysicalDeviceFeatures2 listRequiredDeviceFeatures(std::vector<const char *> &deviceExtensions) override
    {
      return VkPhysicalDeviceFeatures2{};
      //return Integrator_glsl_litert::ListRequiredDeviceFeatures(deviceExtensions);
    }

    virtual std::shared_ptr<IRenderer> createRenderer(RenderInitSettings &settings, vk_utils::VulkanContext &a_ctx, 
                                                      Block &out_preset_blk) override
    {
      //if (settings.renderDevice == DEVICE_CPU)
      {
        auto pObj = std::make_shared<HydraRenderer>();
        HydraRenderPreset preset = pObj->GetPreset();
        save_to_blk(preset, &out_preset_blk);
        return pObj;
      }
      // else
      // {
      //   auto pObj = std::make_shared<GPURenderer>();
      //   pObj->SetVulkanContext(a_ctx);
      //   pObj->InitVulkanObjects(a_ctx.device, a_ctx.physicalDevice, settings.maxWidth * settings.maxHeight); 
      //   RenderPreset preset = pObj->GetPreset();
      //   save_to_blk(preset, &out_preset_blk);
      //   return pObj;
      // }
    }

    virtual void setRenderSettingsBlk(IRenderer *renderer, const Block &preset_blk) override
    {
      HydraRenderer *my_renderer = dynamic_cast<HydraRenderer*>(renderer);
      if (my_renderer)
      {
        HydraRenderPreset preset = my_renderer->GetPreset();
        load_from_blk(preset, &preset_blk);
        my_renderer->SetPreset(preset);
      }
      else
        printf("[HydraPlugin] Failed to cast renderer to proper type\n");
    }
  };
}