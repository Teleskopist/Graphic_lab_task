#include "LiteMath.h"
#include <algorithm>
#include <cmath>

namespace sim_metric
{
  std::vector<LiteMath::float3> sample_voxel_surface(const float voxel[8], float step = 0.1f);
  void sample_voxel_surface(const float voxel[8], std::vector<LiteMath::float3> &points, int &size, float step = 0.1f);

  float chamfer_distance(const LiteMath::float3* set1, int count1, const LiteMath::float3* set2, int count2);
  float hausdorff_distance(const LiteMath::float3* set1, int count1, const LiteMath::float3* set2, int count2);

  float chamfer_metric(const float voxel1[8], const float voxel2[8], float step = 0.1f);
  float hausdorff_metric(const float voxel1[8], const float voxel2[8], float step = 0.1f);
};