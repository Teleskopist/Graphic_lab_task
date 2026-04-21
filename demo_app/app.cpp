#include "app.h"
#include "app_internal.h"
#include "imgui.h"
#include "volk.h"
#include "vk_utils.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/imgui_internal.h"
#include "utils/misc/image_metrics.h"
#include "utils/common/strings.h"
#include "utils/raytracing/render_common.h"
#include "utils/mesh/mesh_internal.h"
#include "core/scene_extension.h"
#include "core/serialization.h"
#include "plugins/plugins.h"
#include "PostEffects/image_metrics/ImgMetricsKernel_glsl.h"

#include <cstdio>
#include <chrono>
#include <filesystem>
#include <memory>

namespace demo_app
{  
  using namespace LiteMath;
  std::string get_time_str()
  {
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string s(30, '\0');
    std::strftime(&s[0], s.size(), "%Y_%m_%d_%H_%M_%S", std::localtime(&now));
    return std::string(s.c_str());
  }

  std::string get_time_str_for_log()
  {
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string s(30, '\0');
    std::strftime(&s[0], s.size(), "%H:%M:%S", std::localtime(&now));
    return std::string(s.c_str());
  }

  void add_to_log(const LogEntry &log_entry, UIContext &ui_ctx)
  {
    ui_ctx.logToShow.push_back(log_entry);
    std::string time_str = get_time_str_for_log();
    std::string type_str;
    switch (log_entry.type)
    {
    case LogEntry::Type::INFO:
      type_str = "INFO";
      break;
    case LogEntry::Type::WARNING:
      type_str = "WARNING";
      break;
    case LogEntry::Type::ERROR:
      type_str = "ERROR";
      break;
    }
    printf("[%s][%s] %s\n", time_str.c_str(), type_str.c_str(), log_entry.message.c_str());
  }

  void update_view(const Settings &settings, RenderingContext &r_ctx)
  {
    const float aspect   = float(r_ctx.width) / float(r_ctx.height);
    auto mProjFix        = OpenglToVulkanProjectionMatrixFix();
    r_ctx.projectionMatrix   = projectionMatrix(settings.camera.fovy, aspect, settings.camera.z_near, settings.camera.z_far);
    r_ctx.worldViewMatrix    = LiteMath::lookAt(settings.camera.pos, settings.camera.lookAt, settings.camera.up);
    r_ctx.inverseProjViewMatrix = LiteMath::inverse4x4(r_ctx.projectionMatrix * transpose(inverse4x4(r_ctx.worldViewMatrix)));
  }

  void create_camera_block(const Settings &settings, Block &blk)
  {
    blk.set_vec3("pos", settings.camera.pos);
    blk.set_vec3("lookAt", settings.camera.lookAt);
    blk.set_vec3("up", settings.camera.up);
    blk.set_double("fovy", settings.camera.fovy);
    blk.set_double("z_near", settings.camera.z_near);
    blk.set_double("z_far", settings.camera.z_far);
  }

  void process_input(float secondsElapsed, CommandBuffer &cmd_buffer, Settings &settings, UIContext &ui_ctx, RenderingContext &r_ctx,
                     RasterizationContext &rast_ctx, RaytracingContext &rt_ctx)
  {    
    if (ui_ctx.hotkeysBlockCooldown > 0)
      return;
    {
      auto prev = settings.render_mode;
      if (LiteApp::getKeyPressed(GLFW_KEY_1))
        settings.render_mode = RenderMode::RASTERIZATION;

      if (LiteApp::getKeyPressed(GLFW_KEY_2))
        settings.render_mode = RenderMode::RAYTRACING;
      if (settings.render_mode != prev)
      {
        Block blk;
        blk.add_enum("render_mode", "RenderMode", (uint32_t)settings.render_mode);
        cmd_buffer.push(Cmd::SET_RENDER_DEVICE, blk);
      }
    }
    if (LiteApp::getKeyPressed(GLFW_KEY_3))
      settings.raytracerId = ui_ctx.raytracersCount > 1 ? (settings.raytracerId+1) % ui_ctx.raytracersCount : 0;
    
    if (LiteApp::getKeyPressed(GLFW_KEY_C))
    {
      Block blk;
      create_camera_block(settings, blk);
      cmd_buffer.push(Cmd::SET_CAMERA, blk); 
    }

    if (LiteApp::getKeyPressed(GLFW_KEY_X))
    {
      Block blk;
      blk.set_vec3("pos", float3(0,0,3));
      blk.set_vec3("lookAt", float3(0,0,0));
      blk.set_vec3("up", float3(0,1,0));
      cmd_buffer.push(Cmd::SET_CAMERA, blk);
    }

    if (LiteApp::getKeyPressed(GLFW_KEY_P))
    {
      //take screenshot
      if ((settings.render_mode == RenderMode::RASTERIZATION && ui_ctx.rasterizationSceneSetUp) ||
          (settings.render_mode == RenderMode::   RAYTRACING && ui_ctx.   raytracingSceneSetUp))
      {
        std::string name = "saves/screenshots/" + get_time_str() + ".png";
        Block blk;
        blk.set_string("save_path", name.c_str());
        blk.set_int("rtId", settings.raytracerId);
        blk.set_int("spp", ui_ctx.highQualityScreenshot ? 16 : 1);
        Block blkc;
        create_camera_block(settings, blkc);
        cmd_buffer.push(Cmd::SET_CAMERA, blkc); 
        cmd_buffer.push(Cmd::TAKE_SCREENSHOT, blk);
      }
    }

    if (LiteApp::getKeyPressed(GLFW_KEY_O))
    {
      Block blk;
      blk.set_int("rtId", ui_ctx.baseRaytracerId);
      blk.set_int("refRtId", ui_ctx.referenceRaytracerId);
      Block blkc;
      create_camera_block(settings, blkc);
      cmd_buffer.push(Cmd::SET_CAMERA, blkc); 
      cmd_buffer.push(Cmd::COMPARE_FRAMES, blk);
    }
    
    if (LiteApp::getKeyPressed(GLFW_KEY_V))
    {
      Block blk;
      blk.add_bool("enable", !ui_ctx.minimalUI);
      cmd_buffer.push(Cmd::SET_MINIMAL_UI, blk);
    }

    if (LiteApp::getKeyPressed(GLFW_KEY_ESCAPE))
    {
      if (ui_ctx.active_cmd)
      {
        delete ui_ctx.active_cmd;
        ui_ctx.active_cmd = nullptr;
      }
      add_to_log({"Exiting", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      glfwSetWindowShouldClose(r_ctx.window, VK_TRUE);
    }

    if (LiteApp::getKeyPressed(GLFW_KEY_LEFT_SHIFT) && settings.camMoveSpeed < 500)
      settings.camMoveSpeed *= 2.0f;

    if (LiteApp::getKeyPressed(GLFW_KEY_LEFT_CONTROL) && settings.camMoveSpeed > 0.0005)
      settings.camMoveSpeed *= 0.5f;
  
    if (LiteApp::getKeyPressed(GLFW_KEY_0))
    {
      if (ui_ctx.sceneProvided &&
          ui_ctx.raytracingSceneSetUp &&
          settings.render_mode == RenderMode::RAYTRACING) // check that we have data
      {
        float x = (float(LiteApp::getMouse()->lastX) + 0.5) / float(r_ctx.width);
        float y = 1.0f - (float(LiteApp::getMouse()->lastY) + 0.5) / float(r_ctx.height);
        Block blk;
        blk.set_vec2("uv", float2(x, y));
        cmd_buffer.push(Cmd::CAST_DEBUG_RAY, blk);
      }
    }

    if (LiteApp::getKeyPressed(GLFW_KEY_Q))
    {
      settings.camera.fovy += 5.0f;
      settings.cameraMoving = true;
    }

    if (LiteApp::getKeyPressed(GLFW_KEY_E))
    {
      settings.camera.fovy -= 5.0f;
      settings.cameraMoving = true;
    }

    //move position of camera based on WASD keys, and FR keys for up and down
    //we use glfwGetKey here, because we want to move camera each frame if the key is pressed
    //while LiteApp::g_keyPressed is just set to true when the key is pressed (1 press = 1 action)
    if (glfwGetKey(r_ctx.window, 'S'))
    {
      settings.camera.offsetPosition(secondsElapsed * settings.camMoveSpeed * -1.0f * settings.camera.forward());
      settings.cameraMoving = true;
    }
    else if (glfwGetKey(r_ctx.window, 'W'))
    {
      settings.camera.offsetPosition(secondsElapsed * settings.camMoveSpeed * settings.camera.forward());
      settings.cameraMoving = true;
    }

    if (glfwGetKey(r_ctx.window, 'A'))
    {
      settings.camera.offsetPosition(secondsElapsed * settings.camMoveSpeed * -1.0f * settings.camera.right());
      settings.cameraMoving = true;
    }
    else if (glfwGetKey(r_ctx.window, 'D'))
    {
      settings.camera.offsetPosition(secondsElapsed * settings.camMoveSpeed * settings.camera.right());
      settings.cameraMoving = true;
    }

    if (glfwGetKey(r_ctx.window, 'F'))
    {
      settings.camera.offsetPosition(secondsElapsed * settings.camMoveSpeed * -1.0f * settings.camera.up);
      settings.cameraMoving = true;
    }
    else if (glfwGetKey(r_ctx.window, 'R'))
    {
      settings.camera.offsetPosition(secondsElapsed * settings.camMoveSpeed * settings.camera.up);
      settings.cameraMoving = true;
    }

    if (ui_ctx.cuttingPlaneActive)
    {
      settings.cutting_plane.is_active = 1;
      
      if (glfwGetKey(r_ctx.window, GLFW_KEY_UP))
      {
        settings.cutting_plane.pos += 0.1f * settings.cutting_plane.normal * secondsElapsed * settings.camMoveSpeed;
      }

      if (glfwGetKey(r_ctx.window, GLFW_KEY_DOWN))
      {
        settings.cutting_plane.pos -= 0.1f * settings.cutting_plane.normal * secondsElapsed * settings.camMoveSpeed;
      }

      if (glfwGetKey(r_ctx.window, GLFW_KEY_LEFT))
      {
        settings.cutting_plane.normal = mul4x3(rotate4x4Y(0.5f * secondsElapsed), settings.cutting_plane.normal);
      }

      if (glfwGetKey(r_ctx.window, GLFW_KEY_RIGHT))
      {
        settings.cutting_plane.normal = mul4x3(rotate4x4Y(-0.5f * secondsElapsed), settings.cutting_plane.normal);
      }

      if (glfwGetKey(r_ctx.window, GLFW_KEY_PAGE_UP))
      {
        settings.cutting_plane.normal = mul4x3(rotate4x4Z(0.5f * secondsElapsed), settings.cutting_plane.normal);
      }

      if (glfwGetKey(r_ctx.window, GLFW_KEY_PAGE_DOWN))
      {
        settings.cutting_plane.normal = mul4x3(rotate4x4Z(-0.5f * secondsElapsed), settings.cutting_plane.normal);
      }
    }
    else
    {
      settings.cutting_plane.is_active = 0;
    }

    //rotate camera based on mouse movement
    //
    if (LiteApp::getMouse()->captureMouse)
    {
      if(LiteApp::getMouse()->capturedMouseJustNow)
        glfwSetCursorPos(r_ctx.window, 0, 0);

      double mouseX, mouseY;
      glfwGetCursorPos(r_ctx.window, &mouseX, &mouseY);
      settings.camera.offsetOrientation(settings.mouseSensitivity * float(mouseY), settings.mouseSensitivity * float(mouseX));
      if (mouseX != 0 || mouseY != 0)
        settings.cameraMoving = true;
      glfwSetCursorPos(r_ctx.window, 0, 0); //reset the mouse, so it doesn't go out of the window
      LiteApp::getMouse()->capturedMouseJustNow = false;
    }

    if (LiteApp::getMouse()->scrollY != 0.0f)
      settings.cameraMoving = true;
    LiteApp::getMouse()->scrollY = 0.0f;

    if (LiteApp::getMouse()->midPressed)//check if middle mouse button were pressed
    {
      LiteApp::getMouse()->midPressed = false;//just one time
      
      if ( ui_ctx.sceneProvided && 
           ui_ctx.raytracingSceneSetUp && 
           settings.render_mode == RenderMode::RAYTRACING)//check that we have data
      {
        float x = (float(LiteApp::getMouse()->lastX) + 0.5) / float(r_ctx.width);
        float y = 1.0f - (float(LiteApp::getMouse()->lastY) + 0.5) / float(r_ctx.height);
        Block blk;
        blk.set_vec2("uv", float2(x, y));
        cmd_buffer.push(Cmd::CAST_DEBUG_RAY, blk);
        ui_ctx.toOpenPopUp = true;
      }
    }
  }

  float get_raytracing_average_time_ms(const UIContext &ui_ctx)
  {
    float average_time = 0;
    int start_idx = ui_ctx.currentTimeIndex + ui_ctx.sliding_window_size - 1;
    int steps = std::min(ui_ctx.sliding_window_size, ui_ctx.frameCounter);
    for (int i = 0; i < steps; i++)
      average_time += ui_ctx.timeSlidingWindow[(start_idx - i) % ui_ctx.sliding_window_size];
    average_time /= steps;
    return average_time;
  }

  void reset_frame_counter(UIContext &ui_ctx)
  {
    ui_ctx.frameCounter = 1;
  }

  void scene_modification_interface(const Settings &settings, UIContext &ui_ctx, 
                                    RenderingContext &r_ctx, RaytracingContext &rt_ctx)
  {
    bool is_open = ImGui::CollapsingHeader("Scene");
    if (!is_open)
      return;

    LiteScene::HydraScene &scene = ui_ctx.scenes[settings.raytracerId];
    SceneUIContext &scene_ctx = ui_ctx.scenesCtx[settings.raytracerId];

    //instances in renderer are places in sequential order, the same way as in SceneUIContext
    //while in the scene itself there is no such guarantee. That is why we update instance
    //i instead of instance h_inst_id in this function
    std::vector<uint32_t> instanceIds;
    std::vector<LiteMath::float4x4> matrices;

    bool disable_all = ImGui::Button("Disable all");
    if (disable_all)
    {
      for (int i = 0; i < scene_ctx.instances.size(); i++)
      {
        SceneUIContext::Instance &instance = scene_ctx.instances[i];
        instance.visible = false;
        instanceIds.push_back(i);
        matrices.push_back(LiteMath::translate4x4(float3(0,1000,0)));
      }
    }

    bool enable_all = ImGui::Button("Enable all");
    if (enable_all)
    {
      for (int i = 0; i < scene_ctx.instances.size(); i++)
      {
        SceneUIContext::Instance &instance = scene_ctx.instances[i];
        instance.visible = true;
        instanceIds.push_back(i);
        matrices.push_back(scene.scenes[0].instances[instance.h_inst_id].matrix);
      }
    }

    bool invert_visibility = ImGui::Button("Invert visibility");
    if (invert_visibility)
    {
      for (int i = 0; i < scene_ctx.instances.size(); i++)
      {
        SceneUIContext::Instance &instance = scene_ctx.instances[i];
        instance.visible = !instance.visible;
        instanceIds.push_back(i);
        matrices.push_back(instance.visible ? 
                           scene.scenes[0].instances[instance.h_inst_id].matrix :
                           LiteMath::translate4x4(float3(0,1000,0)));
      }
    }

    if (ui_ctx.scenesCtx[settings.raytracerId].vtkDataset.unstructured_grids.size() > 0)
    {
      ImGui::Checkbox("Show grid surface\n", &ui_ctx.CAEShowSurface);
      for (int i = 0; i < ui_ctx.scenesCtx[settings.raytracerId].vtkDataset.unstructured_grids.size(); i++)
      {
        vtk::UnstructuredGrid::Type type = ui_ctx.scenesCtx[settings.raytracerId].vtkDataset.unstructured_grids[i].type;
        SceneUIContext::Instance &instance = scene_ctx.instances[i];
        bool should_be_visible = type == vtk::UnstructuredGrid::Type::MIXED || type == vtk::UnstructuredGrid::Type::UNDEFINED ||
                                 (ui_ctx.CAEShowSurface && type == vtk::UnstructuredGrid::Type::SURFACE) ||
                                 (!ui_ctx.CAEShowSurface && type == vtk::UnstructuredGrid::Type::VOLUME);
        if (should_be_visible != instance.visible)
        {
          instance.visible = should_be_visible;
          instanceIds.push_back(i);
          matrices.push_back(instance.visible ? 
                             scene.scenes[0].instances[instance.h_inst_id].matrix :
                             LiteMath::translate4x4(float3(0,1000,0)));
        }
      }
    }

    ImGui::Separator();

    for (int i = 0; i < scene_ctx.instances.size(); i++)
    {
      SceneUIContext::Instance &instance = scene_ctx.instances[i];
      std::string name = std::to_string(instance.h_inst_id) + ". " + scene_ctx.geometries[instance.geom_id].name;
      bool changed = ImGui::Checkbox(name.c_str(), &instance.visible);
      if (changed)
      {
        instanceIds.push_back(i);
        matrices.push_back(instance.visible ? 
                           scene.scenes[0].instances[instance.h_inst_id].matrix :
                           LiteMath::translate4x4(float3(0,1000,0)));
      }
    }

    if (instanceIds.size() > 0)
    {
      rt_ctx.pRayTracers[settings.raytracerId]->UpdateInstances(instanceIds, matrices);
    }
  }

  void CAE_visualization_GUI(int sceneId, UIContext &ui_ctx)
  {
    static int mode_enum_type_id = -1;
    if (mode_enum_type_id == -1)
    {
      Block b;
      b.add_enum("a", "ChannelRenderMode", CHANNEL_RENDER_MODE_DIRECT);
      mode_enum_type_id = b.values[0].ev.type_id;
    }

    bool is_open = ImGui::CollapsingHeader("Dataset visualization");
    if (!is_open)
      return;
    
    static constexpr const char* const names_3[] = {"X", "Y", "Z"};
    constexpr int MAX_COMPONENTS = 32;
    static constexpr const char* const names_32[] = {" 1", " 2", " 3", " 4", " 5", " 6", " 7", " 8", 
                                                     " 9", "10", "11", "12", "13", "14", "15", "16",
                                                     "17", "18", "19", "20", "21", "22", "23", "24",
                                                     "25", "26", "27", "28", "29", "30", "31", "32"};
    
    const auto &info = ui_ctx.scenesCtx[sceneId].cae_info;

    if (ui_ctx.CAEVisSettings.channel == -1 || ui_ctx.CAEVisSettings.channel >= info.names.size())
    {
      ui_ctx.CAEVisSettings.channel = info.names.size()-1;
      ui_ctx.CAEVisSettings.component = 0;
    }

    ImGui::Checkbox("Use Lambert shading", &ui_ctx.CAEVisSettings.lambert_shading);
    ImGui::ListBox("Channel", &ui_ctx.CAEVisSettings.channel, info.c_names.data(), info.c_names.size());
    ImGui::ListBox("Mode", &ui_ctx.CAEVisSettings.vis_mode, get_enum_names(mode_enum_type_id)->data(), get_enum_names(mode_enum_type_id)->size());
    
    bool single_channel_mode = true; //we will have modes to visualize the entire component, not just one channel

    if (single_channel_mode)
    {
      int num_comps = info.num_components[ui_ctx.CAEVisSettings.channel];
      if (ui_ctx.CAEVisSettings.component >= num_comps)
        ui_ctx.CAEVisSettings.component = 0;

      if (num_comps == 2 || num_comps == 3)
      {
        ImGui::ListBox("Component", &ui_ctx.CAEVisSettings.component, names_3, num_comps);
      }
      else if (num_comps > 3)
      {
        ImGui::ListBox("Component", &ui_ctx.CAEVisSettings.component, names_32, std::min(num_comps, MAX_COMPONENTS));
      }
      ImGui::ListBox("Color mode", (int *)&ui_ctx.CAEVisSettings.ctf_preset, CTF_names.data(), CTF_names.size());
    }
  }

  void update_lights(Settings &settings, UIContext &ui_ctx, RaytracingContext &rt_ctx, RasterizationContext&rast_ctx, RenderingContext &r_ctx)
  {
    float3 camera_forward = settings.camera.forward();
    float3 camera_right = settings.camera.right();
    float3 camera_up = settings.camera.up;

    float color_shift = settings.lightIntensity*settings.lightIntensity;
    ui_ctx.cameraLight.space = -normalize(camera_forward - 0.5f * camera_right - 0.5f * camera_up);
    ui_ctx.cameraLight.color = settings.lightColor * color_shift;
    Light directional_light = create_direct_light(settings.lightPos, settings.lightColor * color_shift);
    Light additional_light = create_direct_light(float3(-0.6,1,-0.3), float3(1,1,1)*color_shift);

    std::vector<Light> lights;
    if (ui_ctx.useCameraLight)
      lights.push_back(ui_ctx.cameraLight);
    else
      lights.push_back(directional_light);
    lights.push_back(additional_light);

    if (ui_ctx.raytracingSceneSetUp)
    {
      for (int i = 0; i < ui_ctx.raytracersCount; i++)
        rt_ctx.pRayTracers[i]->SetLights(lights);
    }
    if (rast_ctx.active_rasterizer())
    {
      rast_ctx.active_rasterizer()->SetLights(lights);
    }
  }

  void setup_minimal_GUI_elements(CommandBuffer &cmd_buffer, Settings &settings, UIContext &ui_ctx, RenderingContext &r_ctx,
                                  RasterizationContext &rast_ctx, RaytracingContext &rt_ctx)
  {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    float average_time = get_raytracing_average_time_ms(ui_ctx);
    ImGui::Begin("##fps");
    ImGui::SetWindowPos(ImVec2(0, 0));
    ImGui::Text("Ray tracing time: %.3f ms/frame (%.1f FPS)", average_time, 1000.0f / average_time);
    ImGui::End();

    ImGui::Render();
  }

  static constexpr const char* const numbers[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };
  static constexpr const char* const gpu_mode_items[] = {"CPU", "Compute", "Ray Query"};
  static constexpr uint32_t gpu_mode_devices[] = {DEVICE_CPU, DEVICE_GPU_COMP, DEVICE_GPU_RQ};

  static constexpr const char* const renderer_items[] = {"Multi-Renderer (default)", 
                                                         "Hydra (CPU only)", 
                                                         "Volume renderer",
                                                         "Voxel renderer",
                                                         "CSG direct renderer"};
  static constexpr RenderType renderer_types[] = {RenderType::MULTI,
                                                  RenderType::HYDRA,
                                                  RenderType::VOLUME,
                                                  RenderType::VOXEL,
                                                  RenderType::CSG_DIRECT};

  static constexpr const char *rasterizer_items[] = {
    "Simple (default)",
    "Terrain"
  };

  void base_GUI(CommandBuffer &cmd_buffer, Settings &settings, UIContext &ui_ctx, RenderingContext &r_ctx,
                RasterizationContext &rast_ctx, RaytracingContext &rt_ctx)
  {
    //console log
    {
      int w = ui_ctx.window_width;
      int h = ui_ctx.window_height;
      ImVec2 sz = ImVec2(500, 300);

      ImGui::Begin("Log");
      ImGui::SetWindowPos(ImVec2(w-sz.x, 0));
      ImGui::SetWindowSize(sz);

      for (int i=0;i<ui_ctx.logToShow.size();i++)
        ImGui::TextColored(LogEntry::typeColors[(int)ui_ctx.logToShow[i].type],"%s", ui_ctx.logToShow[i].message.c_str());

      ImGui::End();
    }

    //query settings
    {
      int w = ui_ctx.window_width;
      int h = ui_ctx.window_height;
      ImVec2 sz = ImVec2(500, 80);

      ImGui::Begin("Query settings");
      ImGui::SetWindowPos(ImVec2(w-sz.x, 300));
      ImGui::SetWindowSize(sz);

      ImGui::Checkbox("debug ray", &ui_ctx.queryDebugRay); ImGui::SameLine();
      ImGui::Checkbox("primitive info", &ui_ctx.queryDebugPrimitiveInfo); ImGui::SameLine();
      ImGui::Checkbox("build info", &ui_ctx.queryDebugBuildInfo);

      ImGui::End();
    }

    //comparison GUI
    if (ui_ctx.raytracersCount > 1)
    {
      int w = ui_ctx.window_width;
      int h = ui_ctx.window_height;
      ImVec2 sz = ImVec2(500, 400);

      ImGui::Begin("Comparison settings");
      ImGui::SetWindowPos(ImVec2(w-sz.x, 350));
      ImGui::SetWindowSize(sz);

      ImGui::Text("active raytracer");
      ImGui::ListBox("##a_rt", &settings.raytracerId, numbers, ui_ctx.raytracersCount);

      ImGui::Text("target raytracer");
      ImGui::ListBox("##t_rt", &ui_ctx.baseRaytracerId, numbers, ui_ctx.raytracersCount);

      ImGui::Text("reference raytracer");
      ImGui::ListBox("##r_rt", &ui_ctx.referenceRaytracerId, numbers, ui_ctx.raytracersCount);

      ImGui::Checkbox("save screenshots on comparison", &ui_ctx.saveScreenshotOnComparison);
      
      bool compare_frames = ImGui::Button("Compare frames (target vs. reference)");
      if (compare_frames)
      {
        Block blk;
        blk.set_int("rtId", ui_ctx.baseRaytracerId);
        blk.set_int("refRtId", ui_ctx.referenceRaytracerId);
        Block blkc;
        create_camera_block(settings, blkc);
        cmd_buffer.push(Cmd::SET_CAMERA, blkc); 
        cmd_buffer.push(Cmd::COMPARE_FRAMES, blk);
      }
    
      ImGui::End();
    }
    
    {
      int w = ui_ctx.window_width;
      int h = ui_ctx.window_height;
      ImVec2 sz = ImVec2(500, 0.4*h);

      //print hotkeys
      ImGui::Begin("Your render settings here");
      ImGui::SetWindowPos(ImVec2(0, 0));
      ImGui::SetWindowSize(sz);

      if (ImGui::CollapsingHeader("Hotkeys"))
      {
        ImGui::Text("'1' - Rasterization mode");
        ImGui::Text("'2' - Raytracing mode");
        ImGui::Text("'3' - Switch between raytracers");
        ImGui::Text("'W', 'A', 'S', 'D' - Movement");
        ImGui::Text("'R', 'F' - Up/Down");
        ImGui::Text("'C' - Print current camera");
        ImGui::Text("'Shift' - Faster movement");
        ImGui::Text("'Control' - Slower movement");
        ImGui::Text("'P' - Take a screenshot");
        ImGui::Text("'O' - Compare with reference on this frame");
        ImGui::Text("'V' - Toggle visibility of the UI");
        ImGui::Text("'Esc' - Exit");
        ImGui::NewLine();
      }

      //show FPS
      ImGui::Text("Camera speed: %d",(int)(1000*settings.camMoveSpeed));
      
      if (ui_ctx.raytracingSceneSetUp && settings.render_mode == RenderMode::RAYTRACING)
      {
        float average_time = get_raytracing_average_time_ms(ui_ctx);

        ImGui::Text("Memory used for scene: %.1f MB\n", (float)(ui_ctx.scenesCtx[settings.raytracerId].memoryInRenderer/1024.f/1024.f));
        ImGui::Text("Ray tracing time: %.3f ms/frame (%.1f FPS)", average_time, 1000.0f / average_time);
        ImGui::SameLine(); 
        bool reset_counter = ImGui::Button("Reset counter");
        if (reset_counter)
          reset_frame_counter(ui_ctx);

        if (ui_ctx.calculateLiveMetrics)
        {
          float PSNR = -10 * log10(std::max<double>(1e-10, rt_ctx.pMetricsKernel->m_summ / (r_ctx.width * r_ctx.height)));
          if (ui_ctx.qualityMetricsStat.sample_bins.size() > 0)
          {
            ui_ctx.qualityMetricsStat.total_samples++;
            ui_ctx.qualityMetricsStat.sample_bins[(int)PSNR*METRICS_BINS_PER_UNIT]++;
          }
          ImGui::Text("PSNR: %.1f", PSNR);
        }
        else
        {
          ImGui::NewLine();
        }
        

        if (ui_ctx.active_cmd == nullptr)
        {
          bool record_trajectory = ImGui::Button("Record trajectory");
          if (record_trajectory)
          {
            auto tr_ctx = new RecordTrajectoryCmdCtx();
            tr_ctx->timestamps.push_back(std::chrono::high_resolution_clock::now());
            tr_ctx->cameras.push_back(settings.camera);
            ui_ctx.active_cmd = tr_ctx;
          }
        }
        else if (ui_ctx.active_cmd->cmd == Cmd::RECORD_TRAJECTORY)
        {
          auto tr_ctx = dynamic_cast<RecordTrajectoryCmdCtx*>(ui_ctx.active_cmd);
          assert(tr_ctx);

          //it should be constants or settings
          std::string filename = "saves/saved_trajectory.blk";
          float timestamp_interval = 250; //in milliseconds

          bool stop_recording = ImGui::Button("Stop recording");
          if (stop_recording)
          {
            tr_ctx->timestamps.push_back(std::chrono::high_resolution_clock::now());
            tr_ctx->cameras.push_back(settings.camera);

            float total_time = std::chrono::duration<float, std::milli>(tr_ctx->timestamps.back() - tr_ctx->timestamps.front()).count();
            printf("recorded %d timestamps over %.1f s\n", (int)tr_ctx->timestamps.size(), total_time/1000);

            Block blk;
            for (int i = 0; i < tr_ctx->timestamps.size(); i++)
            {
              Block c_blk;
              float time_since_start = std::chrono::duration<float, std::milli>(tr_ctx->timestamps[i] - tr_ctx->timestamps.front()).count();
              save_to_blk(tr_ctx->cameras[i], &c_blk);
              c_blk.add_double("time_since_start", time_since_start);
              blk.add_block("camera", &c_blk);
              save_block_to_file(filename, blk);
            }

            delete ui_ctx.active_cmd;
            ui_ctx.active_cmd = nullptr;
          }
          else
          {
            auto t_now = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration<float, std::milli>(t_now - tr_ctx->timestamps.back()).count() > timestamp_interval)
            {
              tr_ctx->timestamps.push_back(t_now);
              tr_ctx->cameras.push_back(settings.camera);
            }
          }
        }

        ImGui::SameLine();
        bool take_screenshot = ImGui::Button("Take screenshot");

        ImGui::SameLine();
        ImGui::Checkbox("High quality", &ui_ctx.highQualityScreenshot);
        
        if (take_screenshot)
        {  
          if ((settings.render_mode == RenderMode::RASTERIZATION && ui_ctx.rasterizationSceneSetUp) ||
              (settings.render_mode == RenderMode::   RAYTRACING && ui_ctx.   raytracingSceneSetUp))
          {
            std::string name = "saves/screenshots/" + get_time_str() + ".png";
            Block blk;
            blk.set_string("save_path", name.c_str());
            blk.set_int("rtId", settings.raytracerId);
            blk.set_int("spp", ui_ctx.highQualityScreenshot ? 16 : 1);
            Block blkc;
            create_camera_block(settings, blkc);
            cmd_buffer.push(Cmd::SET_CAMERA, blkc); 
            cmd_buffer.push(Cmd::TAKE_SCREENSHOT, blk);
          }
          else
          {
            add_to_log({"Failed to take screenshot", ui_ctx.logShowTime, LogEntry::Type::WARNING}, ui_ctx);
          }
        }
      }

      //current scene
      if (ui_ctx.rasterizationSceneSetUp || ui_ctx.raytracingSceneSetUp)
      {
        if (ui_ctx.raytracersCount > 1)
        {
          bool clicked = ImGui::Button("<"); 
          ImGui::SameLine();

          if (clicked)
            settings.raytracerId = (settings.raytracerId+ui_ctx.raytracersCount-1) % ui_ctx.raytracersCount;
        }
        
        if (ui_ctx.raytracersCount > 1)
          ImGui::Text("scene %d/%d", settings.raytracerId+1, ui_ctx.raytracersCount);
        else
          ImGui::Text("scene");

        if (ui_ctx.raytracersCount > 1)
        {
          ImGui::SameLine();
          bool clicked = ImGui::Button(">"); 

          if (clicked)
            settings.raytracerId = (settings.raytracerId+1) % ui_ctx.raytracersCount;
        }

        ImGui::Text("models: %d", (int)ui_ctx.scenesCtx[settings.raytracerId].geometries.size());
        ImGui::Text("instances: %d", (int)ui_ctx.scenesCtx[settings.raytracerId].instances.size());

        ImGui::Text("Output folder");
        ImGui::InputTextEx("##scene_out", nullptr, ui_ctx.saveSceneOutputFolder, sizeof(ui_ctx.saveSceneOutputFolder),
                         ImVec2(0.9f*ImGui::GetWindowWidth(), 0), 0);    
      
        bool save = ImGui::Button("Save scene");
        if (save)
        {
          std::string xml_filename = std::string(ui_ctx.saveSceneOutputFolder) + "/scene.xml";
          Block blk;
          blk.add_int("scene_id", settings.raytracerId);
          blk.add_string("xml_filename", xml_filename);
          blk.add_string("folder", ui_ctx.saveSceneOutputFolder);
          cmd_buffer.push(Cmd::SAVE_SCENE, blk);
        }
      }
      else
      {
        ImGui::Text("No scene is loaded");
        ImGui::NewLine();
      }
      ImGui::NewLine();
      
      //loading scene
      ImGui::Text("load scene");
      ImGui::InputTextEx("##scene", nullptr, ui_ctx.userScenePathTextBoxes[0], sizeof(ui_ctx.userScenePathTextBoxes[0]),
                         ImVec2(0.9f*ImGui::GetWindowWidth(), 0), 0);
      
      ImGui::Checkbox("add reference scene", &ui_ctx.showReferenceSceneTextBoxes);
      if (ui_ctx.showReferenceSceneTextBoxes)
      {
        ui_ctx.scenePathTextBoxesCount = MAX_RENDERERS;
        for (int i = 1; i < ui_ctx.scenePathTextBoxesCount; i++)
        {
          std::string label = "reference scene "+std::to_string(i);
          std::string name = "##ref_scene_"+std::to_string(i);
          ImGui::Text("%s",label.c_str());
          ImGui::InputTextEx(name.c_str(), nullptr, ui_ctx.userScenePathTextBoxes[i], sizeof(ui_ctx.userScenePathTextBoxes[i]),
                             ImVec2(0.9f*ImGui::GetWindowWidth(), 0), 0);
        }
      }
      else
      {
        ui_ctx.scenePathTextBoxesCount = 1;
      }

      bool live_metrics_changes = ImGui::Checkbox("calculate live metrics", &ui_ctx.calculateLiveMetrics);
      if (live_metrics_changes)
      {
        Block blk;
        blk.add_bool("enable", ui_ctx.calculateLiveMetrics);
        cmd_buffer.push(Cmd::SET_LIVE_METRICS_CALCULATION, blk);
      }

      bool reload_scene = ImGui::Button("Load scene");
      ImGui::SliderFloat3("Background color", settings.backgroundColor.M, 0.0f, 1.0f);
      bool update_light = false;
      update_light |= ImGui::ColorEdit3("Meshes base color", settings.lightColor.M, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoInputs);
      update_light |= ImGui::SliderFloat3("Light source position", settings.lightPos.M, -10.f, 10.f);
      update_light |= ImGui::SliderFloat("Light intensity", &settings.lightIntensity, 0.25f, 5.0f);
      update_light |= ImGui::Checkbox("Camera light", &ui_ctx.useCameraLight);
      if (update_light || ui_ctx.useCameraLight)
      {
        update_lights(settings, ui_ctx, rt_ctx, rast_ctx, r_ctx);
      }

      ImGui::NewLine();

      if (reload_scene)
      {
        std::vector<std::string> paths;
        for (int i = 0; i < ui_ctx.scenePathTextBoxesCount; i++)
        {
          if (ui_ctx.userScenePathTextBoxes[i][0] != '\0')
          {
            paths.push_back(std::string(ui_ctx.userScenePathTextBoxes[i]));
          }
        }
        Block paths_blk;
        paths_blk.add_arr("paths", paths);
        cmd_buffer.push(Cmd::LOAD_SCENE, paths_blk);
      }

      ImGui::End();
    }

    {
      if (!ui_ctx.rasterizationSceneSetUp && !ui_ctx.raytracingSceneSetUp)
      {
        int w = ui_ctx.window_width;
        int h = ui_ctx.window_height;
        ImVec2 sz = ImVec2(500, 0.7*h);

        ImGui::Begin("Renderer settings");
        ImGui::SetWindowPos(ImVec2(0, 0.4*h));
        ImGui::SetWindowSize(sz);  

        static const char *render_mode_items[] = {"Rasterization", "Ray tracing"};
        bool chosen0 = ImGui::ListBox("RenderMode", (int*)&settings.render_mode, render_mode_items, 2);
        bool chosen1 = ImGui::ListBox("Renderer", (int*) &ui_ctx.chosenRenderer, renderer_items, IM_ARRAYSIZE(renderer_items));
        bool chosen2 = ImGui::ListBox("Backend", (int*) &ui_ctx.chosenRenderDevice, gpu_mode_items, IM_ARRAYSIZE(gpu_mode_items));
        bool chosen3 = ImGui::ListBox("Rasterizer", (int*)&ui_ctx.chosenRasterizer, rasterizer_items, IM_ARRAYSIZE(rasterizer_items));
        if (chosen0 | chosen1 | chosen2 | chosen3)
        {
          Block blk;
          blk.add_enum("render_mode", "RenderMode", (uint32_t)settings.render_mode);
          blk.add_enum("render_type", "RenderType", (uint32_t)renderer_types[ui_ctx.chosenRenderer]);
          blk.add_enum("render_device", "RenderDevice", gpu_mode_devices[ui_ctx.chosenRenderDevice]);
          blk.add_enum("rasterizer_type", "RasterizerType", (uint32_t)ui_ctx.chosenRasterizer);
          cmd_buffer.push(Cmd::SET_RENDER_DEVICE, blk);
        }

        ImGui::End();    
      }
      else if (settings.render_mode == RenderMode::RASTERIZATION)
      {
        int w = ui_ctx.window_width;
        int h = ui_ctx.window_height;
        ImVec2 sz = ImVec2(500, h/2);

        ImGui::Begin("Rasterizer settings");
        ImGui::SetWindowPos(ImVec2(0, h/2));
        ImGui::SetWindowSize(sz);
        if(ImGui::Checkbox("Wireframe mode", &settings.wireframeMode) || ImGui::Checkbox("Show normals", &settings.rasterizerNormals))
          cmd_buffer.push(Cmd::REFRESH_RASTERIZATION);

        {
          auto& rasterizer_names = rast_ctx.rasterizer_names();
          static int chosen_rasterizer = 0;
          bool chosen = ImGui::ListBox("Active rasterizer", &chosen_rasterizer, rasterizer_names.data(), rasterizer_names.size());
          if (chosen) {
            rast_ctx.set_active_rasterizer(chosen_rasterizer);
          }
        }
        
        {
          IRasterizer::Shaders shaders = rast_ctx.active_rasterizer()->getShaderPaths();
          ImGui::Text("Vertex shader: %s", shaders.vertex.string().c_str());
          ImGui::Text("Fragment shader: %s", shaders.fragment.string().c_str());
          if (ImGui::Button("Recompile shaders")) 
          {
            rast_ctx.active_rasterizer()->CompileShaders();
            rast_ctx.refresh(settings.wireframeMode);
          }
        }

        ImGui::End();
      }
      else if (settings.render_mode == RenderMode::RAYTRACING)
      {
        int w = ui_ctx.window_width;
        int h = ui_ctx.window_height;
        ImVec2 sz = ImVec2(500, 0.6*h);

        ImGui::Begin("Viewer settings");
        ImGui::SetWindowPos(ImVec2(0, 0.4*h));
        ImGui::SetWindowSize(sz);

        if (ImGui::Checkbox("Anti-aliasing", &ui_ctx.enableAA))
        {
          Block blk;
          blk.add_bool("enable", ui_ctx.enableAA);
          cmd_buffer.push(Cmd::SET_AA, blk);
        }
        if (ui_ctx.enableAA) {
          ImGui::SliderFloat("AA Factor", &settings.AAFactor, 0.01f, 0.2f);
        }

        if (ImGui::Checkbox("Denoiser", &ui_ctx.enableDenoiser))
        {
          Block blk;
          blk.add_bool("enable", ui_ctx.enableDenoiser);
          cmd_buffer.push(Cmd::SET_DENOISER, blk);
        }


        if (ui_ctx.enableDenoiser) {
          if (ImGui::SliderFloat("Sigma R", &ui_ctx.sigmaR, 0.01f, 10.0f)) {
            Block blk;
            blk.add_double("sigma_r", ui_ctx.sigmaR);
            cmd_buffer.push(Cmd::SET_DENOISER_SIGMA_R, blk);
          }
          if (ImGui::SliderFloat("Sigma S", &ui_ctx.sigmaS, 0.01f, 10.0f)) {
            Block blk;
            blk.add_double("sigma_s", ui_ctx.sigmaS);
            cmd_buffer.push(Cmd::SET_DENOISER_SIGMA_S, blk);
          }
          if (ImGui::SliderInt("Kernel Size", &ui_ctx.kernelSize, 1, 10)) {
            Block blk;
            blk.add_int("kernel_size", ui_ctx.kernelSize);
            cmd_buffer.push(Cmd::SET_DENOISER_KERNEL_SIZE, blk);
          }
        }
        
        ImGui::Checkbox("Cutting plane", &ui_ctx.cuttingPlaneActive);

        if (ui_ctx.scenesCtx[settings.raytracerId].is_CAE_dataset)
          ImGui::Checkbox("CAE visualization", &ui_ctx.datastVisualizationActive);

        scene_modification_interface(settings, ui_ctx, r_ctx, rt_ctx);
        LiteApp::blk_modification_interface(&ui_ctx.RTPresetBlk, "Raytracer settings");

        if (ui_ctx.scenesCtx[settings.raytracerId].is_CAE_dataset && ui_ctx.datastVisualizationActive)
        {
          CAE_visualization_GUI(settings.raytracerId, ui_ctx);

          if (ui_ctx.CAEVisSettings.channel == -1 ||
              ui_ctx.CAEVisSettings.channel >= ui_ctx.scenesCtx[settings.raytracerId].cae_info.names.size()-1)
          {
            ui_ctx.RTPresetBlk.set_enum("render_mode", "MultiRenderMode", MULTI_RENDER_MODE_LAMBERT_NO_TEX);
          }
          else
          {
            if (ui_ctx.CAEVisSettings.lambert_shading == true)
              ui_ctx.RTPresetBlk.set_enum("render_mode", "MultiRenderMode", MULTI_RENDER_MODE_CHANNEL_LAMBERT);
            else
              ui_ctx.RTPresetBlk.set_enum("render_mode", "MultiRenderMode", MULTI_RENDER_MODE_CHANNEL);
            ui_ctx.RTPresetBlk.set_enum("channel_render_mode", "ChannelRenderMode", ui_ctx.CAEVisSettings.vis_mode);
            // there are visual artifacts with rendering internal nodes using Newton/Analytic mode, so set IT for CAE
            ui_ctx.RTPresetBlk.set_enum("sdf_node_intersect", "SDFNodeIntersect", SDF_OCTREE_NODE_INTERSECT_IT);
            ui_ctx.RTPresetBlk.set_int("channel_id", ui_ctx.CAEVisSettings.channel);
            ui_ctx.RTPresetBlk.set_int("component_id", ui_ctx.CAEVisSettings.component);
            ui_ctx.RTPresetBlk.set_int("CTF_id", (uint32_t)ui_ctx.CAEVisSettings.ctf_preset);
          }
        }

        if (ui_ctx.cuttingPlaneActive && ui_ctx.scenesCtx[settings.raytracerId].vtkDataset.unstructured_grids.size() > 0)
        {
          bool cut = ImGui::Button("Cut with plane");
          if (cut)
          {
            Block blk;
            // move plane a bit forward to avoid visual artifacts
            blk.add_vec3("plane_pos", settings.cutting_plane.pos + 0.0001f*settings.cutting_plane.normal);
            blk.add_vec3("plane_normal", settings.cutting_plane.normal);
            cmd_buffer.push(Cmd::CUT_VTK_DATASET, blk);
          }
        }

        ImGui::End();
      }
    }
  }

  void setup_GUI_elements(CommandBuffer &cmd_buffer, Settings &settings, UIContext &ui_ctx, RenderingContext &r_ctx,
                          RasterizationContext &rast_ctx, RaytracingContext &rt_ctx)
  {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    base_GUI(cmd_buffer, settings, ui_ctx, r_ctx, rast_ctx, rt_ctx);
    if (ui_ctx.activePlugin)
    {
      ImGui::SetNextWindowPos(ImVec2(500, 0));
      ImGui::Begin("Plugin");
      ui_ctx.activePlugin->drawInterface(cmd_buffer, settings, ui_ctx);
      ImGui::End();
    }

    if (ImGui::GetActiveID() != 0)
      ui_ctx.hotkeysBlockCooldown = ui_ctx.hotkeysBlockDuration;

    ImGui::Render();
  }

  void calculate_image_metrics(Settings &settings, RenderingContext &r_ctx, RaytracingContext &rt_ctx)
  {
    if (r_ctx.renderDevice == DEVICE_CPU)
    {
      rt_ctx.pMetricsKernel->CalcArraySumm(rt_ctx.raytracedImagesData[0].data(),
                                           rt_ctx.raytracedImagesData[1].data(),
                                           r_ctx.width * r_ctx.height);
    }
    else
    {
      VkCommandBuffer commandBuffer = vk_utils::createCommandBuffer(r_ctx.device, r_ctx.commandPool);
      VkCommandBufferBeginInfo beginCommandBufferInfo = {};
      beginCommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      beginCommandBufferInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

      vkBeginCommandBuffer(commandBuffer, &beginCommandBufferInfo);
      dynamic_cast<ImgMetricsKernel_glsl*>(rt_ctx.pMetricsKernel.get())->CalcArraySummCmd(commandBuffer, nullptr, nullptr, r_ctx.width * r_ctx.height);
      vkEndCommandBuffer(commandBuffer);
      vk_utils::executeCommandBufferNow(commandBuffer, r_ctx.graphicsQueue, r_ctx.device);
      dynamic_cast<ImgMetricsKernel_glsl*>(rt_ctx.pMetricsKernel.get())->ReadPlainMembers(r_ctx.pCopyHelper);
    }
  }

  void recreate_swapchain(const Settings &settings, RenderingContext &r_ctx, RasterizationContext &rast_ctx, 
                          RaytracingContext &rt_ctx)
  {
    recreate_swapchain_base(r_ctx);
    setup_RT_image(r_ctx, rt_ctx);
    setup_quad(r_ctx, settings.quadVertexShaderPath.c_str(), settings.quadFragmentShaderPath.c_str());
    on_screen_resolution_change_RT(r_ctx, rt_ctx);
    on_swapchain_changed_imGui(r_ctx);
  }

  void draw_frame(float a_time, CommandBuffer &cmd_buffer, Settings &settings, UIContext &ui_ctx, RenderingContext &r_ctx, 
                  RasterizationContext &rast_ctx, RaytracingContext &rt_ctx)
  {
    if ( ui_ctx.sceneProvided && 
         ui_ctx.sceneSupportsRasterization &&
        !ui_ctx.rasterizationSceneSetUp && 
         settings.render_mode == RenderMode::RASTERIZATION)
    {
      cmd_buffer.push(Cmd::INIT_RASTERIZATION);
      return;
    }

    if ( ui_ctx.sceneProvided && 
        !ui_ctx.raytracingSceneSetUp && 
         settings.render_mode == RenderMode::RAYTRACING)
    {
      cmd_buffer.push(Cmd::INIT_RAYTRACING);
      return;
    }

    if (ui_ctx.rasterizationSceneSetUp && settings.render_mode == RenderMode::RASTERIZATION)
    {
      rast_ctx.active_rasterizer()->UpdateUniforms();

      vkWaitForFences(r_ctx.device, 1, &r_ctx.frameFences[r_ctx.currentFrame], VK_TRUE, UINT64_MAX);
      vkResetFences(  r_ctx.device, 1, &r_ctx.frameFences[r_ctx.currentFrame]);
    }

    uint32_t imageIdx;
    auto result = r_ctx.swapchain.AcquireNextImage(r_ctx.imageAvailable, &imageIdx);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
      recreate_swapchain(settings, r_ctx, rast_ctx, rt_ctx);
      return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
      RUN_TIME_ERROR("Failed to acquire the next swapchain image!");
    }

    auto currentCmdBuf = r_ctx.cmdBuffersDrawMain[r_ctx.currentFrame];

    VkSemaphore waitSemaphores[] = {r_ctx.imageAvailable};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    bool noSceneRender = false;

    if (ui_ctx.rasterizationSceneSetUp && settings.render_mode == RenderMode::RASTERIZATION)
    {
      rast_ctx.active_rasterizer()->SetCamera(r_ctx.worldViewMatrix, OpenglToVulkanProjectionMatrixFix() * r_ctx.projectionMatrix);
      rast_ctx.active_rasterizer()->RenderCmd(currentCmdBuf, r_ctx.frameBuffers[imageIdx]);
    }
    else if (ui_ctx.raytracingSceneSetUp && settings.render_mode == RenderMode::RAYTRACING)
    {
      RTLayout layout;
      layout.primaryRTId = settings.raytracerId;
      if (ui_ctx.calculateLiveMetrics)
        layout.secondaryRTId = ui_ctx.referenceRaytracerId;

      raytrace(layout, ui_ctx, r_ctx, rt_ctx);
      LiteApp::build_command_buffer_quad(currentCmdBuf, r_ctx.swapchain.GetAttachment(imageIdx).view, r_ctx.rt_pres);
    }
    else
      noSceneRender = true;

    if (ui_ctx.raytracingSceneSetUp && settings.render_mode == RenderMode::RAYTRACING &&
        ui_ctx.calculateLiveMetrics && ui_ctx.raytracersCount == 2)
    {
      calculate_image_metrics(settings, r_ctx, rt_ctx);
    }

    std::vector<VkCommandBuffer> submitCmdBufs;
    VkCommandBuffer currentGUICmdBuf;
    
    bool force_no_GUI = ui_ctx.rasterizationSceneSetUp && settings.render_mode == RenderMode::RASTERIZATION && rast_ctx.active_rasterizer()->NeedScreenshot(); // I don't know if it is ever needed, but you can disable GUI entirely
    if (!force_no_GUI)
    {
      if (!ui_ctx.minimalUI || noSceneRender)
      {
        setup_GUI_elements(cmd_buffer, settings, ui_ctx, r_ctx, rast_ctx, rt_ctx);

      }
      else
        setup_minimal_GUI_elements(cmd_buffer, settings, ui_ctx, r_ctx, rast_ctx, rt_ctx);
      ImDrawData* pDrawData = ImGui::GetDrawData();
      currentGUICmdBuf = build_command_buffer_imGui(imageIdx, pDrawData, r_ctx);
      
      if (noSceneRender)
        submitCmdBufs = {currentGUICmdBuf};
      else
        submitCmdBufs = {currentCmdBuf, currentGUICmdBuf};
    }
    else
    {
      submitCmdBufs = {currentCmdBuf};
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = (uint32_t)submitCmdBufs.size();
    submitInfo.pCommandBuffers = submitCmdBufs.data();

    VkSemaphore signalSemaphores[] = {r_ctx.renderingFinished};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    VK_CHECK_RESULT(vkResetFences(r_ctx.device, 1, &r_ctx.frameFences[r_ctx.currentFrame]));
    VK_CHECK_RESULT(vkQueueSubmit(r_ctx.graphicsQueue, 1, &submitInfo, r_ctx.frameFences[r_ctx.currentFrame]));

    VkResult presentRes = r_ctx.swapchain.QueuePresent(r_ctx.queue, imageIdx, r_ctx.renderingFinished);

    if (force_no_GUI)
      rast_ctx.active_rasterizer()->TakeScreenshotCMD(r_ctx.swapchain.GetAttachment(r_ctx.currentFrame).image);

    if (presentRes == VK_ERROR_OUT_OF_DATE_KHR || presentRes == VK_SUBOPTIMAL_KHR)
    {
      recreate_swapchain(settings, r_ctx, rast_ctx, rt_ctx);
    }
    else if (presentRes != VK_SUCCESS)
    {
      RUN_TIME_ERROR("Failed to present swapchain image");
    }

    r_ctx.currentFrame = (r_ctx.currentFrame + 1) % r_ctx.framesInFlight;

    vkQueueWaitIdle(r_ctx.queue);
  }

  void app_tick(float secondsElapsed, Settings &settings, UIContext &ui_ctx, RaytracingContext &rt_ctx, RenderingContext &r_ctx)
  {
    ui_ctx.hotkeysBlockCooldown -= secondsElapsed;
    settings.cameraMoving = false;

    auto log_iter = ui_ctx.logToShow.begin();
    while (log_iter != ui_ctx.logToShow.end())
    {
      log_iter->time -= secondsElapsed;
      if (log_iter->time <= 0)
        log_iter = ui_ctx.logToShow.erase(log_iter);
      else
        ++log_iter;
    }

    for (int i = 0; i < MAX_RENDERERS; i++)
    {
      if (rt_ctx.pRayTracers[i])
        rt_ctx.pRayTracers[i]->nextFrame(secondsElapsed);
    }

    if (ui_ctx.active_cmd == nullptr)
      return;
    
    if (ui_ctx.active_cmd->cmd == Cmd::PLAYBACK_TRAJECTORY)
    {
      auto *pback = dynamic_cast<PlaybackTrajectoryCmdCtx *>(ui_ctx.active_cmd);
      assert(pback);

      if (pback->time_since_start_ms == 0)
        add_to_log({"Playback started", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);

      pback->time_since_start_ms += 1000.0f*secondsElapsed;
      while (pback->cur_index < pback->times.size() && pback->time_since_start_ms > pback->times[pback->cur_index])
      {
        pback->cur_index++;
      }

      if (pback->time_since_start_ms > pback->times.back())
      {
        add_to_log({"Playback completed", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
        delete ui_ctx.active_cmd;
        ui_ctx.active_cmd = nullptr;
      }
      else
      {
        float prev_t = pback->times[pback->cur_index - 1];
        float next_t = pback->times[pback->cur_index];
        float t = (pback->time_since_start_ms - prev_t) / (next_t - prev_t);
        Camera lerp_cam;
        lerp_cam.pos = lerp(pback->cameras[pback->cur_index - 1].pos, pback->cameras[pback->cur_index].pos, t);
        lerp_cam.lookAt = lerp(pback->cameras[pback->cur_index - 1].lookAt, pback->cameras[pback->cur_index].lookAt, t);
        lerp_cam.up = lerp(pback->cameras[pback->cur_index - 1].up, pback->cameras[pback->cur_index].up, t);
        lerp_cam.fovy = lerp(pback->cameras[pback->cur_index - 1].fovy, pback->cameras[pback->cur_index].fovy, t);
        lerp_cam.z_near = lerp(pback->cameras[pback->cur_index - 1].z_near, pback->cameras[pback->cur_index].z_near, t);
        lerp_cam.z_far = lerp(pback->cameras[pback->cur_index - 1].z_far, pback->cameras[pback->cur_index].z_far, t);
        settings.camera = lerp_cam;
        settings.cameraMoving = (length(pback->cameras[pback->cur_index - 1].pos - pback->cameras[pback->cur_index].pos) > 0.0001f) ||
                                (length(pback->cameras[pback->cur_index - 1].lookAt - pback->cameras[pback->cur_index].lookAt) > 0.0001f) ||
                                (length(pback->cameras[pback->cur_index - 1].up - pback->cameras[pback->cur_index].up) > 0.0001f) ||
                                (abs(pback->cameras[pback->cur_index - 1].fovy - pback->cameras[pback->cur_index].fovy) > 0.0001f);
      }
    }
  }

  void update_raytracer_settings(const Settings &settings, UIContext &ui_ctx, const RenderingContext &r_ctx, RaytracingContext &rt_ctx)
  {
    for (int i = 0; i < MAX_RENDERERS; i++)
    {
      if (!rt_ctx.pRayTracers[i])
        continue;
      rt_ctx.pRayTracers[i]->setCuttingPlane(settings.cutting_plane);
      rt_ctx.pRayTracers[i]->setBackgroundColor(to_float4(settings.backgroundColor, 1.0f));
      rt_ctx.pRayTracers[i]->SetHighlightPreset(settings.highlight_preset);
      ui_ctx.activePlugin->setRenderSettingsBlk(rt_ctx.pRayTracers[i].get(), ui_ctx.RTPresetBlk);
    }
  }

  void frame(CommandBuffer &cmd_buffer, Settings &settings, UIContext &ui_ctx,  RenderingContext &r_ctx, 
             RasterizationContext &rast_ctx, RaytracingContext &rt_ctx)
  {
    if (glfwWindowShouldClose(r_ctx.window))
    {
      cmd_buffer.push(Cmd::SET_RENDER_SETTINGS, ui_ctx.RTPresetBlk); //show last settings in the log, useful if we want to rerun program
      cmd_buffer.push(Cmd::EXIT);
      return;
    }
    static double lastTime = glfwGetTime();
    double thisTime = glfwGetTime();
    double diffTime = thisTime - lastTime;
    lastTime = thisTime;

    LiteApp::clearKeys();
    glfwPollEvents();
    app_tick((float)diffTime, settings, ui_ctx, rt_ctx, r_ctx);
    process_input((float)diffTime, cmd_buffer, settings, ui_ctx, r_ctx, rast_ctx, rt_ctx);
    
    if (!glfwGetWindowAttrib(r_ctx.window, GLFW_ICONIFIED)) {
      {
        int w, h;
        glfwGetWindowSize(r_ctx.window, &w, &h);
        if (w && h && !(w == r_ctx.width && h == r_ctx.height)) {
          w = w / 8 * 8;
          h = h / 8 * 8;
          glfwSetWindowSize(r_ctx.window, w, h);
          glfwGetWindowSize(r_ctx.window, &w, &h);
          r_ctx.width = w;
          r_ctx.height = h;
          ui_ctx.window_width = r_ctx.width;
          ui_ctx.window_height = r_ctx.height;
          recreate_swapchain(settings, r_ctx, rast_ctx, rt_ctx);
          return;
        }
      }

      if (ui_ctx.useCameraLight)
        update_lights(settings, ui_ctx, rt_ctx, rast_ctx, r_ctx);

      update_view(settings, r_ctx);
      if (rast_ctx.active_rasterizer()) {
        rast_ctx.active_rasterizer()->SetBackgroundColor(settings.backgroundColor);
        
      }
      update_antialiasing_settings(settings, ui_ctx,r_ctx, rt_ctx);
      update_raytracer_settings(settings, ui_ctx, r_ctx, rt_ctx);

      draw_frame((float)thisTime, cmd_buffer, settings, ui_ctx, r_ctx, rast_ctx, rt_ctx);

      ui_ctx.currentTimeIndex = (ui_ctx.currentTimeIndex + 1) % ui_ctx.sliding_window_size;
      ui_ctx.frameCounter++;
    }
  }

  VkPhysicalDeviceFeatures2 pluginGetDeviceFeatures(std::vector<const char *> &deviceExtensions, const RenderingContext &r_ctx)
  {

    std::vector<RenderType> types = {RenderType::MULTI, RenderType::HYDRA, RenderType::VOLUME, RenderType::VOXEL, RenderType::CSG_DIRECT};
    for (auto type : types)
    {
      auto plugin = getPlugin(type);
      if (plugin)
        return plugin->listRequiredDeviceFeatures(deviceExtensions);
    }
    return VkPhysicalDeviceFeatures2();
  }

  void init(const Block &blk, Settings &settings, UIContext &ui_ctx, RenderingContext &r_ctx, 
            RasterizationContext &rast_ctx, RaytracingContext &rt_ctx)
  {
    add_to_log({"Initializing application", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);

    settings.deviceID = blk.get_int("deviceId", settings.deviceID);
    settings.desired_window_width = blk.get_int("window_width", settings.desired_window_width);
    settings.desired_window_height = blk.get_int("window_height", settings.desired_window_height);
    settings.fullscreen = blk.get_bool("fullscreen", settings.fullscreen);
    settings.highlight_preset = getEmptyHighlightPreset();

    LiteApp::AppInitSettings appSettings;
    appSettings.deviceId = settings.deviceID;
    appSettings.pDeviceFeaturesProvider = pluginGetDeviceFeatures;
    appSettings.fullscreen = settings.fullscreen;
    appSettings.desiredWidth = settings.desired_window_width;
    appSettings.desiredHeight = settings.desired_window_height;

    LiteApp::init_app(r_ctx, appSettings);
    LiteApp::init_ImGui(r_ctx);

    ui_ctx.window_width = r_ctx.width;
    ui_ctx.window_height = r_ctx.height;
    ui_ctx.renderSetUp = true;

    setup_RT_image(r_ctx, rt_ctx);
    setup_quad(r_ctx, settings.quadVertexShaderPath.c_str(), settings.quadFragmentShaderPath.c_str());

    update_view(settings, r_ctx);
    update_antialiasing_settings(settings, ui_ctx,r_ctx, rt_ctx);
    update_raytracer_settings(settings, ui_ctx, r_ctx, rt_ctx);
  }
}