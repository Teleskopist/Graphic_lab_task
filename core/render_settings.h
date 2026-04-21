#pragma once

//enum GeomType
static constexpr unsigned TYPE_MESH_TRIANGLE              =  0;
static constexpr unsigned TYPE_SDF_GRID                   =  1;

static constexpr unsigned TYPE_SDF_FRAME_OCTREE           =  3;

static constexpr unsigned TYPE_SDF_SVS                    =  5;
static constexpr unsigned TYPE_SDF_SBS                    =  6;

static constexpr unsigned TYPE_SDF_FRAME_OCTREE_TEX       =  8;
static constexpr unsigned TYPE_SDF_MULTI_OCTREE           =  9;
static constexpr unsigned TYPE_SDF_SBS_TEX                = 10;
static constexpr unsigned TYPE_SDF_SBS_COL                = 11;

static constexpr unsigned TYPE_CSG_TREE                   = 13;
static constexpr unsigned TYPE_CSG_ANIM_TREE              = 14;
static constexpr unsigned TYPE_CSG_OP_TREE                = 13;
static constexpr unsigned TYPE_CSG_OP_TREE_ANIM           = 14;

static constexpr unsigned TYPE_GRAPHICS_PRIM              = 17;


static constexpr unsigned TYPE_COCTREE_V3                 = 20;



static constexpr unsigned TYPE_OPENVDB_GRID               = 23;
static constexpr unsigned TYPE_CSGDNF                     = 24;
static constexpr unsigned TYPE_SDF_DAG                    = 25;
static constexpr unsigned TYPE_SCOM2                      = 26;

static constexpr unsigned TYPE_DENSITY_GRID               = 27;
static constexpr unsigned TYPE_COLOR_DENSITY_GRID         = 28;

static constexpr unsigned TYPE_SPARSE_VOXEL_OCTREE        = 29;

static constexpr unsigned TYPE_UNDEFINED                  = 255;

static constexpr unsigned SH_TYPE = 24;              //8 bits for type
static constexpr unsigned GEOM_ID_MASK = 0x00FFFFFFu;

//enum RenderDevice
static constexpr unsigned DEVICE_CPU      = 0; //CPU
static constexpr unsigned DEVICE_GPU      = 1; //GPU, DEVICE_GPU_COMP if available, DEVICE_GPU_COMP otherwise
static constexpr unsigned DEVICE_GPU_COMP = 2; //GPU, software BVH traverse in compute shader
static constexpr unsigned DEVICE_GPU_RQ   = 3; //GPU, hardware BVH traverse in compute shader

//enum RenderASType
static constexpr unsigned AS_TYPE_COMMON     = 0; //BVHRT

//enum SdfNodeIntersect
static constexpr unsigned SDF_OCTREE_NODE_INTERSECT_ST       = 0;// Sphere tracing inside node
static constexpr unsigned SDF_OCTREE_NODE_INTERSECT_ANALYTIC = 1;// Explicitly finding ray/sdf intersection inside node
static constexpr unsigned SDF_OCTREE_NODE_INTERSECT_NEWTON   = 2;// Using Newton method to find ray/sdf intersection inside node
static constexpr unsigned SDF_OCTREE_NODE_INTERSECT_BBOX     = 3;// Intersect with node bbox for debug purposes
static constexpr unsigned SDF_OCTREE_NODE_INTERSECT_IT       = 4;// Interval tracing inside node

//enum MultiRenderMode
static constexpr unsigned MULTI_RENDER_MODE_MASK                 =  0; //white object, black background
static constexpr unsigned MULTI_RENDER_MODE_LAMBERT_NO_TEX       =  1; //Lambert shading, no texture, no shadows
static constexpr unsigned MULTI_RENDER_MODE_DEPTH                =  2; //visualize depth
static constexpr unsigned MULTI_RENDER_MODE_LINEAR_DEPTH         =  3; //visualize depth, color changes linearly in predefined range
static constexpr unsigned MULTI_RENDER_MODE_INVERSE_LINEAR_DEPTH =  4; //visualize depth, color changes linearly in predefined range (inverted)
static constexpr unsigned MULTI_RENDER_MODE_PRIMITIVE            =  5; //each primitive has distinct color from palette
static constexpr unsigned MULTI_RENDER_MODE_TYPE                 =  6; //each type has distinct color from palette
static constexpr unsigned MULTI_RENDER_MODE_GEOM                 =  7; //each geodId has distinct color from palette
static constexpr unsigned MULTI_RENDER_MODE_NORMAL               =  8; //visualize normals (abs for each coordinate)
static constexpr unsigned MULTI_RENDER_MODE_BARYCENTRIC          =  9; //visualize barycentric coordinates for triangle mesh
static constexpr unsigned MULTI_RENDER_MODE_ST_ITERATIONS        = 10; //for SDFs replace primId with number of iterations for sphere tracing
static constexpr unsigned MULTI_RENDER_MODE_PBR_NO_TEX           = 11; //PBR rendering using glTF 2.0f metallic-roughness materials, no texture
static constexpr unsigned MULTI_RENDER_MODE_PHONG_NO_TEX         = 12; //Phong shading, no texture, shadows
static constexpr unsigned MULTI_RENDER_MODE_OUTLINE_PRIM         = 13; //Lambert no tex, outline on primitive border
static constexpr unsigned MULTI_RENDER_MODE_OUTLINE_BORDER       = 14; //Lambert no tex, outline on geometry border
static constexpr unsigned MULTI_RENDER_MODE_TEX_COORDS           = 15; //rendering texture coordinates in RG channels
static constexpr unsigned MULTI_RENDER_MODE_DIFFUSE              = 16; //rendering diffuse color from material
static constexpr unsigned MULTI_RENDER_MODE_LAMBERT              = 17; //Lambert shading, no shadows
static constexpr unsigned MULTI_RENDER_MODE_PHONG                = 18; //Phong shading
static constexpr unsigned MULTI_RENDER_MODE_HSV_DEPTH            = 19; //visualize depth, uses HSV color model
static constexpr unsigned MULTI_RENDER_MODE_LOD                  = 20; //LOD number
static constexpr unsigned MULTI_RENDER_MODE_PBR                  = 21; //PBR rendering using glTF 2.0f metallic-roughness materials
static constexpr unsigned MULTI_RENDER_MODE_MATERIAL             = 22; //rendering material id
static constexpr unsigned MULTI_RENDER_MODE_CHANNEL              = 23; //rendering channel value
static constexpr unsigned MULTI_RENDER_MODE_MO_VOXEL_COUNT       = 24; //voxel count in Multi Octree node
static constexpr unsigned MULTI_RENDER_MODE_MO_FLAGS             = 25; //flags in Multi Octree node
static constexpr unsigned MULTI_RENDER_MODE_INTERSECTION_COUNT   = 26; //number of call for intersection shader for this ray
static constexpr unsigned MULTI_RENDER_MODE_CHANNEL_LAMBERT      = 27; //rendering channel value with lambert shading

//enum NormalMode
static constexpr unsigned NORMAL_MODE_GEOMETRY     = 0; //geometry normal, default for all types of geometry
static constexpr unsigned NORMAL_MODE_VERTEX       = 1; //vertex normal, available only for meshes with vertex normals gives
static constexpr unsigned NORMAL_MODE_SDF_SMOOTHED = 2; //smoothed SDF normal, available only for SBS with layouts containing neighbors

//enum RayGenMode
static constexpr unsigned RAY_GEN_MODE_REGULAR    = 0;
static constexpr unsigned RAY_GEN_MODE_RANDOM     = 1;
static constexpr unsigned RAY_GEN_MODE_HAMMERSLEY = 2;

//enum InterpolationMode
static constexpr unsigned INTERPOLATION_MODE_TRILINEAR = 0;
static constexpr unsigned INTERPOLATION_MODE_TRICUBIC  = 1;

//enum RepresentationMode
static constexpr unsigned REPRESENTATION_MODE_SURFACE = 0;
static constexpr unsigned REPRESENTATION_MODE_VOLUME  = 1;

//enum ChannelRenderMode
static constexpr unsigned CHANNEL_RENDER_MODE_DIRECT       = 0; //just render the values as RGB colors (up to 3 components)
static constexpr unsigned CHANNEL_RENDER_MODE_NORMALIZED   = 1; //render scalar values normalized to [0,1] in red channel
static constexpr unsigned CHANNEL_RENDER_MODE_PALETTE      = 2; //render the colors from the palette, value must be 1-component unsigned int
static constexpr unsigned CHANNEL_RENDER_MODE_CTF          = 3; //render given component with volor based on color transfer function (CTF_id in settings)
static constexpr unsigned CHANNEL_RENDER_MODE_AS_RGB_COLOR = 4; //render 3 components of channel treating them as RGB color
static constexpr unsigned CHANNEL_RENDER_MODE_AS_NORMAL    = 5; //render 3 components of channel treating them as normal vector (like in MULTI_RENDER_MODE_NORMAL mode)

struct MultiRenderPreset
{
  unsigned render_mode;        //enum MultiRenderMode
  unsigned sdf_node_intersect; //enum SdfNodeIntersect
  unsigned normal_mode;        //enum NormalMode
  unsigned ray_gen_mode;       //enum RayGenMode
  unsigned interpolation_mode; //enum InterpolationMode
  unsigned representation_mode;//enum RepresentationMode

  unsigned spp;                //samples per pixel
  unsigned fixed_lod;          //0 or 1, use fixed level of detail or dynamic (based on distance)
  float level_of_detail;       //level of detail that you get on an object 1 meter away from you. It is inverted (worst LOD is 0!)
  float compact_sdf_eps;       //larger epsilon for intersection with compact SDFs, needed to prevent some visual artifacts
  float LOD_distance;          //distance at which original geometry is switched to LOD

  //used only when render_mode == MULTI_RENDER_MODE_CHANNEL
  unsigned channel_render_mode;//enum ChannelRenderMode
  unsigned channel_id;
  unsigned component_id;
  unsigned CTF_id;
};

static MultiRenderPreset getDefaultPreset()
{
  MultiRenderPreset p;
  p.render_mode = MULTI_RENDER_MODE_LAMBERT_NO_TEX;
  p.sdf_node_intersect = SDF_OCTREE_NODE_INTERSECT_ANALYTIC;
  p.normal_mode = NORMAL_MODE_VERTEX;
  p.ray_gen_mode = RAY_GEN_MODE_REGULAR;
  p.interpolation_mode = INTERPOLATION_MODE_TRILINEAR;
  p.representation_mode = REPRESENTATION_MODE_VOLUME;

  p.spp = 1;
  p.fixed_lod = true;
  p.level_of_detail = 16.0f; //always max LOD
  p.compact_sdf_eps = 0.05f;
  p.LOD_distance = 10.0f;

  p.channel_render_mode = CHANNEL_RENDER_MODE_DIRECT;
  p.channel_id = 0;
  p.component_id = 0;
  p.CTF_id = 0;

  return p;
}

//enum HighlightMode
static constexpr unsigned HIGHLIGHT_MODE_NONE      = 0;
static constexpr unsigned HIGHLIGHT_MODE_PRIMITIVE = 1;
static constexpr unsigned HIGHLIGHT_MODE_GEOMETRY  = 2;
static constexpr unsigned HIGHLIGHT_MODE_INSTANCE  = 3;

struct HighlightPreset
{
  unsigned mode;   //enum HighlightMode
  unsigned prim_id;
  unsigned geom_id;
  unsigned inst_id;
};

static HighlightPreset getEmptyHighlightPreset()
{
  HighlightPreset p;
  p.mode    = HIGHLIGHT_MODE_PRIMITIVE;
  p.prim_id = 0xFFFFFFFFu;
  p.geom_id = 0xFFFFFFFFu;
  p.inst_id = 0xFFFFFFFFu;
  return p;
}

#ifndef KERNEL_SLICER
enum class RenderType
{
  MULTI,
  HYDRA,
  VOLUME,
  VOXEL,
  CSG_DIRECT
};
#endif