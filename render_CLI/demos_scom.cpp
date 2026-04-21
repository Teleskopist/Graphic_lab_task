#include "demos_scom.h"
#include "sdf/scom2/scom_builder.h"
#include "MultiRenderer/MultiRenderer.h"
#include "sdf/simple/sdf_converter.h"
#include "utils/misc/image_metrics.h"
#include "utils/mesh/mesh_internal.h"
#include "sdf/build/scene_converter.h"
#include "SimplexNoise.h"
#include "VolumeRenderer/scene_extension_volume.h"
#include "VolumeRenderer/VolumeRenderer.h"
#include "utils/misc/camera.h"
#include "VolumeRenderer/sparse_octree_density_grid.h"
#include "utils/mesh/triangle_list_octree.h"
#include "sdf/build/sparse_octree_builder.h"
#include "HydraRenderer/renderer.h"
#include "utils/mesh/mesh_bvh.h"
#include "utils/common/position_hash.h"
#include "sdf/scom2/scom_utils.h"
#include "sdf/scom2/similarity_compression.h"
#include "sdf/build/sparse_octree_common.h"
#include "VoxelRenderer/voxel_dag.h"
#include "VoxelRenderer/VoxelRenderer.h"
#include "VoxelRenderer/cave_generator.h"
#include "VoxelRenderer/scene_extension_voxel.h"
#include "sdf/multi/sdf_multi.h"

#include "sdf/scom2/radiance_field/sh_compute.h"
#include "sdf/scom2/radiance_field/sh_rotate.h"
#include "sdf/scom2/radiance_field/sh_rotate_table.h"
#include "sdf/scom2/radiance_field/sh_clustering.h"
#include "sdf/scom2/radiance_field/radiance_field.h"
#include "sdf/scom2/radiance_field/sh_visualization.h"

void SCom_demo_by_name(const char *name)
{
  if (strcmp(name, "textured_SDF") == 0)
  {
    SCom_demo_1_textured_SDF();
  }
  else if (strcmp(name, "colored_SDF") == 0)
  {
    SCom_demo_2_colored_SDF();
  }
  else if (strcmp(name, "volume_render") == 0)
  {
    SCom_demo_3_volume_render();
  }
  else if (strcmp(name, "volume_render_4D") == 0)
  {
    SCom_demo_4_volume_render_4D();
  }
  else if (strcmp(name, "header_bunny") == 0)
  {
    SCom_demo_5_header_bunny();
  }
  else if (strcmp(name, "voxel_cave") == 0)
  {
    SCom_demo_6_voxel_cave();
  }
  else if (strcmp(name, "multi_octree") == 0)
  {
    SCom_demo_7_multi_octree();
  }
  else if (strcmp(name, "models_display") == 0)
  {
    SCom_demo_8_models_display();
  }
  else if (strcmp(name, "subgroups") == 0)
  {
    SCom_demo_9_subgroups();
  }
  else if (strcmp(name, "RF_bunny") == 0)
  {
    SCom_demo_10_RF_bunny();
  }
  else if (strcmp(name, "colored_bunny") == 0)
  {
    SCom_demo_11_colored_bunny();
  }
  else if (strcmp(name, "volume_4D_vs_slices") == 0)
  {
    SCom_demo_12_volume_4D_vs_slices();
  }
  else
  {
    printf("Error: Unknown demo name: %s\n", name);
  }
}

void render(
    LiteImage::Image2D<float4> &image,
    std::shared_ptr<MultiRenderer> renderer,
    MultiRenderPreset preset,
    float3 pos = float3(0, 0, 3),
    float3 target = float3(0, 0, 0),
    float3 up = float3(0, 1, 0),
    float fov_degrees = 60)
{
  float z_near = 0.1f;
  float z_far = 100.0f;
  float aspect = (float)image.width() / image.height();
  auto proj = LiteMath::perspectiveMatrix(fov_degrees, aspect, z_near, z_far);
  auto worldView = LiteMath::lookAt(pos, target, up);

  renderer->SetPreset(preset);
  renderer->RenderFloat(image.data(), image.width(), image.height(), worldView, proj, preset, 1);
}

void SCom_demo_1_textured_SDF()
{
  constexpr uint32_t WIDTH = 2048;
  constexpr uint32_t HEIGHT = 2048;

  const float3 pos    = float3(-1, 0, 2);
  const float3 target = float3( 0, 0, 0);
  const float3 up     = float3( 0, 1, 0);

  const std::vector<uint32_t> depths = {5,6,7,8,9};

  auto mesh = cmesh4::LoadMesh("../models/Bunny.obj");

  LiteImage::Image2D<float4> ref(WIDTH, HEIGHT);
  LiteImage::Image2D<float4> res(WIDTH, HEIGHT);

  MultiRenderPreset preset = getDefaultPreset();
  preset.render_mode = MULTI_RENDER_MODE_PHONG;
  //preset.normal_mode = NORMAL_MODE_VERTEX;
  preset.spp = 16;

  LiteImage::Image2D<float4> texture_raw = LiteImage::LoadImage<float4>("scenes/porcelain.jpg");
  LiteImage::Image2D<float4> texture = texture_raw;
  for (int y = 0; y < texture_raw.height(); y++)
  {
    for (int x = 0; x < texture_raw.width(); x++)
    {
      float4 val = texture_raw[uint2(x, y)];
      //val = float4(powf(val.x, 1.0f / 2.2f), powf(val.y, 1.0f / 2.2f), powf(val.z, 1.0f / 2.2f), 1);
      texture[uint2(x, texture_raw.height() - y - 1)] = val;
    }
  }

  if (!std::filesystem::exists("saves/demos/SCom_demo_1_textured_SDF"))
    std::filesystem::create_directories("saves/demos/SCom_demo_1_textured_SDF");

  {
    auto pRender = CreateMultiRenderer(DEVICE_GPU);
    pRender->SetPreset(preset);
    pRender->setBackgroundColor(float4(1, 1, 1, 1));
    pRender->SetViewport(0, 0, WIDTH, HEIGHT);

    uint32_t texId = pRender->AddTexture(texture);
    MultiRendererMaterial mat;
    mat.type = MULTI_RENDER_MATERIAL_TYPE_TEXTURED;
    mat.texId = texId;
    uint32_t matId = pRender->AddMaterial(mat);
    pRender->SetMaterial(matId, 0);

    pRender->SetScene(mesh);
    render(ref, pRender, preset, pos, target, up);
    LiteImage::SaveImage<float4>("saves/demos/SCom_demo_1_textured_SDF/ref.png", ref);
  }

  for (auto depth : depths)
  {
    auto settings = SparseOctreeSettings(depth);
    settings.calculate_mat_id = true;
    settings.calculate_tc = true;
    auto sdf = sdf_converter::create_sdf_frame_octree_tex(settings, mesh);

    auto pRender = CreateMultiRenderer(DEVICE_GPU);
    pRender->SetPreset(preset);
    pRender->setBackgroundColor(float4(1, 1, 1, 1));
    pRender->SetViewport(0, 0, WIDTH, HEIGHT);

    uint32_t texId = pRender->AddTexture(texture);
    MultiRendererMaterial mat;
    mat.type = MULTI_RENDER_MATERIAL_TYPE_TEXTURED;
    mat.texId = texId;
    uint32_t matId = pRender->AddMaterial(mat);
    pRender->SetMaterial(matId, 0);

    pRender->SetScene(sdf);
    render(res, pRender, preset, pos, target, up);

    std::string path = "saves/demos/SCom_demo_1_textured_SDF/sdf_" + std::to_string(depth) + ".png";
    LiteImage::SaveImage<float4>(path.c_str(), res);

    float size = sdf.size() * sizeof(sdf[0]);
    float psnr = image_metrics::PSNR(ref, res);
    printf("SDF depth: %d, size %.1f MB, PSNR: %f\n", depth, size/1024.0f/1024.0f, psnr);
  }
}

void SCom_demo_2_colored_SDF()
{
  constexpr uint32_t WIDTH = 2048;
  constexpr uint32_t HEIGHT = 2048;

  const float3 pos    = float3(-0.563442, -0.303976, 0.558173);
  const float3 target = float3(26.966030, -55.643200, -78.052887);
  const float3 up     = float3(0.182904, 0.832921, -0.522292);
  const float fov_degrees = 60;

  MultiRenderPreset preset = getDefaultPreset();
  preset.render_mode = MULTI_RENDER_MODE_PHONG;
  preset.spp = 16;

  LiteImage::Image2D<float4> ref(WIDTH, HEIGHT);
  LiteImage::Image2D<float4> res(WIDTH, HEIGHT);

  if (!std::filesystem::exists("saves/demos/SCom_demo_2_colored_SDF"))
    std::filesystem::create_directories("saves/demos/SCom_demo_2_colored_SDF");

  int code = std::system("./engine config/sg26_demos/SCom_demo_2_colored_SDF.blk");
  if (code == 0)
  {
    printf("[SCom_demo_2_colored_SDF] Scene created\n");
  }
  else
  {
    printf("[SCom_demo_2_colored_SDF] ERROR: scene creation failed\n");
    return;
  }

  // {
  //   auto full_mesh = cmesh4::MergeMeshes(meshes);
  //   auto pRender = CreateMultiRenderer(DEVICE_GPU);
  //   pRender->SetPreset(preset);
  //   pRender->SetViewport(0, 0, WIDTH, HEIGHT);
  //   pRender->SetScene(full_mesh);
  //   render(ref, pRender, preset, pos, target, up);
  //   LiteImage::SaveImage<float4>("saves/demos/SCom_demo_2_colored_SDF/ref.png", ref);
  // }

  size_t size = 0;
  {
    auto pRender = CreateMultiRenderer(DEVICE_GPU);
    pRender->SetPreset(preset);
    pRender->setBackgroundColor(float4(1, 1, 1, 1));
    pRender->SetViewport(0, 0, WIDTH, HEIGHT);
    pRender->LoadScene("saves/demos/SCom_demo_2_colored_SDF/scene.xml");
    render(res, pRender, preset, pos, target, up, fov_degrees);
    LiteImage::SaveImage<float4>("saves/demos/SCom_demo_2_colored_SDF/res.png", res);
    size = pRender->getSceneSize();
  }
  
  printf("SCom size %.1f MB\n", size/1024.0f/1024.0f);
}

void create_volume_grid_perlin(uint32_t sz, float *grid, float cutoff)
{
  SimplexNoise noiseGen = SimplexNoise();
  for (int i = 0; i < sz; i++)
  {
    for (int j = 0; j < sz; j++)
    {
      for (int k = 0; k < sz; k++)
      {
        float3 p = 2.0f * (float3(i, j, k) / float(sz - 1)) - 1.0f;
        float3 c0 = float3(i, j, k) / float(sz - 1);
        float3 color = (1.0f - c0) * float3(1, 0, 0) + c0 * float3(0, 1, 0);
        float noise = noiseGen.noise(p.x, p.y, p.z) +
                      noiseGen.noise(2.0f * p.y, 2.0f * p.x, 2.0f * p.z) +
                      noiseGen.noise(4.0f * p.z, 4.0f * p.x, 4.0f * p.y) +
                      noiseGen.noise(8.0f * p.x, 8.0f * p.y, 8.0f * p.z) +
                      noiseGen.noise(16.0f * p.y, 16.0f * p.z, 16.0f * p.x);
        noise = std::max(noise - cutoff, 0.0f);
        grid[i * sz * sz + j * sz + k] = 25.0f * std::max(noise * noise * (1 - length(p)), 0.0f);
      }
    }
  }
}

void SCom_demo_3_volume_render()
{
  const uint32_t sz = 512;
  constexpr uint32_t WIDTH = 4096;
  constexpr uint32_t HEIGHT = 4096;

  const float3 pos = float3(-1, 0, 2);
  const float3 target = float3(0, 0, 0);
  const float3 up = float3(0, 1, 0);

  if (!std::filesystem::exists("saves/demos/SCom_demo_3_volume_render"))
    std::filesystem::create_directories("saves/demos/SCom_demo_3_volume_render");
  
  std::vector<float> grid(sz * sz * sz);
  create_volume_grid_perlin(sz, grid.data(), 0.4f);
  
  DensityGrid c_grid;
  c_grid.size = uint3(sz, sz, sz);
  c_grid.data = grid;

  {
    LiteScene::HydraScene scene;
    auto *geom = new LiteScene::DensityGridGeometry();
    uint32_t geomId = scene.add_geometry(geom);
    geom->init(std::move(c_grid));
    scene.add_instance(geomId, float4x4());
    scene.save("saves/demos/SCom_demo_3_volume_render/scene.xml", "saves/demos/SCom_demo_3_volume_render");
  }

  LiteImage::Image2D<float4> ref(WIDTH, HEIGHT);
  LiteImage::Image2D<float4> res(WIDTH, HEIGHT);

  Camera camera;
  camera.pos = float3(2, 0, 2);
  camera.lookAt = float3(0, 0, 0);
  camera.up = float3(0, 1, 0);
  auto proj = projectionMatrix(camera.fovy, (float)WIDTH / (float)HEIGHT, camera.z_near, camera.z_far);
  auto view = LiteMath::lookAt(camera.pos, camera.lookAt, camera.up);

  VolumeRenderPreset preset = getDefaultVolumeRenderPreset();
  preset.alpha_thr = 0.001f;
  preset.use_lighting = 1;
  preset.voxel_traversal_steps = 1;
  preset.spp = 1;
  preset.traversal_mode = VOLUME_TRAVERSAL_MODE_VRT_SS;

  {
    std::shared_ptr<VolumeRenderer> pRender = CreateVolumeRenderer(DEVICE_GPU_COMP);
    pRender->SetPreset(preset);
    pRender->SetViewport(0, 0, WIDTH, HEIGHT);
    pRender->SetBaseColor(float4(0.5, 0.5, 0.5, 1));
    pRender->setBackgroundColor(float4(0.9,0.9,1,1));
    pRender->LoadScene("saves/demos/SCom_demo_3_volume_render/scene.xml");
    pRender->UpdateCamera(view, proj);
    pRender->CommitDeviceData();
    pRender->Clear(WIDTH, HEIGHT);
    pRender->RenderFloat(ref.data(), WIDTH, HEIGHT, 1);
    LiteImage::SaveImage<float4>("saves/demos/SCom_demo_3_volume_render/ref.png", ref);
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  SparseOctreeSettings settings(8, 0.001f, 3);
  sdf_converter::GlobalOctree octree;
  sdf_converter::density_grid_to_global_octree(settings, c_grid, octree, nullptr);

  auto t2 = std::chrono::high_resolution_clock::now();
  scom2::Settings scom_settings = scom2::Settings();
  scom_settings.embedding_type = scom2::EmbeddingType::TRIVIAL;
  scom_settings.internal_brick_use = scom2::InternalBrickUse::PROPER_CLUSTERING;
  scom_settings.branch_descriptor = scom2::BranchDescriptor::SIMPLE_MERGE;
  scom_settings.bricks_use_similarity_compression = true;
  scom_settings.bricks_similarity_threshold = 1.5f;
  scom_settings.branches_use_similarity_compression = true;
  scom_settings.branches_similarity_compression_levels = 4;
  scom_settings.branches_similarity_threshold = 1.5f;
  scom_settings.branches_threshold_reduction_factor = 0.25f;
  scom_settings.bits_per_value = 16;

  scom_settings.support_multi_nodes = false;
  scom_settings.support_channels = false;
  scom_settings.support_surfaces = true;
  scom_settings.custom_max_val = true;
  scom_settings.bricks_use_average_val_transform = false;
  scom_settings.use_close_match_heuristic = true;

  SdfDAG dag;
  SCom2Tree scom_tree;
  sdf_converter::global_octree_to_DAG(octree, dag, scom_settings);

  auto t3 = std::chrono::high_resolution_clock::now();
  scom2::pack_SCom2(scom_settings, dag, scom_tree);
  
  auto t4 = std::chrono::high_resolution_clock::now();
  float time1 = std::chrono::duration<float, std::milli>(t2 - t1).count();
  float time2 = std::chrono::duration<float, std::milli>(t3 - t2).count();
  float time3 = std::chrono::duration<float, std::milli>(t4 - t3).count();
  printf("octree_to_DAG: %.1f ms\n", time1);
  printf("compress: %.1f ms\n", time2);
  printf("pack_SCom2: %.1f ms\n", time3);

  {
    LiteScene::HydraScene scene;
    auto *geom = new LiteScene::SCom2Geometry();
    uint32_t geomId = scene.add_geometry(geom);
    geom->init(std::move(scom_tree));
    scene.add_instance(geomId, float4x4());
    scene.save("saves/demos/SCom_demo_3_volume_render/scene_scom.xml", "saves/demos/SCom_demo_3_volume_render");
  }

  {
    std::shared_ptr<VolumeRenderer> pRender = CreateVolumeRenderer(DEVICE_GPU);
    pRender->SetPreset(preset);
    pRender->SetViewport(0, 0, WIDTH, HEIGHT);
    pRender->SetBaseColor(float4(0.5, 0.5, 0.5, 1));
    pRender->setBackgroundColor(float4(0.9,0.9,1,1));
    //pRender->LoadDAG(dag);
    pRender->LoadScene("saves/demos/SCom_demo_3_volume_render/scene_scom.xml");
    pRender->UpdateCamera(view, proj);
    pRender->CommitDeviceData();
    pRender->Clear(WIDTH, HEIGHT);
    pRender->RenderFloat(res.data(), WIDTH, HEIGHT, 1);
    //float3 dir = normalize(float3(-1,0,-1));
    //auto hit = pRender->RayQuery_NearestHit(float4(2,0,2,0.001f), float4(-0.687201, 0.149057, -0.637114, 1000));
    //printf("color %f %f %f %f\n", hit.coords[0], hit.coords[1], hit.coords[2], hit.coords[3]);
    LiteImage::SaveImage<float4>("saves/demos/SCom_demo_3_volume_render/res.png", res);
  }

  float psnr = image_metrics::PSNR(ref, res);
  printf("PSNR: %f\n", psnr);
}

// requires > 4 GB of VRAM
// the simulation must be computed first 
// (e.g. experiment_smoke_175() in fluid_simulation/main.cpp)
// ffmpeg is required to create a video
void SCom_demo_4_volume_render_4D()
{
  constexpr uint32_t WIDTH = 1920;
  constexpr uint32_t HEIGHT = 1080;

  constexpr uint32_t frames = 600;

  LiteImage::Image2D<float4> res(WIDTH, HEIGHT);

  Camera camera;
  camera.pos = float3(-0.903829, -0.102390, 1.412256);
  camera.lookAt = float3(61.210796, -6.729793, -76.676476);
  camera.up = float3(0.041256, 0.997802, -0.051867);
  camera.fovy = 45.0f;
  auto proj = projectionMatrix(camera.fovy, (float)WIDTH / (float)HEIGHT, camera.z_near, camera.z_far);
  auto view = LiteMath::lookAt(camera.pos, camera.lookAt, camera.up);

  VolumeRenderPreset preset = getDefaultVolumeRenderPreset();
  preset.alpha_thr = 0.01f;
  preset.use_lighting = 1;
  preset.voxel_traversal_steps = 1;
  preset.spp = 1;
  preset.animation_speed = 1.0f;
  preset.traversal_mode = VOLUME_TRAVERSAL_MODE_VRT_SS;

  if (!std::filesystem::exists("saves/demos/SCom_demo_4_volume_render_4D"))
    std::filesystem::create_directories("saves/demos/SCom_demo_4_volume_render_4D");

  DensityMultiGrid grid;
  load_density_grid(grid, "side_projects/fluid_simulation/saves/multi_grid_smoke_175.bin");

  //if (false)
  {
    auto t1 = std::chrono::high_resolution_clock::now();
    SparseOctreeSettings settings(7, 0.001f, 3);
    sdf_converter::GlobalOctree octree;
    sdf_converter::density_grid_to_global_octree(settings, grid, octree, nullptr);

    scom2::Settings scom_settings = scom2::Settings();
    scom_settings.embedding_type = scom2::EmbeddingType::TRIVIAL;
    scom_settings.internal_brick_use = scom2::InternalBrickUse::PROPER_CLUSTERING;
    scom_settings.branch_descriptor = scom2::BranchDescriptor::SIMPLE_MERGE;
    scom_settings.packed_reference_type = scom2::PackedReferenceType::LONG_9_23_32;
    scom_settings.bricks_use_similarity_compression = true;
    scom_settings.bricks_similarity_threshold = 0.75f;
    scom_settings.branches_use_similarity_compression = true;
    scom_settings.branches_similarity_compression_levels = 3;
    scom_settings.branches_similarity_threshold = 0.4f;
    scom_settings.branches_threshold_reduction_factor = 0.125f;
    scom_settings.bits_per_value = 16;

    scom_settings.support_multi_nodes = false;
    scom_settings.support_channels = false;
    scom_settings.support_surfaces = true;
    scom_settings.custom_max_val = true;
    scom_settings.bricks_use_average_val_transform = false;
    scom_settings.use_close_match_heuristic = true;

    SdfDAG dag;
    SCom2Tree scom_tree;
    auto t2 = std::chrono::high_resolution_clock::now();
    sdf_converter::global_octree_to_DAG(octree, dag, scom_settings);
    
    auto t3 = std::chrono::high_resolution_clock::now();
    scom2::pack_SCom2(scom_settings, dag, scom_tree);

    auto t4 = std::chrono::high_resolution_clock::now();
    float time1 = std::chrono::duration<float, std::milli>(t2 - t1).count();
    float time2 = std::chrono::duration<float, std::milli>(t3 - t2).count();
    float time3 = std::chrono::duration<float, std::milli>(t4 - t3).count();
    printf("octree_to_DAG: %.1f ms\n", time1);
    printf("compress: %.1f ms\n", time2);
    printf("pack_SCom2: %.1f ms\n", time3);

    {
      LiteScene::HydraScene scene;
      auto *geom = new LiteScene::SCom2Geometry();
      uint32_t geomId = scene.add_geometry(geom);
      geom->init(std::move(scom_tree));
      scene.add_instance(geomId, float4x4());
      scene.save("saves/demos/SCom_demo_4_volume_render_4D/scene_scom.xml", "saves/demos/SCom_demo_4_volume_render_4D");
    }
  }

  //render original grid
  {
    std::string frames_folder = "saves/demos/SCom_demo_4_volume_render_4D/grid_frames";
    if (!std::filesystem::exists(frames_folder))
      std::filesystem::create_directories(frames_folder);

    std::shared_ptr<VolumeRenderer> pRender = CreateVolumeRenderer(DEVICE_GPU_COMP);
    pRender->SetPreset(preset);
    pRender->SetViewport(0, 0, WIDTH, HEIGHT);
    pRender->SetBaseColor(float4(0.75, 0.75, 0.75, 1));
    pRender->setBackgroundColor(float4(0.45, 0.65, 0.90, 1.0));
    pRender->LoadGrid(grid.size.x, grid.data.data(), grid.timestamps);
    pRender->UpdateCamera(view, proj);
    pRender->CommitDeviceData();
    pRender->Clear(WIDTH, HEIGHT);

    float t_range = grid.timestamps.back() - grid.timestamps.front();
    float t_step = t_range / frames;

    for (int i = 0; i < frames; i++)
    {
      printf("Frame %d/%d\n", i, frames);
      pRender->nextFrame(t_step);
      pRender->UpdateMembersPlainData();
      pRender->RenderFloat(res.data(), WIDTH, HEIGHT, 1);

      char filename[1024];
      snprintf(filename, 1024, "%s/frame_%04d.png", frames_folder.c_str(),i);
      LiteImage::SaveImage<float4>(filename, res);
    }

    char command[1024];
    snprintf(command, 1024, "ffmpeg -framerate 30 -pattern_type glob -i '%s/frame_*.png' -c:v libx264 -pix_fmt yuv420p %s", 
            frames_folder.c_str(), "saves/demos/SCom_demo_4_volume_render_4D/demo_grid.mp4");
    system(command);
  }

  //render scom
  {
    std::string frames_folder = "saves/demos/SCom_demo_4_volume_render_4D/scom_frames";
    if (!std::filesystem::exists(frames_folder))
      std::filesystem::create_directories(frames_folder);

    std::shared_ptr<VolumeRenderer> pRender = CreateVolumeRenderer(DEVICE_GPU_COMP);
    pRender->SetPreset(preset);
    pRender->SetViewport(0, 0, WIDTH, HEIGHT);
    pRender->SetBaseColor(float4(0.75, 0.75, 0.75, 1));
    pRender->setBackgroundColor(float4(0.45, 0.65, 0.90, 1.0));
    pRender->LoadScene("saves/demos/SCom_demo_4_volume_render_4D/scene_scom.xml");
    pRender->UpdateCamera(view, proj);
    pRender->CommitDeviceData();
    pRender->Clear(WIDTH, HEIGHT);

    float t_range = 1.0f;
    float t_step = t_range / frames;

    for (int i = 0; i < frames; i++)
    {
      printf("Frame %d/%d\n", i, frames);
      pRender->nextFrame(t_step);
      pRender->UpdateMembersPlainData();
      pRender->RenderFloat(res.data(), WIDTH, HEIGHT, 1);

      char filename[1024];
      snprintf(filename, 1024, "%s/frame_%04d.png", frames_folder.c_str(),i);
      LiteImage::SaveImage<float4>(filename, res);
    }

    char command[1024];
    snprintf(command, 1024, "ffmpeg -framerate 30 -pattern_type glob -i '%s/frame_*.png' -c:v libx264 -pix_fmt yuv420p %s", 
            frames_folder.c_str(), "saves/demos/SCom_demo_4_volume_render_4D/demo_scom.mp4");
    system(command);
  }
}

void create_volume_grid_perlin_sdf(uint32_t sz, float *grid, sdf_converter::MultithreadedDistanceFunction sdf, float cutoff)
{
  SimplexNoise noiseGen = SimplexNoise();
  #pragma omp parallel for
  for (int i = 0; i < sz; i++)
  {
    uint32_t thread_id = omp_get_thread_num();
    for (int j = 0; j < sz; j++)
    {
      for (int k = 0; k < sz; k++)
      {
        float3 p = 2.0f * (float3(i, j, k) / float(sz - 1)) - 1.0f;
        float3 c0 = float3(i, j, k) / float(sz - 1);
        float3 color = (1.0f - c0) * float3(1, 0, 0) + c0 * float3(0, 1, 0);
        float noise = noiseGen.noise( 4.0f * p.x,  4.0f * p.y,  4.0f * p.z) +
                      noiseGen.noise( 8.0f * p.x,  8.0f * p.y,  8.0f * p.z) +
                      noiseGen.noise(16.0f * p.y, 16.0f * p.z, 16.0f * p.x) +
                      noiseGen.noise(32.0f * p.y, 32.0f * p.z, 32.0f * p.x);
        noise = std::max(noise - cutoff, 0.0f);
        float dist = std::min(1.0f, -10.0f*sdf(p,thread_id) + 0.1f);
        grid[i * sz * sz + j * sz + k] = 15.0f * std::max(noise * noise * dist, 0.0f);
      }
    }
  }
}

void SCom_demo_5_header_bunny()
{
  constexpr uint32_t WIDTH  = 2048;
  constexpr uint32_t HEIGHT = 2048;
  constexpr uint32_t DEPTH  = 7;

  const float3 pos    = float3(-1, -0.1, 2);
  const float3 target = float3( 0, -0.1, 0);
  const float3 up     = float3( 0, 1, 0);
    float z_near = 0.1f;
    float z_far = 100.0f;
    float aspect = (float)WIDTH / (float)HEIGHT;
    auto proj = LiteMath::perspectiveMatrix(60, aspect, z_near, z_far);
    auto worldView = LiteMath::lookAt(pos, target, up);

  auto mesh = cmesh4::LoadMesh("../models/Bunny.obj");
  cmesh4::rescale_mesh(mesh, float3(-0.75f), float3(0.75f));

  LiteImage::Image2D<float4> ref(WIDTH, HEIGHT);
  LiteImage::Image2D<float4> res(WIDTH, HEIGHT);

  MultiRenderPreset preset = getDefaultPreset();
  preset.render_mode = MULTI_RENDER_MODE_PHONG;
  preset.spp = 64;

  LiteImage::Image2D<float4> texture_raw = LiteImage::LoadImage<float4>("scenes/porcelain.jpg");
  LiteImage::Image2D<float4> texture = texture_raw;
  for (int y = 0; y < texture_raw.height(); y++)
  {
    for (int x = 0; x < texture_raw.width(); x++)
    {
      float4 val = texture_raw[uint2(x, y)];
      texture[uint2(x, texture_raw.height() - y - 1)] = val;
    }
  }

  if (!std::filesystem::exists("saves/demos/SCom_demo_5_header_bunny"))
    std::filesystem::create_directories("saves/demos/SCom_demo_5_header_bunny");

  {
    auto settings = SparseOctreeSettings(DEPTH);
    settings.calculate_mat_id = true;
    settings.calculate_tc = true;

    sdf_converter::GlobalOctree g;
    SdfDAG dag;
    SCom2Tree scom;
    auto plo = sdf_converter::create_triangle_list_octree(mesh, sdf_converter::PLOSettings(settings.depth));
    sdf_converter::mesh_octree_to_global_octree(settings, mesh, plo, g);
    sdf_converter::global_octree_to_DAG8_direct(g, dag);
    scom2::pack_SCom2(scom2::Settings(), dag, scom);

    save_scom2(scom, "saves/demos/SCom_demo_5_header_bunny/scom2_0.bin");
  }

  if (false)
  {
    HydraRenderPreset hp = getDefaultHydraRenderPreset();
    hp.spp = 128;

    LiteImage::Image2D<uint32_t> image(WIDTH, HEIGHT);

    std::shared_ptr<HydraRenderer> pRender = std::make_shared<HydraRenderer>(DEVICE_GPU);
    pRender->SetPreset(hp);
    pRender->LoadScene("saves/demos/SCom_demo_5_header_bunny/scene_scom.xml");

    preset.render_mode = MULTI_RENDER_MODE_LAMBERT_NO_TEX;
    preset.sdf_node_intersect = SDF_OCTREE_NODE_INTERSECT_BBOX;
    dynamic_cast<BVHRT*>(pRender->GetAccelStruct())->SetPreset(preset);
    pRender->SetViewport(0, 0, WIDTH, HEIGHT);
    pRender->UpdateCamera(worldView, proj);
    pRender->CommitDeviceData();
    pRender->Clear(WIDTH, HEIGHT);
    pRender->Render(image.data(), WIDTH, HEIGHT);
    LiteImage::SaveImage<uint32_t>("saves/demos/SCom_demo_5_header_bunny/voxel.png", image);

    preset.render_mode = MULTI_RENDER_MODE_LAMBERT_NO_TEX;
    preset.sdf_node_intersect = SDF_OCTREE_NODE_INTERSECT_IT;
    preset.compact_sdf_eps = 1e-6f;
    dynamic_cast<BVHRT*>(pRender->GetAccelStruct())->SetPreset(preset);
    pRender->Render(image.data(), WIDTH, HEIGHT);
    LiteImage::SaveImage<uint32_t>("saves/demos/SCom_demo_5_header_bunny/SDF.png", image);
  }

  if (false)
  {
    auto settings = SparseOctreeSettings(DEPTH);
    settings.calculate_mat_id = true;
    settings.calculate_tc = true;
    auto sdf = sdf_converter::create_sdf_frame_octree_tex(settings, mesh);

    auto pRender = CreateMultiRenderer(DEVICE_GPU);
    pRender->SetViewport(0, 0, WIDTH, HEIGHT);

    uint32_t texId = pRender->AddTexture(texture);
    MultiRendererMaterial mat;
    mat.type = MULTI_RENDER_MATERIAL_TYPE_TEXTURED;
    mat.texId = texId;
    uint32_t matId = pRender->AddMaterial(mat);
    pRender->SetMaterial(matId, 0);
    pRender->SetScene(sdf);
    pRender->setBackgroundColor(float4(0.5, 0.5, 1, 0));
    preset.render_mode = MULTI_RENDER_MODE_PHONG;
    preset.sdf_node_intersect = SDF_OCTREE_NODE_INTERSECT_ANALYTIC;
    pRender->SetPreset(preset);
    render(ref, pRender, preset, pos, target, up);
    LiteImage::SaveImage<float4>("saves/demos/SCom_demo_5_header_bunny/textured_SDF.png", ref);
  }

  // volume bunny
  if (false)
  {
    DensityGrid c_grid;
    sdf_converter::GlobalOctree octree;
    SdfDAG dag;
    SCom2Tree scom;

    const int max_threads = 32;
    const uint32_t sz = 512;

    std::vector<MeshBVH> bvh(max_threads);
    for (unsigned i = 0; i < max_threads; i++)
      bvh[i].init(mesh);
    auto sdf = [&](const float3 &p, unsigned idx) -> float 
    { return bvh[idx].get_signed_distance(p); };

    c_grid.size = uint3(sz, sz, sz);
    c_grid.data.resize(c_grid.size.x * c_grid.size.y * c_grid.size.z);
    create_volume_grid_perlin_sdf(sz, c_grid.data.data(), sdf, -1.05f);

  
    SparseOctreeSettings settings(DEPTH+1, 0.0001f, 3);
    sdf_converter::density_grid_to_global_octree(settings, c_grid, octree, nullptr);

    scom2::Settings scom_settings = scom2::Settings();
    scom_settings.internal_brick_use = scom2::InternalBrickUse::PROPER_CLUSTERING;
    scom_settings.support_multi_nodes = false;
    scom_settings.support_channels = false;
    scom_settings.support_surfaces = true;
    scom_settings.bits_per_value = 32;
    scom_settings.custom_max_val = true;

    sdf_converter::global_octree_to_DAG(octree, dag, scom_settings);

    scom2::pack_SCom2(scom_settings, dag, scom);

    VolumeRenderPreset preset = getDefaultVolumeRenderPreset();
    preset.alpha_thr = 0.001f;
    preset.use_lighting = 1;
    preset.voxel_traversal_steps = 1;
    preset.spp = 1;
    preset.traversal_mode = VOLUME_TRAVERSAL_MODE_VRT_SS;

    std::shared_ptr<VolumeRenderer> pRender = CreateVolumeRenderer(DEVICE_GPU);
    pRender->SetPreset(preset);
    pRender->SetViewport(0, 0, WIDTH, HEIGHT);
    pRender->SetBaseColor(float4(0.5, 0.5, 0.5, 1));
    pRender->setBackgroundColor(float4(0.5, 0.5, 1, 0));
    pRender->LoadSCom(scom);
    pRender->UpdateCamera(worldView, proj);
    pRender->CommitDeviceData();
    pRender->Clear(WIDTH, HEIGHT);
    pRender->RenderFloat(ref.data(), WIDTH, HEIGHT, 1);
    LiteImage::SaveImage<float4>("saves/demos/SCom_demo_5_header_bunny/volume.png", ref);
  }

  //CAE bunny
  {
    auto settings = SparseOctreeSettings(DEPTH);

    sdf_converter::GlobalOctree g;
    SdfDAG dag;
    SCom2Tree scom;
    auto plo = sdf_converter::create_triangle_list_octree(mesh, sdf_converter::PLOSettings(settings.depth));
    sdf_converter::mesh_octree_to_global_octree(settings, mesh, plo, g);
    sdf_converter::global_octree_to_DAG(g, dag, scom2::Settings());

    struct NormalElem
    {
      float3 norms[8] = {float3(0.0f)};
      uint8_t cnts[8] = {0};
      float c_diff = 0.0f;
    };

    CodeMap<NormalElem> normalElems;
    std::vector<uint4> codeByNodeId(dag.nodes.size());
    scom2::traverse_DAG(dag, [&](const SdfDAG &dag, uint32_t nodeId, uint32_t transformId, uint32_t level, uint4 code) -> bool 
    {
      codeByNodeId[nodeId] = code;

      float values[8];
      for (uint32_t i = 0; i < 8; i++)
        values[i] = dag.distances[dag.data_edges[dag.nodes[nodeId].data_edges_offset].data_offset + i];
      for (uint32_t i = 0; i < 8; i++)
      {
        uint4 p_code = code + uint4(i >> 2, (i >> 1) & 1, i & 1, 0);
        //float3 norm = normalize(sdf_converter::eval_dist_trilinear_diff(values, float3(i >> 2, (i >> 1) & 1, i & 1)));
        float3 norm = normalize(sdf_converter::eval_dist_trilinear_diff(values, float3(0.5, 0.5, 0.5)));
        normalElems[p_code].norms[i]+= norm;
        normalElems[p_code]. cnts[i]++;
      }

      return true;
    });

    for (auto &p : normalElems)
    {
      float3 sum = float3(0.0f);
      for (uint32_t i = 0; i < 8; i++)
      {
        //printf("norm[%d] = (%f %f %f)\n", i, p.second.norms[i].x, p.second.norms[i].y, p.second.norms[i].z);
        sum += p.second.norms[i];
      }

      sum = normalize(sum);
      //printf("sum = (%f %f %f)\n", sum.x, sum.y, sum.z);

      float c_diff = 0.0f;
      uint32_t cnt = 0;
      for (uint32_t i = 0; i < 8; i++)
      {
        if (p.second.cnts[i] > 0)
        {
          c_diff += (1.0f - dot(p.second.norms[i], sum));
          cnt++;
        }
      }

      p.second.c_diff = c_diff / (float)cnt;
      //printf("code %d %d %d %d cnt %d c_diff %f\n", p.first.x, p.first.y, p.first.z, p.first.w, cnt, p.second.c_diff);
    }

    dag.point_channels.emplace_back();
    auto &ch = dag.point_channels.back();
    ch.type = DataChannel::Type::FLOAT;
    ch.num_components = 1;
    ch.name = "diffs";
    ch.data_f.resize(8*dag.nodes.size());
    ch.min_val = 0.0f;
    ch.max_val = 0.1f;

    for (uint32_t nodeId = 0; nodeId < dag.nodes.size(); nodeId++)
    {
      for (uint32_t i = 0; i < 8; i++)
      {
        uint4 code = codeByNodeId[nodeId] + uint4(i >> 2, (i >> 1) & 1, i & 1, 0);
        auto it = normalElems.find(code);
        if (it != normalElems.end())
          ch.data_f[8*nodeId + i] = it->second.c_diff;
        else
          ch.data_f[8*nodeId + i] = 0.0f;
      }
    }
    //     float h = code.y/(float)code.w;
    //     //printf("code %d %d %d %d h %f\n", code.x, code.y, code.z, code.w, h);
    //     ch.data_f[8*nodeId + i] = h;
    //   }
    // }

    scom2::pack_SCom2(scom2::Settings(), dag, scom);

    preset.sdf_node_intersect = SDF_OCTREE_NODE_INTERSECT_ANALYTIC;
    preset.render_mode = MULTI_RENDER_MODE_CHANNEL;
    preset.channel_render_mode = CHANNEL_RENDER_MODE_CTF;
    preset.CTF_id = 1;
    preset.channel_id = 0;

    auto pRender = CreateMultiRenderer(DEVICE_GPU);
    pRender->SetViewport(0, 0, WIDTH, HEIGHT);
    pRender->SetScene(scom);
    pRender->setBackgroundColor(float4(0.5, 0.5, 1, 0));
    pRender->SetPreset(preset);
    render(ref, pRender, preset, pos, target, up);
    LiteImage::SaveImage<float4>("saves/demos/SCom_demo_5_header_bunny/CAE.png", ref);
  }
}

void SCom_demo_6_voxel_cave()
{
    constexpr uint32_t WIDTH = 2048;
    constexpr uint32_t HEIGHT = 2048;

    const uint2 size = uint2(1024,1024);
    const uint3 region_size = uint3(1024,1024,1024);
    const uint32_t max_height = 1024;
    
    CaveGeneratorSettings cave_settings;
    cave_settings.initial_fill_probability = 0.5f;
    cave_settings.border_thickness = 1;
    cave_settings.smoothing_iterations = 100;
    cave_settings.birth_threshold = 14;
    cave_settings.survival_threshold = 13;
    cave_settings.min_cave_size = 100000;

    float total_octree_convert_time_ms = 0.0f;
    uint3 regions_count = uint3(size.x/region_size.x, max_height/region_size.y, size.y/region_size.z);
    std::vector<SparseVoxelOctree> octrees(regions_count.x*regions_count.y*regions_count.z);
    std::vector<uint4> codes(regions_count.x*regions_count.y*regions_count.z);
    {
      int X = regions_count.x;
      int Y = regions_count.y;
      int Z = regions_count.z;

      std::vector<uint8_t>  grid_u8(region_size.x*region_size.y*region_size.z);
      std::vector<uint32_t> grid_u32(region_size.x*region_size.y*region_size.z);

      //#pragma omp parallel for collapse(3)
      for (int i = 0; i < X; i++)
      {
        for (int j = 0; j < Y; j++)
        {
          for (int k = 0; k < Z; k++)
          {
            int idx = i*regions_count.y*regions_count.z + j*regions_count.z + k;
            create_procedural_cave(cave_settings, region_size.x, region_size.y, region_size.z, idx, grid_u8.data());

            for (size_t l = 0; l<grid_u8.size(); l++)
              grid_u32[l] = grid_u8[l];

            auto t1 = std::chrono::high_resolution_clock::now();
            auto octreeA = voxel_grid_to_octree(grid_u32, region_size, true);
            auto t2 = std::chrono::high_resolution_clock::now();
            total_octree_convert_time_ms += std::chrono::duration<float, std::milli>(t2 - t1).count();

            octrees[idx] = octreeA;
            codes[idx] = uint4(i, j, k, std::max(regions_count.x, std::max(regions_count.y, regions_count.z)));
            printf("region (%d,%d,%d) converted\n", i,j,k);
          }
        }
      }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    auto octree = merge_voxel_octrees(octrees, codes);
    auto t2 = std::chrono::high_resolution_clock::now();
    total_octree_convert_time_ms += std::chrono::duration<float, std::milli>(t2 - t1).count();

    SdfDAG dag;
    sparse_voxel_octree_to_DAG(octree, dag);
    auto t3 = std::chrono::high_resolution_clock::now();
    total_octree_convert_time_ms += std::chrono::duration<float, std::milli>(t3 - t2).count();

    if (!std::filesystem::exists("saves/demos/SCom_demo_6_voxel_cave"))
      std::filesystem::create_directories("saves/demos/SCom_demo_6_voxel_cave");

    LiteScene::HydraScene scene;
    auto *geom = new LiteScene::SparseVoxelOctreeGeometry();
    uint32_t geomId = scene.add_geometry(geom);
    geom->init(std::move(octree));
    scene.add_instance(geomId, float4x4());
    scene.save("saves/demos/SCom_demo_6_voxel_cave/scene.xml", "saves/demos/SCom_demo_6_voxel_cave");

    LiteImage::Image2D<uint32_t> image_ref(WIDTH, HEIGHT);
    LiteImage::Image2D<uint32_t> image_res(WIDTH, HEIGHT);

    Camera camera;
    camera.pos = float3(2, 0, 2);
    camera.lookAt = float3(0, 0, 0);
    camera.up = float3(0, 1, 0);
    auto proj = projectionMatrix(camera.fovy, (float)WIDTH / (float)HEIGHT, camera.z_near, camera.z_far);
    auto view = LiteMath::lookAt(camera.pos, camera.lookAt, camera.up);

    VoxelRenderPreset preset = getDefaultVoxelRenderPreset();
    preset.spp = 16;
    preset.render_mode = VOXEL_RENDER_MODE_LAMBERT;

    {
      std::shared_ptr<VoxelRenderer> pRender = CreateVoxelRenderer(DEVICE_GPU_COMP);
      pRender->SetViewport(0, 0, WIDTH, HEIGHT);
      pRender->LoadSVO(octree.data, octree.header.max_level_size);
      pRender->UpdateCamera(view, proj);
      pRender->SetPreset(preset);
      pRender->CommitDeviceData();
      pRender->Clear(WIDTH, HEIGHT);
      pRender->Render(image_ref.data(), WIDTH, HEIGHT, 1);
      LiteImage::SaveImage<uint32_t>("saves/demos/SCom_demo_6_voxel_cave/ref.png", image_ref);
    }

    scom2::Settings scom_settings;
    scom_settings.search_algorithm = scom2::SearchAlgorithm::EXACT_HASH;
    scom_settings.embedding_type = scom2::EmbeddingType::NO_EMBEDDING;
    scom_settings.internal_brick_use = scom2::InternalBrickUse::MERGE_ALL;
    scom_settings.branch_descriptor = scom2::BranchDescriptor::SIMPLE_MERGE;
    scom_settings.bricks_use_similarity_compression = true;
    scom_settings.bricks_similarity_threshold = 1e-5f;
    scom_settings.channels_use_similarity_compression = true;
    scom_settings.channels_similarity_threshold = 1e-5f;
    scom_settings.branches_use_similarity_compression = true;
    scom_settings.branches_similarity_compression_levels = 4;
    scom_settings.branches_similarity_threshold = 1e-5f;

    scom_settings.support_surfaces = false;
    scom_settings.support_channels = true;
    scom_settings.support_multi_nodes = false;

    uint32_t nodes_before = dag.nodes.size();
    uint32_t channels_before = dag.voxel_channels[0].data_i.size();
    SCom2Tree scom;

    auto t4 = std::chrono::high_resolution_clock::now();
    scom2::DAG_similarity_compression(dag, scom_settings);

    auto t5 = std::chrono::high_resolution_clock::now();
    scom2::pack_SCom2(scom_settings, dag, scom);

    auto t6 = std::chrono::high_resolution_clock::now();
    const float time_scom = std::chrono::duration<float, std::milli>(t5 - t4).count();
    const float time_pack = std::chrono::duration<float, std::milli>(t6 - t5).count();
    printf("times: to octree: %f ms, to scom: %f ms, pack: %f ms\n", total_octree_convert_time_ms, time_scom, time_pack);

    uint32_t nodes_after = dag.nodes.size();
    uint32_t channels_after = dag.voxel_channels[0].data_i.size();
    {
      std::shared_ptr<VoxelRenderer> pRender = CreateVoxelRenderer(DEVICE_GPU_COMP);
      pRender->SetViewport(0, 0, WIDTH, HEIGHT);
      pRender->LoadSCom(scom);
      pRender->UpdateCamera(view, proj);
      pRender->SetPreset(preset);
      pRender->CommitDeviceData();
      pRender->Clear(WIDTH, HEIGHT);
      pRender->Render(image_res.data(), WIDTH, HEIGHT, 1);
      LiteImage::SaveImage<uint32_t>("saves/demos/SCom_demo_6_voxel_cave/res.png", image_res);
    }

    {
    LiteScene::HydraScene scene;
    auto *geom = new LiteScene::SCom2Geometry();
    uint32_t geomId = scene.add_geometry(geom);
    geom->init(std::move(scom));
    scene.add_instance(geomId, float4x4());
    scene.save("saves/demos/SCom_demo_6_voxel_cave/scene_scom.xml", "saves/demos/SCom_demo_6_voxel_cave");
    }
  }

  void SCom_demo_7_multi_octree()
  {
    constexpr uint32_t WIDTH = 2048;
    constexpr uint32_t HEIGHT = 2048;

    const float3 pos    = float3(-1, 0, 2);
    const float3 target = float3( 0, 0, 0);
    const float3 up     = float3( 0, 1, 0);

    LiteImage::Image2D<float4> res(WIDTH, HEIGHT);

    auto mesh = cmesh4::LoadMesh("scenes/01_simple_scenes/data/bunny.vsgf");
    auto mesh2 = cmesh4::LoadMesh("scenes/01_simple_scenes/data/sphere.vsgf");
    auto mesh3 = cmesh4::LoadMesh("scenes/01_simple_scenes/data/teapot.vsgf");
    cmesh4::transform_mesh(mesh2, LiteMath::scale4x4(float3(0.5f)));
    cmesh4::transform_mesh(mesh3, LiteMath::rotate4x4Y(LiteMath::M_PI/4) * LiteMath::scale4x4(float3(0.95f)));
    std::vector<cmesh4::SimpleMesh> meshes = {mesh, mesh2, mesh3};

    for (int i=0; i < (int)meshes.size(); i++)
    {
      meshes[i].matIndices = std::vector<uint32_t>(meshes[i].TrianglesNum(), i);
    }
    
    SparseOctreeSettings settings;
    settings.depth = 8;
    settings.calculate_mat_id = true;
    SdfMultiOctree mo = sdf_converter::create_sdf_multi_octree(settings, meshes, false);

    MultiRenderPreset preset = getDefaultPreset();
    preset.representation_mode = REPRESENTATION_MODE_SURFACE;
    //preset.render_mode = MULTI_RENDER_MODE_MATERIAL;
    preset.spp = 16;

    if (!std::filesystem::exists("saves/demos/SCom_demo_7_multi_octree"))
      std::filesystem::create_directories("saves/demos/SCom_demo_7_multi_octree");

    size_t size = 0;
    {
      auto pRender = CreateMultiRenderer(DEVICE_GPU);
      pRender->SetPreset(preset);
      pRender->setBackgroundColor(float4(1, 1, 1, 1));
      pRender->SetViewport(0, 0, WIDTH, HEIGHT);
      pRender->SetScene(mo);
      render(res, pRender, preset, pos, target, up);
      LiteImage::SaveImage<float4>("saves/demos/SCom_demo_7_multi_octree/res.png", res);
      size = pRender->getSceneSize();
    }
    
    printf("SCom size %.1f MB\n", size/1024.0f/1024.0f);
  }

void SCom_demo_8_models_display()
{
  //create hydra scene
  LiteScene::HydraScene scene;

  auto add_to_scene = [&](const std::string &path, float3 pos, float scale, float4x4 rotate = float4x4())
  {
    SCom2Tree scom;
    load_scom2(scom, path);

    auto *geom = new LiteScene::SCom2Geometry();
    uint32_t geomId = scene.add_geometry(geom);
    geom->init(std::move(scom));
    scene.add_instance(geomId, LiteMath::translate4x4(float3(pos.x, pos.y, pos.z)) * LiteMath::scale4x4(float3(scale)) * rotate);
  };

  constexpr uint32_t WIDTH = 3000;
  constexpr uint32_t HEIGHT = 1000;

  const float3 pos    = float3( 0, 2, 4.75);
  const float3 target = float3( -0.1, -0.5, 0);
  const float3 up     = float3( 0, 1, 0);
  const float fov_degrees = 50;

  LiteImage::Image2D<float4> res(WIDTH, HEIGHT);

  add_to_scene("saves2/scom_full/Buddha/build/SCOM2/scom_8_003/scom2_0.bin", float3(0,0,0), 1.5f, LiteMath::rotate4x4Y(LiteMath::M_PI));
  add_to_scene("saves2/scom_full/Bunny/build/SCOM2/scom_8_003/scom2_0.bin", float3(0,0,0), 0.5f);
  add_to_scene("saves2/scom_full/Dragon/build/SCOM2/scom_8_003/scom2_0.bin", float3(-2,0,0), 1.0f);
  add_to_scene("saves2/scom_full/Roadbike/build/SCOM2/scom_8_003/scom2_0.bin", float3( 2,0,0), 1.0f);
  add_to_scene("saves2/scom_full/Buggy/build/SCOM2/scom_9_002/scom2_0.bin", float3( 4.3,0,-1), 2.2f, LiteMath::rotate4x4X(-LiteMath::M_PI/2));
  add_to_scene("saves2/scom_full/Stone/build/SCOM2/scom_8_003/scom2_0.bin", float3(-4.3,-1,1.75), 0.7f, LiteMath::rotate4x4Y(-LiteMath::M_PI/2));
  add_to_scene("saves2/scom_full/BMWManifold/build/SCOM2/scom_8_003/scom2_0.bin", float3(-3,0,1), 0.8f, LiteMath::rotate4x4X(-LiteMath::M_PI/2));
  add_to_scene("saves2/scom_full/HumanSkeleton/build/SCOM2/scom_8_003/scom2_0.bin", float3(0.5,0,-0.2), 1.5f, LiteMath::rotate4x4X(-LiteMath::M_PI/2));
  add_to_scene("saves2/scom_full/MotorcycleCylinderHead/build/SCOM2/scom_8_003/scom2_0.bin", float3(-2,-0.5,2.2), 0.5f);

  add_to_scene("saves2/nesi_dataset/thingi32_75496-Marble_Bust_of_a_Priest_normalized_scaled_down/build/SCOM2/scom_7c/scom2_0.bin", 
               float3(-5,-0.5,1), 0.7f, LiteMath::rotate4x4X(-LiteMath::M_PI/2));
  add_to_scene("saves2/nesi_dataset/thingi32_75656-LoraShaeffer_FINAL_normalized_scaled_down/build/SCOM2/scom_7c/scom2_0.bin", 
               float3(-6.5,-0.5,-1), 0.7f, LiteMath::rotate4x4X(-LiteMath::M_PI/3));
  add_to_scene("saves2/nesi_dataset/thingi32_47984-Sapphos_Head_normalized_scaled_down/build/SCOM2/scom_7c/scom2_0.bin", 
               float3(-1,-0.5,1.2), 0.7f, LiteMath::rotate4x4X(-LiteMath::M_PI/2));
  add_to_scene("saves2/nesi_dataset/thingi32_64764-thinking_gargoyle_satue_normalized_scaled_down/build/SCOM2/scom_7c/scom2_0.bin", 
               float3(-6,-0.75,-3.5), 1.0f, LiteMath::rotate4x4X(-LiteMath::M_PI/2));
  add_to_scene("saves2/nesi_dataset/thingi32_354371-angels_multiscan_flat_normalized_scaled_down/build/SCOM2/scom_7c/scom2_0.bin", 
               float3(-5,-0.75,-7), 1.5f, LiteMath::rotate4x4X(-LiteMath::M_PI/2));
  add_to_scene("saves2/nesi_dataset/thingi32_96481-mask1_normalized_scaled_down/build/SCOM2/scom_7c/scom2_0.bin", 
               float3( 0,-0.5,2.2), 0.7f, LiteMath::rotate4x4Z(-LiteMath::M_PI/3));

  add_to_scene("saves2/nesi_dataset/thingi32_527631-holden-observatory_normalized_scaled_down/build/SCOM2/scom_7c/scom2_0.bin", 
               float3(10,-1.5, -10), 2.5f, LiteMath::rotate4x4X(-LiteMath::M_PI/2));

  MultiRenderPreset preset = getDefaultPreset();
  preset.render_mode = MULTI_RENDER_MODE_GEOM;
  preset.spp = 4;

  if (!std::filesystem::exists("saves/demos/SCom_demo_8_models_display"))
    std::filesystem::create_directories("saves/demos/SCom_demo_8_models_display");

  auto pRender = CreateMultiRenderer(DEVICE_GPU);
  pRender->SetPreset(preset);
  pRender->setBackgroundColor(float4(1, 1, 1, 1));
  pRender->SetViewport(0, 0, WIDTH, HEIGHT);
  pRender->LoadScene(scene);
  render(res, pRender, preset, pos, target, up, fov_degrees);
  LiteImage::SaveImage<float4>("saves/demos/SCom_demo_8_models_display/res.png", res);

  scene.save("saves/demos/SCom_demo_8_models_display/scene/scene.xml", "saves/demos/SCom_demo_8_models_display/scene");
}

void SCom_demo_9_subgroups()
{
  constexpr uint32_t WIDTH = 2048;
  constexpr uint32_t HEIGHT = 2048;

  const float3 pos    = float3(-1, 0, 2);
  const float3 target = float3( 0, 0, 0);
  const float3 up     = float3( 0, 1, 0);

  const uint32_t depth = 8;

  std::vector<scom2::TransformSubgroup> subgroups = {
    scom2::TransformSubgroup::NO_TRANSFORMS,
    scom2::TransformSubgroup::DEFAULT,
    scom2::TransformSubgroup::LARGE_DIAGONAL,
    scom2::TransformSubgroup::SIDE_SWAP,
    scom2::TransformSubgroup::ALL_TRANSPOSITIONS};

  auto mesh = cmesh4::LoadMesh("/home/sammael/Downloads/models_SCom/WaterPump.obj");

  LiteImage::Image2D<float4> ref(WIDTH, HEIGHT);
  LiteImage::Image2D<float4> res(WIDTH, HEIGHT);

  MultiRenderPreset preset = getDefaultPreset();
  preset.render_mode = MULTI_RENDER_MODE_LAMBERT_NO_TEX;
  preset.spp = 16;

  if (!std::filesystem::exists("saves/demos/SCom_demo_9_subgroups"))
    std::filesystem::create_directories("saves/demos/SCom_demo_9_subgroups");

  {
    auto pRender = CreateMultiRenderer(DEVICE_GPU);
    pRender->SetPreset(preset);
    pRender->setBackgroundColor(float4(1, 1, 1, 1));
    pRender->SetViewport(0, 0, WIDTH, HEIGHT);
    pRender->SetScene(mesh);
    render(ref, pRender, preset, pos, target, up);
    LiteImage::SaveImage<float4>("saves/demos/SCom_demo_9_subgroups/ref.png", ref);
  }

  for (auto subgroup : subgroups)
  {
    auto settings = SparseOctreeSettings(depth);

    scom2::Settings scom_settings;
    scom_settings.bricks_use_similarity_compression = true;
    scom_settings.bricks_similarity_threshold = 0.03f;
    scom_settings.branches_use_similarity_compression = false;
    scom_settings.branches_similarity_compression_levels = 2;
    scom_settings.branches_similarity_threshold = 0.03f;
    scom_settings.bricks_transform_subgroup = subgroup;
    scom_settings.branches_transform_subgroup = subgroup;
    if (subgroup == scom2::TransformSubgroup::NO_TRANSFORMS || subgroup == scom2::TransformSubgroup::DEFAULT)
      scom_settings.packed_reference_type = scom2::PackedReferenceType::SHORT_6_8_18;
    else
      scom_settings.packed_reference_type = scom2::PackedReferenceType::LONG_9_23_32;

    sdf_converter::GlobalOctree g;
    SdfDAG dag;
    SCom2Tree scom;
    auto plo = sdf_converter::create_triangle_list_octree(mesh, sdf_converter::PLOSettings(settings.depth));
    sdf_converter::mesh_octree_to_global_octree(settings, mesh, plo, g);
    sdf_converter::global_octree_to_DAG(g, dag, scom_settings);
    scom2::pack_SCom2(scom_settings, dag, scom);

    auto pRender = CreateMultiRenderer(DEVICE_GPU);
    pRender->SetPreset(preset);
    pRender->setBackgroundColor(float4(1, 1, 1, 1));
    pRender->SetViewport(0, 0, WIDTH, HEIGHT);
    pRender->SetScene(dag);
    render(res, pRender, preset, pos, target, up);

    std::string path = "saves/demos/SCom_demo_9_subgroups/sdf_" + std::to_string((int)subgroup) + ".png";
    LiteImage::SaveImage<float4>(path.c_str(), res);

    int nodes = dag.distances.size() / 8;
    float size = (scom.nodes.size() + scom.bricks.size())*sizeof(uint32_t);
    float psnr = image_metrics::PSNR(ref, res);
    printf("Subgroup: %d, nodes %d, size %.1f MB, PSNR: %f\n", depth, nodes, size/1024.0f/1024.0f, psnr);
  }
}

void SCom_demo_10_RF_bunny()
{
  if (!std::filesystem::exists("saves/demos/SCom_demo_10_RF_bunny"))
    std::filesystem::create_directories("saves/demos/SCom_demo_10_RF_bunny");

  RadianceField rf;
  sdf_converter::GlobalOctree go;
  SdfDAG dag;
  SCom2Tree scom;

  scom2::Settings scom_settings;
  scom_settings.internal_brick_use = scom2::InternalBrickUse::PROPER_CLUSTERING;
  scom_settings.branch_descriptor = scom2::BranchDescriptor::SIMPLE_MERGE;
  scom_settings.support_multi_nodes = false;
  scom_settings.support_channels = true;
  scom_settings.support_surfaces = true;
  scom_settings.custom_max_val = true;
  scom_settings.bits_per_value = 16;

  load_radiance_field(rf, "/home/sammael/models/radiance_field/bunny/model_bin.bin");
  radiance_field_to_global_octree(rf, go);
  sdf_converter::global_octree_to_DAG8_direct(go, dag);
  scom2::rotate_DAG(LiteMath::float3x3(-1, 0, 0, 0, -1, 0, 0, 0, 1), dag);
  dag.header.user_params[SCENE_EXTENT_USER_PARAM_ID] = rf.scene_extent;
  dag.header.user_params[SCENE_CENTER_X_USER_PARAM_ID] = rf.scene_center.x;
  dag.header.user_params[SCENE_CENTER_Y_USER_PARAM_ID] = rf.scene_center.y;
  dag.header.user_params[SCENE_CENTER_Z_USER_PARAM_ID] = rf.scene_center.z;

  //scom2::DAG_similarity_compression(dag, scom_settings);
  //scom2::rotate_DAG(LiteMath::float3x3(1, 0, 0, 0, -1, 0, 0, 0, 1), dag);
  scom2::pack_SCom2(scom_settings, dag, scom);

    {
      LiteScene::HydraScene scene;
      auto *geom = new LiteScene::SCom2Geometry();
      uint32_t geomId = scene.add_geometry(geom);
      geom->init(std::move(scom));
      scene.add_instance(geomId, float4x4());
      scene.save("saves/demos/SCom_demo_10_RF_bunny/scene.xml", "saves/demos/SCom_demo_10_RF_bunny");
    }


  constexpr uint32_t WIDTH = 512;
  constexpr uint32_t HEIGHT = 512;

  LiteImage::Image2D<float4> ref(WIDTH, HEIGHT);
  LiteImage::Image2D<float4> res(WIDTH, HEIGHT);

  Camera camera;
  camera.pos    = float3(1.902661, -0.726895, -1.429204);
  camera.lookAt = float3(-79.443542, 3.286336, 56.594147);
  camera.up     = float3(0.032672, 0.999194, -0.023305);
  camera.fovy   = 60.0f;
  auto proj = projectionMatrix(camera.fovy, (float)WIDTH / (float)HEIGHT, camera.z_near, camera.z_far);
  auto view = LiteMath::lookAt(camera.pos, camera.lookAt, camera.up);

  VolumeRenderPreset preset = getDefaultVolumeRenderPreset();
  preset.alpha_thr = 0.001f;
  preset.volume_type = VOLUME_TYPE_SVRASTER_RF;

  {
    std::shared_ptr<VolumeRenderer> pRender = CreateVolumeRenderer(DEVICE_GPU_COMP);
    pRender->SetPreset(preset);
    pRender->SetViewport(0, 0, WIDTH, HEIGHT);
    pRender->setBackgroundColor(float4(0.5, 0.5, 1,1));
    pRender->LoadSCom(scom);
    pRender->UpdateCamera(view, proj);
    pRender->CommitDeviceData();
    pRender->Clear(WIDTH, HEIGHT);
    pRender->RenderFloat(ref.data(), WIDTH, HEIGHT, 1);
    LiteImage::SaveImage<float4>("saves/demos/SCom_demo_10_RF_bunny/res.png", ref);
  }

  //visualize harmonics
  const float3 pos    = float3(0, 0, 2);
  const float3 target = float3(0, 0, 0);
  const float3 up     = float3(0, 1, 0);

  const int cnt = 300;
  const int sh_size = SH_MAX_COEFFS * 3;
  const int sh_count = dag.voxel_channels[0].data_f.size() / sh_size;

  MultiRenderPreset mr_preset = getDefaultPreset();
  mr_preset.render_mode = MULTI_RENDER_MODE_TEX_COORDS;
  mr_preset.spp = 16;

  auto pRender = CreateMultiRenderer(DEVICE_GPU);
  pRender->SetPreset(mr_preset);
  pRender->setBackgroundColor(float4(0, 0, 0, 0));
  pRender->SetViewport(0, 0, WIDTH, HEIGHT);

  for (int i = 0; i < cnt; i++)
  {
    int sh_idx = rand() % sh_count;
    auto mesh = create_sh_sphere(SH_MAX_DEGREE, (const float3 *)(dag.voxel_channels[0].data_f.data() + sh_idx * sh_size));

    pRender->SetScene(mesh);
    render(ref, pRender, mr_preset, pos, target, up);
    std::string path = "saves/demos/SCom_demo_10_RF_bunny/sh_" + std::to_string(sh_idx) + ".png";

    if (i % 3 == 0)
    {
      for (int p=0; p<WIDTH*HEIGHT; p++)
        ref.data()[p] = LiteMath::shuffle_yzxw(ref.data()[p]);
    }
    else if (i % 3 == 1)
    {
      for (int p=0; p<WIDTH*HEIGHT; p++)
        ref.data()[p] = LiteMath::shuffle_zyxw(ref.data()[p]);      
    }

    LiteImage::SaveImage<float4>(path.c_str(), ref);
  }
}

void SCom_demo_11_colored_bunny()
{
  constexpr uint32_t WIDTH = 2048;
  constexpr uint32_t HEIGHT = 2048;

  const float3 pos    = float3(-1, -0.1, 2);
  const float3 target = float3( 0, -0.1, 0);
  const float3 up     = float3( 0, 1, 0);

  auto mesh = cmesh4::LoadMesh("../models/Bunny.obj");
  cmesh4::rescale_mesh(mesh, float3(-0.75f), float3(0.75f));

  LiteImage::Image2D<float4> ref(WIDTH, HEIGHT);
  LiteImage::Image2D<float4> res(WIDTH, HEIGHT);

  MultiRenderPreset preset = getDefaultPreset();
  preset.render_mode = MULTI_RENDER_MODE_LAMBERT;
  preset.spp = 16;

  if (!std::filesystem::exists("saves/demos/SCom_demo_11_colored_bunny"))
    std::filesystem::create_directories("saves/demos/SCom_demo_11_colored_bunny");

  {
    auto pRender = CreateMultiRenderer(DEVICE_GPU);
    pRender->SetPreset(preset);
    pRender->setBackgroundColor(float4(0.5, 0.5, 1, 1));
    pRender->SetViewport(0, 0, WIDTH, HEIGHT);
    std::vector<Light> lights = std::vector<Light>{
      create_direct_light({1,0,0},{1,0,0}),
      create_direct_light({0,1,0},{0,1,0}),
      create_direct_light({0,0,1},{0,0,1}),
      create_direct_light({-1,0,0},{0,1,1}),
      create_direct_light({0,-1,0},{1,0,1}),
      create_direct_light({0,0,-1},{1,1,0})
    };

    pRender->SetLights(lights);
    pRender->SetScene(mesh);
    render(ref, pRender, preset, pos, target, up);
    LiteImage::SaveImage<float4>("saves/demos/SCom_demo_11_colored_bunny/res.png", ref);
  }
}

void SCom_demo_12_volume_4D_vs_slices()
{
  constexpr uint32_t WIDTH = 1920;
  constexpr uint32_t HEIGHT = 1080;

  LiteImage::Image2D<float4> ref(WIDTH, HEIGHT);
  LiteImage::Image2D<float4> res(WIDTH, HEIGHT);

  Camera camera;
  camera.pos = float3(-0.903829, -0.102390, 1.412256);
  camera.lookAt = float3(61.210796, -6.729793, -76.676476);
  camera.up = float3(0.041256, 0.997802, -0.051867);
  camera.fovy = 45.0f;
  auto proj = projectionMatrix(camera.fovy, (float)WIDTH / (float)HEIGHT, camera.z_near, camera.z_far);
  auto view = LiteMath::lookAt(camera.pos, camera.lookAt, camera.up);

  VolumeRenderPreset preset = getDefaultVolumeRenderPreset();
  preset.alpha_thr = 0.001f;
  preset.use_lighting = 1;
  preset.voxel_traversal_steps = 1;
  preset.spp = 1;
  preset.animation_speed = 1.0f;
  preset.traversal_mode = VOLUME_TRAVERSAL_MODE_VRT_SS;

  if (!std::filesystem::exists("saves/demos/SCom_demo_12_volume_4D_vs_slices"))
    std::filesystem::create_directories("saves/demos/SCom_demo_12_volume_4D_vs_slices");

  DensityMultiGrid grid;
  load_density_grid(grid, "side_projects/fluid_simulation/saves/multi_grid_smoke_128.bin");

  //convert scene to slices
  if (true)
  {
    float time1 = 0.0f, time2 = 0.0f, time3 = 0.0f;
    const uint64_t slice_size = grid.size.x*grid.size.y*grid.size.z;
    uint64_t bytesize = 0;
    for (uint32_t frame_id = 0; frame_id < grid.frames; frame_id++)
    {
      DensityGrid slice_grid;
      slice_grid.data.resize(slice_size, 0.0f);
      slice_grid.size = grid.size;
      std::copy_n(grid.data.data() + frame_id*slice_size, slice_size, slice_grid.data.data());
      slice_grid.data[slice_size/2] = 0.001f;
      //for (int i=0;i<slice_size;i++)
      //  printf("val[%d]: %f\n", i, slice_grid.data[i]);

      auto t1 = std::chrono::high_resolution_clock::now();
      SparseOctreeSettings settings(6, 0.0001f, 3);
      sdf_converter::GlobalOctree octree;
      sdf_converter::density_grid_to_global_octree(settings, slice_grid, octree, nullptr);

      auto t2 = std::chrono::high_resolution_clock::now();
      scom2::Settings scom_settings = scom2::Settings();
      scom_settings.embedding_type = scom2::EmbeddingType::TRIVIAL;
      scom_settings.internal_brick_use = scom2::InternalBrickUse::PROPER_CLUSTERING;
      scom_settings.branch_descriptor = scom2::BranchDescriptor::SIMPLE_MERGE;
      scom_settings.bricks_use_similarity_compression = true;
      scom_settings.bricks_similarity_threshold = 3.0f;
      scom_settings.branches_use_similarity_compression = true;
      scom_settings.branches_similarity_compression_levels = 3;
      scom_settings.branches_similarity_threshold = 1.5f;
      scom_settings.branches_threshold_reduction_factor = 0.25f;
      scom_settings.bits_per_value = 16;

      scom_settings.support_multi_nodes = false;
      scom_settings.support_channels = false;
      scom_settings.support_surfaces = true;
      scom_settings.custom_max_val = true;
      scom_settings.bricks_use_average_val_transform = false;
      scom_settings.use_close_match_heuristic = true;

      SdfDAG dag;
      SCom2Tree scom_tree;
      sdf_converter::global_octree_to_DAG(octree, dag, scom_settings);

      auto t3 = std::chrono::high_resolution_clock::now();
      scom2::pack_SCom2(scom_settings, dag, scom_tree);
      bytesize += sizeof(uint32_t)*(scom_tree.bricks.size()+scom_tree.nodes.size());
      
      auto t4 = std::chrono::high_resolution_clock::now();
      time1 += std::chrono::duration<float, std::milli>(t2 - t1).count();
      time2 += std::chrono::duration<float, std::milli>(t3 - t2).count();
      time3 += std::chrono::duration<float, std::milli>(t4 - t3).count();

      {
        LiteScene::HydraScene scene;
        auto *geom = new LiteScene::SCom2Geometry();
        uint32_t geomId = scene.add_geometry(geom);
        geom->init(std::move(scom_tree));
        scene.add_instance(geomId, float4x4());

        std::string slice_path = "saves/demos/SCom_demo_12_volume_4D_vs_slices/slice_" + std::to_string(frame_id);
        scene.save(slice_path+"/scene_scom.xml", slice_path);
      }
    }

    printf("SLICES: octree_to_DAG: %.1f ms\n", time1);
    printf("SLICES: compress: %.1f ms\n", time2);
    printf("SLICES: pack_SCom2: %.1f ms\n", time3);
    printf("SLICES: total size %.2f MB\n", bytesize / 1024.0f / 1024.0f);
  }

  //convert scene to 4D SCom
  if (false)
  {
    auto t1 = std::chrono::high_resolution_clock::now();
    SparseOctreeSettings settings(6, 0.0001f, 3);
    sdf_converter::GlobalOctree octree;
    sdf_converter::density_grid_to_global_octree(settings, grid, octree, nullptr);

    scom2::Settings scom_settings = scom2::Settings();
    scom_settings.embedding_type = scom2::EmbeddingType::TRIVIAL;
    scom_settings.internal_brick_use = scom2::InternalBrickUse::PROPER_CLUSTERING;
    scom_settings.branch_descriptor = scom2::BranchDescriptor::SIMPLE_MERGE;
    scom_settings.packed_reference_type = scom2::PackedReferenceType::LONG_9_23_32;
    scom_settings.bricks_use_similarity_compression = true;
    scom_settings.bricks_similarity_threshold = 5.0f;
    scom_settings.branches_use_similarity_compression = true;
    scom_settings.branches_similarity_compression_levels = 3;
    scom_settings.branches_similarity_threshold = 3.0f;
    scom_settings.branches_threshold_reduction_factor = 0.125f;
    scom_settings.bits_per_value = 16;

    scom_settings.support_multi_nodes = false;
    scom_settings.support_channels = false;
    scom_settings.support_surfaces = true;
    scom_settings.custom_max_val = true;
    scom_settings.bricks_use_average_val_transform = false;
    scom_settings.use_close_match_heuristic = true;

    SdfDAG dag;
    SCom2Tree scom_tree;
    auto t2 = std::chrono::high_resolution_clock::now();
    sdf_converter::global_octree_to_DAG(octree, dag, scom_settings);
    
    auto t3 = std::chrono::high_resolution_clock::now();
    scom2::pack_SCom2(scom_settings, dag, scom_tree);

    auto t4 = std::chrono::high_resolution_clock::now();
    float time1 = std::chrono::duration<float, std::milli>(t2 - t1).count();
    float time2 = std::chrono::duration<float, std::milli>(t3 - t2).count();
    float time3 = std::chrono::duration<float, std::milli>(t4 - t3).count();
    printf("octree_to_DAG: %.1f ms\n", time1);
    printf("compress: %.1f ms\n", time2);
    printf("pack_SCom2: %.1f ms\n", time3);

    uint64_t bytesize = sizeof(uint32_t)*(scom_tree.bricks.size()+scom_tree.nodes.size());
    printf("total size %.2f MB\n", bytesize / 1024.0f / 1024.0f);

    {
      LiteScene::HydraScene scene;
      auto *geom = new LiteScene::SCom2Geometry();
      uint32_t geomId = scene.add_geometry(geom);
      geom->init(std::move(scom_tree));
      scene.add_instance(geomId, float4x4());
      scene.save("saves/demos/SCom_demo_12_volume_4D_vs_slices/scene_scom.xml", 
                 "saves/demos/SCom_demo_12_volume_4D_vs_slices");
    }
  }

  //render and measure quality
  const float t_range = grid.timestamps.back() - grid.timestamps.front();
  const uint32_t max_frames = grid.timestamps.size()-1;
  const std::vector<uint32_t> frames = {20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120};

  std::string frames_folder = "saves/demos/SCom_demo_12_volume_4D_vs_slices/frames";
  if (!std::filesystem::exists(frames_folder))
    std::filesystem::create_directories(frames_folder);

  std::shared_ptr<VolumeRenderer> pRenderG = CreateVolumeRenderer(DEVICE_GPU_COMP);
  pRenderG->SetPreset(preset);
  pRenderG->SetViewport(0, 0, WIDTH, HEIGHT);
  pRenderG->SetBaseColor(float4(0.75, 0.75, 0.75, 1));
  pRenderG->setBackgroundColor(float4(0.45, 0.65, 0.90, 1.0));
  pRenderG->LoadGrid(grid.size.x, grid.data.data(), grid.timestamps);
  pRenderG->UpdateCamera(view, proj);
  pRenderG->CommitDeviceData();
  pRenderG->Clear(WIDTH, HEIGHT);

  std::shared_ptr<VolumeRenderer> pRender4D = CreateVolumeRenderer(DEVICE_GPU_COMP);
  pRender4D->SetPreset(preset);
  pRender4D->SetViewport(0, 0, WIDTH, HEIGHT);
  pRender4D->SetBaseColor(float4(0.75, 0.75, 0.75, 1));
  pRender4D->setBackgroundColor(float4(0.45, 0.65, 0.90, 1.0));
  pRender4D->LoadScene("saves/demos/SCom_demo_12_volume_4D_vs_slices/scene_scom.xml");
  pRender4D->UpdateCamera(view, proj);
  pRender4D->CommitDeviceData();
  pRender4D->Clear(WIDTH, HEIGHT);

  float prev_t = 0.0f;
  float prev_tg = 0.0f;
  float av_PSNR_3D = 0.0f;
  float av_PSNR_4D = 0.0f;
  for (uint32_t frame_id : frames)
  {
    const float t = float(frame_id) / float(max_frames);
    const float tg = t_range*float(frame_id+1) / float(max_frames);
    
    pRenderG->nextFrame(tg - prev_tg);
    pRenderG->UpdateMembersPlainData();
    pRenderG->RenderFloat(ref.data(), WIDTH, HEIGHT, 1);

    char filename[1024];
    snprintf(filename, 1024, "%s/grid_%04d.png", frames_folder.c_str(), frame_id);
    LiteImage::SaveImage<float4>(filename, ref);

    pRender4D->nextFrame(t-prev_t);
    pRender4D->UpdateMembersPlainData();
    pRender4D->RenderFloat(res.data(), WIDTH, HEIGHT, 1);

    snprintf(filename, 1024, "%s/SCom4D_%04d.png", frames_folder.c_str(), frame_id);
    LiteImage::SaveImage<float4>(filename, res);
    float PSNR_4D = image_metrics::PSNR(ref, res);

    std::string slice_path = "saves/demos/SCom_demo_12_volume_4D_vs_slices/slice_" + std::to_string(frame_id);
    std::shared_ptr<VolumeRenderer> pRender3D = CreateVolumeRenderer(DEVICE_GPU_COMP);
    pRender3D->SetPreset(preset);
    pRender3D->SetViewport(0, 0, WIDTH, HEIGHT);
    pRender3D->SetBaseColor(float4(0.75, 0.75, 0.75, 1));
    pRender3D->setBackgroundColor(float4(0.45, 0.65, 0.90, 1.0));
    pRender3D->LoadScene((slice_path+"/scene_scom.xml").c_str());
    pRender3D->UpdateCamera(view, proj);
    pRender3D->CommitDeviceData();
    pRender3D->Clear(WIDTH, HEIGHT);

    pRender3D->RenderFloat(res.data(), WIDTH, HEIGHT, 1);
    snprintf(filename, 1024, "%s/SCom3D_%04d.png", frames_folder.c_str(), frame_id);
    LiteImage::SaveImage<float4>(filename, res);
    float PSNR_3D = image_metrics::PSNR(ref, res);

    printf("Frame %d/%d\n", frame_id, max_frames);
    printf("PSNR 4D %f\n", PSNR_4D);
    printf("PSNR 3D %f\n", PSNR_3D);

    prev_t = t;
    prev_tg = tg;

    av_PSNR_3D += PSNR_3D;
    av_PSNR_4D += PSNR_4D;
  }

  printf("Average PSNR 4D %f\n", av_PSNR_4D/float(frames.size()));
  printf("Average PSNR 3D %f\n", av_PSNR_3D/float(frames.size()));
}