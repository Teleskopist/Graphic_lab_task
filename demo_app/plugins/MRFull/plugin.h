#pragma once
#include "core/serialization.h"
#include "demo_app/IRenderPlugin.h"
#include "MultiRenderer/MultiRenderer.h"
#include "MultiRenderer/app_renderer.h"
#include "MultiRenderer/MultiRenderer_slang_comp.h"
std::shared_ptr<MultiRenderer> CreateMultiRenderer_slang_comp(uint32_t AS_type, uint32_t maxPrimitives, vk_utils::VulkanContext a_ctx, size_t a_maxThreadsGenerated);
#if defined(USE_GPU_RQ)
  #include "MultiRenderer/MultiRenderer_slang_comp_rq.h"
  std::shared_ptr<MultiRenderer> CreateMultiRenderer_slang_comp_rq(uint32_t AS_type, uint32_t maxPrimitives, vk_utils::VulkanContext a_ctx, size_t a_maxThreadsGenerated);
#endif

namespace demo_app
{
  class MRFullPlugin : public IRenderPlugin
  {
  public:
    virtual VkPhysicalDeviceFeatures2 listRequiredDeviceFeatures(std::vector<const char *> &deviceExtensions) override
    {
#if defined(USE_GPU_RQ)
    std::vector<const char *> e1, e2;
    VkPhysicalDeviceFeatures2 f1 = MultiRenderer_GPU::ListRequiredDeviceFeatures(e1);
    VkPhysicalDeviceFeatures2 f2 = MultiRenderer_slang_comp_rq::ListRequiredDeviceFeatures(e2);
    deviceExtensions = LiteApp::merge_extensions(e1, e2);
    return LiteApp::merge_device_features(f1, f2);
#else
    return MultiRenderer_GPU::ListRequiredDeviceFeatures(deviceExtensions);
#endif
    }

    virtual std::shared_ptr<IRenderer> createRenderer(RenderInitSettings &settings, vk_utils::VulkanContext &a_ctx, Block &out_preset_blk) override
    {
      load_block_from_file("config/default_render_settings.blk", out_preset_blk);

#if defined(USE_GPU_RQ)
      if (settings.renderDevice == DEVICE_GPU_RQ || settings.renderDevice == DEVICE_GPU)
      {
        printf("engine uses MultiRenderer_gpu_rq (RT-mode)\n");
        return CreateMultiRenderer_slang_comp_rq(settings.ASType , settings.maxPrimitives, a_ctx, settings.maxWidth * settings.maxHeight);
      }
      else
#endif
      {
        if (settings.renderDevice != DEVICE_CPU)
        {
          printf("engine uses MultiRendererApp (compute)\n");
          return CreateMultiRendererApp(settings.ASType, settings.maxPrimitives, a_ctx, settings.maxWidth * settings.maxHeight);
        }
        else // DEVICE_CPU
        {
          printf("engine uses CPU renderer\n");
          return std::shared_ptr<IRenderer>(new MultiRenderer(settings.ASType, settings.maxPrimitives));
        }
      }
    }

    virtual void setRenderSettingsBlk(IRenderer *renderer, const Block &preset_blk) override
    {
      MultiRenderer *my_renderer = dynamic_cast<MultiRenderer*>(renderer);
      if (my_renderer)
      {
        MultiRenderPreset preset = my_renderer->GetPreset();
        load_from_blk(preset, &preset_blk);
        my_renderer->SetPreset(preset);
      }
      else
        printf("[MRFullPlugin] Failed to cast renderer to proper type\n");
    }
    virtual void initContext(UIContext &ui_ctx) override;
    virtual void drawInterface(CommandBuffer &cmd_buffer, Settings &settings, UIContext &ui_ctx) override;
    virtual void loadScene(const Block &args, const char* path, int id, UIContext &ui_ctx) override;
    virtual void executeCommand(const CommandBuffer::Command &cmd, CommandBuffer &cmd_buffer, Settings &settings, 
                                UIContext &ui_ctx, RenderingContext &r_ctx, RasterizationContext &rast_ctx, 
                                RaytracingContext &rt_ctx) override;
  };
}