#pragma once
#include "sdf_common_shared.h"
#include "utils/mesh/mesh.h"
#include "utils/common/data_channel.h"
#include "blk/blk.h"

#include <functional>

struct SparseOctreeSettings
{
  SparseOctreeSettings() = default;
  explicit SparseOctreeSettings(unsigned _depth, bool _fill_all_nodes = false)
  {
    depth = _depth;
    min_depth = _depth;
    split_thr = 0.0f;
    fill_all_nodes = _fill_all_nodes;
  }
  SparseOctreeSettings(unsigned _depth, float _split_thr, unsigned _min_depth = 1, bool _fill_all_nodes = false)
  {
    depth = _depth;
    split_thr = _split_thr;
    min_depth = _min_depth;
    fill_all_nodes = _fill_all_nodes;
    allow_early_stop = _min_depth > 0;
  }

  enum class VoxelMetric
  {
    BASIC,
    SAMPLES,
    MULTI,
    REFERENCE_IOU
  };

  enum class LocalDepthMethod
  {
    PRIMITIVES,
    MATERIALS,
    AV_SQUARE,
    MIN_SQUARE,
    MAX_SQUARE,
    COMPONENTS,
    SURFACES,
    ABS_METRIC,
    SIGN_ANALYSIS,
    NORMAL_ERROR
  };

  unsigned brick_size = 1; // number of voxels in each brick, (min 1)
  unsigned brick_pad = 0;  // how many additional voxels are stored on the borders, 0 is default
  unsigned depth = 7;     //max depth of octree
  unsigned internal_depth = 0; //max depth for internal nodes of an octree (when fill_internal_volume = true), if set to 0, "depth" parameter is used
  unsigned min_depth = 1; //if node depth is less, it will be split no matter if it reduces error or not
  unsigned local_inc_depth = 0;//additional depth for max depth in local bad cases
  float split_thr = 0.0f; //threshold for early stop and pruning
  float local_depth_thr = 0.0f;//threshold for add inc depth methods
  bool  fill_all_nodes = false; //if false, only leaf nodes will be filled with actual distances
  bool  fill_internal_volume = false; //if true, internal volume will be split up to max depth similar to boundary
  bool  allow_multi_nodes = false; //if surface in node cannot be represented with one SDF surface, builder will
                                   //create multiple ones. Global Octree built with this flag must be converted to Multi Octree
  bool  allow_nodes_refill = false; //if it is allowed to call refill_node for some leaf nodes (trying to find plane to estimate
                                    //surface in voxel)
  bool advanced_LOD_generation = false; //build LODs with special bottom-up algorithm, used only when fill_all_nodes is true
  bool sign_analysis = false; //if true, distance field signs will be determined by a separate procedure
  bool allow_early_stop = false; //if true, early stop by split_thr will be enabled
  bool use_pruning = false; //if true, nodes will be removed if they decrease error by less than split_thr

  bool use_point_cloud = false;
  unsigned points_count = 100;
  unsigned max_iterations = 1000;
  float inlier_threshold = 0.001f;
  float enough_points_percent = 0.85f;
  float minimum_iterations = 0.1f;
  float learning_rate = 0.01f;
  unsigned gradient_steps = 100;
  float momentum = 0.9;
  float decay_rate = 0.95;
  float channel_split_thr = 0.0f; //threshold for channel-based pruning

  VoxelMetric voxel_metric = VoxelMetric::MULTI;//method for pruning and early stop
  LocalDepthMethod local_depth_method = LocalDepthMethod::PRIMITIVES;//method for local inc depth applying
  
  bool calculate_tc = false;
  bool calculate_mat_id = false;
};

/*
Converter is a static and unified interface for creating different 
spatial structures to represent Signed Distance Filed.

Every structure can be created from mesh or abstract distance function (std::function)
version with multithreaded function is also provided, but not every building method
will actually use multiple threads.

Converter builds structures to represent SDF in fixed region - [-1,1]^3
Thus mesh should be scaled to unit cube [-1,1]^3 (can be done with cmesh4::normalize_mesh)
Unnormalized mesh will probably be non-watertight in [-1,1]^3 and produce bad result.
*/
namespace sdf_converter
{
  using MultithreadedDistanceFunction = std::function<float(const float3 &, unsigned idx)>;

  struct PointAttributes
  {
    float distance = 1000;
    float2 tex_coord = float2(0, 0);
    unsigned mat = 0u;
    float3 closest_point = float3(1e9, 1e9, 1e9);
    unsigned closest_primitive_id = 0xFFFFFFFFu;
  };

  enum class GlobalOctreeNodeType : uint8_t
  {
    EMPTY,      //leaf with no surface or node with an empty subtree (the latter is usually removed during build)
    LEAF,       //leaf node containing surface (i.e. both positive and negative values)
    EMPTY_NODE, //node with 8 children, but it's own values doesn't represent surface (all values have the same sign)
    NODE        //node with 8 children, containing surface (i.e. both positive and negative values)
  };
  
  struct GlobalOctreeHeader
  {
    unsigned dim = 3; // dimentionality of the GlobalOctree, 2 for quadtree, 3 for octree (default), 4 for ???-tree
    unsigned brick_size = 1; // number of voxels in each brick, (min 1)
    unsigned brick_pad = 0;  // how many additional voxels are stored on the borders, 0 is default
    int tc_channel_id = -1;  // if octree has texture coordinates as one of it's channels, tc_channel_id is a number of this channel
    int mat_channel_id = -1; // if octree has material id as one of it's channels, mat_channel_id is a number of this channel
  };

  struct GlobalOctreeNode
  {
    GlobalOctreeNodeType type = GlobalOctreeNodeType::EMPTY;
    bool is_outside    = true; // used to mark empty external nodes (set to true by default)
    bool is_surfaced  = false; // is it a surface of volume node    
    bool is_internal  = false; // is it an internal node (i.e. has no surface, only channel data)
    unsigned val_off      = 0; // offset in values_f vectors
    unsigned offset       = 0; // offset in nodes vector for next child (0 if its leaf)
    unsigned bricks_count = 0; // number of bricks in this node, empty nodes have 0, internal nodes - 1, regular >= 1
  };

  struct GlobalOctreeDebugInfo
  {
    enum CreationType
    {
      UNKNOWN,               //node was created in method without debug info support or memory was corrupted
      EMPTY,                 //node was initialized as empty with no further actions with it
      FILL_NODE,             //node was created with fill_node method (default way)
      FILL_NODE_MULTI,       //node was created with fill_node_multi method
      REFILL_NODE,           //node was rebuild with refill_node method
      FILL_LOD,              //node was created with fill_LOD_node method from its children
      FROM_SDF,              //node was created from analytic SDF (add_global_node_rec method)
      MERGE,                 //node was created during octree merge process 
      MERGE_WITH_VOXEL_MERGE //node was created during octree merge process and its surfaces were merged
    };
    struct NodeInfo
    {
      CreationType creation_type = UNKNOWN;    //how the node was created
      unsigned primitives_count = 0;           //how many primitives was in PLO node used to create this node
      unsigned surfaces_count = 0;             //how many surfaces this node has
      unsigned connected_components_count = 0; //how many connected components was detected in primitive list used to create this node
      unsigned original_node_id = 0;           //original (before merge) id of this node
      unsigned part_id = 0;                    //id of part used to create this node, 0 if the octree was not created by merge
      bool is_surface_node = false;            //surface or volume node
      uint4 position_code = uint4(0,0,0,0);    //(x,y,z,max_size) - x,y,z in grid [0, max_size]
      int merged_nodes_offset = -1;            //offset in merged_nodes_info, -1 if the octree was not created by merge
      unsigned merged_nodes_count = 0;
      unsigned prim_octree_node_code = 0u;
      unsigned initial_signs = 0u;            //if distance values' signs are changed, this will contain initial signs for first 4 surfaces
      float add_depth_error   = -1.0f;        //error, calculated when determining if node should be split beyond base depth
      float error_from_parent = -1.0f;        //if node was tested for pruning it will contain error from parent, -1 if not tested
                                              //for merged nodes it will contain maximum error from all merged nodes
    };

    std::vector<NodeInfo> nodes_info; //node info for each node
    std::vector<NodeInfo> merged_nodes_info;    //node info from octrees that were merged to create this one
  };

  struct GlobalOctree
  {
    // Although GlobalOctree can theoreticaly handle deeper octrees, meny rendering methods rely on uint16 values to store current depth
    // If the octree is 1m in size, the max depth depth voxel is ~15 um, so 16 is enough for any practical case
    static constexpr unsigned MAX_OCTREE_DEPTH = 16;

    // Max brick size in voxels, can be increased if really needed, but should check SBS implementation first
    // Note, that SCom Tree is limited to 8x8x8 voxels bricks
    static constexpr unsigned MAX_BRICK_SIZE = 16;

    // No hard limitation here, just an arbitrary number to use for allocating memory on stack buring SDF build. Feel free to change
    static constexpr unsigned MAX_SURFACE_COUNT = 64;

    GlobalOctreeHeader header;
    std::vector<GlobalOctreeNode> nodes;
    std::vector<float> values_f;

    std::vector<DataChannel> point_channels;
    std::vector<DataChannel> voxel_channels;

    GlobalOctreeDebugInfo *debug_info = nullptr; //must be manually set before call to mesh_octree_to_global_octree if debug info is needed
  };

  struct GlobalOctreeBuildStat
  {
    SparseOctreeSettings settings;
    unsigned active_nodes = 0;

    //all time in ms
    float time_build_PLO = 0;
    float time_build_flooded_octree = 0;
    float time_sign_analysis = 0;
    float time_calculate_distances = 0;
    float time_decide_to_divide = 0;
    float time_mark_empty_nodes = 0;
    float time_eliminate_empty_nodes = 0;
    float time_pruning = 0;
  };

  void print_octree_build_stat(const GlobalOctreeBuildStat &stat);
}

void load_from_blk(SparseOctreeSettings &settings, const Block *block);
void save_to_blk(const SparseOctreeSettings &settings, Block *block);