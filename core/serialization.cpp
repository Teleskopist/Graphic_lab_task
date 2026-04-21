#include "core/serialization.h"
#include "utils/misc/scene_common.h"

REGISTER_ENUM(RenderDevice,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"DEVICE_CPU", DEVICE_CPU},
                     {"DEVICE_GPU", DEVICE_GPU},
                     {"DEVICE_GPU_COMP", DEVICE_GPU_COMP},
                     {"DEVICE_GPU_RQ", DEVICE_GPU_RQ},
                 }; })());

REGISTER_ENUM(SDFNodeIntersect,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"SDF_OCTREE_NODE_INTERSECT_ST", SDF_OCTREE_NODE_INTERSECT_ST},
                     {"SDF_OCTREE_NODE_INTERSECT_ANALYTIC", SDF_OCTREE_NODE_INTERSECT_ANALYTIC},
                     {"SDF_OCTREE_NODE_INTERSECT_NEWTON", SDF_OCTREE_NODE_INTERSECT_NEWTON},
                     {"SDF_OCTREE_NODE_INTERSECT_BBOX", SDF_OCTREE_NODE_INTERSECT_BBOX},
                     {"SDF_OCTREE_NODE_INTERSECT_IT", SDF_OCTREE_NODE_INTERSECT_IT},
                 }; })());

REGISTER_ENUM(MultiRenderMode,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"MULTI_RENDER_MODE_MASK", MULTI_RENDER_MODE_MASK},
                     {"MULTI_RENDER_MODE_LAMBERT_NO_TEX", MULTI_RENDER_MODE_LAMBERT_NO_TEX},
                     {"MULTI_RENDER_MODE_PBR_NO_TEX", MULTI_RENDER_MODE_PBR_NO_TEX},
                     {"MULTI_RENDER_MODE_DEPTH", MULTI_RENDER_MODE_DEPTH},
                     {"MULTI_RENDER_MODE_LINEAR_DEPTH", MULTI_RENDER_MODE_LINEAR_DEPTH},
                     {"MULTI_RENDER_MODE_INVERSE_LINEAR_DEPTH", MULTI_RENDER_MODE_INVERSE_LINEAR_DEPTH},
                     {"MULTI_RENDER_MODE_PRIMITIVE", MULTI_RENDER_MODE_PRIMITIVE},
                     {"MULTI_RENDER_MODE_TYPE", MULTI_RENDER_MODE_TYPE},
                     {"MULTI_RENDER_MODE_GEOM", MULTI_RENDER_MODE_GEOM},
                     {"MULTI_RENDER_MODE_NORMAL", MULTI_RENDER_MODE_NORMAL},
                     {"MULTI_RENDER_MODE_BARYCENTRIC", MULTI_RENDER_MODE_BARYCENTRIC},
                     {"MULTI_RENDER_MODE_ST_ITERATIONS", MULTI_RENDER_MODE_ST_ITERATIONS},
                     {"MULTI_RENDER_MODE_PHONG_NO_TEX", MULTI_RENDER_MODE_PHONG_NO_TEX},
                     {"MULTI_RENDER_MODE_TEX_COORDS", MULTI_RENDER_MODE_TEX_COORDS},
                     {"MULTI_RENDER_MODE_DIFFUSE", MULTI_RENDER_MODE_DIFFUSE},
                     {"MULTI_RENDER_MODE_LAMBERT", MULTI_RENDER_MODE_LAMBERT},
                     {"MULTI_RENDER_MODE_PHONG", MULTI_RENDER_MODE_PHONG},
                     {"MULTI_RENDER_MODE_HSV_DEPTH", MULTI_RENDER_MODE_HSV_DEPTH},
                     {"MULTI_RENDER_MODE_LOD", MULTI_RENDER_MODE_LOD},
                     {"MULTI_RENDER_MODE_PBR", MULTI_RENDER_MODE_PBR},
                     {"MULTI_RENDER_MODE_MATERIAL", MULTI_RENDER_MODE_MATERIAL},
                     {"MULTI_RENDER_MODE_CHANNEL", MULTI_RENDER_MODE_CHANNEL},
                     {"MULTI_RENDER_MODE_MO_VOXEL_COUNT", MULTI_RENDER_MODE_MO_VOXEL_COUNT},
                     {"MULTI_RENDER_MODE_MO_FLAGS", MULTI_RENDER_MODE_MO_FLAGS},
                     {"MULTI_RENDER_MODE_OUTLINE_PRIM", MULTI_RENDER_MODE_OUTLINE_PRIM},
                     {"MULTI_RENDER_MODE_OUTLINE_BORDER", MULTI_RENDER_MODE_OUTLINE_BORDER},
                     {"MULTI_RENDER_MODE_INTERSECTION_COUNT", MULTI_RENDER_MODE_INTERSECTION_COUNT},
                     {"MULTI_RENDER_MODE_CHANNEL_LAMBERT", MULTI_RENDER_MODE_CHANNEL_LAMBERT},
                 }; })());

REGISTER_ENUM(NormalMode,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"NORMAL_MODE_GEOMETRY", NORMAL_MODE_GEOMETRY},
                     {"NORMAL_MODE_VERTEX", NORMAL_MODE_VERTEX},
                     {"NORMAL_MODE_SDF_SMOOTHED", NORMAL_MODE_SDF_SMOOTHED},
                 }; })());

REGISTER_ENUM(RayGenMode,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"RAY_GEN_MODE_REGULAR", RAY_GEN_MODE_REGULAR},
                     {"RAY_GEN_MODE_RANDOM", RAY_GEN_MODE_RANDOM},
                     {"RAY_GEN_MODE_HAMMERSLEY", RAY_GEN_MODE_HAMMERSLEY},
                 }; })());

REGISTER_ENUM(InterpolationMode,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"INTERPOLATION_MODE_TRILINEAR", INTERPOLATION_MODE_TRILINEAR},
                     {"INTERPOLATION_MODE_TRICUBIC", INTERPOLATION_MODE_TRICUBIC},
                 }; })());

REGISTER_ENUM(RepresentationMode,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"REPRESENTATION_MODE_SURFACE", REPRESENTATION_MODE_SURFACE},
                     {"REPRESENTATION_MODE_VOLUME", REPRESENTATION_MODE_VOLUME},
                 }; })());       

REGISTER_ENUM(ChannelRenderMode,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"CHANNEL_RENDER_MODE_DIRECT", CHANNEL_RENDER_MODE_DIRECT},
                     {"CHANNEL_RENDER_MODE_NORMALIZED", CHANNEL_RENDER_MODE_NORMALIZED},
                     {"CHANNEL_RENDER_MODE_PALETTE", CHANNEL_RENDER_MODE_PALETTE},
                     {"CHANNEL_RENDER_MODE_CTF", CHANNEL_RENDER_MODE_CTF},
                     {"CHANNEL_RENDER_MODE_AS_RGB_COLOR", CHANNEL_RENDER_MODE_AS_RGB_COLOR},
                     {"CHANNEL_RENDER_MODE_AS_NORMAL", CHANNEL_RENDER_MODE_AS_NORMAL},
                 }; })());

REGISTER_ENUM(DemoScene,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"SINGLE_OBJECT", (unsigned)DemoScene::SINGLE_OBJECT},
                     {"CORNELL_BOX", (unsigned)DemoScene::CORNELL_BOX},
                     {"SINGLE_OBJECT_CUBEMAP", (unsigned)DemoScene::SINGLE_OBJECT_CUBEMAP},
                     {"INSTANCES_GRID", (unsigned)DemoScene::INSTANCES_GRID},
                     {"INSTANCES_LARGE_SCENE", (unsigned)DemoScene::INSTANCES_LARGE_SCENE},
                     {"INSTANCES_100K", (unsigned)DemoScene::INSTANCES_100K},
                     {"INSTANCES_ON_PLANE", (unsigned)DemoScene::INSTANCES_ON_PLANE},
                 }; })());

REGISTER_ENUM(RenderType,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"MULTI", (int)RenderType::MULTI},
                     {"HYDRA", (int)RenderType::HYDRA},
                     {"VOLUME", (int)RenderType::VOLUME},
                     {"VOXEL", (int)RenderType::VOXEL},
                     {"CSG_DIRECT", (int)RenderType::CSG_DIRECT},
                 }; })());

REGISTER_ENUM(LightType,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"LIGHT_TYPE_DIRECT", LIGHT_TYPE_DIRECT},
                     {"LIGHT_TYPE_POINT", LIGHT_TYPE_POINT},
                     {"LIGHT_TYPE_AMBIENT", LIGHT_TYPE_AMBIENT},
                 }; })());

REGISTER_ENUM(CamerasDistribution,
              ([]()
                { return std::vector<std::pair<std::string, unsigned>>{
                     {"CAMERAS_DISTRIBUTION_UNIFORM_ON_SPHERE", 0},
                     {"CAMERAS_DISTRIBUTION_SPIRAL_ON_SPHERE", 1},
                     {"CAMERAS_DISTRIBUTION_RINGS_ON_SPHERE", 2}
                 }; })());

REGISTER_ENUM(CamerasSaveFormat,
              ([]()
                { return std::vector<std::pair<std::string, unsigned>>{
                     {"CAMERAS_SAVE_FORMAT_NONE", 0},
                     {"CAMERAS_SAVE_FORMAT_CSV", 1}
                 }; })());

void force_register_common_enums()
{

}

void save_to_blk(const MultiRenderPreset &preset, Block *block)
{
  block->clear();
  block->add_enum("render_mode", "MultiRenderMode", preset.render_mode);
  block->add_enum("sdf_node_intersect", "SDFNodeIntersect", preset.sdf_node_intersect);
  block->add_enum("normal_mode", "NormalMode", preset.normal_mode);
  block->add_enum("ray_gen_mode", "RayGenMode", preset.ray_gen_mode);
  block->add_enum("interpolation_mode", "InterpolationMode", preset.interpolation_mode);
  block->add_enum("representation_mode", "RepresentationMode", preset.representation_mode);
  block->add_int("spp", preset.spp);
  block->add_int("fixed_lod", preset.fixed_lod);
  block->add_double("level_of_detail", preset.level_of_detail);
  block->add_double("compact_sdf_eps", preset.compact_sdf_eps);
  block->add_double("LOD_distance", preset.LOD_distance);

  block->add_enum("channel_render_mode", "ChannelRenderMode", preset.channel_render_mode);
  block->add_int("channel_id", preset.channel_id);
  block->add_int("component_id", preset.component_id);
  block->add_int("CTF_id", preset.CTF_id);
}
void load_from_blk(MultiRenderPreset &preset, const Block *block)
{
  preset = getDefaultPreset();

  preset.render_mode = block->get_enum("render_mode", preset.render_mode);
  preset.sdf_node_intersect = block->get_enum("sdf_node_intersect", preset.sdf_node_intersect);
  preset.normal_mode = block->get_enum("normal_mode", preset.normal_mode);
  preset.ray_gen_mode = block->get_enum("ray_gen_mode", preset.ray_gen_mode);
  preset.interpolation_mode = block->get_enum("interpolation_mode", preset.interpolation_mode);
  preset.representation_mode = block->get_enum("representation_mode", preset.representation_mode);
  preset.spp = block->get_int("spp", preset.spp);
  preset.fixed_lod = block->get_int("fixed_lod", preset.fixed_lod);
  preset.level_of_detail = block->get_double("level_of_detail", preset.level_of_detail);
  preset.compact_sdf_eps = block->get_double("compact_sdf_eps", preset.compact_sdf_eps);
  preset.LOD_distance = block->get_double("LOD_distance", preset.LOD_distance);

  preset.channel_render_mode = block->get_enum("channel_render_mode", preset.channel_render_mode);
  preset.channel_id = block->get_int("channel_id", preset.channel_id);
  preset.component_id = block->get_int("component_id", preset.component_id);
  preset.CTF_id = block->get_int("CTF_id", preset.CTF_id);
}

void load_from_blk(Camera &camera, const Block *block)
{
  camera = Camera();

  camera.pos = block->get_vec3("pos", camera.pos);
  camera.lookAt = block->get_vec3("lookAt", camera.lookAt);
  camera.up = block->get_vec3("up", camera.up);
  camera.fovy = block->get_double("fovy", camera.fovy);
  camera.z_near = block->get_double("z_near", camera.z_near);
  camera.z_far = block->get_double("z_far", camera.z_far);
}
void save_to_blk(const Camera &camera, Block *block)
{
  block->clear();

  block->add_vec3("pos", camera.pos);
  block->add_vec3("lookAt", camera.lookAt);
  block->add_vec3("up", camera.up);
  block->add_double("fovy", camera.fovy);
  block->add_double("z_near", camera.z_near);
  block->add_double("z_far", camera.z_far);
}

void load_from_blk(cmesh4::PreprocessSettings &settings, const Block *block)
{
  settings = cmesh4::PreprocessSettings();

  settings.rescale = block->get_bool("rescale", settings.rescale);
  settings.min_p = block->get_vec3("min_p", settings.min_p);
  settings.max_p = block->get_vec3("max_p", settings.max_p);

  settings.recalculate_normals = block->get_bool("recalculate_normals", settings.recalculate_normals);
  settings.verbose = block->get_bool("verbose", settings.verbose);

  settings.compress_close_vertices = block->get_bool("compress_close_vertices", settings.compress_close_vertices);
  settings.force_merge_distinct_normals = block->get_bool("force_merge_distinct_normals", settings.force_merge_distinct_normals);
  settings.close_vertices_thr = block->get_double("close_vertices_thr", settings.close_vertices_thr);

  settings.align_to_axes = block->get_bool("align_to_axes", settings.align_to_axes);
  settings.align_to_axes_tolerance = block->get_double("align_to_axes_tolerance", settings.align_to_axes_tolerance);
}

void save_to_blk(const cmesh4::PreprocessSettings &settings, Block *block)
{
  block->add_bool("rescale", settings.rescale);
  block->add_vec3("min_p", settings.min_p);
  block->add_vec3("max_p", settings.max_p);

  block->add_bool("recalculate_normals", settings.recalculate_normals);
  block->add_bool("verbose", settings.verbose);

  block->add_bool("compress_close_vertices", settings.compress_close_vertices);
  block->add_bool("force_merge_distinct_normals", settings.force_merge_distinct_normals);
  block->add_double("close_vertices_thr", settings.close_vertices_thr);

  block->add_bool("align_to_axes", settings.align_to_axes);
  block->add_double("align_to_axes_tolerance", settings.align_to_axes_tolerance);
}

void load_from_blk(Light &light, const Block *block)
{
  light = Light();
  light.color = block->get_vec3("color", light.color);
  light.space = block->get_vec3("space", light.space);
  light.type  = block->get_enum("type", light.type);
  light._pad  = 0;
}

void save_to_blk(const Light &light, Block *block)
{
  block->add_vec3("color", light.color);
  block->add_vec3("space", light.space);
  block->add_enum("type", "LightType", light.type);
}