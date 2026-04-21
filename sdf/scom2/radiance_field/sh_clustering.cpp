#include "sh_clustering.h"

SHAdaptiveColorBinner::SHAdaptiveColorBinner(uint32_t approx_bin_count, const float *sh_data, uint32_t sh_count, uint32_t sh_stride)
{
  assert(sh_data != nullptr);
  assert(sh_count > 0);
  assert(approx_bin_count > 0);
  assert(sh_count < approx_bin_count);
  assert(sh_stride >= 3);

  nn_search::Dataset shColorsDatset;
  shColorsDatset.dim = 3;
  shColorsDatset.data_points.resize(sh_count);
  shColorsDatset.all_points.resize(sh_count * 3);
  for (int i = 0; i < sh_count; ++i)
  {
    shColorsDatset.data_points[i].data_offset = i * 3;
    shColorsDatset.data_points[i].original_id = i;
    shColorsDatset.data_points[i].transform_id = 0;
    shColorsDatset.data_points[i].average_val = 0.0f;

    shColorsDatset.all_points[i * 3 + 0] = sh_data[i * sh_stride + 0];
    shColorsDatset.all_points[i * 3 + 1] = sh_data[i * sh_stride + 1];
    shColorsDatset.all_points[i * 3 + 2] = sh_data[i * sh_stride + 2];
  }

  uint32_t points_per_leaf = sh_count / approx_bin_count;
  printf("building tree with %d points per leaf\n", points_per_leaf);
  nn_search::KDTree kd_tree;
  kd_tree.build(shColorsDatset, points_per_leaf);
  m_nodes = kd_tree.get_nodes();
  printf("built tree with %d nodes\n", (int)m_nodes.size());
}

uint32_t SHAdaptiveColorBinner::calculate_bin_hash(const float *values, uint32_t size) const
{
  constexpr uint32_t sh_coef_count = 3 * SH_MAX_COEFFS;
  assert(size % sh_coef_count == 0);

  //binning is based on average color
  float3 sh_color = float3(0,0,0);
  float cnt = 0;
  for (uint32_t i = 0; i < size; i += sh_coef_count)
  {
    float3 color = float3(values[i + 0], values[i + 1], values[i + 2]);

    //if base color is _exactly_ bblack, it is probably just empty space and we can skip it
    if (length(color) > 1e-9f)
    {
      sh_color += color;
      cnt += 1.0f;
    }
  }

  sh_color /= std::max(1.0f, cnt);
  
  uint32_t nodeId = 0;
  while (m_nodes[nodeId].is_leaf == false)
  {
    uint32_t axis = m_nodes[nodeId].axis;
    float split = m_nodes[nodeId].split_val;
    if (sh_color[axis] < split)
    {
      nodeId = m_nodes[nodeId].left;
    }
    else
    {
      nodeId = m_nodes[nodeId].right;
    }
  }

  return nodeId;
}