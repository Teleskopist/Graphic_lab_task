//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common_slang_slang.h"

RWStructuredBuffer<float4> out_color; // 
RWStructuredBuffer<uint> m_RotAddTable; // 
RWStructuredBuffer<uint> m_SComNodes; // 
RWStructuredBuffer<uint> m_SComValues; // 
RWStructuredBuffer<uint> m_SdfDAGTranspositions; // 
RWStructuredBuffer<float4> m_colorBuffer; // 
RWStructuredBuffer<uint> m_packedXY; // 
RWStructuredBuffer<float> m_timestamps; // 
StructuredBuffer<VolumeRenderer_slang_UBO_Data> ubo;

void RaytraceSCom4D_SS_shadow(float4 posAndNear, float4 dirAndFar, inout float out_T) ;

void initEyeRay(uint x, uint y, uint s_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) ;

float4 RaymarchSCom4D(float4 posAndNear, float4 dirAndFar) ;



void RaytraceSCom4D_SS_shadow(float4 posAndNear, float4 dirAndFar, inout float out_T) {
  out_T = 1.0f;
#ifndef DISABLE_SCOM2
  float2 boxNearFar = RayBoxIntersection2(to_float3(posAndNear), SafeInverse(to_float3(dirAndFar)), float3(-1,-1,-1), float3(1,1,1));
  if (boxNearFar.x > boxNearFar.y) 
    return;

  float3 pos_ray_pos = to_float3(posAndNear);
  float3 pos_ray_dir = to_float3(dirAndFar);
  const float time = (float(ubo[0].frame0) + ubo[0].m_dframe) / float(ubo[0].m_timestamps_size - 1);

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

  ExtStackElement4D stack[16];

  int top = 0;
  float d = 1.0f;
  float3 t0 = _t0;
  float3 tm = t0 + 0.5f*_l;

  ExtStackElement4D cur;
  {
    NodeHeadUnpacked node = unpack_node_head(ubo[0].m_header, m_SComNodes[ubo[0].m_header.root_node_off], m_SComNodes[ubo[0].m_header.root_node_off+ubo[0].m_header.links_offset-1]);
    uint currNode = first_node(t0, tm);

    cur.linksOff = ubo[0].m_header.root_node_off + ubo[0].m_header.links_offset + 
                   (node.base_type == SCOM2_CHILD_EMPTY ? 0 : ubo[0].m_header.uints_per_link*(1+ubo[0].m_header.has_channels));
    cur.currNode = currNode | (time < 0.5f ? 0 : 8);
    cur.chActive = node.children_active;
    cur.chTypes  = node.children_types;
    cur.transform = 0;
    cur.p = uint4(0,0,0,0);
    cur.size = 1;
  }

  stack[0].linksOff = 0xFFFFFFFFu;
  stack[0].currNode = 0;
  stack[0].chActive = 0;
  stack[0].chTypes = 0;
  stack[0].p = uint4(0,0,0,0);
  stack[0].size = 0;

  float alpha = 1.0f;
  float3 accumulatedColor = float3(0.0f);

  while (top >= 0 && alpha > ubo[0].m_preset.alpha_thr)
  {
    uint currNode = cur.currNode & 0xF;
    uint rawChildNode = currNode ^ a; 
    uint childN = m_SdfDAGTranspositions[ubo[0].m_SdfDAGBrickTranspositionOffset + cur.transform*16 + rawChildNode];
    uint childLink = cur.linksOff + bitCount(cur.chActive & ((1u << childN) - 1))*ubo[0].m_header.uints_per_link;
    uint childHasData = cur.chActive & (1u << (  childN));
    uint childIsLeaf = cur.chTypes  & (1u << (2*childN));

    d = 1.0f / float(cur.size);
    float3 pf2 = float3(float(cur.p.x) + (((currNode & 4) > 0) ? 1.0f : 0.5f),float(cur.p.y) + (((currNode & 2) > 0) ? 1.0f : 0.5f),float(cur.p.z) + (((currNode & 1) > 0) ? 1.0f : 0.5f));
    float3 tl = _t0 + d * pf2 * _l;

    float time_min = float(cur.p.w) / float(cur.size);
    float time_max = time_min + 1.0f/float(cur.size);
    float time_size = time_max - time_min;
    float voxel_w0_f = (time - time_min) / time_size;

    //this child is empty, go to the next child of this node
    if (min(min(tl.x, tl.y), tl.z) < posAndNear.w || childHasData == 0)
    {
      uint nextChildOfCurNode = new_node(currNode & 0x7, tl.x, tl.y, tl.z);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.currNode = nextChildOfCurNode | (voxel_w0_f < 0.5f ? 0 : 8);
    }
    else if (childIsLeaf > 0) // we intersected with leaf, fill the hit data and return
    {
      uint level_sz = 2 * (cur.size);
      uint3 p = 2 * uint3(cur.p.x,cur.p.y,cur.p.z) + uint3((currNode & 4) >> 2,(currNode & 2) >> 1,currNode & 1);
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
      // printf("start q %f %f %f, end q %f %f %f\n", start_q.x, start_q.y, start_q.z, end_q.x, end_q.y, end_q.z);

      SdfDAGDataEdge de = unpack_data_edge(ubo[0].m_header, ubo[0].m_header.max_val, m_SComNodes[childLink], m_SComNodes[childLink + ubo[0].m_header.child_offset_off]);
      uint offset = ubo[0].m_header.bricks_arr_offset + ubo[0].m_header.bricks_step * de.data_offset;
      uint rotIdx = m_RotAddTable[cur.transform * ubo[0].m_SdfDAGTransformCount + de.rotation_id];
      float values0[8];
      float values1[8];
      for (int i = 0; i < 16; i++)
      {
        int4 pI = int4((i & 4) >> 2,(i & 2) >> 1,i & 1,(i & 8) >> 3);
        int idx = pI.w * 8 + pI.x * 4 + pI.y * 2 + pI.z;
        uint vId0 = m_SdfDAGTranspositions[rotIdx * 16 + idx];
        uint p_val = m_SComValues[offset + vId0 / ubo[0].m_header.values_per_uint];
        uint p_off = (vId0 % ubo[0].m_header.values_per_uint) * ubo[0].m_header.bits_per_value;
        float val = ubo[0].m_header.max_val * (2.0f * ((p_val >> p_off) & ubo[0].m_header.value_mask) / float(ubo[0].m_header.value_mask) - 1) + de.add;
        if (i < 8)
          values0[i] = val;
        else
          values1[i - 8] = val;
      }

      float voxel_dw = 2.0f*voxel_w0_f - float(uint32_t(2.0f*voxel_w0_f));

      // Sample and compositing
      float3 sample_color = float3(ubo[0].m_baseColor.x,ubo[0].m_baseColor.y,ubo[0].m_baseColor.z);
      float s0 = eval_dist_trilinear(values0, 0.5f * (start_q + end_q));
      float s1 = eval_dist_trilinear(values1, 0.5f * (start_q + end_q));
      float density = -1.0f * ((1 - voxel_dw) * s0 + voxel_dw * s1);
      float step = length(end_pos - start_pos);
      float sampleAlpha = exp(-1.0f*density * step);

      accumulatedColor += alpha * (1.0f - sampleAlpha) * sample_color;
      alpha *= sampleAlpha;

      uint nextChildOfCurNode = new_node(currNode & 0x7, tl.x, tl.y, tl.z);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.currNode = nextChildOfCurNode | (voxel_w0_f < 0.5f ? 0 : 8);
    }
    else // this child is a node, move to it
    {
      //printf("3\n");
      uint nextChildOfCurNode = new_node(currNode & 0x7, tl.x, tl.y, tl.z);

      if (nextChildOfCurNode < 8)  // if it is not a last child, leave the current node and place the child in the stack
      {
        cur.currNode = nextChildOfCurNode | (voxel_w0_f < 0.5f ? 0 : 8);
        stack[++top] = cur;
      }

      d = 0.5f / float(cur.size);
      float3 p_f = float3(2*(cur.p.x) + ((currNode & 4) >> 2),2*(cur.p.y) + ((currNode & 2) >> 1),2*(cur.p.z) + (currNode & 1));
      t0 = _t0 + d * p_f * _l;
      tm = t0 + 0.5f*d*_l;
      float voxel_dw = 2.0f*voxel_w0_f - float(uint32_t(2.0f*voxel_w0_f));

      SdfDAGChildEdge ce = unpack_child_edge(ubo[0].m_header, m_SComNodes[childLink], m_SComNodes[childLink + ubo[0].m_header.child_offset_off]);
      NodeHeadUnpacked node = unpack_node_head(ubo[0].m_header, m_SComNodes[ce.child_offset], m_SComNodes[ce.child_offset+ubo[0].m_header.links_offset-1]);
      uint nextNode = first_node(t0, tm);

      cur.linksOff = ce.child_offset + ubo[0].m_header.links_offset + (node.base_links_end == SCOM2_CHILD_EMPTY ? 0 : ubo[0].m_header.uints_per_link*(1+ubo[0].m_header.has_channels));
      cur.transform = m_RotAddTable[cur.transform * ubo[0].m_SdfDAGTransformCount + ce.rotation_id];
      cur.currNode = nextNode | (voxel_dw < 0.5f ? 0 : 8);
      cur.chActive = node.children_active;
      cur.chTypes = node.children_types;
      cur.size = cur.size << 1;
      cur.p = (cur.p << 1) | uint4((currNode & 4) >> 2,(currNode & 2) >> 1,currNode & 1,(currNode & 8) >> 3);
    }
  }

  out_T = alpha;
#endif  
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

float4 RaymarchSCom4D(float4 posAndNear, float4 dirAndFar) {
  float3 m_LightDir = normalize(float3(1,1,1));
  float3 m_LightColor = float3(1,1,1);

#ifndef DISABLE_SCOM2
  float2 boxNearFar = RayBoxIntersection2(to_float3(posAndNear), SafeInverse(to_float3(dirAndFar)), float3(-1,-1,-1), float3(1,1,1));
  if (boxNearFar.x > boxNearFar.y) 
    return ubo[0].m_backgroundColor;

  float3 pos_ray_pos = to_float3(posAndNear);
  float3 pos_ray_dir = to_float3(dirAndFar);
  const float time = (float(ubo[0].frame0) + ubo[0].m_dframe) / float(ubo[0].m_timestamps_size - 1);

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

  ExtStackElement4D stack[16];

  int top = 0;
  float d = 1.0f;
  float3 t0 = _t0;
  float3 tm = t0 + 0.5f*_l;

  ExtStackElement4D cur;
  {
    NodeHeadUnpacked node = unpack_node_head(ubo[0].m_header, m_SComNodes[ubo[0].m_header.root_node_off], m_SComNodes[ubo[0].m_header.root_node_off+ubo[0].m_header.links_offset-1]);
    uint currNode = first_node(t0, tm);

    cur.linksOff = ubo[0].m_header.root_node_off + ubo[0].m_header.links_offset + 
                   (node.base_type == SCOM2_CHILD_EMPTY ? 0 : ubo[0].m_header.uints_per_link*(1+ubo[0].m_header.has_channels));
    cur.currNode = currNode | (time < 0.5f ? 0 : 8);
    cur.chActive = node.children_active;
    cur.chTypes  = node.children_types;
    cur.transform = 0;
    cur.p = uint4(0,0,0,0);
    cur.size = 1;
  }

  stack[0].linksOff = 0xFFFFFFFFu;
  stack[0].currNode = 0;
  stack[0].chActive = 0;
  stack[0].chTypes = 0;
  stack[0].p = uint4(0,0,0,0);
  stack[0].size = 0;

  float alpha = 1.0f;
  float3 accumulatedColor = float3(0.0f);

  while (top >= 0 && alpha > ubo[0].m_preset.alpha_thr)
  {
    uint currNode = cur.currNode & 0xF;
    uint rawChildNode = currNode ^ a; 
    //uint3 voxelPos  = uint3((rawChildNode & 4) >> 2, (rawChildNode & 2) >> 1, rawChildNode & 1);
    //uint32_t childN = uint32_t(dot(m_SdfCompactOctreeRotModifiers[2 * ROT_COUNT + cur.transform], // child node number, from 0 to 8
    //                               int4((int)voxelPos.x, (int)voxelPos.y, (int)voxelPos.z, 1)));
    uint childN = m_SdfDAGTranspositions[ubo[0].m_SdfDAGBrickTranspositionOffset + cur.transform*16 + rawChildNode];
    uint childLink = cur.linksOff + bitCount(cur.chActive & ((1u << childN) - 1))*ubo[0].m_header.uints_per_link;
    uint childHasData = cur.chActive & (1u << (  childN));
    uint childIsLeaf = cur.chTypes  & (1u << (2*childN));

    //printf("intersect %d (%d) linksOff = %d, chActive = %x, chTypes = %x\n", top, currNode, cur.linksOff, cur.chActive, cur.chTypes);

    d = 1.0f / float(cur.size);
    float3 pf2 = float3(float(cur.p.x) + (((currNode & 4) > 0) ? 1.0f : 0.5f),float(cur.p.y) + (((currNode & 2) > 0) ? 1.0f : 0.5f),float(cur.p.z) + (((currNode & 1) > 0) ? 1.0f : 0.5f));
    float3 tl = _t0 + d * pf2 * _l;

    float time_min = float(cur.p.w) / float(cur.size);
    float time_max = time_min + 1.0f/float(cur.size);
    float time_size = time_max - time_min;
    float voxel_w0_f = (time - time_min) / time_size;

    //printf("top = %d curr node = %d\n",top, currNode);

    //this child is empty, go to the next child of this node
    if (min(min(tl.x, tl.y), tl.z) < posAndNear.w || childHasData == 0)
    {
      uint nextChildOfCurNode = new_node(currNode & 0x7, tl.x, tl.y, tl.z);
      //printf("1 %d %d\n", currNode, nextChildOfCurNode);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.currNode = nextChildOfCurNode | (voxel_w0_f < 0.5f ? 0 : 8);
      //printf("A %d\n", cur.currNode);
    }
    else if (childIsLeaf > 0) // we intersected with leaf, fill the hit data and return
    {
      //printf("2\n");
      uint level_sz = 2 * (cur.size);
      uint3 p = 2 * uint3(cur.p.x,cur.p.y,cur.p.z) + uint3((currNode & 4) >> 2,(currNode & 2) >> 1,currNode & 1);
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
      // printf("start q %f %f %f, end q %f %f %f\n", start_q.x, start_q.y, start_q.z, end_q.x, end_q.y, end_q.z);

      SdfDAGDataEdge de = unpack_data_edge(ubo[0].m_header, ubo[0].m_header.max_val, m_SComNodes[childLink], m_SComNodes[childLink + ubo[0].m_header.child_offset_off]);
      uint offset = ubo[0].m_header.bricks_arr_offset + ubo[0].m_header.bricks_step * de.data_offset;
      uint rotIdx = m_RotAddTable[cur.transform * ubo[0].m_SdfDAGTransformCount + de.rotation_id];
      float values0[8];
      float values1[8];
      for (int i = 0; i < 16; i++)
      {
        int4 pI = int4((i & 4) >> 2,(i & 2) >> 1,i & 1,(i & 8) >> 3);
        int idx = pI.w * 8 + pI.x * 4 + pI.y * 2 + pI.z;
        uint vId0 = m_SdfDAGTranspositions[rotIdx * 16 + idx];
        uint p_val = m_SComValues[offset + vId0 / ubo[0].m_header.values_per_uint];
        uint p_off = (vId0 % ubo[0].m_header.values_per_uint) * ubo[0].m_header.bits_per_value;
        float val = ubo[0].m_header.max_val * (2.0f * ((p_val >> p_off) & ubo[0].m_header.value_mask) / float(ubo[0].m_header.value_mask) - 1) + de.add;
        if (i < 8)
          values0[i] = val;
        else
          values1[i - 8] = val;
        //printf("vId0 = %d, offset = %d add = %f, val = %f\n", vId0, offset, de.add, val);
      }

      float voxel_dw = 2.0f*voxel_w0_f - float(uint32_t(2.0f*voxel_w0_f));

      // Sample and compositing
      float3 sample_color = float3(ubo[0].m_baseColor.x,ubo[0].m_baseColor.y,ubo[0].m_baseColor.z);
      float s0 = eval_dist_trilinear(values0, 0.5f * (start_q + end_q));
      float s1 = eval_dist_trilinear(values1, 0.5f * (start_q + end_q));
      float density = -1.0f * ((1 - voxel_dw) * s0 + voxel_dw * s1);
      float step = length(end_pos - start_pos);
      float sampleAlpha = exp(-1.0f*density * step);
      
      float lightAtten = 1.0f;
      if (ubo[0].m_preset.use_lighting == 1 && density > 3.0f*ubo[0].m_preset.alpha_thr)
      {
        float3 pos = 0.5f * (start_pos + end_pos);
        RaytraceSCom4D_SS_shadow(to_float4(pos, 0.0001f), to_float4(m_LightDir, 1000.0f), lightAtten);
      }

      accumulatedColor += alpha * (1.0f - sampleAlpha) * lightAtten * m_LightColor * sample_color;
      alpha *= sampleAlpha;

      uint nextChildOfCurNode = new_node(currNode & 0x7, tl.x, tl.y, tl.z);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.currNode = nextChildOfCurNode | (voxel_w0_f < 0.5f ? 0 : 8);
    }
    else // this child is a node, move to it
    {
      //printf("3\n");
      uint nextChildOfCurNode = new_node(currNode & 0x7, tl.x, tl.y, tl.z);

      if (nextChildOfCurNode < 8)  // if it is not a last child, leave the current node and place the child in the stack
      {
        cur.currNode = nextChildOfCurNode | (voxel_w0_f < 0.5f ? 0 : 8);
        stack[++top] = cur;
      }

      d = 0.5f / float(cur.size);
      float3 p_f = float3(2*(cur.p.x) + ((currNode & 4) >> 2),2*(cur.p.y) + ((currNode & 2) >> 1),2*(cur.p.z) + (currNode & 1));
      t0 = _t0 + d * p_f * _l;
      tm = t0 + 0.5f*d*_l;
      float voxel_dw = 2.0f*voxel_w0_f - float(uint32_t(2.0f*voxel_w0_f));

      SdfDAGChildEdge ce = unpack_child_edge(ubo[0].m_header, m_SComNodes[childLink], m_SComNodes[childLink + ubo[0].m_header.child_offset_off]);
      NodeHeadUnpacked node = unpack_node_head(ubo[0].m_header, m_SComNodes[ce.child_offset], m_SComNodes[ce.child_offset+ubo[0].m_header.links_offset-1]);
      uint nextNode = first_node(t0, tm);

      cur.linksOff = ce.child_offset + ubo[0].m_header.links_offset + (node.base_links_end == SCOM2_CHILD_EMPTY ? 0 : ubo[0].m_header.uints_per_link*(1+ubo[0].m_header.has_channels));
      cur.transform = m_RotAddTable[cur.transform * ubo[0].m_SdfDAGTransformCount + ce.rotation_id];
      cur.currNode = nextNode | (voxel_dw < 0.5f ? 0 : 8);
      cur.chActive = node.children_active;
      cur.chTypes = node.children_types;
      cur.size = cur.size << 1;
      cur.p = (cur.p << 1) | uint4((currNode & 4) >> 2,(currNode & 2) >> 1,currNode & 1,(currNode & 8) >> 3);
    }
  }

  return alpha * ubo[0].m_backgroundColor + to_float4(accumulatedColor, 1);
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
    float4 res = RaymarchSCom4D(rayPos, rayDir);
    m_colorBuffer[y*ubo[0].m_width + x] = kgenArgs.sample_id == 0 ? res : m_colorBuffer[y*ubo[0].m_width + x] + res;
  
  }
}

