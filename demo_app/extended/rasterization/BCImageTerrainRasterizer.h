#pragma once
#include "ImageTerrainRasterizer.h"

namespace demo_app
{

    class BCImageTerrainRasterizer : public ImageTerrainRasterizer
    {
        using ImageTerrainRasterizer::ImageTerrainRasterizer;

        virtual const char *Name() const override;
#ifdef USE_KTX
        virtual void LoadImage(vk_utils::VulkanImageMem &image, const float *data, uint32_t width, uint32_t height) override;
#endif
    };

}
