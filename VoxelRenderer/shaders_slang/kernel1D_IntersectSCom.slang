//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common_slang_slang.h"

[[vk::binding(0, 0)]] RWStructuredBuffer<float4> out_color; // 
[[vk::binding(1, 0)]] RWStructuredBuffer<uint> m_RotAddTable; // 
[[vk::binding(2, 0)]] RWStructuredBuffer<uint> m_SComNodes; // 
[[vk::binding(3, 0)]] RWStructuredBuffer<int4> m_SdfCompactOctreeRotModifiers; // 
[[vk::binding(4, 0)]] RWStructuredBuffer<BlockMetadata> m_blocks; // 
[[vk::binding(5, 0)]] RWStructuredBuffer<float4> m_colorBuffer; // 
[[vk::binding(6, 0)]] RWStructuredBuffer<Light> m_lights; // 
[[vk::binding(7, 0)]] RWStructuredBuffer<uint> m_packedXY; // 
[[vk::binding(8, 0)]] Texture2D<float4> m_textures[]; //
[[vk::binding(8, 0)]] SamplerState m_textures_sam[];
[[vk::binding(9, 0)]] StructuredBuffer<VoxelRenderer_slang_UBO_Data> ubo;

CRT_Hit IntersectSCom(float4 posAndNear, float4 dirAndFar) ;

void initEyeRay(uint x, uint y, uint s_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) ;

float4 Resolve(in CRT_Hit hit, float4 posAndNear, float4 dirAndFar) ;



CRT_Hit IntersectSCom(float4 posAndNear, float4 dirAndFar) {
  CRT_Hit hit;
  hit.geomId = INVALID_IDX;
  hit.instId = INVALID_IDX;
  hit.primId = INVALID_IDX;
  hit.t = dirAndFar.w;
  hit.coords[0] = 1;
  hit.coords[1] = 0;
  hit.coords[2] = 0;
  hit.coords[3] = 0;

  float2 boxNearFar = RayBoxIntersection2(to_float3(posAndNear), SafeInverse(to_float3(dirAndFar)), float3(-1,-1,-1), float3(1,1,1));
  if (boxNearFar.x > boxNearFar.y) 
    return hit;

  float3 pos_ray_pos = to_float3(posAndNear);
  float3 pos_ray_dir = to_float3(dirAndFar);

  //assume octree is box [-1,1]^3
  uint a = 0;
  if (dirAndFar.x < 0)
  {
    pos_ray_pos.x *= -1;
    pos_ray_dir.x *= -1;
    a |= 4;
  }

  if (dirAndFar.y < 0)
  {
    pos_ray_pos.y *= -1;
    pos_ray_dir.y *= -1;
    a |= 2;
  }

  if (dirAndFar.z < 0)
  {
    pos_ray_pos.z *= -1;
    pos_ray_dir.z *= -1;
    a |= 1;
  }

  pos_ray_pos *= -1;
  const float3 pos_ray_dir_inv = SafeInverse(pos_ray_dir);
  const float3 _t0 = pos_ray_pos * pos_ray_dir_inv - pos_ray_dir_inv;
  const float3 _t1 = pos_ray_pos * pos_ray_dir_inv + pos_ray_dir_inv;
  const float3 _l = _t1 - _t0;

  ExtStackElement stack[16];

  int top = 0;
  float d = 1.0f;
  float3 t0 = _t0;
  float3 tm = t0 + 0.5f*_l;

  ExtStackElement cur;
  {
    uint node = m_SComNodes[ubo[0].m_header.root_node_off];
    uint currNode = first_node(t0, tm);

    cur.linksOff = ubo[0].m_header.root_node_off + ubo[0].m_header.links_offset + ((node & 0xFF) == SCOM2_CHILD_EMPTY ? 0 : 1);
    cur.info = currNode | (node & 0xFFFFFF00u);
    cur.transform = 0;
    cur.p_size = uint2(0,1);
  }

  stack[0].linksOff = 0xFFFFFFFFu;
  stack[0].info = 0;
  stack[0].transform = 0;
  stack[0].p_size = uint2(0,0);

  while (top >= 0)
  {
    uint currNode = cur.info & 0x7;
    uint rawChildNode = currNode ^ a; 
    uint3 voxelPos  = uint3((rawChildNode & 4) >> 2,(rawChildNode & 2) >> 1,rawChildNode & 1);
    uint childN = uint32_t(dot(m_SdfCompactOctreeRotModifiers[2 * ROT_COUNT + cur.transform], // child node number, from 0 to 8
                                   int4((int)voxelPos.x,(int)voxelPos.y,(int)voxelPos.z,1)));
    uint childLink = cur.linksOff + bitCount((cur.info >> 24) & ((1u << childN) - 1));
    uint childHasData = cur.info & (1u << (24+childN));
    uint childIsLeaf = cur.info & (1u << (8+2*childN));

    d = 1.0f / float(cur.p_size.y & 0xFFFF);
    float3 pf2 = float3(float(cur.p_size.x >> 16) + (((currNode & 4) > 0) ? 1.0f : 0.5f),float(cur.p_size.x & 0xFFFF) + (((currNode & 2) > 0) ? 1.0f : 0.5f),float(cur.p_size.y >> 16) + (((currNode & 1) > 0) ? 1.0f : 0.5f));
    float3 tl = _t0 + d * pf2 * _l;

    //this child is empty, go to the next child of this node
    if (min(min(tl.x, tl.y), tl.z) < posAndNear.w || childHasData == 0)
    {
      uint nextChildOfCurNode = new_node(currNode, tl.x, tl.y, tl.z);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.info = nextChildOfCurNode  | (cur.info & 0xFFFFFF00u);
    }
    else if (childIsLeaf > 0) // we intersected with leaf, fill the hit data and return
    {
      uint level_sz = 2*(cur.p_size.y & 0xFFFF);
      uint3 p = 2*uint3(cur.p_size.x >> 16,cur.p_size.x & 0xFFFF,cur.p_size.y >> 16) + 
            uint3((currNode & 4) >> 2,(currNode & 2) >> 1,currNode & 1);
      d = 1.0f / float(level_sz);
      float3 p_f = float3(p);
      t0 = _t0 + d * p_f * _l;

      uint p_mask = level_sz - 1;
      uint3 real_p = uint3(((a & 4) > 0) ? (~p.x & p_mask) : p.x,((a & 2) > 0) ? (~p.y & p_mask) : p.y,((a & 1) > 0) ? (~p.z & p_mask) : p.z);

      float tmin = max(t0.x, max(t0.y, t0.z));
      float3 start_pos = to_float3(posAndNear) + tmin * to_float3(dirAndFar);
      float3 min_pos = float3(-1,-1,-1) + 2.0f * d * float3(real_p.x,real_p.y,real_p.z);
      float3 start_q = 0.5f * float(level_sz) * (start_pos - min_pos);

      // if this node size is smaller than m_DAG_max_level_size, it should be textured 
      // as a grid of voxels, so we change start_q accordingly
      float q_mult = float(ubo[0].m_DAG_max_level_size / level_sz);
      start_q = fract(q_mult * clamp(start_q, float3(1e-6f), float3(1.0f - 1e-6f)));

      hit.t = tmin;
      hit.primId = childLink;
      hit.geomId = m_SComNodes[childLink] & ubo[0].m_header.node_offset_mask;
      hit.instId = 0;
      hit.coords[0] = start_q.x;
      hit.coords[1] = start_q.y;
      hit.coords[2] = start_q.z;

      return hit;
    }
    else // this child is a node, move to it
    {
      uint nextChildOfCurNode = new_node(currNode, tl.x, tl.y, tl.z);

      if (nextChildOfCurNode < 8)  // if it is not a last child, leave the current node and place the child in the stack
      {
        cur.info = nextChildOfCurNode | (cur.info & 0xFFFFFF00u);
        stack[++top] = cur;
      }

      d = 0.5f / float(cur.p_size.y & 0xFFFF);
      float3 p_f = float3(2*(cur.p_size.x >> 16) + ((currNode & 4) >> 2),2*(cur.p_size.x & 0xFFFF) + ((currNode & 2) >> 1),2*(cur.p_size.y >> 16) + (currNode & 1));
      t0 = _t0 + d * p_f * _l;
      tm = t0 + 0.5f*d*_l;

      SdfDAGChildEdge ce = unpack_child_edge(ubo[0].m_header, m_SComNodes[childLink], m_SComNodes[childLink]);
      uint node = m_SComNodes[ce.child_offset];
      uint nextNode = first_node(t0, tm);

      cur.linksOff = ce.child_offset + ubo[0].m_header.links_offset + ((node & 0xFF) == SCOM2_CHILD_EMPTY ? 0 : 1);
      cur.transform = m_RotAddTable[cur.transform * ROT_COUNT + ce.rotation_id];
      cur.info = nextNode | (node & 0xFFFFFF00u);
      cur.p_size = (cur.p_size << 1) | uint2(((currNode & 4) << (16 - 2)) | ((currNode & 2) >> 1),(currNode & 1) << 16);
    }
  }

  return hit;
}

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
    CRT_Hit hit = IntersectSCom(rayPos, rayDir);
    float4 res = Resolve(hit, rayPos, rayDir);
    m_colorBuffer[y*ubo[0].m_width + x] = kgenArgs.sample_id == 0 ? res : m_colorBuffer[y*ubo[0].m_width + x] + res;
  
  }
}

