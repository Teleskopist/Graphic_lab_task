#include "offscreen_render.h"
#include "Image2d.h"
#include "MultiRenderer/MultiRenderer.h"
#include "HydraRenderer/renderer.h"
#include "utils/common/strings.h"
#include "utils/misc/camera.h"
#include "core/serialization.h"

#include <filesystem>

#ifdef USE_GPU
#include "vk_context.h"
#endif

OffscreenRenderResultCode render_offscreen(const Block *config)
{
  std::string scene_file = config->get_string("scene");
  int WIDTH = config->get_int("width", 2048);
  int HEIGHT = config->get_int("height", 2048);
  int spp = config->get_int("spp", 1);
  std::string renderer_str = config->get_string("renderer", "MultiRenderer");
  std::string backend_str = config->get_string("backend", "GPU");
  std::string AS_string = config->get_string("AS_type", "AS_TYPE_COMMON");

  if (scene_file.empty())
  {
    printf("ERROR: config has no scene file specified\n");
    return OffscreenRenderResultCode::Error_InvalidConfig;
  }

  Camera base_camera;
  if (config->get_block("camera"))
  {
    load_from_blk(base_camera, config->get_block("camera"));
  }
  else if (config->get_block("base_camera"))
  {
    load_from_blk(base_camera, config->get_block("base_camera"));
  }
  else
  {
    base_camera.pos = config->get_vec3("camera_pos", float3(0.0f, 0.0f, 3.0f));
    base_camera.lookAt = config->get_vec3("camera_target", float3(0.0f, 0.0f, 0.0f));
    base_camera.up = config->get_vec3("camera_up", float3(0.0f, 1.0f, 0.0f));
    base_camera.fovy = config->get_double("camera_fov", 60.0f);
    base_camera.z_near = config->get_double("z_near", 0.01f);
    base_camera.z_far = config->get_double("z_far", 100.0f);
  }

  std::vector<std::string> paths;
  std::vector<Camera> cameras;
  if (config->get_id("cameras_count") < 0) // single camera
  {
    std::string output_file = config->get_string("output_file");
    if (output_file.empty())
    {
      printf("ERROR: config has no output file specified\n");
      return OffscreenRenderResultCode::Error_InvalidConfig;
    }
    paths.push_back(output_file);
    cameras.push_back(base_camera);
  }
  else // multiple cameras
  {
    std::filesystem::path output_folder = std::filesystem::path(config->get_string("output_folder"));
    if (!std::filesystem::exists(output_folder) && !std::filesystem::create_directory(output_folder))
    {
      printf("ERROR: Could not create folder %s\n", output_folder.string().c_str());
      return OffscreenRenderResultCode::Error_CreateFile;
    }

    int cameras_count = config->get_int("cameras_count", 16);
    float cameras_radius = config->get_double("cameras_radius", 5.0f);
    uint32_t cameras_distribution = config->get_enum("cameras_distribution", 0);
    uint32_t cameras_save_format = config->get_enum("cameras_save_format", 0);
    int cameras_ring_count = config->get_int("cameras_ring_count", 8);
    float cameras_ring_angle_amplitude = config->get_double("cameras_ring_angle_amplitude", 45.0f);
    cameras_ring_angle_amplitude = LiteMath::clamp(cameras_ring_angle_amplitude, 0.0f, 89.9f);

    std::vector<LiteMath::float3> cameras_positions(cameras_count, base_camera.pos);

    switch (cameras_distribution) {
      case 1: {
        int cameras_per_ring_count = cameras_count / cameras_ring_count;
        for (int i = 0; i < cameras_count; i++)
        {
          float phi = 2.0f * 3.14159f * i / cameras_per_ring_count;
          float theta = 3.14159f * ((90 - cameras_ring_angle_amplitude) + 2 * ((float)i / cameras_count) * cameras_ring_angle_amplitude) / 180.0f;
          float x = cosf(phi) * sinf(theta);
          float y = cosf(theta);
          float z = sinf(phi) * sinf(theta);

          cameras_positions[i] = LiteMath::float3{x * cameras_radius, y * cameras_radius, z * cameras_radius} + base_camera.lookAt;
        }
      } break;
      case 2: {
        int cameras_per_ring_count = cameras_count / cameras_ring_count;
        for (int i = 0; i < cameras_count; i++)
        {
          float phi = 2.0f * 3.14159f * i / cameras_per_ring_count;
          float theta = 3.14159f * ((90 - cameras_ring_angle_amplitude) + 2 * ((float)(i / cameras_per_ring_count) / cameras_ring_count) * cameras_ring_angle_amplitude) / 180.0f;
          float x = cosf(phi) * sinf(theta);
          float y = cosf(theta);
          float z = sinf(phi) * sinf(theta);

          cameras_positions[i] = LiteMath::float3{x * cameras_radius, y * cameras_radius, z * cameras_radius} + base_camera.lookAt;
        }
      } break;
      default: {
        float phi = 3.14159f * (sqrtf(5.0f) - 1.0f);
        float two_inv_cam_count = 2.0f / (cameras_count - 1);
        for (int i = 0; i < cameras_count; i++)
        {
          float z = 1.0f - (i * two_inv_cam_count);
          float radius = sqrtf(1.0f - z * z);
          float theta = phi * i;
          float x = cosf(theta) * radius;
          float y = sinf(theta) * radius;

          cameras_positions[i] = LiteMath::float3{x * cameras_radius, y * cameras_radius, z * cameras_radius} + base_camera.lookAt;
        }
      }
    }

    for (int i = 0; i < cameras_count; i++)
    {
      std::string output_file = (output_folder / (std::to_string(i) + ".png")).string();
      paths.push_back(output_file);
      cameras.push_back(base_camera);
      cameras[i].pos = cameras_positions[i];
    }

    if (cameras_save_format == 1)
    {
      std::ofstream cameras_csv(output_folder / "cameras.csv");
      cameras_csv << "id,px,py,pz\n";
      for (int i = 0; i < cameras_positions.size(); i++)
      {
        cameras_csv << i << "," << cameras_positions[i].x << "," << cameras_positions[i].y << "," << cameras_positions[i].z << "\n";
      }
      cameras_csv.close();
    }
  }

  LiteImage::Image2D<uint32_t> image(WIDTH, HEIGHT);
  uint32_t backend = RenderDevice_from_string(backend_str);
  uint32_t AS_type = RenderASType_from_string(AS_string);

  std::shared_ptr<IRenderer> renderer;

  if (renderer_str == "HydraRenderer" || renderer_str == "Hydra")
  {
    // hydra renderer gets camera and path tracing options from scene
    // we probably should add more parameters in future
    HydraRenderPreset preset = getDefaultHydraRenderPreset();
    preset.spp = spp;

    std::shared_ptr<HydraRenderer> hr = std::make_shared<HydraRenderer>(backend);
    hr->SetPreset(preset);
    hr->LoadScene(scene_file.c_str());

    renderer = hr;
  }
  else if (renderer_str == "MultiRenderer" || renderer_str == "MR")
  {
    // MultiRenderer is much more flexible, read all possible parameters here
    float4 background_color = config->get_vec4("background_color", float4(0.0f, 0.0f, 0.0f, 1.0f));
    float3 light_dir = config->get_vec3("light_dir", float3(1.0f, 1.0f, 1.0f));
    float3 light_color = config->get_vec3("light_color", float3(2.0f / 3.0f, 2.0f / 3.0f, 2.0f / 3.0f));
    float3 ambient_light_color = config->get_vec3("ambient_light_color", float3(0.25f, 0.25f, 0.25f));

    int base_lod = config->get_int("base_lod", 16);
    bool fixed_lod = config->get_bool("fixed_lod", true);
    int render_mode = config->get_int("render_mode_raw", MULTI_RENDER_MODE_LAMBERT_NO_TEX);

    MultiRenderPreset preset = getDefaultPreset();
    preset.spp = spp;
    preset.fixed_lod = fixed_lod;
    preset.level_of_detail = base_lod;
    preset.render_mode = render_mode;

    // std::vector<Light> lights = {create_direct_light(light_dir, light_color),
    //                              create_ambient_light(ambient_light_color)};

    std::vector<Light> lights = {create_direct_light({1.0f, 0.0f, 0.0f}, {0.5f, 0.0f, 0.0f}),
                                 create_direct_light({-1.0f, 0.0f, 0.0f}, {0.0f, 0.5f, 0.5f}),
                                 create_direct_light({0.0f, 1.0f, 0.0f}, {0.0f, 0.5f, 0.0f}),
                                 create_direct_light({0.0f, -1.0f, 0.0f}, {0.5f, 0.0f, 0.5f}),
                                 create_direct_light({0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.5f}),
                                 create_direct_light({0.0f, 0.0f, -1.0f}, {0.5f, 0.5f, 0.0f})};

    int preferred_bvh_level = config->get_int("preferred_bvh_level", 4);
    BVHRT::preferredBVHLevel = preferred_bvh_level;
    auto mr = CreateMultiRenderer(backend, AS_type);
    mr->SetPreset(preset);
    mr->setBackgroundColor(background_color);
    mr->SetLights(lights);

    // we can either render a full scene or a single mesh
    // TODO: probably add rendering of a specific SDF representation (SCom Tree?)
    std::string format = split(std::string(scene_file.c_str()), '.').back();
    if (format == "xml")
      mr->LoadScene(scene_file.c_str());
    else if (format == "vsgf" || format == "obj" || format == "ply")
    {
      auto mesh = cmesh4::LoadMesh(scene_file.c_str());
      mr->SetScene(mesh);
    }

    renderer = mr;
  }
  else
  {
    printf("ERROR: unknown renderer %s\n", renderer_str.c_str());
    return OffscreenRenderResultCode::Error_InvalidConfig;
  }

  renderer->SetViewport(0, 0, WIDTH, HEIGHT);
  renderer->CommitDeviceData();
  renderer->Clear(WIDTH, HEIGHT);

  for (int i = 0; i < cameras.size(); i++)
  {
    const auto &camera = cameras[i];
    auto worldView = LiteMath::lookAt(camera.pos, camera.lookAt, camera.up);
    float aspect = float(WIDTH) / HEIGHT;
    auto proj = LiteMath::perspectiveMatrix(camera.fovy, aspect, camera.z_near, camera.z_far);
    renderer->UpdateCamera(worldView, proj);
    renderer->UpdateMembersPlainData();
    renderer->Render(image.data(), WIDTH, HEIGHT, 1);
    float t_arr[4] = {0, 0, 0, 0};
    renderer->GetExecutionTime("Render", t_arr);
    LiteImage::SaveImage(paths[i].c_str(), image);
    printf("[%d/%d] Render took %.1f ms\n", i+1, (int)cameras.size(), t_arr[0]);
  }
  return OffscreenRenderResultCode::Ok;
}