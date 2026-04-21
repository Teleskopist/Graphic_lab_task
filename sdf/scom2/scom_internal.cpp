#include "scom_internal.h"
#include "scom_builder.h"

namespace scom2
{
  void calculate_node_count_on_levels_rec(const SdfDAG &dag, uint32_t node_id, uint32_t level, uint32_t *node_counts)
  {
    if (dag.nodes[node_id].data_edges_offset != 0)
      node_counts[level]++;
    
    if (dag.nodes[node_id].children_edges_offset != 0)
    {
      uint32_t ch_count = get_children_count(dag.header);
      for (int i = 0; i < ch_count; ++i)
      {
        uint32_t child_id = dag.child_edges[dag.nodes[node_id].children_edges_offset + i].child_offset;
        if (child_id != 0)
          calculate_node_count_on_levels_rec(dag, child_id, level + 1, node_counts);
      }
    }
  }

  // for similarity compression, both bricks and branches, we compress blocks from
  // different levels separately, because we should use different thresholds
  // (some change in large nodes are more visible than in small ones)
  // so we need to get the base level for which the similarity threshold value
  // is applied. Here we use the level with the most active nodes
  uint32_t get_base_level(const SdfDAG &dag)
  {
    uint32_t node_counts[IDescriptorMaker::MAX_OCTREE_DEPTH+1] = {0};
    calculate_node_count_on_levels_rec(dag, 0, 0, node_counts);
    uint32_t max_count = 0;
    uint32_t base_level = 0;
    for (uint32_t i = 0; i <= IDescriptorMaker::MAX_OCTREE_DEPTH; ++i)
    {
      if (node_counts[i] > max_count)
      {
        max_count = node_counts[i];
        base_level = i;
      }
    }
    return base_level;
  }
}