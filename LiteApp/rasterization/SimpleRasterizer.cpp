#include "SimpleRasterizer.h"

namespace LiteApp
{

    SimpleRasterizer::SimpleRasterizer(RenderingContext &ctx)
        : IRasterizer(ctx), fragmentParamsBuffer(ctx.device, ctx.physicalDevice)
    {
        fragmentParams.baseColor = float4(1.0f);
    }

    const char *SimpleRasterizer::Name() const
    {
        return "Simple";
    }

    void SimpleRasterizer::SetCamera(float4x4 view, float4x4 proj)
    {
        pushConstants.view = view;
        pushConstants.proj = proj;
    }

    void SimpleRasterizer::SetLights(const std::vector<Light> &lights)
    {
        fragmentParams.ambientLight = float4(0.25f, 0.25f, 0.25f, 1.0f);
        auto it = std::begin(fragmentParams.directionalLights);
        auto end = std::end(fragmentParams.directionalLights);
        std::fill(it, end, DirectionalLight{});
        for (const Light &i : lights)
        {
            if (i.type == LIGHT_TYPE_DIRECT && it < end)
            {
                it->dir = to_float4(i.space, 0.0f);
                it->color = to_float4(i.color, 0.0f);
                ++it;
            }
        }
    }

    void SimpleRasterizer::UpdateUniforms()
    {
        fragmentParamsBuffer.write(fragmentParams);
    }

    size_t SimpleRasterizer::pushConstantSize()
    {
        return sizeof(pushConstants);
    }

    void *SimpleRasterizer::pushConstantData()
    {
        return &pushConstants;
    }

    void SimpleRasterizer::pushConstantUpdateModelTransform(LiteMath::float4x4 model)
    {
        pushConstants.model = model;
    }

    IRasterizer::Shaders SimpleRasterizer::getShaderPaths() const
    {
        return {"./shaders/rasterization/simple.vert", "./shaders/rasterization/simple.frag"};
    }

    IRasterizer::Descriptors SimpleRasterizer::createDescriptorSets()
    {
        VkDescriptorSet set = nullptr;
        VkDescriptorSetLayout layout = nullptr;
        renderingContext.pBindings->BindBegin(VK_SHADER_STAGE_FRAGMENT_BIT);
        renderingContext.pBindings->BindBuffer(0, fragmentParamsBuffer.buffer, VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        renderingContext.pBindings->BindEnd(&set, &layout);

        return {{layout}, {set}};
    }

}
