#include "sparse_octree_flood.h"
#include "utils/common/primitive_list_octree.h"
#include "sparse_octree_flood_codes.h"
#include "utils/common/position_hash.h"

#include <stack>
#include <array>
#include <set>
#include <chrono>

namespace sdf_converter
{
  bool equal(const uint4 &a, const uint4 &b);

  void fill_neigh_data(FloodedOctree &out_octree, unsigned node_id, FloodedOctreeNode::Neigh dir,
                       std::vector<unsigned> &local_offsets_stack)
  {
    local_offsets_stack.clear();
    unsigned comp = 1, axis = 1, depth = out_octree.nodes[node_id].depth;
    out_octree.nodes[node_id].neighs_offsets[dir] = out_octree.neighbours.size();
    out_octree.nodes[node_id].neighs_count[dir] = 0;
    switch(dir)
    {
      case FloodedOctreeNode::X_M:
        axis <<= 1;
        comp <<= 1;
      case FloodedOctreeNode::Y_M:
        axis <<= 1;
        comp <<= 1;
      case FloodedOctreeNode::Z_M:
        break;
      case FloodedOctreeNode::X_P:
        axis <<= 1;
      case FloodedOctreeNode::Y_P:
        axis <<= 1;
      case FloodedOctreeNode::Z_P:
        comp = 0;
        break;
      default:
        assert(false);
    }
    unsigned parent = out_octree.nodes[node_id].parent, local_pos = out_octree.nodes[node_id].local_position;
    while ((local_pos & axis) != comp && parent != 0)
    {
      local_offsets_stack.push_back(local_pos);
      local_pos = out_octree.nodes[parent].local_position;
      parent = out_octree.nodes[parent].parent;
    }
    if ((local_pos & axis) == comp)
    {
      local_offsets_stack.push_back(local_pos);
      while (local_offsets_stack.size() > 0 && out_octree.nodes[parent].offset != 0)
      {
        local_pos = local_offsets_stack.back() ^ axis;
        local_offsets_stack.pop_back();
        parent = out_octree.nodes[parent].offset + local_pos;
      }
    }
    if (parent != 0)
    {
      local_offsets_stack.clear();
      local_offsets_stack.push_back(parent);
      for (unsigned i = 0; i < local_offsets_stack.size(); ++i)
      {
        auto neigh_node = local_offsets_stack[i];
        if (out_octree.nodes[neigh_node].offset == 0)
        {
          out_octree.neighbours.push_back(neigh_node);
          out_octree.nodes[node_id].neighs_count[dir]++;
          continue;
        }
        for (unsigned j = 0; j < 8; ++j)
        {
          if ((j & axis) == comp) local_offsets_stack.push_back(out_octree.nodes[neigh_node].offset + j);
        }
      }
    }
  }

  void set_pos_code_rec(FloodedOctree &out_octree, unsigned node_id, uint4 code)
  {
    out_octree.nodes[node_id].pos_code = code;
    if (out_octree.nodes[node_id].offset == 0) return;
    for (unsigned i = 0; i < 8; ++i)
    {
      set_pos_code_rec(out_octree, out_octree.nodes[node_id].offset + i, 2*code + uint4((i & 4) >> 2, (i & 2) >> 1, i & 1, 0));
    }
  }

  void prim_octree_to_flooded_octree_linear(const PrimitiveListOctree &pl_octree, FloodedOctree &out_octree, unsigned flood_start_depth)
  {
    auto t1 = std::chrono::high_resolution_clock::now();
    out_octree.max_depth = 0;
    if (pl_octree.nodes.size() == 0) return;
    out_octree.nodes.resize(pl_octree.nodes.size());
    out_octree.nodes[0].offset = pl_octree.nodes[0].offset;
    out_octree.nodes[0].type = FloodedOctreeNode::UNKNOWN;
    out_octree.nodes[0].pid_intersect_count = pl_octree.nodes[0].pid_intersect_count;
    out_octree.nodes[0].parent = 0;
    out_octree.nodes[0].local_position = 0;
    for (auto j = 0; j < FloodedOctreeNode::N_C; ++j) 
    { 
      out_octree.nodes[0].neighs_offsets[j] = 0; 
      out_octree.nodes[0].neighs_count[j] = 0; 
    }
    out_octree.nodes[0].depth = 0;
    for (unsigned i = 0; i < out_octree.nodes.size(); ++i)
    {
      if (out_octree.nodes[i].offset != 0)
      {
        for (unsigned j = 0; j < 8; ++j)
        {
          auto node_id = out_octree.nodes[i].offset + j;
          if (out_octree.nodes[i].depth + 1 > flood_start_depth || pl_octree.nodes[node_id].offset == 0)
            out_octree.nodes[node_id].type = FloodedOctreeNode::INSIDE;
          else
            out_octree.nodes[node_id].type = FloodedOctreeNode::UNKNOWN;
          out_octree.nodes[node_id].pid_intersect_count = pl_octree.nodes[node_id].pid_intersect_count;
          out_octree.nodes[node_id].offset = pl_octree.nodes[node_id].offset;
          //out_octree.nodes[node_id].type = (out_octree.nodes[node_id].offset == 0) ? FloodedOctreeNode::INSIDE : FloodedOctreeNode::UNKNOWN;
          out_octree.nodes[node_id].parent = i;
          out_octree.nodes[node_id].depth = out_octree.nodes[i].depth + 1;
          out_octree.nodes[node_id].local_position = j;
          if (out_octree.max_depth < out_octree.nodes[i].depth + 1) out_octree.max_depth = out_octree.nodes[i].depth + 1;
        }
      }
    }

    auto t2 = std::chrono::high_resolution_clock::now();

    set_pos_code_rec(out_octree, 0, uint4(0, 0, 0, 1));

    auto t3 = std::chrono::high_resolution_clock::now();

    std::vector<int4> add_codes = {int4(0,0,1,0), int4(0,0,-1,0), int4(0,1,0,0), int4(0,-1,0,0), int4(1,0,0,0), int4(-1,0,0,0)};
        
    std::vector<unsigned> fill_neigh_data_stack;
    fill_neigh_data_stack.reserve(1024);
    for (unsigned i = 1; i < out_octree.nodes.size(); ++i)
    {
      for (unsigned j = FloodedOctreeNode::START; j < FloodedOctreeNode::N_C; ++j)
        fill_neigh_data(out_octree, i, (FloodedOctreeNode::Neigh)j, fill_neigh_data_stack);

      // printf("node (%d %d %d %d)\n", out_octree.nodes[i].pos_code.x, out_octree.nodes[i].pos_code.y, out_octree.nodes[i].pos_code.z, out_octree.nodes[i].pos_code.w);
      // for (int j=0;j<6;j++)
      // {
      //   printf("N%d: ", j);
      //   for (int k=0;k<out_octree.nodes[i].neighs_count[j];k++)
      //   {
      //     int n_idx = out_octree.neighbours[out_octree.nodes[i].neighs_offsets[j] + k];
      //     printf("(%d %d %d %d) ", out_octree.nodes[n_idx].pos_code.x, out_octree.nodes[n_idx].pos_code.y, out_octree.nodes[n_idx].pos_code.z, out_octree.nodes[n_idx].pos_code.w);
      //     if (out_octree.nodes[i].neighs_count[j] == 1)
      //     {
      //       assert(out_octree.nodes[i].offset == 0);
      //       assert(length(float4(int4(out_octree.nodes[n_idx].pos_code) - (int4(out_octree.nodes[i].pos_code)+add_codes[j]))) < 0.5f);
      //     }        
      //   }
      //   printf("\n");
      // }
      // printf("\n");
    }

    auto t4 = std::chrono::high_resolution_clock::now();
    //fill_flood_type(out_octree, 1000);
    fill_all_flood_type(out_octree, flood_start_depth);

    auto t5 = std::chrono::high_resolution_clock::now();
    // float time1 = std::chrono::duration<float, std::milli>(t2 - t1).count();
    // float time2 = std::chrono::duration<float, std::milli>(t3 - t2).count();
    // float time3 = std::chrono::duration<float, std::milli>(t4 - t3).count();
    // float time4 = std::chrono::duration<float, std::milli>(t5 - t4).count();
    // printf("Time: %f %f %f %f\n", time1, time2, time3, time4);
  }

  void fill_depth_flood_type(FloodedOctree &octree, unsigned depth)
  {
    std::array<std::set<unsigned>, 2> ring_buffer;
    uint32_t prev = 0;
    uint32_t  cur = 1;
    for (unsigned i = 0; i < octree.nodes.size(); ++i)
    {
      if (octree.nodes[i].depth == depth || (octree.nodes[i].offset == 0 && octree.nodes[i].depth <= depth))
      {
        if (octree.nodes[i].pid_intersect_count != 0)
        {
          bool is_border = false;
          for (unsigned j = FloodedOctreeNode::START; j < FloodedOctreeNode::N_C && !is_border; ++j)
          {
            if (octree.nodes[i].neighs_count[j] == 0) is_border = true;
          }
          if (is_border)
          {
            octree.nodes[i].type = FloodedOctreeNode::BORDER1;
            ring_buffer[prev].insert(i);
          }
          else
          {
            octree.nodes[i].type = FloodedOctreeNode::INSIDE;
          }
        }
        else
        {
          bool is_border = false;
          for (unsigned j = FloodedOctreeNode::START; j < FloodedOctreeNode::N_C && !is_border; ++j)
          {
            if (octree.nodes[i].neighs_count[j] == 0) is_border = true;
          }
          if (is_border)
          {
            octree.nodes[i].type = FloodedOctreeNode::OUTSIDE;
            ring_buffer[prev].insert(i);
          }
          else
          {
            octree.nodes[i].type = FloodedOctreeNode::INSIDE;
          }
        }
      }
    }

    int steps = 0;
    while (ring_buffer[prev].size() > 0)
    {
      steps++;
      ring_buffer[cur].clear();
      for (auto i : ring_buffer[prev])
      {
        for (unsigned j = FloodedOctreeNode::START; j < FloodedOctreeNode::N_C; ++j)
        {
          for (unsigned k = 0; k < octree.nodes[i].neighs_count[j]; ++k)
          {
            auto neigh = octree.neighbours[octree.nodes[i].neighs_offsets[j] + k];
            while (octree.nodes[neigh].depth > depth)
            {
              neigh = octree.nodes[neigh].parent;
            }
            if (octree.nodes[i].type == FloodedOctreeNode::OUTSIDE && octree.nodes[neigh].type == FloodedOctreeNode::INSIDE)
            {
              if (octree.nodes[neigh].pid_intersect_count == 0)
                octree.nodes[neigh].type = FloodedOctreeNode::OUTSIDE;
              else
                octree.nodes[neigh].type = FloodedOctreeNode::BORDER1;
              ring_buffer[cur].insert(neigh); 
            }
            else if (octree.nodes[i].type == FloodedOctreeNode::OUTSIDE && octree.nodes[neigh].type == FloodedOctreeNode::BORDER2)
            {
              octree.nodes[neigh].type = FloodedOctreeNode::BORDER1;
              ring_buffer[cur].insert(neigh);
            }
            else if (octree.nodes[i].type == FloodedOctreeNode::BORDER1 && (octree.nodes[neigh].type == FloodedOctreeNode::INSIDE || octree.nodes[neigh].type == FloodedOctreeNode::BORDER3) &&
                     octree.nodes[neigh].pid_intersect_count != 0)
            {
              octree.nodes[neigh].type = FloodedOctreeNode::BORDER2;
              ring_buffer[cur].insert(neigh);
            }
            else if (octree.nodes[i].type == FloodedOctreeNode::OUTSIDE && octree.nodes[neigh].type == FloodedOctreeNode::BORDER3)
            {
              octree.nodes[neigh].type = FloodedOctreeNode::BORDER1;
              ring_buffer[cur].insert(neigh);
            }
            else if (octree.nodes[i].type == FloodedOctreeNode::BORDER2 && octree.nodes[neigh].type == FloodedOctreeNode::INSIDE &&
                     octree.nodes[neigh].pid_intersect_count != 0)
            {
              octree.nodes[neigh].type = FloodedOctreeNode::BORDER3;
            }
          }
        }
      }
      prev = cur;
      cur = (cur + 1) % 2;
    }
  }

  void fill_all_flood_type(FloodedOctree &octree, unsigned flood_start_depth)
  {
    for (int i = octree.max_depth; i >= flood_start_depth; --i)
    {
      fill_depth_flood_type(octree, i);
    }
  }

  void fill_flood_type(FloodedOctree &octree, unsigned diff_out_voxel_size = 1)
  {
    std::vector<unsigned> refill_nodes;
    std::vector<unsigned> buffer;
    for (unsigned i = 0; i < octree.nodes.size(); ++i)
    {
      if (octree.nodes[i].offset == 0)
      {
        if (octree.nodes[i].pid_intersect_count != 0)
        {
          bool is_border = false;
          for (unsigned j = FloodedOctreeNode::START; j < FloodedOctreeNode::N_C && !is_border; ++j)
          {
            if (octree.nodes[i].neighs_count[j] == 0) is_border = true;
          }
          if (is_border)
          {
            octree.nodes[i].type = FloodedOctreeNode::BORDER1;
            buffer.push_back(i);
          }
          else
          {
            octree.nodes[i].type = FloodedOctreeNode::INSIDE;
          }
        }
        else if (octree.nodes[i].depth + diff_out_voxel_size <= octree.max_depth)
        {
          octree.nodes[i].type = FloodedOctreeNode::OUTSIDE;
          buffer.push_back(i);
        }
        else
        {
          bool is_border = false;
          for (unsigned j = FloodedOctreeNode::START; j < FloodedOctreeNode::N_C && !is_border; ++j)
          {
            if (octree.nodes[i].neighs_count[j] == 0) is_border = true;
          }
          if (is_border)
          {
            octree.nodes[i].type = FloodedOctreeNode::OUTSIDE;
            buffer.push_back(i);
          }
          else
          {
            octree.nodes[i].type = FloodedOctreeNode::INSIDE;
          }
        }
      }
      else
      {
        octree.nodes[i].type = FloodedOctreeNode::UNKNOWN;
      }
    }

    int steps = 0;
    while (buffer.size() > 0)
    {
      steps++;
      refill_nodes.resize(0);
      refill_nodes.insert(refill_nodes.begin(), buffer.begin(), buffer.end());
      buffer.resize(0);
      for (unsigned idx = 0; idx < refill_nodes.size(); ++idx)
      {
        int i = refill_nodes[idx];
        for (unsigned j = FloodedOctreeNode::START; j < FloodedOctreeNode::N_C; ++j)
        {
          for (unsigned k = 0; k < octree.nodes[i].neighs_count[j]; ++k)
          {
            auto neigh = octree.neighbours[octree.nodes[i].neighs_offsets[j] + k];
            if (octree.nodes[i].type == FloodedOctreeNode::OUTSIDE && octree.nodes[neigh].type == FloodedOctreeNode::INSIDE)
            {
              if (octree.nodes[neigh].pid_intersect_count == 0)
                octree.nodes[neigh].type = FloodedOctreeNode::OUTSIDE;
              else
                octree.nodes[neigh].type = FloodedOctreeNode::BORDER1;
              buffer.push_back(neigh); 
            }
            else if (octree.nodes[i].type == FloodedOctreeNode::OUTSIDE && octree.nodes[neigh].type == FloodedOctreeNode::BORDER2)
            {
              octree.nodes[neigh].type = FloodedOctreeNode::BORDER1;
              buffer.push_back(neigh);
            }
            else if (octree.nodes[i].type == FloodedOctreeNode::BORDER1 && (octree.nodes[neigh].type == FloodedOctreeNode::INSIDE || octree.nodes[neigh].type == FloodedOctreeNode::BORDER3) &&
                     octree.nodes[neigh].pid_intersect_count != 0)
            {
              octree.nodes[neigh].type = FloodedOctreeNode::BORDER2;
              buffer.push_back(neigh);
            }
            else if (octree.nodes[i].type == FloodedOctreeNode::OUTSIDE && octree.nodes[neigh].type == FloodedOctreeNode::BORDER3)
            {
              octree.nodes[neigh].type = FloodedOctreeNode::BORDER1;
              buffer.push_back(neigh);
            }
            else if (octree.nodes[i].type == FloodedOctreeNode::BORDER2 && octree.nodes[neigh].type == FloodedOctreeNode::INSIDE &&
                     octree.nodes[neigh].pid_intersect_count != 0)
            {
              octree.nodes[neigh].type = FloodedOctreeNode::BORDER3;
            }
          }
        }
      }
    }
  }

  std::vector<unsigned> get_active_vertices(unsigned sign)
  {
    std::vector<unsigned> res;
    for (unsigned i = 0; i < 8; ++i)
    {
      if ((sign & (1 << i)) == 0)
        res.push_back(i);
    }
    return res;
  }

  unsigned active_vertices_to_sign(const std::vector<unsigned> &verts)
  {
    unsigned sign = 0b11111111;
    for (unsigned i = 0; i < verts.size(); ++i)
    {
      sign -= (1 << verts[i]);
    }
    return sign;
  }

  std::vector<std::vector<unsigned>> get_connected_components(unsigned sign)
  {
    static unsigned vox_adj_matrix[8][8] = {
      //0  1  2  3  4  5  6  7
       {1, 1, 1, 0, 1, 0, 0, 0}, // 0
       {1, 1, 0, 1, 0, 1, 0, 0}, // 1
       {1, 0, 1, 1, 0, 0, 1, 0}, // 2
       {0, 1, 1, 1, 0, 0, 0, 1}, // 3
       {1, 0, 0, 0, 1, 1, 1, 0}, // 4
       {0, 1, 0, 0, 1, 1, 0, 1}, // 5
       {0, 0, 1, 0, 1, 0, 1, 1}, // 6
       {0, 0, 0, 1, 0, 1, 1, 1}  // 7
     };
    
    constexpr unsigned UNVISITED = 0;
    constexpr unsigned IN_STACK = 1;
    constexpr unsigned VISITED = 2;

    std::vector<std::vector<unsigned>> components;
    std::vector<unsigned> verts = get_active_vertices(sign);
    std::vector<unsigned> visited(verts.size(), UNVISITED);
    unsigned vertices_left = verts.size();
    while (vertices_left > 0)
    {
      components.emplace_back();
      unsigned cur_vert = 0;
      while (visited[cur_vert] != 0)
        cur_vert++;
      
      unsigned stack[64];
      unsigned top = 0;
      stack[top++] = cur_vert;

      while (top > 0)
      {
        unsigned cur = stack[--top];
        if (visited[cur] == IN_STACK)
        {
          visited[cur] = VISITED;
          components.back().push_back(verts[cur]);
          vertices_left--;
        }

        for (unsigned i = 0; i < verts.size(); ++i)
        {
          if (visited[i] == UNVISITED && vox_adj_matrix[verts[cur]][verts[i]] != 0)
          {
            stack[top++] = i;
            visited[i] = IN_STACK;
          }
        }
      }
    }

    // printf("%u : %d comps = {", sign, (int)components.size());
    // for (unsigned i = 0; i < components.size(); ++i)
    // {
    //   printf("{");
    //   for (unsigned j = 0; j < components[i].size(); ++j)
    //   {
    //     printf("%d", components[i][j]);
    //     if (j != components[i].size() - 1)
    //       printf(", ");
    //   }
    //   printf("}");
    //   if (i != components.size() - 1)
    //     printf(", ");
    // }
    // printf("}\n");

    return components;
  }

  // function to generate the sign-to-code table for N6 (nodes with 6 neighbours)
  // should be called only once at the start of the function
  void generate_N6_sign_to_code_table(unsigned out_code_table[256])
  {
    for (unsigned signs = 0; signs < 256; signs++)
    {
      auto cc = get_connected_components(signs);
      unsigned code = CODE_UNDECIDED;
      if (signs == 0)
      {
        code = TYPE_UNDECIDED_N64;
      }
      else if (signs == 0b11111111)
      {
        code = TYPE_VOLUME_INSIDE | NODE_EMPTY | NO_BRICKS | SIGNS_EMPTY;
      }
      else if (cc.size() == 1 && cc[0].size() <= 4)
      {
        unsigned sign0 = active_vertices_to_sign(cc[0]);
        code = TYPE_VOLUME_CORNER_INV | NODE_VOLUME | ONE_BRICK | sign0;
      }
      else if (cc.size() == 2)
      {
        unsigned sign0 = active_vertices_to_sign(cc[0]);
        unsigned sign1 = active_vertices_to_sign(cc[1]) << 8;
        code = TYPE_TWO_SURFACES | NODE_SURFACE | TWO_BRICKS | sign1 | sign0;
      }
      else
      {
        code = TYPE_UNDECIDED_N63;
      }

      out_code_table[signs] = code;
    }
  }

  bool is_border(const FloodedOctreeNode &node)
  {
    return node.type == FloodedOctreeNode::BORDER1 ||
           node.type == FloodedOctreeNode::BORDER2 ||
           node.type == FloodedOctreeNode::BORDER3;
  }

  bool is_full_node(const FloodedOctreeNode &node)
  {
    return node.type == FloodedOctreeNode::BORDER1 ||
           node.type == FloodedOctreeNode::BORDER2 ||
           node.type == FloodedOctreeNode::BORDER3 ||
           node.type == FloodedOctreeNode::INSIDE;
  }

  int2 border_neighbors_code(const FloodedOctree &octree, const FloodedOctreeNode &node)
  {
    int2 code = int2(0,0);
    for (int i=0;i<6;i++)
    {
      //printf("node %d %d %d %d n%d count %d\n", node.pos_code.x, node.pos_code.y, node.pos_code.z, node.pos_code.w, i, node.neighs_count[i]);
      if (node.neighs_count[i] > 1)
        code = int2(-1000,-1000);
      else if (node.neighs_count[i] == 1 && 
               is_full_node(octree.nodes[octree.neighbours[node.neighs_offsets[i]+0]]))
        code += int2(1, 1 << i);
    }
    //printf("code = %d %d\n", code.x, code.y);
    return code;
  }

  struct Indices
  {
    inline unsigned &operator[](int id) { return _values[id]; }
    unsigned _values[6] = {0};
  };

  Indices get_indices(unsigned code)
  {
    Indices ind;
    int cnt = 0;
    for (int i=0;i<6;i++)
    {
      if (code & (1<<i))
        ind[cnt++] = i;
    }
    return ind;
  }

  std::vector<uint32_t> calculate_node_codes(FloodedOctree &octree)
  {
    static bool filled_code_tables = false;
    static unsigned N6_code_table[256] = {0};

    if (!filled_code_tables)
    {
      generate_N6_sign_to_code_table(N6_code_table);
      filled_code_tables = true;
    }

    CodeMap<unsigned> code_map;

    unsigned max_code = 0;
    for (auto &node : octree.nodes)
      max_code = std::max(max_code, node.pos_code.w);
    
    for (auto &node : octree.nodes)
    {
      if (is_full_node(node))
      {
        // code_map is used to calculate the node codes for all levels
        // if node is leaf, it affects all levels from it's own level and below
        // if node is not leaf, it affects only it's own level
        bool is_leaf_node = node.offset == 0;
        uint32_t start_code_w = node.pos_code.w;
        uint32_t end_code_w = is_leaf_node ? max_code : start_code_w;
        for (uint32_t code_w = start_code_w; code_w <= end_code_w; code_w *= 2)
        {
          int N = code_w/node.pos_code.w;
          uint4 base_code = node.pos_code*N;
          for (int i=0;i<=N;i++)
          {
            for (int j=0;j<=N;j++)
            {
              for (int k=0;k<=N;k++)
              {
                int a = (i==0||i==N) + (j==0||j==N) + (k==0||k==N);
                if (a == 0) 
                  continue;
                int corners = a == 3 ? 1 : (a == 2 ? 2 : 4);
                uint4 code = base_code + uint4(i,j,k,0);
                if (code_map.find(code) == code_map.end())
                  code_map[code] = corners;
                else
                  code_map[code] += corners;
              }
            }
          }
        }
      }
    }

    std::stack<unsigned> nodes_stack;
    for (int i=0;i<octree.nodes.size();i++)
    {
      if (is_border(octree.nodes[i]))
        nodes_stack.push(i);
    } 

    int border_nodes = nodes_stack.size();
    int initial_border_nodes = nodes_stack.size();
    while (!nodes_stack.empty())
    {
      unsigned cur_node_id = nodes_stack.top();
      FloodedOctreeNode &node = octree.nodes[cur_node_id];
      nodes_stack.pop();

      int2 b = border_neighbors_code(octree, node);
      int3 planes = int3((b.y & 0b110000) > 0, (b.y & 0b001100) > 0, (b.y & 0b000011) > 0);
      Indices inds = get_indices(b.y);
      unsigned old_code = node.code;
      FloodedOctreeNode::NodeType old_type = node.type;

      if (!is_border(node))
        continue;

      bool clear_node = false;
      if (b.x == 0)
      {
        clear_node = true; //isolated node
        node.code = TYPE_ISOLATED | NODE_EMPTY | NO_BRICKS | SIGNS_EMPTY;
      }
      else if (b.x == 1)
      {
        clear_node = true; //wire edge
        node.code = TYPE_WIRE_EDGE | NODE_EMPTY | NO_BRICKS | SIGNS_EMPTY;
      }
      else if (b.x == 2)
      {
        if (planes.x + planes.y + planes.z == 1)
        {
          clear_node = true; //thin wire
          node.code = TYPE_WIRE | NODE_EMPTY | NO_BRICKS | SIGNS_EMPTY;
        }
        else
        {
          int2 b0 = border_neighbors_code(octree, octree.nodes[octree.neighbours[node.neighs_offsets[inds[0]]]]);
          int2 b1 = border_neighbors_code(octree, octree.nodes[octree.neighbours[node.neighs_offsets[inds[1]]]]);
          if (b0.x > 2 && b0.x < 5 && b1.x > 2 && b1.x < 5)
          {
            clear_node = false; //corner of thin plane
            unsigned signs = 0;
            if (planes.x == 0)
              signs = SIGNS_TWO_SURFACES_X;
            else if (planes.y == 0)
              signs = SIGNS_TWO_SURFACES_Y;
            else //if (planes.z == 0)
              signs = SIGNS_TWO_SURFACES_Z;
            node.code = TYPE_PLANE_CORNER | NODE_SURFACE | TWO_BRICKS | signs;
          }
          else
          {
            clear_node = true;  //bended thin wire or bulge on a surface
            node.code = TYPE_BULGE | NODE_EMPTY | NO_BRICKS | SIGNS_EMPTY;
          }
        }
      }
      else if (b.x == 3)
      {
        if (planes.x + planes.y + planes.z == 2)
        {
          clear_node = false; //edge of the surface
          unsigned signs = 0;
          if (planes.x == 0)
            signs = SIGNS_TWO_SURFACES_X;
          else if (planes.y == 0)
            signs = SIGNS_TWO_SURFACES_Y;
          else //if (planes.z == 0)
            signs = SIGNS_TWO_SURFACES_Z;
          node.code = TYPE_PLANE_EDGE | NODE_SURFACE | TWO_BRICKS | signs;
        }
        else
        {
          int c1 = ((b.y & 0b110000) >> 4) == 1 ? 1 : 0;
          int c2 = ((b.y & 0b001100) >> 2) == 1 ? 1 : 0;
          int c3 = ( b.y & 0b000011      ) == 1 ? 1 : 0;
          uint4 ca0 = uint4(c1,c2,c3,0);

          unsigned i0 = 4*ca0.x + 2*ca0.y + ca0.z;
          unsigned signs = 0b00000000;
          signs |= 1 << i0;

          bool internal_corner = code_map[node.pos_code + ca0] == 8;
          //int2 b0 = border_neighbors_code(octree, octree.nodes[octree.neighbours[node.neighs_offsets[inds[0]]]]);
          //int2 b1 = border_neighbors_code(octree, octree.nodes[octree.neighbours[node.neighs_offsets[inds[1]]]]);
          //int2 b2 = border_neighbors_code(octree, octree.nodes[octree.neighbours[node.neighs_offsets[inds[2]]]]);
          //if (b0.x > 2 && b0.x < 5 && b1.x > 2 && b1.x < 5 && b2.x > 2 && b2.x < 5)
          if (!internal_corner)
          {
            clear_node = false; //edge of bended surface
            unsigned second_plane_signs = (~signs & 0b11111111) << 8;
            node.code = TYPE_PLANE_EDGE_BEND | NODE_SURFACE | TWO_BRICKS | second_plane_signs | signs;
          }
          else
          {
            clear_node = false; // corner volume 
            node.code = TYPE_VOLUME_CORNER | NODE_VOLUME | ONE_BRICK | signs;  
          }   
        }
      }
      else if (b.x == 4)
      {
        if (planes.x + planes.y + planes.z == 2)
        {
          clear_node = false; // surface
          unsigned signs = 0;
          if (planes.x == 0)
            signs = SIGNS_TWO_SURFACES_X;
          else if (planes.y == 0)
            signs = SIGNS_TWO_SURFACES_Y;
          else //if (planes.z == 0)
            signs = SIGNS_TWO_SURFACES_Z;
          node.code = TYPE_PLANE | NODE_SURFACE | TWO_BRICKS | signs;
        }
        else
        {
          // int2 b0 = border_neighbors_code(octree, octree.nodes[octree.neighbours[node.neighs_offsets[inds[0]]]]);
          // int2 b1 = border_neighbors_code(octree, octree.nodes[octree.neighbours[node.neighs_offsets[inds[1]]]]);
          // int2 b2 = border_neighbors_code(octree, octree.nodes[octree.neighbours[node.neighs_offsets[inds[2]]]]);
          // int2 b3 = border_neighbors_code(octree, octree.nodes[octree.neighbours[node.neighs_offsets[inds[3]]]]);
          // if (b0.x > 2 && b0.x < 5 && b1.x > 2 && b1.x < 5 && b2.x > 2 && b2.x < 5 && b3.x > 2 && b3.x < 5)
          int c1 = ((b.y & 0b110000) >> 4) == 1 ? 1 : 0;
          int c2 = ((b.y & 0b001100) >> 2) == 1 ? 1 : 0;
          int c3 = ( b.y & 0b000011      ) == 1 ? 1 : 0;
          uint4 ca0, ca1;
          if ((b.y & 0b110000) == 0b110000)
          {
            ca0 = uint4(0, c2, c3, 0);
            ca1 = uint4(1, c2, c3, 0);
          }
          else if ((b.y & 0b001100) == 0b001100)
          {
            ca0 = uint4(c1, 0, c3, 0);
            ca1 = uint4(c1, 1, c3, 0);
          }
          else if ((b.y & 0b000011) == 0b000011)
          {
            ca0 = uint4(c1, c2, 0, 0);
            ca1 = uint4(c1, c2, 1, 0);
          }
          else
          {
            printf("error\n");
          }

          unsigned i0 = 4*ca0.x + 2*ca0.y + ca0.z;
          unsigned i1 = 4*ca1.x + 2*ca1.y + ca1.z;
          unsigned signs = 0b00000000;
          signs |= 1 << i0;
          signs |= 1 << i1;

          bool internal_edge = code_map[node.pos_code + ca0] == 8 && code_map[node.pos_code + ca1] == 8;
          if (!internal_edge)
          {
            clear_node = false; //bended surface
            unsigned second_plane_signs = (~signs & 0b11111111) << 8;
            node.code = TYPE_PLANE_BEND | NODE_SURFACE | TWO_BRICKS | second_plane_signs | signs;
          }
          else
          {
            clear_node = false; // edge volume
            node.code = TYPE_VOLUME_EDGE | NODE_VOLUME | ONE_BRICK | signs;
          }
        }
      }
      else if (b.x == 5)
      {
        // int2 b0 = border_neighbors_code(octree, octree.nodes[octree.neighbours[node.neighs_offsets[inds[0]]]]);
        // int2 b1 = border_neighbors_code(octree, octree.nodes[octree.neighbours[node.neighs_offsets[inds[1]]]]);
        // int2 b2 = border_neighbors_code(octree, octree.nodes[octree.neighbours[node.neighs_offsets[inds[2]]]]);
        // int2 b3 = border_neighbors_code(octree, octree.nodes[octree.neighbours[node.neighs_offsets[inds[3]]]]);
        // int2 b4 = border_neighbors_code(octree, octree.nodes[octree.neighbours[node.neighs_offsets[inds[4]]]]);
        //if (b0.x > 2 && b0.x < 5 && b1.x > 2 && b1.x < 5 && b2.x > 2 && b2.x < 5 && b3.x > 2 && b3.x < 5 && b4.x > 2 && b4.x < 5)
        uint4 ca[4];
        if (b.y == 0b011111)
        {
          ca[0] = uint4(1, 0, 0, 0);
          ca[1] = uint4(1, 0, 1, 0);
          ca[2] = uint4(1, 1, 0, 0);
          ca[3] = uint4(1, 1, 1, 0);
        }
        else if (b.y == 0b101111)
        {
          ca[0] = uint4(0, 0, 0, 0);
          ca[1] = uint4(0, 0, 1, 0);
          ca[2] = uint4(0, 1, 0, 0);
          ca[3] = uint4(0, 1, 1, 0);
        }
        else if (b.y == 0b110111)
        {
          ca[0] = uint4(0, 1, 0, 0);
          ca[1] = uint4(0, 1, 1, 0);
          ca[2] = uint4(1, 1, 0, 0);
          ca[3] = uint4(1, 1, 1, 0);
        }
        else if (b.y == 0b111011)
        {
          ca[0] = uint4(0, 0, 0, 0);
          ca[1] = uint4(0, 0, 1, 0);
          ca[2] = uint4(1, 0, 0, 0);
          ca[3] = uint4(1, 0, 1, 0);
        }
        else if (b.y == 0b111101)
        {
          ca[0] = uint4(0,0,1,0);
          ca[1] = uint4(0,1,1,0);
          ca[2] = uint4(1,0,1,0);
          ca[3] = uint4(1,1,1,0);
        }
        else if (b.y == 0b111110)
        {
          ca[0] = uint4(0,0,0,0);
          ca[1] = uint4(0,1,0,0);
          ca[2] = uint4(1,0,0,0);
          ca[3] = uint4(1,1,0,0);
        }
        else
        {
          printf("error\n");
        }

        unsigned signs = 0b00000000;
        int internal_corners = 0;
        for (int i=0;i<4;i++)
        {
          if (code_map[node.pos_code + ca[i]] == 8)
          {
            unsigned idx = 4*ca[i].x + 2*ca[i].y + ca[i].z;
            signs |= 1 << idx;
            internal_corners++;
          }
        }

        if (internal_corners < 4)
        {
          clear_node = false; //???
          node.code = TYPE_UNDECIDED_N5;
        }
        else
        {
          clear_node = false; // volume
          node.code = TYPE_VOLUME | NODE_VOLUME | ONE_BRICK | signs;
        }
      }
      else
      {
        int outside_corners = 0;
        int outside_corner_ids[8] = {-1};
        for (int i=0;i<8;i++)
        {
          uint4 ca = uint4((i & 4) >> 2, (i & 2) >> 1, i & 1, 0);
          if (code_map[node.pos_code + ca] != 8)
          {
            outside_corner_ids[outside_corners++] = i;
          }
        }

        unsigned signs = 0b11111111;
        for (int i=0;i<outside_corners;i++)
          signs -= (1 << outside_corner_ids[i]);
        //printf("signs code %d %d %d\n", signs, extract_signs_1(signs), extract_signs_2(signs));
        node.code = N6_code_table[signs];

        // if (outside_corners == 0)
        // {
        //   clear_node = false; //full volume
        //   node.code = TYPE_VOLUME_INSIDE | NODE_EMPTY | NO_BRICKS | 0;
        // }
        // else if (outside_corners < 5)
        // {
        //   unsigned signs = 0b11111111;
        //   for (int i=0;i<outside_corners;i++)
        //     signs -= (1 << outside_corner_ids[i]);

        //   bool is_ok = true;
        //   // bool is_ok = (signs & 0b11110000) == 0b11110000 ||
        //   // (signs & 0b00001111) == 0b00001111 ||
        //   // (signs & 0b11001100) == 0b11001100 ||
        //   // (signs & 0b00110011) == 0b00110011 ||
        //   // (signs & 0b10101010) == 0b10101010 ||
        //   // (signs & 0b01010101) == 0b01010101;
        //   if (is_ok)
        //   {
        //     clear_node = false; //inverted corner (7 minus/1 plus signs)
        //     //if (outside_corners == 1)
        //       node.code = TYPE_VOLUME_CORNER_INV | NODE_VOLUME| ONE_BRICK | signs; 
        //     //else if (outside_corners == 2)
        //     //  node.code = TYPE_VOLUME_EDGE_INV_6 | NODE_VOLUME| ONE_BRICK | signs; 
        //     //else if (outside_corners == 3)
        //     //  node.code = TYPE_VOLUME_EDGE_INV_5 | NODE_VOLUME| ONE_BRICK | signs; 
        //   }
        //   else
        //   {
        //     node.code = TYPE_UNDECIDED_N61;
        //   }
        // }
        // else
        // {
        //   // printf("undecided 62 type %d\n", (int)node.type);
        //   // for (int i=0;i<6;i++)
        //   // {
        //   //   uint4 pos_code = octree.nodes[octree.neighbours[node.neighs_offsets[inds[i]]]].pos_code;
        //   //   printf("N%d - type %d pos code %d %d %d %d\n", i, (int)octree.nodes[octree.neighbours[node.neighs_offsets[inds[i]]]].type,
        //   //   pos_code.x, pos_code.y, pos_code.z, pos_code.w);
        //   // }
        //   // printf("\n");
        //   // for (int i=0;i<8;i++)
        //   // {
        //   //   uint4 ca = uint4((i & 4) >> 2, (i & 2) >> 1, i & 1, 0);
        //   //   printf("corner %d - %d inside\n", i, code_map[node.pos_code + ca]);
        //   // }
        //   clear_node = false; //???
        //   node.code = TYPE_UNDECIDED_N62;         
        // }
      }

      if (clear_node)
      {
        if (is_border(node))
        {
          border_nodes--;
        }
        node.type = FloodedOctreeNode::DELETED_BORDER;
      }

      if (node.code != old_code || node.type != old_type)
      {
        for (int i=0;i<b.x;i++)
        {
          if (is_border(octree.nodes[octree.neighbours[node.neighs_offsets[inds[i]]]]))
            nodes_stack.push(octree.neighbours[node.neighs_offsets[inds[i]]]);
        }
      }
    }

    int type_stat[64] = {0};
    for (int i=0;i<octree.nodes.size();i++)
    {
      if (octree.nodes[i].type == FloodedOctreeNode::BORDER1 ||
          octree.nodes[i].type == FloodedOctreeNode::BORDER2 ||
          octree.nodes[i].type == FloodedOctreeNode::BORDER3 ||
          octree.nodes[i].type == FloodedOctreeNode::DELETED_BORDER)
      {
        int type = octree.nodes[i].code >> 24;
        type_stat[type]++;
      }
    }

    // printf("type stat:\n");
    // for (int i=0;i<type_names.size();i++)
    // {
    //   printf("%s: %d\n", type_names[i], type_stat[i]);
    // }
    // printf("\n");

    // int code_stat[9] = {0};
    // for (auto &p : code_map)
    // {
    //   code_stat[p.second]++;
    // }

    // printf("code stat:\n");
    // for (int i=0;i<9;i++)
    // {
    //   printf("%d corners: %d\n", i, code_stat[i]);
    // }
    // printf("\n");

    std::vector<uint32_t> res_codes(octree.nodes.size());
    for (int i=0;i<octree.nodes.size();i++)
    {
      if (octree.nodes[i].type == FloodedOctreeNode::INSIDE)
        res_codes[i] = TYPE_VOLUME_INSIDE | NODE_EMPTY | NO_BRICKS | 0;
      else if (octree.nodes[i].code == 0)
        res_codes[i] = TYPE_UNKNOWN | NODE_EMPTY | NO_BRICKS | (int)octree.nodes[i].type;
      else
        res_codes[i] = octree.nodes[i].code;
    }
    return res_codes;
  }
}