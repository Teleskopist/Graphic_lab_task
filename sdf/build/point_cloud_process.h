#include "LiteMath.h"
#include "utils/mesh/mesh.h"
#include "sdf/build/sparse_octree_common.h"
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <array>
#include <limits>

namespace point_cloud
{

  void SDF_by_point_cloud(
    const cmesh4::SimpleMesh& mesh,
    const SparseOctreeSettings settings,
    const uint32_t* triangle_ids,
    uint32_t triangle_count,
    const LiteMath::float3& voxel_min,
    const LiteMath::float3& voxel_max,
    sdf_converter::MeshDistFunc &dist_func,
    float *res);

};