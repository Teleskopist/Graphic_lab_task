#include "QuadTreeTerrainRasterizer.h"
#include "utils/misc/terrain.h"
#include "utils/misc/quadtree.h"

namespace demo_app
{

    const char *QuadTreeTerrainRasterizer::Name() const
    {
        return "Terrain (Quad tree)";
    }

    void QuadTreeTerrainRasterizer::LoadHeightMap(const float *data, uint32_t width, uint32_t height)
    {

        std::vector<uint32_t> quadtree = create_quadtree_from_image(data, width, height);

        quadtreeBuffer = BufferWrapper(
            renderingContext.device,
            renderingContext.physicalDevice,
            quadtree.size() * sizeof(uint32_t),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

        uint32_t *p = reinterpret_cast<uint32_t *>(quadtreeBuffer.map());
        std::copy(quadtree.begin(), quadtree.end(), p);
        quadtreeBuffer.unmap();
    }

    IRasterizer::Shaders QuadTreeTerrainRasterizer::getShaderPaths() const
    {
        auto [vs, fs] = TerrainRasterizer::getShaderPaths();
        return {"./shaders/rasterization/terrain_quadtree.vert", fs};
    }

    IRasterizer::Descriptors QuadTreeTerrainRasterizer::createDescriptorSets()
    {
        VkDescriptorSetLayout layout = nullptr;
        VkDescriptorSet set = nullptr;
        renderingContext.pBindings->BindBegin(VK_SHADER_STAGE_VERTEX_BIT);
        renderingContext.pBindings->BindBuffer(0, quadtreeBuffer.buffer, nullptr);
        renderingContext.pBindings->BindEnd(&set, &layout);

        auto [layouts, sets] = TerrainRasterizer::createDescriptorSets();

        layouts.push_back(layout);
        sets.push_back(set);

        return {layouts, sets};
    }

}
