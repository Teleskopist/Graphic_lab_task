#include "BCImageTerrainRasterizer.h"

#ifdef USE_KTX
#include "utils/misc/block_compression.h"
#endif

namespace demo_app
{

    const char *BCImageTerrainRasterizer::Name() const
    {
        return "Terrain (BC)";
    }
#ifdef USE_KTX
    void BCImageTerrainRasterizer::LoadImage(vk_utils::VulkanImageMem &image, const float *data, uint32_t width, uint32_t height)
    {
        std::vector<uint8_t> data1(width * height);
        for (int i = 0; i < data1.size(); i++)
            data1[i] = std::clamp(data[i] * 255, 0.0f, 255.0f);
        std::vector<uint8_t> compressed;
        VkFormat compressed_format = block_compression::compress_image(compressed, data1.data(), width, height, 1);

        image.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vk_utils::createImgAllocAndBind(
            renderingContext.device,
            renderingContext.physicalDevice,
            width,
            height,
            compressed_format,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            &image);
        assert(image.image);
        //  renderingContext.pCopyHelper->UpdateImage(image, data, width, height, sizeof(float), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        block_compression::copy_compressed_image(
            image.image,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            compressed.data(),
            width,
            height,
            1,
            compressed.size(),
            renderingContext.device,
            renderingContext.physicalDevice,
            renderingContext.transferQueue,
            renderingContext.queueFamilyIDXs.transfer);
    }
#endif

}
