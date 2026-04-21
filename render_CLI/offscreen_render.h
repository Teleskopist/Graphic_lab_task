#pragma once
#include "blk/blk.h"

enum class OffscreenRenderResultCode
{
  Ok,
  Error_CreateFile,
  Error_InvalidConfig,
  Error_Internal,
  Error_Unknown,
};

// render a scene to an image according to config
// return 0 on success
OffscreenRenderResultCode render_offscreen(const Block *config);