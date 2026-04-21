#pragma once
#include "IRasterizer.h"
#include "shaders/common.h"
#include "vk_buffer_helpers.h"

namespace LiteApp
{

    class SimpleRasterizer : public IRasterizer
    {
    public:
        SimpleRasterizer(RenderingContext &ctx);

        virtual const char *Name() const override;

        virtual void SetCamera(float4x4 view, float4x4 proj) override final;
        virtual void SetLights(const std::vector<Light> &lights) override final;

        virtual void UpdateUniforms() override;

        virtual Shaders getShaderPaths() const override;

    protected:
        virtual Descriptors createDescriptorSets() override;

    private:
        SimplePushConstants pushConstants;
        SimpleFragmentParams fragmentParams;
        UniformBufferWrapper<SimpleFragmentParams> fragmentParamsBuffer;

        virtual size_t pushConstantSize() override final;
        virtual void *pushConstantData() override final;
        virtual void pushConstantUpdateModelTransform(LiteMath::float4x4 model) override final;
    };

}
