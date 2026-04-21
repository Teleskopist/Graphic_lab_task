#include "app.h"
#include "app_internal.h"
#include "volk.h"
#include "vk_utils.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/imgui_internal.h"
#include "utils/common/csv.h"
#include "utils/common/strings.h"
#include "utils/common/position_hash.h"
#include "utils/common/primitive_list_octree.h"
#include "utils/misc/image_metrics.h"
#include "utils/raytracing/render_common.h"
#include "utils/mesh/mesh_internal.h"
#include "utils/mesh/stl_reader.h"
#include "utils/mesh/triangle_list_octree.h"
#include "utils/vtk/parser.h"
#include "utils/vtk/procedural.h"
#include "core/ICAERenderer.h"
#include "core/serialization.h"
#include "core/scene_extension.h"

#include "extended/rasterization/ImageTerrainRasterizer.h"
#include "extended/rasterization/BCImageTerrainRasterizer.h"
#include "extended/rasterization/QuadTreeTerrainRasterizer.h"
#include "plugins/plugins.h"

#include <cstdio>
#include <chrono>
#include <filesystem>

namespace demo_app
{
  bool check_file(const char *path, const char *file_name_for_log, UIContext &ui_ctx)
  {
    auto file_type = std::filesystem::status(path).type();

    if (file_type == std::filesystem::file_type::regular)
    {
      return true;
    }
    
    if (file_type == std::filesystem::file_type::not_found)
    {
      add_to_log({std::string(file_name_for_log)+" not found: " + std::string(path), ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
      return false;
    }
    
    add_to_log({std::string(file_name_for_log)+" is not a file: " + std::string(path), ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
    return false;
  }

  void fill_scene_CAE_info(UIContext &ui_ctx, int id, const std::vector<DataChannelInfo> &point_channels,
                           const std::vector<DataChannelInfo> &voxel_channels)
  {
    ui_ctx.scenesCtx[id].is_CAE_dataset = true;

    ui_ctx.scenesCtx[id].cae_info = SceneUIContext::CAEInfo();
    ui_ctx.scenesCtx[id].cae_info.names.resize(point_channels.size() + voxel_channels.size() + 1);
    ui_ctx.scenesCtx[id].cae_info.c_names.resize(point_channels.size() + voxel_channels.size() + 1);
    ui_ctx.scenesCtx[id].cae_info.num_components.resize(point_channels.size() + voxel_channels.size() + 1);

    for (size_t i = 0; i < point_channels.size(); i++)
    {
      ui_ctx.scenesCtx[id].cae_info.names[i] = "[P] " + point_channels[i].name;
      ui_ctx.scenesCtx[id].cae_info.c_names[i] = ui_ctx.scenesCtx[id].cae_info.names[i].c_str();
      ui_ctx.scenesCtx[id].cae_info.num_components[i] = point_channels[i].num_components;
    }

    int off = point_channels.size();
    for (size_t i = 0; i < voxel_channels.size(); i++)
    {
      ui_ctx.scenesCtx[id].cae_info.names[i + off] = "[C] " + voxel_channels[i].name;
      ui_ctx.scenesCtx[id].cae_info.c_names[i + off] = ui_ctx.scenesCtx[id].cae_info.names[i + off].c_str();
      ui_ctx.scenesCtx[id].cae_info.num_components[i + off] = voxel_channels[i].num_components;
    }

    ui_ctx.scenesCtx[id].cae_info.names.back() = "Solid color";
    ui_ctx.scenesCtx[id].cae_info.c_names.back() = ui_ctx.scenesCtx[id].cae_info.names.back().c_str();
    ui_ctx.scenesCtx[id].cae_info.num_components.back() = 1;
  }

  void initialize_scene_context(UIContext &ui_ctx, int i)
  {
    std::map<uint32_t, uint32_t> geom_id_to_idx;
    if (ui_ctx.scenes[i].geometries.empty() || ui_ctx.scenes[i].scenes.empty())
      return;
    
    ui_ctx.scenesCtx[i].geometries.resize(ui_ctx.scenes[i].geometries.size());
    ui_ctx.scenesCtx[i].instances.resize(ui_ctx.scenes[i].scenes[0].instances.size());

    int idx = 0;
    for (const auto &[geom_id, geom] : ui_ctx.scenes[i].geometries)
    {
      geom_id_to_idx[geom_id] = idx;
      ui_ctx.scenesCtx[i].geometries[idx].h_geom_id = geom_id;
      ui_ctx.scenesCtx[i].geometries[idx].name = geom->name;
      idx++;
    }

    idx = 0;
    for (const auto &[instance_id, instance] : ui_ctx.scenes[i].scenes[0].instances)
    {
      ui_ctx.scenesCtx[i].instances[idx].h_inst_id = instance_id;
      ui_ctx.scenesCtx[i].instances[idx].h_geom_id = instance.mesh_id;
      ui_ctx.scenesCtx[i].instances[idx].geom_id   = geom_id_to_idx[instance.mesh_id];
      ui_ctx.scenesCtx[i].instances[idx].visible = true;
      idx++;
    }
  }

  void reload_scenes(int count, UIContext &ui_ctx)
  {
    ui_ctx.raytracersCount = count;
    ui_ctx.sceneProvided = true;
    ui_ctx.rasterizationSceneSetUp = false;
    ui_ctx.raytracingSceneSetUp = false;
    ui_ctx.sceneSupportsRasterization = ui_ctx.activeScenePaths[0] != "";
  }

  void load_scene_from_xml(const char* path, int id, UIContext &ui_ctx)
  {
    ui_ctx.activeScenePaths[id] = "";
    add_to_log({"Loading hydra scene from xml: " + std::string(path), ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
    bool loaded = ui_ctx.scenes[id].load(path);

    if (!loaded)
    {
      add_to_log({"Error loading scene xml: " + std::string(path), ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
      return;
    }
    add_to_log({"Loaded xml", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
    add_to_log({"Loaded hydra scene: " + std::string(path), ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
    ui_ctx.activeScenePaths[id] = path;
  }

  void schedule_scene_load(std::vector<const char*> paths, const Block &args, UIContext &ui_ctx)
  {
    assert(paths.size() > 0);
    assert(paths.size() <= MAX_RENDERERS);

    ui_ctx.fixedTransformMatrixSetUp = false;

    bool all_paths_valid = true;
    for (int i = 0; i < paths.size(); i++)
      all_paths_valid &= check_file(paths[i], "Scene", ui_ctx);

    if (all_paths_valid)
    {
      for (int i = 0; i < paths.size(); i++)
      {
        ui_ctx.scenes[i].clear();
        std::string file_ext = file_extension(paths[i]);

        if (file_ext == "xml")
          load_scene_from_xml(paths[i], i, ui_ctx);
        else if (ui_ctx.activePlugin)
          ui_ctx.activePlugin->loadScene(args, paths[i], i, ui_ctx);
        
        initialize_scene_context(ui_ctx, i);
      }

      reload_scenes(paths.size(), ui_ctx);
    }
  }

  void cut_vtk_dataset(unsigned scene_id, const Block &settings_blk, UIContext &ui_ctx)
  {
    ui_ctx.scenes[scene_id].clear();    

    vtk::Dataset &dataset = ui_ctx.scenesCtx[scene_id].vtkDataset;
    for (auto &ug : dataset.unstructured_grids)
    {
      Plane plane;
      plane.is_active = true;
      plane.pos = settings_blk.get_vec3("plane_pos");
      // flip normal because plane cutting in render and here have opposite convention about what to cut
      plane.normal = -1.0f*settings_blk.get_vec3("plane_normal");
      auto mesh = vtk::triangulate_grid_bounds_with_plane(ug, plane, true);

      LiteScene::AugmentedMeshGeometry *geom = new LiteScene::AugmentedMeshGeometry();
      int geomId = ui_ctx.scenes[scene_id].add_geometry(geom);
      geom->init(std::move(mesh)); // init after add_geometry, because only add_geometry puts custom data to it
      ui_ctx.scenes[scene_id].add_instance(geomId, LiteMath::float4x4());
    }

    initialize_scene_context(ui_ctx, scene_id);
    reload_scenes(ui_ctx.raytracersCount, ui_ctx);
  }

  void load_frame(LiteImage::Image2D<uint32_t> &image, int rtId, 
                  const RenderingContext &r_ctx, const RaytracingContext &rt_ctx)
  {
    if (r_ctx.renderDevice == DEVICE_CPU)
      memcpy(image.data(), rt_ctx.raytracedImagesData[rtId].data(), r_ctx.width * r_ctx.height * sizeof(uint32_t));
    else
      r_ctx.pCopyHelper->ReadBuffer(rt_ctx.genColorBuffers[rtId], 0, image.data(), r_ctx.width * r_ctx.height * sizeof(uint32_t));
  }

  void take_screenshot_RT(const char* save_path, int rtId, const RenderingContext &r_ctx, const RaytracingContext &rt_ctx)
  {
    LiteImage::Image2D<uint32_t> image(r_ctx.width, r_ctx.height);
    load_frame(image, rtId, r_ctx, rt_ctx);
    
    //turn it upside down and make opaque
    for (uint32_t y = 0; y < r_ctx.height/2; y++)
    {
      for (uint32_t x = 0; x < r_ctx.width; x++)
      {
        auto tmp = image.data()[x + y * r_ctx.width];
        image.data()[x + y * r_ctx.width] = image.data()[x + (r_ctx.height - y - 1) * r_ctx.width] | 0xFF000000;
        image.data()[x + (r_ctx.height - y - 1) * r_ctx.width] = tmp | 0xFF000000;
      }
    }
    std::filesystem::create_directories(std::filesystem::path(save_path).parent_path());
    LiteImage::SaveImage(save_path, image);
  }

  void compare_with_reference(int rtId, int refRtId, UIContext &ui_ctx, const RenderingContext &r_ctx, const RaytracingContext &rt_ctx)
  {
    LiteImage::Image2D<uint32_t> image(r_ctx.width, r_ctx.height);
    load_frame(image, rtId, r_ctx, rt_ctx);
    LiteImage::Image2D<uint32_t> reference(r_ctx.width, r_ctx.height);
    load_frame(reference, refRtId, r_ctx, rt_ctx);
    
    float PSNR = image_metrics::PSNR(reference, image);
    float FLIP = image_metrics::FLIP(reference, image);
    char buf[256];
    sprintf(buf, "PSNR = %.2f, FLIP = %.4f", PSNR, FLIP);

    add_to_log({std::string(buf), ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
  }

  void compare_frames_raytracers(int rtId, int refRtId, Settings &settings, UIContext &ui_ctx, 
                                 const RenderingContext &r_ctx, RaytracingContext &rt_ctx)
  {
    if (settings.render_mode != RenderMode::RAYTRACING)
    {
      add_to_log({"RenderMode must be raytracing", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
      return;
    }

    if (!ui_ctx.raytracingSceneSetUp)
    {
      add_to_log({"Raytracer is not set up", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
      return;
    }

    if (refRtId >= ui_ctx.raytracersCount)
    {
      add_to_log({"Reference raytracer is not set up", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
      return;
    }

    if (rtId >= ui_ctx.raytracersCount)
    {
      add_to_log({"Target raytracer is not set up", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
      return;
    }

    raytrace(render_only_primaryRT(refRtId), ui_ctx, r_ctx, rt_ctx);
    raytrace(render_only_primaryRT(rtId), ui_ctx, r_ctx, rt_ctx);
    
    compare_with_reference(rtId, refRtId, ui_ctx, r_ctx, rt_ctx);
    if (ui_ctx.saveScreenshotOnComparison)
    {
      std::string name = "saves/screenshots/" + get_time_str() + ".png";
      take_screenshot_RT(name.c_str(), rtId, r_ctx, rt_ctx);

      std::string nameRef = "saves/screenshots/" + get_time_str() + "_ref.png";
      take_screenshot_RT(nameRef.c_str(), refRtId, r_ctx, rt_ctx);
    }
  }

  void cleanup_renderers(const RenderingContext &r_ctx, RaytracingContext &rt_ctx)
  {
    for (int i=0; i<MAX_RENDERERS; i++)
    {
      if (rt_ctx.pRayTracers[i])
      {
        rt_ctx.pRayTracers[i].reset();
        rt_ctx.pRayTracers[i] = nullptr;
        rt_ctx.pAntialiaser[i].reset();
        rt_ctx.pAntialiaser[i] = nullptr;
        rt_ctx.pDenoiser[i].reset();
        rt_ctx.pDenoiser[i] = nullptr;
        
        if(rt_ctx.genColorBuffers[i] != VK_NULL_HANDLE)
        {
          vkDestroyBuffer(r_ctx.device, rt_ctx.genColorBuffers[i], nullptr);
          rt_ctx.genColorBuffers[i] = VK_NULL_HANDLE;
        }

        if(rt_ctx.genColorRawBuffers[i] != VK_NULL_HANDLE)
        {
          vkDestroyBuffer(r_ctx.device, rt_ctx.genColorRawBuffers[i], nullptr);
          rt_ctx.genColorRawBuffers[i] = VK_NULL_HANDLE;
        }

        if(rt_ctx.genColorNoAABuffers[i] != VK_NULL_HANDLE)
        {
          vkDestroyBuffer(r_ctx.device, rt_ctx.genColorNoAABuffers[i], nullptr);
          rt_ctx.genColorNoAABuffers[i] = VK_NULL_HANDLE;
        }

        if(rt_ctx.colorMems[i] != VK_NULL_HANDLE)
        {
          vkFreeMemory(r_ctx.device, rt_ctx.colorMems[i], nullptr);
          rt_ctx.colorMems[i]= VK_NULL_HANDLE;
        }
      }
    }

    rt_ctx.pMetricsKernel.reset();
    rt_ctx.pMetricsKernel = nullptr;
  }

  void cleanup(RenderingContext &r_ctx, RasterizationContext &rast_ctx, RaytracingContext &rt_ctx)
  {
    cleanup_imGui(r_ctx);
    cleanup_pipeline_and_swapchain(r_ctx);
    rast_ctx.clear_rasterizers();
    cleanup_renderers(r_ctx, rt_ctx);
    cleanup(r_ctx);
  }

  void execute_command(const CommandBuffer::Command &cmd, CommandBuffer &cmd_buffer, Settings &settings, 
                       UIContext &ui_ctx, RenderingContext &r_ctx, RasterizationContext &rast_ctx, 
                       RaytracingContext &rt_ctx)
  {
    switch (cmd.type)
    {
      case Cmd::NO_OP:
        break;
      case Cmd::INIT:
        if (!ui_ctx.renderSetUp)
          init(cmd.args, settings, ui_ctx, r_ctx, rast_ctx, rt_ctx);
        break;
      case Cmd::EXIT:
        if (ui_ctx.renderSetUp)
          cleanup(r_ctx, rast_ctx, rt_ctx);
        break;
      case Cmd::LOAD_SCENE:
      {
        std::vector<std::string> paths;
        cmd.args.get_arr("paths", paths);
        if (paths.size() == 0 || paths.size() > MAX_RENDERERS)
          add_to_log({"Invalid number of paths: "+std::to_string(paths.size()), ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
        else
        {
          std::vector<const char*> paths_arr;
          for (int i = 0; i < paths.size(); i++)
            paths_arr.push_back(paths[i].c_str());
          schedule_scene_load(paths_arr, cmd.args, ui_ctx);
        }
      }
        break;
      case Cmd::SAVE_SCENE:
      {
        int id = cmd.args.get_int("scene_id");
        std::string xml_filename = cmd.args.get_string("xml_filename");
        std::string folder = cmd.args.get_string("folder");
        bool saved = ui_ctx.scenes[id].save(xml_filename, folder);
        if (saved)
          add_to_log({"Saved scene to: " + xml_filename, ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
        else
          add_to_log({"Error saving scene to: " + xml_filename, ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
      }
        break;
      case Cmd::SET_ENGINE_SETTINGS:
      {
        settings.backgroundColor = cmd.args.get_vec3("backgroundColor", settings.backgroundColor);
        settings.camMoveSpeed = cmd.args.get_double("camMoveSpeed", settings.camMoveSpeed);
        settings.lightColor = cmd.args.get_vec3("lightColor", settings.lightColor);
        settings.lightPos = cmd.args.get_vec3("lightPos", settings.lightPos);
        settings.mouseSensitivity = cmd.args.get_double("mouseSensitivity", settings.mouseSensitivity);
        settings.validation = cmd.args.get_bool("validation", settings.validation);
        settings.wireframeMode = cmd.args.get_bool("wireframeMode", settings.wireframeMode);
      }
        break;
      case Cmd::SET_RENDER_SETTINGS:
      {
        ui_ctx.RTPresetBlk.copy(&cmd.args);
      }
        break;
      case Cmd::SET_CAMERA:
      {
        float3 pos = cmd.args.get_vec3("pos", settings.camera.pos);
        float3 lookAt = cmd.args.get_vec3("lookAt", settings.camera.lookAt);
        float3 up = cmd.args.get_vec3("up", settings.camera.up);
        float fovy = cmd.args.get_double("fovy", settings.camera.fovy);
        float z_near = cmd.args.get_double("z_near", settings.camera.z_near);
        float z_far = cmd.args.get_double("z_far", settings.camera.z_far);

        settings.camera.pos = pos;
        settings.camera.lookAt = lookAt;
        settings.camera.up = up;
        settings.camera.fovy = fovy;
        settings.camera.z_near = z_near;
        settings.camera.z_far = z_far;

        char buf[256];
        snprintf(buf, sizeof(buf), 
                 "camera {pos:p3 = %f, %f, %f lookAt:p3 = %f, %f, %f up:p3 = %f, %f, %f fovy:r = %f z_near:r = %f z_far:r = %f}\n", 
        settings.camera.pos.x, settings.camera.pos.y, settings.camera.pos.z,
        settings.camera.lookAt.x, settings.camera.lookAt.y, settings.camera.lookAt.z,
        settings.camera.up.x, settings.camera.up.y, settings.camera.up.z,
        settings.camera.fovy, settings.camera.z_near, settings.camera.z_far);
        add_to_log({std::string(buf), ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx); 
        
        update_view(settings, r_ctx);
      }
        break;
      case Cmd::TAKE_SCREENSHOT:
      {
        switch (settings.render_mode)
        {
          case RenderMode::RAYTRACING:
          {
            int spp = cmd.args.get_int("spp", 1);
            if (spp > 1) {
              printf("Taking screenshot with spp = %d\n", spp);
            }
            uint32_t prev_spp = ui_ctx.RTPresetBlk.get_int("spp", 1);
            ui_ctx.RTPresetBlk.set_int("spp", spp);
            update_raytracer_settings(settings, ui_ctx, r_ctx, rt_ctx);
            ui_ctx.RTPresetBlk.set_int("spp", prev_spp);
            if (ui_ctx.renderSetUp && ui_ctx.raytracingSceneSetUp)
            {
              std::string save_path = cmd.args.get_string("save_path");
              int rtId = cmd.args.get_int("rtId");
              raytrace(render_only_primaryRT(rtId), ui_ctx, r_ctx, rt_ctx);
              take_screenshot_RT(save_path.c_str(), rtId, r_ctx, rt_ctx);
              add_to_log({"Screenshot saved to: "+save_path, ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
            }
            else
            {
              add_to_log({"TAKE_SCREENSHOT: Render is not set up", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
            }
            break;
          }
          case RenderMode::RASTERIZATION:
          {
            std::string save_path = cmd.args.get_string("save_path");
            rast_ctx.active_rasterizer()->PrepareScreenshot(save_path);
            add_to_log({"Screenshot saved to: "+save_path, ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
            break;
          }
        }
      }
        break;
      case Cmd::INIT_RAYTRACING:
      {        
        for (int i = 0; i < ui_ctx.raytracersCount; i++)
        {
          setup_RT_scene(ui_ctx, ui_ctx.scenes[i], i, settings, r_ctx, rt_ctx);
          const ICAERenderer *cae_renderer = dynamic_cast<const ICAERenderer *>(rt_ctx.pRayTracers[i].get());
          if (cae_renderer && cae_renderer->hasCAEData())
          {
            auto pch_info = cae_renderer->getCAEPointChannelInfo();
            auto vch_info = cae_renderer->getCAEVoxelChannelInfo();
            fill_scene_CAE_info(ui_ctx, i, pch_info, vch_info);
          }
          else
          {
            ui_ctx.scenesCtx[i].is_CAE_dataset = false;
          }
        }

        on_screen_resolution_change_RT(r_ctx, rt_ctx);

        reset_frame_counter(ui_ctx);
        ui_ctx.raytracingSceneSetUp = true;
      }
        break;
      case Cmd::INIT_RASTERIZATION:
      {
        rast_ctx.clear_rasterizers();
        if (ui_ctx.rasterizerType == RasterizerType::SIMPLE) {
          rast_ctx.add_rasterizer(std::make_unique<SimpleRasterizer>(r_ctx));
        }
        else
        {
          rast_ctx.add_rasterizer(std::make_unique<ImageTerrainRasterizer>(r_ctx));
#ifdef USE_KTX
          rast_ctx.add_rasterizer(std::make_unique<BCImageTerrainRasterizer>(r_ctx));
#endif
          rast_ctx.add_rasterizer(std::make_unique<QuadTreeTerrainRasterizer>(r_ctx));
        }

        rast_ctx.setup(ui_ctx.activeScenePaths[0]);
        rast_ctx.refresh(settings.wireframeMode);

        reset_frame_counter(ui_ctx);
        ui_ctx.rasterizationSceneSetUp = true;
      }
        break;
      case Cmd::REFRESH_RASTERIZATION:
      {
        rast_ctx.refresh(settings.wireframeMode);
        reset_frame_counter(ui_ctx);
        ui_ctx.rasterizationSceneSetUp = true;
      }
        break;
      case Cmd::RENDER_FRAME:
      {
        int rtId = cmd.args.get_int("rtId", settings.raytracerId);
        settings.raytracerId = rtId;
        if (ui_ctx.renderSetUp && (ui_ctx.raytracingSceneSetUp || ui_ctx.rasterizationSceneSetUp))
          frame(cmd_buffer, settings, ui_ctx, r_ctx, rast_ctx, rt_ctx);
        else
          add_to_log({"RENDER_FRAME: Render is not set up", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
      }
        break;
      case Cmd::COMPARE_FRAMES:
      {
        int spp = cmd.args.get_int("spp", 1);
        if (spp > 1) {
          printf("Comparing frames with spp = %d\n", spp);
        }
        int rtId = cmd.args.get_int("rtId");
        int refRtId = cmd.args.get_int("refRtId");
        uint32_t prev_spp = ui_ctx.RTPresetBlk.get_int("spp", 1);
        ui_ctx.RTPresetBlk.set_int("spp", spp);
        update_raytracer_settings(settings, ui_ctx, r_ctx, rt_ctx);
        ui_ctx.RTPresetBlk.set_int("spp", prev_spp);
        compare_frames_raytracers(rtId, refRtId, settings, ui_ctx, r_ctx, rt_ctx);
      }
      case Cmd::SET_RENDER_DEVICE:
      {
        ui_ctx.renderType = (RenderType)cmd.args.get_enum("render_type", (uint32_t)ui_ctx.renderType);
        r_ctx.renderDevice = cmd.args.get_enum("render_device", r_ctx.renderDevice); //enum RenderDevice
        ui_ctx.rasterizerType = (RasterizerType)cmd.args.get_enum("rasterizer_type", (uint32_t)ui_ctx.rasterizerType);
        settings.render_mode = (RenderMode)cmd.args.get_enum("render_mode", (uint32_t)settings.render_mode);

        ui_ctx.activePlugin = getPlugin(ui_ctx.renderType);
        ui_ctx.activePlugin->initContext(ui_ctx);
      }
        break;
      case Cmd::CUT_VTK_DATASET:
      {
        if (!ui_ctx.renderSetUp || !ui_ctx.raytracingSceneSetUp)
          add_to_log({"CUT_VTK_DATASET: Render is not set up", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
        else if (ui_ctx.scenesCtx[settings.raytracerId].vtkDataset.unstructured_grids.empty())
          add_to_log({"CUT_VTK_DATASET: Current scene is not a VTK dataset", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
        else
          cut_vtk_dataset(settings.raytracerId, cmd.args, ui_ctx);
      }
        break;
      case Cmd::SET_AA:
      {
        bool e = cmd.args.get_bool("enable");
        ui_ctx.enableAA = e;
        add_to_log({e ? "AA is enabled" : "AA is disabled", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      }
      break;
      case Cmd::SET_DENOISER:
      {
        bool e = cmd.args.get_bool("enable");
        ui_ctx.enableDenoiser = e;
        add_to_log({e ? "Denoiser is enabled" : "Denoiser is disabled", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      }
      break;
      case Cmd::SET_DENOISER_SIGMA_R:
      {
        ui_ctx.sigmaR = cmd.args.get_double("sigma_r");
      }
      break;
      case Cmd::SET_DENOISER_SIGMA_S:
      {
        ui_ctx.sigmaS = cmd.args.get_double("sigma_s");
      }
      break;
      case Cmd::SET_DENOISER_KERNEL_SIZE:
      {
        ui_ctx.kernelSize = cmd.args.get_int("kernel_size");
      }
      break;
      case Cmd::PLAYBACK_TRAJECTORY:
      {
        std::string trajectory_path = cmd.args.get_string("path_file");
        float playback_speed = cmd.args.get_double("playback_speed", 1.0f);
        Block trajectory_path_blk;
        load_block_from_file(trajectory_path.c_str(), trajectory_path_blk);
        
        PlaybackTrajectoryCmdCtx *pt_ctx = new PlaybackTrajectoryCmdCtx();
        pt_ctx->cameras.reserve(trajectory_path_blk.size());
        pt_ctx->times.reserve(trajectory_path_blk.size());

        for (int i = 0; i < trajectory_path_blk.size(); i++)
        {
          Block *cam_blk = trajectory_path_blk.get_block(i);
          if (cam_blk && trajectory_path_blk.get_name(i) == "camera")
          {
            Camera camera;
            load_from_blk(camera, cam_blk);
            pt_ctx->cameras.push_back(camera);
            pt_ctx->times.push_back(cam_blk->get_double("time_since_start")/playback_speed);
          }
        }

        pt_ctx->time_since_start_ms = 0;
        pt_ctx->cur_index = 0;

        if (pt_ctx->cameras.size() == 0)
          add_to_log({"Failed to load trajectory", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
        else
        {
          add_to_log({"Trajectory loaded", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
          ui_ctx.active_cmd = pt_ctx;
        }
      }
      break;
      case Cmd::SET_MINIMAL_UI:
      {
        printf("Minimal UI is %s\n", cmd.args.get_bool("enable") ? "enabled" : "disabled");
        ui_ctx.minimalUI = cmd.args.get_bool("enable");
      }
      break;
      case Cmd::SET_LIGHTS:
      {
        std::vector<Light> lights;
        for (int i=0; i<cmd.args.size(); i++)
        {
          Block *light_blk = cmd.args.get_block(i);
          if (light_blk)
          {
            Light light;
            load_from_blk(light, light_blk);
            if (light.type == LIGHT_TYPE_DIRECT)
              light.space = normalize(light.space);
            lights.push_back(light);
          }
        }
        for (int i=0; i<ui_ctx.raytracersCount; i++)
          rt_ctx.pRayTracers[i]->SetLights(lights);
      }
      break;
      case Cmd::SET_CAMERA_LIGHT:
      {
        ui_ctx.useCameraLight = cmd.args.get_bool("enable");
      }
      break;
      case Cmd::SET_LIVE_METRICS_CALCULATION:
      {
        ui_ctx.calculateLiveMetrics = cmd.args.get_bool("enable");
        if (ui_ctx.calculateLiveMetrics)
        {
          add_to_log({"Live metrics calculation is enabled", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
          ui_ctx.qualityMetricsStat.total_samples = 0;
          ui_ctx.qualityMetricsStat.sample_bins = std::vector<uint32_t>(100*METRICS_BINS_PER_UNIT + 1, 0);
        }
        else if (ui_ctx.qualityMetricsStat.total_samples > 0)
        {
          constexpr double q = 0.01f; //worst 1%
          double psnr_min = 100.0f;
          double psnr_sum = 0.0f;
          double psnr_bottom_q_sum = 0.0f;

          double counted = 0;
          const double q_lim = q * ui_ctx.qualityMetricsStat.total_samples;
          for (int i=0; i<ui_ctx.qualityMetricsStat.sample_bins.size(); i++)
          {
            if (ui_ctx.qualityMetricsStat.sample_bins[i] == 0)
              continue;

            double psnr = i/METRICS_BINS_PER_UNIT + 0.5f;
            psnr_min = std::min(psnr_min, psnr);
            psnr_sum += psnr * ui_ctx.qualityMetricsStat.sample_bins[i];
            if (counted <= q_lim)
            {
              psnr_bottom_q_sum += psnr * ui_ctx.qualityMetricsStat.sample_bins[i];
              counted += ui_ctx.qualityMetricsStat.sample_bins[i];
            }
          }

          double average_psnr = psnr_sum / ui_ctx.qualityMetricsStat.total_samples;
          double average_psnr_bottom_q = psnr_bottom_q_sum / counted;
          add_to_log({"Live metrics calculation finished", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
          char buf[1024];
          snprintf(buf, 1024, "Average PSNR: %.1f, worst %.1f, bottom %d%% %.1f", 
                   average_psnr, psnr_min, (int)round(q*100), average_psnr_bottom_q);
          add_to_log({buf, ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);

          ui_ctx.qualityMetricsStat.total_samples = 0;
          ui_ctx.qualityMetricsStat.sample_bins.clear();
        }
      }
      break;
      case Cmd::CAST_DEBUG_RAY:
      {
        float2 uv = cmd.args.get_vec2("uv");
        float4 rayPosAndNear, rayDirAndFar;
        rt_ctx.pRayTracers[settings.raytracerId]->initEyeRay(uv.x * r_ctx.width, uv.y * r_ctx.height, 0, &rayPosAndNear, &rayDirAndFar);
        auto acc = rt_ctx.pRayTracers[settings.raytracerId]->GetAccelStruct();
        if (!acc) 
          return;
          
        CRT_Hit hit = acc->RayQuery_NearestHit(rayPosAndNear, rayDirAndFar); // find node on which we click

        char buf[4096];
        snprintf(buf, 4096, "Debug ray\n x: %f, y: %f\npos: %f %f %f\n dir: %f %f %f\n dist: %f\n primId: %d, geomId: %d, instId: %d\n",
                float(LiteApp::getMouse()->lastX), float(LiteApp::getMouse()->lastY), rayPosAndNear.x, rayPosAndNear.y, rayPosAndNear.z,
                rayDirAndFar.x, rayDirAndFar.y, rayDirAndFar.z,
                hit.t, hit.primId, hit.geomId & GEOM_ID_MASK, hit.instId);
        add_to_log({buf, ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      }
      break;
      default:
        break;
    }
  }
}