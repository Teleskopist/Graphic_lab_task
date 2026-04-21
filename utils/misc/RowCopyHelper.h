#pragma once
#include "vkutils/vk_copy.h"
#include "vkutils/vk_images.h"
#include "vkutils/vk_utils.h"

struct RowCopyHelper : vk_utils::SimpleCopyHelper
{
    using SimpleCopyHelper::SimpleCopyHelper;

    void UpdateImageRow(VkImage a_image, const void *a_src, int a_width, int a_height, uint32_t a_row_size, VkImageLayout a_finalLayout)
    {

        const uint32_t size = a_row_size;

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkResetCommandBuffer(cmdBuff, 0);
        vkBeginCommandBuffer(cmdBuff, &beginInfo);

        {
            VkImageSubresourceRange range = {};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = 1;
            vk_utils::setImageLayout(cmdBuff, a_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range,
                                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        }

        {
            void *mappedMemory = nullptr;
            vkMapMemory(dev, stagingBuffMemory, 0, size, 0, &mappedMemory);
            memcpy(mappedMemory, (char *)(a_src), size);

            VkMappedMemoryRange range{};
            range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            range.pNext = nullptr;
            range.memory = stagingBuffMemory;
            range.size = size;
            range.offset = 0;
            vkFlushMappedMemoryRanges(dev, 1, &range);

            vkUnmapMemory(dev, stagingBuffMemory);
        }
        VkImageSubresourceLayers subresourceLayers = {};
        subresourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceLayers.mipLevel = 0;
        subresourceLayers.baseArrayLayer = 0;
        subresourceLayers.layerCount = 1;

        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = uint32_t(a_width);
        copyRegion.bufferImageHeight = uint32_t(a_height);
        copyRegion.imageExtent = VkExtent3D{uint32_t(a_width), uint32_t(a_height), 1};
        copyRegion.imageOffset = VkOffset3D{0, 0, 0};
        copyRegion.imageSubresource = subresourceLayers;

        vkCmdCopyBufferToImage(cmdBuff, stagingBuff, a_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        vkEndCommandBuffer(cmdBuff);

        vk_utils::executeCommandBufferNow(cmdBuff, queue, dev);

        vkResetCommandBuffer(cmdBuff, 0);
        vkBeginCommandBuffer(cmdBuff, &beginInfo);
        VkImageSubresourceRange range = {};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;
        vk_utils::setImageLayout(cmdBuff, a_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, a_finalLayout, range,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT);

        vkEndCommandBuffer(cmdBuff);
        vk_utils::executeCommandBufferNow(cmdBuff, queue, dev);
    }
};
