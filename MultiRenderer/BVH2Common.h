#pragma once

#include "core/ISceneObject.h"
#include "utils/misc/graphics_primitive_common.h"
#include "utils/vdb/openvdb_common.h"
#include "utils/misc/csg_dnf.h"
#include "utils/common/plane.h"
#include "sdf/simple/sdf_simple_shared.h"
#include "sdf/multi/sdf_multi_shared.h"
#include "sdf/scom/scom_shared.h"
#include "sdf/scom2/scom_shared.h"
#include "utils/raytracing/intersection_common.h"

namespace LiteScene
{
  class Geometry;
};

struct BVHRT;
struct SdfDAG;
struct SCom2Tree;
struct SdfGridView;
struct SdfFrameOctreeView;
struct SdfSVSView;
struct SdfSBSView;
struct SdfFrameOctreeTexView;
struct COctreeV3View;
struct SdfMultiOctreeView;

static constexpr uint32_t TAG_NONE             = 0;// !!! #REQUIRED by kernel slicer: Empty/Default impl must have zero both m_tag and offset
static constexpr uint32_t TAG_TRIANGLE         = 1; 
static constexpr uint32_t TAG_COCTREE_BRICKED  = 2; 

static constexpr uint32_t TAG_SDF_BRICK        = 4;
static constexpr uint32_t TAG_SDF_NODE         = 8; 
static constexpr uint32_t TAG_GRAPHICS_PRIM    = 9;
static constexpr uint32_t TAG_SDF_GRID         = 11; 
static constexpr uint32_t TAG_OPENVDB_GRID     = 14;
static constexpr uint32_t TAG_SDF_OCTREE       = 15;
static constexpr uint32_t TAG_CSGDNF           = 16;

static constexpr uint32_t TAG_SDF_DAG          = 18;
static constexpr uint32_t TAG_SCOM2            = 19;

#ifndef KERNEL_SLICER
struct AABB_Info
{
  float3 boxMin;
  float3 boxMax;
  uint32_t ind;
};

struct RayInfo
{
  float3 pos;
  float3 dir;
  int intersected_AABB_count;
  int intersected_prims_count;
  std::vector<AABB_Info> bboxes_info;
};

#endif

// main class
//
struct BVHRT : public ISceneObject
{
  //overiding ISceneObject interface
  BVHRT(const char* a_buildName = nullptr, const char* a_layoutName = nullptr) : 
    m_buildName(a_buildName != nullptr ? a_buildName : ""), 
    m_layoutName(a_layoutName != nullptr ? a_layoutName : "") 
    {
      m_preset = getDefaultPreset();
      m_cuttingPlane = create_disabled_plane();
    }
  ~BVHRT() override {}

  const char* Name() const override { return "BVH2Fat"; }

  void ClearGeom() override;

  virtual uint32_t AddCustomGeom_FromFile(const char *geom_type_name, const char *filename, ISceneObject *fake_this) override;
  uint32_t AddCustomGeom(LiteScene::Geometry *geometry, ISceneObject *fake_this);

  uint32_t AddGeom_Triangles3f(const float *a_vpos3f, size_t a_vertNumber, const uint32_t *a_triIndices, size_t a_indNumber, uint32_t a_flags, size_t vByteStride) override;
  void     UpdateGeom_Triangles3f(uint32_t a_geomId, const float *a_vpos3f, size_t a_vertNumber, const uint32_t *a_triIndices, size_t a_indNumber, uint32_t a_flags, size_t vByteStride) override;

  //generic version, should not be used, use simplier version below
  uint32_t AddGeom_AABB(uint32_t a_typeId, const CRT_AABB* boxMinMaxF8, size_t a_boxNumber, uint32_t a_buildFlags, void** a_customPrimPtrs, size_t a_customPrimCount) override;
  void     UpdateGeom_AABB(uint32_t a_geomId, uint32_t a_typeId, const CRT_AABB* boxMinMaxF8, size_t a_boxNumber, uint32_t a_buildFlags, void** a_customPrimPtrs, size_t a_customPrimCount) override;

#ifndef KERNEL_SLICER  
  uint32_t AddGeom_Triangles3f(const float* a_vpos3f, const float* a_vnorm3f, size_t a_vertNumber, const uint32_t* a_triIndices, 
                               size_t a_indNumber, BuildOptions a_qualityLevel, size_t vByteStride);
  uint32_t AddGeom_SdfGrid(SdfGridView grid, ISceneObject *fake_this, BuildOptions a_qualityLevel = BUILD_HIGH);
  uint32_t AddGeom_SdfFrameOctree(SdfFrameOctreeView octree, ISceneObject *fake_this, BuildOptions a_qualityLevel = BUILD_HIGH);
  uint32_t AddGeom_SdfSVS(SdfSVSView octree, ISceneObject *fake_this, BuildOptions a_qualityLevel = BUILD_HIGH);
  uint32_t AddGeom_SdfSBS(SdfSBSView octree, ISceneObject *fake_this, BuildOptions a_qualityLevel = BUILD_HIGH);
  uint32_t AddGeom_SdfFrameOctreeTex(SdfFrameOctreeTexView octree, ISceneObject *fake_this, BuildOptions a_qualityLevel = BUILD_HIGH);                    
  uint32_t AddGeom_GraphicsPrim(const GraphicsPrimView &prim_view, ISceneObject *fake_this, BuildOptions a_qualityLevel = BUILD_HIGH);
  uint32_t AddGeom_COctreeV3(COctreeV3View octree, unsigned bvh_level, ISceneObject *fake_this, BuildOptions a_qualityLevel = BUILD_HIGH);
  uint32_t AddGeom_SdfMultiOctree(SdfMultiOctreeView octree, ISceneObject *fake_this, BuildOptions a_qualityLevel = BUILD_HIGH);
  uint32_t AddGeom_SdfDAG(const SdfDAG &dag, ISceneObject *fake_this, BuildOptions a_qualityLevel = BUILD_HIGH);
  uint32_t AddGeom_SCom2(const SCom2Tree &tree, ISceneObject *fake_this, BuildOptions a_qualityLevel = BUILD_HIGH);

  void UpdateGeom_Triangles3f(uint32_t a_geomId, const float* a_vpos3f, const float* a_vnorm3f, size_t a_vertNumber, 
                              const uint32_t* a_triIndices, size_t a_indNumber, BuildOptions a_qualityLevel, size_t vByteStride);

  void set_debug_mode(bool enable);
#endif

  void SetPreset(const MultiRenderPreset& a_preset);
  MultiRenderPreset GetPreset() const;

  size_t get_model_size(uint32_t geomId = 0u);

  void ClearScene() override;
  virtual void CommitScene(uint32_t a_qualityLevel) override;

  uint32_t AddInstance(uint32_t a_geomId, const float4x4 &a_matrix) override;
  uint32_t AddInstanceMotion(uint32_t a_geomId, const LiteMath::float4x4* a_matrices, uint32_t a_matrixNumber) override
  { return AddInstance(a_geomId, a_matrices[0]); }

  void UpdateInstance(uint32_t a_instanceId, const float4x4 &a_matrix) override;

  CRT_Hit RayQuery_NearestHit(float4 posAndNear, float4 dirAndFar) override;

  CRT_Hit RayQuery_NearestHitMotion(LiteMath::float4 posAndNear, LiteMath::float4 dirAndFar, float time) override
  { return RayQuery_NearestHit(posAndNear, dirAndFar); }

  bool    RayQuery_AnyHit(float4 posAndNear, float4 dirAndFar) override;
  bool    RayQuery_AnyHitMotion(LiteMath::float4 posAndNear, LiteMath::float4 dirAndFar, float time = 0.0f) override
  { return RayQuery_AnyHit(posAndNear, dirAndFar); }

//protected:

  uint32_t IntersectAllPrimitivesInLeaf(float4 rayPosAndNear, float4 rayDirAndFar, CRT_LeafInfo info, CRT_Hit* pHit);

  void IntersectSDFGrid(const float3 ray_pos, const float3 ray_dir,
                        float tNear, uint32_t instId, uint32_t geomId,
                        uint32_t a_start, uint32_t a_count,
                        CRT_Hit *pHit);

  void IntersectGraphicPrims(const float3& ray_pos, const float3& ray_dir,
                             float tNear, uint32_t instId,
                             uint32_t geomId, uint32_t a_start, uint32_t a_count, CRT_Hit* pHit);

  void IntersectAllTrianglesInLeaf(const float3 ray_pos, const float3 ray_dir,
                                   float tNear, uint32_t instId, uint32_t geomId,
                                   uint32_t a_start, uint32_t a_count,
                                   CRT_Hit *pHit);

  void COctreeIntersectV3(uint32_t type, const float3 ray_pos, const float3 ray_dir,
                         float tNear, uint32_t instId, uint32_t geomId,
                         uint32_t a_start, uint32_t a_count,
                         CRT_Hit *pHit);

  void OctreeNodeIntersect(uint32_t type, const float3 ray_pos, const float3 ray_dir,
                           float tNear, uint32_t instId, uint32_t geomId,
                           uint32_t a_start, uint32_t a_count,
                           CRT_Hit *pHit);

  void RayNodeIntersection(uint32_t type, const float3 ray_pos, const float3 ray_dir, 
                           float tNear, uint32_t geomId, uint32_t bvhNodeId, 
                           float values[8], uint32_t &primId, uint32_t &nodeId, float &d, 
                           float &qNear, float &qFar, float2 &fNearFar, float3 &start_q);

  float COctreeV3_LoadDistanceValues(uint32_t leafType, uint32_t brickOffset, float3 voxelPos, uint32_t v_size, float sz_inv, 
                                     const COctreeV3Header &header, uint32_t transform_code, float values[8]);

  float COctreeV3_LoadDistanceValuesLeafGrid(uint32_t brickOffset, float3 voxelPos, uint32_t v_size, float sz_inv, 
                                             const COctreeV3Header &header, uint32_t transform_code, float values[8]);

  float COctreeV3_LoadDistanceValuesLeafBitPack(uint32_t brickOffset, float3 voxelPos, uint32_t v_size, float sz_inv, 
                                                const COctreeV3Header &header, uint32_t transform_code, float values[8]);

  float COctreeV3_LoadDistanceValuesLeafSlices(uint32_t brickOffset, float3 voxelPos, uint32_t v_size, float sz_inv, 
                                               const COctreeV3Header &header, uint32_t transform_code, float values[8]);

  void COctreeV3_BrickIntersect(uint32_t type, const float3 ray_pos, const float3 ray_dir,
                                float tNear, uint32_t instId, uint32_t geomId, // const COctreeV3Header &header,
                                uint32_t brickOffset, float3 p, float sz, uint32_t transform_code,
                                CRT_Hit *pHit);

  void LocalSurfaceIntersection(uint32_t type, const float3 ray_dir, uint32_t instId, uint32_t geomId,
                                float values[8], uint32_t nodeId, uint32_t primId, float d, float qNear, 
                                float qFar, float2 fNearFar, float3 start_q, bool is_surface_node,
                                CRT_Hit *pHit);

  void LocalSurfaceIntersectionFast(uint32_t type, const float3 ray_dir, uint32_t instId, uint32_t geomId,
                                    float values[8], uint32_t nodeId, uint32_t primId, float d, float qNear, 
                                    float qFar, float2 fNearFar, float3 start_q, bool is_surface_node,
                                    CRT_Hit *pHit);

  void TricubicLocalSurfaceIntersection(uint32_t type, const float3 ray_dir, uint32_t instId, uint32_t geomId, 
                                float values[64], uint32_t nodeId, uint32_t primId, float d, float qNear, 
                                float qFar, float2 fNearFar, float3 start_q,
                                CRT_Hit *pHit);

  
  void OctreeBrickIntersect(uint32_t type, const float3 ray_pos, const float3 ray_dir,
                            float tNear, uint32_t instId, uint32_t geomId,
                            uint32_t a_start, uint32_t a_count,
                            CRT_Hit *pHit);

  void OctreeIntersect(uint32_t type, const float3 ray_pos, const float3 ray_dir,
                       float tNear, uint32_t instId, uint32_t geomId,
                       uint32_t a_start, uint32_t a_count,
                       CRT_Hit *pHit);

  void DAGIntersect(uint32_t type, const float3 ray_pos, const float3 ray_dir,
                    float tNear, uint32_t instId, uint32_t geomId,
                    uint32_t a_start, uint32_t a_count,
                    CRT_Hit *pHit);

  void SCom2Intersect(uint32_t type, const float3 ray_pos, const float3 ray_dir,
                      float tNear, uint32_t instId, uint32_t geomId,
                      uint32_t a_start, uint32_t a_count,
                      CRT_Hit *pHit);

  void SCom2IntersectFast(uint32_t type, const float3 ray_pos, const float3 ray_dir,
                          float tNear, uint32_t instId, uint32_t geomId,
                          uint32_t a_start, uint32_t a_count,
                          CRT_Hit *pHit);

  virtual void BVH2TraverseF32(const float3 ray_pos, const float3 ray_dir, float tNear, 
                               uint32_t instId, uint32_t geomId, bool stopOnFirstHit,
                               CRT_Hit *pHit);
  
  #ifndef KERNEL_SLICER
  RayInfo RI;

  CRT_Hit RayQuery_NearestHitWithRayInfo(float4 posAndNear, float4 dirAndFar);
  void BVH2TraverseF32WithRayInfo(const float3 ray_pos, const float3 ray_dir, float tNear,
                                uint32_t instId, uint32_t geomId, bool stopOnFirstHit,
                                CRT_Hit* pHit);
  #endif

  virtual void AppendTreeData(const std::vector<BVHNodePair>& a_nodes, const std::vector<uint32_t>& a_indices, 
                              const uint32_t *a_triIndices, size_t a_indNumber);

#ifndef KERNEL_SLICER  
  std::vector<BVHNode> GetBoxes_SdfGrid(SdfGridView grid);
  std::vector<BVHNode> GetBoxes_SdfFrameOctree(SdfFrameOctreeView octree);
  std::vector<BVHNode> GetBoxes_SdfFrameOctreeTex(SdfFrameOctreeTexView octree);
#endif

  //helper functions to calculate SDF
  //It is a copy-past of sdfScene functions with the same names
  //Slicer is weak and can't handle calling external functions  ¯\_(ツ)_/¯
  virtual float2 box_intersects(const float3 &min_pos, const float3 &max_pos, const float3 &origin, const float3 &dir);
  virtual float eval_dist_trilinear(const float values[8], float3 dp);
  virtual bool need_normal();
  float load_distance_values(uint32_t nodeId, float3 voxelPos, uint32_t v_size, float sz_inv, const SdfSBSHeader &header, float values[8]);
  float load_tricubic_distance_values(uint32_t nodeId, float3 voxelPos, uint32_t v_size, float sz_inv, const SdfSBSHeader &header, float values[64]);
  float tricubicInterpolation(const float grid[64], const float dp[3]);

#ifndef DISABLE_SDF_GRID
  virtual float eval_distance_sdf_grid(unsigned grid_id, float3 p);
#endif 

  // BVH orig nodes
#if !defined(DISABLE_SDF_FRAME_OCTREE) || !defined(DISABLE_SDF_COCTREE_V3) || !defined(DISABLE_SDF_FRAME_OCTREE_TEX)
  std::vector<BVHNode> m_origNodes;
#endif

  //SDF grid data
#ifndef DISABLE_SDF_GRID
  std::vector<float> m_SdfGridData;       //raw data for all SDF grids
  std::vector<uint32_t> m_SdfGridOffsets; //offset in m_SdfGridData for each SDF grid
  std::vector<uint3> m_SdfGridSizes;      //size for each SDF grid
#endif

  //SDF frame octree data
#ifndef DISABLE_SDF_FRAME_OCTREE
  std::vector<SdfFrameOctreeNode> m_SdfFrameOctreeNodes;//nodes for all SDF octrees
  std::vector<uint32_t> m_SdfFrameOctreeRoots;     //root node ids for each SDF octree
#endif

#ifndef DISABLE_SDF_COCTREE_V3
  COctreeV3Header coctree_v3_header;
  std::vector<uint32_t>             m_SdfCompactOctreeV3Data;
#endif
#if !defined(DISABLE_SDF_COCTREE_V3) || !defined(DISABLE_SCOM2)
  // has 3*48 rot modifiers - for distance values, for voxels, for child nodes
  static constexpr uint32_t ROT_COUNT = 48;
  std::vector<int4>     m_SdfCompactOctreeRotModifiers;
#endif

  //SDF Sparse Voxel Sets
#ifndef DISABLE_SDF_SVS
  std::vector<SdfSVSNode> m_SdfSVSNodes;//nodes for all SDF Sparse Voxel Sets
  std::vector<uint32_t> m_SdfSVSRoots;     //root node ids for each SDF Sparse Voxel Set
#endif

  //SDF Sparse Brick Sets
#ifndef DISABLE_SDF_SBS
  std::vector<SdfSBSNode>   m_SdfSBSNodes;   //nodes for all SDF Sparse Brick Sets
  std::vector<uint32_t>     m_SdfSBSData;    //raw data for all Sparse Brick Sets
  std::vector<float>        m_SdfSBSDataF;    //raw float data for all indexed Sparse Brick Sets
  std::vector<uint32_t>     m_SdfSBSRoots;   //root node ids for each SDF Sparse Voxel Set
  std::vector<SdfSBSHeader> m_SdfSBSHeaders; //header for each SDF Sparse Voxel Set
#endif

  //SDF textured frame octree data
#ifndef DISABLE_SDF_FRAME_OCTREE_TEX
  std::vector<SdfFrameOctreeTexNode> m_SdfFrameOctreeTexNodes;//nodes for all SDF octrees
  std::vector<uint32_t> m_SdfFrameOctreeTexRoots;          //root node ids for each SDF octree
#endif

#ifndef DISABLE_SDF_MULTI_OCTREE
  std::vector<SdfMultiOctreeNode> m_SdfMultiOctreeNodes;//nodes for all SDF multi octrees
  std::vector<float> m_SdfMultiOctreeValues;
#endif

#ifndef DISABLE_SDF_DAG
  std::vector<SdfDAGHeader> m_SdfDAGHeaders;
  std::vector<SdfDAGNode> m_SdfDAGNodes;
  std::vector<SdfDAGChildEdge> m_SdfDAGChildEdges;
  std::vector<SdfDAGDataEdge> m_SdfDAGDataEdges;
  std::vector<float> m_SdfDAGDistances;
  std::vector<uint32_t> m_SdfDAGTranspositions;
  uint32_t m_SdfDAGBrickTranspositionOffset = 0;
  uint32_t m_SdfDAGTransformCount = 0;
#endif
#if !defined(DISABLE_SDF_DAG) || !defined(DISABLE_SCOM2)
  std::vector<float4x4> m_SdfCompactOctreeRotTransforms;
  std::vector<uint32_t> m_RotAddTable;
#endif

#ifndef DISABLE_SCOM2
  std::vector<SCom2Header> m_SCom2Headers;
  std::vector<uint32_t>    m_SCom2Data;
#endif

  // Graphic primitives data
#ifndef DISABLE_GRAPHICS_PRIM
  std::vector<float4> m_GraphicsPrimPoints;
  std::vector<GraphicsPrimHeader> m_GraphicsPrimHeaders;
#endif

  //meshes data
#ifndef DISABLE_MESH
  std::vector<float4>   m_vertPos;
  std::vector<float4>   m_vertNorm;
  std::vector<uint32_t> m_indices;
  std::vector<uint32_t> m_primIndices;
#endif

  //geometric data, indexed by geomId
  std::vector<GeomData> m_geomData;
  std::vector<uint>     m_geomTags;
  std::vector<uint2>    startEnd;

  //instance data
  std::vector<InstanceData> m_instanceData;

  //AABB data, indexed by globalAABBId
  std::vector<uint32_t> m_primIdCount; //for each AABB stores start index and number of primitives stored in this AABB

  //Top Level Acceleration Structure
  std::vector<BVHNode>    m_nodesTLAS;

  //Bottom Level Acceleration Structure
  std::vector<BVHNodePair> m_allNodePairs;


  // Format name the tree build from
  const std::string m_buildName;
  const std::string m_layoutName;

  bool m_firstSceneCommit = true;
  bool debug_cur_pixel = false;
  bool support_fast_scom_traversal = false;
  
  MultiRenderPreset m_preset;
  float m_precomputedLoDThr = 1.0f;
  Plane m_cuttingPlane;

  static uint32_t preferredBVHLevel;
};
