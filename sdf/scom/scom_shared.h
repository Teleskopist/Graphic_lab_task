#pragma once
#include "sdf/common/sdf_common_shared.h"

//enum COctreeNodePackMode
static constexpr unsigned COCTREE_NODE_PACK_MODE_DEFAULT        = 0; //32 bit per child info (32 bit child offset)
static constexpr unsigned COCTREE_NODE_PACK_MODE_SIM_COMP_FULL  = 1; //64 bit per child info (32 bit child offset, (8+24) bit transform code)
static constexpr unsigned COCTREE_NODE_PACK_MODE_SIM_COMP_SMALL = 2; //32 bit per child info (18 bit child offset, (6+8 ) bit transform code)

//enum COctreeLeafType
static constexpr unsigned COCTREE_LEAF_TYPE_NOT_A_LEAF          = 0;
static constexpr unsigned COCTREE_LEAF_TYPE_GRID                = 1; //all values are present, no bit fields, only range and quantized values
static constexpr unsigned COCTREE_LEAF_TYPE_BIT_PACK            = 2; //presence bit field for voxels/values, only for bricks with <= 64 values
static constexpr unsigned COCTREE_LEAF_TYPE_SLICES              = 3; //separate bit fields for each slice, better for big and/or padded bricks

static constexpr unsigned COCTREE_LEAF_TYPE_BITS                = 2;
static constexpr unsigned COCTREE_LEAF_TYPE_MASK                = 0x3;
static constexpr unsigned COCTREE_LOD_LEAF_TYPE_SHIFT           = 30;
static constexpr unsigned COCTREE_MAX_CHILD_INFO_SIZE           = 2; //size in uints
static constexpr unsigned COCTREE_USE_BEST_LEAF_TYPE            = 1000;

//Header is an ultimate descriptor of how COctreeV3 is stored in memory
//It has a lot of redundance and should not be filled manually
//Created in global_octree_to_COctreeV3 and used in render mostly
struct COctreeV3Header
{
  uint32_t brick_size;      //number of voxels in each brick, 1 to 16
  uint32_t brick_pad;       //how many additional voxels are stored on the borders, 0 is default, 1 is for tricubic filtration or normals smoothing
  uint32_t bits_per_value;  //6, 8, 10, 16, 32 bits per value is allowed
  uint32_t uv_size;         //0 if COctreeV3 is not textured, 1 for default (16 for u and v) and 2 for more precision (32 for u and v, not supported)
  uint32_t sim_compression; //0 or 1, indicates if similarity compression is used
  uint32_t lods;            //0 or 1
  
  uint32_t default_leaf_type; //enum COctreeLeafType
  uint32_t fallback_leaf_type;//enum COctreeLeafType
  
  uint32_t node_pack_mode;  
  // precomputed values for non-leaf nodes, fully determined by node_pack_mode
  uint32_t uints_per_child_info;
  uint32_t idx_mask;
  uint32_t idx_sh;
  uint32_t trans_off;
  uint32_t rot_mask;
  uint32_t add_mask;
};