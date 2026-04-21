#include "sparse_octree_builder_core.h"

namespace sdf_converter
{
  void surface_by_N_points(const std::vector<float3> &points, float &A, float &B, float &C, float &D, int iter, bool verbose)
  {
    if (points.size() == 0) return;
    float XX = 0, YY = 0, ZZ = 0, XY = 0, XZ = 0, YZ = 0, X = 0, Y = 0, Z = 0;

    for (int i = 0; i < points.size(); ++i)
    {
      X += points[i].x;
      Y += points[i].y;
      Z += points[i].z;
    }
    for (int i = 0; i < points.size(); ++i)
    {
      XX += (points[i].x - X / points.size()) * (points[i].x - X / points.size());
      XY += (points[i].x - X / points.size()) * (points[i].y - Y / points.size());
      XZ += (points[i].x - X / points.size()) * (points[i].z - Z / points.size());
      YY += (points[i].y - Y / points.size()) * (points[i].y - Y / points.size());
      YZ += (points[i].y - Y / points.size()) * (points[i].z - Z / points.size());
      ZZ += (points[i].z - Z / points.size()) * (points[i].z - Z / points.size());
    }
    float3x3 matr = LiteMath::make_float3x3_from_rows(float3(XX, XY, XZ), float3(XY, YY, YZ), float3(XZ, YZ, ZZ));
    float3x3 res = LiteMath::make_float3x3_from_rows(float3(1, 0, 0), float3(0, 1, 0), float3(0, 0, 1));
    float3x3 buffer1 = LiteMath::make_float3x3_from_rows(float3(1, 0, 0), float3(0, 1, 0), float3(0, 0, 1));
    float3x3 buffer2 = LiteMath::make_float3x3_from_rows(float3(1, 0, 0), float3(0, 1, 0), float3(0, 0, 1));

    for (int i = 0; i < iter; ++i)
    {
      // if (points.size() == 3) printf("%f %f %f / %f %f %f / %f %f %f\n", matr[0][0], matr[0][1], matr[0][2], 
      //                                            matr[1][0], matr[1][1], matr[1][2], 
      //                                            matr[2][0], matr[2][1], matr[2][2]);
      float phi = LiteMath::M_PI / 2.0f, mx = std::abs(matr[0][1]);
      int k = 0, n = 1;
      buffer1 = LiteMath::make_float3x3_from_rows(float3(1, 0, 0), float3(0, 1, 0), float3(0, 0, 1));
      buffer2 = LiteMath::make_float3x3_from_rows(float3(1, 0, 0), float3(0, 1, 0), float3(0, 0, 1));
      if (std::abs(matr[0][2]) > mx) {mx = std::abs(matr[0][2]); k = 0; n = 2;}
      if (std::abs(matr[1][2]) > mx) {mx = std::abs(matr[1][2]); k = 1; n = 2;}
      if (std::abs(matr[k][k] - matr[n][n]) >= 1e-8) phi = std::atan(2 * matr[k][n] / (matr[k][k] - matr[n][n])) * 0.5f;
      buffer1[k][k] = cos(phi);
      buffer1[n][n] = cos(phi);
      buffer1[k][n] = -sin(phi);
      buffer1[n][k] = sin(phi);

      buffer2[k][k] = cos(phi);
      buffer2[n][n] = cos(phi);
      buffer2[k][n] = sin(phi);
      buffer2[n][k] = -sin(phi);

      matr = buffer2 * matr * buffer1;
      res = res * buffer1;
      
    }
    float lambda = matr[0][0];
    int idx = 0;
    if (lambda > matr[1][1]) {lambda = matr[1][1]; idx = 1;}
    if (lambda > matr[2][2]) {lambda = matr[2][2]; idx = 2;}
    A = res[0][idx]; B = res[1][idx]; C = res[2][idx];
    D = -(A * X + B * Y + C * Z) / points.size();
    //if (points.size() == 3) printf("ABCD: %f %f %f %f\n", A, B, C, D);
  }

  unsigned create_missing_data_voxel_list(float vox_data[8], 
    const std::pair<float, float> edges_data[64],//for voxel edge contains min/max intersection offset
    std::vector<float> &values_f, bool verbose)//add at the end of vector n voxels and returns n
  {
    std::vector<unsigned> buffer;
    std::set<unsigned> unused_corners = {0, 1, 2, 3, 4, 5, 6, 7};
    std::vector<std::set<unsigned>> connected_corners;
    while (unused_corners.size() > 0)
    {
      unsigned corner = *unused_corners.begin();
      unused_corners.erase(corner);
      connected_corners.emplace_back();
      connected_corners.back().insert(corner);
      buffer.resize(0);
      buffer.push_back(corner);
      for (unsigned i = 0; i < buffer.size(); ++i)
      {
        for (unsigned j = 1; j < 8; j <<= 1)
        {
          if (unused_corners.find(buffer[i] ^ j) != unused_corners.end())
          {
            if (edges_data[buffer[i] * 8 + (buffer[i] ^ j)].first < 0)
            {
              connected_corners.back().insert(buffer[i] ^ j);
              unused_corners.erase(buffer[i] ^ j);
              buffer.push_back(buffer[i] ^ j);
            }
          }
        }
      }
    }

    std::vector<std::vector<std::pair<unsigned, unsigned>>> edges_for_connections;
    for (auto k : connected_corners)
    {
      buffer = {0};
      edges_for_connections.emplace_back();
      unused_corners = {1, 2, 3, 4, 5, 6, 7};
      unsigned idx = 0;
      while (unused_corners.size() > 0)
      {
        for (unsigned i = idx; i < buffer.size(); ++i)
        {
          for (unsigned j = 1; j < 8; j <<= 1)
          {
            if (unused_corners.find(buffer[i] ^ j) != unused_corners.end() && 
              ((k.find(buffer[i] ^ j) != k.end() && k.find(buffer[i]) == k.end()) || 
                (k.find(buffer[i] ^ j) == k.end() && k.find(buffer[i]) != k.end())))
            {
              edges_for_connections.back().push_back({buffer[i], buffer[i] ^ j});
              buffer.push_back(buffer[i] ^ j);
              unused_corners.erase(buffer[i] ^ j);
            }
          }
        }
        idx = buffer.size();
        unsigned corner = 8;
        for (auto i : unused_corners)
          if (i < corner)
            corner = i;
        if (corner < 8)
        {
          for (unsigned j = 1; j < 8; j <<= 1)
          {
            if ((corner ^ j) < corner)
            {
              edges_for_connections.back().push_back({corner ^ j, corner});
              buffer.push_back(corner);
              unused_corners.erase(corner);
            }
          }
        }
      }
    }

    unsigned result = 0;
    if (connected_corners.size() > 1)
    {
      for (unsigned i = 0; i < connected_corners.size(); ++i)
      {
        std::vector<float3> points;
        unused_corners = {1, 2, 3, 4, 5, 6, 7};
        buffer = {0};
        int idx = 0;
        float v[8] = {0}, A = 1, B = 1, C = 1, D = 1;
        for (auto j : connected_corners[i])
        {
          for (unsigned k = 1; k < 8; k <<= 1)
          {
            if (connected_corners[i].find(j ^ k) == connected_corners[i].end())
            {
              if ((j ^ k) > j)
              {
                float3 point = float3(j >> 2, (j >> 1) & 1, j & 1);
                float3 offset = float3(k >> 2, (k >> 1) & 1, k & 1) * edges_data[j * 8 + (j ^ k)].first;
                points.push_back(point + offset);
              }
              else
              {
                float3 point = float3((j ^ k) >> 2, ((j ^ k) >> 1) & 1, (j ^ k) & 1);
                float3 offset = float3(k >> 2, (k >> 1) & 1, k & 1) * (1.0f - edges_data[j * 8 + (j ^ k)].first);
                points.push_back(point + offset);
              }
            }
          }
        }

        surface_by_N_points(points, A, B, C, D);
        v[0] = D;
        v[1] = D + C;
        v[2] = D + B;
        v[3] = D + C + B;
        v[4] = D + A;
        v[5] = D + C + A;
        v[6] = D + B + A;
        v[7] = D + C + B + A;
        for (unsigned l = 0; l < 8; ++l)
        {
          values_f.push_back(v[l]);
        }
        ++result;
      }
    }
    return result;
  }

  void refill_node(sdf_converter::GlobalOctree &out_octree, std::vector<std::vector<sdf_converter::NodeBuildData>> &checks, 
                   int layer, int i, unsigned int v_size, const sdf_converter::PrimitiveListOctree &pl_octree, SparseOctreeSettings &settings, 
                   std::pair<float, float> edges_data[64], const cmesh4::SimpleMesh &mesh)
  {
    int plo_node_idx = checks[layer][i].prim_list_idx;
    if (pl_octree.nodes[plo_node_idx].pid_intersect_count > 0)
    {
      float3 pos = 2.0f * (checks[layer][i].d * checks[layer][i].pos) - 1.0f;
      for (int tri=0; tri<pl_octree.nodes[plo_node_idx].pid_count; tri++)
      {
        int t_i = pl_octree.primitive_ids[pl_octree.nodes[plo_node_idx].pid_offset+tri];
        float3 a = to_float3(mesh.vPos4f[mesh.indices[3*t_i+0]]);
        float3 b = to_float3(mesh.vPos4f[mesh.indices[3*t_i+1]]);
        float3 c = to_float3(mesh.vPos4f[mesh.indices[3*t_i+2]]);
        for (unsigned n = 0; n < 8; ++n)
        {
          for (unsigned m = 1; m < 8; m <<= 1)
          {
            if ((n | m) != n)
            {
              float3 ch_pos_1 = pos + 2 * (checks[layer][i].d / out_octree.header.brick_size) * 
                                    float3(n >> 2, (n >> 1) & 1, n & 1);
              float3 ch_pos_2 = pos + 2 * (checks[layer][i].d / out_octree.header.brick_size) * 
                                    float3((n | m) >> 2, ((n | m) >> 1) & 1, (n | m) & 1);
              float offset = -1;

              if (cmesh4::intersect_segment_triangle(ch_pos_1, ch_pos_2, a, b, c, offset))
              {
                if (edges_data[8 * n + (n | m)].first < 0)
                {
                  edges_data[8 * n + (n | m)].first = offset;
                  edges_data[8 * n + (n | m)].second = offset;
                  edges_data[8 * (n | m) + n].first = (1 - offset);
                  edges_data[8 * (n | m) + n].second = (1 - offset);
                }
                else
                {
                  if (edges_data[8 * n + (n | m)].first > offset)
                  {
                    edges_data[8 * n + (n | m)].first = offset;
                    edges_data[8 * (n | m) + n].second = (1 - offset);
                  }
                  if (edges_data[8 * n + (n | m)].second < offset)
                  {
                    edges_data[8 * n + (n | m)].second = offset;
                    edges_data[8 * (n | m) + n].first = (1 - offset);
                  }
                }
              }
            }
          }
        }
      }
      int node_idx = checks[layer][i].global_idx;
      bool need_refill = false;
      for (unsigned n = 0; n < 8 && !need_refill; ++n)
      {
        for (unsigned m = 1; m < 8 && !need_refill; m <<= 1)
        {
          if ((n | m) != n)
          {
            float sgn = out_octree.values_f[out_octree.nodes[node_idx].val_off + n] * 
                        out_octree.values_f[out_octree.nodes[node_idx].val_off + (n | m)];
            if (sgn > 0 && edges_data[n * 8 + (n | m)].first >= 0)
            {
              need_refill = true;
            }
          }
        }
      }
      if (need_refill)
      {
        float v[8];
        for (int n = 0; n < 8; ++n)
        {
          v[n] = out_octree.values_f[out_octree.nodes[node_idx].val_off + n];
        }
        unsigned val_off = out_octree.values_f.size();
        //TODO delete old data
        unsigned count = create_missing_data_voxel_list(v, edges_data, out_octree.values_f);
        if (count > out_octree.nodes[node_idx].bricks_count)
        {
          out_octree.nodes[node_idx].bricks_count = count;
          out_octree.nodes[node_idx].val_off = val_off;
          out_octree.nodes[node_idx].is_surfaced = true;
          if (out_octree.nodes[node_idx].type == GlobalOctreeNodeType::EMPTY_NODE)
            out_octree.nodes[node_idx].type = GlobalOctreeNodeType::NODE;
          else if (out_octree.nodes[node_idx].type == GlobalOctreeNodeType::EMPTY)
            out_octree.nodes[node_idx].type = GlobalOctreeNodeType::LEAF;
          
          if (out_octree.debug_info)
          {
            #pragma omp critical (debug_write)
            {
              auto &node_info = get_node_info(out_octree.debug_info, node_idx);
              node_info.creation_type = GlobalOctreeDebugInfo::CreationType::REFILL_NODE;
              node_info.connected_components_count = 1;
              node_info.surfaces_count = count;
              node_info.is_surface_node = true;
            }
          }
        }
        else if (count > 0)
        {
          out_octree.values_f.resize(val_off);
        }
      }
    }
  }

  unsigned tri_connection_code(const cmesh4::SimpleMesh &model, unsigned tri_id_A, unsigned tri_id_B,
                               float3 bbox_min_pos, float3 bbox_max_pos)
  {
    unsigned a1 = model.indices[3 * tri_id_A + 0];
    unsigned a2 = model.indices[3 * tri_id_A + 1];
    unsigned a3 = model.indices[3 * tri_id_A + 2];

    unsigned b1 = model.indices[3 * tri_id_B + 0];
    unsigned b2 = model.indices[3 * tri_id_B + 1];
    unsigned b3 = model.indices[3 * tri_id_B + 2];

    // bool common = (a1 == b1 || a1 == b2 || a1 == b3 ||
    //                a2 == b1 || a2 == b2 || a2 == b3 ||
    //                a3 == b1 || a3 == b2 || a3 == b3);

    // return common ? 1 : 0;

    bool common_12 = (a1 == b1 && a2 == b2) || (a1 == b2 && a2 == b1) ||
                     (a1 == b1 && a2 == b3) || (a1 == b3 && a2 == b1) ||
                     (a1 == b2 && a2 == b3) || (a1 == b3 && a2 == b2);
    bool common_13 = (a1 == b1 && a3 == b2) || (a1 == b2 && a3 == b1) ||
                     (a1 == b1 && a3 == b3) || (a1 == b3 && a3 == b1) ||
                     (a1 == b2 && a3 == b3) || (a1 == b3 && a3 == b2);
    bool common_23 = (a2 == b1 && a3 == b2) || (a2 == b2 && a3 == b1) ||
                     (a2 == b1 && a3 == b3) || (a2 == b3 && a3 == b1) ||
                     (a2 == b2 && a3 == b3) || (a2 == b3 && a3 == b2);

    unsigned connection_code = (common_12 + 2 * common_13 + 3 * common_23);
    return connection_code;
  }

  uint32_t count_connected_components(const sdf_converter::PrimitiveListOctree &pl_octree, 
                                      const cmesh4::SimpleMesh &model,
                                      std::vector<std::vector<sdf_converter::NodeBuildData>> &checks, 
                                      int layer, int idx, float thr)
  {
    unsigned prim_ids[GlobalOctree::MAX_SURFACE_COUNT] = {0};
    unsigned prim_offsets[GlobalOctree::MAX_SURFACE_COUNT] = {0};
    unsigned prim_counts[GlobalOctree::MAX_SURFACE_COUNT] = {0};

    float3 pos = 2.0f * (checks[layer][idx].d * checks[layer][idx].pos) - 1.0f;

    float3 min_pos = pos + 2 * checks[layer][idx].d * float3(0,0,0);
    float3 max_pos = pos + 2 * checks[layer][idx].d * float3(1,1,1);

    unsigned all_prim_offset = pl_octree.nodes[checks[layer][idx].prim_list_idx].pid_offset;
    unsigned all_prim_count = pl_octree.nodes[checks[layer][idx].prim_list_idx].pid_count;

    if (all_prim_count > GlobalOctree::MAX_SURFACE_COUNT) return GlobalOctree::MAX_SURFACE_COUNT;

    constexpr uint8_t NO_LINK = 0;
    constexpr uint8_t LINK = 1;
    constexpr uint8_t LINK_UNKNOWN = 2;
    uint8_t adj_matrix[GlobalOctree::MAX_SURFACE_COUNT*GlobalOctree::MAX_SURFACE_COUNT] = {LINK_UNKNOWN};
    
    //fill adjacency matrix
    for (int i=0; i<all_prim_count; i++)
    {
      adj_matrix[i*all_prim_count + i] = LINK;
      for (int j = i+1; j < all_prim_count; j++)
      {
        unsigned code = tri_connection_code(model, pl_octree.primitive_ids[all_prim_offset+i], 
                                            pl_octree.primitive_ids[all_prim_offset+j], min_pos, max_pos);
        adj_matrix[i*all_prim_count + j] = code > 0 ? LINK : NO_LINK;
        adj_matrix[j*all_prim_count + i] = code > 0 ? LINK : NO_LINK;
      }
    }

    //divide all triangles (i.e. vertices of the graph) into connected components
    constexpr int NOT_VISITED = -2;
    constexpr int IN_STACK    = -1;
    int comps[GlobalOctree::MAX_SURFACE_COUNT];
    unsigned stack[GlobalOctree::MAX_SURFACE_COUNT];
    unsigned top;
    int next_start = 0;
    unsigned prims_left = all_prim_count;
    int cur_off = 0;
    int cur_comp = 0;

    for (int i=0; i<all_prim_count; i++)
      comps[i] = NOT_VISITED;

    while (cur_off < all_prim_count)
    {
      assert(next_start >= 0);
      int cur_start = next_start;
      next_start = -1;

      prim_offsets[cur_comp] = cur_off;
      prim_counts[cur_comp] = 0;

      stack[0] = cur_start;
      top = 1;

      while (top > 0)
      {
        int prim = stack[--top];
        comps[prim] = cur_comp;
        prim_counts[cur_comp]++;
        prim_ids[cur_off++] = pl_octree.primitive_ids[all_prim_offset+prim];
        
        for (int i=0; i<all_prim_count; i++)
        {
          if (comps[i] == NOT_VISITED)
          {
            if (adj_matrix[prim*all_prim_count + i] == LINK)
            {
              comps[i] = IN_STACK;
              stack[top++] = i;
            }
            else
              next_start = i;
          }
        }
      }

      cur_comp++;
      if (cur_comp >= thr) return cur_comp;
    }

    return cur_comp;
  }

  void fill_node_multi(sdf_converter::GlobalOctree &out_octree, std::vector<std::vector<sdf_converter::NodeBuildData>> &checks, 
                       int layer, int idx, uint32_t values_layer_offset, unsigned int v_size, const std::vector<uint32_t> &codes,
                       const sdf_converter::PrimitiveListOctree &pl_octree, SparseOctreeSettings &settings, int pad, int size, 
                       std::vector<std::vector<sdf_converter::PointAttributes>> &attributes, 
                       MeshDistFunc &dist_func, const cmesh4::SimpleMesh &model)
  {
    unsigned thread_id = omp_get_thread_num();
    unsigned dist_per_node = v_size * v_size * v_size;
    unsigned node_id = checks[layer][idx].global_idx;
    unsigned code = checks[layer][idx].is_sign_working ? codes[checks[layer][idx].prim_list_idx] : TYPE_UNKNOWN;
    float3 pos = 2.0f * (checks[layer][idx].d * checks[layer][idx].pos) - 1.0f;
    GlobalOctreeNode &cur_node = out_octree.nodes[node_id];

    // assume the node is empty until we fill it with values
    cur_node.val_off = values_layer_offset + dist_per_node * idx;
    cur_node.offset = 0;
    cur_node.bricks_count = 0;
    cur_node.type = GlobalOctreeNodeType::EMPTY;
    cur_node.is_surfaced = false;
    cur_node.is_outside = extract_type(code) != TYPE_VOLUME_INSIDE;

    if (out_octree.debug_info)
    {
      #pragma omp critical (debug_write)
      {
        auto &node_info = get_node_info(out_octree.debug_info, node_id);
        node_info.creation_type = GlobalOctreeDebugInfo::CreationType::EMPTY;
        node_info.position_code = uint4(checks[layer][idx].pos.x, checks[layer][idx].pos.y, checks[layer][idx].pos.z, 1<<layer);
      }
    }

    // there is no primitves in this node, so it is empty and we skip it
    if (pl_octree.nodes[checks[layer][idx].prim_list_idx].pid_intersect_count == 0 ||
        extract_is_empty(code) == true)
      return;

    // this node is 1) guaranteed to have children
    //              2) won't be checked for split threshold
    //              3) it's values will not be used (!fill_all_nodes)
    //  thus there is no need to fill its values
    if (!settings.fill_all_nodes && (!settings.allow_early_stop) && layer < settings.depth)
      return;

    // find connected components between triangles in this node
    unsigned prim_ids[GlobalOctree::MAX_SURFACE_COUNT] = {0};
    unsigned prim_offsets[GlobalOctree::MAX_SURFACE_COUNT] = {0};
    unsigned prim_counts[GlobalOctree::MAX_SURFACE_COUNT] = {0};
    const unsigned *prim_ptr = nullptr;
    unsigned component_count = 0;
    unsigned main_component  = 0;

    unsigned all_prim_offset = pl_octree.nodes[checks[layer][idx].prim_list_idx].pid_offset;
    unsigned all_prim_count = pl_octree.nodes[checks[layer][idx].prim_list_idx].pid_count;

    if (all_prim_count == 1 || all_prim_count > GlobalOctree::MAX_SURFACE_COUNT)
    {
      prim_offsets[0] = all_prim_offset;
      prim_counts[0]  = all_prim_count;
      prim_ptr = pl_octree.primitive_ids.data();
      component_count = 1;
    }
    else if (all_prim_count == 2) //somewhat simple case
    {
      prim_ptr = prim_ids;

      unsigned connection_code = tri_connection_code(model, 
                                                     pl_octree.primitive_ids[all_prim_offset+0], pl_octree.primitive_ids[all_prim_offset+1],
                                                     pos + 2 * checks[layer][idx].d * float3(0,0,0), pos + 2 * checks[layer][idx].d * float3(1,1,1));

      bool linked = connection_code != 0;

      prim_ids[0] = pl_octree.primitive_ids[all_prim_offset+0];
      prim_ids[1] = pl_octree.primitive_ids[all_prim_offset+1];

      prim_offsets[0] = 0;
      prim_offsets[1] = 1;

      prim_counts[0] = linked ? 2 : 1;
      prim_counts[1] = linked ? 0 : 1;

      component_count = linked ? 1 : 2;

      // static unsigned count_0 = 0;
      // static unsigned count_1 = 0;
      // #pragma omp critical
      // {
      //   if (!linked)
      //   {
      //     {
      //       unsigned a1 = model.indices[3 * prim_ids[0] + 0];
      //       unsigned a2 = model.indices[3 * prim_ids[0] + 1];
      //       unsigned a3 = model.indices[3 * prim_ids[0] + 2];

      //       unsigned b1 = model.indices[3 * prim_ids[1] + 0];
      //       unsigned b2 = model.indices[3 * prim_ids[1] + 1];
      //       unsigned b3 = model.indices[3 * prim_ids[1] + 2];
      //       printf("tris %d %d %d -- %d %d %d\n", a1, a2, a3, b1, b2, b3);
      //     }

      //     float4 a1 = model.vPos4f[model.indices[3 * prim_ids[0] + 0]];
      //     float4 a2 = model.vPos4f[model.indices[3 * prim_ids[0] + 1]];
      //     float4 a3 = model.vPos4f[model.indices[3 * prim_ids[0] + 2]];

      //     float4 b1 = model.vPos4f[model.indices[3 * prim_ids[1] + 0]];
      //     float4 b2 = model.vPos4f[model.indices[3 * prim_ids[1] + 1]];
      //     float4 b3 = model.vPos4f[model.indices[3 * prim_ids[1] + 2]];
      //     float3 box_min = pos + 2 * checks[layer][i].d * float3(0,0,0);
      //     float3 box_max = pos + 2 * checks[layer][i].d * float3(1,1,1);
      //     printf("t1 (%f %f %f)(%f %f %f)(%f %f %f)\n", a1.x, a1.y, a1.z, a2.x, a2.y, a2.z, a3.x, a3.y, a3.z);
      //     printf("t2 (%f %f %f)(%f %f %f)(%f %f %f)\n", b1.x, b1.y, b1.z, b2.x, b2.y, b2.z, b3.x, b3.y, b3.z);
      //     printf("box %f %f %f -- %f %f %f\n\n", box_min.x, box_min.y, box_min.z, box_max.x, box_max.y, box_max.z);
      //   }
      //   count_0 += linked;
      //   count_1 += !linked;
      //   if ((count_0 + count_1)%1000 == 0)
      //     printf("counts %d %d\n", count_0, count_1);
      // }
    }
    else 
    {
      prim_ptr = prim_ids;
      component_count = 0;

      float3 min_pos = pos + 2 * checks[layer][idx].d * float3(0,0,0);
      float3 max_pos = pos + 2 * checks[layer][idx].d * float3(1,1,1);

      constexpr uint8_t NO_LINK = 0;
      constexpr uint8_t LINK = 1;
      constexpr uint8_t LINK_UNKNOWN = 2;
      uint8_t adj_matrix[GlobalOctree::MAX_SURFACE_COUNT*GlobalOctree::MAX_SURFACE_COUNT] = {LINK_UNKNOWN};
      
      //fill adjacency matrix
      for (int i=0; i<all_prim_count; i++)
      {
        adj_matrix[i*all_prim_count + i] = LINK;
        for (int j = i+1; j < all_prim_count; j++)
        {
          unsigned connect_code = tri_connection_code(model, pl_octree.primitive_ids[all_prim_offset+i], 
                                                      pl_octree.primitive_ids[all_prim_offset+j], min_pos, max_pos);
          adj_matrix[i*all_prim_count + j] = connect_code > 0 ? LINK : NO_LINK;
          adj_matrix[j*all_prim_count + i] = connect_code > 0 ? LINK : NO_LINK;
        }
      }

      //divide all triangles (i.e. vertices of the graph) into connected components
      constexpr int NOT_VISITED = -2;
      constexpr int IN_STACK    = -1;
      int comps[GlobalOctree::MAX_SURFACE_COUNT];
      unsigned stack[GlobalOctree::MAX_SURFACE_COUNT];
      unsigned top;
      int next_start = 0;
      unsigned prims_left = all_prim_count;
      int cur_off = 0;
      int cur_comp = 0;

      for (int i=0; i<all_prim_count; i++)
        comps[i] = NOT_VISITED;

      while (cur_off < all_prim_count)
      {
        assert(next_start >= 0);
        int cur_start = next_start;
        next_start = -1;

        prim_offsets[cur_comp] = cur_off;
        prim_counts[cur_comp] = 0;

        stack[0] = cur_start;
        top = 1;

        while (top > 0)
        {
          int prim = stack[--top];
          comps[prim] = cur_comp;
          prim_counts[cur_comp]++;
          prim_ids[cur_off++] = pl_octree.primitive_ids[all_prim_offset+prim];
          
          for (int i=0; i<all_prim_count; i++)
          {
            if (comps[i] == NOT_VISITED)
            {
              if (adj_matrix[prim*all_prim_count + i] == LINK)
              {
                comps[i] = IN_STACK;
                stack[top++] = i;
              }
              else
                next_start = i;
            }
          }
        }

        cur_comp++;
      }

      component_count = cur_comp;

      // if (all_prim_count == 4)
      // {
      //   #pragma omp critical
      //   {
      //     printf("%d components\n", component_count);
      //     for (int j=0;j<all_prim_count;j++)
      //       printf("%d ", comps[j]);
      //     printf("\n");
      //     for (int i=0;i<all_prim_count;i++)
      //     {
      //       for (int j=0;j<all_prim_count;j++)
      //         printf("%d ", adj_matrix[i*all_prim_count + j]);
      //       printf("\n");
      //     }
      //     printf("\n");
      //   }
      // }
    }

    //fill all nodes
    float values_f_tmp[MAX_DISTANCES_PER_NODE];

    bool too_many_values = component_count*dist_per_node > MAX_DISTANCES_PER_NODE;
    assert(!too_many_values);

    bool first_full_component = true;
    for (int comp = 0; comp < component_count; comp++)
    {
      float min_val = 1000.0f;
      float max_val = -1000.0f;
      float coeffs[8] = {};
      if (settings.use_point_cloud && size == 1 && pad == 0)
      {
        point_cloud::SDF_by_point_cloud(model, settings, prim_ptr + prim_offsets[comp], prim_counts[comp], 
                                        pos, pos + 2 * (checks[layer][idx].d / out_octree.header.brick_size),
                                        dist_func, coeffs);
      }
      for (int x = -pad; x <= size + pad; ++x)
      {
        for (int y = -pad; y <= size + pad; ++y)
        {
          for (int z = -pad; z <= size + pad; ++z)
          {
            float3 ch_pos = pos + 2 * (checks[layer][idx].d / out_octree.header.brick_size) * float3(x, y, z);
            unsigned p_idx = v_size * v_size * (x + pad) + v_size * (y + pad) + z + pad;
            attributes[thread_id][p_idx] = dist_func.calculate(model, prim_ptr + prim_offsets[comp], prim_counts[comp], ch_pos);
            float dist = attributes[thread_id][p_idx].distance;
            if (settings.use_point_cloud && size == 1 && pad == 0)
            {
              dist = coeffs[x * 4 + y * 2 + z];
            }
            values_f_tmp[comp*dist_per_node + p_idx] = dist;
            min_val = std::min(min_val, dist);
            max_val = std::max(max_val, dist);
          }
        }
      }


      bool leave_empty = false;
      if (layer > settings.depth)
      {
        float3 min_pos = pos;
        float3 max_pos = pos + 2 * checks[layer][idx].d * float3(1,1,1);
        leave_empty = true;
        for (int j = 0; j < prim_counts[comp]; ++j)
        {
          if (TriangleInBBoxFunc::in_bbox(model, prim_ptr[prim_offsets[comp] + j], min_pos, max_pos))
          {
            leave_empty = false;
            break;
          }
        }
      }

      if (leave_empty)
        std::fill_n(values_f_tmp + comp*dist_per_node, dist_per_node, 1000.0f);

      // load texture coordinates and material id for first component with actual surface
      // if for some reason no such component is found, use the last one
      if (first_full_component && (min_val*max_val <= 0 || comp == component_count-1))
      {
        first_full_component = false;

        // save texture coordinates from 8 corners
        if (out_octree.header.tc_channel_id >= 0)
        {
          auto &tex = out_octree.point_channels[out_octree.header.tc_channel_id];
          for (int j = 0; j < 8; ++j)
          {
            unsigned p_idx = out_octree.header.brick_size * (v_size * v_size * (j >> 2) + v_size * ((j >> 1) & 1) + (j & 1));
            //cur_node.tex_coords[j] = attributes[thread_id][idx].tex_coord;
            tex.data_f[tex.num_components * node_id * 8 + j * 2 + 0] = attributes[thread_id][p_idx].tex_coord.x;
            tex.data_f[tex.num_components * node_id * 8 + j * 2 + 1] = attributes[thread_id][p_idx].tex_coord.y;
          }
        }

        // save material id from center point
        if (out_octree.header.mat_channel_id >= 0)
        {
          //sample center of the voxel
          float3 ch_pos = pos + 2 * (checks[layer][idx].d / out_octree.header.brick_size) * float3(0.5f * size);
          unsigned midx = v_size * v_size * v_size;
          attributes[thread_id][midx] = dist_func.calculate(model, prim_ptr + prim_offsets[comp], prim_counts[comp], ch_pos);
          float dist = attributes[thread_id][midx].distance;
          values_f_tmp[comp * dist_per_node + midx] = dist;

          auto &mat = out_octree.voxel_channels[out_octree.header.mat_channel_id];
          mat.data_i[mat.num_components * node_id] = attributes[thread_id][midx].mat;
        }

        //save data channels if we have augmented mesh
        // if constexpr (std::is_same_v<Model, cmesh4::AugmentedMesh>)
        // {
        //   fill_data_channels_node(model, out_octree, v_size, size, attributes[thread_id], node_id);
        // }
        // else if constexpr (std::is_same_v<Model, vtk::UnstructuredGrid>)
        // {
        //   fill_data_channels_node(model, out_octree, v_size, size, attributes[thread_id], node_id);
        // }
      }
    }

    bool valid_code = code != CODE_UNDECIDED && extract_type(code) != TYPE_UNKNOWN;
    bool decided_code = valid_code && extract_is_decided(code);

    unsigned final_surfaces_count = 0;
    if (component_count > 1 && !decided_code && v_size == 2)//it's temporary decision
      final_surfaces_count = remove_surfaces(values_f_tmp, component_count);
    else
      final_surfaces_count = component_count;

    if (!valid_code || v_size != 2)//it's temporary decision
    {
      // invalid code - do not apply sign analysis
      cur_node.is_surfaced = (final_surfaces_count > 1);
    }
    else if (!decided_code)
    {
      // mark every undecided node as surface, because it is much worse to have
      // volume node instead of surface than the other way
      cur_node.is_surfaced = true;
    }
    else
    {
      if (extract_is_surface(code))
        cur_node.is_surfaced = extract_is_surface(code);

      if (!extract_is_decided(code))
        cur_node.is_surfaced = false;

      // if the code is decided, we transform the node according to code
      unsigned count = std::min(final_surfaces_count, extract_count(code));

      unsigned ffidx = checks[layer][idx].prim_list_idx;
      // final_surfaces_count = count;
      float val_fixed_sign[8 * GlobalOctree::MAX_SURFACE_COUNT];
      unsigned signs[GlobalOctree::MAX_SURFACE_COUNT] = {0};
      static constexpr unsigned bitcount_table[256] = {
          0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
          1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
          1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
          2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
          1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
          2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
          2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
          3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
          1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
          2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
          2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
          3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
          2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
          3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
          3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
          4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};
      for (int j = 0; j < final_surfaces_count; j++)
      {
        signs[j] = 0;
        for (int k = 0; k < 8; k++)
          signs[j] |= (values_f_tmp[8 * j + k] < 0 ? 1 : 0) << k;
      }

      if (out_octree.debug_info)
      {
        #pragma omp critical(debug_write)
        {
          auto &node_info = get_node_info(out_octree.debug_info, node_id);
          node_info.initial_signs = (signs[3] << 24) | (signs[2] << 16) | (signs[1] << 8) | signs[0];
        }
      }

      for (int i = 0; i < count; i++)
      {
        unsigned target_sign = i == 0 ? extract_signs_1(code) : extract_signs_2(code);
        unsigned best_sign_diff = 1000;
        unsigned best_surf_id = 0;
        for (int j = 0; j < final_surfaces_count; j++)
        {
          unsigned sign_diff = bitcount_table[signs[j] ^ target_sign];
          sign_diff = std::min(sign_diff, 8 - sign_diff);

          if (signs[j] > 0 && sign_diff < best_sign_diff)
          {
            best_sign_diff = sign_diff;
            best_surf_id = j;
          }
        }

        for (int j = 0; j < 8; j++)
          val_fixed_sign[8 * i + j] = ((target_sign & (1 << j)) ? -1 : 1) * std::abs(values_f_tmp[8 * best_surf_id + j]);
        signs[best_surf_id] = 0;
      }

      for (int i = 0; i < 8 * count; i++)
        values_f_tmp[i] = val_fixed_sign[i];

      final_surfaces_count = count;
    }

    //if we are goint to have multiple surfaces, allocate memory for additional distances
    //we can be in parallel section, so mark this as critical
    float min_val = 1000;
    float max_val = -1000;
    #pragma omp critical
    {
      if (final_surfaces_count > 1)
      {
        cur_node.val_off = out_octree.values_f.size();
        out_octree.values_f.resize(out_octree.values_f.size() + final_surfaces_count * dist_per_node);
      }

      if (final_surfaces_count > 0)
      {
        for (int i = 0; i < final_surfaces_count * dist_per_node; i++)
        {
          out_octree.values_f[cur_node.val_off + i] = values_f_tmp[i];
          min_val = std::min(min_val, values_f_tmp[i]);
          max_val = std::max(max_val, values_f_tmp[i]);
        }
      }
    }

    bool active_as_leaf = min_val <= 0 && (max_val >= 0 || settings.fill_internal_volume);
    cur_node.type = active_as_leaf ? GlobalOctreeNodeType::LEAF : GlobalOctreeNodeType::EMPTY;
    cur_node.bricks_count = active_as_leaf ? final_surfaces_count : 0;
    cur_node.is_internal  = active_as_leaf && (max_val < 0);

    if (out_octree.debug_info)
    {
      #pragma omp critical (debug_write)
      {
        auto &node_info = get_node_info(out_octree.debug_info, node_id);
        node_info.creation_type = GlobalOctreeDebugInfo::CreationType::FILL_NODE_MULTI;
        node_info.connected_components_count = component_count;
        node_info.primitives_count = pl_octree.nodes[checks[layer][idx].prim_list_idx].pid_count;
        node_info.surfaces_count = cur_node.bricks_count;
        node_info.is_surface_node = cur_node.is_surfaced;
        node_info.prim_octree_node_code = code;
      }
    }
  }

  // simple function to calculate distance between parent and child nodes
  // works only if both nodes are single volume nodes, otherwise set INCOMPARABLE_NODES_DISTANCE
  float parent_child_distance_basic(const GlobalOctree &octree, const GlobalOctreeNode &parent, 
                                    const GlobalOctreeNode &child, uint32_t child_n /*0 to 7 - number of child*/)
  {
    //if (octree.header.brick_pad != 0 || octree.header.brick_size != 1)
    //  return INCOMPARABLE_NODES_DISTANCE;

    if (parent.bricks_count != 1 || child.bricks_count != 1)
      return INCOMPARABLE_NODES_DISTANCE;
    
    if (parent.is_surfaced != child.is_surfaced)
      return INCOMPARABLE_NODES_DISTANCE;

    int v_size = octree.header.brick_size + octree.header.brick_pad * 2 + 1;
    
    //part of parent surface, calculated by interpolation
    float *parent_part = new float[v_size * v_size * v_size];

    float3 ch_offset = float3(child_n >> 2, (child_n >> 1) & 1, child_n & 1);

    /*for (int i = 0; i < 8; i++)
    {
      float3 q = float3(i >> 2, (i >> 1) & 1, i & 1);
      parent_part[i] = trilinear_interp(octree.values_f.data() + parent.val_off, 0.5f*(q + ch_offset));
    }*/
    float min_val = 1000.0, max_val = -1000.0;
    take_brick_part_by_pos_code(octree.header, uint4(child_n >> 2, (child_n >> 1) & 1, child_n & 1, 2), octree.values_f.data() + parent.val_off, 
                                parent_part, min_val, max_val);

    float parent_vox[8], child_vox[8], result = 0;

    for (int i = -octree.header.brick_pad; i < octree.header.brick_size + octree.header.brick_pad; ++i)
    {
      for (int j = -octree.header.brick_pad; j < octree.header.brick_size + octree.header.brick_pad; ++j)
      {
        for (int k = -octree.header.brick_pad; k < octree.header.brick_size + octree.header.brick_pad; ++k)
        {
          for (int idx = 0; idx < 8; ++idx)
          {
            parent_vox[idx] = parent_part[(i + (idx >> 2)) * v_size * v_size + (j + ((idx >> 1) & 1)) * v_size + k + (idx & 1)];
            child_vox[idx] = *(octree.values_f.data() + child.val_off + (i + (idx >> 2)) * v_size * v_size + (j + ((idx >> 1) & 1)) * v_size + k + (idx & 1));
          }
          result += surface_distance_RMSE<4>(parent_vox, child_vox);
        }
      }
    }
    delete [] parent_part;
    return result / ((v_size - 1) * (v_size - 1) * (v_size - 1));
  }

  float parent_child_distance_multi(const GlobalOctree &octree, const GlobalOctreeNode &parent, 
                                    const GlobalOctreeNode &child, uint32_t child_n /*0 to 7 - number of child*/)
  {
    //if (octree.header.brick_pad != 0 || octree.header.brick_size != 1)
    //  return INCOMPARABLE_NODES_DISTANCE;
    
    if (parent.is_surfaced != child.is_surfaced)
      return INCOMPARABLE_NODES_DISTANCE;
    
    if (child.bricks_count * parent.bricks_count == 0)
      return INCOMPARABLE_NODES_DISTANCE;
    
    int v_size = octree.header.brick_size + octree.header.brick_pad * 2 + 1;

    std::vector<float> arr;
    arr.resize(child.bricks_count * parent.bricks_count);

    std::vector<int> comp_ind_ch;
    comp_ind_ch.resize(child.bricks_count);

    float *parent_part = new float[v_size * v_size * v_size];

    for (int i = 0; i < comp_ind_ch.size(); ++i)
    {
      comp_ind_ch[i] = -1;
    }

    std::vector<int> comp_ind_p;
    comp_ind_p.resize(parent.bricks_count);

    for (int i = 0; i < comp_ind_p.size(); ++i)
    {
      comp_ind_p[i] = -1;
    }

    std::set<int> buf;

    //part of parent surface, calculated by interpolation
    float parent_vox[8], child_vox[8];

    float3 ch_offset = float3(child_n >> 2, (child_n >> 1) & 1, child_n & 1);

    for (int num = 0; num < parent.bricks_count; ++num)
    {
      /*for (int i = 0; i < 8; i++)//
      {
        float3 q = float3(i >> 2, (i >> 1) & 1, i & 1);
        parent_part_1[i] = trilinear_interp(octree.values_f.data() + parent.val_off + 8 * num, 0.5f*(q + ch_offset));
        parent_part_2[i] = -parent_part_1[i];
      }*/
      float min_val = 1000.0, max_val = -1000.0;
      take_brick_part_by_pos_code(octree.header, uint4(child_n >> 2, (child_n >> 1) & 1, child_n & 1, 2), 
                                  octree.values_f.data() + parent.val_off + v_size * v_size * v_size * num, 
                                  parent_part, min_val, max_val);
      
      for (int brick_n = 0; brick_n < child.bricks_count; ++brick_n)
      {
        float res = 0;
        for (int i = -octree.header.brick_pad; i < octree.header.brick_size + octree.header.brick_pad; ++i)
        {
          for (int j = -octree.header.brick_pad; j < octree.header.brick_size + octree.header.brick_pad; ++j)
          {
            for (int k = -octree.header.brick_pad; k < octree.header.brick_size + octree.header.brick_pad; ++k)
            {
              for (int idx = 0; idx < 8; ++idx)
              {
                parent_vox[idx] = parent_part[(i + (idx >> 2)) * v_size * v_size + (j + ((idx >> 1) & 1)) * v_size + k + (idx & 1)];
                child_vox[idx] = *(octree.values_f.data() + child.val_off + i * v_size * v_size * v_size + (i + (idx >> 2)) * v_size * v_size + (j + ((idx >> 1) & 1)) * v_size + k + (idx & 1));
              }
              res += surface_distance_RMSE<4>(parent_vox, child_vox);
            }
          }
        }
        res /= (v_size - 1) * (v_size - 1) * (v_size - 1);
        //surface_distance_RMSE<4>(parent_part_1, octree.values_f.data() + child.val_off + i * 8);//
        if (parent.is_surfaced)
        {
          float res_2 = 0;
          for (int i = -octree.header.brick_pad; i < octree.header.brick_size + octree.header.brick_pad; ++i)
          {
            for (int j = -octree.header.brick_pad; j < octree.header.brick_size + octree.header.brick_pad; ++j)
            {
              for (int k = -octree.header.brick_pad; k < octree.header.brick_size + octree.header.brick_pad; ++k)
              {
                for (int idx = 0; idx < 8; ++idx)
                {
                  parent_vox[idx] = -parent_part[(i + (idx >> 2)) * v_size * v_size + (j + ((idx >> 1) & 1)) * v_size + k + (idx & 1)];
                  child_vox[idx] = *(octree.values_f.data() + child.val_off + i * v_size * v_size * v_size + (i + (idx >> 2)) * v_size * v_size + (j + ((idx >> 1) & 1)) * v_size + k + (idx & 1));
                }
                res_2 += surface_distance_RMSE<4>(parent_vox, child_vox);
              }
            }
          }
          res_2 /= (v_size - 1) * (v_size - 1) * (v_size - 1);
          //surface_distance_RMSE<4>(parent_part_2, octree.values_f.data() + child.val_off + i * 8);//
          res = res_2 > res ? res : res_2;
        }
        arr[num * child.bricks_count + brick_n] = res;
        if (comp_ind_ch[brick_n] < 0 || arr[comp_ind_ch[brick_n] * child.bricks_count + brick_n] > res)
        {
          comp_ind_ch[brick_n] = num;
        }
        if (comp_ind_p[num] < 0 || arr[num * child.bricks_count + comp_ind_p[num]] > res)
        {
          comp_ind_p[num] = brick_n;
        }
      }
    }

    delete [] parent_part;

    float result_1 = 0, result_2 = 0;
    int count_1 = 0, count_2 = 0;

    for (int i = 0; i < comp_ind_ch.size(); ++i)
    {
      result_1 += arr[comp_ind_ch[i] * child.bricks_count + i];
      ++count_1;
      buf.insert(comp_ind_ch[i]);
    }
    for (int num = 0; num < parent.bricks_count; ++num)
    {
      if (buf.find(num) == buf.end())
      {
        result_1 += arr[num * child.bricks_count + comp_ind_p[num]];
        ++count_1;
      }
    }
    result_1 /= count_1;
    buf.clear();

    for (int num = 0; num < comp_ind_p.size(); ++num)
    {
      result_2 += arr[num * child.bricks_count + comp_ind_p[num]];
      ++count_2;
      buf.insert(comp_ind_p[num]);
    }
    for (int i = 0; i < child.bricks_count; ++i)
    {
      if (buf.find(i) == buf.end())
      {
        result_2 += arr[comp_ind_ch[i] * child.bricks_count + i];
        ++count_2;
      }
    }
    result_2 /= count_2;

    return result_1 > result_2 ? result_1 : result_2;
  }

  float parent_child_distance_iou_multi(const GlobalOctree &octree, const GlobalOctreeNode &parent, 
                                        const GlobalOctreeNode &child, uint32_t child_n /*0 to 7 - number of child*/)
  {
    //if (octree.header.brick_pad != 0 || octree.header.brick_size != 1)
    //  return INCOMPARABLE_NODES_DISTANCE;
    
    if (parent.is_surfaced != child.is_surfaced)
      return INCOMPARABLE_NODES_DISTANCE;

    float3 ch_offset = float3(child_n >> 2, (child_n >> 1) & 1, child_n & 1);

    int v_size = octree.header.brick_size + octree.header.brick_pad * 2 + 1;

    if (parent.is_surfaced)
    {
      float *parent_part = new float[v_size * v_size * v_size];

      std::vector<float> arr;
      arr.resize(child.bricks_count * parent.bricks_count);

      std::vector<int> comp_ind_ch;
      comp_ind_ch.resize(child.bricks_count);

      for (int i = 0; i < comp_ind_ch.size(); ++i)
      {
        comp_ind_ch[i] = -1;
      }

      std::vector<int> comp_ind_p;
      comp_ind_p.resize(parent.bricks_count);

      for (int i = 0; i < comp_ind_p.size(); ++i)
      {
        comp_ind_p[i] = -1;
      }

      std::set<int> buf;

      //part of parent surface, calculated by interpolation
      float parent_vox_1[8], parent_vox_2[8], child_vox[8];

      for (int num = 0; num < parent.bricks_count; ++num)
      {
        /*for (int i = 0; i < 8; i++)
        {
          float3 q = float3(i >> 2, (i >> 1) & 1, i & 1);
          parent_part_1[i] = trilinear_interp(octree.values_f.data() + parent.val_off + 8 * num, 0.5f*(q + ch_offset));
          parent_part_2[i] = -parent_part_1[i];
        }*/
        float min_val = 1000.0, max_val = -1000.0;
        take_brick_part_by_pos_code(octree.header, uint4(child_n >> 2, (child_n >> 1) & 1, child_n & 1, 2), 
                                    octree.values_f.data() + parent.val_off + v_size * v_size * v_size * num, 
                                    parent_part, min_val, max_val);
        for (int brick_n = 0; brick_n < child.bricks_count; ++brick_n)
        {
          //float res = voxel_distance_IoU<50>(parent_part_1, octree.values_f.data() + child.val_off + i * 8);
          //float res_2 = voxel_distance_IoU<50>(parent_part_2, octree.values_f.data() + child.val_off + i * 8);
          float res = 0, res_2 = 0;
          for (int i = -octree.header.brick_pad; i < octree.header.brick_size + octree.header.brick_pad; ++i)
          {
            for (int j = -octree.header.brick_pad; j < octree.header.brick_size + octree.header.brick_pad; ++j)
            {
              for (int k = -octree.header.brick_pad; k < octree.header.brick_size + octree.header.brick_pad; ++k)
              {
                for (int idx = 0; idx < 8; ++idx)
                {
                  parent_vox_1[idx] = parent_part[(i + (idx >> 2)) * v_size * v_size + (j + ((idx >> 1) & 1)) * v_size + k + (idx & 1)];
                  parent_vox_2[idx] = parent_vox_1[idx];
                  child_vox[idx] = *(octree.values_f.data() + child.val_off + i * v_size * v_size * v_size + (i + (idx >> 2)) * v_size * v_size + (j + ((idx >> 1) & 1)) * v_size + k + (idx & 1));
                }
                res += voxel_distance_IoU<50>(parent_vox_1, child_vox);
                res_2 += voxel_distance_IoU<50>(parent_vox_2, child_vox);
              }
            }
          }
          res /= (v_size - 1) * (v_size - 1) * (v_size - 1);
          res_2 /= (v_size - 1) * (v_size - 1) * (v_size - 1);
          res = res_2 > res ? res : res_2;
          arr[num * child.bricks_count + brick_n] = res;
          if (comp_ind_ch[brick_n] < 0 || arr[comp_ind_ch[brick_n] * child.bricks_count + brick_n] > res)
          {
            comp_ind_ch[brick_n] = num;
          }
          if (comp_ind_p[num] < 0 || arr[num * child.bricks_count + comp_ind_p[num]] > res)
          {
            comp_ind_p[num] = brick_n;
          }
        }
      }

      delete [] parent_part;

      float result_1 = 0, result_2 = 0;
      int count_1 = 0, count_2 = 0;

      for (int i = 0; i < comp_ind_ch.size(); ++i)
      {
        result_1 += arr[comp_ind_ch[i] * child.bricks_count + i];
        ++count_1;
        buf.insert(comp_ind_ch[i]);
      }
      for (int num = 0; num < parent.bricks_count; ++num)
      {
        if (buf.find(num) == buf.end())
        {
          result_1 += arr[num * child.bricks_count + comp_ind_p[num]];
          ++count_1;
        }
      }
      result_1 /= count_1;
      buf.clear();

      for (int num = 0; num < comp_ind_ch.size(); ++num)
      {
        result_2 += arr[num * child.bricks_count + comp_ind_p[num]];
        ++count_2;
        buf.insert(comp_ind_p[num]);
      }
      for (int i = 0; i < parent.bricks_count; ++i)
      {
        if (buf.find(i) == buf.end())
        {
          result_2 += arr[comp_ind_ch[i] * child.bricks_count + i];
          ++count_2;
        }
      }
      result_2 /= count_2;

      return result_1 > result_2 ? result_1 : result_2;
    }
    else if (v_size == 2)
    {
      std::vector<float> parent_p;
      parent_p.resize(8 * parent.bricks_count);
      for (int num = 0; num < parent.bricks_count; ++num)
      {
        for (int i = 0; i < 8; i++)
        {
          float3 q = float3(i >> 2, (i >> 1) & 1, i & 1);
          parent_p[i] = trilinear_interp(octree.values_f.data() + parent.val_off + 8 * num, 0.5f*(q + ch_offset));
        }
      }
      return voxel_distance_IoU<50>(parent_p.data(), octree.values_f.data() + child.val_off, parent.bricks_count, child.bricks_count);
    }
    //else
    float *parent_part = new float[v_size * v_size * v_size];
    std::vector<std::vector<float>> parent_voxs, child_voxs;
    parent_voxs.resize((v_size - 1) * (v_size - 1) * (v_size - 1));
    child_voxs.resize((v_size - 1) * (v_size - 1) * (v_size - 1));
    float result = 0;
    for (int num = 0; num < parent.bricks_count; ++num)
    {
      float min_val = 1000.0, max_val = -1000.0;
      take_brick_part_by_pos_code(octree.header, uint4(child_n >> 2, (child_n >> 1) & 1, child_n & 1, 2), 
                                  octree.values_f.data() + parent.val_off + v_size * v_size * v_size * num, 
                                  parent_part, min_val, max_val);
      for (int i = 0; i < v_size - 1; ++i)
      {
        for (int j = 0; j < v_size - 1; ++j)
        {
          for (int k = 0; k < v_size - 1; ++k)
          {
            for (int idx = 0; idx < 8; ++idx)
            {
              parent_voxs[i * (v_size - 1) * (v_size - 1) + j * (v_size - 1) + k].push_back(parent_part[(i + (idx >> 2)) * v_size * v_size + (j + ((idx >> 1) & 1)) * v_size + k + (idx & 1)]);
            }
          }
        }
      }
    }
    for (int num = 0; num < child.bricks_count; ++num)
    {
      for (int i = 0; i < v_size - 1; ++i)
      {
        for (int j = 0; j < v_size - 1; ++j)
        {
          for (int k = 0; k < v_size - 1; ++k)
          {
            for (int idx = 0; idx < 8; ++idx)
            {
              child_voxs[i * (v_size - 1) * (v_size - 1) + j * (v_size - 1) + k].push_back(*(octree.values_f.data() + v_size * v_size * v_size * num + child.val_off + (i + (idx >> 2)) * v_size * v_size + (j + ((idx >> 1) & 1)) * v_size + k + (idx & 1)));
            }
          }
        }
      }
    }
    delete [] parent_part;
    for (int i = 0; i < v_size - 1; ++i)
    {
      for (int j = 0; j < v_size - 1; ++j)
      {
        for (int k = 0; k < v_size - 1; ++k)
        {
          result += voxel_distance_IoU<50>(parent_voxs[i * (v_size - 1) * (v_size - 1) + j * (v_size - 1) + k].data(), 
                                           child_voxs[i * (v_size - 1) * (v_size - 1) + j * (v_size - 1) + k].data(), 
                                           parent.bricks_count, child.bricks_count);
        }
      }
    }
    return result / ((v_size - 1) * (v_size - 1) * (v_size - 1));
  }
  float3 sample_surface_in_voxel(const float vox[8])
  {
    bool is_filled = false;
    float3 res = float3(-1);
    for (int i = 1; i < 8; ++i)
    {
      if (vox[0] * vox[i] <= 0)
      {
        is_filled = true;
        break;
      }
    }

    unsigned axis = rand() % 3;
    float3 ax;
    for (unsigned cnt = 0; cnt < 100; ++cnt)
    {
      for (unsigned i = 0; i < 3; ++i)
      {
        if (axis == i)
          ax[i] = 0;
        else
          ax[i] = LiteMath::rnd(0, 1);
      }
      float p = trilinear_interp(vox, ax);
      ax[axis] = 1;
      float q = trilinear_interp(vox, ax);
      if (p * q <= 0)
      {
        res = ax;
        res[axis] = p / (p - q);
        break;
      }
      axis = rand() % 3;
    }
    return res;
  }

  void take_part_of_voxel(const float vox[8], float *res, uint32_t child_n)
  {
    float3 minimum = float3(0.0f);
    float3 maximum = float3(1.0f);
    for (int i = 1, idx = 2; i < 8; i <<= 1, --idx)
    {
      if ((child_n & i) == 0)
        maximum[idx] = 0.5;
      else
        minimum[idx] = 0.5;
    }
    for (int i = 0; i < 8; ++i)
    {
      float3 point = minimum;
      if (i & 1)
        point[2] = maximum[2];
      if (i & 2)
        point[1] = maximum[1];
      if (i & 4)
        point[0] = maximum[0];
      res[i] = trilinear_interp(vox, point);
    }
  }

  float find_coeff_for_point(float3 point, const float vox[8], float eps = 1e-5)
  {
    float mean = 0, real = trilinear_interp(vox, point);
    for (unsigned i = 0; i < 3; ++i)
    {
      for (unsigned j = 0; j < 2; ++j)
      {
        float3 p = point;
        float off = eps;
        if (j)
          off = -eps;
        p[i] += off;
        float next = trilinear_interp(vox, p);
        mean += std::abs(real - next);
      }
    }
    return mean / 6.0f;
  }

  float compare_first_voxels_with_seconds(const float *vox_1, unsigned count_1, 
                                          const float *vox_2, unsigned count_2, 
                                          unsigned &comparisons, unsigned num_of_comp = 64)
  {
    comparisons = 0;
    float res = 0;
    for (unsigned i = 0; i < count_1; ++i)
    {
      for (unsigned j = 0; j < num_of_comp; ++j)
      {
        float3 point = sample_surface_in_voxel(vox_1 + i * 8);
        if (point.x < 0)
          break;
        float coeff_1 = find_coeff_for_point(point, vox_1 + i * 8);
        if (coeff_1 > 0)
        {
          float val_1 = trilinear_interp(vox_1 + i * 8, point);
          float tmp = -1;
          for (unsigned k = 0; k < count_2; ++k)
          {
            float coeff_2 = find_coeff_for_point(point, vox_2 + k * 8);
            if (coeff_2 > 0)
            {
              float val_2 = trilinear_interp(vox_2 + k * 8, point);
              float val_1_t = val_1;
              float val_2_t = val_2 * coeff_1 / coeff_2;
              if (tmp < 0)
              {
                ++comparisons;
                tmp = (val_1_t - val_2_t) * (val_1_t - val_2_t);
              }
              else if (tmp > (val_1_t - val_2_t) * (val_1_t - val_2_t))
              {
                tmp = (val_1_t - val_2_t) * (val_1_t - val_2_t);
              }
            }
          }
          if (tmp >= 0)
          {
            res += tmp;
          }
        }
      }
    }
    return res;
  }

  float parent_child_distance_samples(const GlobalOctree &octree, const GlobalOctreeNode &parent, 
                                      const GlobalOctreeNode &child, uint32_t child_n /*0 to 7 - number of child*/)
  {
    //if (octree.header.brick_pad != 0 || octree.header.brick_size != 1)
    //  return INCOMPARABLE_NODES_DISTANCE;
      
    if (parent.bricks_count == 0 && child.bricks_count == 0)
      return 0.0;

    //if (parent.bricks_count == 0 || child.bricks_count == 0)
    //  return INCOMPARABLE_NODES_DISTANCE;
    
    if (parent.is_surfaced != child.is_surfaced)
      return INCOMPARABLE_NODES_DISTANCE;

    int v_size = octree.header.brick_size + octree.header.brick_pad * 2 + 1;
    
    std::vector<float> part_voxels;
    part_voxels.resize(parent.bricks_count * 8);
    unsigned vox_cnt = 0;
    /*for (unsigned i = 0; i < parent.bricks_count; ++i)
    {
      take_part_of_voxel(octree.values_f.data() + parent.val_off + 8 * i, part_voxels.data() + 8 * i, child_n);
    }*/
    unsigned comparisons_1 = 0, comparisons_2 = 0;
    float res = 0;
    /*res += compare_first_voxels_with_seconds(part_voxels.data(), parent.bricks_count, 
                                             octree.values_f.data() + child.val_off, child.bricks_count, comparisons_1);
    res += compare_first_voxels_with_seconds(octree.values_f.data() + child.val_off, child.bricks_count, 
                                             part_voxels.data(), parent.bricks_count, comparisons_2);*/
    float *parent_part = new float[v_size * v_size * v_size];
    std::vector<std::vector<float>> parent_voxs, child_voxs;
    parent_voxs.resize((v_size - 1) * (v_size - 1) * (v_size - 1));
    child_voxs.resize((v_size - 1) * (v_size - 1) * (v_size - 1));
    for (int num = 0; num < parent.bricks_count; ++num)
    {
      float min_val = 1000.0, max_val = -1000.0;
      take_brick_part_by_pos_code(octree.header, uint4(child_n >> 2, (child_n >> 1) & 1, child_n & 1, 2), 
                                  octree.values_f.data() + parent.val_off + v_size * v_size * v_size * num, 
                                  parent_part, min_val, max_val);
      for (int i = 0; i < v_size - 1; ++i)
      {
        for (int j = 0; j < v_size - 1; ++j)
        {
          for (int k = 0; k < v_size - 1; ++k)
          {
            for (int idx = 0; idx < 8; ++idx)
            {
              parent_voxs[i * (v_size - 1) * (v_size - 1) + j * (v_size - 1) + k].push_back(parent_part[(i + (idx >> 2)) * v_size * v_size + (j + ((idx >> 1) & 1)) * v_size + k + (idx & 1)]);
            }
          }
        }
      }
    }
    delete [] parent_part;
    for (int num = 0; num < child.bricks_count; ++num)
    {
      for (int i = 0; i < v_size - 1; ++i)
      {
        for (int j = 0; j < v_size - 1; ++j)
        {
          for (int k = 0; k < v_size - 1; ++k)
          {
            for (int idx = 0; idx < 8; ++idx)
            {
              child_voxs[i * (v_size - 1) * (v_size - 1) + j * (v_size - 1) + k].push_back(*(octree.values_f.data() + v_size * v_size * v_size * num + child.val_off + (i + (idx >> 2)) * v_size * v_size + (j + ((idx >> 1) & 1)) * v_size + k + (idx & 1)));
            }
          }
        }
      }
    }
    for (int i = 0; i < v_size - 1; ++i)
    {
      for (int j = 0; j < v_size - 1; ++j)
      {
        for (int k = 0; k < v_size - 1; ++k)
        {
          res += compare_first_voxels_with_seconds(parent_voxs[i * (v_size - 1) * (v_size - 1) + j * (v_size - 1) + k].data(), parent.bricks_count, 
                                                   child_voxs[i * (v_size - 1) * (v_size - 1) + j * (v_size - 1) + k].data(), child.bricks_count, comparisons_1);
          res += compare_first_voxels_with_seconds(child_voxs[i * (v_size - 1) * (v_size - 1) + j * (v_size - 1) + k].data(), child.bricks_count, 
                                                   parent_voxs[i * (v_size - 1) * (v_size - 1) + j * (v_size - 1) + k].data(), parent.bricks_count, comparisons_2);
        }
      }
    }
    res /= (v_size - 1) * (v_size - 1) * (v_size - 1);
    if (comparisons_1 + comparisons_2 > 0)
      res /= (comparisons_1 + comparisons_2);
    else
      res = INCOMPARABLE_NODES_DISTANCE;
    return res;
  }

  float parent_child_distance(SparseOctreeSettings::VoxelMetric metric,
                              const GlobalOctree &octree, const GlobalOctreeNode &parent,
                              const GlobalOctreeNode &child, uint32_t child_n /*0 to 7 - number of child*/)
  {
    float dist = 0;
    switch (metric)
    {
    case SparseOctreeSettings::VoxelMetric::BASIC:
      dist = parent_child_distance_basic(octree, parent, child, child_n);
      break;
    case SparseOctreeSettings::VoxelMetric::SAMPLES:
      dist = parent_child_distance_samples(octree, parent, child, child_n);
      break;
    case SparseOctreeSettings::VoxelMetric::MULTI:
      dist = parent_child_distance_multi(octree, parent, child, child_n);
      break;
    case SparseOctreeSettings::VoxelMetric::REFERENCE_IOU:
      dist = 1.0 - parent_child_distance_iou_multi(octree, parent, child, child_n);
      break;
    default:
      dist = INCOMPARABLE_NODES_DISTANCE;
      break;
    }
    return dist;
  }

  void init_material_texture_data(GlobalOctree &octree, const SparseOctreeSettings &settings)
  {
    DataChannel tex, mat;
    if (settings.calculate_tc)
    {
      octree.header.tc_channel_id = octree.point_channels.size();
      tex.name = "texture_coordinates";
      tex.num_components = 2;
      tex.type = DataChannel::Type::FLOAT;
      octree.point_channels.push_back(tex);
    }
    if (settings.calculate_mat_id)
    {
      octree.header.mat_channel_id = octree.voxel_channels.size();
      mat.name = "material_id";
      mat.num_components = 1;
      mat.type = DataChannel::Type::INT;
      octree.voxel_channels.push_back(mat);
    }
  }

  void prim_octree_materials(const SimpleMesh &model, const PrimitiveListOctree &pl_octree, unsigned idx, float &err)
  {
    if (pl_octree.nodes[idx].pid_count == 0) return;
    std::set<unsigned int> materials;
    for (int i = 0; i < pl_octree.nodes[idx].pid_count; ++i)
    {
      uint32_t t_i = pl_octree.primitive_ids[pl_octree.nodes[idx].pid_offset + i];
      materials.insert(model.matIndices[t_i]);
    }
    err = (float) materials.size();
  }

  void prim_octree_node_squares(const SimpleMesh &model, const PrimitiveListOctree &pl_octree, unsigned idx, 
                                float &mx, float &mn, float &average)
  {
    if (pl_octree.nodes[idx].pid_count == 0) return;
    for (int i = 0; i < pl_octree.nodes[idx].pid_count; ++i)
    {
      uint32_t t_i = pl_octree.primitive_ids[pl_octree.nodes[idx].pid_offset + i];
      float3 a = to_float3(model.vPos4f[model.indices[3*t_i+0]]);
      float3 b = to_float3(model.vPos4f[model.indices[3*t_i+1]]);
      float3 c = to_float3(model.vPos4f[model.indices[3*t_i+2]]);
      float3 ab = b - a;
      float3 ac = c - a;
      float3 cr = cross(ab, ac);
      float sq = length(cr);
      if (i == 0)
      {
        mx = sq;
        mn = sq;
        average = sq;
      }
      else
      {
        mx = std::max(mx, sq);
        mn = std::min(mn, sq);
        average += sq;
      }
    }
    average /= pl_octree.nodes[idx].pid_count;
  }


  void prune_octree(sdf_converter::GlobalOctree &out_octree, SparseOctreeSettings &settings, std::vector<uint32_t> &node_offsets_by_layer)
  {
    bool has_channels = out_octree.point_channels.size() + out_octree.voxel_channels.size() > 0;
    for (int level = node_offsets_by_layer.size() - 2; level >= settings.min_depth; --level)
    {
      if (node_offsets_by_layer[level+1] == 0)
        continue;
      unsigned nodes_offset = node_offsets_by_layer[level];
      unsigned nodes_count = node_offsets_by_layer[level + 1] - nodes_offset;
      if (nodes_count == 0)
        continue;
      
      #pragma omp parallel for
      for (unsigned n_id = nodes_offset; n_id < nodes_offset + nodes_count; n_id++)
      {
        GlobalOctreeNode &node = out_octree.nodes[n_id];
        if (node.offset == 0 || node.bricks_count == 0)
          continue;

        bool all_children_are_leaves = true;
        for (int i = 0; i < 8; ++i)
        {
          GlobalOctreeNode &ch_node = out_octree.nodes[node.offset + i];
          if (ch_node.offset != 0)
          {
            all_children_are_leaves = false;
            break;
          }
        }

        if (!all_children_are_leaves)
          continue;

        unsigned children_count = 0;
        float max_dist = 0;
        float sum_dist = 0;
        float max_ch_dist = 0;
        for (int i = 0; i < 8; ++i)
        {
          GlobalOctreeNode &ch_node = out_octree.nodes[node.offset + i];
          if (ch_node.bricks_count > 0)
          {
            float dist = parent_child_distance(settings.voxel_metric, out_octree, node, ch_node, i);
            max_dist = std::max(max_dist, dist);
            sum_dist += dist;

            if (has_channels && settings.channel_split_thr > 0.0f)
              max_ch_dist = std::max(max_ch_dist, parent_child_distance_channels(out_octree, n_id, node.offset + i, i));

            children_count++;

            if (out_octree.debug_info)
            {
              #pragma omp critical (debug_write)
              {
                auto &ch_node_info = get_node_info(out_octree.debug_info, node.offset + i);
                ch_node_info.error_from_parent = dist;
              }
            }
          }
        }

        bool prune = children_count > 0 && 
                     max_dist <= settings.split_thr &&
                     (!has_channels || max_ch_dist <= settings.channel_split_thr);
        if (prune)
        {
          for (int i = 0; i < 8; ++i)
          {
            GlobalOctreeNode &ch_node = out_octree.nodes[node.offset + i];
            ch_node.type = GlobalOctreeNodeType::EMPTY;
            ch_node.bricks_count = 0;
            ch_node.offset = 0;
            ch_node.val_off = 0;
          }

          node.type = GlobalOctreeNodeType::LEAF;
          node.offset = 0;
        }
      }
    }
  }

  void generate_LODs(SparseOctreeSettings &settings, std::vector<uint32_t> &node_offsets_by_layer, sdf_converter::GlobalOctree &out_octree)
  {
    printf("node offsets by layer:\n");
    for (int i = 0; i < settings.depth + 1; ++i)
      printf("%d ", node_offsets_by_layer[i]);
    printf("\n");

    for (int i = settings.depth - 1; i >= 0; --i)
    {
      printf("building level %d\n", i);
      unsigned nodes_offset = node_offsets_by_layer[i];
      unsigned nodes_count = node_offsets_by_layer[i + 1] - nodes_offset;
      if (nodes_count == 0)
        continue;

      for (unsigned n_id = nodes_offset; n_id < nodes_offset + nodes_count; n_id++)
      {
        fill_LOD_node(out_octree, n_id);
      }
    }
  }

  unsigned global_octree_count_and_mark_active_nodes_rec(GlobalOctree &octree, unsigned nodeId)
  {
    unsigned ofs = octree.nodes[nodeId].offset;
    if (is_leaf(ofs))
    {
      return (unsigned)(octree.nodes[nodeId].type == GlobalOctreeNodeType::LEAF);
    }   
    else
    {
      unsigned sum = 0;
      for (int i = 0; i < 8; i++)
        sum += global_octree_count_and_mark_active_nodes_rec(octree, ofs + i);
      if (sum == 0) //parent node has no active children
      {
        octree.nodes[nodeId].offset = INVALID_IDX;
        octree.nodes[nodeId].type = GlobalOctreeNodeType::EMPTY;
      }
      else  //parent node has active children so it is active too, type remains the same (NODE or EMPTY_NODE)
      {
        sum++;
      }
      return sum;
    }
  }

  void global_octree_eliminate_invalid_rec(const GlobalOctree &octree_old, unsigned oldNodeId, 
                                                 GlobalOctree &octree_new, unsigned newNodeId,
                                           unsigned *old_to_new_node_idx_remap)
  {
    unsigned size = (octree_new.header.brick_pad * 2 + octree_new.header.brick_size + 1);
    size = size * size * size;

    if ((octree_old.nodes[oldNodeId].offset & INVALID_IDX) == 0 &&
        octree_old.nodes[oldNodeId].bricks_count > 0)
    {
      unsigned prev_off = octree_old.nodes[oldNodeId].val_off;
      octree_new.nodes[newNodeId].val_off = octree_new.values_f.size();
      octree_new.values_f.insert(octree_new.values_f.end(),
                                 octree_old.values_f.begin() + prev_off,
                                 octree_old.values_f.begin() + prev_off + size * octree_old.nodes[oldNodeId].bricks_count);
    }
    else
    {
      octree_new.nodes[newNodeId].offset = 0;
      octree_new.nodes[newNodeId].val_off = 0;
      octree_new.nodes[newNodeId].bricks_count = 0;
    }

    if (old_to_new_node_idx_remap)
      old_to_new_node_idx_remap[oldNodeId] = newNodeId;
  
    unsigned ofs = octree_old.nodes[oldNodeId].offset;
    if (!is_leaf(ofs))
    {
      unsigned new_ofs = octree_new.nodes.size();
      octree_new.nodes[newNodeId].offset = new_ofs;
      for (int i = 0; i < 8; i++)
        octree_new.nodes.push_back(octree_old.nodes[ofs + i]);

      for (int i = 0; i < 8; i++)
        global_octree_eliminate_invalid_rec(octree_old, ofs + i, octree_new, new_ofs + i, old_to_new_node_idx_remap);
    }
  }
}