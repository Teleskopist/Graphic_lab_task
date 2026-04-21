#pragma once
#include "utils/mesh/marching_cubes.h"
#include "sdf/scom2/scom.h"

namespace cmesh4
{
  cmesh4::SimpleMesh create_mesh_marching_cubes_scom2(MarchingCubesSettings settings, SCom2Tree octree, 
                                                      unsigned max_threads, unsigned local_depth = 1);
}