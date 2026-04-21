#pragma once

#include "demo_app/IRenderPlugin.h"

#if USE_MULTI_RENDERER
  #include "MRFull/plugin.h"
#endif
#if USE_VOXEL_RENDERER
#include "VoxelRenderer/VoxelRenderer.h"
#include "VoxelRenderer/VoxelRenderer_slang.h"
#endif
#if USE_VOLUME_RENDERER
#include "VolumeRenderer/VolumeRenderer.h"
#include "VolumeRenderer/VolumeRenderer_slang.h"
#endif
#if USE_HYDRA
  #include "Hydra/plugin.h"
#endif

namespace demo_app
{
  static std::shared_ptr<IRenderPlugin> getPlugin(RenderType type)
  {
    switch (type)
    {
    case RenderType::MULTI:
#if USE_MULTI_RENDERER
      return std::shared_ptr<IRenderPlugin>(new MRFullPlugin());
#else
      return nullptr;
#endif
    case RenderType::HYDRA:
#if USE_HYDRA
      return std::shared_ptr<IRenderPlugin>(new HydraPlugin());
#else
      return nullptr;
#endif
    case RenderType::VOLUME:
#if USE_VOLUME_RENDERER
      return std::make_shared<DemoPlugin<VolumeRenderer, VolumeRenderPreset, VolumeRenderer_slang>>();
#else
      return nullptr;
#endif
    case RenderType::VOXEL:
#if USE_VOXEL_RENDERER
      return std::make_shared<DemoPlugin<VoxelRenderer, VoxelRenderPreset, VoxelRenderer_slang>>();
#else
      return nullptr;
#endif
    case RenderType::CSG_DIRECT:
    default:
      return nullptr;
    }
  }
}