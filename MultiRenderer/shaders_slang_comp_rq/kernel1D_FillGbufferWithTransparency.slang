//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common_slang_comp_rq_slang.h"

RWStructuredBuffer<CRT_Hit> m_gBuffer; // 
RWStructuredBuffer<uint2> m_matIdOffsets; // 
RWStructuredBuffer<uint> m_matIdbyPrimId; // 
RWStructuredBuffer<MultiRendererMaterial> m_materials; // 
RaytracingAccelerationStructure m_pAccelStruct;
RWStructuredBuffer<GraphicsPrimHeader> m_pAccelStruct_m_GraphicsPrimHeaders; // 
RWStructuredBuffer<float4> m_pAccelStruct_m_GraphicsPrimPoints; // 
RWStructuredBuffer<uint> m_pAccelStruct_m_RotAddTable; // 
RWStructuredBuffer<uint> m_pAccelStruct_m_SCom2Data; // 
RWStructuredBuffer<SCom2Header> m_pAccelStruct_m_SCom2Headers; // 
RWStructuredBuffer<int4> m_pAccelStruct_m_SdfCompactOctreeRotModifiers; // 
RWStructuredBuffer<float4x4> m_pAccelStruct_m_SdfCompactOctreeRotTransforms; // 
RWStructuredBuffer<uint> m_pAccelStruct_m_SdfCompactOctreeV3Data; // 
RWStructuredBuffer<SdfDAGChildEdge> m_pAccelStruct_m_SdfDAGChildEdges; // 
RWStructuredBuffer<SdfDAGDataEdge> m_pAccelStruct_m_SdfDAGDataEdges; // 
RWStructuredBuffer<float> m_pAccelStruct_m_SdfDAGDistances; // 
RWStructuredBuffer<SdfDAGHeader> m_pAccelStruct_m_SdfDAGHeaders; // 
RWStructuredBuffer<SdfDAGNode> m_pAccelStruct_m_SdfDAGNodes; // 
RWStructuredBuffer<uint> m_pAccelStruct_m_SdfDAGTranspositions; // 
RWStructuredBuffer<SdfFrameOctreeNode> m_pAccelStruct_m_SdfFrameOctreeNodes; // 
RWStructuredBuffer<uint> m_pAccelStruct_m_SdfFrameOctreeRoots; // 
RWStructuredBuffer<SdfFrameOctreeTexNode> m_pAccelStruct_m_SdfFrameOctreeTexNodes; // 
RWStructuredBuffer<uint> m_pAccelStruct_m_SdfFrameOctreeTexRoots; // 
RWStructuredBuffer<float> m_pAccelStruct_m_SdfGridData; // 
RWStructuredBuffer<uint> m_pAccelStruct_m_SdfGridOffsets; // 
RWStructuredBuffer<uint3> m_pAccelStruct_m_SdfGridSizes; // 
RWStructuredBuffer<SdfMultiOctreeNode> m_pAccelStruct_m_SdfMultiOctreeNodes; // 
RWStructuredBuffer<float> m_pAccelStruct_m_SdfMultiOctreeValues; // 
RWStructuredBuffer<uint> m_pAccelStruct_m_SdfSBSData; // 
RWStructuredBuffer<float> m_pAccelStruct_m_SdfSBSDataF; // 
RWStructuredBuffer<SdfSBSHeader> m_pAccelStruct_m_SdfSBSHeaders; // 
RWStructuredBuffer<SdfSBSNode> m_pAccelStruct_m_SdfSBSNodes; // 
RWStructuredBuffer<uint> m_pAccelStruct_m_SdfSBSRoots; // 
RWStructuredBuffer<SdfSVSNode> m_pAccelStruct_m_SdfSVSNodes; // 
RWStructuredBuffer<uint> m_pAccelStruct_m_SdfSVSRoots; // 
RWStructuredBuffer<BVHNodePair> m_pAccelStruct_m_allNodePairs; // 
RWStructuredBuffer<GeomData> m_pAccelStruct_m_geomData; // 
RWStructuredBuffer<uint> m_pAccelStruct_m_geomTags; // 
RWStructuredBuffer<uint> m_pAccelStruct_m_indices; // 
RWStructuredBuffer<InstanceData> m_pAccelStruct_m_instanceData; // 
RWStructuredBuffer<BVHNode> m_pAccelStruct_m_nodesTLAS; // 
RWStructuredBuffer<BVHNode> m_pAccelStruct_m_origNodes; // 
RWStructuredBuffer<uint> m_pAccelStruct_m_primIdCount; // 
RWStructuredBuffer<uint> m_pAccelStruct_m_primIndices; // 
RWStructuredBuffer<float4> m_pAccelStruct_m_vertNorm; // 
RWStructuredBuffer<float4> m_pAccelStruct_m_vertPos; // 
RWStructuredBuffer<uint2> m_pAccelStruct_remap; // 
RWStructuredBuffer<uint2> m_pAccelStruct_startEnd; // 
RWStructuredBuffer<uint> m_packedXY; // 
RWStructuredBuffer<float4> m_transparencyBuffer; // 
RWStructuredBuffer<MultiRenderer_slang_comp_rq_UBO_Data> ubo;

float m_pAccelStruct_COctreeV3_LoadDistanceValues(uint leafType, uint brickOffset, float3 voxelPos, uint v_size, float sz_inv, in COctreeV3Header header, uint transform_code, inout float values[8]) ;

uint m_pAccelStruct_IntersectAllPrimitivesInLeaf(float4 rayPosAndNear, float4 rayDirAndFar, CRT_LeafInfo info, inout CRT_Hit pHit) ;

void m_pAccelStruct_COctreeV3_BrickIntersect(uint type, const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint brickOffset, float3 p, float sz, uint transform_code_leaf_type, inout CRT_Hit pHit) ;

void initEyeRay(uint x, uint y, uint a_sample_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) ;

void m_pAccelStruct_LocalSurfaceIntersection(uint type, const float3 ray_dir, uint instId, uint geomId, inout float values[8], uint nodeId, uint primId, float d, float qNear, float qFar, float2 fNearFar, float3 start_q, bool is_surface_node, inout CRT_Hit pHit) ;

CRT_Hit m_pAccelStruct_RayQuery_NearestHit(float4 posAndNear, float4 dirAndFar) ;

float m_pAccelStruct_load_distance_values(uint nodeId, float3 voxelPos, uint v_size, float sz_inv, in SdfSBSHeader header, inout float values[8]) ;

float m_pAccelStruct_COctreeV3_LoadDistanceValuesLeafGrid(uint brickOffset, float3 voxelPos, uint v_size, float sz_inv, in COctreeV3Header header, uint transform_code, inout float values[8]) ;

void m_pAccelStruct_IntersectSDFGrid(const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint a_start, uint a_count, inout CRT_Hit pHit) ;

void m_pAccelStruct_COctreeIntersectV3(uint model_type, const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint bvhNodeId, uint a_count, inout CRT_Hit pHit) ;

void m_pAccelStruct_OctreeIntersect(uint type, const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint a_start, uint a_count, inout CRT_Hit pHit) ;

void m_pAccelStruct_RayNodeIntersection(uint type, const float3 ray_pos, const float3 ray_dir, float tNear, uint geomId, uint bvhNodeId, inout float values[8], inout uint primId, inout uint nodeId, inout float d, inout float qNear, inout float qFar, inout float2 fNearFar, inout float3 start_q) ;

void m_pAccelStruct_DAGIntersect(uint type, const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint a_start, uint a_count, inout CRT_Hit pHit) ;

void m_pAccelStruct_SCom2IntersectFast(uint type, const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint a_start, uint a_count, inout CRT_Hit pHit) ;

float m_pAccelStruct_COctreeV3_LoadDistanceValuesLeafSlices(uint brickOffset, float3 voxelPos, uint v_size, float sz_inv, in COctreeV3Header header, uint transform_code, inout float values[8]) ;

float m_pAccelStruct_eval_dist_trilinear(const float values[8], float3 dp) ;

void m_pAccelStruct_OctreeNodeIntersect(uint type, const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint a_start, uint a_count, inout CRT_Hit pHit) ;

void m_pAccelStruct_SCom2Intersect(uint type, const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint a_start, uint a_count, inout CRT_Hit pHit) ;

float4 AnyHit(inout CRT_Hit hit, float4 rayPosAndNear, float4 rayDirAndFar) ;

void m_pAccelStruct_TricubicLocalSurfaceIntersection(uint type, const float3 ray_dir, uint instId, uint geomId, inout float values[64], uint nodeId, uint primId, float d, float qNear, float qFar, float2 fNearFar, float3 start_q, inout CRT_Hit pHit) ;

void m_pAccelStruct_OctreeBrickIntersect(uint type, const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint bvhNodeId, uint a_count, inout CRT_Hit pHit) ;

float m_pAccelStruct_COctreeV3_LoadDistanceValuesLeafBitPack(uint brickOffset, float3 voxelPos, uint v_size, float sz_inv, in COctreeV3Header header, uint transform_code, inout float values[8]) ;

void m_pAccelStruct_IntersectAllTrianglesInLeaf(const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint a_start, uint a_count, inout CRT_Hit pHit) ;

void m_pAccelStruct_BVH2TraverseF32(const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, bool stopOnFirstHit, inout CRT_Hit pHit) ;

float2 m_pAccelStruct_box_intersects(in float3 min_pos, in float3 max_pos, in float3 origin, in float3 dir) ;

void m_pAccelStruct_IntersectGraphicPrims(in float3 ray_pos, in float3 ray_dir, float tNear, uint instId, uint geomId, uint a_start, uint a_count, inout CRT_Hit pHit) ;

bool m_pAccelStruct_need_normal() ;

float m_pAccelStruct_load_tricubic_distance_values(uint nodeId, float3 voxelPos, uint v_size, float sz_inv, in SdfSBSHeader header, inout float values[64]) ;

float m_pAccelStruct_eval_distance_sdf_grid(uint grid_id, float3 pos) ;

void m_pAccelStruct_LocalSurfaceIntersectionFast(uint type, const float3 ray_dir, uint instId, uint geomId, inout float values[8], uint nodeId, uint primId, float d, float qNear, float qFar, float2 fNearFar, float3 start_q, bool is_surface_node, inout CRT_Hit pHit) ;

float m_pAccelStruct_tricubicInterpolation(const float grid[64], const float dp[3]) ;

// RayScene intersection with 'm_pAccelStruct'
//
CRT_Hit m_pAccelStruct_RayQuery_NearestHit(float4 rayPos, float4 rayDir)
{
  RayDesc ray;
  ray.Origin    = rayPos.xyz;
  ray.Direction = rayDir.xyz;
  ray.TMin      = rayPos.w;
  ray.TMax      = rayDir.w;

  RayQuery<RAY_FLAG_FORCE_OPAQUE> rayQuery;
  rayQuery.TraceRayInline(m_pAccelStruct, RAY_FLAG_NONE, 0xff, ray);
  
  CRT_Hit res;
  res.primId = -1;
  res.instId = -1;
  res.geomId = -1;
  res.t      = rayDir.w;
  
  while(rayQuery.Proceed())
  {
    if (rayQuery.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
    {
      // TODO: add opacity check here, use rayQuery.Abort() if didn't find the hit;
      rayQuery.CommitNonOpaqueTriangleHit();
    }
    else if (rayQuery.CandidateType() == CANDIDATE_PROCEDURAL_PRIMITIVE)
    {
      float4 rayPosAndNear = float4(rayQuery.CandidateObjectRayOrigin(), rayPos.w);
      float4 rayDirAndFar  = float4(rayQuery.CandidateObjectRayDirection(), rayDir.w);
      const uint geomIndex = rayQuery.CandidateInstanceID(); // user defined geometry per-inatsnce index
      uint2 remap = m_pAccelStruct_remap[geomIndex];
      CRT_LeafInfo info;
      info.aabbId = rayQuery.CandidatePrimitiveIndex();
      info.primId = remap.x + info.aabbId / remap.y;
      info.instId = rayQuery.CandidateInstanceIndex();
      info.geomId = geomIndex;
      info.rayxId = 0; // TODO: gl_GlobalInvocationID[0];
      info.rayyId = 0; // TODO: gl_GlobalInvocationID[1];
      uint intersected = m_pAccelStruct_IntersectAllPrimitivesInLeaf(rayPosAndNear, rayDirAndFar, info, res);
      if (intersected != 0)
        rayQuery.CommitProceduralPrimitiveHit(res.t);
    }
  } // may omit 'while' loop when 'RAY_FLAG_FORCE_OPAQUE' is used

  if(rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
  {
    res.primId = rayQuery.CommittedPrimitiveIndex(); // auto generated index
    res.geomId = rayQuery.CommittedInstanceID();     // user defined geometry per-inatsnce index
    res.instId = rayQuery.CommittedInstanceIndex();  // auto generated index
    res.t      = rayQuery.CommittedRayT();

    float2 bars = rayQuery.CommittedTriangleBarycentrics();
  
    res.coords[0] = bars.y;
    res.coords[1] = bars.x;
    res.coords[2] = 1.0f - bars.y - bars.x;
    res.coords[3] = 0.0f;
  }

  return res;
}

bool m_pAccelStruct_RayQuery_AnyHit(float4 rayPos, float4 rayDir)
{
  RayDesc ray;
  ray.Origin    = rayPos.xyz;
  ray.Direction = rayDir.xyz;
  ray.TMin      = rayPos.w;
  ray.TMax      = rayDir.w;

  RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> rayQuery;
  rayQuery.TraceRayInline(m_pAccelStruct, RAY_FLAG_NONE, 0xff, ray);
  rayQuery.Proceed();

  return (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT);
}

CRT_Hit m_pAccelStruct_RayQuery_NearestHitMotion(float4 rayPos, float4 rayDir, float t) { return m_pAccelStruct_RayQuery_NearestHit(rayPos, rayDir); }
bool m_pAccelStruct_RayQuery_AnyHitMotion(float4 rayPos, float4 rayDir, float t) { return m_pAccelStruct_RayQuery_AnyHit(rayPos, rayDir); }



float m_pAccelStruct_COctreeV3_LoadDistanceValues(uint leafType, uint brickOffset, float3 voxelPos, uint v_size, float sz_inv, in COctreeV3Header header, uint transform_code, inout float values[8]) {
  switch (leafType)
  {
  case COCTREE_LEAF_TYPE_GRID:
    return m_pAccelStruct_COctreeV3_LoadDistanceValuesLeafGrid(brickOffset, voxelPos, v_size, sz_inv, header, transform_code, values);
  case COCTREE_LEAF_TYPE_BIT_PACK:
    return m_pAccelStruct_COctreeV3_LoadDistanceValuesLeafBitPack(brickOffset, voxelPos, v_size, sz_inv, header, transform_code, values);
  case COCTREE_LEAF_TYPE_SLICES:
    return m_pAccelStruct_COctreeV3_LoadDistanceValuesLeafSlices(brickOffset, voxelPos, v_size, sz_inv, header, transform_code, values);
  }
  return 1e6f;
}

uint m_pAccelStruct_IntersectAllPrimitivesInLeaf(float4 rayPosAndNear, float4 rayDirAndFar, CRT_LeafInfo info, inout CRT_Hit pHit) {
  const float3 ray_pos = to_float3(rayPosAndNear);
  const float3 ray_dir = to_float3(rayDirAndFar);
  const float tNear    = rayPosAndNear.w;
  const float tPrev    = pHit.t;

  const uint globalAABBId = m_pAccelStruct_startEnd[info.geomId].x + info.aabbId;
  const uint start_count_packed = m_pAccelStruct_m_primIdCount[globalAABBId];
  const uint a_start = EXTRACT_START(start_count_packed);
  const uint a_count = EXTRACT_COUNT(start_count_packed);

  const uint geomTag = m_pAccelStruct_m_geomTags[info.geomId];
  switch(geomTag)
  {
    #ifndef DISABLE_MESH
    case TAG_TRIANGLE: 
    m_pAccelStruct_IntersectAllTrianglesInLeaf(ray_pos, ray_dir, tNear, info.instId, info.geomId, a_start, a_count, pHit);
    break;
    #endif

    #ifndef DISABLE_SDF_GRID
    case TAG_SDF_GRID:
    m_pAccelStruct_IntersectSDFGrid(ray_pos, ray_dir, tNear, info.instId, info.geomId, a_start, a_count, pHit);
    break;
    #endif
    
    #if !defined(DISABLE_SDF_SVS) || !defined(DISABLE_SDF_SBS) || !defined(DISABLE_SDF_FRAME_OCTREE)
    case TAG_SDF_NODE:
    m_pAccelStruct_OctreeNodeIntersect(m_pAccelStruct_m_geomData[info.geomId].type, ray_pos, ray_dir, tNear, info.instId, info.geomId, a_start, a_count, pHit);
    break;
    #endif

    #if !defined(DISABLE_SDF_SVS) || !defined(DISABLE_SDF_FRAME_OCTREE_TEX) || !defined(DISABLE_SDF_FRAME_OCTREE)
    case TAG_SDF_BRICK:
    m_pAccelStruct_OctreeBrickIntersect(m_pAccelStruct_m_geomData[info.geomId].type, ray_pos, ray_dir, tNear, info.instId, info.geomId, a_start, a_count, pHit);
    break;
    #endif
    
    #ifndef DISABLE_GRAPHICS_PRIM
    case TAG_GRAPHICS_PRIM:
    m_pAccelStruct_IntersectGraphicPrims(ray_pos, ray_dir, tNear, info.instId, info.geomId, a_start, a_count, pHit);
    break;
    #endif
    
    #ifndef DISABLE_SDF_COCTREE_V3
    case TAG_COCTREE_BRICKED:
    m_pAccelStruct_COctreeIntersectV3(m_pAccelStruct_m_geomData[info.geomId].type, ray_pos, ray_dir, tNear, info.instId, info.geomId, a_start, a_count, pHit);
    break;
    #endif
    
    #ifndef DISABLE_SDF_MULTI_OCTREE
    case TAG_SDF_OCTREE:
    m_pAccelStruct_OctreeIntersect(m_pAccelStruct_m_geomData[info.geomId].type, ray_pos, ray_dir, tNear, info.instId, info.geomId, a_start, a_count, pHit);
    break;
    #endif

    #ifndef DISABLE_SDF_DAG
    case TAG_SDF_DAG:
    m_pAccelStruct_DAGIntersect(m_pAccelStruct_m_geomData[info.geomId].type, ray_pos, ray_dir, tNear, info.instId, info.geomId, a_start, a_count, pHit);
    break;
    #endif

    #ifndef DISABLE_SCOM2
    case TAG_SCOM2:
    if (ubo[0].m_pAccelStruct_support_fast_scom_traversal)
      m_pAccelStruct_SCom2IntersectFast(m_pAccelStruct_m_geomData[info.geomId].type, ray_pos, ray_dir, tNear, info.instId, info.geomId, a_start, a_count, pHit);
    else
      m_pAccelStruct_SCom2Intersect(m_pAccelStruct_m_geomData[info.geomId].type, ray_pos, ray_dir, tNear, info.instId, info.geomId, a_start, a_count, pHit);
    break;
    #endif
  };

  if (ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_INTERSECTION_COUNT)
    pHit.instId = (pHit.instId == 0xFFFFFFFF) ? 1 : pHit.instId+1; 

  return (pHit.t >= tPrev) ? TAG_NONE : geomTag;
}

void m_pAccelStruct_COctreeV3_BrickIntersect(uint type, const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint brickOffset, float3 p, float sz, uint transform_code_leaf_type, inout CRT_Hit pHit) {
#ifndef DISABLE_SDF_COCTREE_V3
  float values[8];

  float d, qNear, qFar;
  float2 fNearFar;
  float3 start_q;

  qNear = 1.0f;

  uint v_size = ubo[0].m_pAccelStruct_coctree_v3_header.brick_size + 2*ubo[0].m_pAccelStruct_coctree_v3_header.brick_pad + 1;
  uint leaf_type = transform_code_leaf_type & COCTREE_LEAF_TYPE_MASK;
  uint transform_code = transform_code_leaf_type >> COCTREE_LEAF_TYPE_BITS;

  float sz_inv = 2.0f/sz;
  
  d = 2.0f/(sz*ubo[0].m_pAccelStruct_coctree_v3_header.brick_size);

  float3 brick_min_pos = float3(-1,-1,-1) + sz_inv*p;
  float3 brick_max_pos = brick_min_pos + sz_inv*float3(1,1,1);
  float3 brick_size = brick_max_pos - brick_min_pos;

  float2 brick_fNearFar = RayBoxIntersection2(ray_pos, SafeInverse(ray_dir), brick_min_pos, brick_max_pos);
  brick_fNearFar.x = max(tNear, brick_fNearFar.x);
  float old_t = pHit.t;

  while (brick_fNearFar.x < brick_fNearFar.y && pHit.t == old_t)
  {
    float3 hit_pos = ray_pos + brick_fNearFar.x*ray_dir;
    float3 local_pos = (hit_pos - brick_min_pos) * (0.5f*sz*ubo[0].m_pAccelStruct_coctree_v3_header.brick_size);
    float3 voxelPos = floor(clamp(local_pos, 1e-6f, ubo[0].m_pAccelStruct_coctree_v3_header.brick_size-1e-6f));

    float3 min_pos = brick_min_pos + d*voxelPos;
    float3 max_pos = min_pos + d*float3(1,1,1);
    float3 size = max_pos - min_pos;
    
    COctreeV3Header header = ubo[0].m_pAccelStruct_coctree_v3_header;
    float vmin = m_pAccelStruct_COctreeV3_LoadDistanceValues(leaf_type, brickOffset, voxelPos, v_size, sz_inv, header, transform_code, values); // (!!!)

    fNearFar = RayBoxIntersection2(ray_pos, SafeInverse(ray_dir), min_pos, max_pos);
    if (tNear < fNearFar.x && vmin <= 0.0f)    
    {
      float3 start_pos = ray_pos + fNearFar.x*ray_dir;
      start_q = (start_pos - min_pos) * (0.5f*sz*ubo[0].m_pAccelStruct_coctree_v3_header.brick_size);
      qFar = (fNearFar.y - fNearFar.x) * (0.5f*sz*ubo[0].m_pAccelStruct_coctree_v3_header.brick_size);
    
      m_pAccelStruct_LocalSurfaceIntersection(type, ray_dir, instId, geomId, values, brickOffset, brickOffset, d, 0.0f, qFar, fNearFar, start_q, false, pHit); /*out*/
    
      if (ubo[0].m_pAccelStruct_m_preset.normal_mode == NORMAL_MODE_SDF_SMOOTHED && m_pAccelStruct_need_normal() && pHit.t < old_t)
      {
        const float beta = 0.5f;
        float3 dp = start_q + ((pHit.t - fNearFar.x)/d)*ray_dir; //linear interpolation coefficients in voxels

        float3 normals[8];
        float values_n[8];
        for (int i=0;i<8;i++)
          normals[i] = float3(0,0,0);
        
        float3 nmq = float3(0.5f,0.5f,0.5f);
        nmq.x = dp.x < 0.5f ? step(-beta, beta, dp.x) : step(-beta, beta, dp.x - 1.0f);
        nmq.y = dp.y < 0.5f ? step(-beta, beta, dp.y) : step(-beta, beta, dp.y - 1.0f);
        nmq.z = dp.z < 0.5f ? step(-beta, beta, dp.z) : step(-beta, beta, dp.z - 1.0f);

        int3 voxelPos0 = int3((int)(dp.x < 0.5f ? voxelPos.x - 1 : voxelPos.x),(int)(dp.y < 0.5f ? voxelPos.y - 1 : voxelPos.y),(int)(dp.z < 0.5f ? voxelPos.z - 1 : voxelPos.z));
        for (int i=0;i<8;i++)
        {
          int3 VoxelPosI = voxelPos0 + int3((i & 4) >> 2,(i & 2) >> 1,i & 1);
          float3 dVoxelPos = float3(VoxelPosI) - voxelPos;
          
          float vmin_n = m_pAccelStruct_COctreeV3_LoadDistanceValues(leaf_type, brickOffset, float3(VoxelPosI), v_size, sz_inv, header, transform_code, values_n); // (!!!)
          if (vmin_n <= 0.0f)
            normals[i] = normalize(eval_dist_trilinear_diff(values_n, dp - dVoxelPos));
        }

        float3 smoothed_norm = (1-nmq.x)*(1-nmq.y)*(1-nmq.z)*normals[0] + 
                               (1-nmq.x)*(1-nmq.y)*(  nmq.z)*normals[1] + 
                               (1-nmq.x)*(  nmq.y)*(1-nmq.z)*normals[2] + 
                               (1-nmq.x)*(  nmq.y)*(  nmq.z)*normals[3] + 
                               (  nmq.x)*(1-nmq.y)*(1-nmq.z)*normals[4] + 
                               (  nmq.x)*(1-nmq.y)*(  nmq.z)*normals[5] + 
                               (  nmq.x)*(  nmq.y)*(1-nmq.z)*normals[6] + 
                               (  nmq.x)*(  nmq.y)*(  nmq.z)*normals[7];
        smoothed_norm = normalize(matmul4x3(m_pAccelStruct_m_instanceData[instId].transformInvTransposed, smoothed_norm));
        float2 encoded_norm = encode_normal(smoothed_norm);

        pHit.coords[2] = encoded_norm.x;
        pHit.coords[3] = encoded_norm.y;
      }
    }
    
    brick_fNearFar.x += max(0.0f, fNearFar.y-brick_fNearFar.x) + 1e-5f;
  }
  
  //ray hit a brick
  if (pHit.t < old_t && ubo[0].m_pAccelStruct_coctree_v3_header.uv_size > 0)
  {
    uint p_size = ubo[0].m_pAccelStruct_coctree_v3_header.brick_size + 2 * ubo[0].m_pAccelStruct_coctree_v3_header.brick_pad;
  
    //this voxel is guaranteed to have surface
    const uint line_distances_offset_bits = 16;
    const uint line_distances_offsets_per_uint = 32 / line_distances_offset_bits;

    uint slice_distance_flags_bits = v_size * v_size;
    uint slice_distance_flags_uints = (slice_distance_flags_bits + 32 - 1) / 32;
    uint distance_flags_size_uints = v_size * slice_distance_flags_uints;
    uint presence_flags_size_uints = (p_size * p_size * p_size + 32 - 1) / 32;
    uint distance_offsets_size_uints = (v_size + line_distances_offsets_per_uint - 1) / line_distances_offsets_per_uint; // 16 bits for offset, should be enough for all cases

    const uint min_range_size_uints = 2;

    //<presence_flags><distance_flags><distance_offsets><min value and range><distances>
    uint off_0 = 0;                                     // presence flags
    uint off_1 = off_0 + presence_flags_size_uints;     // distance flags
    uint off_2 = off_1 + distance_flags_size_uints;     // distance offsets
    uint off_3 = off_2 + distance_offsets_size_uints;   // min value and range
    uint off_4 = off_3 + min_range_size_uints;          // texture coordinates
    uint off_5 = off_4 + 8 * ubo[0].m_pAccelStruct_coctree_v3_header.uv_size; // distances

    float3 pos = ray_pos + pHit.t*ray_dir;
    float3 dp = (pos - brick_min_pos)*(0.5f*sz);

      pHit.coords[0] = (1-dp.x)*(1-dp.y)*(1-dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_4 + 0] >> 16)) + 
                        (1-dp.x)*(1-dp.y)*(  dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_4 + 1] >> 16)) + 
                        (1-dp.x)*(  dp.y)*(1-dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_4 + 2] >> 16)) + 
                        (1-dp.x)*(  dp.y)*(  dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_4 + 3] >> 16)) + 
                        (  dp.x)*(1-dp.y)*(1-dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_4 + 4] >> 16)) + 
                        (  dp.x)*(1-dp.y)*(  dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_4 + 5] >> 16)) + 
                        (  dp.x)*(  dp.y)*(1-dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_4 + 6] >> 16)) + 
                        (  dp.x)*(  dp.y)*(  dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_4 + 7] >> 16));

      pHit.coords[1] = (1-dp.x)*(1-dp.y)*(1-dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_4 + 0] & 0xFFFF)) + 
                        (1-dp.x)*(1-dp.y)*(  dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_4 + 1] & 0xFFFF)) + 
                        (1-dp.x)*(  dp.y)*(1-dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_4 + 2] & 0xFFFF)) + 
                        (1-dp.x)*(  dp.y)*(  dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_4 + 3] & 0xFFFF)) + 
                        (  dp.x)*(1-dp.y)*(1-dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_4 + 4] & 0xFFFF)) + 
                        (  dp.x)*(1-dp.y)*(  dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_4 + 5] & 0xFFFF)) + 
                        (  dp.x)*(  dp.y)*(1-dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_4 + 6] & 0xFFFF)) + 
                        (  dp.x)*(  dp.y)*(  dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_4 + 7] & 0xFFFF));

  }
#endif
}

void initEyeRay(uint x, uint y, uint a_sample_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) {
  float2 HamiltonSequence[16] = {
    float2(0.500000,0.333333),
    float2(0.250000,0.666667),
    float2(0.750000,0.111111),
    float2(0.125000,0.444444),
    float2(0.625000,0.777778),
    float2(0.375000,0.222222),
    float2(0.875000,0.555556),
    float2(0.062500,0.888889),
    float2(0.562500,0.037037),
    float2(0.312500,0.370370),
    float2(0.812500,0.703704),
    float2(0.187500,0.148148),
    float2(0.687500,0.481481),
    float2(0.437500,0.814815),
    float2(0.937500,0.259259),
    float2(0.031250,0.592593)
  };

  uint spp_sqrt = uint32_t(sqrt(ubo[0].m_preset.spp));
  float i_spp_sqrt = 1.0f/spp_sqrt;
  float2 d = (ubo[0].m_AAFrameNum >= 2 || ubo[0].m_preset.ray_gen_mode == RAY_GEN_MODE_RANDOM) ? 
              /*rand2(m_seed, x, y, a_sample_id + m_AAFrameNum + m_seed % 2)*/HamiltonSequence[ubo[0].m_AAFrameNum % 16] : i_spp_sqrt*float2(a_sample_id/spp_sqrt+0.5,a_sample_id%spp_sqrt+0.5);


  float3 rayDir = EyeRayDirNormalized((float(x)+d.x)/float(ubo[0].m_packedXY_width), (float(y)+d.y)/float(ubo[0].m_packedXY_height), ubo[0].m_projInv);
  float3 rayPos = float3(0,0,0);

  transform_ray3f(ubo[0].m_worldViewInv, 
                  rayPos, rayDir);
  
  float rayNear = 1e-9f;
  float rayFar  =  1e9f;
  float sgn = sign(dot(rayPos - ubo[0].m_cuttingPlane.pos, ubo[0].m_cuttingPlane.normal));
  float denom = ubo[0].m_cuttingPlane.is_active > 0 ? dot(ubo[0].m_cuttingPlane.normal, rayDir) : 0.0f;
  rayNear = sgn*abs(denom) < -1e-6f ? dot(ubo[0].m_cuttingPlane.pos - rayPos, ubo[0].m_cuttingPlane.normal) / denom : rayNear;
  rayFar  = sgn*abs(denom) >  1e-6f ? dot(ubo[0].m_cuttingPlane.pos - rayPos, ubo[0].m_cuttingPlane.normal) / denom : rayFar;
  rayPosAndNear = to_float4(rayPos, rayNear < 0 ? rayFar : rayNear);
  rayDirAndFar  = to_float4(rayDir, rayFar < 0 ?    1e9f :  rayFar);
}

void m_pAccelStruct_LocalSurfaceIntersection(uint type, const float3 ray_dir, uint instId, uint geomId, inout float values[8], uint nodeId, uint primId, float d, float qNear, float qFar, float2 fNearFar, float3 start_q, bool is_surface_node, inout CRT_Hit pHit) {
  const float BASE_EPS = 1e-6f;
  const float EPS = (is_surface_node && is_compact_type(type)) ? max(BASE_EPS, ubo[0].m_pAccelStruct_m_preset.compact_sdf_eps*d) : BASE_EPS;
  float d_inv = 1.0f / d;
  float t = qNear;
  bool early_hit = false;
  bool hit = false;
  uint iter = 0;

  float start_dist = 10000;
  float start_sign = 1;

  start_dist = m_pAccelStruct_eval_dist_trilinear(values, start_q + t * ray_dir);
  /*If we want to represent thin surfaces, we should find rays that reach 
  //zero distance regardless of what thww sign of initial distance is.
  //However, it can lead to visual artifacts and disabled by default
  */
  if (is_surface_node || ubo[0].m_pAccelStruct_m_preset.representation_mode == REPRESENTATION_MODE_SURFACE)
    start_sign = sign(start_dist);
  start_dist *= start_sign;

  if (start_dist <= EPS || ubo[0].m_pAccelStruct_m_preset.sdf_node_intersect == SDF_OCTREE_NODE_INTERSECT_BBOX)
  {
    early_hit = true;
    hit = true;
  }
  else if (ubo[0].m_pAccelStruct_m_preset.sdf_node_intersect == SDF_OCTREE_NODE_INTERSECT_ST)
  {
    const uint ST_max_iters = 256;
    float dist = start_dist;
    float3 pp0 = start_q + t * ray_dir;

    while (t < qFar && dist > EPS && iter < ST_max_iters)
    {
      t += dist * d_inv;

      dist = start_sign*m_pAccelStruct_eval_dist_trilinear(values, start_q + t * ray_dir);
      
      float3 pp = start_q + t * ray_dir;
      iter++;
    }
    hit = (dist <= EPS);
  }
  else //if (m_preset.sdf_node_intersect == SDF_OCTREE_NODE_INTERSECT_ANALYTIC ||
       //    m_preset.sdf_node_intersect == SDF_OCTREE_NODE_INTERSECT_NEWTON ||
       //    m_preset.sdf_node_intersect == SDF_OCTREE_NODE_INTERSECT_IT)
  {
    //finding exact intersection between surface sdf(x,y,z) = 0 and ray
    // based on paper "Ray Tracing of Signed Distance Function Grids, 
    // Journal of Computer Graphics Techniques (JCGT), vol. 11, no. 3, 94-113, 2022"
    // http://jcgt.org/published/0011/03/06/

    // define values and constants as proposed in paper
    float s000 = values[0]*d_inv;
    float s001 = values[1]*d_inv;
    float s010 = values[2]*d_inv;
    float s011 = values[3]*d_inv;
    float s100 = values[4]*d_inv;
    float s101 = values[5]*d_inv;
    float s110 = values[6]*d_inv;
    float s111 = values[7]*d_inv;

    float tmp = s101-s001;

    float k0 = s000;
    float k1 = s100-s000;
    float k2 = s010-s000;
    float k3 = s110-s010-k1;
    float k4 = k0-s001;
    float k5 = k1-tmp;
    float k6 = k2-(s011-s001);
    float k7 = k3-(s111-s011-tmp);

    float3 o = start_q;
    float3 d3 = ray_dir;

    float m0 = o.x*o.y;
    float m1 = d3.x*d3.y;
    float m2 = o.x*d3.y + o.y*d3.x;
    float m3 = k5*o.z - k1;
    float m4 = k6*o.z - k2;
    float m5 = k7*o.z - k3;

    float c0 = (k4*o.z - k0) + o.x*m3 + o.y*m4 + m0*m5;
    float c1 = d3.x*m3 + d3.y*m4 + m2*m5 + d3.z*(k4 + k5*o.x + k6*o.y + k7*m0);
    float c2 = m1*m5 + d3.z*(k5*d3.x + k6*d3.y + k7*m2);
    float c3 = k7*m1*d3.z;

    // the surface is defined by equation c3*t^3 + c2*t^2 + c1*t + c0 = 0;
    // solve this equation analytically or numerically using the Newton's method
    // see "Numerical Recipes - The Art of Scientific Computing - 3rd Edition" for details

    if (ubo[0].m_pAccelStruct_m_preset.sdf_node_intersect == SDF_OCTREE_NODE_INTERSECT_ANALYTIC)
    {
      float x1 = 1000;
      float x2 = 1000;
      float x3 = 1000;
      uint power = 0;
      if (abs(c3) > 1e-5f)
      {
        power = 3;
        //it is a cubic equation, transform it to x^3 + a*x^2 + b*x + c = 0
        //use Vieta method to obtain 3 or 1 real roots
        float a = c2/c3;
        float b = c1/c3;
        float c = c0/c3;   

        float Q = (a*a - 3*b)/9;
        float R = (2*a*a*a - 9*a*b + 27*c)/54;
        float Q3 = Q*Q*Q;

        if (R*R < Q3) //equation has three real roots
        {
          float theta = acos(R/sqrt(Q3));
          x1 = -2*sqrt(Q)*cos(theta/3) - a/3;
          x2 = -2*sqrt(Q)*cos((theta+2*M_PI)/3) - a/3;
          x3 = -2*sqrt(Q)*cos((theta-2*M_PI)/3) - a/3;
        }
        else //equation has only one real roots
        {
          float A = -sign(R)*pow(abs(R) + sqrt(R*R - Q3), 1.0f/3.0f);
          float B = abs(A) > EPS ? Q/A : 0;
          x1 = A+B - a/3;
        }
      }
      else if (abs(c2) > 1e-7f)
      {
        power = 2;
        //it is a quadratic equation a*x^2 + b*x + c = 0
        float a = c2;
        float b = c1;
        float c = c0;

        float D = b*b - 4*a*c;
        if (D > 0)
        {
          float q = -0.5f*(b + sign(b)*sqrt(D));
          x1 = q/a;
          if (abs(q) > EPS)
            x2 = c/q; 
        }
      }
      else if (abs(c1) > EPS)
      {
        power = 1;
        //it is a linear equation c1*x + c0 = 0
        x1 = -c0/c1;
      }
      //else
      //no roots or inf roots, something's fucked up so just drop it

      x1 = x1 < 0 ? 1000 : x1;
      x2 = x2 < 0 ? 1000 : x2;
      x3 = x3 < 0 ? 1000 : x3;

      t = min(x1, min(x2, x3));
      hit = (t >= 0 && t <= qFar);
    }
    else if (ubo[0].m_pAccelStruct_m_preset.sdf_node_intersect == SDF_OCTREE_NODE_INTERSECT_NEWTON)
    {
      // our polynom is c3*t^3 + c2*t^2 + c1*t + c0 = 0;
      // it's derivative is  3*c3*t^2 + 2*c2*t + c1 = 0; 
      // find where it equals 0 to determine interval where the root is located
      // by solving a quadratic equation a*x^2 + b*x + c = 0
      float a = 3*c3;
      float b = 2*c2;
      float c = c1;

      float t0 = 0;
      float t1 = qFar;
      float t2 = qFar;
      float t3 = qFar;

      float D = b*b - 4*a*c;
      if (D >= 0)
      {
        float q = -0.5f*(b + sign(b)*sqrt(D));
        t1 = abs(a) > EPS ? q/a : t0;
        t2 = abs(q) > EPS ? c/q : qFar;

        float min_t = min(t1, t2);
        t2 = max(t1, t2);
        t1 = min_t;

        t1 = clamp(t1, t0, t3);
        t2 = clamp(t2, t0, t3);
      }
      
      //calculate sign of initial polynom at each critical point
      bool s0 = c0 > 0;
      bool s1 = (c0 + t1*(c1 + t1*(c2 + t1*c3))) > 0;
      bool s2 = (c0 + t2*(c1 + t2*(c2 + t2*c3))) > 0;
      bool s3 = (c0 + t3*(c1 + t3*(c2 + t3*c3))) > 0;

      //determine the range to apply Newton's method
      float nwt_min = t0;
      float nwt_max = t0;
      if (s0 != s1)
      {
        nwt_min = t0;
        nwt_max = t1;
      }
      else if (s1 != s2)
      {
        nwt_min = t1;
        nwt_max = t2;
      }
      else if (s2 != s3)
      {
        nwt_min = t2;
        nwt_max = t3;
      }

      float rtn = -100;

      if (nwt_min < nwt_max)
      {
        //perform Newton's method
        const uint max_iters = 10;
        uint nwt_iter = 0;
        rtn = 0.5f*(nwt_min + nwt_max);
        float f = 1000;
        while (nwt_iter < max_iters && abs(f) >= EPS)
        {
          f = c0 + rtn*(c1 + rtn*(c2 + rtn*c3));
          float df = c1 + rtn*(2*c2 + rtn*3*c3);
          float dx = f/(df + sign(df)*1e-9f);
          rtn -= dx;
          nwt_iter++;
        }
        t = rtn;
        hit = (t >= 0 && t <= qFar && abs(f) < EPS);
      }
      else
      {
        //no hit
        hit = false;
      }
    }
    else //if (m_preset.sdf_node_intersect == SDF_OCTREE_NODE_INTERSECT_IT)
    {
      const uint IT_max_iters = 256;
      const float k = 2;

      float e = 0.1f*qFar;
      float t_max = abs(c3) < EPS ? 1e6f : -c2/(3*c3);
      float df_max = 3*c3*t_max*t_max + 2*c2*t_max + c1;

      float dist = start_dist;
      float3 pp = start_q + t * ray_dir;

      while (t < qFar && dist > EPS && iter < IT_max_iters)
      {
        float df_1 = 3*c3*t*t + 2*c2*t + c1;
        float df_2 = 3*c3*(t+e)*(t+e) + 2*c2*(t+e) + c1;
        float L = (t_max > t && t_max < t + e) ? max(df_max, max(df_1, df_2)) : max(df_1, df_2);
        L = max(L, EPS);
        float s = min((dist*d_inv)/L, e);
        t += s;
        e = k*s;
        dist = start_sign*m_pAccelStruct_eval_dist_trilinear(values, start_q + t * ray_dir);
        pp = start_q + t * ray_dir;
        iter++;
      }
      hit = (dist <= EPS);
     
    }
  }

  float tReal = fNearFar.x + d * t;


#ifdef ON_CPU
  if (debug_cur_pixel)
  {
    printf("\n");
    printf("sdf type = %u\n", type);
    //printf("node bbox [(%f %f %f)-(%f %f %f)]\n", min_pos.x, min_pos.y, min_pos.z, max_pos.x, max_pos.y, max_pos.z);
    printf("sdf values %f %f %f %f %f %f %f %f\n", 
           values[0], values[1], values[2], values[3],
           values[4], values[5], values[6], values[7]);
    printf("t = %f in [%f, %f], tReal = %f in [%f %f]\n",t,qNear,qFar,tReal,fNearFar.x,fNearFar.y);
    printf("ray_dir = (%f %f %f)\n", ray_dir.x, ray_dir.y, ray_dir.z);
    //printf("ray_pos = (%f %f %f)\n", ray_pos.x, ray_pos.y, ray_pos.z);
    printf("\n");
  }
#endif

  if (t <= qFar && hit && tReal < pHit.t)
  {
    float3 norm = float3(0,0,1);
    if (m_pAccelStruct_need_normal())
    {
      if (early_hit && qNear > 1e-6f) //hit cutting plane, return its normal
      {
        norm = -1.0f*ubo[0].m_pAccelStruct_m_cuttingPlane.normal;
      }
      else if (early_hit && ubo[0].m_pAccelStruct_m_preset.sdf_node_intersect == SDF_OCTREE_NODE_INTERSECT_BBOX) //hit side of the voxel's bbox, return its normal
      {
        norm = float3(0,0,0);
        float3 v = float3(max(start_q.x, 1 - start_q.x),max(start_q.y, 1 - start_q.y),max(start_q.z, 1 - start_q.z));
        int dim = 0;
        if (v.y > v.x && v.y > v.z)
          dim = 1;
        else if (v.z > v.x && v.z > v.y)
          dim = 2;
        norm[dim] = start_q[dim] < 0.5f ? -1.0f : 1.0f;
      }
      else //hit SDF surface, calculate normal with finite differences
      {
        float3 p0 = start_q + t * ray_dir;
        const float h = 0.001f;
        float ddx = (m_pAccelStruct_eval_dist_trilinear(values, p0 + float3(h,0,0)) -
                    m_pAccelStruct_eval_dist_trilinear(values, p0 + float3(-h,0,0))) /
                    (2 * h);
        float ddy = (m_pAccelStruct_eval_dist_trilinear(values, p0 + float3(0,h,0)) -
                    m_pAccelStruct_eval_dist_trilinear(values, p0 + float3(0,-h,0))) /
                    (2 * h);
        float ddz = (m_pAccelStruct_eval_dist_trilinear(values, p0 + float3(0,0,h)) -
                    m_pAccelStruct_eval_dist_trilinear(values, p0 + float3(0,0,-h))) /
                    (2 * h);
        norm = start_sign*float3(ddx,ddy,ddz);
      }
      norm = normalize(matmul4x3(m_pAccelStruct_m_instanceData[instId].transformInvTransposed, norm));
    }
    
    float3 dp = start_q + t * ray_dir;
    float2 encoded_norm = encode_normal(norm);

    pHit.t = tReal;
    pHit.primId = primId;
    pHit.instId = instId;
    pHit.geomId = geomId | (type << SH_TYPE);
    pHit.coords[0] = 0;
    pHit.coords[1] = 0;
    pHit.coords[2] = encoded_norm.x;
    pHit.coords[3] = encoded_norm.y;

    if (ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_ST_ITERATIONS)
      pHit.primId = iter;
    
  #ifndef DISABLE_SDF_FRAME_OCTREE_TEX
    if (type == TYPE_SDF_FRAME_OCTREE_TEX)
    {
      pHit.coords[0] = (1-dp.x)*(1-dp.y)*(1-dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[0] + 
                        (1-dp.x)*(1-dp.y)*(  dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[2] + 
                        (1-dp.x)*(  dp.y)*(1-dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[4] + 
                        (1-dp.x)*(  dp.y)*(  dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[6] + 
                        (  dp.x)*(1-dp.y)*(1-dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[8] + 
                        (  dp.x)*(1-dp.y)*(  dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[10] + 
                        (  dp.x)*(  dp.y)*(1-dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[12] + 
                        (  dp.x)*(  dp.y)*(  dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[14];

      pHit.coords[1] = (1-dp.x)*(1-dp.y)*(1-dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[1] + 
                        (1-dp.x)*(1-dp.y)*(  dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[3] + 
                        (1-dp.x)*(  dp.y)*(1-dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[5] + 
                        (1-dp.x)*(  dp.y)*(  dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[7] + 
                        (  dp.x)*(1-dp.y)*(1-dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[9] + 
                        (  dp.x)*(1-dp.y)*(  dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[11] + 
                        (  dp.x)*(  dp.y)*(1-dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[13] + 
                        (  dp.x)*(  dp.y)*(  dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[15];
    }
  #endif

    // store interpolation quotients instead of normal and tc
    if ((type == TYPE_SDF_MULTI_OCTREE || type == TYPE_SDF_DAG || type == TYPE_SCOM2) &&
        (ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_CHANNEL || ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_CHANNEL_LAMBERT))
    {
      dp = clamp(float(0xFFFFu)*dp, 0.0f, float(0xFFFFu));
      int3 dp_u16 = int3(dp);
      pHit.coords[0] = asfloat((dp_u16.y << 16) | dp_u16.x);
      pHit.coords[1] = asfloat(dp_u16.z);
    }
  }
}

float m_pAccelStruct_load_distance_values(uint nodeId, float3 voxelPos, uint v_size, float sz_inv, in SdfSBSHeader header, inout float values[8]) {
  float vmin = 1e6f;
#ifndef DISABLE_SDF_SBS
  if (header.aux_data == SDF_SBS_NODE_LAYOUT_ID32F_IRGB32F ||
      header.aux_data == SDF_SBS_NODE_LAYOUT_ID32F_IRGB32F_IN)
  {
    uint v_off = m_pAccelStruct_m_SdfSBSNodes[nodeId].data_offset;
    //  (000) (001) (010) (011) (100) (101) (110) (111)

    for (int i = 0; i < 8; i++)
    {
      uint3 vPos = uint3(voxelPos) + uint3((i & 4) >> 2,(i & 2) >> 1,i & 1);
      uint vId = SBS_v_to_i(vPos.x, vPos.y, vPos.z, v_size, header.brick_pad);
      values[i] = m_pAccelStruct_m_SdfSBSDataF[m_pAccelStruct_m_SdfSBSData[v_off + vId]];
      vmin = min(vmin, values[i]);
    }
  }
  else
  {
    uint v_off = m_pAccelStruct_m_SdfSBSNodes[nodeId].data_offset;
    uint vals_per_int = 4 / header.bytes_per_value;
    uint bits = 8 * header.bytes_per_value;
    uint max_val = header.bytes_per_value == 4 ? 0xFFFFFFFF : ((1 << bits) - 1);
    float d_max = 1.73205081f * sz_inv;
    float mult = 2 * d_max / max_val;
    for (int i = 0; i < 8; i++)
    {
      uint3 vPos = uint3(voxelPos) + uint3((i & 4) >> 2,(i & 2) >> 1,i & 1);
      uint vId = SBS_v_to_i(vPos.x, vPos.y, vPos.z, v_size, header.brick_pad);;
      values[i] = -d_max + mult * ((m_pAccelStruct_m_SdfSBSData[v_off + vId / vals_per_int] >> (bits * (vId % vals_per_int))) & max_val);
      vmin = min(vmin, values[i]);
    }
  }
#endif
  return vmin;
}

float m_pAccelStruct_COctreeV3_LoadDistanceValuesLeafGrid(uint brickOffset, float3 voxelPos, uint v_size, float sz_inv, in COctreeV3Header header, uint transform_code, inout float values[8]) {
  float vmin = 1e6f;
#ifndef DISABLE_SDF_COCTREE_V3
  uint rotIdx = transform_code & header.rot_mask;
  const uint min_range_size_uints = 2;

  //<><><><min value and range><tex_coords><distances>
  uint off_3 = 0;                                   // min value and range
  uint off_4 = off_3 + min_range_size_uints;        // texture coordinates
  uint off_5 = off_4 + 8 * header.uv_size;          // distances

  uint vals_per_int = 32 / header.bits_per_value;
  uint bits = header.bits_per_value;
  uint max_val = header.bits_per_value == 32 ? 0xFFFFFFFF : ((1 << bits) - 1);

  float add_transform = header.sim_compression > 0 ? 2.0f*1.73205081f*sz_inv*(2.0f*(float(transform_code & header.add_mask) / float(header.add_mask)) - 1.0f) : 0.0f;

  float min_val = -float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_3 + 0]) / float(0xFFFFFFFFu) + add_transform;
  float range   =  (float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_3 + 1]) / float(0xFFFFFFFFu)) / max_val;

  uint vId0 = uint32_t(dot(m_pAccelStruct_m_SdfCompactOctreeRotModifiers[rotIdx], 
                           int4(int(voxelPos.x) + int(header.brick_pad),int(voxelPos.y) + int(header.brick_pad),int(voxelPos.z) + int(header.brick_pad),1)));
  uint32_t vId, dist;

  vId = vId0;
  dist = ((m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_5 + vId / vals_per_int] >> (bits * (vId % vals_per_int))) & max_val);
  values[0] = min_val + range * dist;
  vmin = min(values[0], vmin);

  vId = vId0 + m_pAccelStruct_m_SdfCompactOctreeRotModifiers[rotIdx].z;
  dist = ((m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_5 + vId / vals_per_int] >> (bits * (vId % vals_per_int))) & max_val);
  values[1] = min_val + range * dist;
  vmin = min(values[1], vmin);

  vId = vId0 + m_pAccelStruct_m_SdfCompactOctreeRotModifiers[rotIdx].y;
  dist = ((m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_5 + vId / vals_per_int] >> (bits * (vId % vals_per_int))) & max_val);
  values[2] = min_val + range * dist;
  vmin = min(values[2], vmin);

  vId = vId0 + m_pAccelStruct_m_SdfCompactOctreeRotModifiers[rotIdx].y + m_pAccelStruct_m_SdfCompactOctreeRotModifiers[rotIdx].z;
  dist = ((m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_5 + vId / vals_per_int] >> (bits * (vId % vals_per_int))) & max_val);
  values[3] = min_val + range * dist;
  vmin = min(values[3], vmin);

  vId = vId0 + m_pAccelStruct_m_SdfCompactOctreeRotModifiers[rotIdx].x;
  dist = ((m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_5 + vId / vals_per_int] >> (bits * (vId % vals_per_int))) & max_val);
  values[4] = min_val + range * dist;
  vmin = min(values[4], vmin);

  vId = vId0 + m_pAccelStruct_m_SdfCompactOctreeRotModifiers[rotIdx].x + m_pAccelStruct_m_SdfCompactOctreeRotModifiers[rotIdx].z;
  dist = ((m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_5 + vId / vals_per_int] >> (bits * (vId % vals_per_int))) & max_val);
  values[5] = min_val + range * dist;
  vmin = min(values[5], vmin);

  vId = vId0 + m_pAccelStruct_m_SdfCompactOctreeRotModifiers[rotIdx].x + m_pAccelStruct_m_SdfCompactOctreeRotModifiers[rotIdx].y; 
  dist = ((m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_5 + vId / vals_per_int] >> (bits * (vId % vals_per_int))) & max_val);
  values[6] = min_val + range * dist;
  vmin = min(values[6], vmin);

  vId = vId0 + m_pAccelStruct_m_SdfCompactOctreeRotModifiers[rotIdx].x + m_pAccelStruct_m_SdfCompactOctreeRotModifiers[rotIdx].y + m_pAccelStruct_m_SdfCompactOctreeRotModifiers[rotIdx].z;
  dist = ((m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_5 + vId / vals_per_int] >> (bits * (vId % vals_per_int))) & max_val);
  values[7] = min_val + range * dist;
  vmin = min(values[7], vmin);
#endif
  return vmin;
}

void m_pAccelStruct_IntersectSDFGrid(const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint a_start, uint a_count, inout CRT_Hit pHit) {
#ifndef DISABLE_SDF_GRID
  const uint type = m_pAccelStruct_m_geomData[geomId].type;
  const uint sdfId = m_pAccelStruct_m_geomData[geomId].offset.x;
  const uint primId = 0;
  const float3 min_pos = float3(-1,-1,-1);
  const float3 max_pos = float3(1,1,1);
  const float dir_len = length(ray_dir);
  const float3 dir = ray_dir / dir_len;

  const float EPS = 1e-5f;
  float2 tNear_tFar = m_pAccelStruct_box_intersects(min_pos, max_pos, ray_pos, dir);
  float t = max(tNear, tNear_tFar.x);
  float tFar = tNear_tFar.y;
  if (t > tFar)
    return;

  uint iter = 0;
  float d = m_pAccelStruct_eval_distance_sdf_grid(sdfId, ray_pos + t * dir);
  while (iter < 1000 && d > EPS && t < tFar)
  {
    t += d + EPS;
    d = m_pAccelStruct_eval_distance_sdf_grid(sdfId, ray_pos + t * dir);
    iter++;
  }

  if (d > EPS)
    return;

  float3 p0 = ray_pos + t * dir;
  if (t > tNear && t < pHit.t)
  {
    float3 norm = float3(0,0,1);
    if (m_pAccelStruct_need_normal())
    {
      const float h = 0.001f;
      float ddx = (m_pAccelStruct_eval_distance_sdf_grid(sdfId, p0 + float3(h,0,0)) -
                   m_pAccelStruct_eval_distance_sdf_grid(sdfId, p0 + float3(-h,0,0))) /
                  (2 * h);
      float ddy = (m_pAccelStruct_eval_distance_sdf_grid(sdfId, p0 + float3(0,h,0)) -
                   m_pAccelStruct_eval_distance_sdf_grid(sdfId, p0 + float3(0,-h,0))) /
                  (2 * h);
      float ddz = (m_pAccelStruct_eval_distance_sdf_grid(sdfId, p0 + float3(0,0,h)) -
                   m_pAccelStruct_eval_distance_sdf_grid(sdfId, p0 + float3(0,0,-h))) /
                  (2 * h);

      norm = normalize(float3(ddx,ddy,ddz));
    }
    float3 n = normalize(matmul4x3(m_pAccelStruct_m_instanceData[instId].transformInvTransposed, norm));
    float2 encoded_norm = encode_normal(n);

    pHit.t = t;
    pHit.primId = primId;
    pHit.instId = instId;
    pHit.geomId = geomId | (type << SH_TYPE);
    pHit.coords[0] = 0;
    pHit.coords[1] = 0;
    pHit.coords[2] = encoded_norm.x;
    pHit.coords[3] = encoded_norm.y;

    if (ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_ST_ITERATIONS)
      pHit.primId = iter;
  }
#endif
}

void m_pAccelStruct_COctreeIntersectV3(uint model_type, const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint bvhNodeId, uint a_count, inout CRT_Hit pHit) {
#ifndef DISABLE_SDF_COCTREE_V3
  float3 pos_ray_pos = ray_pos;
  float3 pos_ray_dir = ray_dir;

  //assume octree is box [-1,1]^3
  uint a = 0;
  if (ray_dir.x < 0)
  {
    pos_ray_pos.x *= -1;
    pos_ray_dir.x *= -1;
    a |= 4;
  }

  if (ray_dir.y < 0)
  {
    pos_ray_pos.y *= -1;
    pos_ray_dir.y *= -1;
    a |= 2;
  }

  if (ray_dir.z < 0)
  {
    pos_ray_pos.z *= -1;
    pos_ray_dir.z *= -1;
    a |= 1;
  }

  float3 min_pos = m_pAccelStruct_m_origNodes[bvhNodeId].boxMin;
  float3 max_pos = m_pAccelStruct_m_origNodes[bvhNodeId].boxMax;
  float3 center = 0.5f * (min_pos + max_pos);

  uint start_sz = uint32_t(2.0f / (max_pos.x - min_pos.x));
  uint3 start_p_orig = uint3(float(start_sz)*0.5f*(min_pos - float3(-1,-1,-1)));
  uint p_mask = start_sz - 1;
  uint3 start_p = uint3(((a & 4) > 0) ? (~start_p_orig.x & p_mask) : start_p_orig.x,((a & 2) > 0) ? (~start_p_orig.y & p_mask) : start_p_orig.y,((a & 1) > 0) ? (~start_p_orig.z & p_mask) : start_p_orig.z);

  uint startNodeOffset = m_pAccelStruct_m_origNodes[bvhNodeId].leftOffset;

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
  uint32_t level_sz;
  float d;
  const float old_t = pHit.t;

  stack[top].nodeId = startNodeOffset;
  stack[top].info = 0;
  stack[top].p_size = uint2((start_p.x << 16) | start_p.y,(start_p.z << 16) | start_sz);

    while (top >= 0)
    {
      level_sz = stack[top].p_size.y & 0xFFFF;
      p = uint3(stack[top].p_size.x >> 16,stack[top].p_size.x & 0xFFFF,stack[top].p_size.y >> 16);
      d = 1.0f/float(level_sz);
      p_f = float3(p);
      t0 = _t0 + d*p_f * _l;
      t1 = _t0 + d*(p_f + 1) * _l;

      float t_out = min(min(t1.x, t1.y), t1.z);
      if (t_out < tNear)
      {
        top--;
        continue;
      }

      bool is_leaf = (stack[top].info & COCTREE_LEAF_TYPE_MASK) != COCTREE_LEAF_TYPE_NOT_A_LEAF;
      if (ubo[0].m_pAccelStruct_coctree_v3_header.lods > 0 && !is_leaf)
      {
        float t = ubo[0].m_pAccelStruct_m_preset.fixed_lod > 0 ? 1.0f : max(t0.x, max(t0.y, t0.z));
        float target_lod_size = (pow(2.0f, ubo[0].m_pAccelStruct_m_preset.level_of_detail) - 0.01f)/t;
        if (float(level_sz*ubo[0].m_pAccelStruct_coctree_v3_header.brick_size) > target_lod_size)
        {
          uint lodLeafId = m_pAccelStruct_m_SdfCompactOctreeV3Data[stack[top].nodeId + 1];
          uint type = (m_pAccelStruct_m_SdfCompactOctreeV3Data[stack[top].nodeId + 0] >> COCTREE_LOD_LEAF_TYPE_SHIFT) & COCTREE_LEAF_TYPE_MASK;
          uint infoOffset = stack[top].nodeId + 1 + ubo[0].m_pAccelStruct_coctree_v3_header.trans_off;
          stack[top].nodeId = lodLeafId;
          stack[top].info = (m_pAccelStruct_m_SdfCompactOctreeV3Data[infoOffset] << COCTREE_LEAF_TYPE_BITS) | type;
          is_leaf = true;

          //we should stop on this LOD level, but this node is empty, so just skip it
          if (type == COCTREE_LEAF_TYPE_NOT_A_LEAF)
          {
            top--;
            continue; //
          }
        }
      }

      if (is_leaf)
      {
        p_mask = level_sz - 1;
        uint3 real_p = uint3(((a & 4) > 0) ? (~p.x & p_mask) : p.x,((a & 2) > 0) ? (~p.y & p_mask) : p.y,((a & 1) > 0) ? (~p.z & p_mask) : p.z);

        uint leafOffset = (stack[top].nodeId & ubo[0].m_pAccelStruct_coctree_v3_header.idx_mask) >> ubo[0].m_pAccelStruct_coctree_v3_header.idx_sh;
        m_pAccelStruct_COctreeV3_BrickIntersect(TYPE_SDF_FRAME_OCTREE, ray_pos, ray_dir, tNear, instId, geomId, leafOffset, float3(real_p), float(level_sz), stack[top].info, pHit);

        if (pHit.t < old_t)
        {
          top = -1;
          if (ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_LOD)
            pHit.primId = uint32_t(log2(float(level_sz*ubo[0].m_pAccelStruct_coctree_v3_header.brick_size)));
        }
        else
          top--;
      }
      else
      { 
        buf_top = 0;
        tm = 0.5f*(t0 + t1);

        currNode = first_node(t0, tm);
        do
        {
          //0-7 bits are child_is_active flags, next 8-15 bits are child_is_leaf flags
          uint childrenInfo = m_pAccelStruct_m_SdfCompactOctreeV3Data[stack[top].nodeId + 0];
          uint childNode = currNode ^ a; //child node number, from 0 to 8

          // if child is active
          if ((childrenInfo & (1u << childNode)) > 0)
          {
            uint localChildPos = ubo[0].m_pAccelStruct_coctree_v3_header.lods + bitCount(childrenInfo & ((1u << childNode) - 1));
            uint childPos = 1 + ubo[0].m_pAccelStruct_coctree_v3_header.uints_per_child_info*localChildPos;
            uint childOffset = m_pAccelStruct_m_SdfCompactOctreeV3Data[stack[top].nodeId + childPos];
            uint childType = (childrenInfo >> (8 + COCTREE_LEAF_TYPE_BITS*childNode)) & COCTREE_LEAF_TYPE_MASK;
            uint childInfoOffset = stack[top].nodeId + childPos + ubo[0].m_pAccelStruct_coctree_v3_header.trans_off;
            uint childInfo = (m_pAccelStruct_m_SdfCompactOctreeV3Data[childInfoOffset] << COCTREE_LEAF_TYPE_BITS) | childType;

            tmp_buf[buf_top].nodeId = childOffset;
            tmp_buf[buf_top].info = childInfo;
            tmp_buf[buf_top].p_size = (stack[top].p_size << 1) | uint2(((currNode & 4) << (16-2)) | ((currNode & 2) >> 1),(currNode & 1) << 16);
            buf_top++;
          }
          currNode = new_node(((currNode & 4) > 0) ? t1.x : tm.x, nn_indices[currNode].x,
                              ((currNode & 2) > 0) ? t1.y : tm.y, nn_indices[currNode].y,
                              ((currNode & 1) > 0) ? t1.z : tm.z, nn_indices[currNode].z);
        } while (currNode<8);

        for (int i = 0; i < buf_top; i++)
        {
          stack[top+i] = tmp_buf[buf_top-i-1];
        }
        top += buf_top - 1;
      }
    }
#endif
}

void m_pAccelStruct_OctreeIntersect(uint type, const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint a_start, uint a_count, inout CRT_Hit pHit) {
#ifndef DISABLE_SDF_MULTI_OCTREE
  float3 pos_ray_pos = ray_pos;
  float3 pos_ray_dir = ray_dir;

  //assume octree is box [-1,1]^3
  uint a = 0;
  if (ray_dir.x < 0)
  {
    pos_ray_pos.x *= -1;
    pos_ray_dir.x *= -1;
    a |= 4;
  }

  if (ray_dir.y < 0)
  {
    pos_ray_pos.y *= -1;
    pos_ray_dir.y *= -1;
    a |= 2;
  }

  if (ray_dir.z < 0)
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
  float old_t = pHit.t;

  stack[top].nodeId = m_pAccelStruct_m_geomData[geomId].offset.x;
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

    bool is_leaf = stack[top].info > 0;
    if (!is_leaf)
    {
      float t = ubo[0].m_pAccelStruct_m_preset.fixed_lod > 0 ? 1.0f : max(t0.x, max(t0.y, t0.z));
      float target_lod_size = (pow(2.0f, ubo[0].m_pAccelStruct_m_preset.level_of_detail) - 0.01f) / t;
      if (float(level_sz) > target_lod_size)
      {
        stack[top].info = m_pAccelStruct_m_SdfMultiOctreeNodes[stack[top].nodeId].data_offset;
        is_leaf = true;

        // we should stop on this LOD level, but this node is empty, so just skip it
        if (m_pAccelStruct_m_SdfMultiOctreeNodes[stack[top].nodeId].voxel_count == 0)
        {
          top--;
          continue; //
        }
      }
    }

    if (is_leaf)
    {
      float tmin = max(t0.x, max(t0.y, t0.z));
      float tmax = min(t1.x, min(t1.y, t1.z));

      {
        uint p_mask = level_sz - 1;
        uint3 real_p = uint3(((a & 4) > 0) ? (~p.x & p_mask) : p.x,((a & 2) > 0) ? (~p.y & p_mask) : p.y,((a & 1) > 0) ? (~p.z & p_mask) : p.z);

        nodeId = stack[top].nodeId;
        float sz = 0.5f * float(level_sz);
        d = 1.0f / sz;
        float3 min_pos = float3(-1,-1,-1) + d * float3(real_p.x,real_p.y,real_p.z);

        fNearFar = float2(tmin,tmax);
        float3 start_pos = ray_pos + fNearFar.x * ray_dir;
        start_q = sz * (start_pos - min_pos);
        qNear = max(0.0f, sz * (tNear - fNearFar.x));
        qFar = sz * (fNearFar.y - fNearFar.x);
      }

#ifdef ON_CPU
      if (debug_cur_pixel)
      {
        printf("qNear, qFar = [%f %f]\n", qNear, qFar);
      }
#endif

      for (int voxel_id = 0; voxel_id < m_pAccelStruct_m_SdfMultiOctreeNodes[stack[top].nodeId].voxel_count; voxel_id++)
      {
        for (int i = 0; i < 8; i++)
          values[i] = m_pAccelStruct_m_SdfMultiOctreeValues[stack[top].info + 8*voxel_id + i];
        
        bool is_surface_node = (m_pAccelStruct_m_SdfMultiOctreeNodes[stack[top].nodeId].flags & MULTI_OCTREE_FLAG_SURFACE) > 0;
        uint primId = nodeId - m_pAccelStruct_m_geomData[geomId].offset.x; //local primitive id in this geometry
        m_pAccelStruct_LocalSurfaceIntersection(TYPE_SDF_MULTI_OCTREE, ray_dir, instId, geomId, values, nodeId, primId, d, qNear, qFar, fNearFar, start_q, is_surface_node, pHit);                                                                                          
      }

      if (ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_MO_VOXEL_COUNT)
      {
        if (pHit.t < old_t)
          pHit.primId = m_pAccelStruct_m_SdfMultiOctreeNodes[stack[top].nodeId].voxel_count;
      }
      else if (ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_MO_FLAGS)
      {
        if (pHit.t < old_t)
          pHit.primId = m_pAccelStruct_m_SdfMultiOctreeNodes[stack[top].nodeId].flags;
      }

      if (pHit.t < old_t)
        top = -1;
      else
        top--;
    }
    else
    {
      buf_top = 0;
      tm = 0.5f * (t0 + t1);

      currNode = first_node(t0, tm);
      do
      {
        uint childrenOff = m_pAccelStruct_m_SdfMultiOctreeNodes[stack[top].nodeId].children_offset;
        uint childNode = currNode ^ a; // child node number, from 0 to 8
        uint childVoxelCount = m_pAccelStruct_m_SdfMultiOctreeNodes[childrenOff + childNode].voxel_count;
        bool childIsNode = m_pAccelStruct_m_SdfMultiOctreeNodes[childrenOff + childNode].children_offset > 0;

        // if child is active
        if (childVoxelCount > 0 || childIsNode)
        {
          tmp_buf[buf_top].nodeId = childrenOff + childNode;
          tmp_buf[buf_top].info = childIsNode ? 0 : m_pAccelStruct_m_SdfMultiOctreeNodes[childrenOff + childNode].data_offset; // > 0 is child is leaf
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
#endif
}

void m_pAccelStruct_RayNodeIntersection(uint type, const float3 ray_pos, const float3 ray_dir, float tNear, uint geomId, uint bvhNodeId, inout float values[8], inout uint primId, inout uint nodeId, inout float d, inout float qNear, inout float qFar, inout float2 fNearFar, inout float3 start_q) {
  float3 min_pos, max_pos;

  if (type == TYPE_SDF_FRAME_OCTREE)
  {
#ifndef DISABLE_SDF_FRAME_OCTREE
    uint sdfId = m_pAccelStruct_m_geomData[geomId].offset.x;
    primId = m_pAccelStruct_m_origNodes[bvhNodeId].leftOffset;
    nodeId = primId + m_pAccelStruct_m_SdfFrameOctreeRoots[sdfId];
    min_pos = m_pAccelStruct_m_origNodes[bvhNodeId].boxMin;
    max_pos = m_pAccelStruct_m_origNodes[bvhNodeId].boxMax;
    float3 size = max_pos - min_pos;

    for (int i=0;i<8;i++)
      values[i] = m_pAccelStruct_m_SdfFrameOctreeNodes[nodeId].values[i];

    fNearFar = RayBoxIntersection2(ray_pos, SafeInverse(ray_dir), min_pos, max_pos);
    float3 start_pos = ray_pos + fNearFar.x*ray_dir;
    d = max(size.x, max(size.y, size.z));
    start_q = (start_pos - min_pos) / d;
    qFar = (fNearFar.y - fNearFar.x) / d;
    qNear = tNear > fNearFar.x ? (tNear - fNearFar.x) / d : 0.0f;
#endif
  }
  else if (type == TYPE_SDF_FRAME_OCTREE_TEX)
  {
#ifndef DISABLE_SDF_FRAME_OCTREE_TEX
    uint sdfId = m_pAccelStruct_m_geomData[geomId].offset.x;
    primId = m_pAccelStruct_m_origNodes[bvhNodeId].leftOffset;
    nodeId = primId + m_pAccelStruct_m_SdfFrameOctreeTexRoots[sdfId];
    min_pos = m_pAccelStruct_m_origNodes[bvhNodeId].boxMin;
    max_pos = m_pAccelStruct_m_origNodes[bvhNodeId].boxMax;
    float3 size = max_pos - min_pos;

    for (int i=0;i<8;i++)
      values[i] = m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].values[i];

    fNearFar = RayBoxIntersection2(ray_pos, SafeInverse(ray_dir), min_pos, max_pos);
    float3 start_pos = ray_pos + fNearFar.x*ray_dir;
    d = max(size.x, max(size.y, size.z));
    start_q = (start_pos - min_pos) / d;
    qFar = (fNearFar.y - fNearFar.x) / d;
    qNear = tNear > fNearFar.x ? (tNear - fNearFar.x) / d : 0.0f;
#endif
  }
  else if (type == TYPE_SDF_SVS)
  {
#ifndef DISABLE_SDF_SVS
    uint sdfId = m_pAccelStruct_m_geomData[geomId].offset.x;
    primId = bvhNodeId;
    nodeId = primId + m_pAccelStruct_m_SdfSVSRoots[sdfId];

    float px = m_pAccelStruct_m_SdfSVSNodes[nodeId].pos_xy >> 16;
    float py = m_pAccelStruct_m_SdfSVSNodes[nodeId].pos_xy & 0x0000FFFF;
    float pz = m_pAccelStruct_m_SdfSVSNodes[nodeId].pos_z_lod_size >> 16;
    float sz = m_pAccelStruct_m_SdfSVSNodes[nodeId].pos_z_lod_size & 0x0000FFFF;
    float d_max = 2*1.73205081f/sz;

    min_pos = float3(-1,-1,-1) + 2.0f*float3(px,py,pz)/sz;
    max_pos = min_pos + 2.0f*float3(1,1,1)/sz;
    float3 size = max_pos - min_pos;

    for (int i=0;i<8;i++)
      values[i] = -d_max + 2.0f*d_max*(1.0f/255.0f)*((m_pAccelStruct_m_SdfSVSNodes[nodeId].values[i/4] >> (8*(i%4))) & 0xFF);

    fNearFar = RayBoxIntersection2(ray_pos, SafeInverse(ray_dir), min_pos, max_pos);
    float3 start_pos = ray_pos + fNearFar.x*ray_dir;
    d = max(size.x, max(size.y, size.z));
    start_q = (start_pos - min_pos) / d;
    qFar = (fNearFar.y - fNearFar.x) / d;
    qNear = tNear > fNearFar.x ? (tNear - fNearFar.x) / d : 0.0f;
#endif
  }
}

void m_pAccelStruct_DAGIntersect(uint type, const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint a_start, uint a_count, inout CRT_Hit pHit) {
#ifndef DISABLE_SDF_DAG
  //It is a naive version that iterates all child nodes, instead of traversing it in octree-like fashion
  //but DAG is a mostly for debugging similarity compression, so it does not really matter
  //Still, we should optimize it if performance testing of octree/64-tree is considered

  const float3 ray_dir_inv = SafeInverse(ray_dir);
  OTStackElement stack[32];

  int top = 0;
  int buf_top = 0;
  uint3 p;
  float3 p_f, t0, t1, tm;
  int currNode;
  uint32_t nodeId;
  uint32_t level_sz;
  float d, level_sz_f, sz_inv, qNear, qFar;
  float2 fNearFar;
  float3 start_q;
  float values[8];
  float old_t = pHit.t;
  SdfDAGHeader header = m_pAccelStruct_m_SdfDAGHeaders[m_pAccelStruct_m_geomData[geomId].offset.y];

  stack[top].nodeId = m_pAccelStruct_m_geomData[geomId].offset.x;
  stack[top].info = 0; //info here is cumulative rotation id of the node
  stack[top].p_size = uint2(0,1);

  while (top >= 0)
  {
    OTStackElement curElem = stack[top];
    top--;

    SdfDAGNode node = m_pAccelStruct_m_SdfDAGNodes[curElem.nodeId];
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
  if (debug_cur_pixel)
  {
    printf("[%d %d %d %d] node %d rot %d\n", p.x, p.y, p.z, level_sz, curElem.nodeId, curElem.info);
  }
#endif
    if (brick_fNearFar.x >= brick_fNearFar.y)
    {
      continue;
    }

    bool is_leaf = node.children_edges_offset == 0;
    if (!is_leaf)
    {
      float t = ubo[0].m_pAccelStruct_m_preset.fixed_lod > 0 ? 1.0f : brick_fNearFar.x;
      float target_lod_size = (pow(2.0f, ubo[0].m_pAccelStruct_m_preset.level_of_detail) - 0.01f) / t;
      is_leaf = float(level_sz) > target_lod_size;
    }

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

        while (brick_fNearFar.x < brick_fNearFar.y && pHit.t == old_t)
        {
          uint v_size = header.brick_size + 2*header.brick_pad + 1;
          float3 hit_pos = ray_pos + brick_fNearFar.x*ray_dir;
          float3 local_pos = (hit_pos - brick_min_pos) * (0.5f*sz*header.brick_size);
          float3 voxelPos = floor(clamp(local_pos, 1e-6f, header.brick_size-1e-6f));

          float3 min_pos = brick_min_pos + d*voxelPos;
          float3 max_pos = min_pos + d*float3(1,1,1);
          float3 size = max_pos - min_pos;

          uint s_id = 0;
          while (s_id < surface_count && pHit.t == old_t)
          {
            uint offset = m_pAccelStruct_m_SdfDAGDataEdges[node.data_edges_offset + s_id].data_offset;
            int4 voxelPosI = int4(int(voxelPos.x),int(voxelPos.y),int(voxelPos.z),1);
            uint rotIdx = m_pAccelStruct_m_RotAddTable[curElem.info*ubo[0].m_pAccelStruct_m_SdfDAGTransformCount + 
                                            m_pAccelStruct_m_SdfDAGDataEdges[node.data_edges_offset + s_id].rotation_id];
            float add = m_pAccelStruct_m_SdfDAGDataEdges[node.data_edges_offset + s_id].add;
            float vmin = 1.0f;
            float vmax = is_surface ? -1.0f : 1.0f;
            for (int i=0;i<8;i++)
            {
              int4 pI = voxelPosI + int4((i & 4) >> 2,(i & 2) >> 1,i & 1,0);
              int idx = pI.x*int(v_size*v_size) + pI.y*int(v_size) + pI.z;
              uint vId0 = m_pAccelStruct_m_SdfDAGTranspositions[rotIdx*v_size*v_size*v_size + idx];
              float val = m_pAccelStruct_m_SdfDAGDistances[offset + vId0] + add;
              values[i] = val - ubo[0].m_pAccelStruct_m_preset.compact_sdf_eps*d;
              //printf("vId0 = %d, offset = %d add = %f, val = %f\n", vId0, offset, add, values[i]);
              vmin = min(vmin, val);
              vmax = max(vmax, val);
            }

            fNearFar = RayBoxIntersection2(ray_pos, SafeInverse(ray_dir), min_pos, max_pos);
            if (fNearFar.x < fNearFar.y && vmin <= 0.0f && vmax >= 0.0f)    
            {
              float3 start_pos = ray_pos + fNearFar.x*ray_dir;
              float vox_sz = (0.5f*sz*header.brick_size);
              start_q = vox_sz * (start_pos - min_pos);
              qNear = max(0.0f, vox_sz * (tNear - fNearFar.x));
              qFar = vox_sz * (fNearFar.y - fNearFar.x);
          
              m_pAccelStruct_LocalSurfaceIntersection(type, ray_dir, instId, geomId, values, curElem.nodeId, primId, d, qNear, qFar, fNearFar, start_q, is_surface, pHit); /*out*/
            }

            // if we need channel data, we must use rotations, but we cannot do it in resolve pass (no rotation info there)
            // however, we can transform interpolation quotients here that will lead to the same result
            if (pHit.t != old_t && 
               (ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_CHANNEL || ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_CHANNEL_LAMBERT))
            {
              uint channelRotId = m_pAccelStruct_m_RotAddTable[curElem.info*ubo[0].m_pAccelStruct_m_SdfDAGTransformCount + 
                                                    node.channels_edge.rotation_id];
              float t = (pHit.t - fNearFar.x) / d;
              float3 dp = start_q + t * ray_dir;
              float3 dp_rot = to_float3(m_pAccelStruct_m_SdfCompactOctreeRotTransforms[channelRotId] * to_float4(dp, 1.0f));
              dp_rot = clamp(float(0xFFFFu)*dp_rot, 0.0f, float(0xFFFFu));
              int3 dp_u16 = int3(dp_rot);
              pHit.coords[0] = asfloat((dp_u16.y << 16) | dp_u16.x);
              pHit.coords[1] = asfloat(dp_u16.z);
            }

            s_id++;
          }
          brick_fNearFar.x += max(0.0f, fNearFar.y-brick_fNearFar.x) + 1e-5f;

          if (pHit.t < old_t)
            top = -1;
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
        int idx = int(voxelPos.x)*v_sz*v_sz + int(voxelPos.y)*v_sz + int(voxelPos.z);
        uint voxId = m_pAccelStruct_m_SdfDAGTranspositions[ubo[0].m_pAccelStruct_m_SdfDAGBrickTranspositionOffset + curElem.info*v_sz*v_sz*v_sz + idx];

        float3 min_pos = brick_min_pos + d*voxelPos;
        float3 max_pos = min_pos + d*float3(1,1,1);
        float3 size = max_pos - min_pos;

        fNearFar = RayBoxIntersection2(neg_pos, neg_dir_inv, min_pos, max_pos);
        SdfDAGChildEdge edge = m_pAccelStruct_m_SdfDAGChildEdges[node.children_edges_offset + voxId];

        //if we itersected with this node bbox and this node is valid
        if (tNear < fNearFar.x && edge.child_offset > 0)    
        {
          SdfDAGNode child_node = m_pAccelStruct_m_SdfDAGNodes[edge.child_offset];
          top++;
          stack[top].nodeId = edge.child_offset;
          stack[top].info = m_pAccelStruct_m_RotAddTable[curElem.info*ubo[0].m_pAccelStruct_m_SdfDAGTransformCount + edge.rotation_id];
          stack[top].p_size = (curElem.p_size*v_sz) + uint2((uint(voxelPos.x) << 16) | uint(voxelPos.y),(uint(voxelPos.z) << 16) | 0);
        }

        neg_brick_fNearFar.x += max(0.0f, fNearFar.y-neg_brick_fNearFar.x) + 0.01f*d;
      }
    }
  }
#endif
}

void m_pAccelStruct_SCom2IntersectFast(uint type, const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint a_start, uint a_count, inout CRT_Hit pHit) {
#ifndef DISABLE_SCOM2
  float3 pos_ray_pos = ray_pos;
  float3 pos_ray_dir = ray_dir;

  // assume octree is box [-1,1]^3
  uint a = 0;
  if (ray_dir.x < 0)
  {
    pos_ray_pos.x *= -1;
    pos_ray_dir.x *= -1;
    a |= 4;
  }

  if (ray_dir.y < 0)
  {
    pos_ray_pos.y *= -1;
    pos_ray_dir.y *= -1;
    a |= 2;
  }

  if (ray_dir.z < 0)
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
  float3 tm = t0 + 0.5f * _l;

  ExtStackElement cur;
  SCom2Header header = m_pAccelStruct_m_SCom2Headers[m_pAccelStruct_m_geomData[geomId].offset.y];
  {
    uint nodeData0 = m_pAccelStruct_m_SCom2Data[header.root_node_off];
    NodeHeadUnpacked node = unpack_node_head(header, nodeData0, nodeData0);
    uint currNode = first_node(t0, tm);

    cur.linksOff = header.root_node_off + header.links_offset + ((node.base_type) == SCOM2_CHILD_EMPTY ? 0 : 1);
    cur.info = (0 << 24) | (node.children_types << 8) | currNode;
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
    uint3 voxelPos = uint3((rawChildNode & 4) >> 2,(rawChildNode & 2) >> 1,rawChildNode & 1);
    uint childN = uint32_t(dot(m_pAccelStruct_m_SdfCompactOctreeRotModifiers[2 * ROT_COUNT + cur.transform], // child node number, from 0 to 8
                                   int4((int)voxelPos.x,(int)voxelPos.y,(int)voxelPos.z,1)));
    uint activeChildrenMask = (((cur.info & 0x00AAAA00u) >> 1) | (cur.info & 0x00555500u)) >> 8;
    uint childLink = cur.linksOff + bitCount(activeChildrenMask & ((1u << (2 * childN)) - 1));
    uint childHasData = activeChildrenMask & (1u << (2 * childN));
    uint childIsLeaf = cur.info & (1u << (8 + 2 * childN));
    uint rotId = cur.info >> 24;

    d = 1.0f / float(cur.p_size.y & 0xFFFF);
    float3 pf2 = float3(float(cur.p_size.x >> 16) + (((currNode & 4) > 0) ? 1.0f : 0.5f),float(cur.p_size.x & 0xFFFF) + (((currNode & 2) > 0) ? 1.0f : 0.5f),float(cur.p_size.y >> 16) + (((currNode & 1) > 0) ? 1.0f : 0.5f));
    float3 tl = _t0 + d * pf2 * _l;

    // this child is empty, go to the next child of this node
    if (min(min(tl.x, tl.y), tl.z) < tNear || childHasData == 0)
    {
      uint nextChildOfCurNode = new_node(currNode, tl.x, tl.y, tl.z);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.info = nextChildOfCurNode | (cur.info & 0xFFFFFF00u);
    }
    else if (childIsLeaf > 0) // we intersected with leaf, fill the hit data and return
    {
      uint level_sz = 2 * (cur.p_size.y & 0xFFFF);
      uint3 p = 2 * uint3(cur.p_size.x >> 16,cur.p_size.x & 0xFFFF,cur.p_size.y >> 16) +
                uint3((currNode & 4) >> 2,(currNode & 2) >> 1,currNode & 1);
      d = 2.0f / float(level_sz);
      float3 p_f = float3(p);
      t0 = _t0 + 0.5f * d * p_f * _l;
      float3 t1 = _t0 + 0.5f * d * (p_f + 1) * _l;

      float max_val = get_max_sdf_val(float(level_sz));

      bool childIsSurface = (cur.info & (1u << (8 + 2 * childN + 1))) > 0;
      uint linkData0 = m_pAccelStruct_m_SCom2Data[childLink];
      SdfDAGDataEdge de = unpack_data_edge(header, max_val, linkData0, linkData0);
      uint offset = header.bricks_arr_offset + header.bricks_step * de.data_offset;
      uint rotIdx = m_pAccelStruct_m_RotAddTable[rotId * ROT_COUNT + de.rotation_id];
      float add = de.add;

      float3 vox_tm = 0.5f * (t0 + t1);
      uint vox_currNode = first_node(t0, vox_tm);

      while (vox_currNode < 8)
      {
        float3 vox_tl = float3((vox_currNode & 4) > 0 ? t1.x : vox_tm.x,(vox_currNode & 2) > 0 ? t1.y : vox_tm.y,(vox_currNode & 1) > 0 ? t1.z : vox_tm.z);

        uint voxId = vox_currNode ^ a;

        float vmin = 1.0f;
        float vmax = childIsSurface ? -1.0f : 1.0f;
        float values[8];
        for (int i = 0; i < 8; i++)
        {
          int4 pI = int4((voxId & 4) >> 2,(voxId & 2) >> 1,voxId & 1,0) + int4((i & 4) >> 2,(i & 2) >> 1,i & 1,1);
          uint vId0 = uint32_t(dot(m_pAccelStruct_m_SdfCompactOctreeRotModifiers[rotIdx], pI));
          uint p_val = m_pAccelStruct_m_SCom2Data[offset + vId0 / header.values_per_uint];
          uint p_off = (vId0 % header.values_per_uint) * header.bits_per_value;
          float val = max_val * (2.0f * ((p_val >> p_off) & header.value_mask) / float(header.value_mask) - 1) + add;
          values[i] = val - 0.5f * ubo[0].m_pAccelStruct_m_preset.compact_sdf_eps * d;
          vmin = min(vmin, val);
          vmax = max(vmax, val);
        }

        if (vmin <= 0.0f && vmax >= 0.0f)
        {
          uint p_mask = level_sz - 1;
          uint3 real_p = uint3(((a & 4) > 0) ? (~p.x & p_mask) : p.x,((a & 2) > 0) ? (~p.y & p_mask) : p.y,((a & 1) > 0) ? (~p.z & p_mask) : p.z);

          float3 vox_t0 = float3((vox_currNode & 4) > 0 ? vox_tm.x : t0.x,(vox_currNode & 2) > 0 ? vox_tm.y : t0.y,(vox_currNode & 1) > 0 ? vox_tm.z : t0.z);
          float3 vox_t1 = float3((vox_currNode & 4) > 0 ? t1.x : vox_tm.x,(vox_currNode & 2) > 0 ? t1.y : vox_tm.y,(vox_currNode & 1) > 0 ? t1.z : vox_tm.z);

          float tmin = max(vox_t0.x, max(vox_t0.y, vox_t0.z));
          float tmax = min(vox_t1.x, min(vox_t1.y, vox_t1.z));
          float3 start_pos = ray_pos + tmin * ray_dir;
          float3 min_pos = float3(-1,-1,-1) + d * float3(real_p.x,real_p.y,real_p.z) + 0.5f*d*float3((voxId & 4)>>2,(voxId & 2)>>1,voxId & 1);

          float vox_sz = float(level_sz);
          float3 start_q = vox_sz * (start_pos - min_pos);
          const float qNear = max(0.0f, vox_sz * (tNear - tmin));
          const float qFar = vox_sz * (tmax - tmin);

          //potentially faster, but somehow consumes more registers...
          /*
          const float d_inv = 1.0f / d;
          float t = qNear;
          float dist = eval_dist_trilinear(values, start_q + t * ray_dir);
          float3 pp0 = start_q + t * ray_dir;

          uint32_t iter = 0;
          while (t < qFar && dist > 1e-6f && iter < 256)
          {
            t += dist * d_inv;

            dist = eval_dist_trilinear(values, start_q + t * ray_dir);
            
            float3 pp = start_q + t * ray_dir;
            iter++;
          }

          if (dist <= 1e-6f)
          {
            float3 p0 = start_q + t * ray_dir;
            float3 norm = normalize(eval_dist_trilinear_diff(values, p0));
            float2 encoded_norm = encode_normal(norm);

            pHit->t = tmin + d * t;
            pHit->primId = childLink;
            pHit->instId = instId;
            pHit->geomId = geomId | (type << SH_TYPE);
            pHit->coords[0] = 0;
            pHit->coords[1] = 0;
            pHit->coords[2] = encoded_norm.x;
            pHit->coords[3] = encoded_norm.y;

            return;
          }
          */

          float last_t = pHit.t;
          m_pAccelStruct_LocalSurfaceIntersectionFast(type, ray_dir, instId, geomId, values, childLink, childLink, 0.5f*d, qNear, qFar, float2(tmin,tmax), start_q, childIsSurface, pHit);                                                                                                                    /*out*/
          if (pHit.t < last_t)
            return;
        }

        vox_currNode = new_node(vox_currNode, vox_tl.x, vox_tl.y, vox_tl.z);
      }

      uint nextChildOfCurNode = new_node(currNode, tl.x, tl.y, tl.z);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.info = nextChildOfCurNode | (cur.info & 0xFFFFFF00u);
    }
    else // this child is a node, move to it
    {
      uint nextChildOfCurNode = new_node(currNode, tl.x, tl.y, tl.z);

      if (nextChildOfCurNode < 8) // if it is not a last child, leave the current node and place the child in the stack
      {
        cur.info = nextChildOfCurNode | (cur.info & 0xFFFFFF00u);
        stack[++top] = cur;
      }

      d = 0.5f / float(cur.p_size.y & 0xFFFF);
      float3 p_f = float3(2 * (cur.p_size.x >> 16) + ((currNode & 4) >> 2),2 * (cur.p_size.x & 0xFFFF) + ((currNode & 2) >> 1),2 * (cur.p_size.y >> 16) + (currNode & 1));
      t0 = _t0 + d * p_f * _l;
      tm = t0 + 0.5f * d * _l;

      uint linkData0 = m_pAccelStruct_m_SCom2Data[childLink];
      SdfDAGChildEdge ce = unpack_child_edge(header, linkData0, linkData0);
      uint nodeData0 = m_pAccelStruct_m_SCom2Data[ce.child_offset];
      NodeHeadUnpacked node = unpack_node_head(header, nodeData0, nodeData0);
      uint nextNode = first_node(t0, tm);
      uint rotIdx = m_pAccelStruct_m_RotAddTable[rotId * ROT_COUNT + ce.rotation_id];

      cur.linksOff = ce.child_offset + header.links_offset + (node.base_type == SCOM2_CHILD_EMPTY ? 0 : 1);
      cur.transform = m_pAccelStruct_m_RotAddTable[cur.transform * ROT_COUNT + ce.rotation_id];
      cur.info = (rotIdx << 24) | (node.children_types << 8) | nextNode;
      cur.p_size = (cur.p_size << 1) | uint2(((currNode & 4) << (16 - 2)) | ((currNode & 2) >> 1),(currNode & 1) << 16);
    }
  }
#endif
}

float m_pAccelStruct_COctreeV3_LoadDistanceValuesLeafSlices(uint brickOffset, float3 voxelPos, uint v_size, float sz_inv, in COctreeV3Header header, uint transform_code, inout float values[8]) {
  float vmin = 1e6f;
#ifndef DISABLE_SDF_COCTREE_V3
  uint rotIdx = transform_code & header.rot_mask;
  uint p_size = header.brick_size + 2 * header.brick_pad;
  uint PFlagPos = uint32_t(dot(m_pAccelStruct_m_SdfCompactOctreeRotModifiers[ROT_COUNT + rotIdx], 
                               int4((int)(voxelPos.x + header.brick_pad),(int)(voxelPos.y + header.brick_pad),(int)(voxelPos.z + header.brick_pad),1)));
  
  //early exit if voxel is not present
  if ((m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + PFlagPos/32] & (1u << (PFlagPos%32))) == 0)
    return 1e6f;
  
  //this voxel is guaranteed to have surface
  const uint line_distances_offset_bits = 16;
  const uint line_distances_offsets_per_uint = 32 / line_distances_offset_bits;

  uint slice_distance_flags_bits = v_size * v_size;
  uint slice_distance_flags_uints = (slice_distance_flags_bits + 32 - 1) / 32;
  uint distance_flags_size_uints = v_size * slice_distance_flags_uints;
  uint presence_flags_size_uints = (p_size * p_size * p_size + 32 - 1) / 32;
  uint distance_offsets_size_uints = (v_size + line_distances_offsets_per_uint - 1) / line_distances_offsets_per_uint; // 16 bits for offset, should be enough for all cases

  const uint min_range_size_uints = 2;

  //<presence_flags><distance_flags><distance_offsets><min value and range><tex_coords><distances>
  uint off_0 = 0;                                   // presence flags
  uint off_1 = off_0 + presence_flags_size_uints;   // distance flags
  uint off_2 = off_1 + distance_flags_size_uints;   // distance offsets
  uint off_3 = off_2 + distance_offsets_size_uints; // min value and range
  uint off_4 = off_3 + min_range_size_uints;        // texture coordinates
  uint off_5 = off_4 + 8 * header.uv_size;          // distances

  uint vals_per_int = 32 / header.bits_per_value;
  uint bits = header.bits_per_value;
  uint max_val = header.bits_per_value == 32 ? 0xFFFFFFFF : ((1 << bits) - 1);

  float add_transform = header.sim_compression > 0 ? 2*1.73205081f*sz_inv*(2*(float(transform_code & header.add_mask) / float(header.add_mask)) - 1) : 0.0f;

  float min_val = -float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_3 + 0]) / float(0xFFFFFFFFu) + add_transform;
  float range   =  (float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_3 + 1]) / float(0xFFFFFFFFu)) / max_val;

  for (int i = 0; i < 8; i++)
  {
    float3 vPosOrig = voxelPos + float3(header.brick_pad) + float3((i & 4) >> 2,(i & 2) >> 1,i & 1);
    uint voxelId = uint32_t(dot(m_pAccelStruct_m_SdfCompactOctreeRotModifiers[rotIdx], int4((int)vPosOrig.x,(int)vPosOrig.y,(int)vPosOrig.z,1)));
    uint sliceId = voxelId / (v_size * v_size);
    uint localId = voxelId % (v_size * v_size);

    uint b0 = m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_1 + slice_distance_flags_uints * sliceId];
    b0 = localId > 31 ? b0 : b0 & ((1u << localId) - 1);
    b0 = bitCount(b0);

    uint b1 = localId > 32 ? m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_1 + slice_distance_flags_uints * sliceId + 1] & ((1u << (localId - 32)) - 1)
                               : 0;
    b1 = bitCount(b1);

    uint sliceOffset = (m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_2 + sliceId / line_distances_offsets_per_uint] >>
                            (line_distances_offset_bits * (sliceId % line_distances_offsets_per_uint))) &
                           (((1u << line_distances_offset_bits) - 1));
    uint localOffset = b0 + b1;

    uint vId0 = sliceOffset + localOffset;
    uint dist0 = ((m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_5 + vId0 / vals_per_int] >> (bits * (vId0 % vals_per_int))) & max_val);
    values[i] = min_val + range * dist0;
  }
  vmin = -1.0f;
#endif
  return vmin;
}

float m_pAccelStruct_eval_dist_trilinear(const float values[8], float3 dp) {
  return (1-dp.x)*(1-dp.y)*(1-dp.z)*values[0] + 
         (1-dp.x)*(1-dp.y)*(  dp.z)*values[1] + 
         (1-dp.x)*(  dp.y)*(1-dp.z)*values[2] + 
         (1-dp.x)*(  dp.y)*(  dp.z)*values[3] + 
         (  dp.x)*(1-dp.y)*(1-dp.z)*values[4] + 
         (  dp.x)*(1-dp.y)*(  dp.z)*values[5] + 
         (  dp.x)*(  dp.y)*(1-dp.z)*values[6] + 
         (  dp.x)*(  dp.y)*(  dp.z)*values[7];
}

void m_pAccelStruct_OctreeNodeIntersect(uint type, const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint a_start, uint a_count, inout CRT_Hit pHit) {
  float values[8];
  uint32_t nodeId, primId;
  float d, qNear, qFar;
  float2 fNearFar;
  float3 start_q;

  qNear = 1.0f;

  m_pAccelStruct_RayNodeIntersection(type, ray_pos, ray_dir, tNear, geomId, a_start, values, primId, nodeId, d, qNear, qFar, fNearFar, start_q); /*out*/

  //fast return if starting point in this exact node or type is not supported
  if (qNear > 0.0f) 
    return;
  
  m_pAccelStruct_LocalSurfaceIntersection(type, ray_dir, instId, geomId, values, nodeId, primId, d, qNear, qFar, fNearFar, start_q, false, pHit); /*out*/
}

void m_pAccelStruct_SCom2Intersect(uint type, const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint a_start, uint a_count, inout CRT_Hit pHit) {
#ifndef DISABLE_SCOM2
  // It is a naive version that iterates all child nodes, instead of traversing it in octree-like fashion
  // TODO: make it faster

  const float3 ray_dir_inv = SafeInverse(ray_dir);
  float3 pos_ray_pos = ray_pos;
  float3 pos_ray_dir = ray_dir;

  // assume octree is box [-1,1]^3
  uint a = 0;
  if (ray_dir.x < 0)
  {
    pos_ray_pos.x *= -1;
    pos_ray_dir.x *= -1;
    a |= 4;
  }

  if (ray_dir.y < 0)
  {
    pos_ray_pos.y *= -1;
    pos_ray_dir.y *= -1;
    a |= 2;
  }

  if (ray_dir.z < 0)
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
  uint3 p;
  float3 p_f, t0, t1, tm;
  uint32_t level_sz;
  float d, level_sz_f;
  SCom2Header header = m_pAccelStruct_m_SCom2Headers[m_pAccelStruct_m_geomData[geomId].offset.y];

  stack[top].nodeId = header.nodes_arr_offset + header.root_node_off;
  stack[top].info = 0; // info here is cumulative rotation id of the node ( 8 bits ),
                       // number of surfaces if it is leaf ( 16 bits ),
                       // and if it is surface node or not if it is leaf ( 1 bit )
  stack[top].p_size = uint2(0,1);

  while (top >= 0)
  {
    OTStackElement curElem = stack[top];
    top--;

    level_sz = curElem.p_size.y & 0xFFFF;
    level_sz_f = float(level_sz);
    p = uint3(curElem.p_size.x >> 16,curElem.p_size.x & 0xFFFF,curElem.p_size.y >> 16);
    d = 1.0f / float(level_sz);
    p_f = float3(p);
    t0 = _t0 + d * p_f * _l;
    t1 = _t0 + d * (p_f + 1) * _l;

    if (min(t1.x, min(t1.y, t1.z)) < tNear)
      continue;

#ifdef ON_CPU
    if (debug_cur_pixel)
    {
      printf("[%d %d %d %d] node %d rot %d\n", p.x, p.y, p.z, level_sz, curElem.nodeId, curElem.info & 0xFF);
    }
#endif
    bool is_leaf = curElem.info > 0xFF;
    bool is_surface = curElem.info > 0xFFFFFFu;
    uint surface_count = (curElem.info >> 8) & 0xFFFF;
    if (!is_leaf)
    {
      float t = ubo[0].m_pAccelStruct_m_preset.fixed_lod > 0 ? 1.0f : max(t0.x, max(t0.y, t0.z));
      float target_lod_size = (pow(2.0f, ubo[0].m_pAccelStruct_m_preset.level_of_detail) - 0.01f) / t;
      is_leaf = float(level_sz) > target_lod_size;
      if (is_leaf) // we need to switch to leaf node for LOD of this node
      {
        NodeHeadUnpacked node = unpack_node_head(header, m_pAccelStruct_m_SCom2Data[curElem.nodeId], m_pAccelStruct_m_SCom2Data[curElem.nodeId + header.links_offset - 1]);
        if (node.base_type == SCOM2_CHILD_EMPTY)
          continue;
        is_surface = (node.base_type & SCOM2_CHILD_TYPE_MASK) == SCOM2_CHILD_LEAF_SURFACE;
        curElem.nodeId = curElem.nodeId + header.links_offset;
        surface_count = node.base_links_end - header.has_channels;
      }
    }

    if (is_leaf)
    {
      // intersect with grid inside leaf
      const float sz_inv = 2.0f / level_sz_f;
      const uint p_mask = level_sz - 1;
      const uint3 real_p = uint3(((a & 4) > 0) ? (~p.x & p_mask) : p.x,((a & 2) > 0) ? (~p.y & p_mask) : p.y,((a & 1) > 0) ? (~p.z & p_mask) : p.z);
      float3 brick_min_pos = float3(-1,-1,-1) + sz_inv * float3(real_p);
      float tmin = max(t0.x, max(t0.y, t0.z));
      float tmax = min(t1.x, min(t1.y, t1.z));
      uint rotId = curElem.info & 0xFF;
      float max_val = get_max_sdf_val(level_sz_f);
      d = 2.0f / (level_sz_f * header.brick_size);
      const float old_t = pHit.t;

      while (tmin < tmax && pHit.t == old_t)
      {
        uint v_size = header.brick_size + 1;
        float3 hit_pos = ray_pos + tmin * ray_dir;
        float3 local_pos = (hit_pos - brick_min_pos) * (0.5f * level_sz_f * header.brick_size);
        float3 voxelPos = floor(clamp(local_pos, 1e-6f, header.brick_size - 1e-6f));

        float3 min_pos = brick_min_pos + d * voxelPos;
        float3 max_pos = min_pos + d * float3(1,1,1);
        float3 size = max_pos - min_pos;
        uint s_id = 0;

        float2 fNearFar = RayBoxIntersection2(ray_pos, SafeInverse(ray_dir), min_pos, max_pos);

        while (fNearFar.x < fNearFar.y && s_id < surface_count && pHit.t == old_t)
        {
          uint link_0 = curElem.nodeId + s_id * header.uints_per_link;
          uint link_1 = link_0 + header.child_offset_off;
          SdfDAGDataEdge de = unpack_data_edge(header, max_val, m_pAccelStruct_m_SCom2Data[link_0], m_pAccelStruct_m_SCom2Data[link_1]);
          uint offset = header.bricks_arr_offset + header.bricks_step * de.data_offset;
          int4 voxelPosI = int4(int(voxelPos.x),int(voxelPos.y),int(voxelPos.z),1);
          uint rotIdx = m_pAccelStruct_m_RotAddTable[rotId * ROT_COUNT + de.rotation_id];
          float add = de.add;
          float vmin = 1.0f;
          float vmax = is_surface ? -1.0f : 1.0f;
          float3 start_q;
          float values[8];
          for (int i = 0; i < 8; i++)
          {
            int4 pI = voxelPosI + int4((i & 4) >> 2,(i & 2) >> 1,i & 1,0);
            uint vId0 = uint32_t(dot(m_pAccelStruct_m_SdfCompactOctreeRotModifiers[rotIdx], pI));
            uint p_val = m_pAccelStruct_m_SCom2Data[offset + vId0 / header.values_per_uint];
            uint p_off = (vId0 % header.values_per_uint) * header.bits_per_value;
            float val = max_val * (2.0f * ((p_val >> p_off) & header.value_mask) / float(header.value_mask) - 1) + add;
            values[i] = val - ubo[0].m_pAccelStruct_m_preset.compact_sdf_eps * d;
            // printf("vId0 = %d, offset = %d add = %f, val = %f\n", vId0, offset, add, values[i]);
            vmin = min(vmin, val);
            vmax = max(vmax, val);
          }

          if (vmin <= 0.0f && vmax >= 0.0f)
          {
            float3 start_pos = ray_pos + fNearFar.x * ray_dir;
            float vox_sz = (0.5f * level_sz_f * header.brick_size);
            start_q = vox_sz * (start_pos - min_pos);
            const float qNear = max(0.0f, vox_sz * (tNear - fNearFar.x));
            const float qFar = vox_sz * (fNearFar.y - fNearFar.x);

            m_pAccelStruct_LocalSurfaceIntersection(type, ray_dir, instId, geomId, values, curElem.nodeId, curElem.nodeId, d, qNear, qFar, fNearFar, start_q, is_surface, pHit);                                                                                                                /*out*/
          }

          // channels
          // if we need channel data, we must use rotations, but we cannot do it in resolve pass (no rotation info there)
          // however, we can transform interpolation quotients here that will lead to the same result
          if (pHit.t != old_t && header.has_channels > 0 && ubo[0].m_pAccelStruct_m_preset.render_mode != MULTI_RENDER_MODE_PRIMITIVE)
          {
            uint ch_off = curElem.nodeId + surface_count * header.uints_per_link;
            SdfDAGChildEdge channel_edge = unpack_child_edge(header, m_pAccelStruct_m_SCom2Data[ch_off], m_pAccelStruct_m_SCom2Data[ch_off + header.child_offset_off]);
            pHit.primId = channel_edge.child_offset;
            if (ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_CHANNEL || ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_CHANNEL_LAMBERT)
            {
              uint channelRotId = m_pAccelStruct_m_RotAddTable[(curElem.info & 0xFF) * ROT_COUNT + channel_edge.rotation_id];
              float t = (pHit.t - fNearFar.x) / d;
              float3 dp = start_q + t * ray_dir;
              float3 dp_rot = to_float3(m_pAccelStruct_m_SdfCompactOctreeRotTransforms[channelRotId] * to_float4(dp, 1.0f));
              dp_rot = clamp(float(0xFFFFu)*dp_rot, 0.0f, float(0xFFFFu));
              int3 dp_u16 = int3(dp_rot);
              pHit.coords[0] = asfloat((dp_u16.y << 16) | dp_u16.x);
              pHit.coords[1] = asfloat(dp_u16.z);
            }
          }

          s_id++;
        }
        tmin += max(0.0f, fNearFar.y - tmin) + 1e-5f;

        if (pHit.t < old_t)
        {
          top = -1;
          if (ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_LOD)
            pHit.primId = uint32_t(log2(float(level_sz*header.brick_size)));
        }
      }
    }
    else
    {
      NodeHeadUnpacked node = unpack_node_head(header, m_pAccelStruct_m_SCom2Data[curElem.nodeId], m_pAccelStruct_m_SCom2Data[curElem.nodeId + header.links_offset - 1]);
      uint node1 = m_pAccelStruct_m_SCom2Data[curElem.nodeId + header.references_offset];
      uint node2 = m_pAccelStruct_m_SCom2Data[curElem.nodeId + header.links_offset - 1];

      int buf_top = 0;
      tm = 0.5f * (t0 + t1);

      int currNode = first_node(t0, tm);
      do
      {
        uint rawChildNode = currNode ^ a; // child node number, from 0 to 8
        uint3 voxelPos = uint3((rawChildNode & 4) >> 2,(rawChildNode & 2) >> 1,rawChildNode & 1);
        uint voxId = uint32_t(dot(m_pAccelStruct_m_SdfCompactOctreeRotModifiers[2 * ROT_COUNT + curElem.info],
                                      int4((int)voxelPos.x,(int)voxelPos.y,(int)voxelPos.z,1)));

        uint ch_type = (node.children_types >> SCOM2_CHILD_TYPE_BITS * voxId) & SCOM2_CHILD_TYPE_MASK;
        uint ch_start_link = voxId == 0 ? node.base_links_end : get_links_end(header, voxId - 1, node1, node2);
        uint ch_end_link = get_links_end(header, voxId, node1, node2);

        // if we itersected with this node bbox and this node is valid
        if (ch_type != SCOM2_CHILD_EMPTY)
        {
          tmp_buf[buf_top].p_size = (curElem.p_size << 1) | uint2(((currNode & 4) << (16 - 2)) | ((currNode & 2) >> 1),(currNode & 1) << 16);

          uint link_off = curElem.nodeId + header.links_offset + ch_start_link * header.uints_per_link;
          if (ch_type == SCOM2_CHILD_NODE)
          {
            SdfDAGChildEdge ce = unpack_child_edge(header, m_pAccelStruct_m_SCom2Data[link_off],
                                                   m_pAccelStruct_m_SCom2Data[link_off + header.child_offset_off]);
            tmp_buf[buf_top].nodeId = header.nodes_arr_offset + ce.child_offset;
            tmp_buf[buf_top].info = m_pAccelStruct_m_RotAddTable[curElem.info * ROT_COUNT + ce.rotation_id];
          }
          else // SCOM2_CHILD_LEAF_VOLUME or SCOM2_CHILD_LEAF_SURFACE
          {
            uint surf_count = ch_end_link - ch_start_link - header.has_channels;
            tmp_buf[buf_top].nodeId = link_off;
            tmp_buf[buf_top].info = (curElem.info & 0xFF) | (surf_count << 8) | (ch_type == SCOM2_CHILD_LEAF_SURFACE ? (1 << 24) : 0);
          }
          buf_top++;
        }

        currNode = new_node(((currNode & 4) > 0) ? t1.x : tm.x, nn_indices[currNode].x,
                            ((currNode & 2) > 0) ? t1.y : tm.y, nn_indices[currNode].y,
                            ((currNode & 1) > 0) ? t1.z : tm.z, nn_indices[currNode].z);
      } while (currNode < 8);

      for (int i = 1; i <= buf_top; i++)
      {
        stack[top + i] = tmp_buf[buf_top - i];
      }
      top += buf_top;
    }
  }
#endif
}

float4 AnyHit(inout CRT_Hit hit, float4 rayPosAndNear, float4 rayDirAndFar) {
  float3 transparencyColor = float3(0,0,0);
  float alpha = 1.0f;
  float4 color = float4(0,0,0,0);
  do {
    transparencyColor += alpha * color.w * to_float3(color);
    alpha *= (1.0f - color.w);
    hit = m_pAccelStruct_RayQuery_NearestHit(rayPosAndNear, rayDirAndFar);
    uint type = hit.geomId >> SH_TYPE;
    uint geomId = hit.geomId & GEOM_ID_MASK;
    if (hit.primId == 0xFFFFFFFF) // no hit
      color = ubo[0].m_backgroundColor;
    else if (type != TYPE_SDF_SBS_COL && type != TYPE_GRAPHICS_PRIM)
    {
      uint matId = m_matIdbyPrimId[m_matIdOffsets[geomId].x + hit.primId % m_matIdOffsets[geomId].y];
      color = m_materials[matId].type == MULTI_RENDER_MATERIAL_TYPE_COLORED ? 
                        m_materials[matId].base_color : float4(0,0,0,1);
    }
    float3 rayPos = to_float3(rayPosAndNear) + to_float3(rayDirAndFar) * hit.t;
    rayPosAndNear = to_float4(rayPos, rayPosAndNear.w);
  } while (alpha > 0.01f);
  return to_float4(transparencyColor, alpha);
}

void m_pAccelStruct_TricubicLocalSurfaceIntersection(uint type, const float3 ray_dir, uint instId, uint geomId, inout float values[64], uint nodeId, uint primId, float d, float qNear, float qFar, float2 fNearFar, float3 start_q, inout CRT_Hit pHit) {
  const float EPS = 1e-6f;
  float d_inv = 1.0f / d;
  float t = qNear;
  bool hit = false;
  uint iter = 0;

  float start_dist = 10000;
  float start_sign = 1;

  float point[3] = {(start_q + t * ray_dir).x, (start_q + t * ray_dir).y, (start_q + t * ray_dir).z};
  start_dist = m_pAccelStruct_tricubicInterpolation(values, point);

  if (ubo[0].m_pAccelStruct_m_preset.representation_mode == REPRESENTATION_MODE_SURFACE)
    start_sign = sign(start_dist);
  start_dist *= start_sign;

  if (start_dist <= EPS || ubo[0].m_pAccelStruct_m_preset.sdf_node_intersect == SDF_OCTREE_NODE_INTERSECT_BBOX)
  {
    hit = true;
  }
  else if (ubo[0].m_pAccelStruct_m_preset.sdf_node_intersect == SDF_OCTREE_NODE_INTERSECT_ST)
  {
    const uint ST_max_iters = 256;
    float dist = start_dist;
    float3 pp0 = start_q + t * ray_dir;

    while (t < qFar && dist > EPS && iter < ST_max_iters)
    {
      t += dist * d_inv;

      point[0] = (start_q + t * ray_dir).x;
      point[1] = (start_q + t * ray_dir).y;
      point[2] = (start_q + t * ray_dir).z;
      dist = m_pAccelStruct_tricubicInterpolation(values, point);
      
      float3 pp = start_q + t * ray_dir;
      iter++;
    }
    hit = (dist <= EPS);
  }

  float tReal = fNearFar.x + d * t;

  if (t <= qFar && hit && tReal < pHit.t)
  {
    float3 norm = float3(0,0,1);
    if (m_pAccelStruct_need_normal())
    {
      float3 p0 = start_q + t * ray_dir;
      const float h = 0.001f;

      float p1[3] = {(p0 + float3(h,0,0)).x, (p0).y, (p0).z};
      float p2[3] = {(p0 + float3(-h,0,0)).x, p0.y, p0.z};
      
      float ddx = (m_pAccelStruct_tricubicInterpolation(values, p1) -
                   m_pAccelStruct_tricubicInterpolation(values, p2)) /
                  (2 * h);

      float p3[3] = {p0.x, (p0 + float3(0,h,0)).y, p0.z};
      float p4[3] = {p0.x, (p0 + float3(0,-h,0)).y, p0.z};
      float ddy = (m_pAccelStruct_tricubicInterpolation(values, p3) -
                   m_pAccelStruct_tricubicInterpolation(values, p4)) /
                  (2 * h);

      float p5[3] = {p0.x, p0.y, (p0 + float3(0,0,h)).z};
      float p6[3] = {p0.x, p0.y, (p0 + float3(0,0,-h)).z};

      float ddz = (m_pAccelStruct_tricubicInterpolation(values, p5) -
                   m_pAccelStruct_tricubicInterpolation(values, p6)) /
                  (2 * h);

      norm = start_sign * normalize(matmul4x3(m_pAccelStruct_m_instanceData[instId].transformInvTransposed, float3(ddx,ddy,ddz)));
    }
    
    float2 encoded_norm = encode_normal(norm);

    pHit.t = tReal;
    pHit.primId = primId;
    pHit.instId = instId;
    pHit.geomId = geomId | (type << SH_TYPE);
    pHit.coords[0] = 0;
    pHit.coords[1] = 0;
    pHit.coords[2] = encoded_norm.x;
    pHit.coords[3] = encoded_norm.y;

    if (ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_ST_ITERATIONS)
      pHit.primId = iter;
    
  #ifndef DISABLE_SDF_FRAME_OCTREE_TEX
    if (type == TYPE_SDF_FRAME_OCTREE_TEX)
    {
      float3 dp = start_q + t * ray_dir;
      
      pHit.coords[0] = (1-dp.x)*(1-dp.y)*(1-dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[0] + 
                        (1-dp.x)*(1-dp.y)*(  dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[2] + 
                        (1-dp.x)*(  dp.y)*(1-dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[4] + 
                        (1-dp.x)*(  dp.y)*(  dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[6] + 
                        (  dp.x)*(1-dp.y)*(1-dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[8] + 
                        (  dp.x)*(1-dp.y)*(  dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[10] + 
                        (  dp.x)*(  dp.y)*(1-dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[12] + 
                        (  dp.x)*(  dp.y)*(  dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[14];

      pHit.coords[1] = (1-dp.x)*(1-dp.y)*(1-dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[1] + 
                        (1-dp.x)*(1-dp.y)*(  dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[3] + 
                        (1-dp.x)*(  dp.y)*(1-dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[5] + 
                        (1-dp.x)*(  dp.y)*(  dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[7] + 
                        (  dp.x)*(1-dp.y)*(1-dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[9] + 
                        (  dp.x)*(1-dp.y)*(  dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[11] + 
                        (  dp.x)*(  dp.y)*(1-dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[13] + 
                        (  dp.x)*(  dp.y)*(  dp.z)*m_pAccelStruct_m_SdfFrameOctreeTexNodes[nodeId].tex_coords[15];
    }
  #endif
  }
}

void m_pAccelStruct_OctreeBrickIntersect(uint type, const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint bvhNodeId, uint a_count, inout CRT_Hit pHit) {
  #ifndef DISABLE_SDF_SBS
  float tricub_values[64], trilinear_values[8]; // values

  uint32_t nodeId, primId;
  float d, qNear, qFar;
  float2 fNearFar;
  float3 start_q;

  qNear = 1.0f;

  uint sdfId = m_pAccelStruct_m_geomData[geomId].offset.x;
  primId = bvhNodeId; //id of bbox in BLAS
  nodeId = primId + m_pAccelStruct_m_SdfSBSRoots[sdfId];
  SdfSBSHeader header = m_pAccelStruct_m_SdfSBSHeaders[sdfId];
  uint v_size = header.brick_size + 2*header.brick_pad + 1;

  float px = m_pAccelStruct_m_SdfSBSNodes[nodeId].pos_xy >> 16;
  float py = m_pAccelStruct_m_SdfSBSNodes[nodeId].pos_xy & 0x0000FFFF;
  float pz = m_pAccelStruct_m_SdfSBSNodes[nodeId].pos_z_lod_size >> 16;
  float sz = m_pAccelStruct_m_SdfSBSNodes[nodeId].pos_z_lod_size & 0x0000FFFF;
  float sz_inv = 2.0f/sz;
  
  d = 2.0f/(sz*header.brick_size);

  float3 brick_min_pos = float3(-1,-1,-1) + sz_inv*float3(px,py,pz);
  float3 brick_max_pos = brick_min_pos + sz_inv*float3(1,1,1);
  float3 brick_size = brick_max_pos - brick_min_pos;

  float2 brick_fNearFar = RayBoxIntersection2(ray_pos, SafeInverse(ray_dir), brick_min_pos, brick_max_pos);
  float old_t = pHit.t;
  while (brick_fNearFar.x < brick_fNearFar.y && pHit.t == old_t)
  {
    float3 hit_pos = ray_pos + brick_fNearFar.x*ray_dir;
    float3 local_pos = (hit_pos - brick_min_pos) * (0.5f*sz*header.brick_size);
    float3 voxelPos = floor(clamp(local_pos, 1e-6f, header.brick_size-1e-6f));

    float3 min_pos = brick_min_pos + d*voxelPos;
    float3 max_pos = min_pos + d*float3(1,1,1);
    float3 size = max_pos - min_pos;

    float vmin = 0;

    if (ubo[0].m_pAccelStruct_m_preset.interpolation_mode == INTERPOLATION_MODE_TRILINEAR)
    {
      vmin = m_pAccelStruct_load_distance_values(nodeId, voxelPos, v_size, sz_inv, header, trilinear_values);
    }
    else
    {
      vmin = m_pAccelStruct_load_tricubic_distance_values(nodeId, voxelPos, v_size, sz_inv, header, tricub_values);
    }

    fNearFar = RayBoxIntersection2(ray_pos, SafeInverse(ray_dir), min_pos, max_pos);
    if (tNear < fNearFar.x && vmin <= 0.0f)    
    {
      float3 start_pos = ray_pos + fNearFar.x*ray_dir;
      start_q = (start_pos - min_pos) * (0.5f*sz*header.brick_size);
      qFar = (fNearFar.y - fNearFar.x) * (0.5f*sz*header.brick_size);
    
      if (ubo[0].m_pAccelStruct_m_preset.interpolation_mode == INTERPOLATION_MODE_TRILINEAR)
      {
        m_pAccelStruct_LocalSurfaceIntersection(type, ray_dir, instId, geomId, trilinear_values, nodeId, primId, d, 0.0f, qFar, fNearFar, start_q, false, pHit); /*out*/
      }
      else
      {
        m_pAccelStruct_TricubicLocalSurfaceIntersection(type, ray_dir, instId, geomId, tricub_values, nodeId, primId, d, 0.0f, qFar, fNearFar, start_q, pHit);
      }
    
      if (ubo[0].m_pAccelStruct_m_preset.interpolation_mode == INTERPOLATION_MODE_TRILINEAR && header.aux_data == SDF_SBS_NODE_LAYOUT_ID32F_IRGB32F_IN &&
          ubo[0].m_pAccelStruct_m_preset.normal_mode == NORMAL_MODE_SDF_SMOOTHED && 
          m_pAccelStruct_need_normal() && pHit.t != old_t)
      {
        const float beta = 0.5f;
        float3 dp = start_q + ((pHit.t - fNearFar.x)/d)*ray_dir; //linear interpolation coefficients in voxels

        float3 normals[8];
        float values_n[8];
        for (int i=0;i<8;i++)
          normals[i] = float3(0,0,0);
        
        float3 nmq = float3(0.5f,0.5f,0.5f);
        nmq.x = dp.x < 0.5f ? step(-beta, beta, dp.x) : step(-beta, beta, dp.x - 1.0f);
        nmq.y = dp.y < 0.5f ? step(-beta, beta, dp.y) : step(-beta, beta, dp.y - 1.0f);
        nmq.z = dp.z < 0.5f ? step(-beta, beta, dp.z) : step(-beta, beta, dp.z - 1.0f);

        int3 voxelPos0 = int3(dp.x < 0.5f ? int(voxelPos.x) - 1 : int(voxelPos.x),dp.y < 0.5f ? int(voxelPos.y) - 1 : int(voxelPos.y),dp.z < 0.5f ? int(voxelPos.z) - 1 : int(voxelPos.z));
        for (int i=0;i<8;i++)
        {
          int3 VoxelPosI = voxelPos0 + int3((i & 4) >> 2,(i & 2) >> 1,i & 1);
          int3 BrickPosI = int3((VoxelPosI.x >= 0 ? (VoxelPosI.x < header.brick_size ? 0 : 1) : -1),(VoxelPosI.y >= 0 ? (VoxelPosI.y < header.brick_size ? 0 : 1) : -1),(VoxelPosI.z >= 0 ? (VoxelPosI.z < header.brick_size ? 0 : 1) : -1));

          uint neighborId = 0;
          neighborId += 3*3 * (BrickPosI.x + 1);
          neighborId +=   3 * (BrickPosI.y + 1);
          neighborId +=       (BrickPosI.z + 1);
          
          float3 dVoxelPos = float3(VoxelPosI) - voxelPos;
          uint32_t neighbor_nodeId;
          float3   neighbor_voxelPos;

          if (neighborId == 9+3+1)//we have our neighbor voxel in the same brick
          {
            neighbor_nodeId = nodeId;
            neighbor_voxelPos = float3(VoxelPosI);
          }
          else //we have our neighbor voxel in a different brick, read it from neigbors data
          {
            uint v_off = m_pAccelStruct_m_SdfSBSNodes[nodeId].data_offset;
            uint neighbors_data_offset = v_size*v_size*v_size + 8;

            neighbor_nodeId = m_pAccelStruct_m_SdfSBSData[v_off + neighbors_data_offset + neighborId];
            neighbor_voxelPos = float3(VoxelPosI) - header.brick_size*float3(BrickPosI);
          }

          if (neighbor_nodeId != INVALID_IDX)
          {
            m_pAccelStruct_load_distance_values(neighbor_nodeId, neighbor_voxelPos, v_size, sz_inv, header, values_n);
            normals[i] = normalize(eval_dist_trilinear_diff(values_n, dp - dVoxelPos));
          }
        }

        float3 smoothed_norm = (1-nmq.x)*(1-nmq.y)*(1-nmq.z)*normals[0] + 
                               (1-nmq.x)*(1-nmq.y)*(  nmq.z)*normals[1] + 
                               (1-nmq.x)*(  nmq.y)*(1-nmq.z)*normals[2] + 
                               (1-nmq.x)*(  nmq.y)*(  nmq.z)*normals[3] + 
                               (  nmq.x)*(1-nmq.y)*(1-nmq.z)*normals[4] + 
                               (  nmq.x)*(1-nmq.y)*(  nmq.z)*normals[5] + 
                               (  nmq.x)*(  nmq.y)*(1-nmq.z)*normals[6] + 
                               (  nmq.x)*(  nmq.y)*(  nmq.z)*normals[7];
        smoothed_norm = normalize(smoothed_norm);
        float2 encoded_norm = encode_normal(smoothed_norm);

        pHit.coords[2] = encoded_norm.x;
        pHit.coords[3] = encoded_norm.y;
      }
    }
    
    brick_fNearFar.x += max(0.0f, fNearFar.y-brick_fNearFar.x) + 1e-6f;
  }
  
  //ray hit a brick
  if (pHit.t < old_t)
  {
    float3 pos = ray_pos + pHit.t*ray_dir;
    float3 dp = (pos - brick_min_pos)*(0.5f*sz);

    if (header.aux_data == SDF_SBS_NODE_LAYOUT_DX_UV16)
    {   
      uint vals_per_int = 4/header.bytes_per_value;
      uint t_off = m_pAccelStruct_m_SdfSBSNodes[nodeId].data_offset + (v_size*v_size*v_size+vals_per_int-1)/vals_per_int;

      pHit.coords[0] = (1-dp.x)*(1-dp.y)*(1-dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfSBSData[t_off+0] >> 16)) + 
                        (1-dp.x)*(1-dp.y)*(  dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfSBSData[t_off+1] >> 16)) + 
                        (1-dp.x)*(  dp.y)*(1-dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfSBSData[t_off+2] >> 16)) + 
                        (1-dp.x)*(  dp.y)*(  dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfSBSData[t_off+3] >> 16)) + 
                        (  dp.x)*(1-dp.y)*(1-dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfSBSData[t_off+4] >> 16)) + 
                        (  dp.x)*(1-dp.y)*(  dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfSBSData[t_off+5] >> 16)) + 
                        (  dp.x)*(  dp.y)*(1-dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfSBSData[t_off+6] >> 16)) + 
                        (  dp.x)*(  dp.y)*(  dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfSBSData[t_off+7] >> 16));

      pHit.coords[1] = (1-dp.x)*(1-dp.y)*(1-dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfSBSData[t_off+0] & 0xFFFF)) + 
                        (1-dp.x)*(1-dp.y)*(  dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfSBSData[t_off+1] & 0xFFFF)) + 
                        (1-dp.x)*(  dp.y)*(1-dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfSBSData[t_off+2] & 0xFFFF)) + 
                        (1-dp.x)*(  dp.y)*(  dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfSBSData[t_off+3] & 0xFFFF)) + 
                        (  dp.x)*(1-dp.y)*(1-dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfSBSData[t_off+4] & 0xFFFF)) + 
                        (  dp.x)*(1-dp.y)*(  dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfSBSData[t_off+5] & 0xFFFF)) + 
                        (  dp.x)*(  dp.y)*(1-dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfSBSData[t_off+6] & 0xFFFF)) + 
                        (  dp.x)*(  dp.y)*(  dp.z)*(1.5259022e-5f*float(m_pAccelStruct_m_SdfSBSData[t_off+7] & 0xFFFF));
    }
    else if (header.aux_data == SDF_SBS_NODE_LAYOUT_DX_RGB8)
    {
      uint vals_per_int = 4/header.bytes_per_value;
      uint t_off = m_pAccelStruct_m_SdfSBSNodes[nodeId].data_offset + (v_size*v_size*v_size+vals_per_int-1)/vals_per_int;

      float3 color = (1-dp.x)*(1-dp.y)*(1-dp.z)*float3((m_pAccelStruct_m_SdfSBSData[t_off+0] >> 0) & 0xFF,(m_pAccelStruct_m_SdfSBSData[t_off+0] >> 8) & 0xFF,(m_pAccelStruct_m_SdfSBSData[t_off+0] >> 16) & 0xFF) + 
                     (1-dp.x)*(1-dp.y)*(  dp.z)*float3((m_pAccelStruct_m_SdfSBSData[t_off+1] >> 0) & 0xFF,(m_pAccelStruct_m_SdfSBSData[t_off+1] >> 8) & 0xFF,(m_pAccelStruct_m_SdfSBSData[t_off+1] >> 16) & 0xFF) + 
                     (1-dp.x)*(  dp.y)*(1-dp.z)*float3((m_pAccelStruct_m_SdfSBSData[t_off+2] >> 0) & 0xFF,(m_pAccelStruct_m_SdfSBSData[t_off+2] >> 8) & 0xFF,(m_pAccelStruct_m_SdfSBSData[t_off+2] >> 16) & 0xFF) + 
                     (1-dp.x)*(  dp.y)*(  dp.z)*float3((m_pAccelStruct_m_SdfSBSData[t_off+3] >> 0) & 0xFF,(m_pAccelStruct_m_SdfSBSData[t_off+3] >> 8) & 0xFF,(m_pAccelStruct_m_SdfSBSData[t_off+3] >> 16) & 0xFF) + 
                     (  dp.x)*(1-dp.y)*(1-dp.z)*float3((m_pAccelStruct_m_SdfSBSData[t_off+4] >> 0) & 0xFF,(m_pAccelStruct_m_SdfSBSData[t_off+4] >> 8) & 0xFF,(m_pAccelStruct_m_SdfSBSData[t_off+4] >> 16) & 0xFF) + 
                     (  dp.x)*(1-dp.y)*(  dp.z)*float3((m_pAccelStruct_m_SdfSBSData[t_off+5] >> 0) & 0xFF,(m_pAccelStruct_m_SdfSBSData[t_off+5] >> 8) & 0xFF,(m_pAccelStruct_m_SdfSBSData[t_off+5] >> 16) & 0xFF) + 
                     (  dp.x)*(  dp.y)*(1-dp.z)*float3((m_pAccelStruct_m_SdfSBSData[t_off+6] >> 0) & 0xFF,(m_pAccelStruct_m_SdfSBSData[t_off+6] >> 8) & 0xFF,(m_pAccelStruct_m_SdfSBSData[t_off+6] >> 16) & 0xFF) + 
                     (  dp.x)*(  dp.y)*(  dp.z)*float3((m_pAccelStruct_m_SdfSBSData[t_off+7] >> 0) & 0xFF,(m_pAccelStruct_m_SdfSBSData[t_off+7] >> 8) & 0xFF,(m_pAccelStruct_m_SdfSBSData[t_off+7] >> 16) & 0xFF);

      color = clamp(floor(color + 0.5f), 0.0f, 255.0f);

      pHit.coords[0] = color.x + color.y/256.0f;
      pHit.coords[1] = color.z;
    }
    else if (header.aux_data == SDF_SBS_NODE_LAYOUT_ID32F_IRGB32F ||
             header.aux_data == SDF_SBS_NODE_LAYOUT_ID32F_IRGB32F_IN)
    {
      uint t_off = m_pAccelStruct_m_SdfSBSNodes[nodeId].data_offset + v_size*v_size*v_size;

      const uint baseOffs0 = m_pAccelStruct_m_SdfSBSData[t_off+0];
      const uint baseOffs1 = m_pAccelStruct_m_SdfSBSData[t_off+1];
      const uint baseOffs2 = m_pAccelStruct_m_SdfSBSData[t_off+2];
      const uint baseOffs3 = m_pAccelStruct_m_SdfSBSData[t_off+3];
      const uint baseOffs4 = m_pAccelStruct_m_SdfSBSData[t_off+4];
      const uint baseOffs5 = m_pAccelStruct_m_SdfSBSData[t_off+5];
      const uint baseOffs6 = m_pAccelStruct_m_SdfSBSData[t_off+6];
      const uint baseOffs7 = m_pAccelStruct_m_SdfSBSData[t_off+7];

      float3 color = (1-dp.x)*(1-dp.y)*(1-dp.z)*float3(m_pAccelStruct_m_SdfSBSDataF[baseOffs0+0],m_pAccelStruct_m_SdfSBSDataF[baseOffs0+1],m_pAccelStruct_m_SdfSBSDataF[baseOffs0+2]) + 
                     (1-dp.x)*(1-dp.y)*(  dp.z)*float3(m_pAccelStruct_m_SdfSBSDataF[baseOffs1+0],m_pAccelStruct_m_SdfSBSDataF[baseOffs1+1],m_pAccelStruct_m_SdfSBSDataF[baseOffs1+2]) + 
                     (1-dp.x)*(  dp.y)*(1-dp.z)*float3(m_pAccelStruct_m_SdfSBSDataF[baseOffs2+0],m_pAccelStruct_m_SdfSBSDataF[baseOffs2+1],m_pAccelStruct_m_SdfSBSDataF[baseOffs2+2]) + 
                     (1-dp.x)*(  dp.y)*(  dp.z)*float3(m_pAccelStruct_m_SdfSBSDataF[baseOffs3+0],m_pAccelStruct_m_SdfSBSDataF[baseOffs3+1],m_pAccelStruct_m_SdfSBSDataF[baseOffs3+2]) + 
                     (  dp.x)*(1-dp.y)*(1-dp.z)*float3(m_pAccelStruct_m_SdfSBSDataF[baseOffs4+0],m_pAccelStruct_m_SdfSBSDataF[baseOffs4+1],m_pAccelStruct_m_SdfSBSDataF[baseOffs4+2]) + 
                     (  dp.x)*(1-dp.y)*(  dp.z)*float3(m_pAccelStruct_m_SdfSBSDataF[baseOffs5+0],m_pAccelStruct_m_SdfSBSDataF[baseOffs5+1],m_pAccelStruct_m_SdfSBSDataF[baseOffs5+2]) + 
                     (  dp.x)*(  dp.y)*(1-dp.z)*float3(m_pAccelStruct_m_SdfSBSDataF[baseOffs6+0],m_pAccelStruct_m_SdfSBSDataF[baseOffs6+1],m_pAccelStruct_m_SdfSBSDataF[baseOffs6+2]) + 
                     (  dp.x)*(  dp.y)*(  dp.z)*float3(m_pAccelStruct_m_SdfSBSDataF[baseOffs7+0],m_pAccelStruct_m_SdfSBSDataF[baseOffs7+1],m_pAccelStruct_m_SdfSBSDataF[baseOffs7+2]);

      color = clamp(floor(255.0f*color + 0.5f), 0.0f, 255.0f);

      pHit.coords[0] = color.x + color.y/256.0f;
      pHit.coords[1] = color.z;
    }
  }
#endif
}

float m_pAccelStruct_COctreeV3_LoadDistanceValuesLeafBitPack(uint brickOffset, float3 voxelPos, uint v_size, float sz_inv, in COctreeV3Header header, uint transform_code, inout float values[8]) {
  float vmin = 1e6f;
#ifndef DISABLE_SDF_COCTREE_V3
  uint rotIdx = transform_code & header.rot_mask;
  uint p_size = header.brick_size + 2 * header.brick_pad;
  uint PFlagPos = uint32_t(dot(m_pAccelStruct_m_SdfCompactOctreeRotModifiers[ROT_COUNT + rotIdx], 
                               int4(int(voxelPos.x) + int(header.brick_pad),int(voxelPos.y) + int(header.brick_pad),int(voxelPos.z) + int(header.brick_pad),1)));
  
  //early exit if voxel is not present
  if ((m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + PFlagPos/32] & (1u << (PFlagPos%32))) == 0)
    return 1e6f;
  
  //this voxel is guaranteed to have surface
  uint distance_flags_size_uints = (v_size * v_size * v_size + 32 - 1) / 32;
  uint presence_flags_size_uints = (p_size * p_size * p_size + 32 - 1) / 32;
  uint min_range_size_uints = 2;

  //<presence_flags><distance_flags><><min value and range><tex_coords><distances>
  uint off_0 = 0;                                   // presence flags
  uint off_1 = off_0 + presence_flags_size_uints;   // distance flags
  uint off_3 = off_1 + distance_flags_size_uints;   // min value and range
  uint off_4 = off_3 + min_range_size_uints;        // texture coordinates
  uint off_5 = off_4 + 8 * header.uv_size;          // distances

  uint vals_per_int = 32 / header.bits_per_value;
  uint bits = header.bits_per_value;
  uint max_val = header.bits_per_value == 32 ? 0xFFFFFFFF : ((1 << bits) - 1);

  float add_transform = header.sim_compression > 0 ? 2*1.73205081f*sz_inv*(2*(float(transform_code & header.add_mask) / float(header.add_mask)) - 1) : 0.0f;

  float min_val = -float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_3 + 0]) / float(0xFFFFFFFFu) + add_transform;
  float range   =  (float(m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_3 + 1]) / float(0xFFFFFFFFu)) / max_val;

  for (int i = 0; i < 8; i++)
  {
    float3 vPosOrig = voxelPos + float3(header.brick_pad) + float3((i & 4) >> 2,(i & 2) >> 1,i & 1);
    uint localId = uint32_t(dot(m_pAccelStruct_m_SdfCompactOctreeRotModifiers[rotIdx], int4((int)vPosOrig.x,(int)vPosOrig.y,(int)vPosOrig.z,1)));

    uint b0 = m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_1];
    b0 = localId > 31 ? b0 : b0 & ((1u << localId) - 1);
    b0 = bitCount(b0);

    uint b1 = localId > 32 ? m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_1 + 1] & ((1u << (localId - 32)) - 1) : 0;
    b1 = bitCount(b1);

    uint vId0 = b0 + b1;
    uint dist0 = ((m_pAccelStruct_m_SdfCompactOctreeV3Data[brickOffset + off_5 + vId0 / vals_per_int] >> (bits * (vId0 % vals_per_int))) & max_val);
    values[i] = min_val + range * dist0;
  }
  vmin = -1.0f;
#endif
  return vmin;
}

void m_pAccelStruct_IntersectAllTrianglesInLeaf(const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, uint a_start, uint a_count, inout CRT_Hit pHit) {
#ifndef DISABLE_MESH
  const uint2 a_geomOffsets = m_pAccelStruct_m_geomData[geomId].offset;

  for (uint triId = a_start; triId < a_start + a_count; triId++)
  {
    const uint A = m_pAccelStruct_m_indices[a_geomOffsets.x + triId*3 + 0];
    const uint B = m_pAccelStruct_m_indices[a_geomOffsets.x + triId*3 + 1];
    const uint C = m_pAccelStruct_m_indices[a_geomOffsets.x + triId*3 + 2];

    const float3 A_pos = to_float3(m_pAccelStruct_m_vertPos[a_geomOffsets.y + A]);
    const float3 B_pos = to_float3(m_pAccelStruct_m_vertPos[a_geomOffsets.y + B]);
    const float3 C_pos = to_float3(m_pAccelStruct_m_vertPos[a_geomOffsets.y + C]);

    const float3 edge1 = B_pos - A_pos;
    const float3 edge2 = C_pos - A_pos;
    const float3 pvec = cross(ray_dir, edge2);
    const float3 tvec = ray_pos - A_pos;
    const float3 qvec = cross(tvec, edge1);

    const float invDet = 1.0f / dot(edge1, pvec);
    const float v = dot(tvec, pvec) * invDet;
    const float u = dot(qvec, ray_dir) * invDet;
    const float t = dot(edge2, qvec) * invDet;

    if (v >= -1e-6f && u >= -1e-6f && (u + v <= 1.0f + 1e-6f) && t > tNear && t < pHit.t) 
    {
      pHit.t = t;
      pHit.primId = triId;
      pHit.instId = instId;
      pHit.geomId = geomId | (TYPE_MESH_TRIANGLE << SH_TYPE);
      pHit.coords[0] = u;
      pHit.coords[1] = v;

      if (m_pAccelStruct_need_normal())
      {
        float3 n = float3(1,0,0);
        if (ubo[0].m_pAccelStruct_m_preset.normal_mode == NORMAL_MODE_GEOMETRY)
        {
          n = cross(edge1, edge2);
        }
        else if (ubo[0].m_pAccelStruct_m_preset.normal_mode == NORMAL_MODE_VERTEX)
        {
          n = to_float3(m_pAccelStruct_m_vertNorm[a_geomOffsets.y + A] * (1.0f - u - v) + m_pAccelStruct_m_vertNorm[a_geomOffsets.y + B] * v + u * m_pAccelStruct_m_vertNorm[a_geomOffsets.y + C]);
        }

        n = normalize(matmul4x3(m_pAccelStruct_m_instanceData[instId].transformInvTransposed, n));
        float2 encoded_norm = encode_normal(n);

        pHit.coords[2] = encoded_norm.x;
        pHit.coords[3] = encoded_norm.y;
      }
      else
      {
        pHit.coords[2] = 0;
        pHit.coords[3] = 0;
      }
    }
  }
#endif
}

void m_pAccelStruct_BVH2TraverseF32(const float3 ray_pos, const float3 ray_dir, float tNear, uint instId, uint geomId, bool stopOnFirstHit, inout CRT_Hit pHit) {
  const uint bvhOffset = m_pAccelStruct_m_geomData[geomId].bvhOffset;

  uint32_t stack[STACK_SIZE];
  int top = 0;
  uint leftNodeOffset = 0;
  bool hitFound = false; //set to true if we found a hit with an opaque object
                         //1) if stopOnFirstHit = true, it is some hit, not closest
                         //2) if we hit and object of some specific type (i.e. octree)
                         //   we can guarantee that the first hit will be the closest

  const float3 rayDirInv = SafeInverse(ray_dir);
  while (top >= 0 && !hitFound)
  {
    while (top >= 0 && ((leftNodeOffset & LEAF_BIT) == 0))
    {
      const BVHNodePair fatNode = m_pAccelStruct_m_allNodePairs[bvhOffset + leftNodeOffset];

      const uint node0_leftOffset = fatNode.left.leftOffset;
      const uint node1_leftOffset = fatNode.right.leftOffset;

      const float2 tm0 = RayBoxIntersection2(ray_pos, rayDirInv, fatNode.left.boxMin, fatNode.left.boxMax);
      const float2 tm1 = RayBoxIntersection2(ray_pos, rayDirInv, fatNode.right.boxMin, fatNode.right.boxMax);

      const bool hitChild0 = (tm0.x <= tm0.y) && (tm0.y >= tNear) && (tm0.x <= pHit.t);
      const bool hitChild1 = (tm1.x <= tm1.y) && (tm1.y >= tNear) && (tm1.x <= pHit.t);

      // traversal decision
      leftNodeOffset = hitChild0 ? node0_leftOffset : node1_leftOffset;

      if (hitChild0 && hitChild1)
      {
        leftNodeOffset = (tm0.x <= tm1.x) ? node0_leftOffset : node1_leftOffset; // GPU style branch
        stack[top]     = (tm0.x <= tm1.x) ? node1_leftOffset : node0_leftOffset; // GPU style branch
        top++;
      }

      if (!hitChild0 && !hitChild1) // both miss, stack.pop()
      {
        top--;
        leftNodeOffset = stack[max(top, 0)];
      }

    } // end while (searchingForLeaf)

    // leaf node, intersect triangles
    //
    if (top >= 0 && leftNodeOffset != 0xFFFFFFFF)
    {
#ifdef STOP_ON_BLAS_LEAF
      pHit->t = tNear;
      pHit->primId = EXTRACT_START(leftNodeOffset);
      pHit->instId = instId;
      pHit->geomId = geomId;
      hitFound = true;
#else
      CRT_LeafInfo leafInfo;
      leafInfo.primId = 0xFFFFFFFF;
      leafInfo.aabbId = EXTRACT_START(leftNodeOffset);
      leafInfo.instId = instId;
      leafInfo.geomId = geomId;
      leafInfo.rayxId = 0xFFFFFFFF;
      leafInfo.rayyId = 0xFFFFFFFF;

      const float SDF_BIAS = 0.01f;
      const float tNearSdf = max(tNear, SDF_BIAS);
      const uint hitTag = m_pAccelStruct_IntersectAllPrimitivesInLeaf(to_float4(ray_pos, tNearSdf), to_float4(ray_dir, 1e9f), leafInfo, pHit);

      hitFound = (hitTag != 0 /*TAG_NONE*/) && (first_hit_is_closest(hitTag) || stopOnFirstHit);
#endif
    }

    // continue BVH traversal
    //
    top--;
    leftNodeOffset = stack[max(top, 0)];

  } // end while (top >= 0)
}

float2 m_pAccelStruct_box_intersects(in float3 min_pos, in float3 max_pos, in float3 origin, in float3 dir) {
  float3 safe_dir = sign(dir) * max(float3(1e-9f), abs(dir));
  float3 tMin = (min_pos - origin) / safe_dir;
  float3 tMax = (max_pos - origin) / safe_dir;
  float3 t1 = min(tMin, tMax);
  float3 t2 = max(tMin, tMax);
  float tNear = max(t1.x, max(t1.y, t1.z));
  float tFar = min(t2.x, min(t2.y, t2.z));

  return float2(tNear,tFar);
}

void m_pAccelStruct_IntersectGraphicPrims(in float3 ray_pos, in float3 ray_dir, float tNear, uint instId, uint geomId, uint a_start, uint a_count, inout CRT_Hit pHit) {
#ifndef DISABLE_GRAPHICS_PRIM

  const float EPS = 1e-5f, T_MAX = 1e15f;
  GraphicsPrimHeader header = m_pAccelStruct_m_GraphicsPrimHeaders[m_pAccelStruct_m_geomData[geomId].offset.x];
  float3 color = header.color;

  uint prim_len = 2u;
  if (header.prim_type == GRAPH_PRIM_POINT)
    prim_len = 1u;
  if (header.prim_type >= GRAPH_PRIM_LINE_COLOR)
    prim_len = 3u;
  
  uint start_index = m_pAccelStruct_m_geomData[geomId].offset.y + prim_len * a_start;
  uint end_index = start_index + prim_len; // 1 per bbox

  bool has_custom_color = header.prim_type >= GRAPH_PRIM_POINT_COLOR;

  if (header.prim_type == GRAPH_PRIM_POINT ||
      header.prim_type == GRAPH_PRIM_POINT_COLOR)
  {
    for (uint i = start_index; i < end_index; i += prim_len)
    {
      float4 point = m_pAccelStruct_m_GraphicsPrimPoints[i];
      if (has_custom_color)
        color = to_float3(m_pAccelStruct_m_GraphicsPrimPoints[i+1]);

      float3 point3 = float3(point.x,point.y,point.z);
      float pt_radius = point.w;
      float3 pos_minus_point3 = ray_pos - point3;

      float b = 2.f * dot(ray_dir, pos_minus_point3);
      float c = dot(pos_minus_point3, pos_minus_point3) - pt_radius * pt_radius;
      float D = b*b - 4*c;
      if (D >= EPS)
      {
        D = sqrt(D);
        float t = -(b + D) * 0.5f;
        if (t < EPS)
          t = (-b + D) * 0.5f;

        if (t > tNear && t < pHit.t)
        {
          float3 norm = normalize((ray_pos + t*ray_dir - point3) * 100.f);
          float2 encoded_norm = encode_normal(norm);

          pHit.t         = t;
          pHit.primId    = a_start;
          pHit.instId    = instId;
          pHit.geomId    = geomId | (TYPE_GRAPHICS_PRIM << SH_TYPE);
          pHit.coords[0] = color.x + color.y/256.0f;
          pHit.coords[1] = color.z;
          pHit.coords[2] = encoded_norm.x;
          pHit.coords[3] = encoded_norm.y;
        }
      }
    }
  }
  else
  {
    for (uint i = start_index; i < end_index; i += prim_len)
    {
      float4 point1 = m_pAccelStruct_m_GraphicsPrimPoints[i  ];
      float4 point2 = m_pAccelStruct_m_GraphicsPrimPoints[i+1];
      if (has_custom_color)
        color = to_float3(m_pAccelStruct_m_GraphicsPrimPoints[i+2]);

      float3 point1_3 = float3(point1.x,point1.y,point1.z);
      float3 point2_3 = float3(point2.x,point2.y,point2.z);
      if (header.prim_type == GRAPH_PRIM_LINE_SEGMENT_DIR ||
          header.prim_type == GRAPH_PRIM_LINE_SEGMENT_DIR_COLOR)
        point2_3 = point2_3 * 0.71f + point1_3 * 0.29f; // so that the line segment doesn't clip through the cone
      float ra = point1.w; // line (cylinder) radius

      float t = T_MAX;
      float3 norm = float3(0.f,0.f,0.f);

      if (header.prim_type == GRAPH_PRIM_LINE ||
          header.prim_type == GRAPH_PRIM_LINE_SEGMENT ||
          header.prim_type == GRAPH_PRIM_LINE_SEGMENT_DIR ||
          header.prim_type == GRAPH_PRIM_LINE_COLOR ||
          header.prim_type == GRAPH_PRIM_LINE_SEGMENT_COLOR ||
          header.prim_type == GRAPH_PRIM_LINE_SEGMENT_DIR_COLOR)
      {
        float3 oc =  ray_pos - point1_3;
        float3 ba = point2_3 - point1_3;

        float baba = dot(ba, ba);
        float bard = dot(ba, ray_dir);
        float baoc = dot(ba, oc);

        float k2 = baba - (bard * bard);
        float k1 = baba*dot(oc, ray_dir) - baoc*bard;
        float k0 = baba*dot(oc, oc) - baoc*baoc - ra*ra*baba;

        float D = k1*k1 - k2*k0;

        if (D > 0.f)
        {
          float dist = -(k1 + sqrt(D)) / k2;
          float y = baoc + dist*bard;

          if (header.prim_type == GRAPH_PRIM_LINE || (y > 0.f && y < baba))
          {
            t = dist;
            norm = normalize(oc+t*ray_dir - ba*y/baba);
          }
          else
          {
            dist = ((y < 0.f ? 0.f : baba) - baoc)/bard;
            if (abs(k1+k2*dist)<sqrt(D))
            {
              t = dist;
              norm = normalize(ba*sign(y));
            }
          }
        }
      }
      if (header.prim_type == GRAPH_PRIM_LINE_SEGMENT_DIR ||
          header.prim_type == GRAPH_PRIM_LINE_SEGMENT_DIR_COLOR)
      {
        ra = length(point2_3 - point1_3) * 0.15f;
        point2_3 = float3(point2.x,point2.y,point2.z);
        float3 pa = point2_3 * 0.7f + point1_3 * 0.3f; // cone length along line segment is 0.3*length(b - a) for now
        float3 ba = point2_3 - pa;
        float3 oa = ray_pos - pa;
        float3 ob = ray_pos - point2_3;
        float  m0 = dot(ba,ba);
        float  m1 = dot(oa,ba);
        float  m2 = dot(ray_dir,ba);
        float  m3 = dot(ray_dir,oa);
        float  m5 = dot(oa,oa);
        float  m9 = dot(ob,ba);

        // cap - only one
        if(m1 < 0.f)
        {
          float3 tmp1 = oa * m2 - ray_dir * m1;
          if(dot(tmp1, tmp1) < (ra*ra*m2*m2))
          {
            t = -m1/m2;
            norm = normalize(-1.0f*ba);
          }
        }
        
        // body
        float rr = ra;
        float hy = m0 + rr*rr;
        float k2 = m0*m0    - m2*m2*hy;
        float k1 = m0*m0*m3 - m1*m2*hy + m0*ra*(rr*m2);
        float k0 = m0*m0*m5 - m1*m1*hy + m0*ra*(rr*m1*2.0f - m0*ra);

        float D = k1*k1 - k2*k0;
        if(D > 0.f)
        {
          float dist = -(k1 + sqrt(D)) / k2;
          float y = m1 + dist * m2;
          if(y >= 0.f && y <= m0)
          {
            t = dist;
            norm = normalize(m0*(m0*(oa+t*ray_dir)+rr*ba*ra)-ba*hy*y);
          }
        }
      }
      if (header.prim_type == GRAPH_PRIM_BOX ||
          header.prim_type == GRAPH_PRIM_BOX_COLOR)
      {
        float3 pa = min(point2_3, point1_3);
        float3 pb = max(point2_3, point1_3);
        float minmax[3][2] = {{pa.x, pb.x}, {pa.y, pb.y}, {pa.z, pb.z}};

        const int4 vert_indices = int4(0,3,5,6);
        for (int vert_ind = 0; vert_ind < 4; ++vert_ind)
        {
          int axis_indices[3] = { int((vert_indices[vert_ind] & 4u) != 0),
                                  int((vert_indices[vert_ind] & 2u) != 0),
                                  int((vert_indices[vert_ind] & 1u) != 0) };
          pa = float3(minmax[0][axis_indices[0]],minmax[1][axis_indices[1]],minmax[2][axis_indices[2]]);
          for (int axis = 0; axis < 3; ++axis)
          {
            pb = pa;
            pb[axis] = minmax[axis][1-axis_indices[axis]];
            float4 norm_dist = rayCapsuleIntersect(ray_pos, ray_dir, pa, pb, ra);
            if (norm_dist.w < t)
            {
              t = norm_dist.w;
              norm = to_float3(norm_dist);
            }
          }
        }
      }

      if (t > tNear && t < pHit.t)
      {
        float2 encoded_norm = encode_normal(norm);

        pHit.t         = t;
        pHit.primId    = a_start;
        pHit.instId    = instId;
        pHit.geomId    = geomId | (TYPE_GRAPHICS_PRIM << SH_TYPE);
        pHit.coords[0] = color.x + color.y/256.0f;
        pHit.coords[1] = color.z;
        pHit.coords[2] = encoded_norm.x;
        pHit.coords[3] = encoded_norm.y;
      }
    }
  }
#endif // DISABLE_GRAPHICS_PRIM
}

bool m_pAccelStruct_need_normal() {
  return ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_LAMBERT_NO_TEX || 
         ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_NORMAL  ||
         ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_PHONG_NO_TEX ||
         ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_LAMBERT ||
         ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_LOD ||
         ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_GEOM ||
         ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_PHONG ||
         ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_CHANNEL ||
         ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_PBR ||
         ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_PBR_NO_TEX ||
         ubo[0].m_pAccelStruct_m_preset.render_mode == MULTI_RENDER_MODE_CHANNEL_LAMBERT;
}

float m_pAccelStruct_load_tricubic_distance_values(uint nodeId, float3 voxelPos, uint v_size, float sz_inv, in SdfSBSHeader header, inout float values[64]) {
  float vmin = 1e6f;
#ifndef DISABLE_SDF_SBS
  if (header.aux_data == SDF_SBS_NODE_LAYOUT_ID32F_IRGB32F ||
      header.aux_data == SDF_SBS_NODE_LAYOUT_ID32F_IRGB32F_IN)
  {
    uint v_off = m_pAccelStruct_m_SdfSBSNodes[nodeId].data_offset;
    
    float3 p0 = float3(int3(voxelPos)) - 1;

    for (int x = 0; x < 4; x++)
    {
      for (int y = 0; y < 4; y++)
      {
        for (int z = 0; z < 4; z++)
        {
          float3 vPos = p0 + float3(x,y,z);
          uint vId = SBS_v_to_i(vPos.x, vPos.y, vPos.z, v_size, header.brick_pad); 
          
          values[16 * z + 4 * y + x] = m_pAccelStruct_m_SdfSBSDataF[m_pAccelStruct_m_SdfSBSData[v_off + vId]];
          vmin = min(vmin, values[16 * z + 4 * y + x]);
        }
      }
    }
  }
  else 
  {
    uint v_off = m_pAccelStruct_m_SdfSBSNodes[nodeId].data_offset;
    uint vals_per_int = 4 / header.bytes_per_value;
    uint bits = 8 * header.bytes_per_value;
    uint max_val = header.bytes_per_value == 4 ? 0xFFFFFFFF : ((1 << bits) - 1);
    float d_max = 1.73205081f * sz_inv;
    float mult = 2 * d_max / max_val;

    float3 p0 = float3(int3(voxelPos)) - 1;

    for (int x = 0; x < 4; x++)
    {
      for (int y = 0; y < 4; y++)
      {
        for (int z = 0; z < 4; z++)
        {
          float3 vPos = p0 + float3(x,y,z);
          uint vId = SBS_v_to_i(vPos.x, vPos.y, vPos.z, v_size, header.brick_pad);
          
          values[16 * z + 4 * y + x] = -d_max + mult * ((m_pAccelStruct_m_SdfSBSData[v_off + vId / vals_per_int] >> (bits * (vId % vals_per_int))) & max_val);
          vmin = min(vmin, values[16 * z + 4 * y + x]);
        }
      }
    }
  }
#endif
  return vmin;
}

float m_pAccelStruct_eval_distance_sdf_grid(uint grid_id, float3 pos) {
  uint off = m_pAccelStruct_m_SdfGridOffsets[grid_id];
  uint3 size = m_pAccelStruct_m_SdfGridSizes[grid_id];

  //bbox for grid is a unit cube
  float3 grid_size_f = float3(size - 1);
  float3 vox_f = grid_size_f*((pos-float3(-1,-1,-1))/float3(2,2,2));// - float3(0.5, 0.5, 0.5);
  vox_f = min(max(vox_f, float3(0.0f)), grid_size_f - float3(1e-5f));
  uint3 vox_u = uint3(vox_f);
  float3 dp = vox_f - float3(vox_u);

  //trilinear sampling
  float res = 0.0f;
  if (ubo[0].m_pAccelStruct_m_preset.interpolation_mode == INTERPOLATION_MODE_TRILINEAR)
  {
    if (vox_u.x < size.x-1 && vox_u.y < size.y-1 && vox_u.z < size.z-1)
    {
      for (uint i = 0;i<2;i++)
      {
        for (uint j = 0;j<2;j++)
        {
          for (uint k = 0;k<2;k++)
          {
            float qx = (1 - dp.x + i*(2*dp.x-1));
            float qy = (1 - dp.y + j*(2*dp.y-1));
            float qz = (1 - dp.z + k*(2*dp.z-1));   
            res += qx*qy*qz*m_pAccelStruct_m_SdfGridData[off + (vox_u.z + k)*size.x*size.y + (vox_u.y + j)*size.x + (vox_u.x + i)];   
          }      
        }
      }
    }
    else
    {
      res += m_pAccelStruct_m_SdfGridData[off + (vox_u.z)*size.x*size.y + (vox_u.y)*size.x + (vox_u.x)]; 
    }
  } // tricubic interpolation
  else if (ubo[0].m_pAccelStruct_m_preset.interpolation_mode == INTERPOLATION_MODE_TRICUBIC)
  {
    if (vox_u.x < size.x-2 && vox_u.y < size.y-2 && vox_u.z < size.z-2 && vox_u.x > 0 && vox_u.y > 0 && vox_u.z > 0)
    {
      uint x0 = vox_u.x - 1u;
      uint y0 = vox_u.y - 1u;
      uint z0 = vox_u.z - 1u;
      float f_dp[3] = {dp.x, dp.y, dp.z}, grid_part[64];

      for (int x = 0; x < 4; ++x)
      {
        for (int y = 0; y < 4; ++y)
        {
          for (int z = 0; z < 4; ++z)
          {
            grid_part[z * 4 * 4 + y * 4 + x] = m_pAccelStruct_m_SdfGridData[off + (z0 + z)*size.x*size.y + (y0 + y)*size.x + (x0 + x)];
          }
        }
      }
      res = m_pAccelStruct_tricubicInterpolation(grid_part, f_dp);
    }
    else if (vox_u.x < size.x-1 && vox_u.y < size.y-1 && vox_u.z < size.z-1)
    {
      for (uint i = 0;i<2;i++)
      {
        for (uint j = 0;j<2;j++)
        {
          for (uint k = 0;k<2;k++)
          {
            float qx = (1 - dp.x + i*(2*dp.x-1));
            float qy = (1 - dp.y + j*(2*dp.y-1));
            float qz = (1 - dp.z + k*(2*dp.z-1));   
            res += qx*qy*qz*m_pAccelStruct_m_SdfGridData[off + (vox_u.z + k)*size.x*size.y + (vox_u.y + j)*size.x + (vox_u.x + i)];   
          }      
        }
      }
    }
    else
    {
      res += m_pAccelStruct_m_SdfGridData[off + (vox_u.z)*size.x*size.y + (vox_u.y)*size.x + (vox_u.x)]; 
    }
  }
  
  return res;
}

void m_pAccelStruct_LocalSurfaceIntersectionFast(uint type, const float3 ray_dir, uint instId, uint geomId, inout float values[8], uint nodeId, uint primId, float d, float qNear, float qFar, float2 fNearFar, float3 start_q, bool is_surface_node, inout CRT_Hit pHit) {
  const float BASE_EPS = 1e-6f;
  const float EPS = 1e-6f;
  float d_inv = 1.0f / d;
  float t = qNear;
  bool hit = false;

  float start_dist = m_pAccelStruct_eval_dist_trilinear(values, start_q + t * ray_dir);


  if (start_dist <= EPS || ubo[0].m_pAccelStruct_m_preset.sdf_node_intersect == SDF_OCTREE_NODE_INTERSECT_BBOX)
  {
    hit = true;
  }
  else
  {
    //finding exact intersection between surface sdf(x,y,z) = 0 and ray
    // based on paper "Ray Tracing of Signed Distance Function Grids, 
    // Journal of Computer Graphics Techniques (JCGT), vol. 11, no. 3, 94-113, 2022"
    // http://jcgt.org/published/0011/03/06/

    // define values and constants as proposed in paper
    float s000 = values[0]*d_inv;
    float s001 = values[1]*d_inv;
    float s010 = values[2]*d_inv;
    float s011 = values[3]*d_inv;
    float s100 = values[4]*d_inv;
    float s101 = values[5]*d_inv;
    float s110 = values[6]*d_inv;
    float s111 = values[7]*d_inv;

    float tmp = s101-s001;

    float k0 = s000;
    float k1 = s100-s000;
    float k2 = s010-s000;
    float k3 = s110-s010-k1;
    float k4 = k0-s001;
    float k5 = k1-tmp;
    float k6 = k2-(s011-s001);
    float k7 = k3-(s111-s011-tmp);

    float3 o = start_q;
    float3 d3 = ray_dir;

    float m0 = o.x*o.y;
    float m1 = d3.x*d3.y;
    float m2 = o.x*d3.y + o.y*d3.x;
    float m3 = k5*o.z - k1;
    float m4 = k6*o.z - k2;
    float m5 = k7*o.z - k3;

    float c0 = (k4*o.z - k0) + o.x*m3 + o.y*m4 + m0*m5;
    float c1 = d3.x*m3 + d3.y*m4 + m2*m5 + d3.z*(k4 + k5*o.x + k6*o.y + k7*m0);
    float c2 = m1*m5 + d3.z*(k5*d3.x + k6*d3.y + k7*m2);
    float c3 = k7*m1*d3.z;

    // the surface is defined by equation c3*t^3 + c2*t^2 + c1*t + c0 = 0;
    // solve this equation analytically or numerically using the Newton's method
    // see "Numerical Recipes - The Art of Scientific Computing - 3rd Edition" for details
    {
      float x1 = 1000;
      float x2 = 1000;
      float x3 = 1000;
      uint power = 0;
      if (abs(c3) > 1e-5f)
      {
        power = 3;
        //it is a cubic equation, transform it to x^3 + a*x^2 + b*x + c = 0
        //use Vieta method to obtain 3 or 1 real roots
        float a = c2/c3;
        float b = c1/c3;
        float c = c0/c3;   

        float Q = (a*a - 3*b)/9;
        float R = (2*a*a*a - 9*a*b + 27*c)/54;
        float Q3 = Q*Q*Q;

        if (R*R < Q3) //equation has three real roots
        {
          float theta = acos(R/sqrt(Q3));
          x1 = -2*sqrt(Q)*cos(theta/3) - a/3;
          x2 = -2*sqrt(Q)*cos((theta+2*M_PI)/3) - a/3;
          x3 = -2*sqrt(Q)*cos((theta-2*M_PI)/3) - a/3;
        }
        else //equation has only one real roots
        {
          float A = -sign(R)*pow(abs(R) + sqrt(R*R - Q3), 1.0f/3.0f);
          float B = abs(A) > EPS ? Q/A : 0;
          x1 = A+B - a/3;
        }
      }
      else if (abs(c2) > 1e-7f)
      {
        power = 2;
        //it is a quadratic equation a*x^2 + b*x + c = 0
        float a = c2;
        float b = c1;
        float c = c0;

        float D = b*b - 4*a*c;
        if (D > 0)
        {
          float q = -0.5f*(b + sign(b)*sqrt(D));
          x1 = q/a;
          if (abs(q) > EPS)
            x2 = c/q; 
        }
      }
      else if (abs(c1) > EPS)
      {
        power = 1;
        //it is a linear equation c1*x + c0 = 0
        x1 = -c0/c1;
      }
      //else
      //no roots or inf roots, something's fucked up so just drop it

      x1 = x1 < 0 ? 1000 : x1;
      x2 = x2 < 0 ? 1000 : x2;
      x3 = x3 < 0 ? 1000 : x3;

      t = min(x1, min(x2, x3));
      hit = (t >= 0 && t <= qFar);
    }
  }

  float tReal = fNearFar.x + d * t;

  if (t <= qFar && hit && tReal < pHit.t)
  {
    float3 norm = float3(0,0,1);
    {
      {
        float3 p0 = start_q + t * ray_dir;
        const float h = 0.001f;
        float ddx = (m_pAccelStruct_eval_dist_trilinear(values, p0 + float3(h,0,0)) -
                    m_pAccelStruct_eval_dist_trilinear(values, p0 + float3(-h,0,0))) /
                    (2 * h);
        float ddy = (m_pAccelStruct_eval_dist_trilinear(values, p0 + float3(0,h,0)) -
                    m_pAccelStruct_eval_dist_trilinear(values, p0 + float3(0,-h,0))) /
                    (2 * h);
        float ddz = (m_pAccelStruct_eval_dist_trilinear(values, p0 + float3(0,0,h)) -
                    m_pAccelStruct_eval_dist_trilinear(values, p0 + float3(0,0,-h))) /
                    (2 * h);
        norm = float3(ddx,ddy,ddz);
      }
      norm = normalize(norm);
    }
    
    float3 dp = start_q + t * ray_dir;
    float2 encoded_norm = encode_normal(norm);

    pHit.t = tReal;
    pHit.primId = primId;
    pHit.instId = instId;
    pHit.geomId = geomId | (type << SH_TYPE);
    pHit.coords[0] = 0;
    pHit.coords[1] = 0;
    pHit.coords[2] = encoded_norm.x;
    pHit.coords[3] = encoded_norm.y;
  }
}

float m_pAccelStruct_tricubicInterpolation(const float grid[64], const float dp[3]) {
  float res = 0;
  float values_yz[16];
  float values_z[4];

  for (uint j = 0; j < 4; ++j)
  {
    for (uint k = 0; k < 4; ++k)
    {
      //m_SdfGridData[off + (vox_u.z)*size.x*size.y + (vox_u.y)*size.x + (vox_u.x)]
      values_yz[4*j + k] = 
        tricubic_spline(
            grid[k*4*4 + (j)*4 + (0)],
            grid[k*4*4 + (j)*4 + (1)],
            grid[k*4*4 + (j)*4 + (2)],
            grid[k*4*4 + (j)*4 + (3)], 
            dp[0]
      );
    }
  }

  for (uint k = 0; k < 4; ++k)
  {
    values_z[k] = 
      tricubic_spline(
          values_yz[4*0 + k], 
          values_yz[4*1 + k], 
          values_yz[4*2 + k], 
          values_yz[4*3 + k], 
          dp[1]
    );
  }

  res = tricubic_spline(values_z[0], values_z[1], values_z[2], values_z[3], dp[2]);

  return res;
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
  
    float4 rayPosAndNear, rayDirAndFar;
    initEyeRay(x, y, kgenArgs.sample_id, rayPosAndNear, rayDirAndFar);
  
    CRT_Hit hit;
    m_transparencyBuffer[y*ubo[0].m_width + x] = AnyHit(hit, rayPosAndNear, rayDirAndFar);
  
    m_gBuffer[y*ubo[0].m_width + x] = hit;
  
  }
}

