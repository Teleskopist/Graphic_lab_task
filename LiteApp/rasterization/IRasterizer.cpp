#include "IRasterizer.h"
#include "LiteMath/Image2d.h"
#include "vk_pipeline.h"

namespace LiteApp
{
    IRasterizer::IRasterizer(RenderingContext &_renderingContext) : renderingContext(_renderingContext)
    {
        LoaderConfig conf = {};
        conf.load_geometry = true;
        conf.load_materials = MATERIAL_LOAD_MODE::NONE;
        sceneManager = std::make_shared<SceneManager>(renderingContext.device, renderingContext.physicalDevice, renderingContext.queueFamilyIDXs.graphics, renderingContext.pCopyHelper, conf);
    }

    IRasterizer::~IRasterizer()
    {
        ClearPipeline();
    }

    void IRasterizer::SetBackgroundColor(float3 color)
    {
        backgroundColor = to_float4(color, 1.0f);
    }

    void IRasterizer::LoadScene(const std::string &path)
    {
        sceneManager->LoadSceneXML(path, false);
    }

    static void do_recompile_shader(std::filesystem::path src, std::filesystem::path bin)
    {
        std::string exe = "glslangValidator";
        std::string cmd = exe + " -V " + src.string() + " -o " + bin.string();
        std::cout << "Executing '" << cmd << "'" << std::endl;
        system(cmd.c_str());
    }

    static bool recompile_shader(std::filesystem::path src, std::filesystem::path bin)
    {
        if (!std::filesystem::exists(bin))
        {
            do_recompile_shader(src, bin);
            return true;
        }
        auto src_t = std::filesystem::last_write_time(src);
        auto bin_t = std::filesystem::last_write_time(bin);
        if (bin_t <= src_t)
        {
            do_recompile_shader(src, bin);
            return true;
        }
        return false;
    }

    void IRasterizer::CompileShaders()
    {
        auto [vs, fs] = getShaderPaths();
        if (recompile_shader(vs, vs.string() + ".spv"))
        {
            std::cout << vs << " is recompiled" << std::endl;
        }
        if (recompile_shader(fs, fs.string() + ".spv"))
        {
            std::cout << fs << " is recompiled" << std::endl;
        }
    }

    void IRasterizer::CreatePipeline()
    {

        auto [vs, fs] = getShaderPaths();

        if (!cachedDescriptorSets)
        {
            auto descs = createDescriptorSets();
            descriptorSetLayouts = descs.layouts;
            descriptorSets = descs.sets;
            cachedDescriptorSets = true;
        }

        ClearPipeline();

        vk_utils::GraphicsPipelineMaker maker;

        std::unordered_map<VkShaderStageFlagBits, std::string> shader_paths;
        shader_paths[VK_SHADER_STAGE_FRAGMENT_BIT] = fs.string() + ".spv";
        shader_paths[VK_SHADER_STAGE_VERTEX_BIT] = vs.string() + ".spv";

        maker.LoadShaders(renderingContext.device, shader_paths);

        pipelineLayout = maker.MakeLayout(renderingContext.device, descriptorSetLayouts, pushConstantSize());
        maker.SetDefaultState(renderingContext.width, renderingContext.height);
        maker.rasterizer.polygonMode = wireFrameMode ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;

        pipeline = maker.MakePipeline(renderingContext.device, sceneManager->GetPipelineVertexInputStateCreateInfo(),
                                      renderingContext.screenRenderPass, {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR});
    }

    void IRasterizer::ClearPipeline()
    {
        if (pipeline)
        {
            vkDestroyPipeline(renderingContext.device, pipeline, VK_NULL_HANDLE);
            pipeline = nullptr;
        }
        if (pipelineLayout)
        {
            vkDestroyPipelineLayout(renderingContext.device, pipelineLayout, VK_NULL_HANDLE);
            pipelineLayout = nullptr;
        }
    }

    void IRasterizer::RenderCmd(VkCommandBuffer cmdBuff, VkFramebuffer frameBuff)
    {
        vkResetCommandBuffer(cmdBuff, 0);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuff, &beginInfo));

        vk_utils::setDefaultViewport(cmdBuff, static_cast<float>(renderingContext.width), static_cast<float>(renderingContext.height));
        vk_utils::setDefaultScissor(cmdBuff, renderingContext.width, renderingContext.height);

        {
            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderingContext.screenRenderPass;
            renderPassInfo.framebuffer = frameBuff;
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = renderingContext.swapchain.GetExtent();

            VkClearValue clearValues[2] = {};
            clearValues[0].color = {backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w};
            clearValues[1].depthStencil = {1.0f, 0};
            renderPassInfo.clearValueCount = 2;
            renderPassInfo.pClearValues = &clearValues[0];

            vkCmdBeginRenderPass(cmdBuff, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

            if (descriptorSets.size() > 0)
            {
                vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSets.size(),
                                        descriptorSets.data(), 0, VK_NULL_HANDLE);
            }
            VkShaderStageFlags stageFlags = (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

            VkDeviceSize zero_offset = 0u;
            VkBuffer vertexBuf = sceneManager->GetVertexBuffer();
            VkBuffer indexBuf = sceneManager->GetIndexBuffer();

            vkCmdBindVertexBuffers(cmdBuff, 0, 1, &vertexBuf, &zero_offset);
            vkCmdBindIndexBuffer(cmdBuff, indexBuf, 0, VK_INDEX_TYPE_UINT32);

            for (uint32_t i = 0; i < sceneManager->InstancesNum(); ++i)
            {
                auto inst = sceneManager->GetInstanceInfo(i);

                LiteMath::float4x4 model = sceneManager->GetInstanceMatrix(i);
                pushConstantUpdateModelTransform(model);

                vkCmdPushConstants(cmdBuff, pipelineLayout, stageFlags, 0,
                                   pushConstantSize(), pushConstantData());

                auto mesh_info = sceneManager->GetMeshInfo(inst.mesh_id);
                vkCmdDrawIndexed(cmdBuff, mesh_info.m_indNum, 1, mesh_info.m_indexOffset, mesh_info.m_vertexOffset, 0);
            }

            vkCmdEndRenderPass(cmdBuff);
        }

        VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuff));
    }

    bool IRasterizer::NeedScreenshot() { return screenshotMode; }

    // Source for the copy is the last rendered swapchain image
    void IRasterizer::PrepareScreenshot(const std::string &savePath)
    {
        screenshotMode = true;
        screenshotPath = savePath;
    }

    void IRasterizer::TakeScreenshotCMD(VkImage srcImage)
    {
        bool supportsBlit = true;

        // Check blit support for source and destination
        VkFormatProperties formatProps;

        // Check if the device supports blitting from optimal images (the swapchain images are in optimal format)
        vkGetPhysicalDeviceFormatProperties(renderingContext.physicalDevice, renderingContext.swapchain.GetFormat(), &formatProps);
        if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
        {
            std::cerr << "1. Device does not support blitting from optimal tiled images, using copy instead of blit!" << std::endl;
            supportsBlit = false;
        }

        // Check if the device supports blitting to linear images
        vkGetPhysicalDeviceFormatProperties(renderingContext.physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
        if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT))
        {
            std::cerr << "2. Device does not support blitting to linear tiled images, using copy instead of blit!" << std::endl;
            supportsBlit = false;
        }

        // Create the linear tiled destination image to copy to and to read the memory from
        VkImageCreateInfo imageCreateCI{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageCreateCI.imageType = VK_IMAGE_TYPE_2D;
        imageCreateCI.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageCreateCI.extent.width = renderingContext.swapchain.GetExtent().width;
        imageCreateCI.extent.height = renderingContext.swapchain.GetExtent().height;
        imageCreateCI.extent.depth = 1;
        imageCreateCI.arrayLayers = 1;
        imageCreateCI.mipLevels = 1;
        imageCreateCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateCI.tiling = VK_IMAGE_TILING_LINEAR;
        imageCreateCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        // Create the image
        VK_CHECK_RESULT(vkCreateImage(renderingContext.device, &imageCreateCI, nullptr, &dstImage));

        // Create memory to back up the image
        VkMemoryRequirements memRequirements;
        VkMemoryAllocateInfo memAllocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        vkGetImageMemoryRequirements(renderingContext.device, dstImage, &memRequirements);
        memAllocInfo.allocationSize = memRequirements.size;

        // Memory must be host visible to copy from
        memAllocInfo.memoryTypeIndex = vk_utils::findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, renderingContext.physicalDevice);
        VK_CHECK_RESULT(vkAllocateMemory(renderingContext.device, &memAllocInfo, nullptr, &dstImageMemory));
        VK_CHECK_RESULT(vkBindImageMemory(renderingContext.device, dstImage, dstImageMemory, 0));

        // Do the actual blit from the swapchain image to our host visible destination image
        VkCommandBuffer copyCmd = vk_utils::createCommandBuffer(renderingContext.device, renderingContext.commandPool);
        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

        VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &beginInfo));

        // Transition destination image to transfer destination layout
        {
            VkImageMemoryBarrier imageMemoryBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.image = dstImage;
            imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            vkCmdPipelineBarrier(
                copyCmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);
        }

        // Transition swapchain image from present to transfer source layout
        {
            VkImageMemoryBarrier imageMemoryBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.image = srcImage;
            imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            vkCmdPipelineBarrier(
                copyCmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);
        }

        // If source and destination support blit we'll blit as this also does automatic format conversion (e.g. from BGR to RGB)
        if (supportsBlit)
        {
            // Define the region to blit (we will blit the whole swapchain image)
            VkOffset3D blitSize;
            blitSize.x = renderingContext.swapchain.GetExtent().width;
            blitSize.y = renderingContext.swapchain.GetExtent().height;
            blitSize.z = 1;
            VkImageBlit imageBlitRegion{};
            imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlitRegion.srcSubresource.layerCount = 1;
            imageBlitRegion.srcOffsets[1] = blitSize;
            imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlitRegion.dstSubresource.layerCount = 1;
            imageBlitRegion.dstOffsets[1] = blitSize;

            // Issue the blit command
            vkCmdBlitImage(
                copyCmd,
                srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &imageBlitRegion,
                VK_FILTER_NEAREST);
        }
        else
        {
            // Otherwise use image copy (requires us to manually flip components)
            VkImageCopy imageCopyRegion{};
            imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageCopyRegion.srcSubresource.layerCount = 1;
            imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageCopyRegion.dstSubresource.layerCount = 1;
            imageCopyRegion.extent.width = renderingContext.swapchain.GetExtent().width;
            imageCopyRegion.extent.height = renderingContext.swapchain.GetExtent().height;
            imageCopyRegion.extent.depth = 1;

            // Issue the copy command
            vkCmdCopyImage(
                copyCmd,
                srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &imageCopyRegion);
        }

        // Transition destination image to general layout, which is the required layout for mapping the image memory later on
        {
            VkImageMemoryBarrier imageMemoryBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageMemoryBarrier.image = dstImage;
            imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            vkCmdPipelineBarrier(
                copyCmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);
        }

        // Transition back the swap chain image after the blit is done
        {
            VkImageMemoryBarrier imageMemoryBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            imageMemoryBarrier.image = srcImage;
            imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            vkCmdPipelineBarrier(
                copyCmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);
        }

        VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &copyCmd;

        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        VkFence fence;
        VK_CHECK_RESULT(vkCreateFence(renderingContext.device, &fenceInfo, nullptr, &fence));
        // Submit to the queue
        VK_CHECK_RESULT(vkQueueSubmit(renderingContext.queue, 1, &submitInfo, fence));
        // Wait for the fence to signal that command buffer has finished executing
        VK_CHECK_RESULT(vkWaitForFences(renderingContext.device, 1, &fence, VK_TRUE, UINT64_MAX));
        vkDestroyFence(renderingContext.device, fence, nullptr);
        vkFreeCommandBuffers(renderingContext.device, renderingContext.commandPool, 1, &copyCmd);

        //// Save screenshot

        // Get layout of the image (including row pitch)
        VkImageSubresource subResource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
        VkSubresourceLayout subResourceLayout;
        vkGetImageSubresourceLayout(renderingContext.device, dstImage, &subResource, &subResourceLayout);

        // Map image memory so we can start copying from it
        const char *data;
        vkMapMemory(renderingContext.device, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void **)&data);
        data += subResourceLayout.offset;

        LiteImage::Image2D<LiteMath::uchar4> screenshot{renderingContext.swapchain.GetExtent().width,
                                              renderingContext.swapchain.GetExtent().height};

        bool colorSwizzle = false;
        // Check if source is BGR
        // Note: Not complete, only contains most common and basic BGR surface formats for demonstration purposes
        if (!supportsBlit)
        {
            std::vector<VkFormat> formatsBGR = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM};
            colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), renderingContext.swapchain.GetFormat()) != formatsBGR.end());
        }

        for (uint32_t j = 0; j < renderingContext.swapchain.GetExtent().height; j++)
        {
            uint32_t *row = (uint32_t *)data;
            for (uint32_t i = 0; i < renderingContext.swapchain.GetExtent().width; i++)
            {
                LiteMath::uchar4 &pixel = screenshot[LiteMath::uint2{i, j}];
                pixel.x = *((char *)row);
                pixel.y = *((char *)row + 1);
                pixel.z = *((char *)row + 2);
                pixel.w = 255;
                if (colorSwizzle)
                    std::swap(pixel.x, pixel.z);

                row++;
            }
            data += subResourceLayout.rowPitch;
        }
        LiteImage::SaveImage(screenshotPath.c_str(), screenshot);

        // Clean up resources
        vkUnmapMemory(renderingContext.device, dstImageMemory);
        vkFreeMemory(renderingContext.device, dstImageMemory, nullptr);
        vkDestroyImage(renderingContext.device, dstImage, nullptr);
        screenshotMode = false;
    }
}
