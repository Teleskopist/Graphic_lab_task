//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common_slang_slang.h"

RWStructuredBuffer<float4> out_color; // 
RWStructuredBuffer<float4x4> m_DAGInverseTransforms; // 
RWStructuredBuffer<uint> m_RotAddTable; // 
RWStructuredBuffer<uint> m_SComNodes; // 
RWStructuredBuffer<uint> m_SComValues; // 
RWStructuredBuffer<int4> m_SdfCompactOctreeRotModifiers; // 
RWStructuredBuffer<float4> m_SphericalHarmonics; // 
RWStructuredBuffer<float4> m_colorBuffer; // 
RWStructuredBuffer<uint> m_packedXY; // 
StructuredBuffer<VolumeRenderer_slang_UBO_Data> ubo;

void RaytraceSCom_SS_shadow(float4 posAndNear, float4 dirAndFar, inout float out_T) ;

float3 computeColorFromSH(float3 vox_c, float3 ro, uint idx) ;

void initEyeRay(uint x, uint y, uint s_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) ;

float4 RaymarchSCom(float4 posAndNear, float4 dirAndFar) ;



void RaytraceSCom_SS_shadow(float4 posAndNear, float4 dirAndFar, inout float out_T) {
  //printf("\n");
  out_T = 1.0f;
#ifndef DISABLE_SCOM2
  float2 boxNearFar = RayBoxIntersection2(to_float3(posAndNear), SafeInverse(to_float3(dirAndFar)), float3(-1,-1,-1), float3(1,1,1));
  if (boxNearFar.x > boxNearFar.y) 
    return;

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

    cur.linksOff = ubo[0].m_header.root_node_off + ubo[0].m_header.links_offset + ((node & 0xFF) == SCOM2_CHILD_EMPTY ? 0 : 1+ubo[0].m_header.has_channels);
    cur.info = currNode | (node & 0xFFFFFF00u);
    cur.transform = 0;
    cur.p_size = uint2(0,1);
  }

  stack[0].linksOff = 0xFFFFFFFFu;
  stack[0].info = 0;
  stack[0].transform = 0;
  stack[0].p_size = uint2(0,0);

  float alpha = 1.0f;
  float3 accumulatedColor = float3(0.0f);

  while (top >= 0 && alpha > ubo[0].m_preset.alpha_thr)
  {
    //printf("top: %d %08X\n", top, cur.info);
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
      //printf("nextChildOfCurNode = %d %f %f %f -- %f\n", nextChildOfCurNode, tl.x, tl.y, tl.z, posAndNear.w);
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
      float3 t1 = _t0 + d * (p_f + 1.0f) * _l;

      uint p_mask = level_sz - 1;
      uint3 real_p = uint3(((a & 4) > 0) ? (~p.x & p_mask) : p.x,((a & 2) > 0) ? (~p.y & p_mask) : p.y,((a & 1) > 0) ? (~p.z & p_mask) : p.z);

      float tmin = max(max(posAndNear.w, t0.x), max(t0.y, t0.z));
      float tmax = min(min(dirAndFar.w, t1.x), min(t1.y, t1.z));
      float3 start_pos = to_float3(posAndNear) + tmin * to_float3(dirAndFar);
      float3 end_pos = to_float3(posAndNear) + tmax * to_float3(dirAndFar);
      float3 min_pos = float3(-1,-1,-1) + 2.0f * d * float3(real_p.x,real_p.y,real_p.z);
      float3 start_q = 0.5f * float(level_sz) * (start_pos - min_pos);
      float3 end_q = 0.5f * float(level_sz) * (end_pos - min_pos);

          SdfDAGDataEdge de = unpack_data_edge(ubo[0].m_header, ubo[0].m_header.max_val, m_SComNodes[childLink], m_SComNodes[childLink + ubo[0].m_header.child_offset_off]);
          uint offset = ubo[0].m_header.bricks_arr_offset + ubo[0].m_header.bricks_step * de.data_offset;
          uint rotIdx = m_RotAddTable[cur.transform * ROT_COUNT + de.rotation_id];
          float values[8];
          for (int i = 0; i < 8; i++)
          {
            int4 pI = int4((i & 4) >> 2,(i & 2) >> 1,i & 1,1);
            uint vId0 = uint32_t(dot(m_SdfCompactOctreeRotModifiers[rotIdx], pI));
            uint p_val = m_SComValues[offset + vId0 / ubo[0].m_header.values_per_uint];
            uint p_off = (vId0 % ubo[0].m_header.values_per_uint) * ubo[0].m_header.bits_per_value;
            values[i] = ubo[0].m_header.max_val * (2.0f * ((p_val >> p_off) & ubo[0].m_header.value_mask) / float(ubo[0].m_header.value_mask) - 1) + de.add;
          }

      // Sample and compositing
      float density = -1.0f * eval_dist_trilinear(values, 0.5f * (start_q + end_q));
      float3 sample_color = to_float3(ubo[0].m_baseColor);

      float localAlpha = 1.0f - exp(-density * length(end_pos - start_pos));
      accumulatedColor += alpha * localAlpha * sample_color;
      alpha *= 1.0f - localAlpha;

      uint nextChildOfCurNode = new_node(currNode, tl.x, tl.y, tl.z);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.info = nextChildOfCurNode  | (cur.info & 0xFFFFFF00u);
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

      SdfDAGChildEdge ce = unpack_child_edge(ubo[0].m_header, m_SComNodes[childLink], m_SComNodes[childLink + ubo[0].m_header.child_offset_off]);
      uint node = m_SComNodes[ce.child_offset];
      uint nextNode = first_node(t0, tm);

      cur.linksOff = ce.child_offset + ubo[0].m_header.links_offset + ((node & 0xFF) == SCOM2_CHILD_EMPTY ? 0 : 1+ubo[0].m_header.has_channels);
      cur.transform = m_RotAddTable[cur.transform * ROT_COUNT + ce.rotation_id];
      cur.info = nextNode | (node & 0xFFFFFF00u);
      cur.p_size = (cur.p_size << 1) | uint2(((currNode & 4) << (16 - 2)) | ((currNode & 2) >> 1),(currNode & 1) << 16);
    }
  }

  out_T = alpha;
#endif
}

float3 computeColorFromSH(float3 vox_c, float3 ro, uint idx) {
  if (ubo[0].m_SphericalHarmonics_size == 0) return float3(1,0,1);

  const float3 dir = normalize(vox_c - ro);
  float4 result = 0.28209479177387814f * m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 0];

  float x = dir.x;
  float y = dir.y;
  float z = dir.z;
  result = result + 0.4886025119029199f * (-y * m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 1]
                                           +z * m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 2] 
                                           -x * m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 3]);

  // printf("v = %f %f %f, q= %f %f %f\n", x, y, z, 
  //   m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 1].x, 
  //   m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 2].y, 
  //   m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 3].z);

  float xx = x * x, yy = y * y, zz = z * z;
  float xy = x * y, yz = y * z, xz = x * z;
  result = result +
           (1.0925484305920792f)  * xy *                    m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 4] +
           (-1.0925484305920792f) * yz *                    m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 5] +
           (0.31539156525252005f) * (2.0f * zz - xx - yy) * m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 6] +
           (-1.0925484305920792f) * xz *                    m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 7] +
           0.5462742152960396f    * (xx - yy) *             m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 8];

  result = result +
           (-0.5900435899266435f) * y * (3.0f * xx - yy) *                    m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 9] +
           (2.890611442640554f)   * xy * z *                                  m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 10] +
           (-0.4570457994644658f) * y * (4.0f * zz - xx - yy) *               m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 11] +
           (0.3731763325901154f)  * z * (2.0f * zz - 3.0f * xx - 3.0f * yy) * m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 12] +
           (-0.4570457994644658f) * x * (4.0f * zz - xx - yy) *               m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 13] +
           (1.445305721320277f)   * z * (xx - yy) *                           m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 14] +
           (-0.5900435899266435f) * x * (xx - 3.0f * yy) *                    m_SphericalHarmonics[idx * SH_COMPONENT_COUNT + 15];
  //printf("result = %f, %f, %f\n", result.x, result.y, result.z);
  // RGB colors is shifted and clamped to obtain non-negative values.
  result = max(result + 0.5f, float4(0.0f));
  //printf("result = %f, %f, %f\n", result.x, result.y, result.z);
  return to_float3(result);
}

void initEyeRay(uint x, uint y, uint s_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) {
  uint spp_sqrt = uint32_t(sqrt(ubo[0].m_preset.spp));
  float i_spp_sqrt = 1.0f/spp_sqrt;
  float2 d = (ubo[0].m_AAFrameNum >= 2) ? rand2(ubo[0].m_seed, x, y, s_id + ubo[0].m_AAFrameNum + ubo[0].m_seed % 2) : 
                                   i_spp_sqrt*float2(s_id/spp_sqrt+0.5,s_id%spp_sqrt+0.5);

  float3 rayDir = EyeRayDirNormalized((float(x)+d.x)/float(ubo[0].m_width), (float(y)+d.y)/float(ubo[0].m_height), ubo[0].m_projInv);
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

float4 RaymarchSCom(float4 posAndNear, float4 dirAndFar) {
  float3 m_LightDir = normalize(float3(1,1,1));
  float3 m_LightColor = float3(1,1,1);

  if (ubo[0].m_preset.volume_type == VOLUME_TYPE_SVRASTER_RF)
  {
    float3 scene_center;
    scene_center.x = ubo[0].m_header.user_params[SCENE_CENTER_X_USER_PARAM_ID];
    scene_center.y = ubo[0].m_header.user_params[SCENE_CENTER_Y_USER_PARAM_ID];
    scene_center.z = ubo[0].m_header.user_params[SCENE_CENTER_Z_USER_PARAM_ID];
    posAndNear.x = (posAndNear.x - scene_center.x) * 2.0f / ubo[0].m_header.user_params[SCENE_EXTENT_USER_PARAM_ID];
    posAndNear.y = (posAndNear.y - scene_center.y) * 2.0f / ubo[0].m_header.user_params[SCENE_EXTENT_USER_PARAM_ID];
    posAndNear.z = (posAndNear.z - scene_center.z) * 2.0f / ubo[0].m_header.user_params[SCENE_EXTENT_USER_PARAM_ID];
  }
#ifndef DISABLE_SCOM2
  float2 boxNearFar = RayBoxIntersection2(to_float3(posAndNear), SafeInverse(to_float3(dirAndFar)), float3(-1,-1,-1), float3(1,1,1));
  if (boxNearFar.x > boxNearFar.y) 
    return ubo[0].m_backgroundColor;

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

    cur.linksOff = ubo[0].m_header.root_node_off + ubo[0].m_header.links_offset + ((node & 0xFF) == SCOM2_CHILD_EMPTY ? 0 : 1+ubo[0].m_header.has_channels);
    cur.info = currNode | (node & 0xFFFFFF00u);
    cur.transform = 0;
    cur.p_size = uint2(0,1);
  }

  stack[0].linksOff = 0xFFFFFFFFu;
  stack[0].info = 0;
  stack[0].transform = 0;
  stack[0].p_size = uint2(0,0);

  float alpha = 1.0f;
  float3 accumulatedColor = float3(0.0f);

  while (top >= 0 && alpha > ubo[0].m_preset.alpha_thr)
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
      float3 t1 = _t0 + d * (p_f + 1.0f) * _l;

      uint p_mask = level_sz - 1;
      uint3 real_p = uint3(((a & 4) > 0) ? (~p.x & p_mask) : p.x,((a & 2) > 0) ? (~p.y & p_mask) : p.y,((a & 1) > 0) ? (~p.z & p_mask) : p.z);

      float tmin = max(max(posAndNear.w, t0.x), max(t0.y, t0.z));
      float tmax = min(min(dirAndFar.w, t1.x), min(t1.y, t1.z));
      float3 start_pos = to_float3(posAndNear) + tmin * to_float3(dirAndFar);
      float3 end_pos = to_float3(posAndNear) + tmax * to_float3(dirAndFar);
      float3 min_pos = float3(-1,-1,-1) + 2.0f * d * float3(real_p.x,real_p.y,real_p.z);
      float3 start_q = 0.5f * float(level_sz) * (start_pos - min_pos);
      float3 end_q = 0.5f * float(level_sz) * (end_pos - min_pos);
      //printf("start q %f %f %f, end q %f %f %f\n", start_q.x, start_q.y, start_q.z, end_q.x, end_q.y, end_q.z);

          SdfDAGDataEdge de = unpack_data_edge(ubo[0].m_header, ubo[0].m_header.max_val, m_SComNodes[childLink], m_SComNodes[childLink + ubo[0].m_header.child_offset_off]);
          uint offset = ubo[0].m_header.bricks_arr_offset + ubo[0].m_header.bricks_step * de.data_offset;
          uint rotIdx = m_RotAddTable[cur.transform * ROT_COUNT + de.rotation_id];
          float values[8];
          for (int i = 0; i < 8; i++)
          {
            int4 pI = int4((i & 4) >> 2,(i & 2) >> 1,i & 1,1);
            uint vId0 = uint32_t(dot(m_SdfCompactOctreeRotModifiers[rotIdx], pI));
            uint p_val = m_SComValues[offset + vId0 / ubo[0].m_header.values_per_uint];
            uint p_off = (vId0 % ubo[0].m_header.values_per_uint) * ubo[0].m_header.bits_per_value;
            values[i] = ubo[0].m_header.max_val * (2.0f * ((p_val >> p_off) & ubo[0].m_header.value_mask) / float(ubo[0].m_header.value_mask) - 1) + de.add;
          }

      // Sample and compositing
      float density = -1.0f * eval_dist_trilinear(values, 0.5f * (start_q + end_q));
      float3 sample_color = to_float3(ubo[0].m_baseColor);
      if (ubo[0].m_preset.volume_type == VOLUME_TYPE_SVRASTER_RF)
      {
        density = 100.0f * ubo[0].m_header.user_params[SCENE_EXTENT_USER_PARAM_ID] * exp_linear_11(density);
        //channel data is stored after bodes/surfaces list and only for leaves
        // the second bitcount extracts number of leaves before the current child to find proper child offset
        uint channelLink = cur.linksOff + bitCount((cur.info >> 24) & 0xFF) + bitCount((cur.info >> 8) & (0x5555 >> (16-2*childN)));
        SdfDAGChildEdge ce = unpack_child_edge(ubo[0].m_header, m_SComNodes[channelLink], m_SComNodes[channelLink + ubo[0].m_header.child_offset_off]);
        float4 v = to_float4(to_float3(posAndNear)  - 0.5f*(start_pos + end_pos), 0.0f);
        uint chRotIdx = m_RotAddTable[cur.transform*ROT_COUNT + ce.rotation_id];
        v = m_DAGInverseTransforms[chRotIdx] * v;

        sample_color = computeColorFromSH(float3(0,0,0), to_float3(v), ce.child_offset);
      }

      float step = length(end_pos - start_pos);
      float sampleAlpha = exp(-1.0f*density * step);
      
      float lightAtten = 1.0f;
      if (ubo[0].m_preset.use_lighting == 1 && density > 3.0f*ubo[0].m_preset.alpha_thr)
      {
        float3 pos = 0.5f * (start_pos + end_pos);
        RaytraceSCom_SS_shadow(to_float4(pos, 0.0001f), to_float4(m_LightDir, 1000.0f), lightAtten);
      }

      accumulatedColor += alpha * (1.0f - sampleAlpha) * lightAtten * m_LightColor * sample_color;
      alpha *= sampleAlpha;

      uint nextChildOfCurNode = new_node(currNode, tl.x, tl.y, tl.z);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.info = nextChildOfCurNode  | (cur.info & 0xFFFFFF00u);
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

      SdfDAGChildEdge ce = unpack_child_edge(ubo[0].m_header, m_SComNodes[childLink], m_SComNodes[childLink + ubo[0].m_header.child_offset_off]);
      uint node = m_SComNodes[ce.child_offset];
      uint nextNode = first_node(t0, tm);

      cur.linksOff = ce.child_offset + ubo[0].m_header.links_offset + ((node & 0xFF) == SCOM2_CHILD_EMPTY ? 0 : 1+ubo[0].m_header.has_channels);
      cur.transform = m_RotAddTable[cur.transform * ROT_COUNT + ce.rotation_id];
      cur.info = nextNode | (node & 0xFFFFFF00u);
      cur.p_size = (cur.p_size << 1) | uint2(((currNode & 4) << (16 - 2)) | ((currNode & 2) >> 1),(currNode & 1) << 16);
    }
  }

  return to_float4(alpha * to_float3(ubo[0].m_backgroundColor) + accumulatedColor, 1);
#else
  return float4(1,0,0,1);
#endif
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
    float4 res = RaymarchSCom(rayPos, rayDir);
    m_colorBuffer[y*ubo[0].m_width + x] = kgenArgs.sample_id == 0 ? res : m_colorBuffer[y*ubo[0].m_width + x] + res;
  
  }
}

