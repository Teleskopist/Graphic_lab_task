#pragma once
#include "TerrainRasterizer.h"

namespace demo_app
{

    class QuadTreeTerrainRasterizer : public TerrainRasterizer
    {
    public:
        using TerrainRasterizer::TerrainRasterizer;

        virtual const char *Name() const override final;

    private:
        virtual Shaders getShaderPaths() const override final;
        virtual Descriptors createDescriptorSets() override final;

        virtual void LoadHeightMap(const float *data, uint32_t width, uint32_t height) override final;

        BufferWrapper quadtreeBuffer;
    };

}
