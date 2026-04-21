//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common_slang_slang.h"

RWStructuredBuffer<float4> out_color; // 
RWStructuredBuffer<uint> m_RotAddTable; // 
RWStructuredBuffer<SdfDAGChildEdge> m_SdfDAGChildEdges; // 
RWStructuredBuffer<SdfDAGDataEdge> m_SdfDAGDataEdges; // 
RWStructuredBuffer<float> m_SdfDAGDistances; // 
RWStructuredBuffer<SdfDAGHeader> m_SdfDAGHeaders; // 
RWStructuredBuffer<SdfDAGNode> m_SdfDAGNodes; // 
RWStructuredBuffer<uint> m_SdfDAGTranspositions; // 
RWStructuredBuffer<float4> m_colorBuffer; // 
RWStructuredBuffer<uint> m_packedXY; // 
RWStructuredBuffer<float> m_timestamps; // 
StructuredBuffer<VolumeRenderer_slang_UBO_Data> ubo;

float4 RaymarchDAG4D(float4 posAndNear, float4 dirAndFar) ;

void initEyeRay(uint x, uint y, uint s_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) ;



float4 RaymarchDAG4D(float4 posAndNear, float4 dirAndFar) {
#ifndef DISABLE_SDF_DAG
  //It is a naive version that iterates all child nodes, instead of traversing it in octree-like fashion
  //but DAG is a mostly for debugging similarity compression, so it does not really matter
  //Still, we should optimize it if performance testing of octree/64-tree is considered

  const float3 ray_pos = to_float3(posAndNear);
  const float3 ray_dir = to_float3(dirAndFar);
  const float tNear = posAndNear.w;
  const float tFar = dirAndFar.w;
  const float time = (float(ubo[0].frame0) + ubo[0].m_dframe) / float(ubo[0].m_timestamps_size - 1);
  const float3 ray_dir_inv = SafeInverse(ray_dir);
  OTStackElement4D stack[32];

  int top = 0;
  int buf_top = 0;
  uint3 p;
  float4 p_f;
  float3 t0, t1, tm;
  int currNode;
  uint32_t nodeId;
  uint32_t level_sz;
  float d, level_sz_f, sz_inv;
  float2 fNearFar;
  float values0[8];
  float values1[8];
  SdfDAGHeader header = m_SdfDAGHeaders[0];//m_SdfDAGHeaders[m_geomData[geomId].offset.y];

  stack[top].nodeId = 0;//m_geomData[geomId].offset.x;
  stack[top].info = 0; 
  stack[top].transform = 0; //cumulative rotation id of the node
  stack[top].p = uint4(0,0,0,0);
  stack[top].size = 1;

  float alpha = 1.0f;
  float3 accumulatedColor = float3(0.0f);

  while (top >= 0 && alpha > ubo[0].m_preset.alpha_thr)
  {
    OTStackElement4D curElem = stack[top];
    top--;

    SdfDAGNode node = m_SdfDAGNodes[curElem.nodeId];
    level_sz = curElem.size;
    level_sz_f = float(level_sz);
    p = to_uint3(curElem.p);
    
    sz_inv = 2.0f/level_sz_f;
    float3 brick_min_pos = float3(-1,-1,-1) + sz_inv*float3(p);
    float3 brick_max_pos = brick_min_pos + sz_inv*float3(1,1,1);
    float3 brick_size = brick_max_pos - brick_min_pos;

    float time_min = float(curElem.p.w) / level_sz_f;
    float time_max = time_min + 1.0f/level_sz_f;
    float time_size = time_max - time_min;
    // printf("[%d %d %d %d][%d] node %d rot %d\n", p.x, p.y, p.z, curElem.p.w, level_sz, curElem.nodeId, curElem.transform);
    // printf("%d %f %f %f %f\n", voxel_w0, voxel_w0_f, time, time_min, time_max);

    float2 brick_fNearFar = RayBoxIntersection2(ray_pos, ray_dir_inv, brick_min_pos, brick_max_pos);
    brick_fNearFar.x = max(tNear, brick_fNearFar.x);

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
        float voxel_w0_f =  float(header.brick_size)*(time - time_min)/time_size;                                            
        uint voxel_w0 = uint32_t(voxel_w0_f);
        float voxel_dw = voxel_w0_f - float(voxel_w0);

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
            int4 voxelPosI = int4(int(voxelPos.x),int(voxelPos.y),int(voxelPos.z),voxel_w0);
            uint rotIdx = m_RotAddTable[curElem.transform*ubo[0].m_SdfDAGTransformCount + 
                                            m_SdfDAGDataEdges[node.data_edges_offset + s_id].rotation_id];
            float add = m_SdfDAGDataEdges[node.data_edges_offset + s_id].add;
            for (int i=0;i<16;i++)
            {
              int4 pI = voxelPosI + int4((i & 4) >> 2,(i & 2) >> 1,i & 1,(i & 8) >> 3);
              int idx = pI.w*int(v_size*v_size*v_size) + pI.x*int(v_size*v_size) + pI.y*int(v_size) + pI.z;
              uint vId0 = m_SdfDAGTranspositions[rotIdx*v_size*v_size*v_size*v_size + idx];
              float val = m_SdfDAGDistances[offset + vId0] + add;
              if (i < 8)
                values0[i] = val;
              else 
                values1[i-8] = val;
              //printf("vId0 = %d, offset = %d add = %f, val = %f\n", vId0, offset, add, values[i]);
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
              float3 sample_color = float3(1,1,1);
              float s0 = eval_dist_trilinear(values0, 0.5f*(start_q + end_q));
              float s1 = eval_dist_trilinear(values1, 0.5f*(start_q + end_q));
              float density = -1.0f*((1-voxel_dw)*s0 + voxel_dw*s1);
              float localAlpha = 1.0f - exp(-density * length(end_pos - start_pos));
              accumulatedColor += alpha * localAlpha * sample_color;
              alpha *= 1.0f - localAlpha;
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

      float voxel_w0_f =  float(header.node_grid_size)*(time - time_min)/time_size;                                            
      uint voxel_w0 = uint32_t(voxel_w0_f);

      while (neg_brick_fNearFar.x < neg_brick_fNearFar.y)
      {
        d = 2.0f/(level_sz_f*v_sz);
        float3 hit_pos = neg_pos + neg_brick_fNearFar.x*neg_dir;
        float3 local_pos = (hit_pos - brick_min_pos) * (0.5f*level_sz_f*v_sz);
        float3 voxelPos = floor(clamp(local_pos, 1e-6f, v_sz-1e-6f));
        int4 voxelPosTime = int4((int)voxelPos.x,(int)voxelPos.y,(int)voxelPos.z,voxel_w0);
        int idx = v_sz*v_sz*v_sz*voxelPosTime.w + v_sz*v_sz*voxelPosTime.x + v_sz*voxelPosTime.y + voxelPosTime.z;
        uint voxId = m_SdfDAGTranspositions[ubo[0].m_SdfDAGBrickTranspositionOffset + curElem.transform*v_sz*v_sz*v_sz*v_sz + idx];

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
          stack[top].transform = m_RotAddTable[curElem.transform*ubo[0].m_SdfDAGTransformCount + edge.rotation_id];
          stack[top].p = v_sz*curElem.p + uint4(voxelPosTime);
          stack[top].size = 2*curElem.size;
        }

        neg_brick_fNearFar.x += max(0.0f, fNearFar.y-neg_brick_fNearFar.x) + 0.01f*d;
      }
    }
  }
  return alpha * ubo[0].m_backgroundColor + to_float4(accumulatedColor, 1);
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
    float4 res = RaymarchDAG4D(rayPos, rayDir);
    m_colorBuffer[y*ubo[0].m_width + x] = kgenArgs.sample_id == 0 ? res : m_colorBuffer[y*ubo[0].m_width + x] + res;
  
  }
}

