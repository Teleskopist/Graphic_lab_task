#pragma once
#include "sdf/scom/scom_shared.h"
#include "utils/misc/scene_common.h"
#include "sdf/common/global_octree.h"
#include "core/scene_extension.h"
#include "blk/blk.h"

// Determines how the octree should be packed into COctreeV3.
// Doesn't deal with similarity compression (it is defined by scom::Settings)
// For other SDF types Settings and Header are the same thing, because they are simplier
struct COctreeV3Settings
{
  uint32_t brick_size = 1;      //number of voxels in each brick, 1 to 16
  uint32_t brick_pad = 0;       //how many additional voxels are stored on the borders, 0 is default, 1 is for tricubic filtration or normals smoothing
  uint32_t bits_per_value = 8;  //6, 8, 10, 16, 32 bits per value is allowed
  uint32_t uv_size = 0;         //0 if COctreeV3 is not textured, 1 for default (16 for u and v) and 2 for more precision (32 for u and v, not supported)
  uint32_t sim_compression = 0; //0 or 1, indicates if similarity compression is used
  uint32_t default_leaf_type = COCTREE_USE_BEST_LEAF_TYPE; //enum COctreeLeafType or COCTREE_USE_BEST_TYPE
  // If true, bricks can be saved as dense grid (COCTREE_LEAF_TYPE_GRID), if default_leaf_type
  // will result in larger memory footprint. It causes branching in shader and may slow it down.
  bool allow_fallback_to_unpacked_leaves = true;
  bool use_lods = false;
  bool print_build_stats = false;
};

//based on node_pack_mode and leaf_pack_mode, fill masks and other values dependant on them
static void fill_coctree_v3_header(COctreeV3Header &header)
{
  if (header.node_pack_mode == COCTREE_NODE_PACK_MODE_DEFAULT)
  {
    header.uints_per_child_info = 1;
    header.idx_sh = 0;
    header.trans_off = 0;
    header.idx_mask = 0xFFFFFFFFu;
    header.rot_mask = 0x00000000u;
    header.add_mask = 0x00000000u;
  }
  else if (header.node_pack_mode == COCTREE_NODE_PACK_MODE_SIM_COMP_FULL)
  {
    header.uints_per_child_info = 2;
    header.idx_sh = 0;
    header.trans_off = 1;
    header.idx_mask = 0xFFFFFFFFu;
    header.rot_mask = 0x0000003Fu;
    header.add_mask = 0x3FFFFF00u;
  }
  else if (header.node_pack_mode == COCTREE_NODE_PACK_MODE_SIM_COMP_SMALL)
  {
    header.uints_per_child_info = 1;
    header.idx_sh = 14;
    header.trans_off = 0;
    header.idx_mask = 0xFFFFC000u; //14-32 bits
    header.rot_mask = 0x0000003Fu; // 0- 6 bits
    header.add_mask = 0x00003FC0u; // 6-14 bits
  }

  //in OctreeIntersectV3 function, childInfo contains leaf type, rotation and add codes simultaneously
  assert((((header.rot_mask | header.add_mask) << COCTREE_LEAF_TYPE_BITS) >> COCTREE_LEAF_TYPE_BITS) == (header.rot_mask | header.add_mask));
  //even if child mask is shifted to the left, we should be able to detect non-leaf nodes with simple == 0 check
  assert(COCTREE_LEAF_TYPE_NOT_A_LEAF == 0);
}

static COctreeV3Header get_default_coctree_v3_header()
{
  COctreeV3Header header;

  header.brick_size = 1;
  header.brick_pad = 0;
  header.bits_per_value = 8;
  header.uv_size = 0;
  header.sim_compression = 0;
  header.lods = 0;
  
  header.default_leaf_type  = COCTREE_LEAF_TYPE_SLICES;
  header.fallback_leaf_type = COCTREE_LEAF_TYPE_GRID;

  header.node_pack_mode = COCTREE_NODE_PACK_MODE_DEFAULT;
  header.uints_per_child_info = 1;
  header.idx_mask = 0xFFFFFFFFu;
  header.idx_sh = 0;
  header.trans_off = 0;
  header.rot_mask = 0x00000000u;
  header.add_mask = 0x00000000u;

  return header;
}

struct COctreeV3
{
  static constexpr unsigned VERSION = 5; // change version if structure changes
  COctreeV3Header header = get_default_coctree_v3_header();
  std::vector<uint32_t> data;
};

struct COctreeV3View
{
  COctreeV3View() = default;
  COctreeV3View(const COctreeV3 &a_octree)
  {
    header = a_octree.header;
    data = a_octree.data.data();
    size = a_octree.data.size();
  }
  COctreeV3View(COctreeV3Header a_header, const std::vector<uint32_t> &a_data)
  {
    header = a_header;
    data = a_data.data();
    size = a_data.size();
  }
  COctreeV3Header header;
  const uint32_t *data = nullptr;
  uint32_t size = 0;
};

void save_coctree_v3(const COctreeV3View &scene, const std::string &path);
void load_coctree_v3(COctreeV3 &scene, const std::string &path);
ModelInfo get_info_coctree_v3(const COctreeV3View &scene);

namespace scom { struct Settings; }

void load_from_blk(COctreeV3Settings &settings, const Block *block);
void save_to_blk(const COctreeV3Settings &settings, Block *block);

void load_from_blk(scom::Settings &settings, const Block *block);
void save_to_blk(const scom::Settings &settings, Block *block);

namespace LiteScene
{
  REGISTER_TYPE(TYPE_COCTREE_V3,       SDFCOctreeGeometry,     COctreeV3,                       "sdf_coctree_v3",   save_coctree_v3,       load_coctree_v3,       get_info_coctree_v3)  
}

namespace sdf_converter
{
  COctreeV3 create_COctree_v3(SparseOctreeSettings settings, COctreeV3Settings header, 
                              const cmesh4::SimpleMesh &mesh, bool verbose = false);
  COctreeV3 create_COctree_v3(SparseOctreeSettings settings, COctreeV3Settings header, scom::Settings scom_settings, 
                              const cmesh4::SimpleMesh &mesh, bool verbose = false);
  COctreeV3 create_COctree_v3(SparseOctreeSettings settings,
                              COctreeV3Settings co_settings,
                              scom::Settings scom_settings,
                              MultithreadedDistanceFunction sdf,
                              unsigned max_threads,
                              bool verbose);
  void global_octree_to_COctreeV3(const GlobalOctree &octree, COctreeV3 &compact_octree, COctreeV3Settings co_settings);
  void global_octree_to_COctreeV3(const GlobalOctree &octree, COctreeV3 &compact_octree, 
                                  COctreeV3Settings co_settings, scom::Settings settings);
}