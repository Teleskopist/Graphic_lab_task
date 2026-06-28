#include <vector>
#include <memory>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <cmath>

#include "Image2d.h"
#include "MultiRenderer/MultiRenderer.h"
#include "utils/mesh/mesh.h"
#include "HydraRenderer/renderer.h"
#include "sdf/scom/similarity_compression.h"
#include "sdf/scom2/2D/scom2D_builder.h"
#include "model_check.h"
#include "utils/common/strings.h"
#include "lsa_map_generator.h"
#include "brick_dataset_generator.h"
#include "scom2_packing_sandbox.h"
#include "merge_scenes.h"
#include "offscreen_render.h"
#include "core/serialization.h"
#include "demos_scom.h"
#include "VoxelRenderer/mesh_voxelizer.h"
#include "VoxelRenderer/VoxelRenderer.h"
#include "utils/common/plane.h"
#include "utils/misc/scene_common.h"
#include "utils/mesh/mesh_internal.h"

#ifdef USE_GPU
#include "vk_context.h"
#endif

using LiteImage::Image2D;

extern double   g_buildTime; // used to get build time from deep underground of code
extern uint64_t g_buildTris; // used to get total tris processed by builder

int main(int argc, const char** argv)
{
  force_register_common_enums();
  
  if (argc > 1)
  {
    if (std::string(argv[1]) == "-check_model" && argc > 2)
    {
      check_model(argv[2]);
      return 0;
    }
    else if(std::string(argv[1]) == "--check-models" && argc > 3)
    {
      check_models(argv[2], argv[3]);
      return 0;
    }
    else if(std::string(argv[1]) == "--generate_lsa" && argc > 2)
    {
      generate_and_save_LSA_map(std::stoi(argv[2]));
      return 0;
    }
    else if(std::string(argv[1]) == "--generate_brick_dataset")
    {
      if (argc == 3)
      {
        bool exists = std::filesystem::exists(argv[2]);
        if (exists)
        {
          Block blk;
          load_block_from_file(argv[2], blk);
          generate_brick_dataset(blk);
          return 0;
        }
        else
        {
          printf("[generate_brick_dataset] config file %s not found\n", argv[2]);
        }
      }
      else if (argc == 4)
      {
        bool exists = std::filesystem::exists(argv[2]);
        uint32_t brick_count = std::stoi(argv[3]);
        if (exists && brick_count > 0)
        {
          generate_brick_dataset(argv[2], std::stoi(argv[3]));
          return 0;
        }
        else if (!exists)
          printf("[generate_brick_dataset] mesh file %s not found\n", argv[2]);
        else if (brick_count == 0)
          printf("[generate_brick_dataset] Use --generate_brick_dataset <mesh_path> <pairs_count>\n");
      }
      else
      {
        printf("Use --generate_brick_dataset <mesh_path> <pairs_count> or\n");
        printf("    --generate_brick_dataset <config_file_path>\n");
      }
      return 1;
    }
    else if(std::string(argv[1]) == "--scom2_packing" && argc > 2)
    {
      scom2_test_packing_modes(argv[2]);
      return 0;
    }
    else if (std::string(argv[1]) == "-merge_scenes" && argc > 3)
    {
      std::string out_folder = argv[2];
      std::vector<std::string> in_folders;
      for (int i = 3; i < argc; i++)
      {
        in_folders.push_back(argv[i]);
      }
      bool success = merge_scenes(in_folders, out_folder);
      return success ? 0 : 1;
    }
    else if (std::string(argv[1]) == "-demo_scom" && argc > 2)
    {
      SCom_demo_by_name(argv[2]);
      return 0;
    }
    else if (std::string(argv[1]) == "-render" && argc > 2)
    {
      // {
      // auto mesh = cmesh4::LoadMeshFromVSGF("saves/crytek_sponza_buddha/data/buddha_norm.vsgf");
      // COctreeV3Settings settings;
      // settings.brick_size = 3;
      // settings.sim_compression = 1;
      // settings.use_lods = true;

      // scom::Settings scom_settings;
      // scom_settings.clustering_algorithm = scom::ClusteringAlgorithm::REPLACEMENT;
      // scom_settings.similarity_threshold = 0.05f;

      // auto coctree = sdf_converter::create_COctree_v3(SparseOctreeSettings(8), settings, scom_settings, mesh);
      // save_coctree_v3(coctree, "saves/buddha_coctree_lods.bin");
      // return 0;
      // }
      
      //COctreeV3 coctree;
      //load_coctree_v3(coctree, "saves/buddha_coctree_lods.bin");
      //save_scene_xml("saves/instances_lods.xml", "buddha_coctree_lods.bin", get_info_coctree_v3(coctree), 0, DemoScene::INSTANCES_GRID);

      const char *render_help = R""""(
      Render Hydra scene/scenes with preset from config
      ./render_app -render path_to_config.blk // render with config file
      ./render_app -render "{scene:s = \"path_to_scene.xml\" output_file:s = \"saves/output.png\"}" //render with config from command line
      
      config can be either a single, i.e.
      {
        scene:s = "path_to_scene.xml" 
        output_file:s = "saves/output.png"
      }
      or a list, in which case every block MUST BE named "render_config"
      {
        render_config
        {
          scene:s = "path_to_scene1.xml" 
          output_file:s = "saves/output1.png"
        }
        render_config
        {
          scene:s = "path_to_scene2.xml" 
          output_file:s = "saves/output2.png"
        }
      })"""";

      if (std::string(argv[2]) == "-h" || std::string(argv[2]) == "--help")
      {
        printf("%s\n", render_help);
        return 0;
      }

      Block base_config;
      std::vector<const Block *> configs;

      if (argv[2][0] == '{')
        load_block_from_string(argv[2], base_config);
      else
        load_block_from_file(argv[2], base_config);      

      for (int i=0; i<base_config.size(); i++)
      {
        if (base_config.get_name(i) == "render_config" && base_config.get_type(i) == Block::ValueType::BLOCK)
          configs.push_back(base_config.get_block(i));
      }

      if (configs.size() == 0)
        configs.push_back(&base_config);

      for (int config_n=0; config_n<configs.size(); config_n++)
      {
        printf("[render_app] Render config %d/%d\n", config_n+1, (int)configs.size());
        int result = (int)render_offscreen(configs[config_n]);
        if (result != 0)
          printf("ERROR: Render failed with code %d\n", result);
        const Block *config = configs[config_n];
      }
      
#ifdef USE_GPU
      vk_utils::globalContextDestroy();
#endif
      return 0;
    }
    else if (std::string(argv[1]) == "-scom2_png_jpeg_comparison")
    {
      scom2::scom2_png_jpeg_comparison(argv[2]);
      return 0;
    }
    else if (std::string(argv[1]) == "-voxelize" && argc > 2)
    {
      const char *voxelize_help = R""""(
Usage:
  ./render_app -voxelize <mesh> [resolution] [options]
  ./render_app -voxelize <mesh> --resolution <resolution> [options]

Options:
  --surface             Build only voxels intersecting the mesh surface (default)
  --solid, --fill       Fill the mesh interior; falls back to surface mode if not watertight
  --resolution <int>    Requested resolution, rounded up to a power of two
  --leaf-grid-size <int>
                        Fast voxelizer local raster grid per top-tree leaf (default: 2)
  --padding <int>       Empty voxel padding for the fast voxelizer (default: surface 1, solid 0)
  --cut-plane-x <float> Save preview with the x-positive side kept, usually 0 for center cut
  --cut-plane-y <float> Save preview with the y-positive side kept, usually 0 for center cut
  --cut-plane-z <float> Save preview with the z-positive side kept, usually 0 for center cut
  --cut-plane-neg-x <float>
                        Save preview with the x-negative side kept
  --cut-plane-neg-y <float>
                        Save preview with the y-negative side kept
  --cut-plane-neg-z <float>
                        Save preview with the z-negative side kept
  --side-view           Save preview from a 90-degree side camera
  --out <directory>     Output directory (default: saves/voxelized)
  --no-preview          Save the SVO and XML without rendering a PNG preview
  -h, --help            Show this help

Examples:
  ./render_app -voxelize models/bunny.obj 512 --surface
  ./render_app -voxelize models/bunny.obj --resolution 1024 --solid
)"""";

      if (std::string(argv[2]) == "-h" || std::string(argv[2]) == "--help")
      {
        printf("%s\n", voxelize_help);
        return 0;
      }

      const char *mesh_path = argv[2];
      int res = 128;
      int leaf_grid_size = 2;
      int padding = -1;
      bool solid = false;
      bool render_preview = true;
      bool side_view = false;
      Plane preview_cut_plane = create_disabled_plane();
      std::string preview_cut_suffix;
      std::string preview_view_suffix;
      std::string out_dir_str;

      int option_start = 3;
      if (argc > 3 && std::string(argv[3]).rfind("--", 0) != 0)
      {
        try
        {
          res = std::stoi(argv[3]);
        }
        catch (...)
        {
          printf("ERROR: invalid voxelization resolution '%s'\n", argv[3]);
          return 1;
        }
        option_start = 4;
      }

      for (int i = option_start; i < argc; ++i)
      {
        const std::string option = argv[i];
        auto require_value = [&](const char *name) -> const char *
        {
          if (i + 1 >= argc)
          {
            printf("ERROR: %s requires a value\n", name);
            return nullptr;
          }
          return argv[++i];
        };

        try
        {
          if (option == "--fill" || option == "--solid")
            solid = true;
          else if (option == "--surface")
            solid = false;
          else if (option == "--resolution")
          {
            const char *value = require_value("--resolution");
            if (value == nullptr)
              return 1;
            res = std::stoi(value);
          }
          else if (option == "--padding")
          {
            const char *value = require_value("--padding");
            if (value == nullptr)
              return 1;
            padding = std::stoi(value);
          }
          else if (option == "--leaf-grid-size" || option == "--leaf-grid")
          {
            const char *value = require_value(option.c_str());
            if (value == nullptr)
              return 1;
            leaf_grid_size = std::stoi(value);
          }
          else if (option == "--cut-plane-x" || option == "--cut-plane-y" || option == "--cut-plane-z" ||
                   option == "--cut-plane-neg-x" || option == "--cut-plane-neg-y" || option == "--cut-plane-neg-z")
          {
            const char *value = require_value(option.c_str());
            if (value == nullptr)
              return 1;
            const float offset = std::stof(value);
            const bool negative_side = option.find("-neg-") != std::string::npos;
            const char axis = option.back();
            const float sign = negative_side ? -1.0f : 1.0f;
            preview_cut_plane.is_active = 1;
            preview_cut_plane.pos = LiteMath::float3(0.0f, 0.0f, 0.0f);
            preview_cut_plane.normal = LiteMath::float3(axis == 'x' ? sign : 0.0f,
                                                        axis == 'y' ? sign : 0.0f,
                                                        axis == 'z' ? sign : 0.0f);
            preview_cut_plane.pos = LiteMath::float3(axis == 'x' ? offset : 0.0f,
                                                     axis == 'y' ? offset : 0.0f,
                                                     axis == 'z' ? offset : 0.0f);
            preview_cut_suffix = std::string("_cut_") + (negative_side ? "neg_" : "") + axis + "_" + value;
          }
          else if (option == "--out")
          {
            const char *value = require_value("--out");
            if (value == nullptr)
              return 1;
            out_dir_str = value;
          }
          else if (option == "--no-preview")
            render_preview = false;
          else if (option == "--side-view")
          {
            side_view = true;
            preview_view_suffix = "_side";
          }
          else if (option == "-h" || option == "--help")
          {
            printf("%s\n", voxelize_help);
            return 0;
          }
          else
          {
            printf("ERROR: unknown voxelization option '%s'\n%s\n", option.c_str(), voxelize_help);
            return 1;
          }
        }
        catch (...)
        {
          printf("ERROR: invalid value for voxelization option '%s'\n", option.c_str());
          return 1;
        }
      }

      if (res < 2 || res > (1 << 21))
      {
        printf("ERROR: voxelization resolution must be between 2 and 2097152\n");
        return 1;
      }
      if (padding < -1)
      {
        printf("ERROR: --padding must be non-negative\n");
        return 1;
      }
      if (leaf_grid_size < 1)
      {
        printf("ERROR: --leaf-grid-size must be positive\n");
        return 1;
      }
      if (!std::filesystem::exists(mesh_path))
      {
        printf("ERROR: mesh file %s not found\n", mesh_path);
        return 1;
      }

      printf("[voxelize] Loading mesh %s\n", mesh_path);
      auto mesh = cmesh4::LoadMesh(mesh_path);
      if (mesh.TrianglesNum() == 0)
      {
        printf("ERROR: mesh '%s' contains no triangles\n", mesh_path);
        return 1;
      }

      Voxelizer::SparseVoxelOctreeBuildSettings settings;
      settings.resolution = static_cast<uint32_t>(res);
      settings.mode = solid ? Voxelizer::SparseVoxelizationMode::SOLID
                            : Voxelizer::SparseVoxelizationMode::SURFACE_ONLY;
      settings.leaf_grid_size = static_cast<uint32_t>(leaf_grid_size);
      settings.padding = padding >= 0 ? static_cast<uint32_t>(padding) : 0xFFFFFFFFu;
      settings.verbose = true;

      LiteMath::float3 min_p;
      LiteMath::float3 max_p;
      cmesh4::get_bbox(mesh, &min_p, &max_p);
      uint32_t output_resolution = 0;

      printf("[voxelize] Building %s SVO at requested resolution %d\n",
             solid ? "solid" : "surface", res);
      SparseVoxelOctree svo;
      try
      {
        svo = Voxelizer::build_sparse_voxel_octree_from_mesh(
            mesh, settings);
        output_resolution = svo.header.max_level_size;
      }
      catch (const std::bad_alloc &)
      {
        printf("ERROR: voxelization ran out of memory at resolution %d. "
               "Try a lower resolution.\n",
               res);
        return 1;
      }
      catch (const std::exception &e)
      {
        printf("ERROR: voxelization failed: %s\n", e.what());
        return 1;
      }

      std::filesystem::path out_dir = out_dir_str.empty() ? std::filesystem::path("saves/voxelized") : std::filesystem::path(out_dir_str);
      if (!std::filesystem::exists(out_dir))
        std::filesystem::create_directories(out_dir);
      std::string base = std::filesystem::path(mesh_path).stem().string();
      std::string bin_path = (out_dir / (base + "_" + std::to_string(output_resolution) + ".svo")).string();
      printf("[voxelize] Saving SVO to %s\n", bin_path.c_str());
      save_SVO(svo, bin_path);

      std::string xml_path = (out_dir / (base + "_" + std::to_string(output_resolution) + ".xml")).string();
      save_scene_xml(xml_path, bin_path, get_info_SVO(svo), 0, DemoScene::SINGLE_OBJECT);

      if (output_resolution >= 65536 && render_preview)
      {
        printf("[voxelize] Skipping preview: VoxelRenderer traversal currently supports at most depth 15\n");
        render_preview = false;
      }
      if (!render_preview)
        return 0;

      std::string out_image = (out_dir / (base + "_" + std::to_string(output_resolution) + preview_cut_suffix + preview_view_suffix + ".png")).string();
      auto voxel_renderer = CreateVoxelRenderer(DEVICE_CPU);
      voxel_renderer->LoadSVO(svo.data, svo.header.max_level_size);
      voxel_renderer->setCuttingPlane(preview_cut_plane);

      int img_width = 1024, img_height = 1024;
      voxel_renderer->SetViewport(0, 0, img_width, img_height);
      voxel_renderer->CommitDeviceData();
      voxel_renderer->Clear(img_width, img_height);

      LiteMath::float3 center = (min_p + max_p) * 0.5f;
      LiteMath::float3 extent = max_p - min_p;
      const float fov = 60.0f;
      const float radius = std::max(0.5f * LiteMath::length(extent), 0.01f);
      const float camera_distance =
          1.25f * radius / std::tan(0.5f * fov * LiteMath::M_PI / 180.0f);
      const LiteMath::float3 camera_direction =
          LiteMath::normalize(side_view ? LiteMath::float3(1.4f, 0.55f, 0.0f)
                                        : LiteMath::float3(1.0f, 0.55f, 1.4f));
      LiteMath::float3 cam_pos = center + camera_direction * camera_distance;
      auto worldView = LiteMath::lookAt(cam_pos, center, LiteMath::float3(0, 1, 0));
      float aspect = float(img_width) / img_height;
      const float z_near = std::max(0.001f, camera_distance - 2.0f * radius);
      const float z_far = std::max(z_near + 1.0f, camera_distance + 4.0f * radius);
      auto proj = LiteMath::perspectiveMatrix(fov, aspect, z_near, z_far);
      voxel_renderer->UpdateCamera(worldView, proj);
      voxel_renderer->UpdateMembersPlainData();

      LiteImage::Image2D<uint32_t> image(img_width, img_height);
      voxel_renderer->Render(image.data(), img_width, img_height, 1);

      printf("[voxelize] Saving render output to %s\n", out_image.c_str());
      LiteImage::SaveImage(out_image.c_str(), image);

      return 0;
    }
  }

  printf("ERROR: command line arguments are incorrect\n");
  return 1;
}
