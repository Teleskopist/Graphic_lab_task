#include "voxel_dag.h"
#include "voxel_octree_internal.h"
#include "utils/common/data_channel.h"
#include "sdf/scom2/clustering/symmetric_groups.h"

//there are no distances in the voxel, so all leaves just reference one "stub" distance grid
SdfDAGDataEdge get_stub_data_edge()
{
  SdfDAGDataEdge edge;
  edge.add = 0.0f;
  edge.data_offset = 0;
  edge.rotation_id = 0;
  edge.type_id = 0;
  return edge;
}

void sparse_voxel_octree_to_DAG_rec(const SparseVoxelOctree &octree, SdfDAG &dag, uint32_t nodeId, bool isLeaf)
{
  if (isLeaf)
  {
    uint32_t dagNodeId = dag.nodes.size();
    dag.nodes.emplace_back();
    SdfDAGNode node;
    node.voxel_count_flags = DAG_pack_voxel_count_flags(1, false, true);
    node.children_edges_offset = 0;
    node.data_edges_offset = dag.data_edges.size();
    node.channels_edge.rotation_id = 0;
    node.channels_edge.child_offset = dag.voxel_channels[0].data_i.size();

    dag.data_edges.push_back(get_stub_data_edge());
    dag.nodes[dagNodeId] = node;
    dag.voxel_channels[0].data_i.push_back(octree.data[nodeId]);
  }
  else
  {
    uint32_t dagNodeId = dag.nodes.size();
    dag.nodes.emplace_back();
    SdfDAGNode node;
    node.voxel_count_flags = DAG_pack_voxel_count_flags(1, false, true);
    node.children_edges_offset = dag.child_edges.size();
    node.data_edges_offset = dag.data_edges.size();

    node.channels_edge.rotation_id = 0;
    node.channels_edge.child_offset = dag.voxel_channels[0].data_i.size();
    dag.voxel_channels[0].data_i.push_back(0); //no blocks in this node, only in leaves
    dag.data_edges.push_back(get_stub_data_edge());

    dag.child_edges.resize(node.children_edges_offset + 8);

    PackedOctreeNode octree_node = get_node(octree.data[nodeId]);
    uint32_t childrenOffset = octree_node.is_far ? octree.data[octree_node.short_ptr + nodeId] : (octree_node.short_ptr + nodeId);
    uint32_t ch_n = 0;
    for (int i = 0; i < 8; i++)
    {
      SdfDAGChildEdge edge;
      edge.rotation_id = 0;
      if (octree_node.child_has_data & (1 << i))
      {
        edge.child_offset = dag.nodes.size();
        bool is_leaf = octree_node.child_is_leaf & (1 << i);
        sparse_voxel_octree_to_DAG_rec(octree, dag, childrenOffset + ch_n, is_leaf);
        ch_n++;
      }
      else
      {
        edge.child_offset = 0;
      }
      dag.child_edges[node.children_edges_offset + i] = edge;
    }
    dag.nodes[dagNodeId] = node;
  }
}

void sparse_voxel_octree_to_DAG(const SparseVoxelOctree &octree, SdfDAG &dag)
{
  dag.header = get_default_SdfDAGHeader();
  dag.header.dim = 3;
  dag.header.brick_size = 1;
  dag.header.transform_subgroup = (uint32_t)scom2::TransformSubgroup::DEFAULT; //48 elements - rotations and mirroring

  dag.distances.resize(8, -1.0f);
  dag.nodes.reserve(octree.data.size());
  dag.child_edges.reserve(octree.data.size());
  dag.data_edges.reserve(octree.data.size());
  
  dag.voxel_channels.emplace_back();
  dag.voxel_channels[0].type = DataChannel::Type::INT;
  dag.voxel_channels[0].num_components = 1;
  dag.voxel_channels[0].data_i.reserve(octree.data.size());

  dag.data_edges.emplace_back(); //0 data is reserved to indicate empty node
  dag.child_edges.emplace_back();
  sparse_voxel_octree_to_DAG_rec(octree, dag, 0, false);
  uint32_t emptyChildId = dag.nodes.size();

  //In some places it is assumed that DAG has only valid children, so add empty node
  //and link it to all empty children
  SdfDAGNode emptyNode;
  emptyNode.voxel_count_flags = DAG_pack_voxel_count_flags(0, false, false);
  emptyNode.children_edges_offset = 0;
  emptyNode.data_edges_offset = 0;
  emptyNode.channels_edge.rotation_id = 0;
  emptyNode.channels_edge.child_offset = 0;
  dag.nodes.push_back(emptyNode);

  for (auto &childEdge : dag.child_edges)
  {
    if (childEdge.child_offset == 0)
      childEdge.child_offset = emptyChildId;
  }

  //print_DAG_rec(dag, 0);

  dag.nodes.shrink_to_fit();
  dag.child_edges.shrink_to_fit();
  dag.data_edges.shrink_to_fit();
  dag.voxel_channels[0].data_i.shrink_to_fit();
}