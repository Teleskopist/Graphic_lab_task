#pragma once
#include "sdf/common/sdf_common_shared.h"

//################################################################################
// Constants and plain data structures definitions. Used both on GPU with slicer 
// and on CPU.
//################################################################################

//enum SCom2ChildType
static constexpr unsigned SCOM2_CHILD_EMPTY        = 0;
static constexpr unsigned SCOM2_CHILD_LEAF_VOLUME  = 1;
static constexpr unsigned SCOM2_CHILD_LEAF_SURFACE = 3;
static constexpr unsigned SCOM2_CHILD_NODE         = 2;

static constexpr unsigned SCOM2_CHILD_TYPE_BITS    = 2;
static constexpr unsigned SCOM2_CHILD_TYPE_MASK    = (1 << SCOM2_CHILD_TYPE_BITS) - 1;

static constexpr unsigned SCOM2_MAGIC_NUMBER = 0xffffdefa;
static constexpr unsigned SCOM2_VERSION = 4; // bump every time the format changes

struct SCom2Header
{
  uint32_t brick_size;      // values brick size    (default = 1, i.e. 2x2x2 values)
  uint32_t v_size;          // brick_size + 2*brick_pad + 1
  uint32_t bits_per_value;
  uint32_t values_per_uint;
  uint32_t value_mask;
  uint32_t bitmask_len;     // bitmask length in uints
  uint32_t dimension;

  uint32_t child_rot_shift;
  uint32_t child_rot_mask;
  uint32_t child_add_shift;
  uint32_t child_add_mask;
  uint32_t child_offset_mask;
  uint32_t child_offset_off;
  uint32_t node_offset_mask;
  uint32_t uints_per_link;
  uint32_t unique_brick_prefix;
  uint32_t unique_brick_offset_mask;

  uint32_t children_types_shift;
  uint32_t children_types_mask;
  uint32_t base_reference_shift;
  uint32_t children_active_bits_shift;
  uint32_t children_active_bits_mask;
  uint32_t references_offset;
  uint32_t reference_bits;
  uint32_t reference_mask;
  uint32_t references_per_uint;
  uint32_t links_offset;
  uint32_t max_surface_count;
  uint32_t max_surface_count_per_leaf;

  uint32_t bricks_step;
  uint32_t bricks_arr_offset;
  uint32_t nodes_arr_offset;
  uint32_t root_node_off;

  uint32_t has_channels;
  uint32_t has_surfaces;
  uint32_t has_multi_nodes;

  int tex_id_off;
  int mat_id_off;
  int all_float_tex_id_off;
  int all_int_mat_id_off;

  float max_val;
  uint32_t max_depth;

  float user_params[7];

  //to allow further extensions without breaking binary compatibility
  uint32_t _pad0;
  uint32_t _pad1;
  uint32_t _pad2;
  uint32_t _pad3;
  uint32_t _pad4;
};

struct SdfDAGDataEdge
{
  uint32_t data_offset; // offset in distances vector
  uint32_t rotation_id;
  uint32_t type_id;     // SdfMultiOctreeFlags
  float    add;
};

struct SdfDAGChildEdge
{
  uint32_t child_offset; // offset in nodes vector
  uint32_t rotation_id;
};

struct SdfDAGNode
{
  uint32_t children_edges_offset; // offset in vector of children edges, 0 offset means it's a leaf
  uint32_t data_edges_offset;     // offset in vector of data edges, 0 offset means it has no surface
  SdfDAGChildEdge channels_edge;  // offset+rotation for channels (offset in num_components*component_size steps).
  uint32_t voxel_count_flags;     // low 16 bits are number of voxels in this node, 0 mean it does not contain surface, 
                                  // high 16 bits are flags (SdfMultiOctreeFlags)
  uint32_t _pad;
};

struct SdfDAGHeader
{
  uint32_t brick_size;      // values brick size    (default = 1, i.e. 2x2x2 values)
  uint32_t brick_pad;       // values brick padding (default = 0)
  uint32_t node_grid_size;  // node grid size       (default = 2, i.e. 2x2x2 children, octree)
  uint32_t dim;             // dimentionality of the DAG, 2 for quadtree, 3 for octree (default), 4 for ???-tree

  uint32_t transform_subgroup; // enum class TransformSubgroup
  float user_params[7];        // some place to store user-defined params for specific types of DAGs, e.g. scene_extent for Radiance Fields

  int tex_id_off;
  int mat_id_off;
  int all_float_tex_id_off;
  int all_int_mat_id_off;
};

struct NodeHeadUnpacked
{
  uint32_t base_type;
  uint32_t children_types;
  uint32_t base_links_end;
  uint32_t children_active;
};

static SdfDAGHeader get_default_SdfDAGHeader()
{
  SdfDAGHeader header;
  header.brick_size = 1;
  header.brick_pad = 0;
  header.node_grid_size = 2; //2x2x2 - typical octree node
  header.dim = 3;

  header.transform_subgroup = 0;
  header.user_params[0] = 0;
  header.user_params[1] = 0;
  header.user_params[2] = 0;
  header.user_params[3] = 0;
  header.user_params[4] = 0;
  header.user_params[5] = 0;
  header.user_params[6] = 0;

  header.tex_id_off = -1;
  header.mat_id_off = -1;
  header.all_float_tex_id_off = -1;
  header.all_int_mat_id_off = -1;

  return header;
}

static SCom2Header get_default_SCom2Header()
{
  SCom2Header header;

  header.brick_size = 1;
  header.v_size = 2;
  header.bits_per_value = 8;
  header.values_per_uint = 4;
  header.value_mask = 0xFFu;
  header.bitmask_len = 0;
  header.dimension = 3;

  header.child_rot_shift = 26;
  header.child_rot_mask = 0x0000003Fu;
  header.child_add_shift = 0;
  header.child_add_mask = 0x03FFFFFFu;
  header.child_offset_mask = 0xFFFFFFFFu;
  header.node_offset_mask = 0xFFFFFFFFu;
  header.child_offset_off = 1;
  header.uints_per_link = 2;
  header.unique_brick_prefix = 0x30 << header.child_rot_shift;
  header.unique_brick_offset_mask = 0xFFFFFFFFu;

  header.children_types_shift = 8;
  header.children_types_mask = 0xFFFFu;
  header.base_reference_shift = 24;
  header.children_active_bits_shift = 24;
  header.children_active_bits_mask = 0xFFu;
  header.references_offset = 1;
  header.reference_bits = 8;
  header.reference_mask = 0xFFu;
  header.references_per_uint = 4;
  header.links_offset = 2;
  header.max_surface_count = 15;
  header.max_surface_count_per_leaf = 15;

  header.bricks_step = 2;
  header.bricks_arr_offset = 0;
  header.nodes_arr_offset  = 0;
  header.root_node_off = 0;

  header.has_channels = 0;
  header.has_surfaces = 0;
  header.has_multi_nodes = 0;
  header.tex_id_off = -1;
  header.mat_id_off = -1;
  header.all_float_tex_id_off = -1;
  header.all_int_mat_id_off = -1;

  header.max_val = -1;
  header.max_depth = 1;

  header.user_params[0] = 0;
  header.user_params[1] = 0;
  header.user_params[2] = 0;
  header.user_params[3] = 0;
  header.user_params[4] = 0;
  header.user_params[5] = 0;
  header.user_params[6] = 0;

  return header;
};

static NodeHeadUnpacked unpack_node_head(const SCom2Header &header, uint32_t node0, uint32_t node1)
{
  NodeHeadUnpacked head_unpacked;
  head_unpacked.base_type = node0 & 0xFFu;

  if (header.has_multi_nodes != 0)
    head_unpacked.children_types = (node0 >> header.children_types_shift) & header.children_types_mask;
  else
    head_unpacked.children_types = (node1 >> header.children_types_shift) & header.children_types_mask;

  if (header.has_multi_nodes != 0)
    head_unpacked.base_links_end = (node0 >> header.base_reference_shift) & header.reference_mask;
  else
    head_unpacked.base_links_end = (head_unpacked.base_type == SCOM2_CHILD_EMPTY ? 0 : (header.has_surfaces == 0 ? header.has_channels : 1+header.has_channels));
  
  head_unpacked.children_active = (node0 >> header.children_active_bits_shift) & header.children_active_bits_mask;
  return head_unpacked;
}

static SdfDAGDataEdge unpack_data_edge(const SCom2Header &header, float max_val, uint32_t edge0, uint32_t edge1)
{
  SdfDAGDataEdge edge;
  bool is_unique = (edge0 & header.unique_brick_prefix) == header.unique_brick_prefix;
  bool no_add = is_unique || (header.child_add_mask == 0);
  if (header.max_val > 0)
    edge.add = no_add ? 0.0f : header.max_val * (2 * ((edge0 >> header.child_add_shift) & header.child_add_mask) / float(header.child_add_mask) - 1);
  else
    edge.add = no_add ? 0.0f : max_val * (2 * ((edge0 >> header.child_add_shift) & header.child_add_mask) / float(header.child_add_mask) - 1);

  edge.rotation_id = is_unique ? 0 : (edge0 >> header.child_rot_shift) & header.child_rot_mask;
  edge.data_offset = is_unique ? edge1 & header.unique_brick_offset_mask : edge1 & header.child_offset_mask;
  edge.type_id = 0;
  return edge;
}

static SdfDAGChildEdge unpack_child_edge(const SCom2Header &header, uint32_t edge0, uint32_t edge1)
{
  SdfDAGChildEdge edge;
  edge.rotation_id = (edge0 >> header.child_rot_shift) & header.child_rot_mask;
  edge.child_offset = edge1 & header.node_offset_mask;
  return edge;
}

static float get_max_sdf_val(float level_sz)
{
  return /*sqrt(3)*/ 1.7320508f * 2.0f / level_sz;
}

static float get_max_sdf_val(float level_sz, uint32_t dim)
{
  return sqrtf(float(dim)) * 2.0f / level_sz;
}

static uint32_t get_links_end(const SCom2Header &header, uint32_t child_n, uint32_t node1, uint32_t node2)//TODO: think about 4D here
{
  if (header.has_multi_nodes != 0)
  {
    uint32_t sh = child_n*header.reference_bits;
    return sh >= 32 ? ((node2 >> sh%32) & header.reference_mask) : ((node1 >> sh) & header.reference_mask);
  }
  else if (header.has_surfaces > 0)
  {
    uint32_t base_links_end = ((node1 & 0xFFu) == SCOM2_CHILD_EMPTY ? 0 : 1);
    uint32_t ch_active = (node1 >> header.children_active_bits_shift) & header.children_active_bits_mask;
    uint32_t ch_n = bitCount(ch_active & ((1<<(1+child_n))-1));
    return (1+header.has_channels)*(base_links_end)+ch_n;
  }
  else
  {
    uint32_t base_links_end = ((node1 & 0xFFu) == SCOM2_CHILD_EMPTY ? 0 : 1);
    uint32_t ch_active = (node1 >> header.children_active_bits_shift) & header.children_active_bits_mask;
    uint32_t ch_n = bitCount(ch_active & ((1<<(1+child_n))-1));
    return (header.has_channels)*(base_links_end+ch_n);
  }
}

static uint32_t DAG_pack_voxel_count_flags(uint32_t count, bool is_surface, bool is_inside)
{
  return count | (is_surface ? (1<<16) : 0) | (is_inside ? (1<<17) : 0);
}

static bool DAG_node_is_full(uint32_t voxel_count_flags)
{
  return (voxel_count_flags & 0xFFFFu) != 0;
}

static uint32_t DAG_extract_count(uint32_t voxel_count_flags)
{
  return voxel_count_flags & 0xFFFFu;
}

static bool DAG_extract_is_surface(uint32_t voxel_count_flags)
{
  return ((voxel_count_flags >> 16) & 0x1) > 0;
}

static bool DAG_extract_is_inside(uint32_t voxel_count_flags)
{
  return ((voxel_count_flags >> 17) & 0x1) > 0;
}