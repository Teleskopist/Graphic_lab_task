#include "lite_app.h"

namespace LiteApp
{
  static bool g_keyPressed[MAXKEYS];
  static Mouse g_mouse;
  
  bool getKeyPressed(uint32_t key)
  {
    return g_keyPressed[key];
  }

  Mouse *getMouse()
  {
    return &g_mouse;
  }

  void on_keyboard_pressed(GLFWwindow* window, int key, int, int action, int)
  {
    if(key >= 0 && key < MAXKEYS && action == GLFW_PRESS)
      g_keyPressed[key] = true;
  }

  void on_mouse_button_clicked(GLFWwindow* window, int button, int action, int)
  {
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
      g_mouse.captureMouse = !g_mouse.captureMouse;
    
    if (button == GLFW_MOUSE_BUTTON_MIDDLE)
    {
      if (action == GLFW_PRESS)
        g_mouse.midPressed = true;
      else if (action == GLFW_RELEASE)
        g_mouse.midPressed = false;
    }

    if (g_mouse.captureMouse)
    {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      g_mouse.capturedMouseJustNow = true;
    }
    else
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }

  void on_mouse_move(GLFWwindow*, double xpos, double ypos)
  {
    if (g_mouse.firstMouse)
    {
      g_mouse.lastX      = float(xpos);
      g_mouse.lastY      = float(ypos);
      g_mouse.firstMouse = false;
    }

    g_mouse.lastX = float(xpos);
    g_mouse.lastY = float(ypos);
  }

  void on_mouse_scroll(GLFWwindow*, double, double yoffset)
  {
    g_mouse.scrollY = float(yoffset);
  }

  GLFWwindow* init_glfw_window(int width, int height, bool fullscreen)
  {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    GLFWwindow *window = nullptr;
    if (fullscreen)
    {
      const GLFWvidmode* mode = glfwGetVideoMode(monitor);
      window = glfwCreateWindow(mode->width, mode->height, "LiteRT Demo App", monitor, nullptr);
    }
    else
    {
      int x0,y0,size_x,size_y;
      glfwGetMonitorWorkarea(monitor, &x0, &y0, &size_x, &size_y);
      int w = std::min(width, size_x)/8*8;
      int h = std::min(height, size_y)/8*8;
      window = glfwCreateWindow(w, h, "LiteRT Demo App", nullptr, nullptr);
    }

    glfwSetKeyCallback        (window, on_keyboard_pressed);
    glfwSetCursorPosCallback  (window, on_mouse_move);
    glfwSetMouseButtonCallback(window, on_mouse_button_clicked);
    glfwSetScrollCallback     (window, on_mouse_scroll);
    glfwSwapInterval(0);

    return window;
  }

  void clearKeys() 
  { 
    memset(g_keyPressed,  0, MAXKEYS*sizeof(bool)); 
  }

  void cleanup_pipeline_and_swapchain(RenderingContext &r_ctx)
  {
    if (!r_ctx.cmdBuffersDrawMain.empty())
    {
      vkFreeCommandBuffers(r_ctx.device, r_ctx.commandPool, static_cast<uint32_t>(r_ctx.cmdBuffersDrawMain.size()),
                          r_ctx.cmdBuffersDrawMain.data());
      r_ctx.cmdBuffersDrawMain.clear();
    }

    for (size_t i = 0; i < r_ctx.frameFences.size(); i++)
    {
      vkDestroyFence(r_ctx.device, r_ctx.frameFences[i], nullptr);
    }
    r_ctx.frameFences.clear();

    vk_utils::deleteImg(r_ctx.device, &r_ctx.depthBuffer);

    for (size_t i = 0; i < r_ctx.frameBuffers.size(); i++)
    {
      vkDestroyFramebuffer(r_ctx.device, r_ctx.frameBuffers[i], nullptr);
    }
    r_ctx.frameBuffers.clear();

    if(r_ctx.screenRenderPass != VK_NULL_HANDLE)
    {
      vkDestroyRenderPass(r_ctx.device, r_ctx.screenRenderPass, nullptr);
      r_ctx.screenRenderPass = VK_NULL_HANDLE;
    }

    r_ctx.swapchain.Cleanup();
  }

  void cleanup(RenderingContext &r_ctx)
  {
    cleanup_pipeline_and_swapchain(r_ctx);
    
    if(r_ctx.surface != VK_NULL_HANDLE)
    {
      vkDestroySurfaceKHR(r_ctx.instance, r_ctx.surface, nullptr);
      r_ctx.surface = VK_NULL_HANDLE;
    }

    if(r_ctx.rt_pres.rtImageSampler != VK_NULL_HANDLE)
    {
      vkDestroySampler(r_ctx.device, r_ctx.rt_pres.rtImageSampler, nullptr);
      r_ctx.rt_pres.rtImageSampler = VK_NULL_HANDLE;
    }
    vk_utils::deleteImg(r_ctx.device, &r_ctx.rt_pres.rtImage);

    r_ctx.rt_pres.pFSQuad = nullptr;

    if (r_ctx.imageAvailable != VK_NULL_HANDLE)
    {
      vkDestroySemaphore(r_ctx.device, r_ctx.imageAvailable, nullptr);
      r_ctx.imageAvailable = VK_NULL_HANDLE;
    }
    if (r_ctx.renderingFinished != VK_NULL_HANDLE)
    {
      vkDestroySemaphore(r_ctx.device, r_ctx.renderingFinished, nullptr);
      r_ctx.renderingFinished = VK_NULL_HANDLE;
    }

    if (r_ctx.commandPool != VK_NULL_HANDLE)
    {
      vkDestroyCommandPool(r_ctx.device, r_ctx.commandPool, nullptr);
      r_ctx.commandPool = VK_NULL_HANDLE;
    }

    r_ctx.pBindings = nullptr;
    // rast_ctx.sceneMgr = nullptr;
    r_ctx.pCopyHelper = nullptr;

    if(r_ctx.device != VK_NULL_HANDLE)
    {
      vkDestroyDevice(r_ctx.device, nullptr);
      r_ctx.device = VK_NULL_HANDLE;
    }

    if(r_ctx.debugReportCallback != VK_NULL_HANDLE)
    {
      vkDestroyDebugReportCallbackEXT(r_ctx.instance, r_ctx.debugReportCallback, nullptr);
      r_ctx.debugReportCallback = VK_NULL_HANDLE;
    }

    if(r_ctx.instance != VK_NULL_HANDLE)
    {
      vkDestroyInstance(r_ctx.instance, nullptr);
      r_ctx.instance = VK_NULL_HANDLE;
    }
  }

  void recreate_swapchain_base(RenderingContext &r_ctx)
  {
    vkDeviceWaitIdle(r_ctx.device);

    cleanup_pipeline_and_swapchain(r_ctx);
    auto oldImagesNum = r_ctx.swapchain.GetImageCount();
    r_ctx.queue = r_ctx.swapchain.CreateSwapChain(r_ctx.physicalDevice, r_ctx.device, r_ctx.surface, 
                                                  r_ctx.width, r_ctx.height,
                                                  oldImagesNum, r_ctx.vsync);

    std::vector<VkFormat> depthFormats = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };                                                            
    vk_utils::getSupportedDepthFormat(r_ctx.physicalDevice, depthFormats, &r_ctx.depthBuffer.format);
    
    r_ctx.screenRenderPass = vk_utils::createDefaultRenderPass(r_ctx.device, r_ctx.swapchain.GetFormat(),r_ctx.depthBuffer.format);
    r_ctx.depthBuffer      = vk_utils::createDepthTexture(r_ctx.device, r_ctx.physicalDevice, r_ctx.width, r_ctx.height, r_ctx.depthBuffer.format);
    r_ctx.frameBuffers     = vk_utils::createFrameBuffers(r_ctx.device, r_ctx.swapchain, r_ctx.screenRenderPass, r_ctx.depthBuffer.view);

    r_ctx.frameFences.resize(r_ctx.framesInFlight);
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (size_t i = 0; i < r_ctx.framesInFlight; i++)
    {
      VK_CHECK_RESULT(vkCreateFence(r_ctx.device, &fenceInfo, nullptr, &r_ctx.frameFences[i]));
    }

    r_ctx.cmdBuffersDrawMain = vk_utils::createCommandBuffers(r_ctx.device, r_ctx.commandPool, r_ctx.framesInFlight);
  }

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallbackFn(
    VkDebugReportFlagsEXT                       flags,
    VkDebugReportObjectTypeEXT                  objectType,
    uint64_t                                    object,
    size_t                                      location,
    int32_t                                     messageCode,
    const char* pLayerPrefix,
    const char* pMessage,
    void* pUserData)
  {
    (void)flags;
    (void)objectType;
    (void)object;
    (void)location;
    (void)messageCode;
    (void)pUserData;
    std::cout << pLayerPrefix << ": " << pMessage << std::endl;
    return VK_FALSE;
  }


  void setup_validation_layers(RenderingContext &r_ctx, bool enable)
  {
    r_ctx.enableValidation = enable;
    r_ctx.validationLayers.push_back("VK_LAYER_KHRONOS_validation");
    r_ctx.validationLayers.push_back("VK_LAYER_LUNARG_monitor");
  }

  void create_instance(RenderingContext &r_ctx)
  {
    VkApplicationInfo appInfo = {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext              = nullptr;
    appInfo.pApplicationName   = "VkRender";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 2, 0);
    appInfo.pEngineName        = "RayTracingSample";
    appInfo.engineVersion      = VK_MAKE_API_VERSION(0, 1, 2, 0);
    appInfo.apiVersion         = VK_MAKE_API_VERSION(0, 1, 2, 0);

    r_ctx.instance = vk_utils::createInstance(r_ctx.enableValidation, r_ctx.validationLayers, r_ctx.instanceExtensions, &appInfo);

    if (r_ctx.enableValidation)
      vk_utils::initDebugReportCallback(r_ctx.instance, &debugReportCallbackFn, &r_ctx.debugReportCallback);
  }

  VkPhysicalDeviceFeatures2 setup_device_features()
  {
    static VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelStructFeatures{};
    static VkPhysicalDeviceBufferDeviceAddressFeatures enabledDeviceAddressFeatures{};
    static VkPhysicalDeviceRayQueryFeaturesKHR enabledRayQueryFeatures{};
    static VkPhysicalDeviceFeatures2 features2{};

    if (false)
    {
      enabledRayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
      enabledRayQueryFeatures.rayQuery = VK_TRUE;
      enabledRayQueryFeatures.pNext = nullptr;
      
      enabledDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
      enabledDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
      enabledDeviceAddressFeatures.pNext = &enabledRayQueryFeatures;
      
      enabledAccelStructFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
      enabledAccelStructFeatures.accelerationStructure = VK_TRUE;
      enabledAccelStructFeatures.pNext = &enabledDeviceAddressFeatures;
      
      features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
      features2.pNext = &enabledAccelStructFeatures;  
    }
    else
    {
      features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
      features2.pNext = nullptr;      
    } 

      return features2;
  }

  std::vector<const char*> setup_device_extensions()
  {
    std::vector<const char*> deviceExtensions;
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    
    if(false)
    {
      deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
      deviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
      // Required by VK_KHR_acceleration_structure
      deviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
      deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
      deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
      // Required by VK_KHR_ray_tracing_pipeline
      deviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
      // Required by VK_KHR_spirv_1_4
      deviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
    }

    return deviceExtensions;
  }

  std::vector<const char *> merge_extensions(const std::vector<const char *> &e1, const std::vector<const char *> &e2)
  {
    std::vector<const char *> result = e1;
    //for (const char *r : result)
    // printf("extension %s\n", r);
    for (const char *e : e2)
    {
      bool found = false;
      for (const char *r : result)
      {
        if (strcmp(r, e) == 0)
        {
          found = true;
          break;
        }
      }
      if (!found)
      {
        result.push_back(e);
        //printf("add extension %s\n", e);
      }
    }
    return result;
  }

  VkPhysicalDeviceFeatures2 merge_device_features(VkPhysicalDeviceFeatures2 &f1, VkPhysicalDeviceFeatures2 &f2)
  {
    std::vector<VkStructureType> all_types = {f1.sType};

    VkPhysicalDeviceFeatures2 *p_feature = &f1;
    while (p_feature->pNext)
    {
      p_feature = (VkPhysicalDeviceFeatures2 *)p_feature->pNext;
      all_types.push_back(p_feature->sType);
      //printf("new type %u\n", (unsigned)p_feature->sType);
    }

    VkPhysicalDeviceFeatures2 *p_feature2 = &f2;
    while (p_feature2)
    {
      bool found = false;
      for (const VkStructureType &t : all_types)
      {
        if (p_feature2->sType == t)
        {
          found = true;
          break;
        }
      }
      if (!found)
      {
        p_feature->pNext = p_feature2;
        p_feature = p_feature2;
        //printf("new type %u\n", (unsigned)p_feature->sType);
      }
      else
      {
        //printf("existing type %u\n", (unsigned)p_feature2->sType);
      }
      p_feature2 = (VkPhysicalDeviceFeatures2 *)p_feature2->pNext;
    }

    return f1;
  }

  void create_device(uint32_t a_deviceId, RenderingContext &r_ctx,
                     const std::vector<const char *> &extensions_raytracer,
                     VkPhysicalDeviceFeatures2 features_raytracer)
  {
    std::vector<const char *> extensions_self = setup_device_extensions();
    VkPhysicalDeviceFeatures2 features_self  = setup_device_features();

    std::vector<const char *> extensions = merge_extensions(extensions_self, extensions_raytracer);
    VkPhysicalDeviceFeatures2 features = merge_device_features(features_self, features_raytracer);

    r_ctx.physicalDevice = vk_utils::findPhysicalDevice(r_ctx.instance, true, a_deviceId, extensions);

    r_ctx.device = vk_utils::createLogicalDevice(r_ctx.physicalDevice, r_ctx.validationLayers, extensions,
                                                 r_ctx.enabledDeviceFeatures, r_ctx.queueFamilyIDXs,
                                                 VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT, features.pNext);

    vkGetDeviceQueue(r_ctx.device, r_ctx.queueFamilyIDXs.graphics, 0, &r_ctx.graphicsQueue);
    vkGetDeviceQueue(r_ctx.device, r_ctx.queueFamilyIDXs.transfer, 0, &r_ctx.transferQueue);
    vkGetDeviceQueue(r_ctx.device, r_ctx.queueFamilyIDXs.compute,  0, &r_ctx.computeQueue);
  }

  void init_vulkan(RenderingContext &r_ctx, uint32_t deviceId, bool use_validation,
                   const char** a_instanceExtensions, uint32_t a_instanceExtensionsCount,
                   const std::vector<const char *> &extensions_raytracer,
                   VkPhysicalDeviceFeatures2 features_raytracer)
  {
    for(size_t i = 0; i < a_instanceExtensionsCount; ++i)
      r_ctx.instanceExtensions.push_back(a_instanceExtensions[i]);

    setup_validation_layers(r_ctx, use_validation);
    VK_CHECK_RESULT(volkInitialize());
    create_instance(r_ctx);
    volkLoadInstance(r_ctx.instance);

    create_device(deviceId, r_ctx, extensions_raytracer, features_raytracer);
    volkLoadDevice(r_ctx.device);

    r_ctx.commandPool = vk_utils::createCommandPool(r_ctx.device, r_ctx.queueFamilyIDXs.graphics,
                                                    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    r_ctx.cmdBuffersDrawMain.reserve(r_ctx.framesInFlight);
    r_ctx.cmdBuffersDrawMain = vk_utils::createCommandBuffers(r_ctx.device, r_ctx.commandPool, r_ctx.framesInFlight);

    r_ctx.frameFences.resize(r_ctx.framesInFlight);
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (size_t i = 0; i < r_ctx.framesInFlight; i++)
    {
      VK_CHECK_RESULT(vkCreateFence(r_ctx.device, &fenceInfo, nullptr, &r_ctx.frameFences[i]));
    }

    r_ctx.pCopyHelper = std::make_shared<vk_utils::PingPongCopyHelper>(r_ctx.physicalDevice, r_ctx.device, r_ctx.transferQueue,
                                                                       r_ctx.queueFamilyIDXs.transfer, r_ctx.stagingMemSize);

    r_ctx.pAllocatorSpecial = vk_utils::CreateMemoryAlloc_Special(r_ctx.device, r_ctx.physicalDevice);
  }

  void init_presentation(RenderingContext &r_ctx)
  {
    r_ctx.queue = r_ctx.swapchain.CreateSwapChain(r_ctx.physicalDevice, r_ctx.device, r_ctx.surface,
                                                  r_ctx.width, r_ctx.height, 
                                                  r_ctx.framesInFlight, r_ctx.vsync);
    r_ctx.currentFrame = 0;

    std::vector<VkFormat> depthFormats = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };
    vk_utils::getSupportedDepthFormat(r_ctx.physicalDevice, depthFormats, &r_ctx.depthBuffer.format);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreateSemaphore(r_ctx.device, &semaphoreInfo, nullptr, &r_ctx.imageAvailable));
    VK_CHECK_RESULT(vkCreateSemaphore(r_ctx.device, &semaphoreInfo, nullptr, &r_ctx.renderingFinished));
    r_ctx.screenRenderPass = vk_utils::createDefaultRenderPass(r_ctx.device, r_ctx.swapchain.GetFormat(), r_ctx.depthBuffer.format);


    r_ctx.depthBuffer  = vk_utils::createDepthTexture(r_ctx.device, r_ctx.physicalDevice, r_ctx.width, r_ctx.height, r_ctx.depthBuffer.format);
    r_ctx.frameBuffers = vk_utils::createFrameBuffers(r_ctx.device, r_ctx.swapchain, r_ctx.screenRenderPass, r_ctx.depthBuffer.view);
  }    

  void build_command_buffer_quad(VkCommandBuffer a_cmdBuff, VkImageView a_targetImageView, 
                                 const RenderingContext::RTImagePresenter &rt_pres)
  {
    vkResetCommandBuffer(a_cmdBuff, 0);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VK_CHECK_RESULT(vkBeginCommandBuffer(a_cmdBuff, &beginInfo));
    {
      float scaleAndOffset[4] = {0.5f, 0.5f, -0.5f, +0.5f};
      rt_pres.pFSQuad->SetRenderTarget(a_targetImageView);
      rt_pres.pFSQuad->DrawCmd(a_cmdBuff, rt_pres.quadDS, scaleAndOffset);
    }

    VK_CHECK_RESULT(vkEndCommandBuffer(a_cmdBuff));
  }

  void setup_quad(RenderingContext &r_ctx, const char *quad_vertex_shader_path, const char *quad_fragment_shader_path, bool a_drawFromBuffer)
  {
    vk_utils::deleteImg(r_ctx.device, &r_ctx.rt_pres.rtImage);

    // change format and usage according to your implementation of RT
    r_ctx.rt_pres.rtImage.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vk_utils::createImgAllocAndBind(r_ctx.device, r_ctx.physicalDevice, r_ctx.width, r_ctx.height,
                                    VK_FORMAT_R8G8B8A8_UNORM,
                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                    &r_ctx.rt_pres.rtImage);

    if (r_ctx.rt_pres.rtImageSampler == VK_NULL_HANDLE)
    {
      r_ctx.rt_pres.rtImageSampler = vk_utils::createSampler(r_ctx.device, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK);
    }

    vk_utils::RenderTargetInfo2D rtargetInfo = {};
    rtargetInfo.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    rtargetInfo.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    rtargetInfo.format         = r_ctx.swapchain.GetFormat();
    rtargetInfo.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    rtargetInfo.size           = r_ctx.swapchain.GetExtent();
    rtargetInfo.drawFromBuffer = VkBool32(a_drawFromBuffer);
    
    r_ctx.rt_pres.pFSQuad.reset();
    r_ctx.rt_pres.pFSQuad = std::make_shared<vk_utils::QuadRenderer>(0, 0, r_ctx.width, r_ctx.height);
    r_ctx.rt_pres.pFSQuad->Create(r_ctx.device, quad_vertex_shader_path, quad_fragment_shader_path, rtargetInfo);

    if(a_drawFromBuffer)
    {
      r_ctx.pBindings->BindBegin(VK_SHADER_STAGE_FRAGMENT_BIT);
      r_ctx.pBindings->BindBuffer(0, r_ctx.rt_pres.frameBuffer);
      r_ctx.pBindings->BindEnd(&r_ctx.rt_pres.quadDS, &r_ctx.rt_pres.quadDSLayout);
    }
    else
    {
      r_ctx.pBindings->BindBegin(VK_SHADER_STAGE_FRAGMENT_BIT);
      r_ctx.pBindings->BindImage(0, r_ctx.rt_pres.rtImage.view, r_ctx.rt_pres.rtImageSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
      r_ctx.pBindings->BindEnd(&r_ctx.rt_pres.quadDS, &r_ctx.rt_pres.quadDSLayout);
    }
  }

  void init_app(RenderingContext &r_ctx, AppInitSettings settings)
  {
    r_ctx.window = LiteApp::init_glfw_window(settings.desiredWidth, settings.desiredHeight, settings.fullscreen);
    
    int w, h;
    glfwGetFramebufferSize(r_ctx.window, &w, &h);
    if (w != settings.desiredWidth || h != settings.desiredHeight)
      printf("Window size changed to %dx%d\n", w, h);

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions;
    auto features = settings.pDeviceFeaturesProvider(extensions, r_ctx);
    init_vulkan(r_ctx, settings.deviceId, settings.enableValidation, 
                glfwExtensions, glfwExtensionCount, extensions, features);

    glfwCreateWindowSurface(r_ctx.instance, r_ctx.window, nullptr, &r_ctx.surface);
    init_presentation(r_ctx);

    std::vector<std::pair<VkDescriptorType, uint32_t> > dtypes = 
    {
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             1},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     1}
    };

    // set large a_maxSets, because every window resize will cause the descriptor set for quad being to be recreated
    r_ctx.pBindings = std::make_shared<vk_utils::DescriptorMaker>(r_ctx.device, dtypes, 1000);
  }
}