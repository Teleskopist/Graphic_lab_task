/*
This folder is an implementation of SCom Tree 2 - second generation of SCom Tree,
compact octree-like data structure for 3D geometry representation.

There are 4 public headers here:

1. scom_shared.h - data structures and helper functions for rendering, can be used both in C++ and shader code 
2. scom.h - main data structures, serialization and helper functions for rendering
3. scom_builder.h - interface for SCom Tree 2 builder
4. scom_utils.h - helper functions for working with SCom Tree 2, such as rotation tables, debug stuff etc.

There are 2 main data structures:

SdfDAG - directed acyclic graph, leaves contain SDF multi-brick (with optional channels), nodes have links
to children and their own SDF multi-bricks for LoDs. Links consist of a rotation code, offset and values 
shift in case of data links. 
SdfDAG is an intermediate data structure, uncompressed version of SCom2Tree. It can be rendered for debugging purposes.

SCom2Tree - SCom Tree 2, a main data strcture here. Compact, powerful and not very convenient to use. It stores all
data except channels in a flat uint32_t arrays - nodes and bricks.

SCom2Tree is created from the GlobalOctree with the following algorithm (top-level steps):
1) Convert GlobalOctree to SdfDAG. On this step no compression is performed, no data is lost. The resulted DAG is
   a tree with trivial links. 
global_octree_to_DAG8_direct() - create octree-like DAG (2x2x2 children)
make_shallow_DAG() - convert DAG8 to DAG64 to DAG512 etc., reduce height in half by increasing number of children

2) Perform similarity compression for SdfDAG. There are 3 types of similarity compression:
   1. for bricks, where every brick is a N*N*N grid of SDF values
   2. for channels
   3. for branches, where both SDF and channel values from branch are considered. Multiple levels of branches
      can be comressed.
   After compression, some data is lost and SdfDAG is no longer a tree. 
  DAG_similarity_compression() is a main function here.
  It uses clustering during the compression, see perform_clustering()

3) Convert SdfDAG to SCom2Tree. This step is mostly a data packing with bit fields and so on.
   There is not much quality loss here usual, only on SDF values quantization.
   However, it introduces some limitation on number of different elements because it uses a lot
   of short indices.

   pack_SCom2()

*/
#pragma once
#include "sdf/scom2/scom_shared.h"
#include "utils/misc/scene_common.h"
#include "utils/common/data_channel.h"
#include "core/scene_extension.h"
#include <vector>
#include <string>

//################################################################################
// CPU-specific functions and data structures
//################################################################################
struct SdfDAG
{
  SdfDAGHeader header = get_default_SdfDAGHeader();

  std::vector<SdfDAGNode> nodes;
  std::vector<SdfDAGChildEdge> child_edges;
  std::vector<SdfDAGDataEdge> data_edges; 
  std::vector<float> distances;

  std::vector<DataChannel> point_channels;
  std::vector<DataChannel> voxel_channels;
};
struct SCom2Tree
{
  SCom2Header header;
  std::vector<uint32_t> nodes;
  std::vector<uint32_t> bricks;

  std::vector<DataChannel> point_channels;
  std::vector<DataChannel> voxel_channels;
};

void save_sdf_DAG(const SdfDAG &scene, const std::string &path);
void load_sdf_DAG(SdfDAG &scene, const std::string &path);
ModelInfo get_info_sdf_DAG(const SdfDAG &scene);

void save_scom2(const SCom2Tree &scene, const std::string &path);
void load_scom2(SCom2Tree &scene, const std::string &path);
ModelInfo get_info_scom2(const SCom2Tree &scene);

namespace LiteScene
{
  REGISTER_TYPE(TYPE_SDF_DAG, SDFDAGGeometry, SdfDAG,    "sdf_dag", save_sdf_DAG, load_sdf_DAG, get_info_sdf_DAG)
  REGISTER_TYPE(TYPE_SCOM2,   SCom2Geometry,  SCom2Tree, "scom2",   save_scom2,   load_scom2,   get_info_scom2)
}