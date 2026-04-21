#pragma once
#include "sdf/common/sdf_common_shared.h"

// enum SBSInSide
static constexpr unsigned SBS_IN_SIDE_X_NEG = 0u;
static constexpr unsigned SBS_IN_SIDE_X_POS = 1u;
static constexpr unsigned SBS_IN_SIDE_Y_NEG = 2u;
static constexpr unsigned SBS_IN_SIDE_Y_POS = 3u;
static constexpr unsigned SBS_IN_SIDE_Z_NEG = 4u;
static constexpr unsigned SBS_IN_SIDE_Z_POS = 5u;

// enum SdfSBSNodeLayout
static constexpr unsigned SDF_SBS_NODE_LAYOUT_UNDEFINED        = 0 << 24u; //should be interpreted as SDF_SBS_NODE_LAYOUT_D for legacy reasons
static constexpr unsigned SDF_SBS_NODE_LAYOUT_DX               = 1 << 24u; //v_size^3 distance values (<bytes_per_value> bytes each)
static constexpr unsigned SDF_SBS_NODE_LAYOUT_DX_UV16          = 2 << 24u; //v_size^3 distance values (<bytes_per_value> bytes each), 8 tex coords (2*2 bytes each)
static constexpr unsigned SDF_SBS_NODE_LAYOUT_DX_RGB8          = 3 << 24u; //v_size^3 distance values (<bytes_per_value> bytes each), 8 RBG colors (4 bytes, with padding)
static constexpr unsigned SDF_SBS_NODE_LAYOUT_ID32F_IRGB32F    = 4 << 24u; //v_size^3 indices to distance values (1 float each), 8 indices to RBG colors (3 float)
static constexpr unsigned SDF_SBS_NODE_LAYOUT_ID32F_IRGB32F_IN = 5 << 24u; //v_size^3 indices to distance values (1 float each), 8 indices to RBG colors (3 float),
                                                                           //27 indices to adjacent bricks, INVALID_IDX indicates that the is no adjacent 
                                                                           //voxel on this side, otherwise it is an index of this brick in nodes array
static constexpr unsigned SDF_SBS_NODE_LAYOUT_MASK             = 0xFF000000;

struct SdfFrameOctreeNode
{
  float values[8];
  unsigned offset; // offset for children (they are stored together). 0 offset means it's a leaf  
};

//node for SparseVoxelSet, basically the same as SdfFrameOctree, but more compact
struct SdfSVSNode
{
  uint32_t pos_xy; //position of voxel in it's LOD
  uint32_t pos_z_lod_size; //size of it's LOD, (i.e. 2^LOD)
  uint32_t values[2]; //compressed distance values, 1 byte per value
};

//node for SparseBrickSet, similar idea to SparseVoxelSet, but values stored in bricks of KxKxK voxels,
//and values are shared between voxels, which leads to smaller memory footprint
struct SdfSBSNode
{
  uint32_t pos_xy; //position of start voxel of the block in it's LOD
  uint32_t pos_z_lod_size; //size of it's LOD, (i.e. 2^LOD)
  uint32_t data_offset; //offset in data vector for block with distance values, offset is in uint32_t, not bytes 
  uint32_t _pad;
};

//headed contains some info about particular SBS, such as size of a brick, precision of stored values etc
//and also some precomputed values based on them to reduce the number of calculations during rendering
struct SdfSBSHeader
{
  uint32_t brick_size;      //number of voxels in each brick, 1 to 16
  uint32_t brick_pad;       //how many additional voxels are stored on the borders, 0 is default, 1 is required for tricubic filtration
  uint32_t bytes_per_value; //1, 2 or 4 bytes per value is allowed
  uint32_t aux_data;        //SdfSBSNodeLayout
};

struct SdfFrameOctreeTexNode
{
  float tex_coords[16];
  float values[8];
  unsigned offset; // offset for children (they are stored together). 0 offset means it's a leaf  
  unsigned material_id;
};