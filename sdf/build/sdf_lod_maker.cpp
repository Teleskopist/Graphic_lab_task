#include "sdf_lod_maker.h"

namespace sdf_converter
{
  bool is_valid_surface(const float *values);

  static constexpr int MAX_SURFACES = 256;
  static constexpr int SIDE_X_NEG = 0;
  static constexpr int SIDE_X_POS = 1;
  static constexpr int SIDE_Y_NEG = 2;
  static constexpr int SIDE_Y_POS = 3;
  static constexpr int SIDE_Z_NEG = 4;
  static constexpr int SIDE_Z_POS = 5;
  static constexpr int SIDE_COUNT = 6;

  static constexpr int NONE = -1;
  static constexpr int SURFACE_INVALID = 0;
  static constexpr int SURFACE_UNUSED  = 1;
  static constexpr int SURFACE_USED    = 2;

  struct Surface
  {
    float values[8] = {0.0f};
    bool side_intersect[SIDE_COUNT] = {false};
    unsigned child_id = 0;
    unsigned flags = 0; //temporary field, used during processing
  };

  float4 get_side(const Surface &surface, int side)
  {
    switch (side)
    {
      case SIDE_X_NEG: return float4(surface.values[0], surface.values[1], surface.values[2], surface.values[3]);
      case SIDE_X_POS: return float4(surface.values[4], surface.values[5], surface.values[6], surface.values[7]);
      case SIDE_Y_NEG: return float4(surface.values[0], surface.values[1], surface.values[4], surface.values[5]);
      case SIDE_Y_POS: return float4(surface.values[2], surface.values[3], surface.values[6], surface.values[7]);
      case SIDE_Z_NEG: return float4(surface.values[0], surface.values[2], surface.values[4], surface.values[6]);
      case SIDE_Z_POS: return float4(surface.values[1], surface.values[3], surface.values[5], surface.values[7]);
      default: assert(false);
    }
    return float4();
  }

  float maximum(float4 v)
  {
    return std::max(v.x, std::max(v.y, std::max(v.z, v.w)));
  }

  float minimum(float4 v)
  {
    return std::min(v.x, std::min(v.y, std::min(v.z, v.w)));
  }

  void calculate_side_intersect(Surface &surface)
  {
    for (int i = 0; i < SIDE_COUNT; i++)
      surface.side_intersect[i] = maximum(get_side(surface, i)) * minimum(get_side(surface, i)) < 0;
  }

  float sides_distance(float4 a, float4 b)
  {
    //printf("dintance %+.3f %+.3f -- %+.3f %+.3f\n", a.x, a.y, b.x, b.y);
    //printf("         %+.3f %+.3f -- %+.3f %+.3f\n", a.z, a.w, b.z, b.w);
    return length(normalize(a) - normalize(b));
  }

  static float bilinear_interpolation(float4 a, float2 dp)
  {
    return (1-dp.x)*(1-dp.y)*a.x + 
          (1-dp.x)*(  dp.y)*a.y + 
          (  dp.x)*(1-dp.y)*a.z + 
          (  dp.x)*(  dp.y)*a.w;
  }

  float sides_metric(float4 a, float4 b, bool along_first_axis, float step = 0.25f)
  {
    float tmp1 = 0, tmp2 = 0, tmp3 = 0, tmp4 = 0, res = 0.0f;
    float2 mul(1.0f, 0.0f), add(0.0f, 1.0f);
    unsigned cnt = 0;
    if (!along_first_axis)
    {
      mul = float2(0.0f, 1.0f);
      add = float2(1.0f, 0.0f);
    }
    for (float i = 0.0f; i <= 1.0f; i += step)
    {
      ++cnt;
      tmp1 = bilinear_interpolation(a, i * mul);
      tmp2 = bilinear_interpolation(b, i * mul);
      tmp3 = bilinear_interpolation(a, i * mul + add);
      tmp4 = bilinear_interpolation(b, i * mul + add);
      if (tmp1 * tmp2 < 0)
      {
        tmp2 = -tmp2;
        tmp4 = -tmp4;
      }
      res += length(normalize(float2(tmp2, tmp4)) - normalize(float2(tmp1, tmp3)));
    }
    return res / cnt;
  }

  static const int neighbor_codes[8*SIDE_COUNT] =
  {
    //         X_NEG,            X_POS,            Y_NEG,            Y_POS,            Z_NEG,            Z_POS
                NONE, 4*8 + SIDE_X_NEG,             NONE, 2*8 + SIDE_Y_NEG,             NONE, 1*8 + SIDE_Z_NEG,
                NONE, 5*8 + SIDE_X_NEG,             NONE, 3*8 + SIDE_Y_NEG, 0*8 + SIDE_Z_POS,             NONE,
                NONE, 6*8 + SIDE_X_NEG, 0*8 + SIDE_Y_POS,             NONE,             NONE, 3*8 + SIDE_Z_NEG,
                NONE, 7*8 + SIDE_X_NEG, 1*8 + SIDE_Y_POS,             NONE, 2*8 + SIDE_Z_POS,             NONE,
    0*8 + SIDE_X_POS,             NONE,             NONE, 6*8 + SIDE_Y_NEG,             NONE, 5*8 + SIDE_Z_NEG,
    1*8 + SIDE_X_POS,             NONE,             NONE, 7*8 + SIDE_Y_NEG, 4*8 + SIDE_Z_POS,             NONE,
    2*8 + SIDE_X_POS,             NONE, 4*8 + SIDE_Y_POS,             NONE,             NONE, 7*8 + SIDE_Z_NEG,
    3*8 + SIDE_X_POS,             NONE, 5*8 + SIDE_Y_POS,             NONE, 6*8 + SIDE_Z_POS,             NONE,     
  };

  static const int neighbors[8][SIDE_COUNT] =
  {
    {NONE,    4,    NONE,    2,    NONE,    1},
    {NONE,    5,    NONE,    3,    0,    NONE},
    {NONE,    6,    0,    NONE,    NONE,    3},
    {NONE,    7,    1,    NONE,    2,    NONE},
    {0,    NONE,    NONE,    6,    NONE,    5},
    {1,    NONE,    NONE,    7,    4,    NONE},
    {2,    NONE,    4,    NONE,    NONE,    7},
    {3,    NONE,    5,    NONE,    6,    NONE},
  };

  static const int codeA[8][8] = 
  {
    //        0,          1,          2,          3,          4,          5,          6,          7
    {      NONE, SIDE_Z_POS, SIDE_Y_POS,       NONE, SIDE_X_POS,       NONE,       NONE,       NONE},
    {SIDE_Z_NEG,       NONE,       NONE, SIDE_Y_POS,       NONE, SIDE_X_POS,       NONE,       NONE},
    {SIDE_Y_NEG,       NONE,       NONE, SIDE_Z_POS,       NONE,       NONE, SIDE_X_POS,       NONE},
    {      NONE, SIDE_Y_NEG, SIDE_Z_NEG,       NONE,       NONE,       NONE,       NONE, SIDE_X_POS},
    {SIDE_X_NEG,       NONE,       NONE,       NONE,       NONE, SIDE_Z_POS, SIDE_Y_POS,       NONE},
    {      NONE, SIDE_X_NEG,       NONE,       NONE, SIDE_Z_NEG,       NONE,       NONE, SIDE_Y_POS},
    {      NONE,       NONE, SIDE_X_NEG,       NONE, SIDE_Y_NEG,       NONE,       NONE, SIDE_Z_POS},
    {      NONE,       NONE,       NONE, SIDE_X_NEG,       NONE, SIDE_Y_NEG, SIDE_Z_NEG,       NONE},
  };

  static const int3 active_neighbor_codes[8] =
  {
    int3(4*8 + SIDE_X_NEG, 2*8 + SIDE_Y_NEG, 1*8 + SIDE_Z_NEG),
    int3(5*8 + SIDE_X_NEG, 3*8 + SIDE_Y_NEG, 0*8 + SIDE_Z_POS),
    int3(6*8 + SIDE_X_NEG, 0*8 + SIDE_Y_POS, 3*8 + SIDE_Z_NEG),
    int3(7*8 + SIDE_X_NEG, 1*8 + SIDE_Y_POS, 2*8 + SIDE_Z_POS),
    int3(0*8 + SIDE_X_POS, 6*8 + SIDE_Y_NEG, 5*8 + SIDE_Z_NEG),
    int3(1*8 + SIDE_X_POS, 7*8 + SIDE_Y_NEG, 4*8 + SIDE_Z_POS),
    int3(2*8 + SIDE_X_POS, 4*8 + SIDE_Y_POS, 7*8 + SIDE_Z_NEG),
    int3(3*8 + SIDE_X_POS, 5*8 + SIDE_Y_POS, 6*8 + SIDE_Z_POS),
  };

  void print_surface(const Surface &surface)
  {
    printf(R""""( 
        ^ Y
        |
     (% .4f)--------(% .4f)
       /|              /|
      / |             / |
     /  |            /  |
 (% .4f)-------(% .4f)
    |   |           |   |
    |(% .4f)------|(% .4f)--> X
    |  /            |  /
    | /             | /
    |/              |/
 (% .4f)-------(% .4f)
   /
  Z 
    )"""", 100*surface.values[2], 100*surface.values[6], 100*surface.values[3], 100*surface.values[7], 
           100*surface.values[0], 100*surface.values[4], 100*surface.values[1], 100*surface.values[5]);
  printf("\n");
  }

  struct CornerValues
  {
    float values[8] = {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000};
    int count = 0;
  };

  void fill_corner_values(const Surface *all_surfaces, int surface_ids[8],
                          CornerValues brickCorners[3][3][3])
  {
    // Populate the brickCorners with values from each voxel's corners
    for (int voxel = 0; voxel < 8; voxel++)
    {
      int z_vox = voxel % 2;
      int y_vox = (voxel / 2) % 2;
      int x_vox = voxel / 4;

      for (int corner = 0; corner < 8; corner++)
      {
        int dz = corner & 1;
        int dy = (corner >> 1) & 1;
        int dx = (corner >> 2) & 1;

        int X = x_vox + dx;
        int Y = y_vox + dy;
        int Z = z_vox + dz;

        CornerValues *current = &brickCorners[X][Y][Z];
        if (surface_ids[voxel] != NONE)
          current->values[current->count++] = all_surfaces[surface_ids[voxel]].values[corner];
      }
    }
  }

  void printBrick(float brick[8][8], bool single_corner_value)
  {
    CornerValues brickCorners[3][3][3] = {{{0}}}; // Initialize all to zero

    // Populate the brickCorners with values from each voxel's corners
    for (int voxel = 0; voxel < 8; voxel++)
    {
      int z_vox = voxel % 2;
      int y_vox = (voxel / 2) % 2;
      int x_vox = voxel / 4;

      for (int corner = 0; corner < 8; corner++)
      {
        int dz = corner & 1;
        int dy = (corner >> 1) & 1;
        int dx = (corner >> 2) & 1;

        int X = x_vox + dx;
        int Y = y_vox + dy;
        int Z = z_vox + dz;

        if (X < 3 && Y < 3 && Z < 3)
        { // Ensure within bounds
          struct CornerValues *current = &brickCorners[X][Y][Z];
          if (current->count < 8)
          {
            current->values[current->count++] = brick[voxel][corner];
          }
        }
      }
    }

    // Print each Z layer
    for (int y = 2; y >= 0; y--)
    {
      printf("Layer Y=%d:\n", y);
      for (int z = 0; z < 3; z++)
      {
        printf("Z=%d: ", z);
        for (int x = 0; x < 3; x++)
        {
          struct CornerValues *corner = &brickCorners[x][y][z];
          if (single_corner_value)
          {
            float base_value = corner->values[0];
            for (int i = 0; i < corner->count; i++)
            {
              base_value = std::min(base_value, corner->values[i]);
            }
            if (base_value < 1000)
              printf("[%+.3f] ", 100 * base_value);
            else
              printf("[------] ");
          }
          else
          {
            printf("[");
            for (int i = 0; i < corner->count; i++)
            {
              if (i > 0)
                printf(", ");
              if (corner->values[i] < 1000)
                printf("%+.3f", 100 * corner->values[i]);
              else
                printf("------");
            }
            printf("] ");
            for (int i = 0; i < 8 * (8 - corner->count); i++)
              printf(" ");
          }
        }
        printf("\n");
      }
      printf("\n");
    }
  }

  uint3 v_id_code(unsigned id)
  {
    return uint3(id/4, (id/2)%2, id%2);
  }

  bool eq(uint3 a, uint3 b)
  {
    return a.x == b.x && a.y == b.y && a.z == b.z;
  }

  void calculate_and_print_edge_connections()
  {
    struct Edge
    {
      unsigned ch_id; //0 to 8, voxel
      unsigned from;  //0 to 8, value, both for child and parent
      unsigned to;    //0 to 8, value, both for child and parent
      unsigned second; //0 or 1
    };
    Edge edges[24] = {
      {0, 0, 1, 0}, {1, 0, 1, 1}, {0, 0, 2, 0}, {2, 0, 2, 1}, 
      {2, 2, 3, 0}, {3, 2, 3, 1}, {1, 1, 3, 0}, {3, 1, 3, 1},
      {4, 4, 5, 0}, {5, 4, 5, 1}, {4, 4, 6, 0}, {6, 4, 6, 1}, 
      {6, 6, 7, 0}, {7, 6, 7, 1}, {5, 5, 7, 0}, {7, 5, 7, 1}, 
      {0, 0, 4, 0}, {4, 0, 4, 1}, {2, 2, 6, 0}, {6, 2, 6, 1},
      {1, 1, 5, 0}, {5, 1, 5, 1}, {3, 3, 7, 0}, {7, 3, 7, 1}       
    };

    int edge_connections[24][6];
    for (int i=0;i<24;i++)
    {
      int c = 0;
      for (int j=0;j<6;j++)
        edge_connections[i][j] = -1;
      for (int j=0;j<24;j++)
      {
        if (i == j)
          continue;
        uint3 codeA1 = v_id_code(edges[i].ch_id) + v_id_code(edges[i].from);
        uint3 codeB1 = v_id_code(edges[i].ch_id) + v_id_code(edges[i].to);
        uint3 codeA2 = v_id_code(edges[j].ch_id) + v_id_code(edges[j].from);
        uint3 codeB2 = v_id_code(edges[j].ch_id) + v_id_code(edges[j].to);
        if (eq(codeA1, codeA2) || eq(codeA1, codeB2) || eq(codeB1, codeA2) || eq(codeB1, codeB2))
        {
          edge_connections[i][c] = j;
          c++;
        }
      }
    }
    for (int i=0;i<24;i++)
    {
      printf("{%d %d %d}, ", edge_connections[i][0], edge_connections[i][1], edge_connections[i][2]);
      if ((i+1)%4 == 0)
        printf("\n");
    }
  }

  void fill_LOD_node(GlobalOctree &octree, unsigned node_idx, bool verbose)
  {
    constexpr float max_side_distance = 0.1f;

    //no children/valid children, nothing to do
    if (octree.nodes[node_idx].offset == 0 || octree.nodes[node_idx].offset == INVALID_IDX)
      return;
    
    //TODO: support for channel data
    assert(octree.voxel_channels.size() == 0 && octree.point_channels.size() == 0);

    unsigned total_surface_count = 0;
    unsigned surface_offsets[9];
    Surface  all_surfaces[8*MAX_SURFACES];
    float brick[8][8];

    //fill array with initial surfaces
    surface_offsets[0] = 0;
    for (int i = 0; i < 8; i++)
    {
      const auto &node = octree.nodes[octree.nodes[node_idx].offset + i];
      if (node.bricks_count == 0)
      {
        for (int k = 0; k < 8; k++)
          brick[i][k] = 1000;
      }
      for (int j = 0; j < node.bricks_count; j++)
      {
        if (is_valid_surface(octree.values_f.data() + node.val_off + j * 8))
        {
          for (int k = 0; k < 8; k++)
            brick[i][k] = octree.values_f[node.val_off + j * 8 + k];

          for (int k = 0; k < 8; k++)
            all_surfaces[total_surface_count].values[k] = octree.values_f[node.val_off + j * 8 + k];
          
          all_surfaces[total_surface_count].flags = SURFACE_UNUSED;
          all_surfaces[total_surface_count].child_id = i;
          calculate_side_intersect(all_surfaces[total_surface_count]);
          total_surface_count++;
        }
      }
      surface_offsets[i+1] = total_surface_count;
    }

    if (total_surface_count == 0)
      return;

    if (verbose)
    {
    for (int j=0;j<8;j++)
      all_surfaces[total_surface_count+1].values[j] = 0.08888f;

    for (int i=0;i<2;i++)
    {
      for (int j=0;j<2;j++)
      {
        for (int k=0;k<2;k++)
        {
          unsigned off = surface_offsets[4*i + 2*j + k];
          if (off == surface_offsets[4*i + 2*j + k + 1])
            off = total_surface_count+1;
          printf("%d (%d %d %d) off = %d\n", 4*i + 2*j + k, i, j, k, off);
          print_surface(all_surfaces[off]);
        }
      }
    }
    printBrick(brick, false);
    printBrick(brick, true);
    printf("\n\n");
    }

    //trying to merge surfaces into multi-surface (surface for parent that consists of multiple child surfaces)
    //make LOD surface for each multi-surface independently
    unsigned surfaces_left = total_surface_count;
    unsigned multi_surface_count = 0;
    while (surfaces_left > 0)
    {
      //first, find some unused surface as a starting point
      unsigned surface_id = 0;
      while (all_surfaces[surface_id].flags != SURFACE_UNUSED)
        surface_id++;
      
      //surface ids of current multi-surface
      int surface_ids[8];
      int2 codes_stack[3*8]; //parent ch_id + code
      int top = 0;
      for (int i = 0; i < 8; i++)
        surface_ids[i] = NONE;
      
      int start_ch_id = all_surfaces[surface_id].child_id;
      surface_ids[start_ch_id] = surface_id;
      surfaces_left--;
      all_surfaces[surface_id].flags = SURFACE_USED;
      for (int i = 0; i < SIDE_COUNT; i++)
      {
        if (all_surfaces[surface_id].side_intersect[i] && neighbors[start_ch_id][i] != NONE)
          codes_stack[top++] = int2(start_ch_id, neighbors[start_ch_id][i]);
      }

      bool hanging_edges = false;
      while (top > 0 && !hanging_edges)
      {
        int2 code = codes_stack[--top];
        //printf("testing %d %d side %d\n", code.x, code.y, codeA[code.x][code.y]);
        float4 side_a = get_side(all_surfaces[surface_ids[code.x]], codeA[code.x][code.y]);
        
        //find surface in id2 with best match for surface in id1
        //if it is found, add it to multi-surface and add its neighbors to the stack
        if (surface_ids[code.y] == NONE)
        {
          float min_distance = max_side_distance;
          int min_surface_id = NONE;
          //printf("check surfaces %d - %d\n", surface_offsets[code.y], surface_offsets[code.y + 1]);
          for (int i = surface_offsets[code.y]; i < surface_offsets[code.y + 1]; i++)
          {
            if (all_surfaces[i].flags != SURFACE_UNUSED)
              continue;
            float dist = sides_distance(side_a, get_side(all_surfaces[i], codeA[code.y][code.x]));
            if (dist < min_distance)
            {
              min_distance = dist;
              min_surface_id = i;
            }
          }
          if (min_surface_id != NONE)
          {
            surface_ids[code.y] = min_surface_id;
            surfaces_left--;
            all_surfaces[min_surface_id].flags = SURFACE_USED;
            for (int i = 0; i < SIDE_COUNT; i++)
            {
              if (all_surfaces[min_surface_id].side_intersect[i] && neighbors[code.y][i] != NONE)
                codes_stack[top++] = int2(code.y, neighbors[code.y][i]);
            }
          }
          else
            hanging_edges = true;
        }
        else //check if distance between surface in id1 and id2 is small enough, otherwise it is hanging edge
        {
          float4 side_b = get_side(all_surfaces[surface_ids[code.y]], codeA[code.y][code.x]);
          if (sides_distance(side_a, side_b) >= max_side_distance)
            hanging_edges = true;
        }
      }

      if (hanging_edges)
      {
        printf("discarded multi-surface %d because of hanging edge\n", multi_surface_count);
        continue;
      }

      if (verbose)
        printf("found multi-surface %d\n", multi_surface_count);
      multi_surface_count++;

      CornerValues brickCorners[3][3][3] = {{{0}}}; // Initialize all to zero
      fill_corner_values(all_surfaces, surface_ids, brickCorners);
      
      constexpr float VALUE_UNDEFINED = 1000.0f;
      float new_values[8] = {VALUE_UNDEFINED, VALUE_UNDEFINED, VALUE_UNDEFINED, VALUE_UNDEFINED, 
                             VALUE_UNDEFINED, VALUE_UNDEFINED, VALUE_UNDEFINED, VALUE_UNDEFINED};
    
      //I. Iterate over all outer edges in child voxels and force new surface to intersect each of them
      //   exactly in the same point that it does in the child voxel.
      
      struct Edge
      {
        unsigned ch_id; //0 to 8, voxel
        unsigned from;  //0 to 8, value, both for child and parent
        unsigned to;    //0 to 8, value, both for child and parent
        unsigned second; //0 or 1
      };
      Edge edges[24] = {
        {0, 0, 1, 0}, {1, 0, 1, 1}, {0, 0, 2, 0}, {2, 0, 2, 1}, 
        {2, 2, 3, 0}, {3, 2, 3, 1}, {1, 1, 3, 0}, {3, 1, 3, 1},
        {4, 4, 5, 0}, {5, 4, 5, 1}, {4, 4, 6, 0}, {6, 4, 6, 1}, 
        {6, 6, 7, 0}, {7, 6, 7, 1}, {5, 5, 7, 0}, {7, 5, 7, 1}, 
        {0, 0, 4, 0}, {4, 0, 4, 1}, {2, 2, 6, 0}, {6, 2, 6, 1},
        {1, 1, 5, 0}, {5, 1, 5, 1}, {3, 3, 7, 0}, {7, 3, 7, 1}       
      };

      bool match_failed = false;
      for (const Edge &edge : edges)
      {
        if (surface_ids[edge.ch_id] == NONE)
          continue;
        
        float v0 = all_surfaces[surface_ids[edge.ch_id]].values[edge.from];
        float v1 = all_surfaces[surface_ids[edge.ch_id]].values[edge.to];
        if (v0*v1 >= 0)
          continue;
        
        float q = 1/(1 - v1/v0);

        //initialize one of the values in parent
        if (new_values[edge.from] == VALUE_UNDEFINED && new_values[edge.to] == VALUE_UNDEFINED)
        {
          if (edge.second)
            new_values[edge.to] = v1;
          else
            new_values[edge.from] = v0;
        }
        
        float q_new = edge.second ? (q + 1)/2 : q/2;
        if (new_values[edge.from] == VALUE_UNDEFINED)
        {
          new_values[edge.from] = (1/(q_new-1))*new_values[edge.to];
        }
        else if (new_values[edge.to] == VALUE_UNDEFINED)
        {
          new_values[edge.to] = ((q_new-1)/q_new)*new_values[edge.from];
        }
        else //both values are defined
        {
          float q_real = 1/(1 - new_values[edge.from]/new_values[edge.to]);
          float dq = std::abs(q_real - q_new);
          printf("dq %f\n", dq);
          if (dq > 1e-5f) //cannot find values that match this edge equation without breaking others
          {
            match_failed = true;
            for (int i=0;i<8;i++)
              new_values[i] = VALUE_UNDEFINED;
            break;
          }
        }
      }

      calculate_and_print_edge_connections();

      if (!match_failed)
      {
      Surface ns;
      for (int i=0;i<8;i++)
        ns.values[i] = new_values[i];
      print_surface(ns);
      printBrick(brick, true);
      }
      // printf("new values %f %f %f %f %f %f %f %f\n", new_values[0], new_values[1], new_values[2], new_values[3],
      //        new_values[4], new_values[5], new_values[6], new_values[7]);

      //II. If there are some undefined values left (which is usually the case), find the best values
      //    by minimizing the difference in distance values in the children's corners.
    }
  }
}