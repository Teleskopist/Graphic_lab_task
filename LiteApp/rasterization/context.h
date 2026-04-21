#pragma once
#include "IRasterizer.h"

namespace LiteApp
{
    class RasterizationContext
    {
    public:
        RasterizationContext();
        ~RasterizationContext();

        void add_rasterizer(std::unique_ptr<IRasterizer> rasterizer);
        void clear_rasterizers();

        void setup(const std::string &scene_path);
        void refresh(bool wireframe);

        IRasterizer *active_rasterizer();

        void set_active_rasterizer(size_t index);
        const std::vector<const char *> &rasterizer_names() const;

    private:
        size_t active_rasterizer_index = 0;
        std::vector<std::unique_ptr<IRasterizer>> rasterizers;
        std::vector<const char *> rasterizer_names_;
    };

}
