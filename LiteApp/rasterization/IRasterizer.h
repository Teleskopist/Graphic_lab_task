#pragma once
#include <string>
#include <memory>
#include <LiteMath.h>
#include "LiteScene/scene_mgr.h"
#include "LiteApp/lite_app.h"
#include "core/IRenderer.h"
#include <filesystem>

namespace LiteApp
{
    struct Settings;

    class IRasterizer
    {
    public:
        IRasterizer(RenderingContext &renderingContext);
        virtual ~IRasterizer();

        virtual const char *Name() const = 0;

        virtual void SetWireframe(bool enable) { wireFrameMode = enable; }
        void CreatePipeline();
        void ClearPipeline();

        void SetBackgroundColor(float3 color);
        virtual void SetCamera(LiteMath::float4x4 view, LiteMath::float4x4 proj) = 0;
        virtual void SetLights(const std::vector<Light> &lights) = 0;

        virtual void LoadScene(const std::string &path);

        virtual void UpdateUniforms() = 0;
        void RenderCmd(VkCommandBuffer cmdBuff, VkFramebuffer frameBuff);
        void PrepareScreenshot(const std::string &path);
        void TakeScreenshotCMD(VkImage srcImage);
        bool NeedScreenshot();

        struct Shaders
        {
            std::filesystem::path vertex;
            std::filesystem::path fragment;
        };
        virtual Shaders getShaderPaths() const = 0;

        void CompileShaders();

    protected:
        struct Descriptors
        {
            std::vector<VkDescriptorSetLayout> layouts;
            std::vector<VkDescriptorSet> sets;
        };

        virtual Descriptors createDescriptorSets() = 0;
        virtual size_t pushConstantSize() = 0;
        virtual void *pushConstantData() = 0;
        virtual void pushConstantUpdateModelTransform(LiteMath::float4x4 model) = 0;

        bool wireFrameMode = false;
        float4 backgroundColor;

        bool screenshotMode = false;
        std::string screenshotPath;
        VkImage dstImage = VK_NULL_HANDLE;              // screenshot image
        VkDeviceMemory dstImageMemory = VK_NULL_HANDLE; // screenshot image memory

        std::shared_ptr<SceneManager> sceneManager = nullptr;
        RenderingContext &renderingContext;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
        bool cachedDescriptorSets = false;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        std::vector<VkDescriptorSet> descriptorSets;
    };
}
