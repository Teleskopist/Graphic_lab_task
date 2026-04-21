#include "app.h"
#include "LiteApp/rasterization/context.h"
#include "utils/common/arg_parser.h"
#include "plugins/plugins.h"

REGISTER_ENUM(Cmd,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"NO_OP", (int)Cmd::NO_OP},
                     {"INIT", (int)Cmd::INIT},
                     {"EXIT", (int)Cmd::EXIT},
                     {"LOAD_SCENE", (int)Cmd::LOAD_SCENE},
                     {"SAVE_SCENE", (int)Cmd::SAVE_SCENE},
                     {"CONVERT_SCENE", (int)Cmd::CONVERT_SCENE},
                     {"SET_ENGINE_SETTINGS", (int)Cmd::SET_ENGINE_SETTINGS},
                     {"SET_RENDER_SETTINGS", (int)Cmd::SET_RENDER_SETTINGS},
                     {"SET_CAMERA", (int)Cmd::SET_CAMERA},
                     {"TAKE_SCREENSHOT", (int)Cmd::TAKE_SCREENSHOT},
                     {"INIT_RAYTRACING", (int)Cmd::INIT_RAYTRACING},
                     {"INIT_RASTERIZATION", (int)Cmd::INIT_RASTERIZATION},
                     {"REFRESH_RASTERIZATION", (int)Cmd::REFRESH_RASTERIZATION},
                     {"RENDER_FRAME", (int)Cmd::RENDER_FRAME},
                     {"COMPARE_FRAMES", (int)Cmd::COMPARE_FRAMES},
                     {"SET_RENDER_DEVICE", (int)Cmd::SET_RENDER_DEVICE},
                     {"CUT_VTK_DATASET", (int)Cmd::CUT_VTK_DATASET},
                     {"SET_AA", (int)Cmd::SET_AA},
                     {"RECORD_TRAJECTORY", (int)Cmd::RECORD_TRAJECTORY},
                     {"PLAYBACK_TRAJECTORY", (int)Cmd::PLAYBACK_TRAJECTORY},
                     {"SET_MINIMAL_UI", (int)Cmd::SET_MINIMAL_UI},
                     {"PRELOAD_VTK_DATASET", (int)Cmd::PRELOAD_VTK_DATASET},
                     {"SET_DENOISER", (int)Cmd::SET_DENOISER},
                     {"SET_DENOISER_SIGMA_R", (int)Cmd::SET_DENOISER_SIGMA_R},
                     {"SET_DENOISER_SIGMA_S", (int)Cmd::SET_DENOISER_SIGMA_S},
                     {"SET_LIGHTS", (int)Cmd::SET_LIGHTS},
                     {"SET_CAMERA_LIGHT", (int)Cmd::SET_CAMERA_LIGHT},
                     {"SET_DENOISER_KERNEL_SIZE", (int)Cmd::SET_DENOISER_KERNEL_SIZE},
                     {"SET_LIVE_METRICS_CALCULATION", (int)Cmd::SET_LIVE_METRICS_CALCULATION},
                     {"CHANGE_VISIBILITY", (int)Cmd::CHANGE_VISIBILITY},
                     {"CAST_DEBUG_RAY", (int)Cmd::CAST_DEBUG_RAY},
                 }; })());



REGISTER_ENUM(RenderMode,
              ([]()
               { using namespace demo_app;
                return std::vector<std::pair<std::string, unsigned>>{
                    {"RASTERIZATION", (int)RenderMode::RASTERIZATION},
                    {"RAYTRACING", (int)RenderMode::RAYTRACING}
                 }; })());

REGISTER_ENUM(RasterizerType,
              ([]()
               { using namespace demo_app;
                return std::vector<std::pair<std::string, unsigned>>{
                    {"SIMPLE", (int)RasterizerType::SIMPLE},
                    {"TERRAIN", (int)RasterizerType::TERRAIN}
                 }; })());

REGISTER_ENUM(ConversionType,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"MESH_TO_SDF", (int)demo_app::ConversionType::MESH_TO_SDF},
                     {"CAD_PARTS_TO_MULTI_OCTREE", (int)demo_app::ConversionType::CAD_PARTS_TO_MULTI_OCTREE},
                     {"VTK_DATASET_TO_OCTREE", (int)demo_app::ConversionType::VTK_DATASET_TO_OCTREE},
                     {"VOLUME_TO_OCTREE", (int)demo_app::ConversionType::VOLUME_TO_OCTREE},
                     {"CAD_FOLDER_TO_MULTI_OCTREE", (int)demo_app::ConversionType::CAD_FOLDER_TO_MULTI_OCTREE},
                 }; })());

REGISTER_ENUM(SDFType,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"REGULAR_GRID", (int)demo_app::SDFType::REGULAR_GRID},
                     {"FRAME_OCTREE", (int)demo_app::SDFType::FRAME_OCTREE},
                     {"SCOM_TREE", (int)demo_app::SDFType::SCOM_TREE},
                     {"MULTI_OCTREE", (int)demo_app::SDFType::MULTI_OCTREE},
                     {"SCOM_TREE_2", (int)demo_app::SDFType::SCOM_TREE_2},
                     {"SDF_DAG", (int)demo_app::SDFType::SDF_DAG},
                     {"FLOOD_OCTREE_TYPES", (int)demo_app::SDFType::FLOOD_OCTREE_TYPES},
                     {"FLOOD_OCTREE_CODES", (int)demo_app::SDFType::FLOOD_OCTREE_CODES},
                 }; })());

REGISTER_ENUM(CADSDFType,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"MULTI_OCTREE", (int)demo_app::CADSDFType::MULTI_OCTREE},
                     {"SCOM_TREE_2", (int)demo_app::CADSDFType::SCOM_TREE_2},
                 }; })());

REGISTER_ENUM(ASDFType,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"MULTI_OCTREE", (int)demo_app::ASDFType::MULTI_OCTREE},
                     {"SCOM_TREE_2", (int)demo_app::ASDFType::SCOM_TREE_2},
                     {"SDF_DAG", (int)demo_app::ASDFType::SDF_DAG},
                 }; })());

REGISTER_ENUM(VolumeGridType,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"VOLUME_GRID_3D_DENSITY", (int)demo_app::VolumeGridType::VOLUME_GRID_3D_DENSITY},
                     {"VOLUME_GRID_4D_DENSITY", (int)demo_app::VolumeGridType::VOLUME_GRID_4D_DENSITY},
                     {"VOLUME_GRID_3D_RGB_DENSITY", (int)demo_app::VolumeGridType::VOLUME_GRID_3D_RGB_DENSITY},
                     {"VOLUME_GRID_4D_RGB_DENSITY", (int)demo_app::VolumeGridType::VOLUME_GRID_4D_RGB_DENSITY},
                 }; })());

REGISTER_ENUM(CTFPreset,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"SHADES_OF_GRAY", (int)demo_app::CTFPreset::SHADES_OF_GRAY},
                     {"COOL_TO_WARM", (int)demo_app::CTFPreset::COOL_TO_WARM},
                 }; })());

REGISTER_ENUM(VisibilityChangeType,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"HIDE_CURRENT", (int)demo_app::VisibilityChangeType::HIDE_CURRENT},
                     {"HIDE_OTHERS", (int)demo_app::VisibilityChangeType::HIDE_OTHERS},
                     {"SHOW_ALL", (int)demo_app::VisibilityChangeType::SHOW_ALL},
                 };  })());

int main(int argc, const char ** argv)
{
  Block init_args;
  Block render_device_args;
  Block engine_settings_args;
  Block load_scene_args;

  std::string command_list_path = "";
  if (argc == 2 && std::string(argv[1]) != "-help" && std::string(argv[1]) != "--help")
  {
    command_list_path = argv[1];
  }
  else
  {
    ConsoleArgParser p(argc, argv);

    if (p.hasFlag("-validation") || p.hasFlag("--validation"))
      engine_settings_args.add_bool("validation", true);
    
    if (p.hasFlag("-CPU") || p.hasFlag("--CPU"))
      render_device_args.add_enum("render_device", "RenderDevice", DEVICE_CPU);
    else if (p.hasFlag("-GPU") || p.hasFlag("--GPU"))
      render_device_args.add_enum("render_device", "RenderDevice", DEVICE_GPU_COMP);

    std::string render_name;
    if (p.hasParam("-renderer", &render_name) || p.hasParam("--renderer", &render_name))
    {
      auto enum_info = *get_enum_info("RenderType");
      for (int i=0; i<enum_info.size(); i++)
      {
        if (enum_info[i].first == render_name)
        {
          render_device_args.add_enum("render_type", "RenderType", enum_info[i].second);
          break;
        }
      }
    }

    std::string scene_path = "";
    if (p.hasParam("-scene", &scene_path))
    {
      std::string ref_scene_path_1 = "";
      std::string ref_scene_path_2 = "";
      if (p.hasParam("-ref_scene_1", &ref_scene_path_1) && p.hasParam("-ref_scene_2", &ref_scene_path_2))
        load_scene_args.set_arr("paths", std::vector<std::string>{scene_path, ref_scene_path_1, ref_scene_path_2});
      else if (p.hasParam("-ref_scene", &ref_scene_path_1))
        load_scene_args.set_arr("paths", std::vector<std::string>{scene_path, ref_scene_path_1});
      else
        load_scene_args.set_arr("paths", std::vector<std::string>{scene_path});
    }

    if (p.hasFlag("-no_rescale"))
      load_scene_args.add_bool("rescale", false);

    int w,h,deviceId;
    if (p.hasIntParam("-w", &w) || p.hasIntParam("--width", &w))
      init_args.add_int("window_width", w);
    if (p.hasIntParam("-h", &h) || p.hasIntParam("--height", &h))
      init_args.add_int("window_height", h);
    if (p.hasFlag("-fullscreen") || p.hasFlag("--fullscreen"))
      init_args.add_bool("fullscreen", true);
    if (p.hasIntParam("-deviceId", &deviceId) || p.hasIntParam("--deviceId", &deviceId))
      init_args.add_int("deviceId", deviceId);

    if (p.hasFlag("-help") || p.hasFlag("--help"))
    {
      p.print_help();
      return 0;
    }
  }

  Block settings_blk_list;
  CommandBuffer cmd_buffer;

  demo_app::Settings settings{};
  demo_app::UIContext ui_ctx{};
  demo_app::RaytracingContext rt_ctx{};
  LiteApp::RenderingContext r_ctx{};
  LiteApp::RasterizationContext rast_ctx{};
  
  if (!command_list_path.empty())
    load_block_from_file(command_list_path, settings_blk_list);

  //load commands from file
  if (settings_blk_list.size() > 0)
  {
    printf("Loading commands from file\n");
    for (int i = 0; i < settings_blk_list.size(); i++)
    {
      Block *blk = settings_blk_list.get_block(i);
      if (!blk)
        printf("Command list must contain only blocks\n");
      else
      {
        Cmd cmd_type = (Cmd)blk->get_enum("cmd_code", (int)Cmd::NO_OP);
        cmd_buffer.push(cmd_type, *blk);
      }
    }
  }
  else   //push init command and load scene if provided
  {
    cmd_buffer.push(Cmd::INIT, init_args);
    if (render_device_args.size() > 0)
      cmd_buffer.push(Cmd::SET_RENDER_DEVICE, render_device_args);
    if (engine_settings_args.size() > 0)
      cmd_buffer.push(Cmd::SET_ENGINE_SETTINGS, engine_settings_args);
    if (load_scene_args.size() > 0)
      cmd_buffer.push(Cmd::LOAD_SCENE, load_scene_args);
  }

  // initiali plugin with default renderer
  ui_ctx.activePlugin = demo_app::getPlugin(ui_ctx.renderType);
  ui_ctx.activePlugin->initContext(ui_ctx);

  bool exit_app = false;
  while (!exit_app)
  {
    //execute all commands from the command buffer
    while (!cmd_buffer.empty() && ui_ctx.active_cmd == nullptr)
    {
      if (cmd_buffer.front().type != Cmd::EXIT)
        cmd_buffer.log();
      demo_app::execute_command(cmd_buffer.front(), cmd_buffer, settings, ui_ctx, r_ctx, rast_ctx, rt_ctx);
      if (ui_ctx.activePlugin)
        ui_ctx.activePlugin->executeCommand(cmd_buffer.front(), cmd_buffer, settings, ui_ctx, r_ctx, rast_ctx, rt_ctx);
      if (cmd_buffer.front().type == Cmd::EXIT)
      {
        exit_app = true;
        cmd_buffer.pop();
        break;
      }
      else
        cmd_buffer.pop();
    }

    //no rendering, no loops, just exit
    if (!ui_ctx.renderSetUp)
      exit_app = true;

    if (!exit_app)
      demo_app::frame(cmd_buffer, settings, ui_ctx, r_ctx, rast_ctx, rt_ctx);
  }

  return 0;
}
