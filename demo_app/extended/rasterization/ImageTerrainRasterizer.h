#pragma once
#include "TerrainRasterizer.h"

namespace demo_app
{

    class ImageTerrainRasterizer : public TerrainRasterizer
    {
    public:
        ImageTerrainRasterizer(RenderingContext &ctx);
        ~ImageTerrainRasterizer();

        virtual const char *Name() const override;

    private:
        virtual Shaders getShaderPaths() const override final;
        virtual Descriptors createDescriptorSets() override final;

        virtual void LoadHeightMap(const float *data, uint32_t width, uint32_t height) override final;
        virtual void LoadImage(vk_utils::VulkanImageMem &image, const float *data, uint32_t width, uint32_t height);

        vk_utils::VulkanImageMem image{};
        VkSampler imageSampler = nullptr;
    };

}
