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
#include "utils/vtk/parser.h"
#include "utils/vtk/procedural.h"
#include "sdf/build/sparse_octree_builder.h"
#include "sdf/build/sparse_octree_common.h"
#include "sdf/build/sparse_octree_augmentation.h"
#include "sdf/build/sparse_octree_unstructured_grid.h"
#include "utils/mesh/mesh_internal.h"
#include "utils/mesh/stl_reader.h"
#include "utils/common/csv.h"
#include "utils/common/strings.h"
#include "utils/common/position_hash.h"
#include "utils/misc/density_grid.h"
#include "sdf/scom/similarity_compression.h"
#include "sdf/scom2/scom_builder.h"
#include "VolumeRenderer/sparse_octree_density_grid.h"
#include "VolumeRenderer/scene_extension_volume.h"

namespace demo_app
{
  scom2::Settings get_default_volume_scom_settings()
  {
    scom2::Settings base_scom2_settings = scom2::Settings();
    base_scom2_settings.internal_brick_use = scom2::InternalBrickUse::PROPER_CLUSTERING;
    base_scom2_settings.branch_descriptor = scom2::BranchDescriptor::SIMPLE_MERGE;
    base_scom2_settings.bricks_use_similarity_compression = false;
    base_scom2_settings.bricks_similarity_threshold = 0.05f;
    base_scom2_settings.branches_use_similarity_compression = false;
    base_scom2_settings.branches_similarity_compression_levels = 2;
    base_scom2_settings.branches_similarity_threshold = 0.05f;
    base_scom2_settings.packed_reference_type = scom2::PackedReferenceType::LONG_9_23_32;
    base_scom2_settings.bits_per_value = 16;
    base_scom2_settings.support_multi_nodes = false;
    base_scom2_settings.custom_max_val = true;
    base_scom2_settings.use_close_match_heuristic = true;

    return base_scom2_settings;
  }

  void load_default_blocks(UIContext &ui_ctx)
  {
    save_to_blk(SparseOctreeSettings(), &ui_ctx.sparseOctreeSettingsBlk);
    save_to_blk(COctreeV3Settings(), &ui_ctx.COctreeSettingsBlk);
    save_to_blk(scom::Settings(), &ui_ctx.SComSettingsBlk);
    save_to_blk(scom2::Settings(), &ui_ctx.SCom2SettingsBlk);
    save_to_blk(get_default_volume_scom_settings(), &ui_ctx.volumeSCom2SettingsBlk);

    auto sc_settings = scene_converter::Settings();
    sc_settings.octree_settings.depth = 9;
    save_to_blk(sc_settings, &ui_ctx.sceneConvertSettingsBlk);
  }

  std::string to_binary(unsigned val)
  {
    char values[64] = {0};
    int pos = 0;
    for (int i = 31; i >= 0; i--)
    {
      values[pos++] = val & (1 << i) ? '1' : '0';
      if (i % 8 == 0 && i != 0)
        values[pos++] = '\'';
    }
    values[pos] = '\0';
    return std::string(values);
  }

  void print_primitive_info(CRT_Hit &hit, demo_app::UIContext &ui_ctx, BVHRT *BVH_RT)
  {
    uint32_t nodeId = hit.primId;

    add_to_log({"===", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx); // logging
    std::stringstream log;
    log << nodeId;
    add_to_log({log.str().c_str(), ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
    unsigned F = BVH_RT->m_SdfMultiOctreeNodes[nodeId].flags;
    add_to_log({"flags: " + to_binary(F), ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
    char buf[4096];
    snprintf(buf, 4096, "       %8d|%8d|%8d|%8d", (F >> 24) & 0xFF, (F >> 16) & 0xFF, (F >> 8) & 0xFF, F & 0xFF);
    add_to_log({buf, ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
    for (int i = 0; i < BVH_RT->m_SdfMultiOctreeNodes[nodeId].voxel_count; ++i)
    {
      if (i != 0)
        add_to_log({"---", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      for (int j = 0; j < 2; ++j)
      {
        log.clear();
        for (int u = 0; u < 4; ++u)
        {
          log << BVH_RT->m_SdfMultiOctreeValues[BVH_RT->m_SdfMultiOctreeNodes[nodeId].data_offset + 8 * i + j * 4 + u];
          log << " ";
        }
        add_to_log({log.str().c_str(), ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      }
    }
    add_to_log({"===", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
  }

  void print_sdf_debug_info(const sdf_converter::GlobalOctreeDebugInfo &info, unsigned prim_id, bool secondary, UIContext &ui_ctx)
  {
    if (!secondary && prim_id >= info.nodes_info.size())
    {
      std::string message = "No debug info for primitive " + std::to_string(prim_id);
      add_to_log({message, ui_ctx.logShowTime, LogEntry::Type::WARNING}, ui_ctx);
    }
    else if (secondary && prim_id >= info.merged_nodes_info.size())
    {
      std::string message = "No debug info for merged primitive " + std::to_string(prim_id);
      add_to_log({message, ui_ctx.logShowTime, LogEntry::Type::WARNING}, ui_ctx);
    }
    else
    {
      static std::vector<const char *> creation_type_names = {"Unknown", "Empty", "Fill Node", "Fill Node Multi",
                                                              "Refill Node", "Fill LOD", "From Analytic SDF", "Merge",
                                                              "Merge with Voxel Merge"};
      auto &node_info = secondary ? info.merged_nodes_info[prim_id] : info.nodes_info[prim_id];
      bool is_merge = node_info.merged_nodes_offset >= 0;
      char buf[4096];
      snprintf(buf, 4096, "===================\n");
      add_to_log({buf, ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      snprintf(buf, 4096, "Node id: %d\n", node_info.original_node_id);
      add_to_log({buf, ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      snprintf(buf, 4096, "Position code: (%d %d %d %d)\n", node_info.position_code.x, node_info.position_code.y, node_info.position_code.z,
               node_info.position_code.w);
      add_to_log({buf, ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      snprintf(buf, 4096, "%s node\n", node_info.is_surface_node ? "Surface" : "Volume");
      add_to_log({buf, ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      if (is_merge)
        snprintf(buf, 4096, "Created: %s (from %d nodes)\n", creation_type_names[node_info.creation_type], node_info.merged_nodes_count);
      else
        snprintf(buf, 4096, "Created: %s\n", creation_type_names[node_info.creation_type]);
      add_to_log({buf, ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      snprintf(buf, 4096, "Tris/Components/Surfaces: %d/%d/%d\n", node_info.primitives_count, node_info.connected_components_count, node_info.surfaces_count);
      add_to_log({buf, ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      unsigned F = node_info.prim_octree_node_code;
      add_to_log({"code:  " + to_binary(F), ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      snprintf(buf, 4096, "       %8d|%8d|%8d|%8d", (F >> 24) & 0xFF, (F >> 16) & 0xFF, (F >> 8) & 0xFF, F & 0xFF);
      add_to_log({buf, ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      add_to_log({"signs: " + to_binary(node_info.initial_signs), ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      snprintf(buf, 4096, "Error with parent: %.8f\n", node_info.error_from_parent);
      add_to_log({buf, ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      snprintf(buf, 4096, "Add depth error: %.8f\n", node_info.add_depth_error);
      add_to_log({buf, ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);

      if (is_merge)
      {
        snprintf(buf, 4096, "Merged from %d nodes:\n", node_info.merged_nodes_count);
        add_to_log({buf, ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
        for (int i = 0; i < node_info.merged_nodes_count; ++i)
          print_sdf_debug_info(info, node_info.merged_nodes_offset + i, true, ui_ctx);
      }

      snprintf(buf, 4096, "===================\n");
      add_to_log({buf, ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
    }
  }

  bool try_print_extra_debug(float x, float y, Settings &settings, UIContext &ui_ctx, const RenderingContext &r_ctx,
                             const RaytracingContext &rt_ctx)
  {
    auto acc = rt_ctx.pRayTracers[settings.raytracerId]->GetAccelStruct();
    if (!acc)
      return false;
    auto BVH_RT = dynamic_cast<BVHRT *>(acc->UnderlyingImpl(0));
    auto mr = dynamic_cast<MultiRenderer *>(rt_ctx.pRayTracers[settings.raytracerId].get());

    if (!BVH_RT && !mr)
      return false;

    float4 rayPosAndNear, rayDirAndFar;
    rt_ctx.pRayTracers[settings.raytracerId]->initEyeRay(x * r_ctx.width, y * r_ctx.height, 0, &rayPosAndNear, &rayDirAndFar);
    if (ui_ctx.queryDebugRay)
      BVH_RT->set_debug_mode(true); // enable debug mode, renderer will print logs in console
    MultiRenderPreset prev_preset = BVH_RT->GetPreset();
    MultiRenderPreset tmp_preset = prev_preset;
    tmp_preset.render_mode = MULTI_RENDER_MODE_MASK; // some render modes change meaning of CRT_Hit fields,
                                                     // so some simple render mode should be used for debug ray
    BVH_RT->SetPreset(tmp_preset);
    CRT_Hit hit;

    hit = BVH_RT->RayQuery_NearestHit(rayPosAndNear, rayDirAndFar); // find node on which we click

    BVH_RT->SetPreset(prev_preset);
    BVH_RT->set_debug_mode(false);

    if (hit.geomId == 0xFFFFFFFFu) // empty space, reset ids in highlight_preset
    {
      settings.highlight_preset.prim_id = 0xFFFFFFFFu;
      settings.highlight_preset.geom_id = 0xFFFFFFFFu;
      settings.highlight_preset.inst_id = 0xFFFFFFFFu;
    }
    else
    {
      settings.highlight_preset.prim_id = hit.primId;
      settings.highlight_preset.geom_id = hit.geomId;
      settings.highlight_preset.inst_id = hit.instId;

      // if we clicked on multi octree, some debug info can be printed
      if (hit.geomId >> SH_TYPE == TYPE_SDF_MULTI_OCTREE && hit.primId < BVH_RT->m_SdfMultiOctreeNodes.size())
      {
        if (ui_ctx.queryDebugPrimitiveInfo)
          print_primitive_info(hit, ui_ctx, BVH_RT);
        if (ui_ctx.queryDebugBuildInfo && ui_ctx.scenesCtx[settings.raytracerId].has_SDF_build_debug_info)
          print_sdf_debug_info(ui_ctx.scenesCtx[settings.raytracerId].sdf_build_debug_info, hit.primId, false, ui_ctx);
      }
    }
    return true;
  }

  bool preload_vtk_dataset(const char* path, int id, UIContext &ui_ctx)
  {
    add_to_log({"Loading VTK dataset: " + std::string(path), ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
    vtk::Dataset &dataset = ui_ctx.scenesCtx[id].vtkDataset;

    std::string file_ext = file_extension(path);
    if (file_ext == "blk")
      vtk::create_procedural_dataset(path, dataset);
    else
      vtk::read_vtk_dataset(path, dataset);
    bool valid = dataset.unstructured_grids.size() > 0;
    if (!valid)
      add_to_log({"Invalid dataset", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
    else
    {
      add_to_log({"Loaded "+std::to_string(dataset.unstructured_grids.size())+" unstructured grid(s)", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
    
      auto &cae_info = ui_ctx.preloaded_cae_info;

      for (const auto &pa : dataset.unstructured_grids[0].point_data_arrays)
      {
        cae_info.point_channels.emplace_back();
        cae_info.point_channels.back().type = pa.values.type;
        cae_info.point_channels.back().num_components = pa.num_components;
        cae_info.point_channels.back().name = pa.name;
      }

      for (const auto &ca : dataset.unstructured_grids[0].cell_data_arrays)
      {
        cae_info.cell_channels.emplace_back();
        cae_info.cell_channels.back().type = ca.values.type;
        cae_info.cell_channels.back().num_components = ca.num_components;
        cae_info.cell_channels.back().name = ca.name;
      }
    }
    return valid;
  }

  void convert_mesh_to_sdf_scene(const char *input_file, bool load_reference_scene, bool load_combined_scene,
                                 const Block &settings_blk, UIContext &ui_ctx)
  {
    assert(settings_blk.get_block("sparse_octree_settings"));

    cmesh4::SimpleMesh mesh = cmesh4::preprocess_mesh(cmesh4::LoadMesh(input_file, false), cmesh4::PreprocessSettings());

    SparseOctreeSettings settings;
    load_from_blk(settings, settings_blk.get_block("sparse_octree_settings"));

    SDFType sdf_type = (SDFType)settings_blk.get_enum("sdf_type");

    int id = 0;      

    ui_ctx.scenes[id].clear();
    int geomId = -1;
    sdf_converter::GlobalOctreeDebugInfo sdf_build_debug_info;

    switch (sdf_type)
    {
    case SDFType::REGULAR_GRID:
      {
        GridSettings grid_settings;
        if (settings_blk.get_block("grid_settings"))
          load_from_blk(grid_settings, settings_blk.get_block("grid_settings"));
        else
          printf("[convert_mesh_to_sdf_scene] ERROR: type is SDFType::REGULAR_GRID, but no grid settings\n");

        auto grid = sdf_converter::create_sdf_grid(grid_settings, mesh);
        auto *geom = new LiteScene::SDFGridGeometry();
        geomId = ui_ctx.scenes[id].add_geometry(geom);
        geom->init(std::move(grid));//init after add_geometry, because only add_geometry puts custom data to it
      }
      break;
    case SDFType::FRAME_OCTREE:
      {
        auto octree = sdf_converter::create_sdf_frame_octree(settings, mesh);
        auto *geom = new LiteScene::SDFFrameOctreeGeometry();
        geomId = ui_ctx.scenes[id].add_geometry(geom);
        geom->init(std::move(octree));//init after add_geometry, because only add_geometry puts custom data to it
      }
      break;
    case SDFType::SCOM_TREE:
      {
        scom::Settings scom_settings;
        if (settings_blk.get_block("scom_settings"))
          load_from_blk(scom_settings, settings_blk.get_block("scom_settings"));
        else
          printf("[convert_mesh_to_sdf_scene] ERROR: type is SDFType::SCOM_TREE, but no scom settings\n");

        COctreeV3Settings co_settings;
        if (settings_blk.get_block("coctree_settings"))
          load_from_blk(co_settings, settings_blk.get_block("coctree_settings"));
        else
          printf("[convert_mesh_to_sdf_scene] ERROR: type is SDFType::SCOM_TREE, but no coctree settings\n");

        auto coctree = sdf_converter::create_COctree_v3(settings, co_settings, scom_settings, mesh, true);
        auto *geom = new LiteScene::SDFCOctreeGeometry();
        geomId = ui_ctx.scenes[id].add_geometry(geom);
        geom->init(std::move(coctree));//init after add_geometry, because only add_geometry puts custom data to it
      }
      break;
    case SDFType::MULTI_OCTREE:
      {
        bool save_debug_info = settings_blk.get_bool("save_sdf_debug_info", false);
        sdf_converter::GlobalOctree g;
        sdf_converter::GlobalOctreeBuildStat build_stat;
        if (save_debug_info)
          g.debug_info = &sdf_build_debug_info;

        {
          auto t1 = std::chrono::steady_clock::now();
          auto plo = sdf_converter::create_triangle_list_octree(mesh, sdf_converter::PLOSettings(settings.depth));
          auto t2 = std::chrono::steady_clock::now();
          build_stat.time_build_PLO = std::chrono::duration<float, std::milli>(t2 - t1).count();
          mesh_octree_to_global_octree(settings, mesh, plo, g, &build_stat);
        }
        SdfMultiOctree multi_octree;
        global_octree_to_multi_octree(g, multi_octree);
        sdf_converter::print_octree_build_stat(build_stat);

        auto *geom = new LiteScene::SDFMultiOctreeGeometry();
        geomId = ui_ctx.scenes[id].add_geometry(geom);
        geom->init(std::move(multi_octree));//init after add_geometry, because only add_geometry puts custom data to it
      }
      break;
    case SDFType::FLOOD_OCTREE_TYPES:
    case SDFType::FLOOD_OCTREE_CODES:
    {
      auto plo = sdf_converter::create_triangle_list_octree(mesh, sdf_converter::PLOSettings(settings.depth));
      sdf_converter::FloodedOctree fo;
      sdf_converter::prim_octree_to_flooded_octree_linear(plo, fo, settings.min_depth);
      sdf_converter::calculate_node_codes(fo);
      SdfMultiOctree multi_octree;

      if (sdf_type == SDFType::FLOOD_OCTREE_TYPES)
        sdf_converter::visualize_flooded_octree(fo, multi_octree, false, false);
      else
        sdf_converter::visualize_flooded_octree(fo, multi_octree, false, true);

      auto *geom = new LiteScene::SDFMultiOctreeGeometry();
      geomId = ui_ctx.scenes[id].add_geometry(geom);
      geom->init(std::move(multi_octree));//init after add_geometry, because only add_geometry puts custom data to it
    }
      break;
    case SDFType::SDF_DAG:
    {
      scom2::Settings scom2_settings;
      if (settings_blk.get_block("scom2_settings"))
        load_from_blk(scom2_settings, settings_blk.get_block("scom2_settings"));
      else
        printf("[convert_mesh_to_sdf_scene] ERROR: type is SDFType::SDF_DAG, but no scom2 settings\n");

      SdfDAG dag = sdf_converter::create_sdf_dag(settings, scom2_settings, mesh);
      auto *geom = new LiteScene::SDFDAGGeometry();
      geomId = ui_ctx.scenes[id].add_geometry(geom);
      geom->init(std::move(dag)); // init after add_geometry, because only add_geometry puts custom data to it
    }
      break;
    case SDFType::SCOM_TREE_2:
    {
      scom2::Settings scom2_settings;
      if (settings_blk.get_block("scom2_settings"))
        load_from_blk(scom2_settings, settings_blk.get_block("scom2_settings"));
      else
        printf("[convert_mesh_to_sdf_scene] ERROR: type is SDFType::SCOM_TREE_2, but no scom2 settings\n");

      SCom2Tree scom2_tree = sdf_converter::create_scom2_tree(settings, scom2_settings, mesh);
      auto *geom = new LiteScene::SCom2Geometry();
      geomId = ui_ctx.scenes[id].add_geometry(geom);
      geom->init(std::move(scom2_tree)); // init after add_geometry, because only add_geometry puts custom data to it
    }
      break;
    default:
      break;
    }

    if (load_combined_scene)
    {
      uint32_t originalId = ui_ctx.scenes[id].add_mesh(mesh);
      ui_ctx.scenes[id].add_instance(originalId, LiteMath::float4x4());
      ui_ctx.scenes[id].geometries[geomId]->custom_data.append_attribute(L"original_id").set_value(originalId);
    }
    else
    {
      ui_ctx.scenes[id].add_instance(geomId, LiteMath::float4x4());
    }
    initialize_scene_context(ui_ctx, id);

    if (sdf_build_debug_info.nodes_info.size() > 0)
    {
      ui_ctx.scenesCtx[id].has_SDF_build_debug_info = true;
      ui_ctx.scenesCtx[id].sdf_build_debug_info = sdf_build_debug_info;
    }

    if (load_reference_scene)
    {
      id++;
      ui_ctx.scenes[id].clear();
      geomId = ui_ctx.scenes[id].add_mesh(mesh);
      ui_ctx.scenes[id].add_instance(geomId, LiteMath::float4x4());
      initialize_scene_context(ui_ctx, id);
    }

    reload_scenes(id+1, ui_ctx);  
  }

  void convert_CAD_to_sdf_scene(std::vector<cmesh4::SimpleMesh> &orig_meshes, const std::vector<std::string> &mesh_names,
                                bool load_reference_scene, bool show_original_parts, bool load_combined_scene,
                                const CSVData &csv_data, const Block &settings_blk, UIContext &ui_ctx)
  {
    auto t_start = std::chrono::steady_clock::now();
    int mat_count = csv_data.row_count == 0 ? 1 : csv_data.row_count;
    bool recreate_components = false;

    CADSDFType sdf_type = (CADSDFType)settings_blk.get_enum("CAD_to_SDF_type");

    for (int i=0;i<orig_meshes.size();i++)
    {
      orig_meshes[i].matIndices = std::vector<unsigned int>(orig_meshes[i].TrianglesNum(), i);
    }
    scene_converter::Settings settings;
    load_from_blk(settings, &settings_blk);
    std::vector<cmesh4::SimpleMesh> meshes = cmesh4::preprocess_mesh_parts(orig_meshes, cmesh4::PreprocessSettings(), false);
    
    CompoundScene comp_scene;
    scene_converter::convert_CAD_parts_array(meshes, settings, comp_scene);

    int id = 0;      
    ui_ctx.scenes[id].clear();
    
    //if (mat_count > 1)
    //write_csv_color_to_scene(ui_ctx.scenes[id], csv_data, csv_data.row_count);
    //set_mat_ids_from_csv(csv_data, meshes, mesh_names, true);
    
    assert(comp_scene.compounds.size() > 0);
    assert(comp_scene.octrees.size() == comp_scene.compounds.size());

    for (int part_id = 0; part_id < comp_scene.octrees.size(); part_id++)
    {
      auto &octree = comp_scene.octrees[part_id];
      uint32_t geomId = INVALID_IDX;
      switch (sdf_type)
      {
      case CADSDFType::MULTI_OCTREE:
      {
        SdfMultiOctree multi_octree;
        global_octree_to_multi_octree(octree, multi_octree);
        auto *geom = new LiteScene::SDFMultiOctreeGeometry();
        geomId = ui_ctx.scenes[id].add_geometry(geom);
        geom->init(std::move(multi_octree)); // init after add_geometry, because only add_geometry puts custom data to it
      }
      break;
      case CADSDFType::SCOM_TREE_2:
      {
        scom2::Settings scom2_settings;
        if (settings_blk.get_block("scom2_settings"))
          load_from_blk(scom2_settings, settings_blk.get_block("scom2_settings"));
        else
          printf("[convert_mesh_to_sdf_scene] ERROR: type is SDFType::SCOM_TREE_2, but no scom2 settings\n");

        if (scom2_settings.octree_levels_to_bricks > 0)
        {
          uint32_t depth_increase_mult = 1 << scom2_settings.octree_levels_to_bricks;
          sdf_converter::GlobalOctree go_ring[2];
          go_ring[0] = std::move(octree);
          for (uint32_t i = 0; i < scom2_settings.octree_levels_to_bricks; i++)
            sdf_converter::upsample_global_octree(go_ring[i % 2], go_ring[(i + 1) % 2]);
          octree = std::move(go_ring[scom2_settings.octree_levels_to_bricks % 2]);
        }

        SdfDAG dag;
        sdf_converter::global_octree_to_DAG(octree, dag, scom2_settings);
        SCom2Tree scom2_tree;
        pack_SCom2(scom2_settings, dag, scom2_tree);
        auto *geom = new LiteScene::SCom2Geometry();
        geomId = ui_ctx.scenes[id].add_geometry(geom);
        geom->init(std::move(scom2_tree)); // init after add_geometry, because only add_geometry puts custom data to it
      }
      break;
      default:
        break;
      };
      if (load_combined_scene)
      {
        uint32_t originalId = ui_ctx.scenes[id].add_mesh(comp_scene.compounds[part_id]);
        ui_ctx.scenes[id].add_instance(originalId, LiteMath::float4x4());
        ui_ctx.scenes[id].geometries[geomId]->custom_data.append_attribute(L"original_id").set_value(originalId);
      }
      else
      {
        ui_ctx.scenes[id].add_instance(geomId, LiteMath::float4x4());
      }
    }
    
    initialize_scene_context(ui_ctx, id);

    if (comp_scene.debug_infos.size() > 0)
    {
      ui_ctx.scenesCtx[id].has_SDF_build_debug_info = true;
      ui_ctx.scenesCtx[id].sdf_build_debug_info = comp_scene.debug_infos[0];
    }

    if (load_reference_scene)
    {
      id++;
      ui_ctx.scenes[id].clear();

      //if (mat_count > 1)
      //write_csv_color_to_scene(ui_ctx.scenes[id], csv_data, orig_meshes.size());
      
      const std::vector<cmesh4::SimpleMesh> &ms = show_original_parts ? meshes : comp_scene.compounds;
      for (auto &mesh : ms)
      {
        if (cmesh4::check_is_valid(mesh) == false)
          continue;
        int geomId = ui_ctx.scenes[id].add_mesh(mesh);
        ui_ctx.scenes[id].add_instance(geomId, LiteMath::float4x4());
      }
      initialize_scene_context(ui_ctx, id);
    }

    reload_scenes(id+1, ui_ctx);

    auto t_end = std::chrono::steady_clock::now();
    float total_time_ms = std::chrono::duration<float, std::milli>(t_end - t_start).count();
    printf("Total build time  = %.1f s\n", total_time_ms/1000);
  }

  void convert_VTK_to_octree(const char *input_file, bool load_reference_scene, const Block &settings_blk, UIContext &ui_ctx)
  {
    int id_sdf = 0;
    int id_ref = 1;
    auto t_start = std::chrono::steady_clock::now();
    
    vtk::Dataset &dataset = ui_ctx.scenesCtx[id_ref].vtkDataset;
    bool dataset_loaded = dataset.unstructured_grids.size() > 0;
    if (!dataset_loaded)
      dataset_loaded = preload_vtk_dataset(input_file, id_ref, ui_ctx);

    vtk::finish_loading_vtk_dataset(dataset, settings_blk.get_block("vtk_load_options"));

    SparseOctreeSettings settings;
    load_from_blk(settings, settings_blk.get_block("octree_settings"));

    ASDFType asdf_type = (ASDFType)settings_blk.get_enum("ASDF_representation");
    bool compress_channels = settings_blk.get_bool("compress_channels", false);

    ui_ctx.scenes[id_sdf].clear();
    for (auto &ug : dataset.unstructured_grids)
    {
      if (settings.fill_internal_volume == true && ug.type == vtk::UnstructuredGrid::Type::SURFACE)
        continue;
      sdf_converter::GlobalOctree out_octree;

      vtk::mark_border_cells(ug);

      //temporary workaround, as we cannot create sdf from 2D cells at the moment
      if (settings.fill_internal_volume == false)
      {
        auto mesh = vtk::triangulate_grid_bounds(ug, false);
        auto t1 = std::chrono::high_resolution_clock::now();
        auto plo = sdf_converter::create_primitive_list_octree<cmesh4::AugmentedMesh, sdf_converter::AugTriangleInBBoxFunc, true>(
                   mesh, mesh.indices.size()/3, sdf_converter::PLOSettings(settings.depth));
        auto t2 = std::chrono::high_resolution_clock::now();
        sdf_converter::GlobalOctreeBuildStat stat;
        stat.time_build_PLO = std::chrono::duration<float, std::milli>(t2 - t1).count();
        sdf_converter::augmented_mesh_to_global_octree(settings, mesh, plo, out_octree, &stat);
        sdf_converter::print_octree_build_stat(stat);
      }
      else
      {
        auto t1 = std::chrono::high_resolution_clock::now();
        auto plo = sdf_converter::create_primitive_list_octree<vtk::UnstructuredGrid, sdf_converter::UGCellInBBoxFunc, true>(
                   ug, ug.cells_count, sdf_converter::PLOSettings(settings.depth, 0, 1.5f, true, settings.fill_all_nodes ? 0 : settings.depth));
        auto t2 = std::chrono::high_resolution_clock::now();
        sdf_converter::GlobalOctreeBuildStat stat;
        stat.time_build_PLO = std::chrono::duration<float, std::milli>(t2 - t1).count();
        sdf_converter::unstructured_grid_to_global_octree(settings, ug, plo, out_octree, &stat, true);
        sdf_converter::print_octree_build_stat(stat);
      }

      std::string dataset_name = ug.name.empty() ? "unnamed_dataset" : ug.name;
      
      //storing dataset is required if we want to cut it (re-triangulation), but if it 
      //is too large, there can be not enough memory to store everything: SDF, mesh, dataset
      //so it is better to delete it here before next memory spike (loading to renderer)
      if (!load_reference_scene || dataset.unstructured_grids[0].p_cells.size() > 5'000'000)
        ug = vtk::UnstructuredGrid();
      
      switch (asdf_type)
      {
        case ASDFType::MULTI_OCTREE:
        {
          auto geom_m = new LiteScene::SDFAugmentedMultiOctreeGeometry();
          uint32_t geomId_m = ui_ctx.scenes[id_sdf].add_geometry(geom_m);
          geom_m->init(SdfMultiOctreeAugmented()); //init after add_geometry, because only add_geometry puts custom data to it
          sdf_converter::global_octree_to_augmented_multi_octree(out_octree, geom_m->model);
          geom_m->name = dataset_name;
          ui_ctx.scenes[id_sdf].add_instance(geomId_m, LiteMath::float4x4());
        }
        break;
        case ASDFType::SDF_DAG:
        {
          scom2::Settings scom2_settings;
          if (settings_blk.get_block("scom2_settings"))
            load_from_blk(scom2_settings, settings_blk.get_block("scom2_settings"));
          auto geom_c = new LiteScene::SDFDAGGeometry();
          uint32_t geomId_c = ui_ctx.scenes[id_sdf].add_geometry(geom_c);
          geom_c->init(SdfDAG()); //init after add_geometry, because only add_geometry puts custom data to it
          sdf_converter::global_octree_to_DAG(out_octree, geom_c->model, scom2_settings);
          geom_c->name = dataset_name;
          ui_ctx.scenes[id_sdf].add_instance(geomId_c, LiteMath::float4x4());
        }
        break;
        case ASDFType::SCOM_TREE_2:
        {
          scom2::Settings scom2_settings;
          if (settings_blk.get_block("scom2_settings"))
            load_from_blk(scom2_settings, settings_blk.get_block("scom2_settings"));

          SdfDAG dag;
          sdf_converter::global_octree_to_DAG(out_octree, dag, scom2_settings);

          auto geom_c = new LiteScene::SCom2Geometry();
          uint32_t geomId_c = ui_ctx.scenes[id_sdf].add_geometry(geom_c);
          geom_c->init(SCom2Tree()); //init after add_geometry, because only add_geometry puts custom data to it
          scom2::pack_SCom2(scom2_settings, dag, geom_c->model);
          geom_c->name = dataset_name;
          ui_ctx.scenes[id_sdf].add_instance(geomId_c, LiteMath::float4x4());

          if (compress_channels)
          {
            for (auto &c : geom_c->model.point_channels)
            {
              if (c.type == DataChannel::Type::FLOAT)
                vtk::compress_float_channel(c);
            }
            for (auto &c : geom_c->model.voxel_channels)
            {
              if (c.type == DataChannel::Type::FLOAT)
                vtk::compress_float_channel(c);
            }
          }
        }
        break;
        default:
          assert(false);
      };
    }
    initialize_scene_context(ui_ctx, id_sdf);

    auto t_end = std::chrono::steady_clock::now();
    float total_time_ms = std::chrono::duration<float, std::milli>(t_end - t_start).count();
    printf("Total build time  = %.1f s\n", total_time_ms/1000);

    if (load_reference_scene)
    {
      ui_ctx.scenes[id_ref].clear();
      for (auto &ug : dataset.unstructured_grids)
      {
        auto mesh = vtk::triangulate_grid_bounds(ug, false);

        LiteScene::AugmentedMeshGeometry *geom = new LiteScene::AugmentedMeshGeometry();
        int geomId = ui_ctx.scenes[id_ref].add_geometry(geom);
        geom->init(std::move(mesh)); //init after add_geometry, because only add_geometry puts custom data to it
        if (!ug.name.empty())
          geom->name = ug.name;
        ui_ctx.scenes[id_ref].add_instance(geomId, LiteMath::float4x4());
      }
      initialize_scene_context(ui_ctx, id_ref);
    }
  
    reload_scenes(load_reference_scene ? 2 : 1, ui_ctx);
  }

  void convert_volume_to_scom(const char *input_file, bool load_reference_scene,
                              const Block &settings_blk, UIContext &ui_ctx)
  {
    std::string path = input_file;
    SparseOctreeSettings settings;
    scom2::Settings scom2_settings;
    VolumeGridType grid_type = (VolumeGridType)settings_blk.get_enum("volume_type");
    load_from_blk(settings, settings_blk.get_block("sparse_octree_settings"));
    load_from_blk(scom2_settings, settings_blk.get_block("scom2_settings"));

    //split thr must not be 0 in scom2
    settings.split_thr = std::max(settings.split_thr, 1e-9f);

    ui_ctx.scenes[0].clear();
    if (load_reference_scene)
      ui_ctx.scenes[1].clear();

    sdf_converter::GlobalOctree octree;
    if (grid_type == VolumeGridType::VOLUME_GRID_3D_DENSITY)
    {
      DensityGrid grid;
      load_density_grid(grid, path);
      sdf_converter::density_grid_to_global_octree(settings, grid, octree, nullptr);

      if (load_reference_scene)
      {
        LiteScene::DensityGridGeometry  *geom = new LiteScene::DensityGridGeometry();
        int geomId = ui_ctx.scenes[1].add_geometry(geom);
        geom->init(std::move(grid)); //init after add_geometry, because only add_geometry puts custom data to it
        ui_ctx.scenes[1].add_instance(geomId, LiteMath::float4x4());
      }
    }
    else if (grid_type == VolumeGridType::VOLUME_GRID_4D_DENSITY)
    {
      DensityMultiGrid grid;
      load_density_grid(grid, path);
      sdf_converter::density_grid_to_global_octree(settings, grid, octree, nullptr);

      if (load_reference_scene)
      {
        LiteScene::DensityMultiGridGeometry  *geom = new LiteScene::DensityMultiGridGeometry();
        int geomId = ui_ctx.scenes[1].add_geometry(geom);
        geom->init(std::move(grid)); //init after add_geometry, because only add_geometry puts custom data to it
        ui_ctx.scenes[1].add_instance(geomId, LiteMath::float4x4());
      }
    }
    else if (grid_type == VolumeGridType::VOLUME_GRID_3D_RGB_DENSITY)
    {
      add_to_log({"3D RGB density grid not supported", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
    }
    else if (grid_type == VolumeGridType::VOLUME_GRID_4D_RGB_DENSITY)
    {
      add_to_log({"4D RGB density grid not supported", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
    }

    SdfDAG dag;
    SCom2Tree scom2_tree;
    sdf_converter::global_octree_to_DAG(octree, dag, scom2_settings);
    octree = sdf_converter::GlobalOctree();

    scom2::pack_SCom2(scom2_settings, dag, scom2_tree);
    dag = SdfDAG();

    {
      LiteScene::SCom2Geometry *geom = new LiteScene::SCom2Geometry();
      int geomId = ui_ctx.scenes[0].add_geometry(geom);
      geom->init(std::move(scom2_tree));
      ui_ctx.scenes[0].add_instance(geomId, LiteMath::float4x4());
    }
    
    initialize_scene_context(ui_ctx, 0);
    if (load_reference_scene)
      initialize_scene_context(ui_ctx, 1);
    reload_scenes(load_reference_scene ? 2 : 1, ui_ctx);
  }

  CSVData read_colors_csv_if_present(const std::string &filename)
  {
    std::filesystem::path path(filename);
    std::filesystem::path csv_path = path.parent_path() / std::filesystem::path("name-col-type.csv");

    if (std::filesystem::exists(csv_path))
      return load_csv(csv_path.string());
    else
      return CSVData();
  }

  void load_scene_from_mesh(const char* path, int id, UIContext &ui_ctx, bool rescale)
  {
    add_to_log({"Loading mesh: " + std::string(path), ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
    float4x4 transform = float4x4();
    auto mesh = cmesh4::LoadMesh(path, false, true, &transform, rescale);
    bool valid = cmesh4::check_is_valid(mesh, true);
    if (!valid)
      add_to_log({"Invalid mesh", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
    else
    {
      add_to_log({"Loaded mesh", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      int geomId = ui_ctx.scenes[id].add_mesh(mesh);
      ui_ctx.scenes[id].add_instance(geomId, LiteMath::float4x4());

      if (!ui_ctx.fixedTransformMatrixSetUp)
      {
        ui_ctx.fixedTransformMatrixSetUp = true;
        ui_ctx.fixedTransformMatrix = transform;
      }
    }
    
    ui_ctx.activeScenePaths[id] = "";
  }

  void write_csv_color_to_scene(LiteScene::HydraScene &scene, const CSVData &csv_data, int materials_count)
  {
    for (int i = 0; i < materials_count; i++)
    {
      float4 color(1, 0, 1, 1);
      if (i < csv_data.row_count)
        sscanf(csv_data[1][i].c_str(), "(%f, %f, %f)", &color.x, &color.y, &color.z);

      LiteScene::DiffuseMaterial *mat = new LiteScene::DiffuseMaterial(i, "color_mat_" + std::to_string(i));
      LiteScene::ColorHolder ch;
      ch.color = color;
      mat->reflectance = ch;
      mat->bsdf_type = LiteScene::DiffuseMaterial::BSDF::LAMBERT;
      scene.materials[i] = mat;
    }
  }

  void set_mat_ids_from_csv(const CSVData &csv_data, std::vector<cmesh4::SimpleMesh> &meshes, const std::vector<std::string> &mesh_names,
                            bool merge_identical_materials)
  {
    std::map<std::string, uint32_t> material_id_by_part_name;
    PositionMap<uint32_t> unique_mat_id_by_color;
    for (int i = 0; i < csv_data.row_count; i++)
    {
      uint32_t mat_id = i;
      float3 color(1, 0, 1);
      sscanf(csv_data[1][i].c_str(), "(%f, %f, %f)", &color.x, &color.y, &color.z);
      if (merge_identical_materials)
      {
        auto it = unique_mat_id_by_color.find(color);
        if (it == unique_mat_id_by_color.end())
          unique_mat_id_by_color[color] = i;
        else
          mat_id = it->second;
      }
      material_id_by_part_name[csv_data[0][i]] = mat_id;
    }

    printf("%d parts, %d (%d) materials\n", (int)mesh_names.size(), (int)material_id_by_part_name.size(), csv_data.row_count);

    if (merge_identical_materials)
      printf("%d/%d unique materials\n", (int)unique_mat_id_by_color.size(), csv_data.row_count);

    std::map<std::string, uint32_t> count_by_part_name;
    for (int i = 0; i < meshes.size(); i++)
    {
      auto cit = count_by_part_name.find(mesh_names[i]);
      if (cit == count_by_part_name.end())
        count_by_part_name[mesh_names[i]] = 0;
      uint32_t id = count_by_part_name[mesh_names[i]];

      auto it = material_id_by_part_name.find(mesh_names[i]);
      auto alternative_it = material_id_by_part_name.find(mesh_names[i] + "_" + std::to_string(id));
      if (it != material_id_by_part_name.end())
        cmesh4::set_mat_id(meshes[i], it->second);
      else if (alternative_it != material_id_by_part_name.end()) {
        cmesh4::set_mat_id(meshes[i], alternative_it->second);
        count_by_part_name[mesh_names[i]]++;
      }
      else
      {
        printf("part name = <%s> is not found!!!\n", mesh_names[i].c_str());
        cmesh4::set_mat_id(meshes[i], material_id_by_part_name.size() - 1);
      }
    }
  }

  void load_mesh_with_CSV_colors(const char* path, int id, const CSVData &csv_data, UIContext &ui_ctx)
  {
    add_to_log({"Loading mesh with CSV colors: " + std::string(path), ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
    std::vector<cmesh4::SimpleMesh> meshes;
    std::vector<std::string> mesh_names;
    cmesh4::load_mesh_array_from_obj(path, meshes, &mesh_names);
    write_csv_color_to_scene(ui_ctx.scenes[id], csv_data, csv_data.row_count);
    set_mat_ids_from_csv(csv_data, meshes, mesh_names, false);

    auto mesh = cmesh4::MergeMeshes(meshes);
    cmesh4::PreprocessSettings ps;
    ps.force_merge_distinct_normals = false;
    float4x4 transform;
    mesh = cmesh4::preprocess_mesh(mesh, ps, &transform);
    bool valid = cmesh4::check_is_valid(mesh, true);
    if (!valid)
      add_to_log({"Invalid mesh", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
    else
    {
      add_to_log({"Loaded mesh", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      int geomId = ui_ctx.scenes[id].add_mesh(mesh);
      ui_ctx.scenes[id].add_instance(geomId, LiteMath::float4x4());

      if (!ui_ctx.fixedTransformMatrixSetUp)
      {
        ui_ctx.fixedTransformMatrixSetUp = true;
        ui_ctx.fixedTransformMatrix = transform;
      }
    }
    
    ui_ctx.activeScenePaths[id] = "";    
  }

  void load_scene_from_SDF_grid(const char* path, int id, UIContext &ui_ctx)
  {
    add_to_log({"Loading SDF grid: " + std::string(path), ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
    SdfGrid grid;
    load_sdf_grid(grid, path);
    bool valid = grid.data.size() > 0;
    if (!valid)
      add_to_log({"Invalid SDF grid", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
    else
    {
      add_to_log({"Loaded SDF grid", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      LiteScene::SDFGridGeometry *geom = new LiteScene::SDFGridGeometry();
      int geomId = ui_ctx.scenes[id].add_geometry(geom);
      geom->init(std::move(grid)); //init after add_geometry, because only add_geometry puts custom data to it
      ui_ctx.scenes[id].add_instance(geomId, LiteMath::float4x4());
    }    
  }

  void load_scene_from_SDF_SBS(const char *path, int id, UIContext &ui_ctx)
  {
    add_to_log({"Loading SDF SBS: " + std::string(path), ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
    SdfSBS sbs;
    load_sdf_SBS(sbs, path);
    bool valid = sbs.nodes.size() && sbs.values.size() && sbs.values_f.size();
    if (!valid)
      add_to_log({"Invalid SDF SBS", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
    else
    {
      add_to_log({"Loaded SDF SBS", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      LiteScene::SDFSBSGeometry *geom = new LiteScene::SDFSBSGeometry();
      int geomId = ui_ctx.scenes[id].add_geometry(geom);
      geom->init(std::move(sbs)); //init after add_geometry, because only add_geometry puts custom data to it
      ui_ctx.scenes[id].add_instance(geomId, LiteMath::float4x4());
    }    
  }

  void load_scene_from_SDF_octree(const char* path, int id, UIContext &ui_ctx)
  {
    add_to_log({"Loading SDF frame octree: " + std::string(path), ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
    std::vector<SdfFrameOctreeNode> frame_octree;
    load_sdf_frame_octree(frame_octree, path);
    bool valid = frame_octree.size() > 0;
    if (!valid)
      add_to_log({"Invalid SDF octree", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
    else
    {
      add_to_log({"Loaded SDF octree", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      LiteScene::SDFFrameOctreeGeometry *geom = new LiteScene::SDFFrameOctreeGeometry();
      int geomId = ui_ctx.scenes[id].add_geometry(geom);
      geom->init(std::move(frame_octree)); //init after add_geometry, because only add_geometry puts custom data to it
      ui_ctx.scenes[id].add_instance(geomId, LiteMath::float4x4());
    }    
  }

  void load_scene_from_SDF_multi_octree(const char* path, int id, UIContext &ui_ctx)
  {
    add_to_log({"Loading SDF frame octree: " + std::string(path), ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
    SdfMultiOctree multi_octree;
    load_sdf_multi_octree(multi_octree, path);
    printf("sizes %d %d\n", (int)multi_octree.nodes.size(), (int)multi_octree.values.size());
    bool valid = multi_octree.nodes.size() > 0;
    if (!valid)
      add_to_log({"Invalid SDF multi octree", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
    else
    {
      add_to_log({"Loaded SDF multi octree", ui_ctx.logShowTime, LogEntry::Type::INFO}, ui_ctx);
      LiteScene::SDFMultiOctreeGeometry *geom = new LiteScene::SDFMultiOctreeGeometry();
      int geomId = ui_ctx.scenes[id].add_geometry(geom);
      geom->init(std::move(multi_octree)); //init after add_geometry, because only add_geometry puts custom data to it
      ui_ctx.scenes[id].add_instance(geomId, LiteMath::float4x4());
    }    
  }

  void load_scene_from_vtk(const char* path, int id, UIContext &ui_ctx)
  {
    vtk::Dataset &dataset = ui_ctx.scenesCtx[id].vtkDataset;
    bool dataset_loaded = dataset.unstructured_grids.size() > 0;
    if (!dataset_loaded)
      dataset_loaded = preload_vtk_dataset(path, id, ui_ctx);
    
    if (dataset_loaded)
    {
      vtk::finish_loading_vtk_dataset(dataset);

      for (auto &ug : dataset.unstructured_grids)
      {
        auto mesh = vtk::triangulate_grid_bounds(ug, false);

        LiteScene::AugmentedMeshGeometry *geom = new LiteScene::AugmentedMeshGeometry();
        int geomId = ui_ctx.scenes[id].add_geometry(geom);
        geom->init(std::move(mesh)); //init after add_geometry, because only add_geometry puts custom data to it
        if (!ug.name.empty())
          geom->name = ug.name;
        ui_ctx.scenes[id].add_instance(geomId, LiteMath::float4x4());
      }
    }
  }

  void MRFullPlugin::initContext(UIContext &ui_ctx)
  {
    load_default_blocks(ui_ctx);
  }

  void MRFullPlugin::loadScene(const Block &args, const char *path, int id, UIContext &ui_ctx)
  {
    std::string file_ext = file_extension(path);
    CSVData colors_csv = read_colors_csv_if_present(path);
    bool is_procedural_mesh = false;
    bool is_procedural_vtk = false;
    if (file_ext == "blk")
    {
      Block proc_settings;
      load_block_from_file(path, proc_settings);
      std::string generator_name = proc_settings.get_string("generator", "unknown");
      if (generator_name == "mesh" || generator_name == "composite")
        is_procedural_mesh = true;
      else if (generator_name == "vtk")
        is_procedural_vtk = true;
      else
        add_to_log({".blk must contain valid generator name to be treated as model", ui_ctx.logShowTime, LogEntry::Type::ERROR}, ui_ctx);
    }

    bool rescale = args.get_bool("rescale", true);
    if (file_ext == "obj")
    {
      if (colors_csv.row_count > 0)
        load_mesh_with_CSV_colors(path, id, colors_csv, ui_ctx);
      else
        load_scene_from_mesh(path, id, ui_ctx, rescale);
    }
    else if (file_ext == "vsgf" || file_ext == "ply" || file_ext == "stl" || is_procedural_mesh)
      load_scene_from_mesh(path, id, ui_ctx, rescale);
    else if (file_ext == "grid")
      load_scene_from_SDF_grid(path, id, ui_ctx);
    else if (file_ext == "octree")
      load_scene_from_SDF_octree(path, id, ui_ctx);
    else if (file_ext == "mo")
      load_scene_from_SDF_multi_octree(path, id, ui_ctx);
    else if (file_ext == "vtk" || file_ext == "vtp" || file_ext == "vtm" || file_ext == "vtu" || is_procedural_vtk)
      load_scene_from_vtk(path, id, ui_ctx);
    else if (file_ext == "bin")
      load_scene_from_SDF_SBS(path, id, ui_ctx);
    else if (file_ext == "png")
    {
      ui_ctx.activeScenePaths[id] = path;
    }
    else
      add_to_log({"Invalid scene format. xml, obj, vsgf, ply, stl, blk, step, octree, vtk, vtp, vtm, vtu are supported",
                  ui_ctx.logShowTime, LogEntry::Type::ERROR},
                 ui_ctx);
  }

  void MRFullPlugin::executeCommand(const CommandBuffer::Command &cmd, CommandBuffer &cmd_buffer, 
                                    Settings &settings, UIContext &ui_ctx, RenderingContext &r_ctx, 
                                    RasterizationContext &rast_ctx, RaytracingContext &rt_ctx)
  {
    switch (cmd.type)
    {
      case Cmd::CAST_DEBUG_RAY:
      {
        float2 uv = cmd.args.get_vec2("uv");
        try_print_extra_debug(uv.x, uv.y, settings, ui_ctx, r_ctx, rt_ctx);
      }
      break;
      case Cmd::CONVERT_SCENE:
      {
        ConversionType c_type = (ConversionType)cmd.args.get_enum("conversion_mode");
        std::string input_file = cmd.args.get_string("input_file");
        bool load_reference_scene = cmd.args.get_bool("load_reference_scene");
        bool load_combined_scene = cmd.args.get_bool("load_combined_scene");
        if (c_type == ConversionType::CAD_PARTS_TO_MULTI_OCTREE)
        {
          std::vector<cmesh4::SimpleMesh> meshes;
          std::vector<std::string> mesh_names;
          if (std::filesystem::path(input_file).extension() == ".obj")
            cmesh4::load_mesh_array_from_obj(input_file.c_str(), meshes, &mesh_names);
          else
          {
            meshes.push_back(cmesh4::LoadMesh(input_file.c_str(), true, true));
            mesh_names.push_back(input_file);
          }
          Block *settings_blk = cmd.args.get_block("settings");
          CSVData colors_csv = read_colors_csv_if_present(input_file);
          convert_CAD_to_sdf_scene(meshes, mesh_names, 
                                   load_reference_scene, ui_ctx.conversionCADLoadSeparateParts, 
                                   load_combined_scene, colors_csv, *settings_blk, ui_ctx);
        }
        else if (c_type == ConversionType::CAD_FOLDER_TO_MULTI_OCTREE)
        {
          std::vector<cmesh4::SimpleMesh> meshes;
          std::vector<std::string> mesh_names;
          for (const auto &dir_entry : std::filesystem::directory_iterator(input_file))
          {
            if (dir_entry.path().extension() == ".stl")
            {
              meshes.push_back(cmesh4::LoadMeshFromStl(dir_entry.path().string(), true));
              mesh_names.push_back(dir_entry.path().filename().string());
            }
            else if (dir_entry.path().extension() == ".obj")
            {
              meshes.push_back(cmesh4::LoadMeshFromObj(dir_entry.path().string().c_str(), true));
              mesh_names.push_back(dir_entry.path().filename().string());
            }
          }
          Block *settings_blk = cmd.args.get_block("settings");
          CSVData colors_csv = read_colors_csv_if_present(input_file);
          convert_CAD_to_sdf_scene(meshes, mesh_names, load_reference_scene, ui_ctx.conversionCADLoadSeparateParts, 
                                   load_combined_scene, colors_csv, *settings_blk, ui_ctx);          
        }
        else if (c_type == ConversionType::MESH_TO_SDF)
        {
          convert_mesh_to_sdf_scene(input_file.c_str(), load_reference_scene, load_combined_scene, cmd.args, ui_ctx);
        }
        else if (c_type == ConversionType::VTK_DATASET_TO_OCTREE)
        {
          convert_VTK_to_octree(input_file.c_str(), load_reference_scene, cmd.args, ui_ctx);
        }
        else if (c_type == ConversionType::VOLUME_TO_OCTREE)
        {
          convert_volume_to_scom(input_file.c_str(), load_reference_scene, cmd.args, ui_ctx);
        }
      }
      break;
      case Cmd::PRELOAD_VTK_DATASET:
      {
        int rtId = cmd.args.get_int("rtId", settings.raytracerId);
        std::string path = cmd.args.get_string("input_file");
        preload_vtk_dataset(path.c_str(), rtId, ui_ctx);
      }
      break;
      default:
      break;
    }
  }
}