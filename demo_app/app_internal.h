#pragma once

#include "app.h"
#include "LiteApp/rasterization/context.h"

struct CSVData;

namespace demo_app
{
  //app
  void add_to_log(const LogEntry &log_entry, UIContext &ui_ctx);
  void reset_frame_counter(UIContext &ui_ctx);
  std::string get_time_str();

  //raytracing
  void raytrace(RTLayout rtLayout, UIContext &ui_ctx, const RenderingContext &r_ctx, RaytracingContext &rt_ctx);

  void on_screen_resolution_change_RT(const RenderingContext &r_ctx, RaytracingContext &rt_ctx);
  void setup_RT_scene(UIContext&ui_ctx, LiteScene::HydraScene &scene, uint32_t rtId, const Settings &settings, RenderingContext &r_ctx, RaytracingContext &rt_ctx);
  void setup_RT_image(RenderingContext &r_ctx, RaytracingContext &rt_ctx);
  void update_raytracer_settings(const Settings &settings, UIContext&ui_ctx, const RenderingContext &r_ctx, RaytracingContext &rt_ctx);
  void update_antialiasing_settings(const Settings &settings, UIContext &ui_ctx, RenderingContext &r_ctx, RaytracingContext &rt_ctx);
  void update_view(const Settings &settings, RenderingContext &r_ctx);

  void initialize_scene_context(UIContext &ui_ctx, int i);
  void reload_scenes(int count, UIContext &ui_ctx);
}