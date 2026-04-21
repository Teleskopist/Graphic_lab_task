#include "utils/mesh/marching_cubes_lookup_table.h"
#include "clustering/symmetric_groups.h"
#include "marching_cubes_scom2.h"
#include "scom_utils.h"

#include "omp.h"

namespace cmesh4
{

  void collect_dag_leaves(SdfDAG octree, std::vector<uint4> &data, std::vector<uint32_t> &levels, std::vector<uint32_t> &idxs, std::vector<uint32_t> &transforms)
  {
    auto func = [&data, &levels, &idxs, &transforms](const SdfDAG &dag, uint32_t nodeId, 
                                                    uint32_t transformId, uint32_t level, uint4 code)
    {
      uint32_t child_off = dag.nodes[nodeId].children_edges_offset;
      if (child_off == 0)
      {
        uint32_t data_off = dag.nodes[nodeId].data_edges_offset;
        if (data_off != 0)
        {
          idxs.push_back(nodeId);
          data.push_back(code);
          transforms.push_back(transformId);
          levels.push_back(level);
        }
        return false;
      }
      return true;
    };
    scom2::traverse_DAG(octree, func);
  }

  float3 grad(float *val, float3 p)
  {
    float dx = (1 - p.y) * (1 - p.z) * (val[4] - val[0]) + 
               (1 - p.y) *      p.z  * (val[5] - val[1]) + 
                    p.y  * (1 - p.z) * (val[6] - val[2]) + 
                    p.y  *      p.z  * (val[7] - val[3]);
    
    float dy = (1 - p.x) * (1 - p.z) * (val[2] - val[0]) + 
               (1 - p.x) *      p.z  * (val[3] - val[1]) + 
                    p.x  * (1 - p.z) * (val[6] - val[4]) + 
                    p.x  *      p.z  * (val[7] - val[5]);

    float dz = (1 - p.y) * (1 - p.x) * (val[1] - val[0]) + 
               (1 - p.y) *      p.x  * (val[5] - val[4]) + 
                    p.y  * (1 - p.x) * (val[3] - val[2]) + 
                    p.y  *      p.x  * (val[7] - val[6]);
    
    return float3(dx, dy, dz);
  }

  float trilinear_interp(float *val, float3 p)
  {
    return val[0] * (1 - p.x) * (1 - p.y) * (1 - p.z) + 
           val[1] * (1 - p.x) * (1 - p.y) *      p.z  + 
           val[2] * (1 - p.x) *      p.y  * (1 - p.z) + 
           val[3] * (1 - p.x) *      p.y  *      p.z  + 
           val[4] *      p.x  * (1 - p.y) * (1 - p.z) + 
           val[5] *      p.x  * (1 - p.y) *      p.z  + 
           val[6] *      p.x  *      p.y  * (1 - p.z) + 
           val[7] *      p.x  *      p.y  *      p.z;
  }

  cmesh4::SimpleMesh create_mesh_marching_cubes_scom2(MarchingCubesSettings settings, SCom2Tree octree, unsigned max_threads, unsigned local_depth)
  {
    assert(octree.header.dimension == 3);
    assert(local_depth > 0);

    std::vector<uint4> data;
    std::vector<uint32_t> levels;
    std::vector<uint32_t> transforms;
    std::vector<uint32_t> idxs;

    SdfDAG dag;
    scom2::unpack_SCom2(octree, dag);
    collect_dag_leaves(dag, data, levels, idxs, transforms);

    uint32_t v_size = dag.header.brick_size + 2*dag.header.brick_pad + 1;
    
    const scom2::Subgroup *bricks_sg = scom2::create_subgroup((scom2::TransformSubgroup)dag.header.transform_subgroup,
                                                            dag.header.brick_size + 2 * dag.header.brick_pad + 1);
    std::vector<uint32_t> transpositions;
    transpositions.clear();
    transpositions.reserve(bricks_sg->elements.size()*bricks_sg->elements[0].indices.size());
    for (auto &elem : bricks_sg->elements)
      for (auto &t : elem.indices)
        transpositions.push_back(t);

    float3 size = settings.max_pos - settings.min_pos;
    std::vector<std::vector<float3>> vertices(max_threads);
    std::vector<std::vector<float3>> normals(max_threads);
    //max_threads = 1;
    #pragma omp parallel for num_threads(max_threads) 
    for (int thread_id = 0; thread_id < max_threads; thread_id++)
    {
      unsigned steps = (data.size() + max_threads - 1) / max_threads;
      unsigned start = thread_id * steps;
      unsigned end = std::min((thread_id + 1) * steps, (unsigned int)data.size());
      for (int i = start; i < end; ++i)
      {
        // float cubeValues[8];
        // float3 cubePositions[8];
        // float3 vertlist[12];
        // unsigned cubeIndex = 0;

        for (int j = 0; j < DAG_extract_count(dag.nodes[idxs[i]].voxel_count_flags); ++j)
        {
          for (int xi = dag.header.brick_pad; xi < v_size - dag.header.brick_pad - 1; ++xi)
          {
            for (int yi = dag.header.brick_pad; yi < v_size - dag.header.brick_pad - 1; ++yi)
            {
              for (int zi = dag.header.brick_pad; zi < v_size - dag.header.brick_pad - 1; ++zi)
              {
                float vox_vals[8];
                uint32_t data_edge_off = dag.nodes[idxs[i]].data_edges_offset + j;
                int4 voxelPosI = int4(xi, yi, zi, 1);

                uint32_t data_off = dag.data_edges[data_edge_off].data_offset;
                uint32_t rot_id = dag.data_edges[data_edge_off].rotation_id;
                float add = dag.data_edges[data_edge_off].add;

                uint32_t rotIdx = bricks_sg->cayley_table[rot_id][transforms[i]];

                for (int l = 0; l < 8; ++l)
                {
                  int4 pI = voxelPosI + int4(l >> 2, (l >> 1) & 1, l & 1, 0);
                  int idx = pI.x*int(v_size*v_size) + pI.y*int(v_size) + pI.z;
                  uint32_t vId0 = transpositions[rotIdx*v_size*v_size*v_size + idx];
                  float val = dag.distances[data_off + vId0] + add;
                  vox_vals[l] = val;
                }

                for (int local = 0; local < local_depth * local_depth * local_depth; ++local)
                {
                  float cubeValues[8];
                  float3 cubePositions[8];
                  float3 vertlist[12];
                  unsigned cubeIndex = 0;

                  float3 loc_p = float3((local / local_depth) / local_depth, (local / local_depth) % local_depth, local % local_depth);
                  for (int l = 0; l < 8; l++)
                  {
                    float val = trilinear_interp(vox_vals, (loc_p + pOffsets[l]) / (float)local_depth);
                    cubeValues[l] = val;

                    uint4 data_i = data[i];
                    float3 pos = local_depth*dag.header.brick_size*float3(data_i.x, data_i.y, data_i.z) + local_depth*float3(xi, yi, zi) - local_depth*dag.header.brick_pad + loc_p + pOffsets[l];
                    float sz = std::pow(dag.header.node_grid_size, levels[i]) * dag.header.brick_size * local_depth;
                    cubePositions[l] = settings.min_pos + size*((pos) / float3(sz));
                  }

                  for (int l = 0; l < 8; l++)
                  {
                    if (cubeValues[l] < settings.iso_level)
                      cubeIndex |= (1 << l);
                  }

                  if (edgeTable[cubeIndex] == 0)
                    continue;
                  
                  // Find the vertices where the surface intersects the cube
                  if (edgeTable[cubeIndex] & 1)
                    vertlist[0] = vertex_interp(settings.iso_level, cubePositions[0], cubePositions[1], cubeValues[0], cubeValues[1]);
                  if (edgeTable[cubeIndex] & 2)
                    vertlist[1] = vertex_interp(settings.iso_level, cubePositions[1], cubePositions[2], cubeValues[1], cubeValues[2]);
                  if (edgeTable[cubeIndex] & 4)
                    vertlist[2] = vertex_interp(settings.iso_level, cubePositions[2], cubePositions[3], cubeValues[2], cubeValues[3]);
                  if (edgeTable[cubeIndex] & 8)
                    vertlist[3] = vertex_interp(settings.iso_level, cubePositions[3], cubePositions[0], cubeValues[3], cubeValues[0]);
                  if (edgeTable[cubeIndex] & 16)
                    vertlist[4] = vertex_interp(settings.iso_level, cubePositions[4], cubePositions[5], cubeValues[4], cubeValues[5]);
                  if (edgeTable[cubeIndex] & 32)
                    vertlist[5] = vertex_interp(settings.iso_level, cubePositions[5], cubePositions[6], cubeValues[5], cubeValues[6]);
                  if (edgeTable[cubeIndex] & 64)
                    vertlist[6] = vertex_interp(settings.iso_level, cubePositions[6], cubePositions[7], cubeValues[6], cubeValues[7]);
                  if (edgeTable[cubeIndex] & 128)
                    vertlist[7] = vertex_interp(settings.iso_level, cubePositions[7], cubePositions[4], cubeValues[7], cubeValues[4]);
                  if (edgeTable[cubeIndex] & 256)
                    vertlist[8] = vertex_interp(settings.iso_level, cubePositions[0], cubePositions[4], cubeValues[0], cubeValues[4]);
                  if (edgeTable[cubeIndex] & 512)
                    vertlist[9] = vertex_interp(settings.iso_level, cubePositions[1], cubePositions[5], cubeValues[1], cubeValues[5]);
                  if (edgeTable[cubeIndex] & 1024)
                    vertlist[10] = vertex_interp(settings.iso_level, cubePositions[2], cubePositions[6], cubeValues[2], cubeValues[6]);
                  if (edgeTable[cubeIndex] & 2048)
                    vertlist[11] = vertex_interp(settings.iso_level, cubePositions[3], cubePositions[7], cubeValues[3], cubeValues[7]);

                  //float3 worldPos = settings.min_pos + size*(float3(xi, yi, zi) / float3(settings.size));
                  const int *edges = triTable[cubeIndex];

                  float sz = std::pow(dag.header.node_grid_size, levels[i]) * dag.header.brick_size * local_depth;
                  uint4 data_i = data[i];
                  for (int i = 0; edges[i] != -1; i++)
                  {
                    float3 p = vertlist[edges[i]];

                    float3 position = (((p - settings.min_pos) / size) * float3(sz)) - 
                                      local_depth*dag.header.brick_size*float3(data_i.x, data_i.y, data_i.z) + local_depth*dag.header.brick_pad - 
                                      local_depth*float3(xi, yi, zi) - loc_p;

                    float vox[8];
                    for (int i = 0; i < 8; ++i)
                    {
                      vox[(int)pOffsets[i].x * 4 + (int)pOffsets[i].y * 2 + (int)pOffsets[i].z] = cubeValues[i];
                    }
                    float3 n = -normalize(grad(vox, position) + float3(1e-8f, 1e-8f, 1e-8f));

                    vertices[thread_id].push_back(p);
                    normals[thread_id].push_back(n);
                  }
                }
              }
            }
          }
        }
      }
    }

    cmesh4::SimpleMesh mesh;
    unsigned mesh_size = 0;
    for (auto &vvec : vertices)
      mesh_size += vvec.size();

    mesh.vPos4f.reserve(mesh_size);
    mesh.vNorm4f.reserve(mesh_size);
    mesh.vTexCoord2f.reserve(mesh_size);
    mesh.vTang4f.reserve(mesh_size);

    mesh.matIndices.reserve(mesh_size/3);
    mesh.indices.reserve(mesh_size);

    for (int vvec=0; vvec<vertices.size(); vvec++)
    {
      for (int i=0;i<vertices[vvec].size();i+=3)
      {
        float3 a = vertices[vvec][i];
        float3 b = vertices[vvec][i+1];
        float3 c = vertices[vvec][i+2];
        float3 n1 = normals[vvec][i];
        float3 n2 = normals[vvec][i+1];
        float3 n3 = normals[vvec][i+2];

        float3 tang1 = LiteMath::normalize(LiteMath::cross(n1, a - b));
        float3 tang2 = LiteMath::normalize(LiteMath::cross(n2, b - c));
        float3 tang3 = LiteMath::normalize(LiteMath::cross(n3, c - a));

        float2 tc = float2(0, 0);

        mesh.vPos4f.push_back(LiteMath::to_float4(a, 1));
        mesh.vPos4f.push_back(LiteMath::to_float4(b, 1));
        mesh.vPos4f.push_back(LiteMath::to_float4(c, 1));

        mesh.vNorm4f.push_back(LiteMath::to_float4(n1, 0));
        mesh.vNorm4f.push_back(LiteMath::to_float4(n2, 0));
        mesh.vNorm4f.push_back(LiteMath::to_float4(n3, 0));

        mesh.vTang4f.push_back(LiteMath::to_float4(tang1, 0));
        mesh.vTang4f.push_back(LiteMath::to_float4(tang2, 0));
        mesh.vTang4f.push_back(LiteMath::to_float4(tang3, 0));

        mesh.vTexCoord2f.push_back(tc);
        mesh.vTexCoord2f.push_back(tc);
        mesh.vTexCoord2f.push_back(tc);

        mesh.matIndices.push_back(0);

        mesh.indices.push_back(mesh.indices.size());
        mesh.indices.push_back(mesh.indices.size());
        mesh.indices.push_back(mesh.indices.size());
      }
    }

    printf("Marching Cubes: %u vertices, %u triangles\n", (unsigned)mesh.vPos4f.size(), (unsigned)mesh.indices.size()/3);

    return mesh;
    //return cmesh4::SimpleMesh();
  }
}