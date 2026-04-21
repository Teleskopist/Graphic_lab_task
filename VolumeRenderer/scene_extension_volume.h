#pragma once
#include "utils/misc/density_grid.h"
#include "core/scene_extension.h"

namespace LiteScene
{
  REGISTER_TYPE(TYPE_DENSITY_GRID,       DensityGridGeometry,           DensityGrid,           "density_grid",             save_density_grid,   load_density_grid,     get_info_density_grid)
  REGISTER_TYPE(TYPE_COLOR_DENSITY_GRID, ColorDensityGridGeometry,      ColorDensityGrid,      "color_density_grid",       save_density_grid,   load_density_grid,     get_info_density_grid)
  REGISTER_TYPE(TYPE_DENSITY_GRID,       DensityMultiGridGeometry,      DensityMultiGrid,      "density_multi_grid",       save_density_grid,   load_density_grid,     get_info_density_grid)
  REGISTER_TYPE(TYPE_COLOR_DENSITY_GRID, ColorDensityMultiGridGeometry, ColorDensityMultiGrid, "color_density_multi_grid", save_density_grid,   load_density_grid,     get_info_density_grid)
}