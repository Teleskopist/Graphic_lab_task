#include "VolumeRenderer.h"
#include "scene_extension_volume.h"
#include "core/scene_extension.h"
#include "blk/blk.h"
#include "sdf/scom2/scom_utils.h"
#include "sdf/scom2/clustering/symmetric_groups.h"

#if defined(USE_GPU)
  #include "vk_context.h"
  #include "VolumeRenderer_slang.h"
  std::shared_ptr<VolumeRenderer> CreateVolumeRenderer_slang(vk_utils::VulkanContext a_ctx, size_t a_maxThreadsGenerated);
#endif
std::shared_ptr<VolumeRenderer> CreateVolumeRenderer(unsigned /*enum RenderDevice*/ device)
{
  static bool downgrade_warning_required = true;

  #if defined(USE_GPU)
  vk_utils::VulkanContext context;
  if (device != DEVICE_CPU) {
  //create context with maximum supported features (i.e. with HW-RTX support even if we want comp shaders)
  std::vector<const char*> requiredExtensions;
  VkPhysicalDeviceFeatures2 deviceFeatures = VolumeRenderer_slang::ListRequiredDeviceFeatures(requiredExtensions);

  //globalContextInit will initialize context on first use
  context = vk_utils::globalContextInit(requiredExtensions, false, vk_utils::CHOOSE_DEVICE_BY_NAME, &deviceFeatures);
  }
#endif
  uint32_t actual_device = device;
  
  //choose actual device
  if (device != DEVICE_CPU)
  {
    #if defined(USE_GPU)
      actual_device = DEVICE_GPU_COMP;
    #else
      actual_device = DEVICE_CPU;
    #endif
  }

  //create MultiRenderer according to actual device and AS_type
  if (actual_device == DEVICE_CPU)
  {
    return std::shared_ptr<VolumeRenderer>(new VolumeRenderer());
  }
#if defined(USE_GPU)
  else if (actual_device == DEVICE_GPU_COMP)
  {
    return CreateVolumeRenderer_slang(context, 256);
  }
#endif
  else
  {
    printf("CreateVolumeRenderer::Unsupported combination\n");
    return nullptr;
  }
  return nullptr;
}

VolumeRenderer::VolumeRenderer()
{
  m_seed = 0;
  m_backgroundColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
  m_baseColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
  m_cuttingPlane = create_disabled_plane();
  m_preset = getDefaultVolumeRenderPreset();
  m_AAFrameNum = 0;
  m_time = 0.0f;
  frame0 = 0;
  frame1 = 0;
  m_dframe = 0.0f;
  m_grid_type = TYPE_DENSITY_GRID;
}

const char *VolumeRenderer::Name() const 
{ 
  return "VolumeRenderer";
}

bool VolumeRenderer::LoadScene(const char *scenePath)
{
  LiteScene::HydraScene scene;
  scene.load(scenePath);
  return LoadScene(scene);
}

bool VolumeRenderer::LoadScene(LiteScene::HydraScene &scene)
{
  //load geometries
  uint32_t grid_count = 0;
  for (auto& geom_pair : scene.geometries)
  {
    if (geom_pair.second->type_name == LiteScene::DensityGridGeometry::get_type_name())
    {
      cast_or_create(LiteScene::DensityGridGeometry, dgrid, scene.metadata, geom_pair.second);
      if (dgrid->model.size.x == dgrid->model.size.y && dgrid->model.size.x == dgrid->model.size.z)
      {
        LoadGrid(dgrid->model.size.x, dgrid->model.data.data(), {0.0f});
        grid_count++;
      }
      else
      {
        printf("[VolumeRenderer::LoadScene] Only square grids are supported\n");
      }
    }
    else if (geom_pair.second->type_name == LiteScene::ColorDensityGridGeometry::get_type_name())
    {
      cast_or_create(LiteScene::ColorDensityGridGeometry, cgrid, scene.metadata, geom_pair.second);
      if (cgrid->model.size.x == cgrid->model.size.y && cgrid->model.size.x == cgrid->model.size.z)
      {
        LoadGrid(cgrid->model.size.x, cgrid->model.data.data(), {0.0f});
        grid_count++;
      }
      else
      {
        printf("[VolumeRenderer::LoadScene] Only square grids are supported\n");
      }
    }
    else if (geom_pair.second->type_name == LiteScene::DensityMultiGridGeometry::get_type_name())
    {
      cast_or_create(LiteScene::DensityMultiGridGeometry, dmgrid, scene.metadata, geom_pair.second);
      if (dmgrid->model.size.x == dmgrid->model.size.y && dmgrid->model.size.x == dmgrid->model.size.z)
      {
        float duration = dmgrid->model.timestamps[dmgrid->model.timestamps.size() - 1] - dmgrid->model.timestamps[0];
        for (auto &t : dmgrid->model.timestamps)
          t = (t - dmgrid->model.timestamps[0]) / duration;
        LoadGrid(dmgrid->model.size.x, dmgrid->model.data.data(), dmgrid->model.timestamps);
        grid_count++;
      }
      else
      {
        printf("[VolumeRenderer::LoadScene] Only square grids are supported\n");
      }
    }
    else if (geom_pair.second->type_name == LiteScene::ColorDensityMultiGridGeometry::get_type_name())
    {
      cast_or_create(LiteScene::ColorDensityMultiGridGeometry, cdmgrid, scene.metadata, geom_pair.second);
      if (cdmgrid->model.size.x == cdmgrid->model.size.y && cdmgrid->model.size.x == cdmgrid->model.size.z)
      {
        float duration = cdmgrid->model.timestamps[cdmgrid->model.timestamps.size() - 1] - cdmgrid->model.timestamps[0];
        for (auto &t : cdmgrid->model.timestamps)
          t = (t - cdmgrid->model.timestamps[0]) / duration;
        LoadGrid(cdmgrid->model.size.x, cdmgrid->model.data.data(), cdmgrid->model.timestamps);
        grid_count++;
      }
      else
      {
        printf("[VolumeRenderer::LoadScene] Only square grids are supported\n");
      }
    }
    else if (geom_pair.second->type_name == LiteScene::SDFDAGGeometry::get_type_name())
    {
      cast_or_create(LiteScene::SDFDAGGeometry, dag, scene.metadata, geom_pair.second);
      std::vector<float> timestamps = {0.0f};
      //4D DAG should have at least 2 timestamps to allow animation
      if (dag->model.header.dim == 4)
        timestamps = {0.0f, 1.0f};
      LoadDAG(dag->model, timestamps);
      grid_count++;
    }
    else if (geom_pair.second->type_name == LiteScene::SCom2Geometry::get_type_name())
    {
      cast_or_create(LiteScene::SCom2Geometry, scom, scene.metadata, geom_pair.second);
      std::vector<float> timestamps = {0.0f};
      //4D DAG should have at least 2 timestamps to allow animation
      if (scom->model.header.dimension == 4)
        timestamps = {0.0f, 1.0f};
      LoadSCom(scom->model, timestamps);
      grid_count++;
    }
    else
    {
      printf("[VolumeRenderer::LoadScene] Unsupported custom geometry (id = %d, type_name = %s,name = %s)\n", 
             geom_pair.first, geom_pair.second->type_name.c_str(), geom_pair.second->name.c_str());
    }
  }

  if (grid_count == 0)
  {
    printf("[VolumeRenderer::LoadScene] No grids found in scene\n");
    return false;
  }
  if (grid_count > 1)
  {
    printf("[VolumeRenderer::LoadScene] Multiple grids are not supported\n");
    return false;
  }
  return true;
}

void VolumeRenderer::SetViewport(int xStart, int yStart, int width, int height)
{
  m_width  = width;
  m_height = height;
  m_packedXY_width  = PACK_XY_BLOCK_SIZE*((width + PACK_XY_BLOCK_SIZE - 1) / PACK_XY_BLOCK_SIZE);
  m_packedXY_height = PACK_XY_BLOCK_SIZE*((height + PACK_XY_BLOCK_SIZE - 1) / PACK_XY_BLOCK_SIZE);;
  
  m_packedXY.resize(m_packedXY_width*m_packedXY_height);
  m_colorBuffer.resize(m_width*m_height);
}

void VolumeRenderer::SetAccelStruct(std::shared_ptr<ISceneObject> customAccelStruct)
{
  printf("[VolumeRenderer::SetAccelStruct] Custom acceleration structures are not supported!\n");
}

ISceneObject *VolumeRenderer::GetAccelStruct()
{ 
  return this;
}

void VolumeRenderer::GetExecutionTime(const char *funcName, float out[4])
{
  printf("[VolumeRenderer::GetExecutionTime] Not implemented\n]");
}

void VolumeRenderer::UpdateCamera(const float4x4 &worldView, const float4x4 &proj)
{
  m_proj = proj;
  m_worldView = worldView;
  m_projInv      = inverse4x4(proj);
  m_worldViewInv = inverse4x4(worldView);
}

void VolumeRenderer::SetLights(const std::vector<Light> &lights)
{

}

void VolumeRenderer::SetFrameNum(uint32_t num)
{
  m_AAFrameNum = num;
}

size_t VolumeRenderer::getSceneSize()
{
  return sizeof(float4)*m_colored_grid.size() + sizeof(float)*m_grid.size() +
         m_SdfDAGNodes.size() * sizeof(SdfDAGNode) +
         m_SdfDAGDistances.size() * sizeof(float) +
         m_SdfDAGChildEdges.size() * sizeof(SdfDAGChildEdge) +
         m_SdfDAGDataEdges.size() * sizeof(SdfDAGDataEdge) +
         m_SComNodes.size() * sizeof(uint32_t) + 
         m_SComValues.size() * sizeof(uint32_t) +
         m_SphericalHarmonics.size() * sizeof(float4);
}

void VolumeRenderer::ClearGeom()
{
  m_grid.clear();
  m_colored_grid.clear();
}

void VolumeRenderer::ClearScene()
{

}

void VolumeRenderer::CommitScene(uint32_t options)
{

}

uint32_t VolumeRenderer::AddInstance(uint32_t geomId, const float4x4 &matrix)
{
  printf("[VolumeRenderer::AddInstance] Not implemented\n");
  return INVALID_IDX;
}

void  VolumeRenderer::UpdateInstance(uint32_t instanceId, const float4x4& matrix)
{
  printf("[VolumeRenderer::UpdateInstance] Not implemented\n");
}

void VolumeRenderer::LoadGrid(uint32_t size, const float *grid, const std::vector<float> &timestamps)
{
  m_grid_type = TYPE_DENSITY_GRID;
  m_timestamps = timestamps;
  m_sz = size;
  m_vox_cnt = m_sz*m_sz*m_sz;
  m_grid.resize(m_vox_cnt*timestamps.size());
  memcpy(m_grid.data(), grid, sizeof(float)*m_sz*m_sz*m_sz*timestamps.size());
}

void VolumeRenderer::LoadGrid(uint32_t size, const float4 *grid, const std::vector<float> &timestamps)
{
  m_grid_type = TYPE_COLOR_DENSITY_GRID;
  m_timestamps = timestamps;
  m_sz = size;
  m_vox_cnt = m_sz*m_sz*m_sz;
  m_colored_grid.resize(m_vox_cnt*timestamps.size());
  memcpy(m_colored_grid.data(), grid, sizeof(float4)*m_sz*m_sz*m_sz*timestamps.size());
}

void VolumeRenderer::LoadDAG(const SdfDAG &dag, std::vector<float> timestamps)
{
  assert(dag.nodes.size() > 0);
  assert(dag.child_edges.size() > 0);
  assert(dag.data_edges.size() > 0);
  assert(dag.distances.size() > 0);

  //only default transform subgroup is supported
  assert(dag.header.transform_subgroup == 0);

  scom2::initialize_rot_modifiers(m_SdfCompactOctreeRotModifiers, dag.header.brick_size + 2 * dag.header.brick_pad + 1);
  scom2::initialize_rot_modifiers(m_SdfCompactOctreeRotModifiers, dag.header.brick_size + 2 * dag.header.brick_pad);
  scom2::initialize_rot_modifiers(m_SdfCompactOctreeRotModifiers, dag.header.node_grid_size);
  scom2::initialize_rot_transforms(m_SdfCompactOctreeRotTransforms, dag.header.brick_size + 2 * dag.header.brick_pad + 1);

  const scom2::Subgroup *bricks_sg = scom2::create_subgroup((scom2::TransformSubgroup)dag.header.transform_subgroup,
                                                            dag.header.brick_size + 2 * dag.header.brick_pad + 1, dag.header.dim);
  const scom2::Subgroup *branches_sg = scom2::create_subgroup((scom2::TransformSubgroup)dag.header.transform_subgroup,
                                                              dag.header.node_grid_size, dag.header.dim);

  if (bricks_sg->elements.size() != branches_sg->elements.size())
  {
    printf("[AddGeom_SdfDAG] Bricks and branches subgroups must have same number of elements to render\n");
    assert(false);
  }
  if (bricks_sg->elements.size() != (m_SdfCompactOctreeRotModifiers.size()/3) && 
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

  m_DAGInverseTransforms.resize(m_SdfDAGTransformCount);
  for (int i=0;i<m_SdfDAGTransformCount;i++)
    m_DAGInverseTransforms[i] = m_SdfCompactOctreeRotTransforms[branches_sg->inverse_indices[i]];

  //DAG is always a unit cube
  float4 mn = float4(-1,-1,-1,1);
  float4 mx = float4( 1, 1, 1,1);

  unsigned h_offset = m_SdfDAGHeaders.size();
  unsigned n_offset = m_SdfDAGNodes.size();
  unsigned ce_offset = m_SdfDAGChildEdges.size();
  unsigned de_offset = m_SdfDAGDataEdges.size();
  unsigned d_offset = m_SdfDAGDistances.size();

  // m_geomData.emplace_back();
  // m_geomData.back().boxMin = mn;
  // m_geomData.back().boxMax = mx;
  // m_geomData.back().offset = uint2(n_offset, h_offset);
  // m_geomData.back().bvhOffset = m_allNodePairs.size();
  // m_geomData.back().type = TYPE_SDF_DAG;

  m_timestamps = timestamps;
  m_grid_type = TYPE_SDF_DAG;

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

  if (dag.voxel_channels.size() + dag.point_channels.size() > 0)
  {
    if (dag.point_channels.size() == 0 && dag.voxel_channels.size() == 1 &&
        dag.voxel_channels[0].type == DataChannel::Type::FLOAT)
    {
      if (dag.voxel_channels[0].num_components == 3*SH_COMPONENT_COUNT)
      {
        printf("[VolumeRenderer::LoadDAG] Found channel with spherical harmonics data, %d components\n", dag.voxel_channels[0].num_components);
        m_SphericalHarmonics.resize(dag.voxel_channels[0].data_f.size()/3);
        for (int i=0; i<m_SphericalHarmonics.size(); i++)
        {
          m_SphericalHarmonics[i].x = dag.voxel_channels[0].data_f[i*3+0];
          m_SphericalHarmonics[i].y = dag.voxel_channels[0].data_f[i*3+1];
          m_SphericalHarmonics[i].z = dag.voxel_channels[0].data_f[i*3+2];
          m_SphericalHarmonics[i].w = 0.0f;
        }
      }
      else
      {
        printf("[VolumeRenderer::LoadDAG] WARNING: Found channel with incompatible spherical harmonics data, %d components, %d expected\n", 
               dag.voxel_channels[0].num_components, 3*SH_COMPONENT_COUNT);
      }
    }
    else 
    {
      printf("[VolumeRenderer::LoadDAG] WARNING: Only spherical harmonics data is supported, data channels will be ignored\n");
    }
  }
}

void VolumeRenderer::LoadSCom(const SCom2Tree &dag, std::vector<float> timestamps)
{
  // check that input DAG is not empty
  assert(dag.nodes.size() > 0);
  assert(dag.bricks.size() > 0);

  // check that it was creatated as a voxel DAG, i.e. with some specific settings
  // we assume this in rendering, which allows us to skip some checks and make 
  // rendering faster
  assert(dag.header.has_multi_nodes == false);
  assert(dag.header.has_surfaces == true);
  assert(dag.header.dimension == 3 || dag.header.dimension == 4);
  if (dag.header.dimension == 3)
    assert(dag.header.child_offset_off == 0); // PackedReferenceType::SHORT_6_8_18 expected by renderer
  if (dag.header.dimension == 4)
    assert(dag.header.child_offset_off == 1); // PackedReferenceType::LONG_9_23_32 required for 4D SCom

  scom2::initialize_rot_modifiers(m_SdfCompactOctreeRotModifiers, dag.header.brick_size + 1);
  scom2::initialize_rot_modifiers(m_SdfCompactOctreeRotModifiers, dag.header.brick_size);
  scom2::initialize_rot_modifiers(m_SdfCompactOctreeRotModifiers, 2);
  scom2::initialize_rot_transforms(m_SdfCompactOctreeRotTransforms, 2);

  const scom2::Subgroup *bricks_sg = scom2::create_subgroup(scom2::TransformSubgroup::DEFAULT, dag.header.brick_size + 1, dag.header.dimension);
  const scom2::Subgroup *branches_sg = scom2::create_subgroup(scom2::TransformSubgroup::DEFAULT, 2, dag.header.dimension);

  if (bricks_sg->elements.size() != branches_sg->elements.size())
  {
    printf("[AddGeom_SdfDAG] Bricks and branches subgroups must have same number of elements to render\n");
    assert(false);
  }
  if (bricks_sg->elements.size() != (m_SdfCompactOctreeRotModifiers.size()/3) && 
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

  m_DAGInverseTransforms.resize(m_SdfDAGTransformCount);
  for (int i=0;i<m_SdfDAGTransformCount;i++)
    m_DAGInverseTransforms[i] = m_SdfCompactOctreeRotTransforms[branches_sg->inverse_indices[i]];

  m_grid_type = TYPE_SCOM2;
  m_SComNodes = dag.nodes;
  m_SComValues = dag.bricks;
  m_header = dag.header;
  m_timestamps = timestamps;

  if (dag.voxel_channels.size() + dag.point_channels.size() > 0)
  {
    if (dag.point_channels.size() == 0 && dag.voxel_channels.size() == 1 &&
        dag.voxel_channels[0].type == DataChannel::Type::FLOAT)
    {
      if (dag.voxel_channels[0].num_components == 3*SH_COMPONENT_COUNT)
      {
        printf("[VolumeRenderer::LoadDAG] Found channel with spherical harmonics data, %d components\n", dag.voxel_channels[0].num_components);
        m_SphericalHarmonics.resize(dag.voxel_channels[0].data_f.size()/3);
        for (int i=0; i<m_SphericalHarmonics.size(); i++)
        {
          m_SphericalHarmonics[i].x = dag.voxel_channels[0].data_f[i*3+0];
          m_SphericalHarmonics[i].y = dag.voxel_channels[0].data_f[i*3+1];
          m_SphericalHarmonics[i].z = dag.voxel_channels[0].data_f[i*3+2];
          m_SphericalHarmonics[i].w = 0.0f;
        }
      }
      else
      {
        printf("[VolumeRenderer::LoadDAG] WARNING: Found channel with incompatible spherical harmonics data, %d components, %d expected\n", 
               dag.voxel_channels[0].num_components, 3*SH_COMPONENT_COUNT);
      }
    }
    else 
    {
      printf("[VolumeRenderer::LoadDAG] WARNING: Only spherical harmonics data is supported, data channels will be ignored\n");
    }
  }
}

void VolumeRenderer::nextFrame(float dt)
{
  float mult = 1.0f;
  if (m_preset.ignore_timestamps)
    mult = m_timestamps.back();
  
  m_time += mult*dt*m_preset.animation_speed;

  while (m_time > m_timestamps.back())
  {
    m_time = m_timestamps.size() == 1 ? 0.0f : m_time - m_timestamps.back();
  }

  frame1 = 0;
  while (m_time > m_timestamps[frame1])
  {
    frame1++;
  }
  frame0 = frame1 == 0 ? 0 : frame1 - 1;

  m_dframe = frame0 == frame1 ? 0.0f : (m_time - m_timestamps[frame0]) / (m_timestamps[frame1] - m_timestamps[frame0]);
}