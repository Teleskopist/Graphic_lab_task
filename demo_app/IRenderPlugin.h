#pragma once
#include "core/IRenderer.h"
#include "app.h"
#include "vk_context.h"

namespace demo_app
{
  class IRenderPlugin
  {
  public:
    struct RenderInitSettings
    {
      uint32_t ASType = AS_TYPE_COMMON;
      uint32_t renderDevice = DEVICE_GPU;
      uint32_t maxPrimitives = 0;
      uint32_t maxWidth  = 4096;
      uint32_t maxHeight = 4096;
    };

    virtual ~IRenderPlugin() {}
    virtual VkPhysicalDeviceFeatures2 listRequiredDeviceFeatures(std::vector<const char *> &deviceExtensions) = 0;
    virtual std::shared_ptr<IRenderer> createRenderer(RenderInitSettings &settings, vk_utils::VulkanContext &a_ctx, Block &out_preset_blk) = 0;
    virtual void setRenderSettingsBlk(IRenderer *renderer, const Block &preset_blk) = 0;

    virtual void initContext(UIContext &ui_ctx) { }
    virtual void drawInterface(CommandBuffer &cmd_buffer, Settings &settings, UIContext &ui_ctx) { }
    virtual void loadScene(const Block &args, const char* path, int id, UIContext &ui_ctx) {}
    virtual void executeCommand(const CommandBuffer::Command &cmd, CommandBuffer &cmd_buffer, Settings &settings, 
                                UIContext &ui_ctx, RenderingContext &r_ctx, RasterizationContext &rast_ctx, 
                                RaytracingContext &rt_ctx) { }
  };

  template <typename Renderer,
            typename RenderPreset,
            typename GPURenderer>
  class DemoPlugin : public IRenderPlugin
  {
  public:
    virtual VkPhysicalDeviceFeatures2 listRequiredDeviceFeatures(std::vector<const char *> &deviceExtensions) override
    {
      return GPURenderer::ListRequiredDeviceFeatures(deviceExtensions);
    }
    virtual std::shared_ptr<IRenderer> createRenderer(RenderInitSettings &settings, vk_utils::VulkanContext &a_ctx, 
                                                      Block &out_preset_blk) override
    {
      if (settings.renderDevice == DEVICE_CPU)
      {
        auto pObj = std::make_shared<Renderer>();
        RenderPreset preset = pObj->GetPreset();
        save_to_blk(preset, &out_preset_blk);
        return pObj;
      }
      else
      {
        auto pObj = std::make_shared<GPURenderer>();
        pObj->SetVulkanContext(a_ctx);
        pObj->InitVulkanObjects(a_ctx.device, a_ctx.physicalDevice, settings.maxWidth * settings.maxHeight); 
        RenderPreset preset = pObj->GetPreset();
        save_to_blk(preset, &out_preset_blk);
        return pObj;
      }
    }

    virtual void setRenderSettingsBlk(IRenderer *renderer, const Block &preset_blk) override
    {
      Renderer *my_renderer = dynamic_cast<Renderer*>(renderer);
      if (my_renderer)
      {
        RenderPreset preset = my_renderer->GetPreset();
        load_from_blk(preset, &preset_blk);
        my_renderer->SetPreset(preset);
      }
      else
        printf("[DemoPlugin] Failed to cast renderer to proper type\n");
    }
  };
}