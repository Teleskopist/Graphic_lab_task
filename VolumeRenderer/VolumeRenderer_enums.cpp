#include "VolumeRenderer_enums.h"

#include "VolumeRenderer.h"
#include "blk/blk.h"

REGISTER_ENUM(VolumeTraversalMode,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"VOLUME_TRAVERSAL_MODE_BRESENHAM", VOLUME_TRAVERSAL_MODE_BRESENHAM},
                     {"VOLUME_TRAVERSAL_MODE_DDA", VOLUME_TRAVERSAL_MODE_DDA},
                     {"VOLUME_TRAVERSAL_MODE_DDA_BRANCHLESS", VOLUME_TRAVERSAL_MODE_DDA_BRANCHLESS},
                     {"VOLUME_TRAVERSAL_MODE_VRT_SS", VOLUME_TRAVERSAL_MODE_VRT_SS},
                 }; })());

REGISTER_ENUM(VolumeType,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"VOLUME_TYPE_LINEAR_DENSITY", VOLUME_TYPE_LINEAR_DENSITY},
                     {"VOLUME_TYPE_SVRASTER_RF", VOLUME_TYPE_SVRASTER_RF},
                 }; })());

void load_from_blk(VolumeRenderPreset &settings, const Block *block)
{
  settings = getDefaultVolumeRenderPreset();
  
  settings.traversal_mode = block->get_enum("traversal_mode", settings.traversal_mode);
  settings.volume_type = block->get_enum("volume_type", settings.volume_type);
  settings.spp = block->get_int("spp", settings.spp);
  settings.voxel_traversal_steps = block->get_int("voxel_traversal_steps", settings.voxel_traversal_steps);
  settings.alpha_thr = block->get_double("alpha_thr", settings.alpha_thr);
  settings.animation_speed = block->get_double("animation_speed", settings.animation_speed);
  settings.ignore_timestamps = block->get_bool("ignore_timestamps", settings.ignore_timestamps);
  settings.use_lighting = block->get_bool("use_lighting", settings.use_lighting);
}

void save_to_blk(const VolumeRenderPreset &settings, Block *block)
{
  block->set_enum("traversal_mode", "VolumeTraversalMode", settings.traversal_mode);
  block->set_enum("volume_type", "VolumeType", settings.volume_type);
  block->set_int("spp", settings.spp);
  block->set_int("voxel_traversal_steps", settings.voxel_traversal_steps);
  block->set_double("alpha_thr", settings.alpha_thr);
  block->set_double("animation_speed", settings.animation_speed);
  block->set_bool("ignore_timestamps", settings.ignore_timestamps);
  block->set_bool("use_lighting", settings.use_lighting);
}

void force_register_VolumeRenderer_enums()
{

}