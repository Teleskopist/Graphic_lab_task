//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common_slang_slang.h"

[[vk::binding(0, 0)]] RWStructuredBuffer<float4> out_color; // 
[[vk::binding(1, 0)]] RWStructuredBuffer<uint> m_RotAddTable; // 
[[vk::binding(2, 0)]] RWStructuredBuffer<int4> m_SdfCompactOctreeRotModifiers; // 
[[vk::binding(3, 0)]] RWStructuredBuffer<int> m_SdfDAGBlockIds; // 
[[vk::binding(4, 0)]] RWStructuredBuffer<SdfDAGChildEdge> m_SdfDAGChildEdges; // 
[[vk::binding(5, 0)]] RWStructuredBuffer<SdfDAGDataEdge> m_SdfDAGDataEdges; // 
[[vk::binding(6, 0)]] RWStructuredBuffer<float> m_SdfDAGDistances; // 
[[vk::binding(7, 0)]] RWStructuredBuffer<SdfDAGHeader> m_SdfDAGHeaders; // 
[[vk::binding(8, 0)]] RWStructuredBuffer<SdfDAGNode> m_SdfDAGNodes; // 
[[vk::binding(9, 0)]] RWStructuredBuffer<BlockMetadata> m_blocks; // 
[[vk::binding(10, 0)]] RWStructuredBuffer<float4> m_colorBuffer; // 
[[vk::binding(11, 0)]] RWStructuredBuffer<Light> m_lights; // 
[[vk::binding(12, 0)]] RWStructuredBuffer<uint> m_packedXY; // 
[[vk::binding(13, 0)]] Texture2D<float4> m_textures[]; //
[[vk::binding(13, 0)]] SamplerState m_textures_sam[];
[[vk::binding(14, 0)]] StructuredBuffer<VoxelRenderer_slang_UBO_Data> ubo;

void initEyeRay(uint x, uint y, uint s_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) ;

CRT_Hit IntersectDAG(float4 posAndNear, float4 dirAndFar) ;

float4 Resolve(in CRT_Hit hit, float4 posAndNear, float4 dirAndFar) ;



void initEyeRay(uint x, uint y, uint s_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) {
  uint spp_sqrt = uint32_t(sqrt(ubo[0].m_preset.spp));
  float i_spp_sqrt = 1.0f/spp_sqrt;
  float2 d = (ubo[0].m_AAFrameNum >= 2) ? rand2(ubo[0].m_seed, x, y, s_id + ubo[0].m_AAFrameNum + ubo[0].m_seed % 2) : 
                                   i_spp_sqrt*float2(s_id/spp_sqrt+0.5,s_id%spp_sqrt+0.5);

  float3 rayDir = EyeRayDirNormalized((float(x)+d.x)/float(ubo[0].m_packedXY_width), (float(y)+d.y)/float(ubo[0].m_packedXY_height), ubo[0].m_projInv);
  float3 rayPos = float3(0,0,0);

  transform_ray3f(ubo[0].m_worldViewInv, 
                  rayPos, rayDir);
  
  float rayNear = 1e-5f;
  float rayFar  =  1e5f;
  float sgn = sign(dot(rayPos - ubo[0].m_cuttingPlane.pos, ubo[0].m_cuttingPlane.normal));
  float denom = ubo[0].m_cuttingPlane.is_active > 0 ? dot(ubo[0].m_cuttingPlane.normal, rayDir) : 0.0f;
  rayNear = sgn*abs(denom) < -1e-6f ? dot(ubo[0].m_cuttingPlane.pos - rayPos, ubo[0].m_cuttingPlane.normal) / denom : rayNear;
  rayFar  = sgn*abs(denom) >  1e-6f ? dot(ubo[0].m_cuttingPlane.pos - rayPos, ubo[0].m_cuttingPlane.normal) / denom : rayFar;
  rayPosAndNear = to_float4(rayPos, rayNear < 0 ? rayFar : rayNear);
  rayDirAndFar  = to_float4(rayDir, rayFar < 0 ?    1e9f :  rayFar);
}

CRT_Hit IntersectDAG(float4 posAndNear, float4 dirAndFar) {
  CRT_Hit hit;
  hit.geomId = INVALID_IDX;
  hit.instId = INVALID_IDX;
  hit.primId = INVALID_IDX;
  hit.t = dirAndFar.w;
  hit.coords[0] = 1;
  hit.coords[1] = 0;
  hit.coords[2] = 0;
  hit.coords[3] = 0;

#ifndef DISABLE_SDF_DAG
  //It is a naive version that iterates all child nodes, instead of traversing it in octree-like fashion
  //but DAG is a mostly for debugging similarity compression, so it does not really matter
  //Still, we should optimize it if performance testing of octree/64-tree is considered

  const float3 ray_pos = to_float3(posAndNear);
  const float3 ray_dir = to_float3(dirAndFar);
  const float tNear = posAndNear.w;
  const float tFar = dirAndFar.w;
  const float3 ray_dir_inv = SafeInverse(ray_dir);
  OTStackElement stack[32];

  int top = 0;
  int buf_top = 0;
  uint3 p;
  float3 p_f, t0, t1, tm;
  int currNode;
  uint32_t nodeId;
  uint32_t level_sz;
  float d, level_sz_f, sz_inv;
  float2 fNearFar;
  float values[8];
  SdfDAGHeader header = m_SdfDAGHeaders[0];//m_SdfDAGHeaders[m_geomData[geomId].offset.y];

  stack[top].nodeId = 0;//m_geomData[geomId].offset.x;
  stack[top].info = 0; //info here is cumulative rotation id of the node
  stack[top].p_size = uint2(0,1);

  while (top >= 0)
  {
    OTStackElement curElem = stack[top];
    top--;

    SdfDAGNode node = m_SdfDAGNodes[curElem.nodeId];
    level_sz = curElem.p_size.y & 0xFFFF;
    level_sz_f = float(level_sz);
    p = uint3(curElem.p_size.x >> 16,curElem.p_size.x & 0xFFFF,curElem.p_size.y >> 16);
    
    sz_inv = 2.0f/level_sz_f;
    float3 brick_min_pos = float3(-1,-1,-1) + sz_inv*float3(p);
    float3 brick_max_pos = brick_min_pos + sz_inv*float3(1,1,1);
    float3 brick_size = brick_max_pos - brick_min_pos;

    float2 brick_fNearFar = RayBoxIntersection2(ray_pos, ray_dir_inv, brick_min_pos, brick_max_pos);
    brick_fNearFar.x = max(tNear, brick_fNearFar.x);

#ifdef ON_CPU
    //printf("[%d %d %d %d] node %d rot %d\n", p.x, p.y, p.z, level_sz, curElem.nodeId, curElem.info);
#endif
    if (brick_fNearFar.x >= brick_fNearFar.y)
    {
      continue;
    }

    bool is_leaf = node.children_edges_offset == 0;
    // if (!is_leaf)
    // {
    //   float t = m_preset.fixed_lod > 0 ? 1.0f : brick_fNearFar.x;
    //   float target_lod_size = (std::pow(2.0f, m_preset.level_of_detail) - 0.01f) / t;
    //   is_leaf = float(level_sz) > target_lod_size;
    // }

    if (is_leaf)
    {
      //intersect with grid inside leaf
      if (DAG_node_is_full(node.voxel_count_flags) && node.data_edges_offset > 0)
      {
        //intersect leaf
        float sz = level_sz_f;
        d = 2.0f/(sz*header.brick_size);
        bool is_surface = DAG_extract_is_surface(node.voxel_count_flags);
        uint surface_count = DAG_extract_count(node.voxel_count_flags);
        uint primId = node.channels_edge.child_offset; // primId is used to access channel data in Resolve pass
                                                           // so we must put channels offset here to apply sim. comp. to channels

        while (brick_fNearFar.x < brick_fNearFar.y)
        {
          uint v_size = header.brick_size + 2*header.brick_pad + 1;
          float3 hit_pos = ray_pos + brick_fNearFar.x*ray_dir;
          float3 local_pos = (hit_pos - brick_min_pos) * (0.5f*sz*header.brick_size);
          float3 voxelPos = floor(clamp(local_pos, 1e-6f, header.brick_size-1e-6f));

          float3 min_pos = brick_min_pos + d*voxelPos;
          float3 max_pos = min_pos + d*float3(1,1,1);
          float3 size = max_pos - min_pos;

          uint s_id = 0;
          while (s_id < surface_count)
          {
            uint offset = m_SdfDAGDataEdges[node.data_edges_offset + s_id].data_offset;
            int4 voxelPosI = int4(int(voxelPos.x),int(voxelPos.y),int(voxelPos.z),1);
            uint rotIdx = m_RotAddTable[curElem.info*ROT_COUNT + m_SdfDAGDataEdges[node.data_edges_offset + s_id].rotation_id];
            float add = m_SdfDAGDataEdges[node.data_edges_offset + s_id].add;
            float vmin = 1.0f;
            float vmax = is_surface ? -1.0f : 1.0f;
            for (int i=0;i<8;i++)
            {
              int4 pI = voxelPosI + int4((i & 4) >> 2,(i & 2) >> 1,i & 1,0);
              uint vId0 = uint32_t(dot(m_SdfCompactOctreeRotModifiers[rotIdx], pI));
              float val = m_SdfDAGDistances[offset + vId0] + add;
              values[i] = val;
              //printf("vId0 = %d, offset = %d add = %f, val = %f\n", vId0, offset, add, values[i]);
              vmin = min(vmin, val);
              vmax = max(vmax, val);
            }

            fNearFar = RayBoxIntersection2(ray_pos, SafeInverse(ray_dir), min_pos, max_pos);
            if (fNearFar.x < fNearFar.y && vmin <= 0.0f && vmax >= 0.0f)    
            {
              float3 start_pos = ray_pos + fNearFar.x * ray_dir;
              float vox_sz = (0.5f * level_sz_f * header.brick_size);
              float3 start_q = vox_sz * (start_pos - min_pos);
              
              // if this node size is smaller than m_SVO_max_level_size, it should be textured 
              // as a grid of voxels, so we change start_q accordingly
              float q_mult = float(ubo[0].m_DAG_max_level_size / level_sz);
              start_q = clamp(start_q, float3(1e-6f), float3(1.0f - 1e-6f));
              start_q = fract(start_q * q_mult);

              hit.t = fNearFar.x;
              hit.primId = node.data_edges_offset;
              hit.geomId = m_SdfDAGBlockIds[node.channels_edge.child_offset];
              
              hit.coords[0] = start_q.x;
              hit.coords[1] = start_q.y;
              hit.coords[2] = start_q.z;
              return hit;
            }
            s_id++;
          }
          brick_fNearFar.x += max(0.0f, fNearFar.y-brick_fNearFar.x) + 1e-5f;
        }
      }
    }
    else if (brick_fNearFar.x < brick_fNearFar.y)
    {
      //traverse children and add to stack (in reverse order)
      int v_sz = header.node_grid_size;
      float t_neg = (brick_fNearFar.y + tNear + 1);
      float3 neg_pos = ray_pos + t_neg * ray_dir;
      float3 neg_dir = -ray_dir;
      float3 neg_dir_inv = -ray_dir_inv;
      float2 neg_brick_fNearFar = float2(t_neg - brick_fNearFar.y,t_neg - brick_fNearFar.x);

      while (neg_brick_fNearFar.x < neg_brick_fNearFar.y)
      {
        d = 2.0f/(level_sz_f*v_sz);
        float3 hit_pos = neg_pos + neg_brick_fNearFar.x*neg_dir;
        float3 local_pos = (hit_pos - brick_min_pos) * (0.5f*level_sz_f*v_sz);
        float3 voxelPos = floor(clamp(local_pos, 1e-6f, v_sz-1e-6f));
        uint voxId = uint32_t(dot(m_SdfCompactOctreeRotModifiers[2*ROT_COUNT + curElem.info], 
                                      int4((int)voxelPos.x,(int)voxelPos.y,(int)voxelPos.z,1)));

        float3 min_pos = brick_min_pos + d*voxelPos;
        float3 max_pos = min_pos + d*float3(1,1,1);
        float3 size = max_pos - min_pos;

        fNearFar = RayBoxIntersection2(neg_pos, neg_dir_inv, min_pos, max_pos);
        SdfDAGChildEdge edge = m_SdfDAGChildEdges[node.children_edges_offset + voxId];

        //if we itersected with this node bbox and this node is valid
        if (tNear < fNearFar.x && edge.child_offset > 0)    
        {
          SdfDAGNode child_node = m_SdfDAGNodes[edge.child_offset];
          top++;
          stack[top].nodeId = edge.child_offset;
          stack[top].info = m_RotAddTable[curElem.info*ROT_COUNT + edge.rotation_id];
          stack[top].p_size = (curElem.p_size*v_sz) + uint2((uint(voxelPos.x) << 16) | uint(voxelPos.y),(uint(voxelPos.z) << 16) | 0);
        }

        neg_brick_fNearFar.x += max(0.0f, fNearFar.y-neg_brick_fNearFar.x) + 0.01f*d;
      }
    }
  }
  return hit;
#endif
}

float4 Resolve(in CRT_Hit hit, float4 posAndNear, float4 dirAndFar) {
  if (hit.primId == INVALID_IDX)
  {
    float4 c = ubo[0].m_backgroundColor;
    c.w = ubo[0].m_AAFrameNum == 0 ? 1.0f : -1.0f;
    return c;
  }

  // if we use AA, we should put depth into alpha channel
  float alpha_depth = ubo[0].m_AAFrameNum == 0 ? 1.0f : hit.t;
  float3 q = float3(hit.coords[0],hit.coords[1],hit.coords[2]);
  float3 norm = float3(0,0,0);
  float3 v = float3(max(q.x, 1 - q.x),max(q.y, 1 - q.y),max(q.z, 1 - q.z));
  int dim = 0;
  if (v.y > v.x && v.y > v.z)
    dim = 1;
  else if (v.z > v.x && v.z > v.y)
    dim = 2;
  norm[dim] = q[dim] < 0.5f ? -1.0f : 1.0f;

  if (ubo[0].m_preset.render_mode == VOXEL_RENDER_MODE_MASK)
  {
    return float4(1,1,1,alpha_depth);
  }
  else if (ubo[0].m_preset.render_mode == VOXEL_RENDER_MODE_NORMAL)
  {
    v = abs(norm);
    return float4(0.25f,0.25f,0.25f,0.5f) + 0.5f*float4(pow(v.x, 1.0f/1.5f),pow(v.y, 1.0f/1.5f),pow(v.z, 1.0f/1.5f),alpha_depth);
  }
  else if (ubo[0].m_preset.render_mode == VOXEL_RENDER_MODE_PRIMITIVE)
  {
    return decode_RGBA8(m_palette[(hit.primId) % palette_size]); 
  }
  else if (ubo[0].m_preset.render_mode == VOXEL_RENDER_MODE_LAMBERT ||
           ubo[0].m_preset.render_mode == VOXEL_RENDER_MODE_LAMBERT_NO_TEX)
  {
    uint side = 2*dim + (q[dim] > 0.5f ? 0 : 1);
    float3 color = float3(1,0,1);
    if (ubo[0].m_preset.render_mode == MULTI_RENDER_MODE_LAMBERT_NO_TEX)
      color = float3(1,1,1);
    else
    {
      float tmp = dim == 2 ? q.x : q.z;
      float2 tc = float2(q[dim] > 0.5f ? tmp : 1.0f-tmp,1.0f-q[(dim+1)%2]);
      uint texId = m_blocks[hit.geomId].texIds[side];
      color = to_float3(m_textures[NonUniformResourceIndex(texId)].SampleLevel(m_textures_sam[NonUniformResourceIndex(texId)], tc, 0));
    }

    float3 final_color = float3(0,0,0);
    for (int i=0;i<ubo[0].m_lights_size;i++)
    {
      if (m_lights[i].type == LIGHT_TYPE_DIRECT)
      {
        float light_q = max(0.0f, dot(norm, m_lights[i].space));
        final_color += m_lights[i].color * color * light_q;
      }
      else if (m_lights[i].type == LIGHT_TYPE_POINT)
      {
        float3 surf_pos = to_float3(posAndNear) + hit.t * to_float3(dirAndFar);
        float3 dir = m_lights[i].space - surf_pos;
        float l = length(dir);
        dir /= l;
        float light_q = max(0.0f, dot(norm, dir));
        final_color += m_lights[i].color * color * light_q / (l*l);
      }
      else
        final_color += m_lights[i].color * color;
    }

    return to_float4(final_color, alpha_depth);
  }

  return float4(1,0,1,alpha_depth);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct KernelArgs
{
  uint sample_id;
  uint count; 
  uint iNumElementsY; 
  uint iNumElementsZ; 
  uint tFlagsMask;    
};



[shader("compute")]
[numthreads(256, 1, 1)]
void main(uint3 a_globalTID: SV_DispatchThreadID, uint3 a_localTID : SV_GroupThreadID, uniform KernelArgs kgenArgs)
{
  bool runThisThread = true;
  const uint tidX = uint(a_globalTID[0]); 
  if(tidX >= kgenArgs.count + 0)
    runThisThread = false;
    // KERNEL BODY:
  //
  if(runThisThread)
  {
  
    const uint XY = m_packedXY[tidX];
    uint x  = (XY & 0x0000FFFF);
    uint y  = (XY & 0xFFFF0000) >> 16;
  
    float4 rayPos, rayDir;
    initEyeRay(x, y, kgenArgs.sample_id, rayPos, rayDir);
    CRT_Hit hit = IntersectDAG(rayPos, rayDir);
    float4 res = Resolve(hit, rayPos, rayDir);
    m_colorBuffer[y*ubo[0].m_width + x] = kgenArgs.sample_id == 0 ? res : m_colorBuffer[y*ubo[0].m_width + x] + res;
  
  }
}

