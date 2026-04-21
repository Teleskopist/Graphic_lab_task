#include "lite_app_imgui.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/imgui_internal.h"
#include "vkutils/vk_context.h"
#include "blk/blk.h"

namespace LiteApp
{
	PFN_vkVoidFunction vulkanLoaderFunction(const char* function_name, void*) 
  { 
    return vkGetInstanceProcAddr(vk_utils::globalContextGet().instance, function_name); 
  }

  void init_ImGui(RenderingContext &r_ctx)
  {
		IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(r_ctx.window, true);

    vk_utils::DescriptorTypesVec descrTypes = {
      { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }};

    r_ctx.gui.descriptorPool = vk_utils::createDescriptorPool(r_ctx.device, descrTypes, (uint32_t)descrTypes.size() * 1000,
                                                                VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

    vk_utils::RenderTargetInfo2D rtInfo = {};
    rtInfo.format = r_ctx.swapchain.GetFormat();
    rtInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    rtInfo.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    rtInfo.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    r_ctx.gui.renderpass   = vk_utils::createRenderPass(r_ctx.device, rtInfo);
    r_ctx.gui.framebuffers = vk_utils::createFrameBuffers(r_ctx.device, r_ctx.swapchain, r_ctx.gui.renderpass);
    r_ctx.gui.commandPool  = vk_utils::createCommandPool(r_ctx.device, r_ctx.queueFamilyIDXs.graphics, 
                                                           VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    r_ctx.gui.cmdBuffers.reserve(r_ctx.swapchain.GetImageCount());
    r_ctx.gui.cmdBuffers = vk_utils::createCommandBuffers(r_ctx.device, r_ctx.gui.commandPool, r_ctx.swapchain.GetImageCount());

    ImGui_ImplVulkan_InitInfo init_info {};

    init_info.Instance       = r_ctx.instance;
    init_info.PhysicalDevice = r_ctx.physicalDevice;
    init_info.Device         = r_ctx.device;
    init_info.QueueFamily    = r_ctx.queueFamilyIDXs.graphics;
    init_info.Queue          = r_ctx.queue;
    init_info.PipelineInfoMain.RenderPass = r_ctx.gui.renderpass;
    init_info.PipelineCache  = VK_NULL_HANDLE;
    init_info.DescriptorPool = r_ctx.gui.descriptorPool;
    init_info.Allocator      = VK_NULL_HANDLE;
    init_info.MinImageCount  = r_ctx.swapchain.GetMinImageCount();
    init_info.ImageCount     = r_ctx.swapchain.GetImageCount();
    init_info.CheckVkResultFn = nullptr;

    ImGui_ImplVulkan_LoadFunctions(VK_MAKE_API_VERSION(0, 1, 2, 0), vulkanLoaderFunction);
    ImGui_ImplVulkan_Init(&init_info);
  }

  VkCommandBuffer build_command_buffer_imGui(uint32_t a_swapchainFrameIdx, void* a_userData, const RenderingContext &r_ctx)
  {
    auto currentCmdBuf = r_ctx.gui.cmdBuffers[a_swapchainFrameIdx];

    VkCommandBufferBeginInfo cmdBeginInfo = {};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK_RESULT(vkBeginCommandBuffer(currentCmdBuf, &cmdBeginInfo));

    VkRenderPassBeginInfo rpassBeginInfo = {};
    rpassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpassBeginInfo.renderPass  = r_ctx.gui.renderpass;
    rpassBeginInfo.framebuffer = r_ctx.gui.framebuffers[a_swapchainFrameIdx];
    rpassBeginInfo.renderArea.extent = r_ctx.swapchain.GetExtent();
    rpassBeginInfo.clearValueCount = 0;
    rpassBeginInfo.pClearValues = nullptr;
    vkCmdBeginRenderPass(currentCmdBuf, &rpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_RenderDrawData(static_cast<ImDrawData*>(a_userData), currentCmdBuf);

    vkCmdEndRenderPass(currentCmdBuf);

    vkEndCommandBuffer(currentCmdBuf);

    return currentCmdBuf;
  }

  void on_swapchain_changed_imGui(RenderingContext &r_ctx)
  {
    for(auto fbuf : r_ctx.gui.framebuffers)
      vkDestroyFramebuffer(r_ctx.device, fbuf, VK_NULL_HANDLE);
    
    r_ctx.gui.framebuffers.clear();
    r_ctx.gui.framebuffers = vk_utils::createFrameBuffers(r_ctx.device, r_ctx.swapchain, r_ctx.gui.renderpass);
  }

  void cleanup_imGui(RenderingContext &r_ctx)
  {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    for(auto fbuf : r_ctx.gui.framebuffers)
      vkDestroyFramebuffer(r_ctx.device, fbuf, VK_NULL_HANDLE);
    r_ctx.gui.framebuffers.clear();

    if(r_ctx.gui.renderpass)
    {
      vkDestroyRenderPass(r_ctx.device, r_ctx.gui.renderpass, VK_NULL_HANDLE);
      r_ctx.gui.renderpass = VK_NULL_HANDLE;
    }
    
    if(r_ctx.gui.commandPool)
    {
      vkDestroyCommandPool(r_ctx.device, r_ctx.gui.commandPool, VK_NULL_HANDLE);
      r_ctx.gui.commandPool = VK_NULL_HANDLE;  
    }

    if(r_ctx.gui.descriptorPool)
    {
      vkDestroyDescriptorPool(r_ctx.device, r_ctx.gui.descriptorPool, VK_NULL_HANDLE);
      r_ctx.gui.descriptorPool = VK_NULL_HANDLE;   
    }

    ImGui::DestroyContext();
  }

  bool input_text(const char *label, std::string &text)
  {
    static char __input_buf[4096];
    int len = text.size();
    strncpy(__input_buf, text.c_str(), text.size() + 1);
    bool get = ImGui::InputText(label, __input_buf, 4096, ImGuiInputTextFlags_EnterReturnsTrue);
    text = std::string(__input_buf);

    return get;
  }

void blk_modification_interface(Block *b, const std::string &title)
{
  bool is_open = ImGui::CollapsingHeader(title.c_str());
  if (!is_open)
    return;
  if (!b)
  {
    ImGui::Text("empty");
    return;
  }

  for (int i = 0; i < b->size(); i++)
  {
    Block::Value &val = b->values[i];
    const char *name = b->names[i].c_str();
    switch (val.type)
    {
    case Block::ValueType::BOOL:
      ImGui::Checkbox(name, &(val.b));
      break;
    case Block::ValueType::INT:
    {
      int v = val.i;
      ImGui::InputInt(name, &v);
      val.i = v;
    }
    break;
    case Block::ValueType::UINT64:
    {
      int v = val.u;
      ImGui::InputInt(name, &v);
      val.u = v;
    }
    break;
    case Block::ValueType::DOUBLE:
      ImGui::InputDouble(name, &(val.d), 1.0, 10.0);
      break;
    case Block::ValueType::VEC2:
    {
      float v[2];
      v[0] = val.v2.x;
      v[1] = val.v2.y;
      ImGui::InputFloat2(name, v);
      val.v2.x = v[0];
      val.v2.y = v[1];
    }
    break;
    case Block::ValueType::VEC3:
    {
      float v[3];
      v[0] = val.v3.x;
      v[1] = val.v3.y;
      v[2] = val.v3.z;
      ImGui::InputFloat3(name, v);
      val.v3.x = v[0];
      val.v3.y = v[1];
      val.v3.z = v[2];
    }
    break;
    case Block::ValueType::VEC4:
    {
      float v[4];
      v[0] = val.v4.x;
      v[1] = val.v4.y;
      v[2] = val.v4.z;
      v[3] = val.v4.w;
      ImGui::InputFloat4(name, v);
      val.v4.x = v[0];
      val.v4.y = v[1];
      val.v4.z = v[2];
      val.v4.w = v[3];
    }
    break;
    case Block::ValueType::IVEC2:
    {
      int v[2];
      v[0] = val.iv2.x;
      v[1] = val.iv2.y;
      ImGui::InputInt2(name, v);
      val.iv2.x = v[0];
      val.iv2.y = v[1];
    }
    break;
    case Block::ValueType::IVEC3:
    {
      int v[3];
      v[0] = val.iv3.x;
      v[1] = val.iv3.y;
      v[2] = val.iv3.z;
      ImGui::InputInt3(name, v);
      val.iv3.x = v[0];
      val.iv3.y = v[1];
      val.iv3.z = v[2];
    }
    break;
    case Block::ValueType::IVEC4:
    {
      int v[4];
      v[0] = val.iv4.x;
      v[1] = val.iv4.y;
      v[2] = val.iv4.z;
      v[3] = val.iv4.w;
      ImGui::InputInt4(name, v);
      val.iv4.x = v[0];
      val.iv4.y = v[1];
      val.iv4.z = v[2];
      val.iv4.w = v[3];
    }
    break;
    case Block::ValueType::MAT4:
    {
      std::string n0 = std::string(name) + "[0]";
      std::string n1 = std::string(name) + "[1]";
      std::string n2 = std::string(name) + "[2]";
      std::string n3 = std::string(name) + "[3]";
      ImGui::InputFloat4(n0.c_str(), &(val.m4(0, 0)));
      ImGui::InputFloat4(n1.c_str(), &(val.m4(0, 1)));
      ImGui::InputFloat4(n2.c_str(), &(val.m4(0, 2)));
      ImGui::InputFloat4(n3.c_str(), &(val.m4(0, 3)));
    }
    break;
    case Block::ValueType::ENUM:
    {
      ImGui::ListBox(name, (int *)&(val.ev.val_id), get_enum_names(val.ev.type_id)->data(),
                     get_enum_names(val.ev.type_id)->size());
    }
    break;
    case Block::ValueType::STRING:
    {
      input_text(name, *(val.s));
    }
    break;
    case Block::ValueType::BLOCK:
      blk_modification_interface(val.bl, b->names[i]);
      break;
    default:
      ImGui::Text("not supported");
      break;
    }
  }
}
}