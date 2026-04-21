#include "ImageTerrainRasterizer.h"

namespace demo_app
{

    ImageTerrainRasterizer::ImageTerrainRasterizer(RenderingContext &ctx) : TerrainRasterizer(ctx)
    {
    }

    ImageTerrainRasterizer::~ImageTerrainRasterizer()
    {
        if (image.image)
            vk_utils::deleteImg(renderingContext.device, &image);
        if (imageSampler)
            vkDestroySampler(renderingContext.device, imageSampler, nullptr);
    }

    const char *ImageTerrainRasterizer::Name() const
    {
        return "Terrain (Image)";
    }

    void ImageTerrainRasterizer::LoadHeightMap(const float *data, uint32_t width, uint32_t height)
    {
        imageSampler = vk_utils::createSampler(
            renderingContext.device,
            VK_FILTER_LINEAR,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK);

        LoadImage(image, data, width, height);
    }

    void ImageTerrainRasterizer::LoadImage(vk_utils::VulkanImageMem &image, const float *data, uint32_t width, uint32_t height)
    {
        image.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vk_utils::createImgAllocAndBind(
            renderingContext.device,
            renderingContext.physicalDevice,
            width,
            height,
            VK_FORMAT_R32_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            &image);

        renderingContext.pCopyHelper->UpdateImage(image.image, data, width, height, sizeof(float), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    IRasterizer::Shaders ImageTerrainRasterizer::getShaderPaths() const
    {
        auto [vs, fs] = TerrainRasterizer::getShaderPaths();
        return {"./shaders/rasterization/terrain_image.vert", fs};
    }

    IRasterizer::Descriptors ImageTerrainRasterizer::createDescriptorSets()
    {
        VkDescriptorSetLayout layout = nullptr;
        VkDescriptorSet set = nullptr;
        renderingContext.pBindings->BindBegin(VK_SHADER_STAGE_VERTEX_BIT);
        renderingContext.pBindings->BindImage(0, image.view, imageSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        renderingContext.pBindings->BindEnd(&set, &layout);

        auto [layouts, sets] = TerrainRasterizer::createDescriptorSets();

        layouts.push_back(layout);
        sets.push_back(set);

        return {layouts, sets};
    }

}
