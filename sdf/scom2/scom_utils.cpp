#include "scom_utils.h"
#include "clustering/symmetric_groups.h"
#include "utils/misc/scene_common.h"

void save_sdf_DAG(const SdfDAG &scene, const std::string &path)
{
  std::ofstream fs(path, std::ios::binary);
  unsigned nodes_size = scene.nodes.size();
  unsigned child_edges_size = scene.child_edges.size();
  unsigned data_edges_size = scene.data_edges.size();
  unsigned distances_size = scene.distances.size();
  unsigned vc_count = scene.point_channels.size();
  unsigned fc_count = scene.voxel_channels.size();
  fs.write((const char *)&nodes_size, sizeof(unsigned));
  fs.write((const char *)&child_edges_size, sizeof(unsigned));
  fs.write((const char *)&data_edges_size, sizeof(unsigned));
  fs.write((const char *)&distances_size, sizeof(unsigned));
  fs.write((const char *)&vc_count, sizeof(unsigned));
  fs.write((const char *)&fc_count, sizeof(unsigned));
  fs.write((const char *)&scene.header, sizeof(SdfDAGHeader));

  fs.write((const char *)scene.nodes.data(), scene.nodes.size() * sizeof(SdfDAGNode));
  fs.write((const char *)scene.child_edges.data(), scene.child_edges.size() * sizeof(SdfDAGChildEdge));
  fs.write((const char *)scene.data_edges.data(), scene.data_edges.size() * sizeof(SdfDAGDataEdge));
  fs.write((const char *)scene.distances.data(), scene.distances.size() * sizeof(float));

  for (auto &ch : scene.point_channels)
    save_data_channel(fs, ch);

  for (auto &ch : scene.voxel_channels)
    save_data_channel(fs, ch);
  
  fs.flush();
  fs.close();
}

void load_sdf_DAG(SdfDAG &scene, const std::string &path)
{
  std::ifstream fs(path, std::ios::binary);
  unsigned nodes_size = 0;
  unsigned child_edges_size = 0;
  unsigned data_edges_size = 0;
  unsigned distances_size = 0;
  unsigned vc_count = 0;
  unsigned fc_count = 0;
  fs.read((char *)&nodes_size, sizeof(unsigned));
  fs.read((char *)&child_edges_size, sizeof(unsigned));
  fs.read((char *)&data_edges_size, sizeof(unsigned));
  fs.read((char *)&distances_size, sizeof(unsigned));
  fs.read((char *)&vc_count, sizeof(unsigned));
  fs.read((char *)&fc_count, sizeof(unsigned));
  fs.read((char *)&scene.header, sizeof(SdfDAGHeader));

  scene.nodes.resize(nodes_size);
  scene.child_edges.resize(child_edges_size);
  scene.data_edges.resize(data_edges_size);
  scene.distances.resize(distances_size);
  scene.point_channels.resize(vc_count);
  scene.voxel_channels.resize(fc_count);

  fs.read((char *)scene.nodes.data(), nodes_size * sizeof(SdfDAGNode));
  fs.read((char *)scene.child_edges.data(), child_edges_size * sizeof(SdfDAGChildEdge));
  fs.read((char *)scene.data_edges.data(), data_edges_size * sizeof(SdfDAGDataEdge));
  fs.read((char *)scene.distances.data(), distances_size * sizeof(float));

  for (auto &ch : scene.point_channels)
    load_data_channel(fs, ch);

  for (auto &ch : scene.voxel_channels)
    load_data_channel(fs, ch);
  
  fs.close();
}

ModelInfo get_info_sdf_DAG(const SdfDAG &scene)
{
  size_t octree_size = scene.nodes.size() * sizeof(SdfDAGNode) + 
                       scene.child_edges.size() * sizeof(SdfDAGChildEdge) + 
                       scene.data_edges.size() * sizeof(SdfDAGDataEdge) + 
                       scene.distances.size() * sizeof(float) +
                       sizeof(SdfDAGHeader);

  size_t augment_size = 0;
  for (auto &ch : scene.point_channels)
    augment_size += calculate_bytesize(ch);

  for (auto &ch : scene.voxel_channels)
    augment_size += calculate_bytesize(ch);

  ModelInfo info;
  info.name = "sdf_dag";
  info.bytesize = octree_size + augment_size;
  info.num_primitives = scene.nodes.size();

  return info;
}

void save_scom2(const SCom2Tree &scene, const std::string &path)
{
  std::ofstream fs(path, std::ios::binary);

  uint32_t magic_number = SCOM2_MAGIC_NUMBER;
  uint32_t version = SCOM2_VERSION;
  uint32_t num_nodes = scene.nodes.size();
  uint32_t num_bricks = scene.bricks.size();
  uint32_t vc_size = scene.voxel_channels.size();
  uint32_t pc_size = scene.point_channels.size();

  fs.write((const char *)&magic_number, sizeof(uint32_t));
  fs.write((const char *)&version, sizeof(uint32_t));
  fs.write((const char *)&num_nodes, sizeof(uint32_t));
  fs.write((const char *)&num_bricks, sizeof(uint32_t));
  fs.write((const char *)&vc_size, sizeof(uint32_t));
  fs.write((const char *)&pc_size, sizeof(uint32_t));
  fs.write((const char *)&scene.header, sizeof(SCom2Header));

  fs.write((const char *)scene.nodes.data(), scene.nodes.size() * sizeof(uint32_t));
  fs.write((const char *)scene.bricks.data(), scene.bricks.size() * sizeof(uint32_t));

  for (auto &ch : scene.voxel_channels)
    save_data_channel(fs, ch);

  for (auto &ch : scene.point_channels)
    save_data_channel(fs, ch);

  fs.flush();
  fs.close();
}

void load_scom2_legacy(SCom2Tree &scene, const std::string &path)
{
  printf("Loading legacy SCom2 file (without channels). Please save the scene to repack it into the new format.\n");
  std::ifstream fs(path, std::ios::binary);

  uint32_t num_nodes = 0;
  uint32_t num_bricks = 0;

  fs.read((char *)&scene.header, sizeof(SCom2Header));
  fs.read((char *)&num_nodes, sizeof(uint32_t));
  fs.read((char *)&num_bricks, sizeof(uint32_t));

  scene.nodes.resize(num_nodes);
  scene.bricks.resize(num_bricks);

  fs.read((char *)scene.nodes.data(), num_nodes * sizeof(uint32_t));
  fs.read((char *)scene.bricks.data(), num_bricks * sizeof(uint32_t));

  fs.close();
}

void load_scom2(SCom2Tree &scene, const std::string &path)
{
  std::ifstream fs(path, std::ios::binary);

  uint32_t magic_number = 0;
  uint32_t version = 0;
  uint32_t num_nodes = 0;
  uint32_t num_bricks = 0;
  uint32_t vc_count = 0;
  uint32_t pc_count = 0;

  fs.read((char *)&magic_number, sizeof(uint32_t));

  if (magic_number != SCOM2_MAGIC_NUMBER)
  {
    fs.close();
    load_scom2_legacy(scene, path);
    return;
  }

  fs.read((char *)&version, sizeof(uint32_t));

  if (version != SCOM2_VERSION)
  {
    fs.close();
    printf("[ERROR] SCom2 version mismatch (save is version %u, current version is %u)\n", version, SCOM2_VERSION);
    return;
  }

  fs.read((char *)&num_nodes, sizeof(uint32_t));
  fs.read((char *)&num_bricks, sizeof(uint32_t));
  fs.read((char *)&vc_count, sizeof(uint32_t));
  fs.read((char *)&pc_count, sizeof(uint32_t));
  fs.read((char *)&scene.header, sizeof(SCom2Header));

  scene.nodes.resize(num_nodes);
  scene.bricks.resize(num_bricks);

  fs.read((char *)scene.nodes.data(), num_nodes * sizeof(uint32_t));
  fs.read((char *)scene.bricks.data(), num_bricks * sizeof(uint32_t));

  scene.voxel_channels.resize(vc_count);
  scene.point_channels.resize(pc_count);

  for (auto &ch : scene.voxel_channels)
    load_data_channel(fs, ch);

  for (auto &ch : scene.point_channels)
    load_data_channel(fs, ch);

  fs.close();
}

ModelInfo get_info_scom2(const SCom2Tree &scene)
{
  ModelInfo info;
  info.name = "scom2";
  info.bytesize = sizeof(SCom2Header) + 
                  scene.nodes.size() * sizeof(uint32_t) + 
                  scene.bricks.size() * sizeof(uint32_t);
  info.num_primitives = scene.nodes.size();
  return info;
}

namespace scom2
{
  static bool equal(const uint4 &a, const uint4 &b)
  {
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
  }

  uint2 get_node_offset_rot_from_pos_code(const SCom2Tree &tree, const uint4 &pos_code)
  {
    static bool rot_initialized = false;
    static std::vector<int4> rot_modifiers;
    static std::vector<uint32_t> rot_add_table;
    if (!rot_initialized)
    {
      rot_initialized = true;
      initialize_rot_modifiers(rot_modifiers, 2);
      const scom2::Subgroup *bricks_sg = scom2::create_subgroup(scom2::TransformSubgroup::DEFAULT, 2);
      uint32_t subgroup_size = bricks_sg->elements.size();
      assert(subgroup_size == ROT_COUNT);
      rot_add_table.resize(subgroup_size * subgroup_size, 0);
      for (int i = 0; i < subgroup_size; i++)
        for (int j = 0; j < subgroup_size; j++)
          rot_add_table[i*subgroup_size + j] = bricks_sg->cayley_table[j][i];
    }

    uint4 cur_pos_code = uint4(0, 0, 0, 1);
    uint32_t cur_off = tree.header.root_node_off;
    uint32_t cur_rot = 0;
    bool cur_node_ok = true;

    //printf("getting offset for node [%d %d %d %d]\n", pos_code.x, pos_code.y, pos_code.z, pos_code.w);
    while (cur_node_ok && !equal(cur_pos_code, pos_code))
    {
      NodeHeadUnpacked node = unpack_node_head(tree.header, tree.nodes[cur_off], tree.nodes[cur_off + tree.header.links_offset - 1]);
      uint32_t node1 = tree.nodes[cur_off + tree.header.references_offset];
      uint32_t node2 = tree.nodes[cur_off + tree.header.links_offset - 1];

      uint4 child_pos = pos_code / (pos_code.w / cur_pos_code.w / 2) - 2 * cur_pos_code;
      uint32_t child_id = uint32_t(dot(rot_modifiers[cur_rot], int4(child_pos)));

      //printf("pos [%d %d %d %d], off %d, rot %d - loading child [%d %d %d %d]\n", 
      //       cur_pos_code.x, cur_pos_code.y, cur_pos_code.z, cur_pos_code.w, cur_off, cur_rot, child_pos.x, child_pos.y, child_pos.z, child_pos.w);

      uint32_t ch_type = (node.children_types >> SCOM2_CHILD_TYPE_BITS * child_id) & SCOM2_CHILD_TYPE_MASK;
      uint32_t ch_start_link = child_id == 0 ? node.base_links_end : get_links_end(tree.header, child_id - 1, node1, node2);

      //printf("child type %d\n", ch_type);
      if (ch_type == SCOM2_CHILD_NODE)
      {
        uint32_t link_off = cur_off + tree.header.links_offset + ch_start_link * tree.header.uints_per_link;
        SdfDAGChildEdge ce = unpack_child_edge(tree.header, tree.nodes[link_off],
                                               tree.nodes[link_off + tree.header.child_offset_off]);
        //printf("child offset %d, rot %d\n", ce.child_offset, ce.rotation_id);
        cur_off = ce.child_offset;
        cur_rot = rot_add_table[cur_rot * scom2::ROT_COUNT + ce.rotation_id];
        cur_pos_code = 2 * cur_pos_code + child_pos;
      }
      else
      {
        cur_node_ok = false;
      }
      //printf("\n");
    }

    return cur_node_ok ? uint2(cur_off, cur_rot) : uint2(INVALID_IDX, INVALID_IDX);
  }

  void print_DAG_rec(const SdfDAG &dag, uint32_t node_id)
  {
    uint32_t ch_count = int_pow(dag.header.node_grid_size, dag.header.dim);
    const SdfDAGNode &node = dag.nodes[node_id];
    printf("node %d\n", node_id);
    if (node.children_edges_offset == 0)
      printf("  leaf\n");
    else
    {
      printf(" %d children: ", ch_count);
      for (int i = 0; i < ch_count; i++)
      {
        uint32_t child_node_id = dag.child_edges[node.children_edges_offset + i].child_offset;
        uint32_t rot_id = dag.child_edges[node.children_edges_offset + i].rotation_id;
        printf("%d(%d) ", child_node_id, rot_id);
      }
      printf("\n");
    }
    printf("%d surface(s): ", DAG_extract_count(node.voxel_count_flags));
    for (int i = 0; i < (DAG_extract_count(node.voxel_count_flags)); i++)
    {
      auto e = dag.data_edges[node.data_edges_offset + i];
      printf("%d (%d %d %f) ", e.data_offset, e.rotation_id, e.type_id, e.add);
    }
    printf("\n");

    if (node.children_edges_offset != 0)
    {
      for (int i = 0; i < ch_count; i++)
      {
        uint32_t child_node_id = dag.child_edges[node.children_edges_offset + i].child_offset;
        if (child_node_id != 0)
          print_DAG_rec(dag, child_node_id);
      }
    }
  }

  uint32_t calculate_DAG_max_depth_rec(const SdfDAG &dag, uint32_t nodeId)
  {
    const SdfDAGNode &node = dag.nodes[nodeId];
    if (node.children_edges_offset == 0)
      return 1;
    else
    {
      uint32_t max_depth = 0;
      for (int i = 0; i < int_pow(dag.header.node_grid_size, dag.header.dim); i++)
      {
        auto off = dag.child_edges[node.children_edges_offset + i].child_offset;
        if (off != 0)
          max_depth = std::max(max_depth, calculate_DAG_max_depth_rec(dag, off));
      }
      return max_depth + 1;
    }
  }

  void traverse_DAG_rec(const SdfDAG &dag, const dag_callback_t &callback, const scom2::Subgroup *subgroup,
                        uint32_t children_count, uint32_t nodeId, uint32_t transformId, uint32_t level, uint4 code)
  {
    const SdfDAGNode &node = dag.nodes[nodeId];
    bool continue_traversal = callback(dag, nodeId, transformId, level, code);
    if (continue_traversal && node.children_edges_offset != 0)
    {
      for (int i = 0; i < children_count; i++)
      {
        uint32_t child_node_id = dag.child_edges[node.children_edges_offset + i].child_offset;
        uint32_t rotId = dag.child_edges[node.children_edges_offset + i].rotation_id;
        uint32_t child_transformId = subgroup->cayley_table[rotId][transformId];
        uint32_t child_level = level + 1;
        uint32_t childIdx = subgroup->elements[subgroup->inverse_indices[transformId]][i];
        uint4 child_code = dag.header.node_grid_size*code + scom2::idx_to_code(childIdx, dag.header.node_grid_size);
        if (child_node_id != 0)
          traverse_DAG_rec(dag, callback, subgroup, children_count, child_node_id, child_transformId, child_level, child_code);
      }
    }
  }

  void traverse_DAG(const SdfDAG &dag, dag_callback_t callback)
  {
    //code is uint4, so no more than 4 dimensions
    assert(dag.header.dim <= 4);

    const scom2::Subgroup *subgroup = scom2::create_subgroup((scom2::TransformSubgroup)dag.header.transform_subgroup,
                                                            dag.header.node_grid_size, dag.header.dim);
    uint32_t ch_count = int_pow(dag.header.node_grid_size, dag.header.dim);
    traverse_DAG_rec(dag, callback, subgroup, ch_count, 0, 0, 0, uint4(0,0,0,1));
  }

  void rotate_DAG(LiteMath::float3x3 rot, SdfDAG &dag)
  {
    assert(dag.header.dim == 3);
    assert(dag.header.transform_subgroup == (uint32_t)scom2::TransformSubgroup::DEFAULT);

    //first, find the transposition id corresponding to the rotation
    std::vector<float4x4> rot_transforms;
    scom2::initialize_rot_transforms(rot_transforms, dag.header.node_grid_size);

    int transposition_id = -1;
    for (int i = 0; i < rot_transforms.size(); i++)
    {
      float diff = 0.0f;
      for (int j=0;j<9;j++)
        diff += abs(rot_transforms[i][j/3][j%3] - rot[j/3][j%3]);
      if (diff < 1e-9f)
      {
        transposition_id = i;
        break;
      }
    }

    if (transposition_id == -1)
    {
      printf("[rotate_DAG] ERROR: given rotation was not found among the primitive rotations. Rotation: [%f %f %f, %f %f %f, %f %f %f]\n",
            rot[0][0], rot[0][1], rot[0][2], rot[1][0], rot[1][1], rot[1][2], rot[2][0], rot[2][1], rot[2][2]);
      return;
    }

    const auto *subgroup = scom2::create_subgroup(scom2::TransformSubgroup::DEFAULT, dag.header.node_grid_size, dag.header.dim);

    uint32_t root_id = 0;
    uint32_t ch_count = int_pow(dag.header.node_grid_size, dag.header.dim);
    std::vector<SdfDAGChildEdge> ch_edges(ch_count);

    for (int i = 0; i < ch_count; i++)
      ch_edges[i] = dag.child_edges[dag.nodes[root_id].children_edges_offset + i];
      
    for (int i = 0; i < ch_count; i++)
    {
      auto edge = ch_edges[subgroup->elements[transposition_id][i]];
      edge.rotation_id = subgroup->cayley_table[transposition_id][edge.rotation_id];
      dag.child_edges[dag.nodes[root_id].children_edges_offset + i] = edge;
    }
  }
}