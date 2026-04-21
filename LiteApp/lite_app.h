#pragma once

#define VK_NO_PROTOTYPES

#include "volk.h"
#include "vk_utils.h"
#include "vkutils/vk_descriptor_sets.h"
#include "vkutils/geom/vk_mesh.h"
#include "vkutils/vk_fbuf_attachment.h"
#include "vkutils/vk_quad.h"
#include "vkutils/vk_images.h"
#include "vkutils/vk_swapchain.h"
#include "vkutils/vk_alloc.h"
#include "GLFW/glfw3.h"
#include "core/render_settings.h"
#include "LiteMath.h"

#include <memory>
#include <vector>
#include <string>

namespace LiteApp
{
  static constexpr uint32_t MAXKEYS = 384;

  struct Mouse
  {
    bool firstMouse   = true;
    bool captureMouse = false;
    bool capturedMouseJustNow = false;

    bool midPressed = false;

    float lastX, lastY, scrollY;
  };

  struct RenderingContext
  {
    uint32_t renderDevice = DEVICE_GPU_COMP; //enum RenderDevice
    uint64_t stagingMemSize = 16 * 16 * 1024u;
    uint32_t framesInFlight = 2u;
    bool vsync = false;

    uint32_t width;
    uint32_t height;

    LiteMath::float4x4 projectionMatrix;
    LiteMath::float4x4 worldViewMatrix;
    LiteMath::float4x4 inverseProjViewMatrix;

    GLFWwindow *window;

    VkPhysicalDeviceFeatures enabledDeviceFeatures = {};
    std::vector<const char*> instanceExtensions    = {};
    bool enableValidation;
    std::vector<const char*> validationLayers;
  
    VkInstance       instance       = VK_NULL_HANDLE;
    VkCommandPool    commandPool    = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice         device         = VK_NULL_HANDLE;
    VkQueue          graphicsQueue  = VK_NULL_HANDLE;
    VkQueue          transferQueue  = VK_NULL_HANDLE;
    VkQueue          computeQueue   = VK_NULL_HANDLE;

    std::shared_ptr<vk_utils::ICopyEngine> pCopyHelper;
    std::shared_ptr<vk_utils::IMemoryAlloc> pAllocatorSpecial;

    vk_utils::QueueFID_T queueFamilyIDXs {UINT32_MAX, UINT32_MAX, UINT32_MAX};

    VkDebugReportCallbackEXT debugReportCallback = nullptr;

    std::vector<VkFence> frameFences;
    std::vector<VkCommandBuffer> cmdBuffersDrawMain;

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VulkanSwapChain swapchain;
    std::vector<VkFramebuffer> frameBuffers;
    vk_utils::VulkanImageMem depthBuffer{};

    uint32_t    currentFrame      = 0u;
    VkQueue     queue             = VK_NULL_HANDLE;
    VkSemaphore imageAvailable    = VK_NULL_HANDLE;
    VkSemaphore renderingFinished = VK_NULL_HANDLE;

    VkDescriptorSet dSet = VK_NULL_HANDLE;
    VkDescriptorSetLayout dSetLayout = VK_NULL_HANDLE;
    VkRenderPass screenRenderPass = VK_NULL_HANDLE; // rasterization renderpass

    std::shared_ptr<vk_utils::DescriptorMaker> pBindings = nullptr;

    //GUI stuff (for use with ImGUI)
    struct GUI
    {
      VkRenderPass renderpass = VK_NULL_HANDLE;
      std::vector<VkFramebuffer> framebuffers;
      VkCommandPool commandPool = VK_NULL_HANDLE;
      std::vector<VkCommandBuffer> cmdBuffers;
      VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    };
    GUI gui;

    // stuff for presenting final images to screen, required for raytracing using IRenderer interface
    struct RTImagePresenter
    {
      std::shared_ptr<vk_utils::IQuad> pFSQuad;
      VkDescriptorSet quadDS = VK_NULL_HANDLE;
      VkDescriptorSetLayout quadDSLayout = VK_NULL_HANDLE;
      vk_utils::VulkanImageMem rtImage;
      VkSampler                rtImageSampler = VK_NULL_HANDLE;
      VkBuffer                 frameBuffer = VK_NULL_HANDLE;
    };
    RTImagePresenter rt_pres;
  };

  //VkPhysicalDeviceFeatures2 ListRequiredDeviceFeatures(std::vector<const char *> &deviceExtensions, const RenderingContext &r_ctx);

  struct AppInitSettings
  {
    int desiredWidth      = 1600;
    int desiredHeight     = 900;
    bool fullscreen       = false;
    bool enableValidation = false;
    uint32_t deviceId     = vk_utils::CHOOSE_DEVICE_BY_NAME;
    VkPhysicalDeviceFeatures2(*pDeviceFeaturesProvider)(std::vector<const char *> &deviceExtensions, const RenderingContext &r_ctx) = nullptr;
  };

  void init_app(RenderingContext &r_ctx, AppInitSettings settings);

  bool getKeyPressed(uint32_t key);
  Mouse *getMouse();
  void clearKeys();

  GLFWwindow* init_glfw_window(int width, int height, bool fullscreen = false);
  void cleanup_pipeline_and_swapchain(RenderingContext &r_ctx);
  void cleanup(RenderingContext &r_ctx);
  void recreate_swapchain_base(RenderingContext &r_ctx);
  void init_vulkan(RenderingContext &r_ctx, uint32_t deviceId, bool use_validation,
                   const char** a_instanceExtensions, uint32_t a_instanceExtensionsCount,
                   const std::vector<const char *> &extensions_raytracer,
                   VkPhysicalDeviceFeatures2 features_raytracer);
  void init_presentation(RenderingContext &r_ctx);
  void build_command_buffer_quad(VkCommandBuffer a_cmdBuff, VkImageView a_targetImageView, const RenderingContext::RTImagePresenter &rt_pres);
  void setup_quad(RenderingContext &r_ctx, const char *quad_vertex_shader_path, const char *quad_fragment_shader_path, bool a_readFromBuffer = false);
  VkPhysicalDeviceFeatures2 merge_device_features(VkPhysicalDeviceFeatures2 &f1, VkPhysicalDeviceFeatures2 &f2);
  std::vector<const char *> merge_extensions(const std::vector<const char *> &e1, const std::vector<const char *> &e2);
}