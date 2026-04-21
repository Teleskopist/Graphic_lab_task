//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common_slang_slang.h"

RWStructuredBuffer<float4> out_color; // 
RWStructuredBuffer<float4x4> m_DAGInverseTransforms; // 
RWStructuredBuffer<uint> m_RotAddTable; // 
RWStructuredBuffer<int4> m_SdfCompactOctreeRotModifiers; // 
RWStructuredBuffer<SdfDAGChildEdge> m_SdfDAGChildEdges; // 
RWStructuredBuffer<SdfDAGDataEdge> m_SdfDAGDataEdges; // 
RWStructuredBuffer<float> m_SdfDAGDistances; // 
RWStructuredBuffer<SdfDAGHeader> m_SdfDAGHeaders; // 
RWStructuredBuffer<SdfDAGNode> m_SdfDAGNodes; // 
RWStructuredBuffer<float4> m_SphericalHarmonics; // 
RWStructuredBuffer<float4> m_colorBuffer; // 
RWStructuredBuffer<uint> m_packedXY; // 
StructuredBuffer<VolumeRenderer_slang_UBO_Data> ubo;

float3 computeColorFromSH(float3 vox_c, float3 ro, uint idx) ;

void initEyeRay(uint x, uint y, uint s_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) ;

float4 RaymarchDAG(float4 posAndNear, float4 dirAndFar) ;



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

float4 RaymarchDAG(float4 posAndNear, float4 dirAndFar) {
#ifndef DISABLE_SDF_DAG
  //It is a naive version that iterates all child nodes, instead of traversing it in octree-like fashion
  //but DAG is a mostly for debugging similarity compression, so it does not really matter
  //Still, we should optimize it if performance testing of octree/64-tree is considered

  float3 ray_pos = to_float3(posAndNear);
  const float3 ray_dir = to_float3(dirAndFar);
  const float tNear = posAndNear.w;
  const float tFar = dirAndFar.w;
  const float3 ray_dir_inv = SafeInverse(ray_dir);
  OTDeepStackElement stack[48];

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
  if (ubo[0].m_preset.volume_type == VOLUME_TYPE_SVRASTER_RF)
  {
    float3 scene_center;
    scene_center.x = header.user_params[SCENE_CENTER_X_USER_PARAM_ID];
    scene_center.y = header.user_params[SCENE_CENTER_Y_USER_PARAM_ID];
    scene_center.z = header.user_params[SCENE_CENTER_Z_USER_PARAM_ID];
    ray_pos = (ray_pos - scene_center) * 2.0f / header.user_params[SCENE_EXTENT_USER_PARAM_ID];
  }

  stack[top].nodeId = 0;//m_geomData[geomId].offset.x;
  stack[top].info = 0; //info here is cumulative rotation id of the node
  stack[top].i = 0;
  stack[top].j = 0;
  stack[top].k = 0;
  stack[top].size = 1;

  float alpha = 1.0f;
  float3 accumulatedColor = float3(0.0f);

  while (top >= 0 && alpha > ubo[0].m_preset.alpha_thr)
  {
    OTDeepStackElement curElem = stack[top];
    top--;

    SdfDAGNode node = m_SdfDAGNodes[curElem.nodeId];
    level_sz = curElem.size;
    level_sz_f = float(level_sz);
    p = uint3(curElem.i,curElem.j,curElem.k);
    
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

        while (brick_fNearFar.x < brick_fNearFar.y && alpha > ubo[0].m_preset.alpha_thr)
        {
          uint v_size = header.brick_size + 2*header.brick_pad + 1;
          float3 hit_pos = ray_pos + brick_fNearFar.x*ray_dir;
          float3 local_pos = (hit_pos - brick_min_pos) * (0.5f*sz*header.brick_size);
          float3 voxelPos = floor(clamp(local_pos, 1e-6f, header.brick_size-1e-6f));

          float3 min_pos = brick_min_pos + d*voxelPos;
          float3 max_pos = min_pos + d*float3(1,1,1);
          float3 size = max_pos - min_pos;

          uint s_id = 0;
          while (s_id < surface_count && alpha > ubo[0].m_preset.alpha_thr)
          {
            uint offset = m_SdfDAGDataEdges[node.data_edges_offset + s_id].data_offset;
            int4 voxelPosI = int4(int(voxelPos.x),int(voxelPos.y),int(voxelPos.z),1);
            uint rotIdx = m_RotAddTable[curElem.info*ROT_COUNT + m_SdfDAGDataEdges[node.data_edges_offset + s_id].rotation_id];
            float add = m_SdfDAGDataEdges[node.data_edges_offset + s_id].add;

            for (int i=0;i<8;i++)
            {
              int4 pI = voxelPosI + int4((i & 4) >> 2,(i & 2) >> 1,i & 1,0);
              uint vId0 = uint32_t(dot(m_SdfCompactOctreeRotModifiers[rotIdx], pI));
              float val = m_SdfDAGDistances[offset + vId0] + add;
              values[i] = val;
            }

            fNearFar = RayBoxIntersection2(ray_pos, SafeInverse(ray_dir), min_pos, max_pos);
            if (fNearFar.x < fNearFar.y)    
            {
              float3 start_pos = ray_pos + fNearFar.x*ray_dir;
              float3 end_pos   = ray_pos + fNearFar.y*ray_dir;
              float vox_sz = (0.5f*sz*header.brick_size);
              float3 start_q = vox_sz * (start_pos - min_pos);
              float3 end_q = vox_sz * (end_pos - min_pos);
          
              // Sample and compositing
              float i_steps = 1.0f/float(ubo[0].m_preset.voxel_traversal_steps);
              for (int smp = 0; smp < ubo[0].m_preset.voxel_traversal_steps; smp++)
              {
                float smp_q = i_steps*(smp + 0.5f);
                float density = -1.0f*eval_dist_trilinear(values, start_q * (1.0f - smp_q) + end_q * smp_q);
                float3 sample_color = to_float3(ubo[0].m_baseColor);
                if (ubo[0].m_preset.volume_type == VOLUME_TYPE_SVRASTER_RF)
                {
                  density = 100.0f*header.user_params[SCENE_EXTENT_USER_PARAM_ID]*exp_linear_11(density);
                  float4 v = to_float4(ray_pos - 0.5f*(start_pos + end_pos), 0.0f);
                  uint chRotIdx = m_RotAddTable[curElem.info*ROT_COUNT + node.channels_edge.rotation_id];
                  v = m_DAGInverseTransforms[chRotIdx] * v;

                  sample_color = computeColorFromSH(float3(0,0,0), to_float3(v), node.channels_edge.child_offset);
                }
                density = max(density, 0.0f);
                float localAlpha = 1.0f - exp(-density * i_steps * length(end_pos - start_pos));
                accumulatedColor += alpha * localAlpha * sample_color;
                alpha *= 1.0f - localAlpha;
              }
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
          stack[top].i = curElem.i*v_sz + uint(voxelPos.x);
          stack[top].j = curElem.j*v_sz + uint(voxelPos.y);
          stack[top].k = curElem.k*v_sz + uint(voxelPos.z);
          stack[top].size = curElem.size*v_sz;
        }

        neg_brick_fNearFar.x += max(0.0f, fNearFar.y-neg_brick_fNearFar.x) + 0.01f*d;
      }
    }
  }
  return alpha * ubo[0].m_backgroundColor + to_float4(accumulatedColor, 1);
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
    float4 res = RaymarchDAG(rayPos, rayDir);
    m_colorBuffer[y*ubo[0].m_width + x] = kgenArgs.sample_id == 0 ? res : m_colorBuffer[y*ubo[0].m_width + x] + res;
  
  }
}

