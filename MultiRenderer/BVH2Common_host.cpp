#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cfloat>
#include <memory>
#include <array>
#include <map>

#include "BVH2Common.h"
#include "sdf/build/sparse_octree_builder.h"
#include "sdf/scom/similarity_compression.h"
#include "core/scene_extension.h"
#include "sdf/simple/sdf_simple.h"
#include "sdf/multi/sdf_multi.h"
#include "sdf/scom2/scom_utils.h"
#include "sdf/scom2/clustering/symmetric_groups.h"
#include "utils/common/bit_cast.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static constexpr std::size_t reserveSize = 1000;

uint32_t BVHRT::preferredBVHLevel = 0;

static uint32_t type_to_tag(uint32_t type)
{
  switch (type)
  {
  case TYPE_MESH_TRIANGLE:
    return TAG_TRIANGLE;

  case TYPE_SDF_GRID:
    return TAG_SDF_GRID;
  
  case TYPE_SDF_SVS:
  case TYPE_SDF_FRAME_OCTREE:
  case TYPE_SDF_FRAME_OCTREE_TEX:
    return TAG_SDF_NODE;
  
  case TYPE_SDF_SBS:
  case TYPE_SDF_SBS_TEX:
  case TYPE_SDF_SBS_COL:
    return TAG_SDF_BRICK;
  
  case TYPE_GRAPHICS_PRIM:
    return TAG_GRAPHICS_PRIM;
  
  case TYPE_COCTREE_V3:
    return TAG_COCTREE_BRICKED;

  case TYPE_OPENVDB_GRID:
    return TAG_OPENVDB_GRID;
  
  case TYPE_CSGDNF:
    return TAG_CSGDNF;

  case TYPE_SDF_MULTI_OCTREE:
    return TAG_SDF_OCTREE;

  case TYPE_SDF_DAG:
    return TAG_SDF_DAG;

  case TYPE_SCOM2:
    return TAG_SCOM2;

  default:
    return TAG_NONE;
  }
}

void add_border_nodes_rec(COctreeV3View octree, uint32_t max_bvh_level, 
                          std::vector<BVHNode> &nodes,
                          uint32_t nodeId, float3 p, float d, uint32_t level)
{
  if (nodeId >= octree.size)
  {
    //it is probably ok, just empty node was not trimmed for some reason
    printf("level %d, error: nodeId %d >= octree.size %d\n", level, nodeId, octree.size);
    return;
  }
#ifdef ON_CPU
  assert(COctreeV3::VERSION == 5); //if version is changed, this function should be revisited, as some changes may be needed
#endif
  unsigned childrenInfo = octree.data[nodeId + 0];
  unsigned children_leaves = childrenInfo & 0xFFFF00u;

  if (children_leaves || max_bvh_level == level)
  {
    //stop here
    nodes.emplace_back();
    nodes.back().leftOffset = nodeId;
    nodes.back().boxMin = float3(-1,-1,-1) + 2.0f*p*d;
    nodes.back().boxMax = float3(-1,-1,-1) + 2.0f*(p+float3(1,1,1))*d;
    //printf("added box (%f, %f, %f), (%f, %f, %f)\n", nodes.back().boxMin.x, nodes.back().boxMin.y, nodes.back().boxMin.z, nodes.back().boxMax.x, nodes.back().boxMax.y, nodes.back().boxMax.z);
  }
  else
  {
    int child_pos = 0;
    int uint_per_child = octree.header.uints_per_child_info;
    for (int i = 0; i < 8; i++)
    {
      // if child is active
      if ((childrenInfo & (1u << i)) > 0)
      {
        float ch_d = d / 2;
        float3 ch_p = 2 * p + float3((i & 4) >> 2, (i & 2) >> 1, i & 1);
        add_border_nodes_rec(octree, max_bvh_level, nodes, octree.data[nodeId + 1 + uint_per_child*child_pos], ch_p, ch_d, level+1);
        child_pos++;
      }
    }
  }
}

std::vector<BVHNode> GetBoxes_COctreeV3(COctreeV3View octree, uint32_t max_bvh_level, 
                                        uint32_t nodeId, float3 p, float d, uint32_t level)
{
  std::vector<BVHNode> nodes;
  add_border_nodes_rec(octree, max_bvh_level, nodes, 0, float3(0,0,0), 1.0f, 0);

  return nodes;
}

void BVHRT::ClearGeom()
{
  m_geomData.reserve(std::max<std::size_t>(reserveSize, m_geomData.capacity()));
  m_geomData.resize(0);
  //m_abstractObjectPtrs.reserve(m_geomData.capacity());
  //m_abstractObjectPtrs.resize(0);

  startEnd.reserve(m_geomData.capacity());
  startEnd.resize(0);

  m_indices.reserve(std::max<std::size_t>(100000 * 3, m_indices.capacity()));
  m_indices.resize(0);

  m_vertPos.reserve(std::max<std::size_t>(100000, m_vertPos.capacity()));
  m_vertPos.resize(0);

  m_primIndices.reserve(std::max<std::size_t>(100000, m_primIndices.capacity()));
  m_primIndices.resize(0);

  m_allNodePairs.reserve(std::max<std::size_t>(100000, m_allNodePairs.capacity()));
  m_allNodePairs.resize(0);

  //m_abstractObjects.reserve(reserveSize);
  //m_abstractObjects.resize(0);
  m_geomTags.reserve(m_geomData.capacity());
  m_geomTags.resize(0);

  ClearScene();
}

uint32_t BVHRT::AddCustomGeom_FromFile(const char *geom_type_name, const char *filename, ISceneObject *fake_this)
{
  LiteScene::SceneMetadata metadata;
  metadata.scene_xml_folder = ".";
  pugi::xml_node node = metadata.xml_doc.append_child(LiteScene::s2ws(geom_type_name).c_str());
  node.append_attribute(L"loc").set_value(LiteScene::s2ws(filename).c_str());
  node.append_attribute(L"id").set_value(0);
    
  LiteScene::Geometry *geom = new LiteScene::CustomGeometry();
  geom->type_name = geom_type_name;
  geom->custom_data = node;

  if (geom_type_name == LiteScene::SDFGridGeometry::get_type_name())
  {
    cast_or_create(LiteScene::SDFGridGeometry, grid, metadata, geom);
    return AddGeom_SdfGrid(grid->model, fake_this);
  }
  if (geom_type_name == LiteScene::SDFFrameOctreeGeometry::get_type_name())
  {
    cast_or_create(LiteScene::SDFFrameOctreeGeometry, frame_octree, metadata, geom);
    return AddGeom_SdfFrameOctree(frame_octree->model, fake_this);
  }
  if (geom_type_name == LiteScene::SDFSVSGeometry::get_type_name())
  {
    cast_or_create(LiteScene::SDFSVSGeometry, svs, metadata, geom);
    return AddGeom_SdfSVS(svs->model, fake_this);
  }
  if (geom_type_name == LiteScene::SDFSBSGeometry::get_type_name())
  {
    cast_or_create(LiteScene::SDFSBSGeometry, sbs, metadata, geom);
    return AddGeom_SdfSBS(sbs->model, fake_this);
  }
  if (geom_type_name == LiteScene::SDFCOctreeGeometry::get_type_name())
  {
    cast_or_create(LiteScene::SDFCOctreeGeometry, coctree, metadata, geom);
    return AddGeom_COctreeV3(coctree->model, preferredBVHLevel, fake_this);
  }
  if (geom_type_name == LiteScene::SDFMultiOctreeGeometry::get_type_name())
  {
    cast_or_create(LiteScene::SDFMultiOctreeGeometry, multi_octree, metadata, geom);
    return AddGeom_SdfMultiOctree(multi_octree->model, fake_this);
  }
  if (geom_type_name == LiteScene::SDFAugmentedMultiOctreeGeometry::get_type_name())
  {
    cast_or_create(LiteScene::SDFAugmentedMultiOctreeGeometry, multi_octree_a, metadata, geom);
    return AddGeom_SdfMultiOctree(multi_octree_a->model.octree, fake_this);
  }
  if (geom_type_name == LiteScene::SDFDAGGeometry::get_type_name())
  {
    cast_or_create(LiteScene::SDFDAGGeometry, dag_a, metadata, geom);
    return AddGeom_SdfDAG(dag_a->model, fake_this);
  }
  if (geom_type_name == LiteScene::SCom2Geometry::get_type_name())
  {
    cast_or_create(LiteScene::SCom2Geometry, scom2, metadata, geom);
    return AddGeom_SCom2(scom2->model, fake_this);
  }

  printf("BVHRT::AddCustomGeom_FromFile: Unsupported geometry type: %s\n", geom_type_name);
  return INVALID_IDX;
}

uint32_t BVHRT::AddCustomGeom(LiteScene::Geometry *geometry, ISceneObject *fake_this)
{
  auto *grid = dynamic_cast<LiteScene::SDFGridGeometry*>(geometry);
  if (grid)
  {
    return AddGeom_SdfGrid(grid->model, fake_this);
  }

  auto *frame_octree = dynamic_cast<LiteScene::SDFFrameOctreeGeometry*>(geometry);
  if (frame_octree)
  {
    return AddGeom_SdfFrameOctree(frame_octree->model, fake_this);
  }

  auto *svs = dynamic_cast<LiteScene::SDFSVSGeometry*>(geometry);
  if (svs)
  {
    return AddGeom_SdfSVS(svs->model, fake_this);
  }

  auto *sbs = dynamic_cast<LiteScene::SDFSBSGeometry*>(geometry);
  if (sbs)
  {
    return AddGeom_SdfSBS(sbs->model, fake_this);
  } 

  auto *coctree = dynamic_cast<LiteScene::SDFCOctreeGeometry*>(geometry);
  if (coctree)
  {
    return AddGeom_COctreeV3(coctree->model, preferredBVHLevel, fake_this);
  }

  auto *multi_octree = dynamic_cast<LiteScene::SDFMultiOctreeGeometry*>(geometry);
  if (multi_octree)
  {
    return AddGeom_SdfMultiOctree(multi_octree->model, fake_this);
  }

  auto *multi_octree_a = dynamic_cast<LiteScene::SDFAugmentedMultiOctreeGeometry*>(geometry);
  if (multi_octree_a)
  {
    return AddGeom_SdfMultiOctree(multi_octree_a->model.octree, fake_this);
  }

  auto *dag_a = dynamic_cast<LiteScene::SDFDAGGeometry*>(geometry);
  if (dag_a)
  {
    return AddGeom_SdfDAG(dag_a->model, fake_this);
  }

  auto *scom2 = dynamic_cast<LiteScene::SCom2Geometry*>(geometry);
  if (scom2)
  {
    return AddGeom_SCom2(scom2->model, fake_this);
  }

  auto *custom = dynamic_cast<LiteScene::CustomGeometry*>(geometry);
  if (custom)
  {
    printf("[AddCustomGeom] Cannot add custom geometry of unknown type\n");
    return uint32_t(-1);
  }

  printf("[AddCustomGeom] Cannot add geometry of unknown type\n");
  return uint32_t(-1);
}

void BVHRT::AppendTreeData(const std::vector<BVHNodePair>& a_nodes, const std::vector<uint32_t>& a_indices, const uint32_t *a_triIndices, size_t a_indNumber)
{
  m_allNodePairs.insert(m_allNodePairs.end(), a_nodes.begin(), a_nodes.end());
  m_primIndices.insert(m_primIndices.end(), a_indices.begin(), a_indices.end());
  
  const size_t oldIndexSize  = m_indices.size();
  m_indices.resize(oldIndexSize + a_indices.size()*3);
  for(size_t i=0;i<a_indices.size();i++)
  {
    const uint32_t triId = a_indices[i];
    m_indices[oldIndexSize + 3*i+0] = a_triIndices[triId*3+0];
    m_indices[oldIndexSize + 3*i+1] = a_triIndices[triId*3+1];
    m_indices[oldIndexSize + 3*i+2] = a_triIndices[triId*3+2];
  }
}

uint32_t BVHRT::AddGeom_Triangles3f(const float *a_vpos3f, size_t a_vertNumber, const uint32_t *a_triIndices, size_t a_indNumber, uint32_t a_qualityLevel, size_t vByteStride)
{
  return AddGeom_Triangles3f(a_vpos3f, nullptr, a_vertNumber, a_triIndices, a_indNumber, BuildOptions(a_qualityLevel), vByteStride);
}
uint32_t BVHRT::AddGeom_Triangles3f(const float* a_vpos3f, const float* a_vnorm3f, size_t a_vertNumber, const uint32_t* a_triIndices, 
                                    size_t a_indNumber, BuildOptions a_qualityLevel, size_t vByteStride)
{
  const size_t vStride = vByteStride / 4;
  assert(vByteStride % 4 == 0);

  const uint32_t currGeomId = uint32_t(m_geomData.size());
  const size_t oldSizeVert  = m_vertPos.size();
  const size_t oldSizeInd   = m_indices.size();

  m_vertPos.resize(oldSizeVert + a_vertNumber);
  m_vertNorm.resize(oldSizeVert + a_vertNumber);

  Box4f bbox;
  for (size_t i = 0; i < a_vertNumber; i++)
  {
    const float4 v = float4(a_vpos3f[i * vStride + 0], a_vpos3f[i * vStride + 1], a_vpos3f[i * vStride + 2], 1.0f);
    m_vertPos[oldSizeVert + i] = v;
    m_vertNorm[oldSizeVert + i] = a_vnorm3f ? float4(a_vnorm3f[i * vStride + 0], a_vnorm3f[i * vStride + 1], a_vnorm3f[i * vStride + 2], 1.0f) : float4(1.0f, 0.0f, 0.0f, 1.0f);
    bbox.include(v);
  }

  // Build BVH for each geom and append it to big buffer;
  // append data to global arrays and fix offsets

  //m_abstractObjects.resize(m_abstractObjects.size() + 1); 
  //new (m_abstractObjects.data() + m_abstractObjects.size() - 1) GeomDataTriangle();
  //m_abstractObjects.back().geomId = m_abstractObjects.size() - 1;
  //m_abstractObjects.back().m_tag = type_to_tag(TYPE_MESH_TRIANGLE);

  m_geomData.emplace_back();
  m_geomData.back().boxMin = bbox.boxMin;
  m_geomData.back().boxMax = bbox.boxMax;
  m_geomData.back().offset = uint2(oldSizeInd, oldSizeVert);
  m_geomData.back().bvhOffset = m_allNodePairs.size();
  m_geomData.back().type = TYPE_MESH_TRIANGLE;

  m_geomTags.push_back(TAG_TRIANGLE);

  std::vector<unsigned> startCount;
  
  auto presets = BuilderPresetsFromString(m_buildName.c_str());
  auto layout  = LayoutPresetsFromString(m_layoutName.c_str());
  auto bvhData = BuildBVHFat((const float*)(m_vertPos.data() + oldSizeVert), a_vertNumber, 16, a_triIndices, a_indNumber, startCount, presets, layout);

  AppendTreeData(bvhData.nodes, bvhData.indices, a_triIndices, a_indNumber);

  const size_t oldSize = m_primIdCount.size();
  m_primIdCount.insert(m_primIdCount.end(), startCount.begin(), startCount.end());
  startEnd.push_back(uint2(uint32_t(oldSize), uint32_t(m_primIdCount.size())));

  return uint32_t(startEnd.size() - 1);
}

void BVHRT::UpdateGeom_Triangles3f(uint32_t a_geomId, const float *a_vpos3f, size_t a_vertNumber,
                                   const uint32_t *a_triIndices, size_t a_indNumber, uint32_t a_flags, size_t vByteStride)
{
  UpdateGeom_Triangles3f(a_geomId, a_vpos3f, nullptr, a_vertNumber, a_triIndices, a_indNumber, BuildOptions(a_flags), vByteStride);
}

void BVHRT::UpdateGeom_Triangles3f(uint32_t a_geomId, const float *a_vpos3f, const float *a_vnorm3f, size_t a_vertNumber,
                                   const uint32_t *a_triIndices, size_t a_indNumber, BuildOptions a_qualityLevel, size_t vByteStride)
{
  if (m_geomData.size() <= a_geomId)
  {
    printf("[UpdateGeom_Triangles3f] Invalid geometry id\n");
    return;
  }

  const size_t vStride = vByteStride / 4;
  assert(vByteStride % 4 == 0);

  GeomData &geomData = m_geomData[a_geomId];
  uint32_t vertOffset = geomData.offset.x;
  uint32_t indOffset = geomData.offset.y;
  uint32_t bvhOffset = geomData.bvhOffset;
  uint32_t primIdCountOffset = startEnd[a_geomId].x;
  uint32_t primIndicesOffset = indOffset/3; //???

  //TODO: check if we are not damaging other models
  if (vertOffset + a_vertNumber > m_vertPos.size() &&
      indOffset + a_indNumber > m_indices.size())
  {
    printf("[UpdateGeom_Triangles3f] Cannot update geometry with model larger than existing one\n");
    return;
  }

  // update vertices
  Box4f bbox;
  for (size_t i = 0; i < a_vertNumber; i++)
  {
    const float4 v = float4(a_vpos3f[i * vStride + 0], a_vpos3f[i * vStride + 1], a_vpos3f[i * vStride + 2], 1.0f);
    m_vertPos[vertOffset + i] = v;
    m_vertNorm[indOffset + i] = a_vnorm3f ? float4(a_vnorm3f[i * vStride + 0], a_vnorm3f[i * vStride + 1], a_vnorm3f[i * vStride + 2], 1.0f) : float4(1.0f, 0.0f, 0.0f, 1.0f);
    bbox.include(v);
  }

  // update bbox
  geomData.boxMin = bbox.boxMin;
  geomData.boxMax = bbox.boxMax;

  std::vector<unsigned> startCount;

  auto presets = BuilderPresetsFromString(m_buildName.c_str());
  auto layout = LayoutPresetsFromString(m_layoutName.c_str());
  auto bvhData = BuildBVHFat((const float *)(m_vertPos.data() + vertOffset), a_vertNumber, 16, a_triIndices, a_indNumber, startCount, presets, layout);

  if (bvhData.nodes.size() + bvhOffset > m_allNodePairs.size())
  {
    printf("[UpdateGeom_Triangles3f] Warning - resizing m_allNodePairs, it won't work on GPU this way\n");
    m_allNodePairs.resize(bvhData.nodes.size() + bvhOffset);
  }
  if (startCount.size() + primIdCountOffset > m_primIdCount.size())
  {
    printf("[UpdateGeom_Triangles3f] Warning - resizing primIdCountOffset, it won't work on GPU this way\n");
    m_primIdCount.resize(startCount.size() + primIdCountOffset);
  }

  for (size_t i = 0; i < bvhData.nodes.size(); i++)
    m_allNodePairs[bvhOffset + i] = bvhData.nodes[i];
  for (size_t i = 0; i < bvhData.indices.size(); i++)
    m_primIndices[primIndicesOffset + i] = bvhData.indices[i];
  for (size_t i = 0; i < startCount.size(); i++)
    m_primIdCount[primIdCountOffset + i] = startCount[i];

  for(size_t i=0;i<bvhData.indices.size();i++)
  {
    const uint32_t triId = bvhData.indices[i];
    m_indices[indOffset + 3*i+0] = a_triIndices[triId*3+0];
    m_indices[indOffset + 3*i+1] = a_triIndices[triId*3+1];
    m_indices[indOffset + 3*i+2] = a_triIndices[triId*3+2];
  }
}

uint32_t BVHRT::AddGeom_AABB(uint32_t a_typeId, const CRT_AABB* boxMinMaxF8, size_t a_boxNumber, uint32_t a_buildFlags, void** a_customPrimPtrs, size_t a_customPrimCount)
{
  // append data to global arrays and fix offsets
  auto presets = BuilderPresetsFromString(m_buildName.c_str());
  auto layout  = LayoutPresetsFromString(m_layoutName.c_str());
  auto bvhData = BuildBVHFatCustom((const BVHNode*)boxMinMaxF8, a_boxNumber, presets, layout);
  
  m_allNodePairs.insert(m_allNodePairs.end(), bvhData.nodes.begin(), bvhData.nodes.end());

  const size_t oldSize = m_primIdCount.size();
  m_primIdCount.resize(oldSize + a_boxNumber);
  for (int i=0; i<a_boxNumber; i++)
    m_primIdCount[oldSize+i] = i;
  startEnd.push_back(uint2(uint32_t(oldSize), uint32_t(m_primIdCount.size())));
  
  m_geomTags.push_back(a_typeId);

  return uint32_t(startEnd.size() - 1);
}

void BVHRT::UpdateGeom_AABB(uint32_t a_geomId, uint32_t a_typeId, const CRT_AABB* boxMinMaxF8, size_t a_boxNumber, uint32_t a_buildFlags, void** a_customPrimPtrs, size_t a_customPrimCount)
{
  std::cout << "[BVHRT::UpdateGeom_AABB]: " << "not implemeted!" << std::endl; // not planned for this implementation (possible in general)
}

uint32_t BVHRT::AddGeom_SdfGrid(SdfGridView grid, ISceneObject *fake_this, BuildOptions a_qualityLevel)
{
  assert(grid.size.x*grid.size.y*grid.size.z > 0);
  assert(grid.size.x*grid.size.y*grid.size.z < (1u<<28)); //huge grids shouldn't be here
  //SDF grid is always a unit cube
  float4 mn = float4(-1,-1,-1,1);
  float4 mx = float4( 1, 1, 1,1);

  //fill geom data array
  //m_abstractObjects.resize(m_abstractObjects.size() + 1); 
  //new (m_abstractObjects.data() + m_abstractObjects.size() - 1) GeomDataSdfGrid();
  //m_abstractObjects.back().geomId = m_abstractObjects.size() - 1;
  //m_abstractObjects.back().m_tag = type_to_tag(TYPE_SDF_GRID);

  m_geomData.emplace_back();
  m_geomData.back().boxMin = mn;
  m_geomData.back().boxMax = mx;
  m_geomData.back().offset = uint2(m_SdfGridOffsets.size(), 0);
  m_geomData.back().bvhOffset = m_allNodePairs.size();
  m_geomData.back().type = TYPE_SDF_GRID;

  //m_abstractObjectPtrs.push_back(m_abstractObjects.data() + m_abstractObjects.size() - 1); // WARNING! THSI ASSUME WE DON't realloc m_abstractObjects array!!! 
  //void* pPointer = m_abstractObjectPtrs.back();

  //fill grid-specific data arrays
  m_SdfGridOffsets.push_back(m_SdfGridData.size());
  m_SdfGridSizes.push_back(grid.size);
  m_SdfGridData.insert(m_SdfGridData.end(), grid.data, grid.data + grid.size.x*grid.size.y*grid.size.z);

  //create list of bboxes for BLAS
  std::vector<BVHNode> orig_nodes = GetBoxes_SdfGrid(grid);
  
  return fake_this->AddGeom_AABB(TAG_SDF_GRID, (const CRT_AABB*)orig_nodes.data(), orig_nodes.size(), (uint32_t)a_qualityLevel, nullptr, 1); // &pPointer, 1);
}

uint32_t BVHRT::AddGeom_SdfFrameOctree(SdfFrameOctreeView octree, ISceneObject *fake_this, BuildOptions a_qualityLevel)
{
  assert(octree.size > 0);
  assert(octree.size < (1u<<28)); //huge grids shouldn't be here
  //SDF octree is always a unit cube
  float4 mn = float4(-1,-1,-1,1);
  float4 mx = float4( 1, 1, 1,1);

  //fill geom data array
  //m_abstractObjects.resize(m_abstractObjects.size() + 1); 
  //new (m_abstractObjects.data() + m_abstractObjects.size() - 1) GeomDataSdfNode();
  //m_abstractObjects.back().geomId = m_abstractObjects.size() - 1;
  //m_abstractObjects.back().m_tag = type_to_tag(TYPE_SDF_FRAME_OCTREE);

  m_geomData.emplace_back();
  m_geomData.back().boxMin = mn;
  m_geomData.back().boxMax = mx;
  m_geomData.back().offset = uint2(m_SdfFrameOctreeRoots.size(), 0);
  m_geomData.back().bvhOffset = m_allNodePairs.size();
  m_geomData.back().type = TYPE_SDF_FRAME_OCTREE;

  //fill octree-specific data arrays
  unsigned n_offset = m_SdfFrameOctreeNodes.size();
  m_SdfFrameOctreeRoots.push_back(n_offset);
  m_SdfFrameOctreeNodes.insert(m_SdfFrameOctreeNodes.end(), octree.nodes, octree.nodes + octree.size);
  for (int i=n_offset;i<m_SdfFrameOctreeNodes.size();i++)
    m_SdfFrameOctreeNodes[i].offset += n_offset;

  //create list of bboxes for BLAS
  std::vector<BVHNode> orig_nodes = GetBoxes_SdfFrameOctree(octree);
  m_origNodes = orig_nodes;
  
  return fake_this->AddGeom_AABB(TAG_SDF_NODE, (const CRT_AABB*)orig_nodes.data(), orig_nodes.size(), (uint32_t)a_qualityLevel, nullptr, 1);
}

uint32_t BVHRT::AddGeom_SdfSVS(SdfSVSView octree, ISceneObject *fake_this, BuildOptions a_qualityLevel)
{
  assert(octree.size > 0);
  assert(octree.size < (1u<<28)); //huge grids shouldn't be here
  //SDF octree is always a unit cube
  float4 mn = float4(-1,-1,-1,1);
  float4 mx = float4( 1, 1, 1,1);

  //choose only those node that have both positive and negative values in distance array, i.e. borders
  std::vector<SdfSVSNode> border_nodes(octree.nodes, octree.nodes+octree.size);

/*border_nodes.reserve(octree.size);
  for (int i=0;i<octree.size;i++)
  {
    float sz = octree.nodes[i].pos_z_lod_size & 0x0000FFFF;
    float d_max = 2*1.73205081f/sz;

    bool less = false;
    bool more = false;
    for (int j=0;j<8;j++)
    {
      float val = -d_max + 2.0f*d_max*(1.0f/255.0f)*((octree.nodes[i].values[j/4] >> (8*(j%4))) & 0xFF);
      if (val <= 0)
        less = true;
      else if (val >= 0)
        more = true;
    }

    if (less)
      border_nodes.push_back(octree.nodes[i]);
  }*/ 

  //fill geom data array
  //m_abstractObjects.resize(m_abstractObjects.size() + 1); 
  //new (m_abstractObjects.data() + m_abstractObjects.size() - 1) GeomDataSdfNode();
  //m_abstractObjects.back().geomId = m_abstractObjects.size() - 1;
  //m_abstractObjects.back().m_tag = type_to_tag(TYPE_SDF_SVS);

  m_geomData.emplace_back();
  m_geomData.back().boxMin = mn;
  m_geomData.back().boxMax = mx;
  m_geomData.back().offset = uint2(m_SdfSVSRoots.size(), 0);
  m_geomData.back().bvhOffset = m_allNodePairs.size();
  m_geomData.back().type = TYPE_SDF_SVS;

  //fill octree-specific data arrays
  unsigned n_offset = m_SdfSVSNodes.size();
  m_SdfSVSRoots.push_back(n_offset);
  m_SdfSVSNodes.insert(m_SdfSVSNodes.end(), border_nodes.begin(), border_nodes.end());

  //create list of bboxes for BLAS
  std::vector<BVHNode> orig_nodes(border_nodes.size());
  for (int i=0;i<border_nodes.size();i++)
  {
    float px = border_nodes[i].pos_xy >> 16;
    float py = border_nodes[i].pos_xy & 0x0000FFFF;
    float pz = border_nodes[i].pos_z_lod_size >> 16;
    float sz = border_nodes[i].pos_z_lod_size & 0x0000FFFF;
    orig_nodes[i].boxMin = float3(-1,-1,-1) + 2.0f*float3(px,py,pz)/sz;
    orig_nodes[i].boxMax = orig_nodes[i].boxMin + 2.0f*float3(1,1,1)/sz;
  }
  
  return fake_this->AddGeom_AABB(TAG_SDF_NODE, (const CRT_AABB*)orig_nodes.data(), orig_nodes.size(), (uint32_t)a_qualityLevel, nullptr, 1);
}

uint32_t BVHRT::AddGeom_SdfSBS(SdfSBSView octree, ISceneObject *fake_this, BuildOptions a_qualityLevel)
{
  assert(octree.size > 0 && octree.values_count > 0);
  assert(octree.size < (1u<<28) && octree.values_count < (1u<<28));

  uint32_t type = TYPE_SDF_SBS;
  uint32_t node_layout = octree.header.aux_data & SDF_SBS_NODE_LAYOUT_MASK;
  if (node_layout == SDF_SBS_NODE_LAYOUT_UNDEFINED || node_layout == SDF_SBS_NODE_LAYOUT_DX) //only distances
  {
    type = TYPE_SDF_SBS;
  }
  else if (node_layout == SDF_SBS_NODE_LAYOUT_DX_UV16) //textured
  {
    type = TYPE_SDF_SBS_TEX;
  }
  else if (node_layout == SDF_SBS_NODE_LAYOUT_DX_RGB8 ||     //colored
           node_layout == SDF_SBS_NODE_LAYOUT_ID32F_IRGB32F || 
           node_layout == SDF_SBS_NODE_LAYOUT_ID32F_IRGB32F_IN) 
  {
    type = TYPE_SDF_SBS_COL;
  }
  else
  {
    printf("unsupported node layout %u\n", node_layout);
  }

  //SDF octree is always a unit cube
  float4 mn = float4(-1,-1,-1,1);
  float4 mx = float4( 1, 1, 1,1);

  //fill geom data array
  //m_abstractObjects.resize(m_abstractObjects.size() + 1); 
  uint32_t typeTag = type_to_tag(type);
  //new (m_abstractObjects.data() + m_abstractObjects.size() - 1) GeomDataSdfBrick();
  //m_abstractObjects.back().geomId = m_abstractObjects.size() - 1;
  //m_abstractObjects.back().m_tag = typeTag;


  m_geomData.emplace_back();
  m_geomData.back().boxMin = mn;
  m_geomData.back().boxMax = mx;
  m_geomData.back().offset = uint2(m_SdfSBSRoots.size(), 0);
  m_geomData.back().bvhOffset = m_allNodePairs.size();
  m_geomData.back().type = type;

  //fill octree-specific data arrays
  unsigned n_offset = m_SdfSBSNodes.size();
  unsigned v_offset = m_SdfSBSData.size();
  m_SdfSBSRoots.push_back(n_offset);
  m_SdfSBSHeaders.push_back(octree.header);
  m_SdfSBSNodes.insert(m_SdfSBSNodes.end(), octree.nodes, octree.nodes + octree.size);
  m_SdfSBSData.insert(m_SdfSBSData.end(), octree.values, octree.values + octree.values_count);

  for (int i=n_offset; i<m_SdfSBSNodes.size(); i++)
    m_SdfSBSNodes[i].data_offset += v_offset;
  
  if (node_layout == SDF_SBS_NODE_LAYOUT_ID32F_IRGB32F|| 
      node_layout == SDF_SBS_NODE_LAYOUT_ID32F_IRGB32F_IN) //indexed layout reqires float values
  {
    unsigned f_offset = m_SdfSBSDataF.size();
    m_SdfSBSDataF.insert(m_SdfSBSDataF.end(), octree.values_f, octree.values_f + octree.values_f_count);

    //all integer values are indices, so we need to remap them
    for (int i=v_offset; i<m_SdfSBSData.size(); i++)
      m_SdfSBSData[i] += f_offset;
  }

  //create list of bboxes for BLAS
  std::vector<BVHNode> orig_nodes;

  //one node for each brick
    orig_nodes.resize(octree.size);
    for (int i=0;i<octree.size;i++)
    {
      float px = octree.nodes[i].pos_xy >> 16;
      float py = octree.nodes[i].pos_xy & 0x0000FFFF;
      float pz = octree.nodes[i].pos_z_lod_size >> 16;
      float sz = octree.nodes[i].pos_z_lod_size & 0x0000FFFF;

      orig_nodes[i].boxMin = float3(-1,-1,-1) + 2.0f*float3(px,py,pz)/sz;
      orig_nodes[i].boxMax = orig_nodes[i].boxMin + 2.0f*float3(1,1,1)/sz;
    }

  //edge case - one node for whole scene
  //add one more node, because embree breaks when
  //it tries to build BVH with only one node
  if (orig_nodes.size() == 1)
  {
    orig_nodes.resize(2);
    orig_nodes[1].boxMin = orig_nodes[0].boxMin - 0.001f*float3(1,1,1);
    orig_nodes[1].boxMax = orig_nodes[0].boxMin;
  }
  
  return fake_this->AddGeom_AABB(typeTag, (const CRT_AABB*)orig_nodes.data(), orig_nodes.size(), (uint32_t)a_qualityLevel, nullptr, 1);
}

uint32_t BVHRT::AddGeom_SdfFrameOctreeTex(SdfFrameOctreeTexView octree, ISceneObject *fake_this, BuildOptions a_qualityLevel)
{
  assert(octree.size > 0);
  assert(octree.size < (1u<<28)); //huge grids shouldn't be here
  //SDF octree is always a unit cube
  float4 mn = float4(-1,-1,-1,1);
  float4 mx = float4( 1, 1, 1,1);

  //fill geom data array
  //m_abstractObjects.resize(m_abstractObjects.size() + 1); 
  //new (m_abstractObjects.data() + m_abstractObjects.size() - 1) GeomDataSdfNode();
  //m_abstractObjects.back().geomId = m_abstractObjects.size() - 1;
  //m_abstractObjects.back().m_tag = type_to_tag(TYPE_SDF_FRAME_OCTREE_TEX);

  m_geomData.emplace_back();
  m_geomData.back().boxMin = mn;
  m_geomData.back().boxMax = mx;
  m_geomData.back().offset = uint2(m_SdfFrameOctreeTexRoots.size(), 0);
  m_geomData.back().bvhOffset = m_allNodePairs.size();
  m_geomData.back().type = TYPE_SDF_FRAME_OCTREE_TEX;

  //fill octree-specific data arrays
  unsigned n_offset = m_SdfFrameOctreeTexNodes.size();
  m_SdfFrameOctreeTexRoots.push_back(n_offset);
  m_SdfFrameOctreeTexNodes.insert(m_SdfFrameOctreeTexNodes.end(), octree.nodes, octree.nodes + octree.size);
  for (int i=n_offset;i<m_SdfFrameOctreeTexNodes.size();i++)
    m_SdfFrameOctreeTexNodes[i].offset += n_offset;

  //create list of bboxes for BLAS
  std::vector<BVHNode> orig_nodes = GetBoxes_SdfFrameOctreeTex(octree);
  m_origNodes = orig_nodes;
  
  return fake_this->AddGeom_AABB(TAG_SDF_NODE, (const CRT_AABB*)m_origNodes.data(), m_origNodes.size());
}

std::vector<BVHNode> getBoxes_OpenVDB_Grid(const OpenVDB_Grid& grid)
{
  std::vector<BVHNode> nodes;
  nodes.resize(2);
  nodes[0].boxMin = float3(-1,-1,-1);
  nodes[0].boxMax = float3(0.99,0.99,0.99);
  nodes[1].boxMin = float3(0.99,0.99,0.99);
  nodes[1].boxMax = float3(1,1,1);
  return nodes;
}

std::vector<BVHNode> getBoxes_CSGDNF(const CSGDNF& dnf)
{
  std::vector<BVHNode> nodes;
  nodes.resize(2);
  nodes[0].boxMin = float3(-1,-1,-1);
  nodes[0].boxMax = float3(0.99,0.99,0.99);
  nodes[1].boxMin = float3(0.99,0.99,0.99);
  nodes[1].boxMax = float3(1,1,1);
  return nodes;
}

BVHNode getDiskAABB(float3 pt, float3 norm, float rad)
{
  BVHNode res_node{};
  norm = normalize(norm) * rad;

  float xy_len = std::sqrt(norm.x * norm.x + norm.y * norm.y);
  float xz_len = std::sqrt(norm.x * norm.x + norm.z * norm.z);
  float yz_len = std::sqrt(norm.y * norm.y + norm.z * norm.z);
  res_node.boxMin = pt - float3(yz_len, xz_len, xy_len);
  res_node.boxMax = pt + float3(yz_len, xz_len, xy_len);
  return res_node;
}

uint32_t BVHRT::AddGeom_GraphicsPrim(const GraphicsPrimView &prim_view, ISceneObject *fake_this, BuildOptions a_qualityLevel)
{
  float3 mn = float3( 1, 1, 1);
  float3 mx = float3(-1,-1,-1);

  std::vector<BVHNode> orig_nodes;

  //m_abstractObjects.resize(m_abstractObjects.size() + 1); 
  //new (m_abstractObjects.data() + m_abstractObjects.size() - 1) GeomDataGraphicsPrim();
  //m_abstractObjects.back().geomId = m_abstractObjects.size() - 1;
  //m_abstractObjects.back().m_tag = TAG_GRAPHICS_PRIM;

  m_geomData.emplace_back();
  m_geomData.back().offset = uint2(m_GraphicsPrimHeaders.size(), m_GraphicsPrimPoints.size());
  m_geomData.back().bvhOffset = m_allNodePairs.size();
  m_geomData.back().type = TYPE_GRAPHICS_PRIM;

  m_GraphicsPrimHeaders.push_back(prim_view.header);
  m_GraphicsPrimPoints.insert(m_GraphicsPrimPoints.end(), prim_view.points, prim_view.points + prim_view.size);


  uint32_t arr_step = 2u;
  if (prim_view.header.prim_type == GRAPH_PRIM_POINT)
    arr_step = 1u;
  if (prim_view.header.prim_type >= GRAPH_PRIM_LINE_COLOR)
    arr_step = 3u;
  
  for (uint32_t i = 0u; i < prim_view.size; i += arr_step)
  {
    BVHNode new_node{};
    if (prim_view.header.prim_type == GRAPH_PRIM_POINT ||
        prim_view.header.prim_type == GRAPH_PRIM_POINT_COLOR)
    {
      float4 pt_rad = prim_view.points[i];
      new_node.boxMin = to_float3(pt_rad) - pt_rad.w;
      new_node.boxMax = to_float3(pt_rad) + pt_rad.w;
    }
    else if (prim_view.header.prim_type == GRAPH_PRIM_LINE ||
             prim_view.header.prim_type == GRAPH_PRIM_LINE_COLOR)
    {
      float4 pt1_rad = prim_view.points[i  ];
      float4 pt2     = prim_view.points[i+1];

      float3 pos = to_float3(pt2);
      float3 dir = normalize(to_float3(pt1_rad) - pos);
      float3 inv_dir = float3(dir.x > 1e-4f ? 1.f / dir.x : 0.f,
                              dir.y > 1e-4f ? 1.f / dir.y : 0.f,
                              dir.z > 1e-4f ? 1.f / dir.z : 0.f);

      // line-box intersection
      float2 t{0.f};
      bool pos_on_positive1 = false;
      bool pos_on_negative1 = false;

      for (int axis = 0; axis < 3; ++axis)
      {
        if (dir[axis] > 1e-4f)
        {
          float dist = 1.f - pos[axis];
          if (std::abs(dist) < 1e-4f)
            pos_on_positive1 = true;
          else
          {
            float t1 = dist / dir[axis];
            if (t.x == 0.f || std::abs(t1) < std::abs(t.x))
              t.x = t1;
          }

          dist = -1.f - pos[axis];
          if (std::abs(dist) < 1e-4f)
            pos_on_negative1 = true;
          else
          {
            float t2 = dist / dir[axis];
            if (t.y == 0.f || std::abs(t2) < std::abs(t.y))
              t.y = t2;
          }
        }
      }

      float3 intersect1 = pos_on_positive1 ? pos : pos + t.x * dir;
      float3 intersect2 = pos_on_negative1 ? pos : pos + t.y * dir;

      new_node = getDiskAABB(intersect1, dir, pt1_rad.w);

      BVHNode tmp_node = getDiskAABB(intersect2, dir, pt1_rad.w);
      new_node.boxMin = min(new_node.boxMin, tmp_node.boxMin);
      new_node.boxMax = max(new_node.boxMax, tmp_node.boxMax);
    }
    else if (prim_view.header.prim_type == GRAPH_PRIM_LINE_SEGMENT ||
             prim_view.header.prim_type == GRAPH_PRIM_LINE_SEGMENT_COLOR)
    {
      float4 pt1_rad = prim_view.points[i  ];
      float4 pt2     = prim_view.points[i+1];
      float3 dir = normalize(to_float3(pt2 - pt1_rad));

      new_node = getDiskAABB(to_float3(pt2), dir, pt1_rad.w);

      BVHNode tmp_node = getDiskAABB(to_float3(pt1_rad), dir, pt1_rad.w);
      new_node.boxMin = min(new_node.boxMin, tmp_node.boxMin);
      new_node.boxMax = max(new_node.boxMax, tmp_node.boxMax);
    }
    else if (prim_view.header.prim_type == GRAPH_PRIM_LINE_SEGMENT_DIR ||
             prim_view.header.prim_type == GRAPH_PRIM_LINE_SEGMENT_DIR_COLOR)
    {
      float4 pt1_rad = prim_view.points[i];
      float3 pt2 = to_float3(prim_view.points[i+1]);
      float3 pt1 = to_float3(pt1_rad);
      float3 dir = pt2 - pt1;

      float cone_ra = length(pt2 - pt1) * 0.15f;
      float3 pa = pt2 * 0.7f + pt1 * 0.3f;

      new_node = getDiskAABB(pt1, dir, pt1_rad.w); // arrow start
      new_node.boxMin = min(new_node.boxMin, pt2); // arrow point
      new_node.boxMax = max(new_node.boxMax, pt2); // arrow point

      BVHNode tmp_node = getDiskAABB(pa, dir, cone_ra);
      new_node.boxMin = min(new_node.boxMin, tmp_node.boxMin); // arrow cone
      new_node.boxMax = max(new_node.boxMax, tmp_node.boxMax); // arrow cone
    }
    else if (prim_view.header.prim_type == GRAPH_PRIM_BOX ||
             prim_view.header.prim_type == GRAPH_PRIM_BOX_COLOR)
    {
      float4 pt_min_rad = prim_view.points[i];
      float3 pt_max = to_float3(prim_view.points[i+1]);
      new_node.boxMin = min(to_float3(pt_min_rad), pt_max) - pt_min_rad.w;
      new_node.boxMax = max(to_float3(pt_min_rad), pt_max) + pt_min_rad.w;
    }

    mn = min(mn, new_node.boxMin);
    mx = max(mx, new_node.boxMax);
    orig_nodes.push_back(new_node);
  }

  if (orig_nodes.size() == 1)
  {
    orig_nodes.resize(2);
    orig_nodes[1].boxMin = orig_nodes[0].boxMin - 0.001f;
    orig_nodes[1].boxMax = orig_nodes[0].boxMin;
  }

  m_geomData.back().boxMin = to_float4(mn, 1);
  m_geomData.back().boxMax = to_float4(mx, 1);

  return fake_this->AddGeom_AABB(TAG_GRAPHICS_PRIM, (const CRT_AABB*)orig_nodes.data(), orig_nodes.size(), (uint32_t)a_qualityLevel, nullptr, 1);
}

uint32_t BVHRT::AddGeom_COctreeV3(COctreeV3View octree, unsigned bvh_level, ISceneObject *fake_this, BuildOptions a_qualityLevel)
{
#ifdef ON_CPU
  assert(COctreeV3::VERSION == 5); //if version is changed, this function should be revisited, as some changes may be needed
#endif

  scom::initialize_rot_modifiers(m_SdfCompactOctreeRotModifiers, octree.header.brick_size + 2 * octree.header.brick_pad + 1);
  scom::initialize_rot_modifiers(m_SdfCompactOctreeRotModifiers, octree.header.brick_size + 2 * octree.header.brick_pad);
  scom::initialize_rot_modifiers(m_SdfCompactOctreeRotModifiers, 2); //SCom Tree is always octree, so 2x2x2 children in each node

  assert(octree.size > 0);
  assert(octree.size < (1u<<28)); //huge grids shouldn't be here

  coctree_v3_header = octree.header;

  //SDF octree is always a unit cube
  float4 mn = float4(-1,-1,-1,1);
  float4 mx = float4( 1, 1, 1,1);

  //fill geom data array
  //m_abstractObjects.resize(m_abstractObjects.size() + 1); 
  //new (m_abstractObjects.data() + m_abstractObjects.size() - 1) GeomDataCOctreeBricked();
  //m_abstractObjects.back().geomId = m_abstractObjects.size() - 1;
  //m_abstractObjects.back().m_tag = type_to_tag(TYPE_COCTREE_V3);

  m_geomData.emplace_back();
  m_geomData.back().boxMin = mn;
  m_geomData.back().boxMax = mx;
  m_geomData.back().offset = uint2(0, 0);
  m_geomData.back().bvhOffset = m_allNodePairs.size();
  m_geomData.back().type = TYPE_COCTREE_V3;

  //fill octree-specific data arrays
  m_SdfCompactOctreeV3Data = std::vector<uint32_t>(octree.data, octree.data + octree.size);

  //list of bboxes for BLAS
  std::vector<BVHNode> orig_nodes = GetBoxes_COctreeV3(octree, bvh_level, 0, float3(0,0,0), 1.0f, 0);

  if (orig_nodes.size() < 2)
  {
    orig_nodes.resize(2);
    orig_nodes[0].boxMin = float3(-1,-1,-1);
    orig_nodes[0].boxMax = float3(1,1,1);
    orig_nodes[0].leftOffset = 0;

    orig_nodes[1].boxMin = float3(-1.0001,-1.0001,-1.0001);
    orig_nodes[1].boxMax = float3(-1,-1,-1);
    orig_nodes[1].leftOffset = 0;
  }

  m_origNodes = orig_nodes;
  
  return fake_this->AddGeom_AABB(type_to_tag(TYPE_COCTREE_V3), (const CRT_AABB*)orig_nodes.data(), orig_nodes.size(), (uint32_t)a_qualityLevel, nullptr, 1);
}

uint32_t BVHRT::AddGeom_SdfMultiOctree(SdfMultiOctreeView octree, ISceneObject *fake_this, BuildOptions a_qualityLevel)
{
  assert(octree.nodes_size > 0);
  assert(octree.values_size > 0);
  assert(octree.nodes_size < (1u<<28)); //huge grids shouldn't be here

  //SDF octree is always a unit cube
  float4 mn = float4(-1,-1,-1,1);
  float4 mx = float4( 1, 1, 1,1);

  //fill geom data array
  //m_abstractObjects.resize(m_abstractObjects.size() + 1); 
  //new (m_abstractObjects.data() + m_abstractObjects.size() - 1) GeomDataSdfOctree();
  //m_abstractObjects.back().geomId = m_abstractObjects.size() - 1;
  //m_abstractObjects.back().m_tag = type_to_tag(TYPE_SDF_MULTI_OCTREE);

  unsigned n_offset = m_SdfMultiOctreeNodes.size();
  unsigned v_offset = m_SdfMultiOctreeValues.size();

  m_geomData.emplace_back();
  m_geomData.back().boxMin = mn;
  m_geomData.back().boxMax = mx;
  m_geomData.back().offset = uint2(n_offset, 0);
  m_geomData.back().bvhOffset = m_allNodePairs.size();
  m_geomData.back().type = TYPE_SDF_MULTI_OCTREE;

  //fill octree-specific data arrays
  m_SdfMultiOctreeNodes.insert(m_SdfMultiOctreeNodes.end(), octree.nodes, octree.nodes + octree.nodes_size);
  m_SdfMultiOctreeValues.insert(m_SdfMultiOctreeValues.end(), octree.values, octree.values + octree.values_size);
  for (int i=n_offset;i<m_SdfMultiOctreeNodes.size();i++)
  {
    if (m_SdfMultiOctreeNodes[i].children_offset > 0)
      m_SdfMultiOctreeNodes[i].children_offset += n_offset;
    m_SdfMultiOctreeNodes[i].data_offset     += v_offset;
  }

  //list of bboxes for BLAS
  std::vector<BVHNode> orig_nodes;
  orig_nodes.resize(2);
  orig_nodes[0].boxMin = float3(-1, -1, -1);
  orig_nodes[0].boxMax = float3(1, 1, 1);
  orig_nodes[0].leftOffset = 0;

  orig_nodes[1].boxMin = float3(-1.0001, -1.0001, -1.0001);
  orig_nodes[1].boxMax = float3(-1, -1, -1);
  orig_nodes[1].leftOffset = 0;

  return fake_this->AddGeom_AABB(type_to_tag(TYPE_SDF_MULTI_OCTREE), (const CRT_AABB*)orig_nodes.data(), orig_nodes.size(), (uint32_t)a_qualityLevel, nullptr, 1);
}

uint32_t BVHRT::AddGeom_SdfDAG(const SdfDAG &dag, ISceneObject *fake_this, BuildOptions a_qualityLevel)
{
  assert(dag.nodes.size() > 0);
  assert(dag.child_edges.size() > 0);
  assert(dag.data_edges.size() > 0);
  assert(dag.distances.size() > 0);

  scom::initialize_rot_transforms(m_SdfCompactOctreeRotTransforms, dag.header.brick_size + 1);
  scom::initialize_rot_modifiers(m_SdfCompactOctreeRotModifiers, dag.header.node_grid_size);
  scom::initialize_rot_transforms(m_SdfCompactOctreeRotTransforms, 2); //for channels
  const scom2::Subgroup *bricks_sg = scom2::create_subgroup((scom2::TransformSubgroup)dag.header.transform_subgroup,
                                                            dag.header.brick_size + 2 * dag.header.brick_pad + 1);
  const scom2::Subgroup *branches_sg = scom2::create_subgroup((scom2::TransformSubgroup)dag.header.transform_subgroup,
                                                              dag.header.node_grid_size);

  if (bricks_sg->elements.size() != branches_sg->elements.size())
  {
    printf("[AddGeom_SdfDAG] Bricks and branches subgroups must have same number of elements to render\n");
    assert(false);
  }
  if (bricks_sg->elements.size() != m_SdfCompactOctreeRotModifiers.size() && 
      (dag.point_channels.size() + dag.voxel_channels.size()) > 0)
  {
    printf("[AddGeom_SdfDAG] Channels cannot be properly rendered while using custom subgroups\n");
    assert(false);
  }

  m_SdfDAGTransformCount = bricks_sg->elements.size();
  m_SdfDAGBrickTranspositionOffset = m_SdfDAGTransformCount*bricks_sg->elements[0].indices.size();
  m_SdfDAGTranspositions.clear();
  m_SdfDAGTranspositions.reserve(2*m_SdfDAGTransformCount*bricks_sg->elements[0].indices.size());
  for (auto &elem : bricks_sg->elements)
    for (auto &t : elem.indices)
      m_SdfDAGTranspositions.push_back(t);
  for (auto &elem : branches_sg->elements)
    for (auto &t : elem.indices)
      m_SdfDAGTranspositions.push_back(t);

  m_RotAddTable.resize(m_SdfDAGTransformCount*m_SdfDAGTransformCount, 0);
  for (int i=0; i<m_SdfDAGTransformCount; i++)
    for (int j=0; j<m_SdfDAGTransformCount; j++)
      m_RotAddTable[i*m_SdfDAGTransformCount + j] = bricks_sg->cayley_table[j][i];

  //SDF octree is always a unit cube
  float4 mn = float4(-1,-1,-1,1);
  float4 mx = float4( 1, 1, 1,1);

  //fill geom data array
  // m_abstractObjects.resize(m_abstractObjects.size() + 1); 
  // new (m_abstractObjects.data() + m_abstractObjects.size() - 1) GeomDataSdfDAG();
  // m_abstractObjects.back().geomId = m_abstractObjects.size() - 1;
  // m_abstractObjects.back().m_tag = type_to_tag(TYPE_SDF_DAG);

  unsigned h_offset = m_SdfDAGHeaders.size();
  unsigned n_offset = m_SdfDAGNodes.size();
  unsigned ce_offset = m_SdfDAGChildEdges.size();
  unsigned de_offset = m_SdfDAGDataEdges.size();
  unsigned d_offset = m_SdfDAGDistances.size();

  m_geomData.emplace_back();
  m_geomData.back().boxMin = mn;
  m_geomData.back().boxMax = mx;
  m_geomData.back().offset = uint2(n_offset, h_offset);
  m_geomData.back().bvhOffset = m_allNodePairs.size();
  m_geomData.back().type = TYPE_SDF_DAG;

  //fill DAG-specific data arrays
  m_SdfDAGHeaders.push_back(dag.header);

  m_SdfDAGNodes.insert(m_SdfDAGNodes.end(), dag.nodes.begin(), dag.nodes.end());
  m_SdfDAGChildEdges.insert(m_SdfDAGChildEdges.end(), dag.child_edges.begin(), dag.child_edges.end());
  m_SdfDAGDataEdges.insert(m_SdfDAGDataEdges.end(), dag.data_edges.begin(), dag.data_edges.end());
  m_SdfDAGDistances.insert(m_SdfDAGDistances.end(), dag.distances.begin(), dag.distances.end());

  //change offsets inside DAG nodes
  for (int i=n_offset;i<m_SdfDAGNodes.size();i++)
  {
    if (m_SdfDAGNodes[i].channels_edge.child_offset > 0)
      m_SdfDAGNodes[i].channels_edge.child_offset += n_offset;
    if (m_SdfDAGNodes[i].children_edges_offset > 0)
      m_SdfDAGNodes[i].children_edges_offset += ce_offset;
    if (m_SdfDAGNodes[i].data_edges_offset > 0)
      m_SdfDAGNodes[i].data_edges_offset += de_offset;
  }

  //change offsets inside DAG data edges
  for (int i=de_offset;i<m_SdfDAGDataEdges.size();i++)
  {
    if (m_SdfDAGDataEdges[i].data_offset > 0)
      m_SdfDAGDataEdges[i].data_offset += d_offset;
  }

  //change offsets inside DAG child edges
  for (int i=ce_offset;i<m_SdfDAGChildEdges.size();i++)
  {
    if (m_SdfDAGChildEdges[i].child_offset > 0)
      m_SdfDAGChildEdges[i].child_offset += n_offset;
  }

  //list of bboxes for BLAS
  std::vector<BVHNode> orig_nodes;
  orig_nodes.resize(2);
  orig_nodes[0].boxMin = float3(-1, -1, -1);
  orig_nodes[0].boxMax = float3(1, 1, 1);
  orig_nodes[0].leftOffset = 0;

  orig_nodes[1].boxMin = float3(-1.0001, -1.0001, -1.0001);
  orig_nodes[1].boxMax = float3(-1, -1, -1);
  orig_nodes[1].leftOffset = 0;

  return fake_this->AddGeom_AABB(type_to_tag(TYPE_SDF_DAG), (const CRT_AABB*)orig_nodes.data(), orig_nodes.size(), (uint32_t)a_qualityLevel, nullptr, 1);
}

uint32_t BVHRT::AddGeom_SCom2(const SCom2Tree &tree, ISceneObject *fake_this, BuildOptions a_qualityLevel)
{
  assert(tree.nodes.size() > 0);
  assert(tree.bricks.size() > 0);

  scom::initialize_rot_modifiers(m_SdfCompactOctreeRotModifiers, tree.header.brick_size + 1);
  scom::initialize_rot_modifiers(m_SdfCompactOctreeRotModifiers, tree.header.brick_size);
  scom::initialize_rot_modifiers(m_SdfCompactOctreeRotModifiers, 2); //nodes are 2x2x2, SCom2 is always an octree
  scom::initialize_rot_transforms(m_SdfCompactOctreeRotTransforms, tree.header.brick_size + 1);

  const scom2::Subgroup *bricks_sg = scom2::create_subgroup(scom2::TransformSubgroup::DEFAULT, 2);
  uint32_t subgroup_size = bricks_sg->elements.size();
  assert(subgroup_size == ROT_COUNT);
  m_RotAddTable.resize(subgroup_size*subgroup_size, 0);
  for (int i=0; i<subgroup_size; i++)
    for (int j=0; j<subgroup_size; j++)
      m_RotAddTable[i*subgroup_size + j] = bricks_sg->cayley_table[j][i];

  //SDF octree is always a unit cube
  float4 mn = float4(-1,-1,-1,1);
  float4 mx = float4( 1, 1, 1,1);

  unsigned n_offset = m_SCom2Data.size();

  m_geomData.emplace_back();
  m_geomData.back().boxMin = mn;
  m_geomData.back().boxMax = mx;
  m_geomData.back().offset = uint2(n_offset, m_SCom2Headers.size());
  m_geomData.back().bvhOffset = m_allNodePairs.size();
  m_geomData.back().type = TYPE_SCOM2;

  //fill SCom2-specific data arrays
  m_SCom2Headers.push_back(tree.header);

  m_SCom2Data.insert(m_SCom2Data.end(), tree.nodes.begin(), tree.nodes.end());
  unsigned b_offset = m_SCom2Data.size();
  m_SCom2Data.insert(m_SCom2Data.end(), tree.bricks.begin(), tree.bricks.end());

  // We do not change offsets inside SCom2 nodes as it is to much 
  // hustle to pack and unpack them here. Just use offsets from 
  // m_geomData and header during rendering.
  m_SCom2Headers.back().bricks_arr_offset = b_offset;
  m_SCom2Headers.back().nodes_arr_offset = n_offset;

  //list of bboxes for BLAS
  std::vector<BVHNode> orig_nodes;
  orig_nodes.resize(2);
  orig_nodes[0].boxMin = float3(-1, -1, -1);
  orig_nodes[0].boxMax = float3(1, 1, 1);
  orig_nodes[0].leftOffset = 0;

  orig_nodes[1].boxMin = float3(-1.0001, -1.0001, -1.0001);
  orig_nodes[1].boxMax = float3(-1, -1, -1);
  orig_nodes[1].leftOffset = 0;

  return fake_this->AddGeom_AABB(type_to_tag(TYPE_SCOM2), (const CRT_AABB*)orig_nodes.data(), orig_nodes.size(), (uint32_t)a_qualityLevel, nullptr, 1);  
}

void BVHRT::set_debug_mode(bool enable)
{
  debug_cur_pixel = enable;
}

void BVHRT::ClearScene()
{
  m_instanceData.reserve(std::max<std::size_t>(reserveSize, m_instanceData.capacity()));
  m_instanceData.resize(0);

  m_firstSceneCommit = true;
}

void DebugPrintNodes(const std::vector<BVHNode>& nodes, const std::string& a_fileName)
{
  std::ofstream fout(a_fileName.c_str());

  for(size_t i=0;i<nodes.size();i++)
  {
    const auto& currBox = nodes[i];
    fout << "node[" << i << "]:" << std::endl;
    fout << "  bmin = { " << currBox.boxMin[0] << " " << currBox.boxMin[1] << " " << currBox.boxMin[2] << " } | " << currBox.leftOffset  << std::endl;
    fout << "  bmax = { " << currBox.boxMax[0] << " " << currBox.boxMax[1] << " " << currBox.boxMax[2] << " } | " << currBox.escapeIndex << std::endl;
  } 
}

void DebugPrintBoxes(const std::vector<Box4f>& nodes, const std::string& a_fileName)
{
  std::ofstream fout(a_fileName.c_str());

  for(size_t i=0;i<nodes.size();i++)
  {
    const auto& currBox = nodes[i];
    fout << "node[" << i << "]:" << std::endl;
    fout << "  bmin = { " << currBox.boxMin[0] << " " << currBox.boxMin[1] << " " << currBox.boxMin[2] << " " << currBox.boxMin[3]  << std::endl;
    fout << "  bmax = { " << currBox.boxMax[0] << " " << currBox.boxMax[1] << " " << currBox.boxMax[2] << " " << currBox.boxMax[3] << std::endl;
  } 
}

void BVHRT::CommitScene(uint32_t a_qualityLevel)
{
  assert(m_instanceData.size() > 0);

  //if there is only 1 instance, there is no need in TLAS
  if (m_instanceData.size() > 1)
  {
    std::vector<Box4f> instBoxes(m_instanceData.size());
    for (size_t i = 0; i < m_instanceData.size(); i++)
      instBoxes[i] = Box4f(m_instanceData[i].boxMin, m_instanceData[i].boxMax);
    
    BuilderPresets presets = {BVH2_LEFT_OFFSET, BVHQuality::HIGH, 1};
    m_nodesTLAS = BuildBVH((const BVHNode *)instBoxes.data(), instBoxes.size(), presets).nodes;
  }
  else
  {
    m_nodesTLAS = {BVHNode()};
    m_nodesTLAS[0].boxMin = to_float3(m_instanceData[0].boxMin);
    m_nodesTLAS[0].boxMax = to_float3(m_instanceData[0].boxMax);
    m_nodesTLAS[0].leftOffset = LEAF_BIT;
    m_nodesTLAS[0].escapeIndex = LEAF_NORMAL;
  }

  m_firstSceneCommit = false;

  //Create a vector of pointers from geom data
  //It is required to use virtual functions on GPU
  //And call different intersection shaders for each type

  //m_abstractObjectPtrs.resize(m_abstractObjects.size());
  //for (size_t i = 0; i < m_abstractObjects.size(); i++)
  //  m_abstractObjectPtrs[i] = m_abstractObjects.data() + i;
}

uint32_t BVHRT::AddInstance(uint32_t a_geomId, const float4x4 &a_matrix)
{
  if(a_geomId & CRT_GEOM_MASK_AABB_BIT) // TEMP SOLUTION !!!
    a_geomId = (a_geomId & 0x7fffffff); // TEMP SOLUTION !!!

  const auto &boxMin = m_geomData[a_geomId].boxMin;
  const auto &boxMax = m_geomData[a_geomId].boxMax;

  // (1) mult mesh bounding box vertices with matrix to form new bouding box for instance
  float4 boxVertices[8]{
      a_matrix * float4{boxMin.x, boxMin.y, boxMin.z, 1.0f},
      a_matrix * float4{boxMax.x, boxMin.y, boxMin.z, 1.0f},
      a_matrix * float4{boxMin.x, boxMax.y, boxMin.z, 1.0f},
      a_matrix * float4{boxMin.x, boxMin.y, boxMax.z, 1.0f},
      a_matrix * float4{boxMax.x, boxMax.y, boxMin.z, 1.0f},
      a_matrix * float4{boxMax.x, boxMin.y, boxMax.z, 1.0f},
      a_matrix * float4{boxMin.x, boxMax.y, boxMax.z, 1.0f},
      a_matrix * float4{boxMax.x, boxMax.y, boxMax.z, 1.0f},
  };

  Box4f newBox;
  for (size_t i = 0; i < 8; i++)
    newBox.include(boxVertices[i]);

  // (2) append bounding box and matrices
  const uint32_t oldSize = uint32_t(m_instanceData.size());
  InstanceData instance;
  instance.boxMin = newBox.boxMin;
  instance.boxMax = newBox.boxMax;
  instance.geomId = a_geomId;
  instance.transform = a_matrix;
  instance.transformInv = inverse4x4(a_matrix);
  instance.transformInvTransposed = transpose(inverse4x4(a_matrix));

  m_instanceData.push_back(instance);

  return oldSize;
}

void BVHRT::UpdateInstance(uint32_t a_instanceId, const float4x4 &a_matrix)
{
  if(a_instanceId > m_instanceData.size())
  {
    std::cout << "[BVHRT::UpdateInstance]: " << "bad instance id == " << a_instanceId << "; size == " << m_instanceData.size() << std::endl;
    return;
  }

  const uint32_t geomId = m_instanceData[a_instanceId].geomId;
  const float4 boxMin   = m_geomData[geomId].boxMin;
  const float4 boxMax   = m_geomData[geomId].boxMax;

  // (1) mult mesh bounding box vertices with matrix to form new bouding box for instance
  float4 boxVertices[8]{
      a_matrix * float4{boxMin.x, boxMin.y, boxMin.z, 1.0f},
      a_matrix * float4{boxMax.x, boxMin.y, boxMin.z, 1.0f},
      a_matrix * float4{boxMin.x, boxMax.y, boxMin.z, 1.0f},
      a_matrix * float4{boxMin.x, boxMin.y, boxMax.z, 1.0f},
      a_matrix * float4{boxMax.x, boxMax.y, boxMin.z, 1.0f},
      a_matrix * float4{boxMax.x, boxMin.y, boxMax.z, 1.0f},
      a_matrix * float4{boxMin.x, boxMax.y, boxMax.z, 1.0f},
      a_matrix * float4{boxMax.x, boxMax.y, boxMax.z, 1.0f},
  };

  Box4f newBox;
  for (size_t i = 0; i < 8; i++)
    newBox.include(boxVertices[i]);

  m_instanceData[a_instanceId].boxMin = newBox.boxMin;
  m_instanceData[a_instanceId].boxMax = newBox.boxMax;
  m_instanceData[a_instanceId].transform = a_matrix;
  m_instanceData[a_instanceId].transformInv = inverse4x4(a_matrix);
}

std::vector<BVHNode> BVHRT::GetBoxes_SdfGrid(SdfGridView grid)
{
  std::vector<BVHNode> nodes;
  nodes.resize(2);
  nodes[0].boxMin = float3(-1,-1,-1);
  nodes[0].boxMax = float3(1,1,0);
  nodes[1].boxMin = float3(-1,-1,0);
  nodes[1].boxMax = float3(1,1,1);
  return nodes;
}

void add_border_nodes_rec(const SdfFrameOctreeView &octree, std::vector<BVHNode> &nodes,
                          unsigned idx, float3 p, float d)
{
  unsigned ofs = octree.nodes[idx].offset;
  if (ofs == 0) 
  {
    bool less = false;
    bool more = false;
    for (int i=0;i<8;i++)
    {
      if (octree.nodes[idx].values[i] <= 0)
        less = true;
      else if (octree.nodes[idx].values[i] >= 0)
        more = true;
    }

    if (less && more)
    {
      float3 min_pos = 2.0f*(d*p) - 1.0f;
      float3 max_pos = min_pos + 2.0f*d*float3(1,1,1);
      nodes.emplace_back();
      nodes.back().boxMax = max_pos;
      nodes.back().boxMin = min_pos;
      nodes.back().leftOffset = idx; //just store idx here, it will be later replaced by real offset in BVHBuilder anyway
    }
  }
  else
  {
    for (int i = 0; i < 8; i++)
    {
      float ch_d = d / 2;
      float3 ch_p = 2 * p + float3((i & 4) >> 2, (i & 2) >> 1, i & 1);
      add_border_nodes_rec(octree, nodes, ofs + i, ch_p, ch_d);
    }
  }
}

std::vector<BVHNode> BVHRT::GetBoxes_SdfFrameOctree(SdfFrameOctreeView octree)
{
  std::vector<BVHNode> nodes;
  add_border_nodes_rec(octree, nodes, 0, float3(0,0,0), 1);
  return nodes;
}

void add_border_nodes_rec(const SdfFrameOctreeTexView &octree, std::vector<BVHNode> &nodes,
                          unsigned idx, float3 p, float d)
{
  unsigned ofs = octree.nodes[idx].offset;
  if (ofs == 0) 
  {
    bool less = false;
    bool more = false;
    for (int i=0;i<8;i++)
    {
      if (octree.nodes[idx].values[i] <= 0)
        less = true;
      else if (octree.nodes[idx].values[i] >= -sqrt(3)*d)
        more = true;
    }

    if (less && more)
    {
      float3 min_pos = 2.0f*(d*p) - 1.0f;
      float3 max_pos = min_pos + 2.0f*d*float3(1,1,1);
      nodes.emplace_back();
      nodes.back().boxMax = max_pos;
      nodes.back().boxMin = min_pos;
      nodes.back().leftOffset = idx; //just store idx here, it will be later replaced by real offset in BVHBuilder anyway
    }
  }
  else
  {
    for (int i = 0; i < 8; i++)
    {
      float ch_d = d / 2;
      float3 ch_p = 2 * p + float3((i & 4) >> 2, (i & 2) >> 1, i & 1);
      add_border_nodes_rec(octree, nodes, ofs + i, ch_p, ch_d);
    }
  }
}

std::vector<BVHNode> BVHRT::GetBoxes_SdfFrameOctreeTex(SdfFrameOctreeTexView octree)
{
  std::vector<BVHNode> nodes;
  add_border_nodes_rec(octree, nodes, 0, float3(0,0,0), 1);

  return nodes;
}

void BVHRT::SetPreset(const MultiRenderPreset& a_preset)
{ 
  m_preset = a_preset; 
  m_precomputedLoDThr = std::pow(2.0f, m_preset.level_of_detail) - 0.01f;

  // if we have only one SCom2 object without some features, we can use faster traversal
  // later we can extend this to other variants of SCom 
  support_fast_scom_traversal = false;
  if (m_SCom2Headers.size() == 1 && m_preset.render_mode == MULTI_RENDER_MODE_LAMBERT_NO_TEX)
  {
    auto &header = m_SCom2Headers[0];
    support_fast_scom_traversal = header.has_multi_nodes == 0 &&
                                  header.has_channels == 0 &&
                                  header.v_size == 3 &&
                                  header.links_offset == 1 &&
                                  header.child_offset_off == 0;
  }
}

MultiRenderPreset BVHRT::GetPreset() const 
{ 
  return m_preset; 
}

size_t BVHRT::get_model_size(uint32_t geomId)
{
  size_t model_size = 0ul, bvh_size = 0ul;
  GeomData geom_data = m_geomData[geomId];
  uint32_t tag = type_to_tag(geom_data.type);


  // Compute BVH size

  {
    uint32_t next_offset_geom_bvh = m_allNodePairs.size();
    if (geomId < m_geomData.size() - 1)
      next_offset_geom_bvh = m_geomData[geomId + 1].bvhOffset;
    
    bvh_size = sizeof(BVHNodePair) * (next_offset_geom_bvh - geom_data.bvhOffset);
  }

  // Compute model size

  switch (tag)
  {
    case TAG_NONE:
    {
      break;
    }
    case TAG_TRIANGLE:
    {
#ifndef DISABLE_MESH
      model_size = sizeof(uint32_t) * (m_indices.size() + m_primIndices.size())
                 + sizeof(float4) * (m_vertPos.size() + m_vertNorm.size());
#endif
      break;
    }
    case TAG_SDF_GRID:
    {
#ifndef DISABLE_SDF_GRID
      model_size = sizeof(float) * m_SdfGridData.size()
                 + sizeof(uint3) * m_SdfGridSizes.size()
                 + sizeof(uint32_t) * m_SdfGridOffsets.size();
#endif
      break;
    }
    case TAG_SDF_NODE:
    {
      if (geom_data.type == TYPE_SDF_FRAME_OCTREE)
      {
#ifndef DISABLE_SDF_FRAME_OCTREE
        model_size = sizeof(SdfFrameOctreeNode) * m_SdfFrameOctreeNodes.size();

        //It is a real size, but we pretend to use it without BVH for benchmark, so count only nodes
        //model_size = sizeof(SdfFrameOctreeNode) * m_SdfFrameOctreeNodes.size()
        //           + sizeof(uint32_t) * m_SdfFrameOctreeRoots.size()
        //           + sizeof(BVHNode) * m_origNodes.size();
#endif
      }
      else if (geom_data.type == TYPE_SDF_FRAME_OCTREE_TEX)
      {
#ifndef DISABLE_SDF_FRAME_OCTREE_TEX
        model_size = sizeof(SdfFrameOctreeTexNode) * m_SdfFrameOctreeTexNodes.size()
                   + sizeof(uint32_t) * m_SdfFrameOctreeTexRoots.size()
                   + sizeof(BVHNode) * m_origNodes.size();
#endif
      }
      else if (geom_data.type == TYPE_SDF_SVS)
      {
#ifndef DISABLE_SDF_SVS
        model_size = sizeof(SdfSVSNode) * m_SdfSVSNodes.size()
                   + sizeof(uint32_t) * m_SdfSVSRoots.size();
#endif
      }
      break;
    }
    case TAG_SDF_BRICK:
    {
#ifndef DISABLE_SDF_SBS
      model_size = sizeof(SdfSBSNode) * m_SdfSBSNodes.size()
                 + sizeof(uint32_t) * (m_SdfSBSData.size() + m_SdfSBSRoots.size())
                 + sizeof(float) * m_SdfSBSDataF.size()
                 + sizeof(SdfSBSHeader) * m_SdfSBSHeaders.size();
#endif
      break;
    }
    case TAG_GRAPHICS_PRIM:
    {
#ifndef DISABLE_GRAPHICS_PRIM
      model_size = sizeof(float4) * m_GraphicsPrimPoints.size()
                 + sizeof(GraphicsPrimHeader) * m_GraphicsPrimHeaders.size();
#endif
      break;
    }
    case TAG_COCTREE_BRICKED:
    {
#ifndef DISABLE_SDF_COCTREE_V3
      model_size = sizeof(uint32_t) * (m_SdfCompactOctreeV3Data.size())
                 + sizeof(LiteMath::int4) * m_SdfCompactOctreeRotModifiers.size();
      if (geom_data.type == TYPE_COCTREE_V3)
        model_size += sizeof(COctreeV3Header);
#endif
      break;
    }
    case TAG_SDF_OCTREE:
    {
#ifndef DISABLE_SDF_MULTI_OCTREE
      model_size = sizeof(SdfMultiOctreeNode) * m_SdfMultiOctreeNodes.size()
                 + sizeof(float) * m_SdfMultiOctreeValues.size()
                 + sizeof(BVHNode) * m_origNodes.size();
#endif
      break;
    }
    case TAG_SDF_DAG:
    {
#ifndef DISABLE_SDF_DAG
      model_size = sizeof(SdfDAGHeader) * m_SdfDAGHeaders.size() +
                   sizeof(SdfDAGNode) * m_SdfDAGNodes.size() +
                   sizeof(SdfDAGChildEdge) * m_SdfDAGChildEdges.size() +
                   sizeof(SdfDAGDataEdge) * m_SdfDAGDataEdges.size() +
                   sizeof(float) * m_SdfDAGDistances.size();
#endif 
    }
    case TAG_SCOM2:
#ifndef DISABLE_SCOM2
    {
      model_size = sizeof(SCom2Header) * m_SCom2Headers.size() +
                   sizeof(uint32_t) * m_SCom2Data.size();
    }
#endif
    default:
      break;
  }

  return model_size + bvh_size;
}

std::shared_ptr<ISceneObject> CreateSceneRT(const char* a_implName, const char* a_buildName, const char* a_layoutName)
{
  if (std::string(a_implName) == "BVH2Common")
    return std::shared_ptr<ISceneObject>(new BVHRT(a_buildName, a_layoutName));
  else
    return std::shared_ptr<ISceneObject>();
}

