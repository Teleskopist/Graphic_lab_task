#include "app.h"
#include "app_internal.h"
#include "volk.h"
#include "vk_utils.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "PostEffects/antialiasing/Antialiaser_slang.h"
#include "PostEffects/denoising/Denoiser_glsl.h"
#include "PostEffects/image_metrics/ImgMetricsKernel_glsl.h"
#include "core/scene_extension.h"
#include "core/serialization.h"
#include "IRenderPlugin.h"

using Antialiaser_GPU = Antialiaser_slang;
using Denoiser_GPU = Denoiser_glsl;
using ImgMetricsKernel_gpu = ImgMetricsKernel_glsl;

#include <cstdio>
#include <chrono>

namespace demo_app
{
  using namespace LiteMath;
  void raytrace_CPU(RTLayout rtLayout, UIContext &ui_ctx, const RenderingContext &r_ctx, RaytracingContext &rt_ctx)
  {
    auto t1 = std::chrono::high_resolution_clock::now();
    for (uint32_t rtId : get_active_RT_list(rtLayout))
    {
      rt_ctx.pRayTracers[rtId]->UpdateCamera(r_ctx.worldViewMatrix, OpenglToVulkanProjectionMatrixFix() * r_ctx.projectionMatrix);
      if (ui_ctx.enableAA || ui_ctx.enableDenoiser)
      {
        rt_ctx.pRayTracers[rtId]->RenderFloat(rt_ctx.raytracedImagesDataRaw[rtId].data(), r_ctx.width, r_ctx.height);
        if (ui_ctx.enableAA && ui_ctx.enableDenoiser) {
          rt_ctx.pDenoiser[rtId]->DenoiseFloat(r_ctx.width, r_ctx.height, rt_ctx.raytracedImagesDataRaw[rtId].data(), rt_ctx.raytracedImagesDataNoAA[rtId].data());
          rt_ctx.pAntialiaser[rtId]->AntialiasAfterDenoiser(r_ctx.width, r_ctx.height, rt_ctx.raytracedImagesDataNoAA[rtId].data(), rt_ctx.raytracedImagesData[rtId].data());
        } else if (ui_ctx.enableAA) {
          rt_ctx.pAntialiaser[rtId]->Antialias(r_ctx.width, r_ctx.height, rt_ctx.raytracedImagesDataRaw[rtId].data(), rt_ctx.raytracedImagesData[rtId].data());
        } else {
          rt_ctx.pDenoiser[rtId]->Denoise(r_ctx.width, r_ctx.height, rt_ctx.raytracedImagesDataRaw[rtId].data(), rt_ctx.raytracedImagesData[rtId].data());
        }
      }
      else
        rt_ctx.pRayTracers[rtId]->Render(rt_ctx.raytracedImagesData[rtId].data(), r_ctx.width, r_ctx.height);
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    ui_ctx.timeSlidingWindow[ui_ctx.currentTimeIndex] = std::chrono::duration<float, std::milli>(t2 - t1).count();
    
    r_ctx.pCopyHelper->UpdateImage(r_ctx.rt_pres.rtImage.image, rt_ctx.raytracedImagesData[rtLayout.primaryRTId].data(), r_ctx.width, r_ctx.height, 4,
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }

  void raytrace_GPU(RTLayout rtLayout, UIContext &ui_ctx, const RenderingContext &r_ctx, RaytracingContext &rt_ctx)
  {    
    auto t1 = std::chrono::high_resolution_clock::now();

    for (uint32_t rtId : get_active_RT_list(rtLayout))
    {
      rt_ctx.pRayTracers[rtId]->UpdateCamera(r_ctx.worldViewMatrix, OpenglToVulkanProjectionMatrixFix() * r_ctx.projectionMatrix);
      rt_ctx.pRayTracers[rtId]->UpdatePlainMembers(r_ctx.pCopyHelper);
      dynamic_cast<Antialiaser_GPU *>(rt_ctx.pAntialiaser[rtId].get())->UpdatePlainMembers(r_ctx.pCopyHelper);
      dynamic_cast<Denoiser_GPU *>(rt_ctx.pDenoiser[rtId].get())->UpdatePlainMembers(r_ctx.pCopyHelper);
    }

    // do ray tracing
    VkCommandBuffer commandBuffer = vk_utils::createCommandBuffer(r_ctx.device, r_ctx.commandPool);
    VkCommandBufferBeginInfo beginCommandBufferInfo = {};
    beginCommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginCommandBufferInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginCommandBufferInfo);
  
    for (uint32_t rtId : get_active_RT_list(rtLayout))
    {
      if (ui_ctx.enableAA || ui_ctx.enableDenoiser)
      {
        rt_ctx.pRayTracers[rtId]->RenderFloatCmd(commandBuffer, nullptr, r_ctx.width, r_ctx.height);
        if (ui_ctx.enableAA && ui_ctx.enableDenoiser) {
          dynamic_cast<Denoiser_GPU *>(rt_ctx.pDenoiser[rtId].get())->DenoiseFloatCmd(commandBuffer, r_ctx.width, r_ctx.height, nullptr, nullptr);
          dynamic_cast<Antialiaser_GPU *>(rt_ctx.pAntialiaser[rtId].get())->AntialiasAfterDenoiserCmd(commandBuffer, r_ctx.width, r_ctx.height, nullptr, nullptr);
        } else if (ui_ctx.enableAA) {
          dynamic_cast<Antialiaser_GPU *>(rt_ctx.pAntialiaser[rtId].get())->AntialiasCmd(commandBuffer, r_ctx.width, r_ctx.height, nullptr, nullptr);
        } else {
          dynamic_cast<Denoiser_GPU *>(rt_ctx.pDenoiser[rtId].get())->DenoiseCmd(commandBuffer, r_ctx.width, r_ctx.height, nullptr, nullptr);
        }
      }
      else
        rt_ctx.pRayTracers[rtId]->RenderCmd(commandBuffer, nullptr, r_ctx.width, r_ctx.height);
    }

    auto t2 = std::chrono::high_resolution_clock::now();

    // prepare buffer and image for copy command
    {
      VkBufferMemoryBarrier transferBuff = {};

      transferBuff.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      transferBuff.pNext = nullptr;
      transferBuff.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      transferBuff.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      transferBuff.size = VK_WHOLE_SIZE;
      transferBuff.offset = 0;
      transferBuff.buffer = rt_ctx.genColorBuffers[rtLayout.primaryRTId];
      transferBuff.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
      transferBuff.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

      VkImageMemoryBarrier transferImage;
      transferImage.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      transferImage.pNext = nullptr;
      transferImage.srcAccessMask = 0;
      transferImage.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      transferImage.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      transferImage.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      transferImage.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      transferImage.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      transferImage.image = r_ctx.rt_pres.rtImage.image;

      transferImage.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      transferImage.subresourceRange.baseMipLevel = 0;
      transferImage.subresourceRange.baseArrayLayer = 0;
      transferImage.subresourceRange.layerCount = 1;
      transferImage.subresourceRange.levelCount = 1;

      vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &transferBuff, 1, &transferImage);
    }

    // execute copy
    //
    {
      VkImageSubresourceLayers subresourceLayers = {};
      subresourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      subresourceLayers.mipLevel = 0;
      subresourceLayers.baseArrayLayer = 0;
      subresourceLayers.layerCount = 1;

      VkBufferImageCopy copyRegion = {};
      copyRegion.bufferOffset = 0;
      copyRegion.bufferRowLength = uint32_t(r_ctx.width);
      copyRegion.bufferImageHeight = uint32_t(r_ctx.height);
      copyRegion.imageExtent = VkExtent3D{uint32_t(r_ctx.width), uint32_t(r_ctx.height), 1};
      copyRegion.imageOffset = VkOffset3D{0, 0, 0};
      copyRegion.imageSubresource = subresourceLayers;

      vkCmdCopyBufferToImage(commandBuffer, rt_ctx.genColorBuffers[rtLayout.primaryRTId], r_ctx.rt_pres.rtImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
    }

    // get back normal image layout
    {
      VkImageMemoryBarrier transferImage;
      transferImage.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      transferImage.pNext = nullptr;
      transferImage.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      transferImage.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      transferImage.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      transferImage.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      transferImage.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      transferImage.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      transferImage.image = r_ctx.rt_pres.rtImage.image;

      transferImage.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      transferImage.subresourceRange.baseMipLevel = 0;
      transferImage.subresourceRange.baseArrayLayer = 0;
      transferImage.subresourceRange.layerCount = 1;
      transferImage.subresourceRange.levelCount = 1;

      vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &transferImage);
    }

    vkEndCommandBuffer(commandBuffer);

    auto t3 = std::chrono::high_resolution_clock::now();

    vk_utils::executeCommandBufferNow(commandBuffer, r_ctx.graphicsQueue, r_ctx.device);
    vkFreeCommandBuffers(r_ctx.device, r_ctx.commandPool, 1, &commandBuffer);

    auto t4 = std::chrono::high_resolution_clock::now();

    float time_1 = std::chrono::duration<float, std::milli>(t2 - t1).count();
    float time_2 = std::chrono::duration<float, std::milli>(t3 - t2).count();
    float time_3 = std::chrono::duration<float, std::milli>(t4 - t3).count();

    ui_ctx.averageRTTransferTime = (1 - ui_ctx.averagingFactor) * time_1 + ui_ctx.averagingFactor * ui_ctx.averageRTTransferTime;
    ui_ctx.averageRTPrepareTime = (1 - ui_ctx.averagingFactor) * time_2 + ui_ctx.averagingFactor * ui_ctx.averageRTPrepareTime;
    ui_ctx.averageRTExecuteTime = (1 - ui_ctx.averagingFactor) * time_3 + ui_ctx.averagingFactor * ui_ctx.averageRTExecuteTime;

    ui_ctx.timeSlidingWindow[ui_ctx.currentTimeIndex] = time_3;
  }

  void on_screen_resolution_change_RT(const RenderingContext &r_ctx, RaytracingContext &rt_ctx)
  {
    for (int i = 0; i < MAX_RENDERERS; i++)
    {
      if (!rt_ctx.pRayTracers[i])
        continue;

      rt_ctx.pRayTracers[i]->SetViewport(0, 0, r_ctx.width, r_ctx.height);
      rt_ctx.pRayTracers[i]->CommitDeviceData();
      rt_ctx.pAntialiaser[i]->CommitDeviceData();
      rt_ctx.pDenoiser[i]->CommitDeviceData();
      if (r_ctx.renderDevice != DEVICE_CPU)
      {
        rt_ctx.genColorRawBuffers[i] = vk_utils::createBuffer(r_ctx.device, r_ctx.width * r_ctx.height * sizeof(LiteMath::float4),
                                                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        rt_ctx.genColorNoAABuffers[i] = vk_utils::createBuffer(r_ctx.device, r_ctx.width * r_ctx.height * sizeof(LiteMath::float4),
                                                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        rt_ctx.genColorBuffers[i] = vk_utils::createBuffer(r_ctx.device, r_ctx.width * r_ctx.height * sizeof(uint32_t),
                                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        rt_ctx.colorMems[i] = vk_utils::allocateAndBindWithPadding(r_ctx.device, r_ctx.physicalDevice, {rt_ctx.genColorRawBuffers[i], rt_ctx.genColorNoAABuffers[i], rt_ctx.genColorBuffers[i]});

        rt_ctx.pRayTracers[i]->SetVulkanInOutFor_RenderFloat(rt_ctx.genColorRawBuffers[i], 0);
        dynamic_cast<Antialiaser_GPU *>(rt_ctx.pAntialiaser[i].get())->SetVulkanInOutFor_Antialias(rt_ctx.genColorRawBuffers[i], 0, rt_ctx.genColorBuffers[i], 0);
        dynamic_cast<Denoiser_GPU *>(rt_ctx.pDenoiser[i].get())->SetVulkanInOutFor_DenoiseFloat(rt_ctx.genColorRawBuffers[i], 0, rt_ctx.genColorNoAABuffers[i], 0);
        dynamic_cast<Antialiaser_GPU *>(rt_ctx.pAntialiaser[i].get())->SetVulkanInOutFor_AntialiasAfterDenoiser(rt_ctx.genColorNoAABuffers[i], 0, rt_ctx.genColorBuffers[i], 0);
        dynamic_cast<Denoiser_GPU *>(rt_ctx.pDenoiser[i].get())->SetVulkanInOutFor_Denoise(rt_ctx.genColorRawBuffers[i], 0, rt_ctx.genColorBuffers[i], 0);
        rt_ctx.pRayTracers[i]->SetVulkanInOutFor_Render(rt_ctx.genColorBuffers[i], 0);
      }
      rt_ctx.pRayTracers[i]->Clear(r_ctx.width, r_ctx.height);
    }

    if (r_ctx.renderDevice != DEVICE_CPU)
    {
      rt_ctx.pMetricsKernel->CommitDeviceData();
      dynamic_cast<ImgMetricsKernel_gpu*>(rt_ctx.pMetricsKernel.get())->SetVulkanInOutFor_CalcArraySumm(
        rt_ctx.genColorBuffers[0], 0, rt_ctx.genColorBuffers[1], 0);
    }
  }

  void setup_RT_scene(UIContext&ui_ctx, LiteScene::HydraScene &scene, uint32_t rtId, const Settings &settings, RenderingContext &r_ctx, RaytracingContext &rt_ctx)
  {
    vk_utils::VulkanContext a_ctx;
    a_ctx.instance = r_ctx.instance;
    a_ctx.device = r_ctx.device;
    a_ctx.physicalDevice = r_ctx.physicalDevice;
    a_ctx.commandPool = r_ctx.commandPool;
    a_ctx.computeQueue = r_ctx.computeQueue;
    a_ctx.transferQueue = r_ctx.transferQueue;

    a_ctx.pCopyHelper = r_ctx.pCopyHelper;
    a_ctx.pAllocatorCommon = nullptr;
    a_ctx.pAllocatorSpecial = r_ctx.pAllocatorSpecial;

    IRenderPlugin::RenderInitSettings init_s;
    init_s.ASType = AS_TYPE_COMMON;
    init_s.maxHeight = r_ctx.height;
    init_s.maxWidth = r_ctx.width;
    init_s.maxPrimitives = scene.get_total_number_of_primitives();
    init_s.renderDevice = r_ctx.renderDevice;

    assert(ui_ctx.activePlugin);
    ui_ctx.RTPresetBlk.clear();
    rt_ctx.pRayTracers[rtId] = ui_ctx.activePlugin->createRenderer(init_s, a_ctx, ui_ctx.RTPresetBlk);

    if (r_ctx.renderDevice == DEVICE_CPU)
    {
      rt_ctx.pAntialiaser[rtId] = std::shared_ptr<Antialiaser>(new Antialiaser());
      rt_ctx.pAntialiaser[rtId]->m_AABuffer.resize(r_ctx.width * r_ctx.height);
      rt_ctx.pAntialiaser[rtId]->m_AABuffer1.resize(r_ctx.width * r_ctx.height);

      rt_ctx.pDenoiser[rtId] = std::shared_ptr<Denoiser>(new Denoiser());

      rt_ctx.pMetricsKernel = std::make_shared<ImgMetricsKernel>();
    }
    else
    {
      rt_ctx.pAntialiaser[rtId] = std::make_shared<Antialiaser_GPU>();
      dynamic_cast<Antialiaser_GPU *>(rt_ctx.pAntialiaser[rtId].get())->SetVulkanContext(a_ctx);
      dynamic_cast<Antialiaser_GPU *>(rt_ctx.pAntialiaser[rtId].get())->InitVulkanObjects(a_ctx.device, a_ctx.physicalDevice, r_ctx.width * r_ctx.height);
      rt_ctx.pAntialiaser[rtId]->m_AABuffer.resize(r_ctx.width * r_ctx.height);
      rt_ctx.pAntialiaser[rtId]->m_AABuffer1.resize(r_ctx.width * r_ctx.height);

      rt_ctx.pDenoiser[rtId] = std::make_shared<Denoiser_GPU>();
      dynamic_cast<Denoiser_GPU *>(rt_ctx.pDenoiser[rtId].get())->SetVulkanContext(a_ctx);
      dynamic_cast<Denoiser_GPU *>(rt_ctx.pDenoiser[rtId].get())->InitVulkanObjects(a_ctx.device, a_ctx.physicalDevice, r_ctx.width * r_ctx.height);

      rt_ctx.pMetricsKernel = std::make_shared<ImgMetricsKernel_gpu>();
      dynamic_cast<ImgMetricsKernel_gpu*>(rt_ctx.pMetricsKernel.get())->SetVulkanContext(a_ctx);
      dynamic_cast<ImgMetricsKernel_gpu*>(rt_ctx.pMetricsKernel.get())->InitVulkanObjects(a_ctx.device, a_ctx.physicalDevice, r_ctx.width * r_ctx.height);
    }

    rt_ctx.pRayTracers[rtId]->LoadScene(scene);
    if (rt_ctx.pRayTracers[rtId]->GetAccelStruct())
      rt_ctx.pRayTracers[rtId]->GetAccelStruct()->CommitScene();

    float memory = rt_ctx.pRayTracers[rtId]->getSceneSize() * (1.f / (1024 * 1024));
    ui_ctx.scenesCtx[rtId].memoryInRenderer = rt_ctx.pRayTracers[rtId]->getSceneSize();
    printf("Scene %d takes: %.1f MB\n", rtId, memory);
  }

  void setup_RT_image(RenderingContext &r_ctx, RaytracingContext &rt_ctx)
  {
    for (int i = 0; i < MAX_RENDERERS; i++)
    {
      rt_ctx.raytracedImagesData[i].resize(r_ctx.width * r_ctx.height);
      rt_ctx.raytracedImagesDataRaw[i].resize(r_ctx.width * r_ctx.height);
      rt_ctx.raytracedImagesDataNoAA[i].resize(r_ctx.width * r_ctx.height);
    }
  }

  void raytrace(RTLayout rtLayout, UIContext &ui_ctx, const RenderingContext &r_ctx, RaytracingContext &rt_ctx)
  {
    if (r_ctx.renderDevice != DEVICE_CPU)
      raytrace_GPU(rtLayout, ui_ctx, r_ctx, rt_ctx);
    else
      raytrace_CPU(rtLayout, ui_ctx, r_ctx, rt_ctx);
  }

  void update_antialiasing_settings(const Settings &settings, UIContext &ui_ctx, RenderingContext &r_ctx, RaytracingContext &rt_ctx)
  {
    for (int i = 0; i < MAX_RENDERERS; i++)
    {
      if (rt_ctx.pDenoiser[i]) {
        if (ui_ctx.enableDenoiser) {
          rt_ctx.pDenoiser[i]->m_SigmaR = ui_ctx.sigmaR;
          rt_ctx.pDenoiser[i]->m_SigmaS = ui_ctx.sigmaS;
          rt_ctx.pDenoiser[i]->m_KernelSize = ui_ctx.kernelSize;
        }
      }

      if (rt_ctx.pAntialiaser[i])
      {
        if (!ui_ctx.enableAA) {
          ui_ctx.AAFrameNum = 0;
          rt_ctx.pAntialiaser[i]->m_AAFactor = 1.0f;
          rt_ctx.pAntialiaser[i]->m_AAFrameNum = 0;
        }
        else {
          ui_ctx.AAFrameNum = ++rt_ctx.pAntialiaser[i]->m_AAFrameNum;
          if (ui_ctx.AAFrameNum == 1) {
            rt_ctx.pAntialiaser[i]->m_AAFactor = 1.0f;
            rt_ctx.pAntialiaser[i]->m_currentMatrix = OpenglToVulkanProjectionMatrixFix() * r_ctx.projectionMatrix * r_ctx.worldViewMatrix;
            rt_ctx.pAntialiaser[i]->m_invProj = inverse4x4(OpenglToVulkanProjectionMatrixFix() * r_ctx.projectionMatrix);
            rt_ctx.pAntialiaser[i]->m_invWorldView = inverse4x4(r_ctx.worldViewMatrix);
            rt_ctx.pAntialiaser[i]->m_previousMatrix = rt_ctx.pAntialiaser[i]->m_currentMatrix;
          } else {
            rt_ctx.pAntialiaser[i]->m_AAFactor = settings.AAFactor;
            rt_ctx.pAntialiaser[i]->m_previousMatrix = rt_ctx.pAntialiaser[i]->m_currentMatrix;
            if (settings.cameraMoving) {
              rt_ctx.pAntialiaser[i]->m_currentMatrix = OpenglToVulkanProjectionMatrixFix() * r_ctx.projectionMatrix * r_ctx.worldViewMatrix;
              rt_ctx.pAntialiaser[i]->m_invProj = inverse4x4(OpenglToVulkanProjectionMatrixFix() * r_ctx.projectionMatrix);
              rt_ctx.pAntialiaser[i]->m_invWorldView = inverse4x4(r_ctx.worldViewMatrix);
            }
          }
        }
        rt_ctx.pRayTracers[i]->SetFrameNum(ui_ctx.AAFrameNum);
      }
    }
  }
}