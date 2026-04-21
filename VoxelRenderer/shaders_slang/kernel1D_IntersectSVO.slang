//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common_slang_slang.h"

[[vk::binding(0, 0)]] RWStructuredBuffer<float4> out_color; // 
[[vk::binding(1, 0)]] RWStructuredBuffer<uint> m_SVO; // 
[[vk::binding(2, 0)]] RWStructuredBuffer<BlockMetadata> m_blocks; // 
[[vk::binding(3, 0)]] RWStructuredBuffer<float4> m_colorBuffer; // 
[[vk::binding(4, 0)]] RWStructuredBuffer<Light> m_lights; // 
[[vk::binding(5, 0)]] RWStructuredBuffer<uint> m_packedXY; // 
[[vk::binding(6, 0)]] Texture2D<float4> m_textures[]; //
[[vk::binding(6, 0)]] SamplerState m_textures_sam[];
[[vk::binding(7, 0)]] StructuredBuffer<VoxelRenderer_slang_UBO_Data> ubo;

float4 Resolve(in CRT_Hit hit, float4 posAndNear, float4 dirAndFar) ;

CRT_Hit IntersectSVO(float4 posAndNear, float4 dirAndFar) ;

void initEyeRay(uint x, uint y, uint s_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) ;



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

CRT_Hit IntersectSVO(float4 posAndNear, float4 dirAndFar) {
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
  const float tNear = max(posAndNear.w, boxNearFar.x);
  const float tFar  = min(dirAndFar.w, boxNearFar.y);
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

  const uint3 nn_indices[8] = {uint3(4,2,1), uint3(5,3,8), uint3(6,8,3), uint3(7,8,8),
                               uint3(8,6,5), uint3(8,7,8), uint3(8,8,7), uint3(8,8,8)};

  OTStackElement stack[32];
  OTStackElement tmp_buf[4];

  int top = 0;
  int buf_top = 0;
  uint3 p;
  float3 p_f, t0, t1, tm;
  int currNode;
  uint32_t nodeId;
  uint32_t level_sz;
  float d, qNear, qFar;
  float2 fNearFar;
  float3 start_q;
  float values[8];

  stack[top].nodeId = 0;
  stack[top].info = 0;
  stack[top].p_size = uint2(0,1);

  while (top >= 0)
  {
    level_sz = stack[top].p_size.y & 0xFFFF;
    p = uint3(stack[top].p_size.x >> 16,stack[top].p_size.x & 0xFFFF,stack[top].p_size.y >> 16);
    d = 1.0f / float(level_sz);
    p_f = float3(p);
    t0 = _t0 + d * p_f * _l;
    t1 = _t0 + d * (p_f + 1) * _l;

    float t_out = min(min(t1.x, t1.y), t1.z);
    if (t_out < tNear)
    {
      top--;
      continue;
    }

    if (stack[top].info > 0)
    {
      float tmin = max(t0.x, max(t0.y, t0.z));
      float tmax = min(t1.x, min(t1.y, t1.z));

      uint p_mask = level_sz - 1;
      uint3 real_p = uint3(((a & 4) > 0) ? (~p.x & p_mask) : p.x,((a & 2) > 0) ? (~p.y & p_mask) : p.y,((a & 1) > 0) ? (~p.z & p_mask) : p.z);

      nodeId = stack[top].nodeId;
      float sz = 0.5f * float(level_sz);
      d = 1.0f / sz;
      float3 min_pos = float3(-1,-1,-1) + d * float3(real_p.x,real_p.y,real_p.z);

      fNearFar = float2(tmin,tmax);
      float3 start_pos = to_float3(posAndNear) + fNearFar.x * to_float3(dirAndFar);
      start_q = sz * (start_pos - min_pos);

      // if this node size is smaller than m_SVO_max_level_size, it should be textured 
      // as a grid of voxels, so we change start_q accordingly
      float q_mult = float(ubo[0].m_SVO_max_level_size / level_sz);
      start_q = clamp(start_q, float3(1e-6f), float3(1.0f - 1e-6f));
      start_q = fract(start_q * q_mult);

      hit.t = fNearFar.x;
      hit.primId = stack[top].nodeId;
      hit.geomId = m_SVO[stack[top].nodeId];
      hit.instId = 0;
      //printf("leaf geomId %d\n", hit.geomId);

      hit.coords[0] = start_q.x;
      hit.coords[1] = start_q.y;
      hit.coords[2] = start_q.z;
      return hit;
    }
    else
    {
      uint node = m_SVO[stack[top].nodeId];
      uint shortPtr = stack[top].nodeId + (node & MAX_CHILD_POINTER);
      uint childrenOff = (node & IS_FAR_BIT) > 0 ? m_SVO[shortPtr] : shortPtr;

      buf_top = 0;
      tm = 0.5f * (t0 + t1);
      currNode = first_node(t0, tm);
      do
      {
        uint childN = currNode ^ a; // child node number, from 0 to 8
        uint childHasData = node & (1u << (24+childN));
        //printf("intersect child %d\n", childN);

        // if child is active
        if (childHasData > 0)
        {
          uint childPos = bitCount((node >> 24) & ((1u << childN) - 1));
          uint childIsLeaf = (node & (1u << (16+childN)));
          //printf("child %d %d (%d)\n", childrenOff, childPos, childIsLeaf);
          tmp_buf[buf_top].nodeId = childrenOff + childPos;
          tmp_buf[buf_top].info = childIsLeaf; // > 0 is child is leaf
          tmp_buf[buf_top].p_size = (stack[top].p_size << 1) | uint2(((currNode & 4) << (16 - 2)) | ((currNode & 2) >> 1),(currNode & 1) << 16);
          buf_top++;
        }
        currNode = new_node(((currNode & 4) > 0) ? t1.x : tm.x, nn_indices[currNode].x,
                            ((currNode & 2) > 0) ? t1.y : tm.y, nn_indices[currNode].y,
                            ((currNode & 1) > 0) ? t1.z : tm.z, nn_indices[currNode].z);
      } while (currNode < 8);

      for (int i = 0; i < buf_top; i++)
      {
        stack[top + i] = tmp_buf[buf_top - i - 1];
      }
      top += buf_top - 1;
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
    CRT_Hit hit = IntersectSVO(rayPos, rayDir);
    float4 res = Resolve(hit, rayPos, rayDir);
    m_colorBuffer[y*ubo[0].m_width + x] = kgenArgs.sample_id == 0 ? res : m_colorBuffer[y*ubo[0].m_width + x] + res;
  
  }
}

