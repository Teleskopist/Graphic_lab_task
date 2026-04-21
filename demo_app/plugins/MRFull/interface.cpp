#include "core/serialization.h"
#include "plugin.h"
#include "utils/misc/density_grid.h"
#include "sdf/scom/similarity_compression.h"
#include "sdf/scom2/scom_builder.h"
#include "sdf/build/scene_converter.h"
#include "sdf/multi/sdf_multi_debug.h"
#include "VolumeRenderer/sparse_octree_density_grid.h"
#include "VolumeRenderer/scene_extension_volume.h"
#include "demo_app/app_internal.h"
#include "LiteApp/lite_app_imgui.h"
#include "imgui/imgui_internal.h"

namespace demo_app
{

  void mesh_scom_conversion_interface(int conv_type, CommandBuffer &cmd_buffer, const Settings &settings, UIContext &ui_ctx)
  {
    ImGui::Text("Input mesh (.obj, .ply, .vsgf)");
    ImGui::InputTextEx("##mesh_in2", nullptr, ui_ctx.convertMeshInputPath, sizeof(ui_ctx.convertMeshInputPath),
                       ImVec2(0.9f * ImGui::GetWindowWidth(), 0), 0);

    ImGui::Checkbox("Load reference parts as a separate scene", &ui_ctx.conversionLoadReferenceScene);
    ImGui::Checkbox("Load combined scene (mesh + SDF as a LOD)", &ui_ctx.conversionLoadCombinedScene);
    ImGui::ListBox("SDF representation", &ui_ctx.conversionSDFTypeIdx, conversion_sdf_types.data(), conversion_sdf_types.size());

    switch (ui_ctx.conversionSDFTypeIdx)
    {
    case (int)SDFType::REGULAR_GRID:
    {
      int size = ui_ctx.gridSettingsBlk.get_int("size", 32);
      ImGui::InputInt("Grid size", &size, 0, 0);
      ImGui::SameLine();
      bool down = ImGui::Button("-");
      ImGui::SameLine();
      bool up = ImGui::Button("+");
      size = up ? 2 * size : size;
      size = down ? size / 2 : size;
      ui_ctx.gridSettingsBlk.set_int("size", size);
    }
    break;
    case (int)SDFType::SCOM_TREE:
      LiteApp::blk_modification_interface(&ui_ctx.sparseOctreeSettingsBlk, "Octree settings");
      LiteApp::blk_modification_interface(&ui_ctx.COctreeSettingsBlk, "COctree settings");
      LiteApp::blk_modification_interface(&ui_ctx.SComSettingsBlk, "SCom settings");
      break;
    case (int)SDFType::MULTI_OCTREE:
      ImGui::Checkbox("Save SDF build debug info", &ui_ctx.conversionSaveSDFDebugInfo);
      LiteApp::blk_modification_interface(&ui_ctx.sparseOctreeSettingsBlk, "Octree settings");
      break;
    case (int)SDFType::SDF_DAG:
    case (int)SDFType::SCOM_TREE_2:
      LiteApp::blk_modification_interface(&ui_ctx.sparseOctreeSettingsBlk, "Octree settings");
      LiteApp::blk_modification_interface(&ui_ctx.SCom2SettingsBlk, "SCom2 settings");
      break;
    default:
      LiteApp::blk_modification_interface(&ui_ctx.sparseOctreeSettingsBlk, "Octree settings");
      break;
    }

    bool convert = ImGui::Button("Convert");
    if (convert)
    {
      Block blk;
      blk.set_string("input_file", ui_ctx.convertMeshInputPath);
      blk.set_bool("load_reference_scene", ui_ctx.conversionLoadReferenceScene);
      blk.set_bool("load_combined_scene", ui_ctx.conversionLoadCombinedScene);
      blk.set_bool("save_sdf_debug_info", ui_ctx.conversionSaveSDFDebugInfo);
      if (ui_ctx.conversionSDFTypeIdx == (int)SDFType::REGULAR_GRID)
      {
        blk.set_block("grid_settings", &ui_ctx.gridSettingsBlk);
      }
      else
      {
        blk.set_block("sparse_octree_settings", &ui_ctx.sparseOctreeSettingsBlk);
      }
      if (ui_ctx.conversionSDFTypeIdx == (int)SDFType::SCOM_TREE)
      {
        blk.set_block("coctree_settings", &ui_ctx.COctreeSettingsBlk);
        blk.set_block("scom_settings", &ui_ctx.SComSettingsBlk);
      }
      if (ui_ctx.conversionSDFTypeIdx == (int)SDFType::SDF_DAG ||
          ui_ctx.conversionSDFTypeIdx == (int)SDFType::SCOM_TREE_2)
      {
        blk.set_block("scom2_settings", &ui_ctx.SCom2SettingsBlk);
      }
      blk.set_enum("conversion_mode", "ConversionType", (int)conv_type);
      blk.set_enum("sdf_type", "SDFType", ui_ctx.conversionSDFTypeIdx);
      cmd_buffer.push(Cmd::CONVERT_SCENE, blk);
    }
  }

  void CAD_parts_conversion_interface(int conv_type, CommandBuffer &cmd_buffer, const Settings &settings, UIContext &ui_ctx)
  {
    if (conv_type == (int)ConversionType::CAD_PARTS_TO_MULTI_OCTREE)
      ImGui::Text("Input mesh (.obj)");
    else if (conv_type == (int)ConversionType::CAD_FOLDER_TO_MULTI_OCTREE)
      ImGui::Text("Input folder");
    ImGui::InputTextEx("##mesh_in3", nullptr, ui_ctx.convertMeshInputPath, sizeof(ui_ctx.convertMeshInputPath),
                       ImVec2(0.9f * ImGui::GetWindowWidth(), 0), 0);

    ImGui::ListBox("CAD to SDF type", &ui_ctx.conversionCADSDFTypeIdx, conversion_cad_to_sdf_types.data(), conversion_cad_to_sdf_types.size());

    ImGui::Checkbox("Load reference parts as a separate scene", &ui_ctx.conversionLoadReferenceScene);
    ImGui::Checkbox("Load combined scene (mesh + SDF as a LOD)", &ui_ctx.conversionLoadCombinedScene);
    ImGui::Checkbox("Load reference CAD parts as separate models", &ui_ctx.conversionCADLoadSeparateParts);

    LiteApp::blk_modification_interface(&ui_ctx.sceneConvertSettingsBlk, "Settings");
    switch (ui_ctx.conversionCADSDFTypeIdx)
    {
    case (int)CADSDFType::MULTI_OCTREE:
      break;
    case (int)CADSDFType::SCOM_TREE_2:
      LiteApp::blk_modification_interface(&ui_ctx.SCom2SettingsBlk, "SCom2 settings");
      break;
    default:
      break;
    }

    bool convert = ImGui::Button("Convert");
    if (convert)
    {
      Block blk;
      blk.set_string("input_file", ui_ctx.convertMeshInputPath);
      blk.set_bool("load_reference_scene", ui_ctx.conversionLoadReferenceScene);
      blk.set_bool("load_combined_scene", ui_ctx.conversionLoadCombinedScene);
      blk.set_block("settings", &ui_ctx.sceneConvertSettingsBlk);
      blk.set_enum("conversion_mode", "ConversionType", (int)conv_type);
      blk.get_block("settings")->set_enum("CAD_to_SDF_type", "CADSDFType", ui_ctx.conversionCADSDFTypeIdx);
      if (ui_ctx.conversionCADSDFTypeIdx == (int)CADSDFType::SCOM_TREE_2)
      {
        blk.get_block("settings")->set_block("scom2_settings", &ui_ctx.SCom2SettingsBlk);
      }
      cmd_buffer.push(Cmd::CONVERT_SCENE, blk);
    }
  }

  void VTK_conversion_interface(int conv_type, CommandBuffer &cmd_buffer, const Settings &settings, UIContext &ui_ctx)
  {
    ImGui::Text("Input file (.vtu, .vtp, .vtm)");
    ImGui::InputTextEx("##vtk_in3", nullptr, ui_ctx.convertMeshInputPath, sizeof(ui_ctx.convertMeshInputPath),
                       ImVec2(0.9f * ImGui::GetWindowWidth(), 0), 0);

    uint32_t ref_scene_id = 1;
    bool dataset_preloaded = ui_ctx.scenesCtx[ref_scene_id].vtkDataset.unstructured_grids.size() > 0;
    if (dataset_preloaded)
    {
      ImGui::Text("Choose channels to use from dataset");
      if (ImGui::CollapsingHeader("Point channels"))
      {
        uint32_t cnt = ui_ctx.preloaded_cae_info.point_channels.size();
        for (uint32_t i = 0; i < cnt; i++)
        {
          ImGui::Checkbox(ui_ctx.preloaded_cae_info.point_channels[i].name.c_str(), &ui_ctx.preloaded_cae_info.point_channels[i].is_chosen);
        }
      }
      if (ImGui::CollapsingHeader("Cell channels"))
      {
        uint32_t cnt = ui_ctx.preloaded_cae_info.cell_channels.size();
        for (uint32_t i = 0; i < cnt; i++)
        {
          ImGui::Checkbox(ui_ctx.preloaded_cae_info.cell_channels[i].name.c_str(), &ui_ctx.preloaded_cae_info.cell_channels[i].is_chosen);
        }
      }
      ImGui::NewLine();
    }
    else
    {
      bool preload = ImGui::Button("Preload dataset");
      if (preload)
      {
        Block blk;
        blk.set_string("input_file", ui_ctx.convertMeshInputPath);
        blk.set_int("rtId", ref_scene_id);
        cmd_buffer.push(Cmd::PRELOAD_VTK_DATASET, blk);
      }
    }

    ImGui::Checkbox("Load triangulated dataset as a separate scene", &ui_ctx.conversionLoadReferenceScene);
    ImGui::Checkbox("Convert volume", &ui_ctx.VTKConversionVolume);
    ImGui::InputInt("Octree depth", &ui_ctx.VTKConversionOctreeDepth);
    if (ui_ctx.VTKConversionVolume)
      ImGui::InputInt("Internal depth", &ui_ctx.VTKConversionInternalDepth);
    ImGui::InputFloat("Surface prune threshold", &ui_ctx.VTKConversionPruneThr, 1e-6f, 1e-5f, "%.6f");
    ImGui::InputFloat("Channel prune threshold", &ui_ctx.VTKConversionChannelPruneThr, 0.01f, 0.1f);
    ImGui::Checkbox("Compress channels (float -> FP8)", &ui_ctx.conversionCompressVTKChannels);
    ImGui::ListBox("ASDF representation", &ui_ctx.conversionASDFTypeIdx, conversion_asdf_types.data(), conversion_asdf_types.size());

    switch (ui_ctx.conversionASDFTypeIdx)
    {
    case (int)ASDFType::SDF_DAG:
    case (int)ASDFType::SCOM_TREE_2:
      LiteApp::blk_modification_interface(&ui_ctx.SCom2SettingsBlk, "SCom2 settings");
      break;
    default:
      break;
    }

    bool convert = ImGui::Button("Convert");
    if (convert)
    {
      Block sdf_settings_blk;
      SparseOctreeSettings sdf_settings;
      sdf_settings.depth = ui_ctx.VTKConversionOctreeDepth;
      sdf_settings.internal_depth = ui_ctx.VTKConversionInternalDepth;
      sdf_settings.fill_internal_volume = ui_ctx.VTKConversionVolume;
      sdf_settings.channel_split_thr = ui_ctx.VTKConversionChannelPruneThr;
      sdf_settings.split_thr = ui_ctx.VTKConversionPruneThr;
      sdf_settings.use_pruning = ui_ctx.VTKConversionChannelPruneThr > 0;
      sdf_settings.fill_all_nodes = true;
      save_to_blk(sdf_settings, &sdf_settings_blk);

      Block vtk_load_options;
      if (dataset_preloaded)
      {
        std::vector<std::string> point_arrays_whitelist;
        std::vector<std::string> cell_arrays_whitelist;

        for (auto &ch : ui_ctx.preloaded_cae_info.point_channels)
        {
          if (ch.is_chosen)
            point_arrays_whitelist.push_back(ch.name);
        }

        for (auto &ch : ui_ctx.preloaded_cae_info.cell_channels)
        {
          if (ch.is_chosen)
            cell_arrays_whitelist.push_back(ch.name);
        }
        vtk_load_options.set_arr("point_arrays_whitelist", point_arrays_whitelist);
        vtk_load_options.set_arr("cell_arrays_whitelist", cell_arrays_whitelist);
      }

      Block blk;
      blk.set_string("input_file", ui_ctx.convertMeshInputPath);
      blk.set_bool("load_reference_scene", ui_ctx.conversionLoadReferenceScene);
      blk.set_enum("conversion_mode", "ConversionType", (int)conv_type);
      blk.set_enum("ASDF_representation", "ASDFType", ui_ctx.conversionASDFTypeIdx);
      blk.set_bool("compress_channels", ui_ctx.conversionCompressVTKChannels);
      blk.set_block("vtk_load_options", &vtk_load_options);
      blk.set_block("octree_settings", &sdf_settings_blk);
      switch (ui_ctx.conversionASDFTypeIdx)
      {
      case (int)ASDFType::SDF_DAG:
      case (int)ASDFType::SCOM_TREE_2:
        blk.set_block("scom2_settings", &ui_ctx.SCom2SettingsBlk);
        break;
      default:
        break;
      }

      cmd_buffer.push(Cmd::CONVERT_SCENE, blk);
    }
  }

  void volume_conversion_interface(int conv_type, CommandBuffer &cmd_buffer, const Settings &settings, UIContext &ui_ctx)
  {
    ImGui::Text("Input volume path (.bin)");
    static char path[1024] = "";
    ImGui::InputTextEx("##volume_in1", nullptr, path, sizeof(path),
                       ImVec2(0.9f * ImGui::GetWindowWidth(), 0), 0);
    ImGui::Checkbox("load reference scene", &ui_ctx.conversionLoadReferenceScene);
    ImGui::ListBox("Input volume type", &ui_ctx.conversionVolumeType, vodume_grid_types.data(), vodume_grid_types.size());
    LiteApp::blk_modification_interface(&ui_ctx.sparseOctreeSettingsBlk, "Octree settings");
    LiteApp::blk_modification_interface(&ui_ctx.volumeSCom2SettingsBlk, "SCom2 settings");

    bool convert = ImGui::Button("Convert");
    if (convert)
    {
      Block blk;
      blk.set_string("input_file", path);
      blk.set_bool("load_reference_scene", ui_ctx.conversionLoadReferenceScene);
      blk.set_enum("conversion_mode", "ConversionType", (int)conv_type);
      blk.set_enum("volume_type", "VolumeType", (int)ui_ctx.conversionVolumeType);
      blk.set_block("sparse_octree_settings", &ui_ctx.sparseOctreeSettingsBlk);
      blk.set_block("scom2_settings", &ui_ctx.volumeSCom2SettingsBlk);

      cmd_buffer.push(Cmd::CONVERT_SCENE, blk);
    }
  }

  void MRFullPlugin::drawInterface(CommandBuffer &cmd_buffer, Settings &settings, UIContext &ui_ctx)
  {
    ImGui::ListBox("Conversion type", &ui_ctx.conversionTypeIdx, conversion_types.data(), conversion_types.size());

    switch (ui_ctx.conversionTypeIdx)
    {
    case (int)ConversionType::MESH_TO_SDF:
      mesh_scom_conversion_interface(ui_ctx.conversionTypeIdx, cmd_buffer, settings, ui_ctx);
      break;
    case (int)ConversionType::CAD_PARTS_TO_MULTI_OCTREE:
      CAD_parts_conversion_interface(ui_ctx.conversionTypeIdx, cmd_buffer, settings, ui_ctx);
      break;
    case (int)ConversionType::VTK_DATASET_TO_OCTREE:
      VTK_conversion_interface(ui_ctx.conversionTypeIdx, cmd_buffer, settings, ui_ctx);
      break;
    case (int)ConversionType::VOLUME_TO_OCTREE:
      volume_conversion_interface(ui_ctx.conversionTypeIdx, cmd_buffer, settings, ui_ctx);
      break;
    case (int)ConversionType::CAD_FOLDER_TO_MULTI_OCTREE:
      CAD_parts_conversion_interface(ui_ctx.conversionTypeIdx, cmd_buffer, settings, ui_ctx);
      break;
    default:
      break;
    }
  }
}