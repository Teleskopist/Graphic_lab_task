#pragma once
#include "scom.h"
#include "scom_builder.h"
#include "clustering/octahedral_groups.h"
#include "utils/common/data_channel.h"
#include "utils/common/int_pow.h"
#include "utils/common/matrix.h"

#include <functional>

static inline uint32_t get_v_size(const SdfDAGHeader &header)
{
  return 2 * header.brick_pad + header.brick_size + 1;
}
static inline uint32_t get_v_count(const SdfDAGHeader &header)
{
  return int_pow(get_v_size(header), header.dim);
}
static inline uint32_t get_children_count(const SdfDAGHeader &header)
{
  return int_pow(header.node_grid_size, header.dim);
}

namespace scom2
{
  static uint4 idx_to_code(uint32_t idx, uint32_t sz)
  {
    uint4 code;
    code.z = idx % sz;
    code.y = (idx / sz) % sz;
    code.x = (idx / sz / sz) % sz;
    code.w = (idx / sz / sz / sz);
    return code;
  }

  static uint32_t code_to_idx(uint4 code, uint32_t sz)
  {
    return code.w * sz * sz * sz + code.x * sz * sz + code.y * sz + code.z;
  }

  // if there is a node with this position code, returns its offset in SCom2Tree nodes array and rotation
  // if it is a leaf, or does not exit at all, returns (INVALID_IDX, INVALID_IDX)
  uint2 get_node_offset_rot_from_pos_code(const SCom2Tree &tree, const uint4 &pos_code);

  // Calculates maximum depth of DAG, assumes it is actually a DAG, i.e. there are no cycles
  uint32_t calculate_DAG_max_depth_rec(const SdfDAG &dag, uint32_t nodeId);

  // prints DAG recursively
  void print_DAG_rec(const SdfDAG &dag, uint32_t node_id);

  // DAG traversal, callback function is called for each node, return true to continue traversal or false to stop
  // transformId is an index of cumulative transformation applied to the node
  // code is a position of the node in space, (x,y,z,w), each in [0, grid_size-1]. For 3-dimensions, w is always 0, 
  using dag_callback_t = std::function<bool(const SdfDAG &dag, uint32_t nodeId, uint32_t transformId, uint32_t level, uint4 code)>;
  void traverse_DAG(const SdfDAG &dag, dag_callback_t callback);

  // rotates DAG by changing rotation ids for the first level of nodes, only one of 48 primitive rotations is allowed
  void rotate_DAG(LiteMath::float3x3 rot, SdfDAG &dag);
}