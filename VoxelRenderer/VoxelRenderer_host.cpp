#include "VoxelRenderer.h"
#include "core/scene_extension.h"
#include "scene_extension_voxel.h"
#include "minecraft_blocks.h"
#include "blk/blk.h"
#include "sdf/scom2/scom_utils.h"
#include "sdf/scom2/clustering/symmetric_groups.h"
#include <filesystem>

REGISTER_ENUM(VoxelRenderMode,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"VOXEL_RENDER_MODE_MASK", VOXEL_RENDER_MODE_MASK},
                     {"VOXEL_RENDER_MODE_LAMBERT_NO_TEX", VOXEL_RENDER_MODE_LAMBERT_NO_TEX},
                     {"VOXEL_RENDER_MODE_LAMBERT", VOXEL_RENDER_MODE_LAMBERT},
                     {"VOXEL_RENDER_MODE_NORMAL", VOXEL_RENDER_MODE_NORMAL},
                     {"VOXEL_RENDER_MODE_PRIMITIVE", VOXEL_RENDER_MODE_PRIMITIVE},
                 }; })());

REGISTER_ENUM(OctreeTraversalMode,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"OCTREE_TRAVERSAL_MODE_DEFAULT", OCTREE_TRAVERSAL_MODE_DEFAULT},
                     {"OCTREE_TRAVERSAL_MODE_FAST", OCTREE_TRAVERSAL_MODE_FAST},
                 }; })());

#if defined(USE_GPU)
  #include "vk_context.h"
  #include "VoxelRenderer_slang.h"
  std::shared_ptr<VoxelRenderer> CreateVoxelRenderer_slang(vk_utils::VulkanContext a_ctx, size_t a_maxThreadsGenerated);
#endif
std::shared_ptr<VoxelRenderer> CreateVoxelRenderer(unsigned /*enum RenderDevice*/ device)
{
  static bool downgrade_warning_required = true;

  #if defined(USE_GPU)
  vk_utils::VulkanContext context;
  if (device != DEVICE_CPU) {
  //create context with maximum supported features (i.e. with HW-RTX support even if we want comp shaders)
  std::vector<const char*> requiredExtensions;
  VkPhysicalDeviceFeatures2 deviceFeatures = VoxelRenderer_slang::ListRequiredDeviceFeatures(requiredExtensions);

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
    return std::shared_ptr<VoxelRenderer>(new VoxelRenderer());
  }
#if defined(USE_GPU)
  else if (actual_device == DEVICE_GPU_COMP)
  {
    return CreateVoxelRenderer_slang(context, 256);
  }
#endif
  else
  {
    printf("CreateVoxelRenderer::Unsupported combination\n");
    return nullptr;
  }
  return nullptr;
}

void load_from_blk(VoxelRenderPreset &settings, const Block *block)
{
  settings = getDefaultVoxelRenderPreset();

  settings.spp = block->get_int("spp", settings.spp);
  settings.render_mode = block->get_enum("render_mode", settings.render_mode);
  settings.octree_traversal_mode = block->get_enum("octree_traversal_mode", settings.octree_traversal_mode);
}

void save_to_blk(const VoxelRenderPreset &settings, Block *block)
{
  block->set_int("spp", settings.spp);
  block->set_enum("render_mode", "VoxelRenderMode", settings.render_mode);
  block->set_enum("octree_traversal_mode", "OctreeTraversalMode", settings.octree_traversal_mode);
}

VoxelRenderer::VoxelRenderer()
{
  m_seed = 0;
  m_backgroundColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
  m_cuttingPlane = create_disabled_plane();
  m_preset = getDefaultVoxelRenderPreset();
  m_AAFrameNum = 0;
  m_data_type = TYPE_UNDEFINED;
  m_lights = {create_direct_light(float3(1,1,1), float3(2.0f/3.0f)), create_ambient_light(float3(0.0, 0.0, 0.0)),
              create_ambient_light(float3(0.25, 0.25, 0.25))};

  std::map<std::string, uint32_t> tex_name_to_id;
  std::vector<std::string> tex_names = {"missed", "stone", "grass_side_carried",
                                        "grass_carried", "dirt", "cobblestone", "planks_oak", "bedrock", "water",
                                        "lava", "sand", "gravel", "gold_ore", "iron_ore", "coal_ore", "log_oak",
                                        "log_oak_top", "snow", "leaves_oak_carried"};
  m_textures.resize(tex_names.size());
  for (int i=0;i<tex_names.size();i++) 
  {
    std::string tex_path = "scenes/textures/minecraft/" + tex_names[i] + ".png";
    if (!std::filesystem::exists(tex_path))
    {
      printf("VoxelRenderer::load_textures: texture %s not found\n", tex_path.c_str());
      continue;
    }
    std::shared_ptr<Image2D<LiteMath::float4>> pTexture1 = 
    std::make_shared<Image2D<LiteMath::float4>>(
      LiteImage::LoadImage<float4>(tex_path.c_str(), 1.0f));
    Sampler sampler;
    sampler.filter   = Sampler::Filter::NEAREST; 
    sampler.addressU = Sampler::AddressMode::CLAMP;
    sampler.addressV = Sampler::AddressMode::CLAMP;
    m_textures[i] = MakeCombinedTexture2D(pTexture1, sampler);
    tex_name_to_id[tex_names[i]] = i;
  }

  auto create_metadata_1 = [&](std::string tex_name) -> BlockMetadata 
  {
    BlockMetadata metadata;
    metadata.texIds[BLOCK_SIDE_FRONT]  = tex_name_to_id[tex_name];
    metadata.texIds[BLOCK_SIDE_LEFT]   = tex_name_to_id[tex_name];
    metadata.texIds[BLOCK_SIDE_RIGHT]  = tex_name_to_id[tex_name];
    metadata.texIds[BLOCK_SIDE_BACK]   = tex_name_to_id[tex_name];
    metadata.texIds[BLOCK_SIDE_TOP]    = tex_name_to_id[tex_name];
    metadata.texIds[BLOCK_SIDE_BOTTOM] = tex_name_to_id[tex_name];
    return metadata;
  };

  auto create_metadata_3 = [&](std::string side, std::string top, std::string bottom) -> BlockMetadata 
  {
    BlockMetadata metadata;
    metadata.texIds[BLOCK_SIDE_FRONT]  = tex_name_to_id[side];
    metadata.texIds[BLOCK_SIDE_LEFT]   = tex_name_to_id[side];
    metadata.texIds[BLOCK_SIDE_RIGHT]  = tex_name_to_id[side];
    metadata.texIds[BLOCK_SIDE_BACK]   = tex_name_to_id[side];
    metadata.texIds[BLOCK_SIDE_TOP]    = tex_name_to_id[top];
    metadata.texIds[BLOCK_SIDE_BOTTOM] = tex_name_to_id[bottom];
    return metadata;
  };

  auto create_metadata_6 = [&](std::string front, std::string left, std::string right, 
                               std::string back, std::string top, std::string bottom) -> BlockMetadata 
  {
    BlockMetadata metadata;
    metadata.texIds[BLOCK_SIDE_FRONT]  = tex_name_to_id[front];
    metadata.texIds[BLOCK_SIDE_LEFT]   = tex_name_to_id[left];
    metadata.texIds[BLOCK_SIDE_RIGHT]  = tex_name_to_id[right];
    metadata.texIds[BLOCK_SIDE_BACK]   = tex_name_to_id[back];
    metadata.texIds[BLOCK_SIDE_TOP]    = tex_name_to_id[top];
    metadata.texIds[BLOCK_SIDE_BOTTOM] = tex_name_to_id[bottom];
    return metadata;
  };

  m_blocks.resize(256, create_metadata_1("missed"));
  //if non-empty block has type 0, render it with default texture of stone, this can happen if we're rendering DAG 
  m_blocks[(int)BlockType::Air] = create_metadata_1("stone"); 
  m_blocks[(int)BlockType::Stone] = create_metadata_1("stone");
  m_blocks[(int)BlockType::Grass] = create_metadata_3("grass_side_carried", "grass_carried", "dirt");
  m_blocks[(int)BlockType::Dirt] = create_metadata_1("dirt");
  m_blocks[(int)BlockType::Cobblestone] = create_metadata_1("cobblestone");
  m_blocks[(int)BlockType::Planks] = create_metadata_1("planks");
  m_blocks[(int)BlockType::Sapling] = create_metadata_1("missed");
  m_blocks[(int)BlockType::Bedrock] = create_metadata_1("bedrock");
  m_blocks[(int)BlockType::Water] = create_metadata_1("water");
  m_blocks[(int)BlockType::WaterFlow] = create_metadata_1("water");
  m_blocks[(int)BlockType::Lava] = create_metadata_1("lava");
  m_blocks[(int)BlockType::LavaFlow] = create_metadata_1("lava");
  m_blocks[(int)BlockType::Sand] = create_metadata_1("sand");
  m_blocks[(int)BlockType::Gravel] = create_metadata_1("gravel");
  m_blocks[(int)BlockType::OreGold] = create_metadata_1("gold_ore");
  m_blocks[(int)BlockType::OreIron] = create_metadata_1("iron_ore");
  m_blocks[(int)BlockType::OreCoal] = create_metadata_1("coal_ore");
  m_blocks[(int)BlockType::Wood] = create_metadata_3("log_oak", "log_oak_top", "log_oak_top");
  m_blocks[(int)BlockType::Leaves] = create_metadata_1("leaves_oak_carried");
  m_blocks[(int)BlockType::Snow] = create_metadata_1("snow");
  //TODO: can add more
}

const char *VoxelRenderer::Name() const 
{ 
  return "VoxelRenderer";
}

bool VoxelRenderer::LoadScene(const char *scenePath)
{
  LiteScene::HydraScene scene;
  scene.load(scenePath);
  return LoadScene(scene);
}

bool VoxelRenderer::LoadScene(LiteScene::HydraScene &scene)
{
  //load geometries
  uint32_t grid_count = 0;
  for (auto& geom_pair : scene.geometries)
  {
    if (geom_pair.second->type_name == LiteScene::SDFDAGGeometry::get_type_name())
    {
      cast_or_create(LiteScene::SDFDAGGeometry, dag, scene.metadata, geom_pair.second);
      assert(dag->model.header.dim == 3);
      LoadDAG(dag->model);
      grid_count++;
    }
    else if (geom_pair.second->type_name == LiteScene::SparseVoxelOctreeGeometry::get_type_name())
    {
      cast_or_create(LiteScene::SparseVoxelOctreeGeometry, svo, scene.metadata, geom_pair.second);
      LoadSVO(svo->model.data, svo->model.header.max_level_size);
      grid_count++;
    }
    else if (geom_pair.second->type_name == LiteScene::SCom2Geometry::get_type_name())
    {
      cast_or_create(LiteScene::SCom2Geometry, scom, scene.metadata, geom_pair.second);
      assert(scom->model.header.dimension == 3);
      LoadSCom(scom->model);
      grid_count++;
    }
    else
    {
      printf("[VoxelRenderer::LoadScene] Unsupported custom geometry (id = %d, type_name = %s,name = %s)\n", 
             geom_pair.first, geom_pair.second->type_name.c_str(), geom_pair.second->name.c_str());
    }
  }

  if (grid_count == 0)
  {
    printf("[VoxelRenderer::LoadScene] No grids found in scene\n");
    return false;
  }
  if (grid_count > 1)
  {
    printf("[VoxelRenderer::LoadScene] Multiple grids are not supported\n");
    return false;
  }
  return true;
}

void VoxelRenderer::SetViewport(int xStart, int yStart, int width, int height)
{
  m_width  = width;
  m_height = height;
  m_packedXY_width  = PACK_XY_BLOCK_SIZE*((width + PACK_XY_BLOCK_SIZE - 1) / PACK_XY_BLOCK_SIZE);
  m_packedXY_height = PACK_XY_BLOCK_SIZE*((height + PACK_XY_BLOCK_SIZE - 1) / PACK_XY_BLOCK_SIZE);;
  
  m_packedXY.resize(m_packedXY_width*m_packedXY_height);
  m_colorBuffer.resize(m_width*m_height);
}

void VoxelRenderer::SetAccelStruct(std::shared_ptr<ISceneObject> customAccelStruct)
{
  printf("[VoxelRenderer::SetAccelStruct] Custom acceleration structures are not supported!\n");
}

ISceneObject *VoxelRenderer::GetAccelStruct()
{ 
  return this;
}

void VoxelRenderer::GetExecutionTime(const char *funcName, float out[4])
{
  printf("[VoxelRenderer::GetExecutionTime] Not implemented\n]");
}

void VoxelRenderer::UpdateCamera(const float4x4 &worldView, const float4x4 &proj)
{
  m_proj = proj;
  m_worldView = worldView;
  m_projInv      = inverse4x4(proj);
  m_worldViewInv = inverse4x4(worldView);
}

void VoxelRenderer::SetLights(const std::vector<Light> &lights)
{
  for (int i = 0; i < std::min(m_lights.size(), lights.size()); i++)
    m_lights[i] = lights[i];
  Update_m_lights(0, m_lights.size());
}

void VoxelRenderer::SetFrameNum(uint32_t num)
{
  m_AAFrameNum = num;
}

size_t VoxelRenderer::getSceneSize()
{
  return m_SdfDAGNodes.size() * sizeof(SdfDAGNode) +
         m_SdfDAGDistances.size() * sizeof(float) +
         m_SdfDAGChildEdges.size() * sizeof(SdfDAGChildEdge) +
         m_SdfDAGDataEdges.size() * sizeof(SdfDAGDataEdge) +
         m_SVO.size() * sizeof(uint32_t) +
         m_SComNodes.size() * sizeof(uint32_t);
}

void VoxelRenderer::ClearGeom()
{
  m_SdfDAGNodes.clear();
  m_SdfDAGDistances.clear();
  m_SdfDAGChildEdges.clear();
  m_SdfDAGDataEdges.clear();
}

void VoxelRenderer::ClearScene()
{

}

void VoxelRenderer::CommitScene(uint32_t options)
{

}

uint32_t VoxelRenderer::AddInstance(uint32_t geomId, const float4x4 &matrix)
{
  printf("[VoxelRenderer::AddInstance] Not implemented\n");
  return INVALID_IDX;
}

void  VoxelRenderer::UpdateInstance(uint32_t instanceId, const float4x4& matrix)
{
  printf("[VoxelRenderer::UpdateInstance] Not implemented\n");
}

void VoxelRenderer::LoadSVO(const std::vector<uint32_t> &data, uint32_t max_level_size)
{
  m_data_type = TYPE_SPARSE_VOXEL_OCTREE;
  m_SVO = data;
  m_SVO_max_level_size = max_level_size;
}

void VoxelRenderer::LoadDAG(const SdfDAG &dag)
{
  assert(dag.nodes.size() > 0);
  assert(dag.child_edges.size() > 0);
  assert(dag.data_edges.size() > 0);
  assert(dag.distances.size() > 0);

  //only default transform subgroup is supported
  assert(dag.header.transform_subgroup == 0);

  m_DAG_max_level_size = 1 << (scom2::calculate_DAG_max_depth_rec(dag, 0) - 1);

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

  //DAG is always a unit cube
  float4 mn = float4(-1,-1,-1,1);
  float4 mx = float4( 1, 1, 1,1);

  unsigned h_offset = m_SdfDAGHeaders.size();
  unsigned n_offset = m_SdfDAGNodes.size();
  unsigned ce_offset = m_SdfDAGChildEdges.size();
  unsigned de_offset = m_SdfDAGDataEdges.size();
  unsigned d_offset = m_SdfDAGDistances.size();

  m_data_type = TYPE_SDF_DAG;

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

  if (dag.point_channels.size() == 0 && dag.voxel_channels.size() == 1 &&
      dag.voxel_channels[0].type == DataChannel::Type::INT &&
      dag.voxel_channels[0].num_components == 1)
  {
    // DAG contain only one voxel channel, which is interpreted as block id here
    m_SdfDAGBlockIds = dag.voxel_channels[0].data_i;
    printf("created m_SdfDAGBlockIds for %d blocks\n", (int)m_SdfDAGBlockIds.size());
  }
  else if (dag.point_channels.size() > 0 || dag.voxel_channels.size() > 0)
  {
    //there are some data that we don't know how to use. Probably some SDF or CAE model was loaded
    printf("[VoxelRenderer] Warning: DAG contain some channels that cannot be used by voxel renderer\n");
  }
  else
  {
    //no channels were provided, use default
    m_SdfDAGBlockIds = {(int)BlockType::Stone};

    //set all channel offsets in DAG to 0
    for (int i=0;i<m_SdfDAGNodes.size();i++)
      m_SdfDAGNodes[i].channels_edge.child_offset = 0;
  }
}

void VoxelRenderer::LoadSCom(const SCom2Tree &dag)
{
  // check that input DAG is not empty
  assert(dag.nodes.size() > 0);
  assert(dag.bricks.size() > 0);

  // check that it was creatated as a voxel DAG, i.e. with some specific settings
  // we assume this in rendering, which allows us to skip some checks and make 
  // rendering faster
  assert(dag.voxel_channels.size() > 0);
  assert(dag.header.has_multi_nodes == false);
  assert(dag.header.has_surfaces == false);
  assert(dag.header.has_channels == true);
  assert(dag.header.dimension == 3);
  assert(dag.header.child_offset_off == 0);

  scom2::initialize_rot_modifiers(m_SdfCompactOctreeRotModifiers, dag.header.brick_size + 1);
  scom2::initialize_rot_modifiers(m_SdfCompactOctreeRotModifiers, dag.header.brick_size);
  scom2::initialize_rot_modifiers(m_SdfCompactOctreeRotModifiers, 2); //nodes are 2x2x2, SCom2 is always an octree
  scom2::initialize_rot_transforms(m_SdfCompactOctreeRotTransforms, dag.header.brick_size + 1);

  const scom2::Subgroup *bricks_sg = scom2::create_subgroup(scom2::TransformSubgroup::DEFAULT, 2);
  uint32_t subgroup_size = bricks_sg->elements.size();
  assert(subgroup_size == ROT_COUNT);
  m_RotAddTable.resize(subgroup_size*subgroup_size, 0);
  for (int i=0; i<subgroup_size; i++)
    for (int j=0; j<subgroup_size; j++)
      m_RotAddTable[i*subgroup_size + j] = bricks_sg->cayley_table[j][i];  

  SCom2Tree copy_dag = dag;
  scom2::replace_channel_links_to_values(copy_dag);

  m_data_type = TYPE_SCOM2;
  m_SComNodes = copy_dag.nodes;
  m_header = dag.header;
  m_DAG_max_level_size = 1 << (copy_dag.header.max_depth - 1);
}