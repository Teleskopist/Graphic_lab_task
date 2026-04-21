#include <vector>
#include <memory>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <filesystem>

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

      const char *info = R""""(
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
        printf("%s\n", info);
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
  }

  printf("ERROR: command line arguments are incorrect\n");
  return 1;
}
