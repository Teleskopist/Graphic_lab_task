#pragma once
#include "LiteApp/rasterization/SimpleRasterizer.h"

using LiteApp::IRasterizer;
using LiteApp::SimpleRasterizer;
using LiteApp::RenderingContext;

namespace demo_app
{
    class TerrainRasterizer : public SimpleRasterizer
    {
    public:
        using SimpleRasterizer::SimpleRasterizer;

        virtual void LoadScene(const std::string &path) override final;

    private:
        virtual void LoadHeightMap(const float *data, uint32_t width, uint32_t height) = 0;
    };

}
