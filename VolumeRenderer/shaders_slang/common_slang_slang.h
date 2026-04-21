/////////////////////////////////////////////////////////////////////
/////////////  Required  Shader Features ////////////////////////////
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
/////////////////// include files ///////////////////////////////////
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
/////////////////// declarations in class ///////////////////////////
/////////////////////////////////////////////////////////////////////
static const float FLT_MAX = 1e37f;
static const float FLT_MIN = -1e37f;
static const float FLT_EPSILON = 1e-6f;
static const float EPSILON     = 1e-6f;
static const float DEG_TO_RAD  = 0.017453293f; 

float  SQR(float x)  { return x * x; }
double SQR(double x) { return x * x; }
int    SQR(int x)    { return x * x; }
uint   SQR(uint x)   { return x * x; }

bool  isfinite(float x)            { return !isinf(x); }
float copysign(float mag, float s) { return abs(mag)*sign(s); }

struct complex
{
  float re, im;
};

complex make_complex(float re, float im) { 
  complex res;
  res.re = re;
  res.im = im;
  return res;
}

complex to_complex(float re) { return make_complex(re, 0.0f);}
float real(complex z) { return z.re;}
float imag(complex z) { return z.im; }
float complex_norm(complex z) { return z.re * z.re + z.im * z.im; }
float complex_abs(complex z) { return sqrt(complex_norm(z)); }
complex complex_sqrt(complex z) 
{
  float n = complex_abs(z);
  float t1 = sqrt(0.5f * (n + abs(z.re)));
  float t2 = 0.5f * z.im / t1;
  if (n == 0.0f)
    return to_complex(0.0f);
  if (z.re >= 0.0f)
    return make_complex(t1, t2);
  else
    return make_complex(abs(t2), copysign(t1, z.im));
}

complex operator+(complex a, complex b) { return make_complex(a.re + b.re, a.im + b.im); }
complex operator+(complex a, float b)   { return make_complex(a.re + b, a.im); }
complex operator+(float a, complex b)   { return make_complex(a + b.re, b.im); }

complex operator-(complex a, complex b) { return make_complex(a.re - b.re, a.im - b.im); }
complex operator-(complex a, float b)   { return make_complex(a.re - b, a.im); }
complex operator-(float a, complex b)   { return make_complex(a - b.re, -b.im); }

complex operator*(complex a, complex b) { return make_complex(a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re); }
complex operator*(complex a, float b)   { return make_complex(a.re * b, a.im * b); }
complex operator*(float a, complex b)   { return make_complex(a * b.re, a * b.im); }

complex operator/(complex a, complex b)
{
  float norm = complex_norm(b);
  return make_complex( (a.re * b.re + a.im * b.im) / norm, (a.im * b.re - a.re * b.im) / norm);
}

complex operator/(complex a, float b)
{
  return make_complex(a.re / b, a.im / b);
}

complex operator/(float a, complex b)
{
  float norm = complex_norm(b);
  return make_complex((a * b.re) / norm, (-a * b.im) / norm);
}

#define MAXFLOAT FLT_MAX
static const uint NONE = 0x00000000;
static const uint BUILD_LOW = 0x00000001;
static const uint BUILD_MEDIUM = 0x00000002;
static const uint BUILD_HIGH = 0x00000004;
static const uint BUILD_REFIT = 0x00000008;
static const uint MOTION_BLUR = 0x00000010;
static const uint BUILD_SKIP = 0x00000020;
static const uint BUILD_OPTIONS_MAX_ENUM = 0x7FFFFFFF;
struct CRT_Hit 
{
  float    t;         ///< intersection distance from ray origin to object
  uint primId; 
  uint instId;
  uint geomId;    ///< use 4 most significant bits for geometry type; thay are zero for triangles 
  float    coords[4]; ///< custom intersection data; for triangles coords[0] and coords[1] stores baricentric coords (u,v)
};
static const uint CRT_GEOM_MASK_AABB_BIT = 0x80000000;
static const uint CRT_GEOM_MASK_AABB_BIT_RM = 0x7fffffff;
struct CRT_LeafInfo 
{
  uint aabbId; ///<! id of aabb/box  inside BLAS
  uint primId; ///<! id of primitive inside BLAS
  uint instId; ///<! instance id
  uint geomId; ///<! end-to-end index of custom geometry processerd with AABB
  uint rayxId; ///<! unique ray id (x coord on image)
  uint rayyId; ///<! unique ray id (y coord on image)
};
static const uint TYPE_MESH_TRIANGLE = 0;
static const uint TYPE_SDF_GRID = 1;
static const uint TYPE_SDF_FRAME_OCTREE = 3;
static const uint TYPE_SDF_SVS = 5;
static const uint TYPE_SDF_SBS = 6;
static const uint TYPE_SDF_FRAME_OCTREE_TEX = 8;
static const uint TYPE_SDF_MULTI_OCTREE = 9;
static const uint TYPE_SDF_SBS_TEX = 10;
static const uint TYPE_SDF_SBS_COL = 11;
static const uint TYPE_CSG_TREE = 13;
static const uint TYPE_CSG_ANIM_TREE = 14;
static const uint TYPE_CSG_OP_TREE = 13;
static const uint TYPE_CSG_OP_TREE_ANIM = 14;
static const uint TYPE_GRAPHICS_PRIM = 17;
static const uint TYPE_COCTREE_V3 = 20;
static const uint TYPE_OPENVDB_GRID = 23;
static const uint TYPE_CSGDNF = 24;
static const uint TYPE_SDF_DAG = 25;
static const uint TYPE_SCOM2 = 26;
static const uint TYPE_DENSITY_GRID = 27;
static const uint TYPE_COLOR_DENSITY_GRID = 28;
static const uint TYPE_SPARSE_VOXEL_OCTREE = 29;
static const uint TYPE_UNDEFINED = 255;
static const uint SH_TYPE = 24;
static const uint GEOM_ID_MASK = 0x00FFFFFFu;
static const uint DEVICE_CPU = 0;
static const uint DEVICE_GPU = 1;
static const uint DEVICE_GPU_COMP = 2;
static const uint DEVICE_GPU_RQ = 3;
static const uint AS_TYPE_COMMON = 0;
static const uint SDF_OCTREE_NODE_INTERSECT_ST = 0;
static const uint SDF_OCTREE_NODE_INTERSECT_ANALYTIC = 1;
static const uint SDF_OCTREE_NODE_INTERSECT_NEWTON = 2;
static const uint SDF_OCTREE_NODE_INTERSECT_BBOX = 3;
static const uint SDF_OCTREE_NODE_INTERSECT_IT = 4;
static const uint MULTI_RENDER_MODE_MASK = 0;
static const uint MULTI_RENDER_MODE_LAMBERT_NO_TEX = 1;
static const uint MULTI_RENDER_MODE_DEPTH = 2;
static const uint MULTI_RENDER_MODE_LINEAR_DEPTH = 3;
static const uint MULTI_RENDER_MODE_INVERSE_LINEAR_DEPTH = 4;
static const uint MULTI_RENDER_MODE_PRIMITIVE = 5;
static const uint MULTI_RENDER_MODE_TYPE = 6;
static const uint MULTI_RENDER_MODE_GEOM = 7;
static const uint MULTI_RENDER_MODE_NORMAL = 8;
static const uint MULTI_RENDER_MODE_BARYCENTRIC = 9;
static const uint MULTI_RENDER_MODE_ST_ITERATIONS = 10;
static const uint MULTI_RENDER_MODE_PBR_NO_TEX = 11;
static const uint MULTI_RENDER_MODE_PHONG_NO_TEX = 12;
static const uint MULTI_RENDER_MODE_OUTLINE_PRIM = 13;
static const uint MULTI_RENDER_MODE_OUTLINE_BORDER = 14;
static const uint MULTI_RENDER_MODE_TEX_COORDS = 15;
static const uint MULTI_RENDER_MODE_DIFFUSE = 16;
static const uint MULTI_RENDER_MODE_LAMBERT = 17;
static const uint MULTI_RENDER_MODE_PHONG = 18;
static const uint MULTI_RENDER_MODE_HSV_DEPTH = 19;
static const uint MULTI_RENDER_MODE_LOD = 20;
static const uint MULTI_RENDER_MODE_PBR = 21;
static const uint MULTI_RENDER_MODE_MATERIAL = 22;
static const uint MULTI_RENDER_MODE_CHANNEL = 23;
static const uint MULTI_RENDER_MODE_MO_VOXEL_COUNT = 24;
static const uint MULTI_RENDER_MODE_MO_FLAGS = 25;
static const uint MULTI_RENDER_MODE_INTERSECTION_COUNT = 26;
static const uint MULTI_RENDER_MODE_CHANNEL_LAMBERT = 27;
static const uint NORMAL_MODE_GEOMETRY = 0;
static const uint NORMAL_MODE_VERTEX = 1;
static const uint NORMAL_MODE_SDF_SMOOTHED = 2;
static const uint RAY_GEN_MODE_REGULAR = 0;
static const uint RAY_GEN_MODE_RANDOM = 1;
static const uint RAY_GEN_MODE_HAMMERSLEY = 2;
static const uint INTERPOLATION_MODE_TRILINEAR = 0;
static const uint INTERPOLATION_MODE_TRICUBIC = 1;
static const uint REPRESENTATION_MODE_SURFACE = 0;
static const uint REPRESENTATION_MODE_VOLUME = 1;
static const uint CHANNEL_RENDER_MODE_DIRECT = 0;
static const uint CHANNEL_RENDER_MODE_NORMALIZED = 1;
static const uint CHANNEL_RENDER_MODE_PALETTE = 2;
static const uint CHANNEL_RENDER_MODE_CTF = 3;
static const uint CHANNEL_RENDER_MODE_AS_RGB_COLOR = 4;
static const uint CHANNEL_RENDER_MODE_AS_NORMAL = 5;
struct MultiRenderPreset
{
  uint render_mode;        //enum MultiRenderMode
  uint sdf_node_intersect; //enum SdfNodeIntersect
  uint normal_mode;        //enum NormalMode
  uint ray_gen_mode;       //enum RayGenMode
  uint interpolation_mode; //enum InterpolationMode
  uint representation_mode;//enum RepresentationMode

  uint spp;                //samples per pixel
  uint fixed_lod;          //0 or 1, use fixed level of detail or dynamic (based on distance)
  float level_of_detail;       //level of detail that you get on an object 1 meter away from you. It is inverted (worst LOD is 0!)
  float compact_sdf_eps;       //larger epsilon for intersection with compact SDFs, needed to prevent some visual artifacts
  float LOD_distance;          //distance at which original geometry is switched to LOD

  //used only when render_mode == MULTI_RENDER_MODE_CHANNEL
  uint channel_render_mode;//enum ChannelRenderMode
  uint channel_id;
  uint component_id;
  uint CTF_id;
};
static const uint HIGHLIGHT_MODE_NONE = 0;
static const uint HIGHLIGHT_MODE_PRIMITIVE = 1;
static const uint HIGHLIGHT_MODE_GEOMETRY = 2;
static const uint HIGHLIGHT_MODE_INSTANCE = 3;
struct HighlightPreset
{
  uint mode;   //enum HighlightMode
  uint prim_id;
  uint geom_id;
  uint inst_id;
};
struct Plane
{
  float3 normal;
  uint is_active;
  float3 pos;
  uint _pad;
};
static const uint LIGHT_TYPE_DIRECT = 0;
static const uint LIGHT_TYPE_POINT = 1;
static const uint LIGHT_TYPE_AMBIENT = 2;
static const uint MULTI_RENDER_MATERIAL_TYPE_COLORED = 0;
static const uint MULTI_RENDER_MATERIAL_TYPE_TEXTURED = 1;
static const uint DEFAULT_MATERIAL = 0u;
static const uint DEFAULT_TEXTURE = 0u;
static const uint MULTI_RENDER_MAX_TEXTURES = 256;
static const uint PACK_XY_BLOCK_SIZE = 8;
static const uint HAS_NO_CHANNELS = 0xFFFFFFFF;
struct MultiRendererMaterial
{
  uint type;
  uint texId; // valid if type == MULTI_RENDER_MATERIAL_TYPE_TEXTURED 
  float metallic;
  float roughness;
  float4 base_color; // valid if type == MULTI_RENDER_MATERIAL_TYPE_COLORED
};
struct HydraSceneProperties
{
  uint num_primitives;
};
static const uint CHANNEL_DATA_TYPE_INT = 0;
static const uint CHANNEL_DATA_TYPE_FLOAT = 1;
static const uint CHANNEL_DATA_TYPE_FP8 = 2;
static const uint CHANNEL_TYPE_VERTEX = 0;
static const uint CHANNEL_TYPE_FACE = 1;
struct ChannelRenderInfo
{
  uint components;// number of components in the channel
  uint offset;    // offset in vector where the actual data for channel is stored
  uint type;      // enum ChannelType
  uint data_type; // enum ChannelDataType
  float min_val;
  float max_val;
};
struct ColorTranferFunctionInfo
{
  uint sample_points_offset;
  uint sample_points_count;
  uint sample_steps;
};
struct Light
{
  float3 space; // position or direction
  uint type;// enum LightType
  float3 color; // intensity included
  uint _pad;
};
static const uint INVALID_IDX = 1u<<31u;
struct OTStackElement
{
  uint nodeId;
  uint info;
  uint2 p_size;
};
struct ExtStackElement
{
  uint linksOff;
  uint info;
  uint transform;
  uint2 p_size;
};
static const uint SCOM2_CHILD_EMPTY = 0;
static const uint SCOM2_CHILD_LEAF_VOLUME = 1;
static const uint SCOM2_CHILD_LEAF_SURFACE = 3;
static const uint SCOM2_CHILD_NODE = 2;
static const uint SCOM2_CHILD_TYPE_BITS = 2;
static const uint SCOM2_CHILD_TYPE_MASK = (1 << SCOM2_CHILD_TYPE_BITS) - 1;
static const uint SCOM2_MAGIC_NUMBER = 0xffffdefa;
static const uint SCOM2_VERSION = 4;
struct SCom2Header
{
  uint brick_size;      // values brick size    (default = 1, i.e. 2x2x2 values)
  uint v_size;          // brick_size + 2*brick_pad + 1
  uint bits_per_value;
  uint values_per_uint;
  uint value_mask;
  uint bitmask_len;     // bitmask length in uints
  uint dimension;

  uint child_rot_shift;
  uint child_rot_mask;
  uint child_add_shift;
  uint child_add_mask;
  uint child_offset_mask;
  uint child_offset_off;
  uint node_offset_mask;
  uint uints_per_link;
  uint unique_brick_prefix;
  uint unique_brick_offset_mask;

  uint children_types_shift;
  uint children_types_mask;
  uint base_reference_shift;
  uint children_active_bits_shift;
  uint children_active_bits_mask;
  uint references_offset;
  uint reference_bits;
  uint reference_mask;
  uint references_per_uint;
  uint links_offset;
  uint max_surface_count;
  uint max_surface_count_per_leaf;

  uint bricks_step;
  uint bricks_arr_offset;
  uint nodes_arr_offset;
  uint root_node_off;

  uint has_channels;
  uint has_surfaces;
  uint has_multi_nodes;

  int tex_id_off;
  int mat_id_off;
  int all_float_tex_id_off;
  int all_int_mat_id_off;

  float max_val;
  uint max_depth;

  float user_params[7];

  //to allow further extensions without breaking binary compatibility
  uint _pad0;
  uint _pad1;
  uint _pad2;
  uint _pad3;
  uint _pad4;
};
struct SdfDAGDataEdge
{
  uint data_offset; // offset in distances vector
  uint rotation_id;
  uint type_id;     // SdfMultiOctreeFlags
  float    add;
};
struct SdfDAGChildEdge
{
  uint child_offset; // offset in nodes vector
  uint rotation_id;
};
struct SdfDAGNode
{
  uint children_edges_offset; // offset in vector of children edges, 0 offset means it's a leaf
  uint data_edges_offset;     // offset in vector of data edges, 0 offset means it has no surface
  SdfDAGChildEdge channels_edge;  // offset+rotation for channels (offset in num_components*component_size steps).
  uint voxel_count_flags;     // low 16 bits are number of voxels in this node, 0 mean it does not contain surface, 
                                  // high 16 bits are flags (SdfMultiOctreeFlags)
  uint _pad;
};
struct SdfDAGHeader
{
  uint brick_size;      // values brick size    (default = 1, i.e. 2x2x2 values)
  uint brick_pad;       // values brick padding (default = 0)
  uint node_grid_size;  // node grid size       (default = 2, i.e. 2x2x2 children, octree)
  uint dim;             // dimentionality of the DAG, 2 for quadtree, 3 for octree (default), 4 for ???-tree

  uint transform_subgroup; // enum class TransformSubgroup
  float user_params[7];        // some place to store user-defined params for specific types of DAGs, e.g. scene_extent for Radiance Fields

  int tex_id_off;
  int mat_id_off;
  int all_float_tex_id_off;
  int all_int_mat_id_off;
};
struct NodeHeadUnpacked
{
  uint base_type;
  uint children_types;
  uint base_links_end;
  uint children_active;
};
static const uint LEAF_NORMAL = 0xFFFFFFFF;
static const uint LEAF_EMPTY = 0xFFFFFFFD;
static const uint ESCAPE_ROOT = 0xFFFFFFFE;
struct BVHNode 
  {
    float3 boxMin;
    uint   leftOffset; //!< please note that when LEAF_BIT (0x80000000) is set in leftOffset, this node is a leaf
    float3 boxMax;
    uint   escapeIndex;
  };
struct BVHNodePair
  { 
    BVHNode left;
    BVHNode right;
  };
struct BoxHit
{
  uint id;
  float tHit;
};
static const int LBVH_MAXHITS = 32;
static const int STACK_SIZE = 80;
static const uint START_MASK = 0x0FFFFFFF;
static const uint END_MASK = 0xF0000000;
static const uint SIZE_MASK = 0x70000000;
static const uint LEAF_BIT = 0x80000000;
static const uint EMPTY_NODE = 0x7FFFFFFF;
struct GeomData
{
  float4 boxMin;
  float4 boxMax;

  uint bvhOffset;
  uint type; // enum GeomType
  uint2 offset;

  uint hasChannels;
  uint _pad[3];
};
struct InstanceData
{
  float4 boxMin;
  float4 boxMax;
  uint geomId;
  uint _pad[7];
  float4x4 transform;
  float4x4 transformInv;
  float4x4 transformInvTransposed; //for normals
};
static const uint VOLUME_TRAVERSAL_MODE_BRESENHAM = 0;
static const uint VOLUME_TRAVERSAL_MODE_DDA = 1;
static const uint VOLUME_TRAVERSAL_MODE_DDA_BRANCHLESS = 2;
static const uint VOLUME_TRAVERSAL_MODE_VRT_SS = 3;
static const uint VOLUME_TYPE_LINEAR_DENSITY = 0;
static const uint VOLUME_TYPE_SVRASTER_RF = 1;
static const uint SCENE_EXTENT_USER_PARAM_ID = 0;
static const uint SCENE_CENTER_X_USER_PARAM_ID = 1;
static const uint SCENE_CENTER_Y_USER_PARAM_ID = 2;
static const uint SCENE_CENTER_Z_USER_PARAM_ID = 3;
struct VolumeRenderPreset
{
  uint traversal_mode; // VolumeTraversalMode
  uint volume_type; // VolumeType
  uint spp;
  uint voxel_traversal_steps;
  uint ignore_timestamps;
  uint use_lighting;
  float alpha_thr;
  float animation_speed;
};
static const uint ROT_COUNT = 48;
static const uint SH_COMPONENT_COUNT = (3+1)*(3+1);
static const float GEPSILON = 2e-5f;
static const float DEPSILON = 1e-20f;
struct OTDeepStackElement
{
  uint nodeId;
  uint info;
  uint i;
  uint j;
  uint k;
  uint size;
};
struct OTStackElement4D
{
  uint4 p;
  uint nodeId;
  uint info;
  uint size;
  uint transform;
};
struct ExtStackElement4D
{
  uint4 p;
  uint size;
  uint linksOff;
  uint currNode;
  uint transform;

  uint chActive;
  uint chTypes;
  uint _pad0;
  uint _pad1;
};

#define M_PI          3.14159265358979323846f
#define M_TWOPI       6.28318530717958647692f
#define INV_PI        0.31830988618379067154f
#define INV_TWOPI     0.15915494309189533577f

struct VolumeRenderer_slang_UBO_Data
{
  float4x4 m_projInv; 
  float4x4 m_worldViewInv; 
  Plane m_cuttingPlane; 
  float4 m_backgroundColor; 
  float4 m_baseColor; 
  SCom2Header m_header; 
  VolumeRenderPreset m_preset; 
  uint frame0; 
  uint frame1; 
  uint m_AAFrameNum; 
  uint m_SdfDAGBrickTranspositionOffset; 
  uint m_SdfDAGTransformCount; 
  float m_dframe; 
  uint m_grid_type; 
  uint m_height; 
  uint m_packedXY_height; 
  uint m_packedXY_width; 
  uint m_seed; 
  uint m_sz; 
  uint m_vox_cnt; 
  uint m_width; 
  uint m_DAGInverseTransforms_capacity; 
  uint m_DAGInverseTransforms_size; 
  uint m_RotAddTable_capacity; 
  uint m_RotAddTable_size; 
  uint m_SComNodes_capacity; 
  uint m_SComNodes_size; 
  uint m_SComValues_capacity; 
  uint m_SComValues_size; 
  uint m_SdfCompactOctreeRotModifiers_capacity; 
  uint m_SdfCompactOctreeRotModifiers_size; 
  uint m_SdfDAGChildEdges_capacity; 
  uint m_SdfDAGChildEdges_size; 
  uint m_SdfDAGDataEdges_capacity; 
  uint m_SdfDAGDataEdges_size; 
  uint m_SdfDAGDistances_capacity; 
  uint m_SdfDAGDistances_size; 
  uint m_SdfDAGHeaders_capacity; 
  uint m_SdfDAGHeaders_size; 
  uint m_SdfDAGNodes_capacity; 
  uint m_SdfDAGNodes_size; 
  uint m_SdfDAGTranspositions_capacity; 
  uint m_SdfDAGTranspositions_size; 
  uint m_SphericalHarmonics_capacity; 
  uint m_SphericalHarmonics_size; 
  uint m_colorBuffer_capacity; 
  uint m_colorBuffer_size; 
  uint m_colored_grid_capacity; 
  uint m_colored_grid_size; 
  uint m_grid_capacity; 
  uint m_grid_size; 
  uint m_packedXY_capacity; 
  uint m_packedXY_size; 
  uint m_timestamps_capacity; 
  uint m_timestamps_size; 
  uint dummy_last;
};



/////////////////////////////////////////////////////////////////////
/////////////////// local functions /////////////////////////////////
/////////////////////////////////////////////////////////////////////

float4x4 make_float4x4_from_cols(float4 a, float4 b, float4 c, float4 d) { return transpose(float4x4(a, b, c, d)); }
float4x4 make_float4x4_from_rows(float4 a, float4 b, float4 c, float4 d) { return float4x4(a, b, c, d); }

float4x4 translate4x4(float3 delta)
{
  return float4x4(float4(1.0f, 0.0f, 0.0f, delta.x),
                  float4(0.0f, 1.0f, 0.0f, delta.y),
                  float4(0.0f, 0.0f, 1.0f, delta.z),
                  float4(0.0f, 0.0f, 0.0f, 1.0f));
}

float4x4 rotate4x4X(float phi)
{
  return float4x4(float4(1.0f, 0.0f,       0.0f,      0.0f),
                  float4(0.0f, +cos(phi), -sin(phi), 0.0f),
                  float4(0.0f, +sin(phi), +cos(phi), 0.0f),
                  float4(0.0f, 0.0f,       0.0f,      1.0f));
}

float4x4 rotate4x4Y(float phi)
{
  return float4x4(float4(+cos(phi), 0.0f, +sin(phi), 0.0f),
                  float4(0.0f,      1.0f, 0.0f,      0.0f),
                  float4(-sin(phi), 0.0f, +cos(phi), 0.0f),
                  float4(0.0f,      0.0f, 0.0f,      1.0f));
}

float4x4 rotate4x4Z(float phi)
{
  return float4x4(float4(+cos(phi), -sin(phi), 0.0f, 0.0f),
                  float4(+sin(phi), +cos(phi), 0.0f, 0.0f),
                  float4(0.0f,      0.0f,      1.0f, 0.0f),
                  float4(0.0f,      0.0f,      0.0f, 1.0f));
}

float3x3 rotate3x3X(float phi)
{
  return float3x3(float3(1.0f, 0.0f,       0.0f),
                  float3(0.0f, +cos(phi), -sin(phi)),
                  float3(0.0f, +sin(phi), +cos(phi)));
}

float3x3 rotate3x3Y(float phi)
{
  return float3x3(float3(+cos(phi), 0.0f, +sin(phi)),
                  float3(0.0f,      1.0f, 0.0f),
                  float3(-sin(phi), 0.0f, +cos(phi)));
}

float3x3 rotate3x3Z(float phi)
{
  return float3x3(float3(+cos(phi), -sin(phi), 0.0f),
                  float3(+sin(phi), +cos(phi), 0.0f),
                  float3(0.0f,      0.0f,      1.0f));
}

float3x3 inverse3x3(float3x3 m)
{
  float det = determinant(m);
  if (abs(det) < 1e-8)
    return float3x3(0, 0, 0, 0, 0, 0, 0, 0, 0);
    
  float invDet = 1.0 / det;
  
  float3x3 adj;
  adj._11 = +(m._22 * m._33 - m._23 * m._32) * invDet;
  adj._12 = -(m._12 * m._33 - m._13 * m._32) * invDet;
  adj._13 = +(m._12 * m._23 - m._13 * m._22) * invDet;
  
  adj._21 = -(m._21 * m._33 - m._23 * m._31) * invDet;
  adj._22 = +(m._11 * m._33 - m._13 * m._31) * invDet;
  adj._23 = -(m._11 * m._23 - m._13 * m._21) * invDet;
  
  adj._31 = +(m._21 * m._32 - m._22 * m._31) * invDet;
  adj._32 = -(m._11 * m._32 - m._12 * m._31) * invDet;
  adj._33 = +(m._11 * m._22 - m._12 * m._21) * invDet;
  
  return adj;
}

//float4x4 inverse4x4(float4x4 m) { return inverse(m); }
//float3 mul4x3(float4x4 m, float3 v) { return (m*float4(v, 1.0f)).xyz; }
//float3 mul3x3(float4x4 m, float3 v) { return (m*float4(v, 0.0f)).xyz; }

float3x3 make_float3x3(float3 a, float3 b, float3 c) { // different way than mat3(a,b,c)
  return float3x3(a.x, a.y, a.z,
                  b.x, b.y, b.z,
                  c.x, c.y, c.z);
}

void set_col2f(inout float2x2 m, int col, float2 value) { m[0][col] = value.x; m[1][col] = value.y; }
void set_col3f(inout float3x3 m, int col, float3 value) { m[0][col] = value.x; m[1][col] = value.y; m[2][col] = value.z; }
void set_col4f(inout float4x4 m, int col, float4 value) { m[0][col] = value.x; m[1][col] = value.y; m[2][col] = value.z; m[3][col] = value.w; }

float4 cross3(float4 a, float4 b) { return float4(cross(a.xyz, b.xyz), 1.0f); }

struct Box4f 
{ 
  float4 boxMin; 
  float4 boxMax;
};  

// Have them in lang to do less rewriting, don't have them in GLSL backend
//
uint4  make_uint4(uint x, uint y, uint z, uint w) { return uint4(x, y, z, w); }
int4   make_int4(int x, int y, int z, int w) { return int4(x, y, z, w); }
float4 make_float4(float x, float y, float z, float w) { return float4(x, y, z, w); }
uint3  make_uint3(uint x, uint y, uint z) { return uint3(x, y, z); }
int3   make_int3(int x, int y, int z) { return int3(x, y, z); }
float3 make_float3(float x, float y, float z) { return float3(x, y, z); }
uint2  make_uint2(uint x, uint y) { return uint2(x, y); }
int2   make_int2(int x, int y) { return int2(x, y); }
float2 make_float2(float x, float y) { return float2(x, y); }

float3 to_float3(float4 f4)         { return f4.xyz; }
float4 to_float4(float3 v, float w) { return float4(v.x, v.y, v.z, w); }
uint3  to_uint3 (uint4 f4)          { return f4.xyz;  }
uint4  to_uint4 (uint3 v, uint w)   { return uint4(v.x, v.y, v.z, w);  }
int3   to_int3  (int4 f4)           { return f4.xyz;   }
int4   to_int4  (int3 v, int w)     { return int4(v.x, v.y, v.z, w);   }

float4 mul4x4x4(float4x4 m, float4 v) { return mul(m,v); }
float3 mul3x3  (float4x4 m, float3 v) { return to_float3(mul(m, to_float4(v, 0.0f))); }
float3 mul4x3  (float4x4 m, float3 v) { return to_float3(mul(m, to_float4(v, 1.0f))); }

float4   operator*(float4x4 m, float4 v) { return mul(m,v); }
float3   operator*(float4x4 m, float3 v) { return mul(m, float4(v,1.0f)).xyz; }
float3   operator*(float3x3 m, float3 v) { return mul(m,v); }

double4   operator*(double4x4 m,  double4 v) { return mul(m,v); }
double3   operator*(double4x4 m,  double3 v) { return mul(m, double4(v,1.0f)).xyz; }
double3   operator*(double3x3 m,  double3 v) { return mul(m,v); }

static inline uint bitCount(uint x) { return countbits(x); }

float eval_dist_trilinear(const float values[8], float3 dp) ;
float3 mymul4x3(float4x4 m, float3 v) ;
float3 rand3(uint seed, uint x, uint y, uint iter) ;
SdfDAGChildEdge unpack_child_edge(in SCom2Header header, uint edge0, uint edge1) ;
int new_node(uint i, float txm, float tym, float tzm) ;
int first_node(float3 t0, float3 tm) ;
NodeHeadUnpacked unpack_node_head(in SCom2Header header, uint node0, uint node1) ;
SdfDAGDataEdge unpack_data_edge(in SCom2Header header, float max_val, uint edge0, uint edge1) ;
float exp_linear_11(float x) ;
uint DAG_extract_count(uint voxel_count_flags) ;
void transform_ray3f(float4x4 a_mWorldViewInv, inout float3 ray_pos, inout float3 ray_dir) ;
bool DAG_node_is_full(uint voxel_count_flags) ;
float2 RayBoxIntersection2(float3 rayOrigin, float3 rayDirInv, float3 boxMin, float3 boxMax) ;
float3 SafeInverse(float3 d) ;
float3 EyeRayDirNormalized(float x, float y, float4x4 a_mViewProjInv) ;
float2 rand2(uint seed, uint x, uint y, uint iter) ;
bool DAG_extract_is_surface(uint voxel_count_flags) ;
uint SuperBlockIndex2DOpt(uint tidX, uint tidY, uint a_width) ;

float eval_dist_trilinear(const float values[8], float3 dp) {
  return (1-dp.x)*(1-dp.y)*(1-dp.z)*values[0] + 
         (1-dp.x)*(1-dp.y)*(  dp.z)*values[1] + 
         (1-dp.x)*(  dp.y)*(1-dp.z)*values[2] + 
         (1-dp.x)*(  dp.y)*(  dp.z)*values[3] + 
         (  dp.x)*(1-dp.y)*(1-dp.z)*values[4] + 
         (  dp.x)*(1-dp.y)*(  dp.z)*values[5] + 
         (  dp.x)*(  dp.y)*(1-dp.z)*values[6] + 
         (  dp.x)*(  dp.y)*(  dp.z)*values[7];
}

float3 mymul4x3(float4x4 m, float3 v) {
  return to_float3(m*to_float4(v, 1.0f));
}

float3 rand3(uint seed, uint x, uint y, uint iter) {
  x = x + 1233u*(iter+seed) % 171u;
  y = y + 453u*(iter+seed) % 765u;
  uint3 v = uint3(x,y,x ^ y);

  v = v * 1664525u + 1013904223u;

  v.x += v.y * v.z;
  v.y += v.z * v.x;
  v.z += v.x * v.y;

  v.x ^= v.x >> 16u;
  v.y ^= v.y >> 16u;
  v.z ^= v.z >> 16u;

  v.x += v.y * v.z;
  v.y += v.z * v.x;
  v.z += v.x * v.y;

  return float3(v) * (1.0f/float(0xffffffffu));
}

SdfDAGChildEdge unpack_child_edge(in SCom2Header header, uint edge0, uint edge1) {
  SdfDAGChildEdge edge;
  edge.rotation_id = (edge0 >> header.child_rot_shift) & header.child_rot_mask;
  edge.child_offset = edge1 & header.node_offset_mask;
  return edge;
}

int new_node(uint i, float txm, float tym, float tzm) {
  uint x = 4 + i;
  uint y = 2 + (i&0x5) + ((i&0x2) << 3);
  uint z = 1 + (i&0x6) + ((i&0x1) << 3);
  return (txm < tym) ? (txm < tzm ? x : z) : (tym < tzm ? y : z);
}

int first_node(float3 t0, float3 tm) {
uint answer = 0;   // initialize to 00000000
// select the entry plane and set bits
if(t0.x > t0.y){
    if(t0.x > t0.z){ // PLANE YZ
        if(t0.x > tm.y) answer|=2;    // set bit at position 1
        if(t0.x > tm.z) answer|=1;    // set bit at position 0
        return (int) answer;
    }
}
else {
    if(t0.y > t0.z){ // PLANE XZ
        if(tm.x < t0.y) answer|=4;    // set bit at position 2
        if(t0.y > tm.z) answer|=1;    // set bit at position 0
        return (int) answer;
    }
}
// PLANE XY
if(tm.x < t0.z) answer|=4;    // set bit at position 2
if(tm.y < t0.z) answer|=2;    // set bit at position 1
return (int) answer;
}

NodeHeadUnpacked unpack_node_head(in SCom2Header header, uint node0, uint node1) {
  NodeHeadUnpacked head_unpacked;
  head_unpacked.base_type = node0 & 0xFFu;

  if (header.has_multi_nodes != 0)
    head_unpacked.children_types = (node0 >> header.children_types_shift) & header.children_types_mask;
  else
    head_unpacked.children_types = (node1 >> header.children_types_shift) & header.children_types_mask;

  if (header.has_multi_nodes != 0)
    head_unpacked.base_links_end = (node0 >> header.base_reference_shift) & header.reference_mask;
  else
    head_unpacked.base_links_end = (head_unpacked.base_type == SCOM2_CHILD_EMPTY ? 0 : (header.has_surfaces == 0 ? header.has_channels : 1+header.has_channels));
  
  head_unpacked.children_active = (node0 >> header.children_active_bits_shift) & header.children_active_bits_mask;
  return head_unpacked;
}

SdfDAGDataEdge unpack_data_edge(in SCom2Header header, float max_val, uint edge0, uint edge1) {
  SdfDAGDataEdge edge;
  bool is_unique = (edge0 & header.unique_brick_prefix) == header.unique_brick_prefix;
  bool no_add = is_unique || (header.child_add_mask == 0);
  if (header.max_val > 0)
    edge.add = no_add ? 0.0f : header.max_val * (2 * ((edge0 >> header.child_add_shift) & header.child_add_mask) / float(header.child_add_mask) - 1);
  else
    edge.add = no_add ? 0.0f : max_val * (2 * ((edge0 >> header.child_add_shift) & header.child_add_mask) / float(header.child_add_mask) - 1);

  edge.rotation_id = is_unique ? 0 : (edge0 >> header.child_rot_shift) & header.child_rot_mask;
  edge.data_offset = is_unique ? edge1 & header.unique_brick_offset_mask : edge1 & header.child_offset_mask;
  edge.type_id = 0;
  return edge;
}

float exp_linear_11(float x) {
  return (x > 1.1f) ? x : exp(0.909090909091f * x - 0.904689820196f);
}

uint DAG_extract_count(uint voxel_count_flags) {
  return voxel_count_flags & 0xFFFFu;
}

void transform_ray3f(float4x4 a_mWorldViewInv, inout float3 ray_pos, inout float3 ray_dir) {
  float3 pos  = mymul4x3(a_mWorldViewInv, (ray_pos));
  float3 pos2 = mymul4x3(a_mWorldViewInv, ((ray_pos) + 100.0f*(ray_dir)));

  float3 diff = pos2 - pos;

  (ray_pos)  = pos;
  (ray_dir)  = normalize(diff);
}

bool DAG_node_is_full(uint voxel_count_flags) {
  return (voxel_count_flags & 0xFFFFu) != 0;
}

float2 RayBoxIntersection2(float3 rayOrigin, float3 rayDirInv, float3 boxMin, float3 boxMax) {
  const float lo  = rayDirInv.x * (boxMin.x - rayOrigin.x);
  const float hi  = rayDirInv.x * (boxMax.x - rayOrigin.x);
  const float lo1 = rayDirInv.y * (boxMin.y - rayOrigin.y);
  const float hi1 = rayDirInv.y * (boxMax.y - rayOrigin.y);
  const float lo2 = rayDirInv.z * (boxMin.z - rayOrigin.z);
  const float hi2 = rayDirInv.z * (boxMax.z - rayOrigin.z);

  const float tmin = max(min(lo, hi), min(lo1, hi1));
  const float tmax = min(max(lo, hi), max(lo1, hi1));

  return float2(max(tmin, min(lo2, hi2)),min(tmax, max(lo2, hi2)));
}

float3 SafeInverse(float3 d) {
  const float ooeps = 1.0e-36f; // Avoid div by zero.
  float3 res;
  res.x = 1.0f / (abs(d.x) > ooeps ? d.x : abs(ooeps)*sign(d.x));
  res.y = 1.0f / (abs(d.y) > ooeps ? d.y : abs(ooeps)*sign(d.y));
  res.z = 1.0f / (abs(d.z) > ooeps ? d.z : abs(ooeps)*sign(d.z));
  return res;
}

float3 EyeRayDirNormalized(float x, float y, float4x4 a_mViewProjInv) {
  float4 pos = float4(2.0f*x - 1.0f,-2.0f*y + 1.0f,1.0f,1.0f);
  pos = a_mViewProjInv * pos;
  pos /= pos.w;
  return normalize(to_float3(pos));
}

float2 rand2(uint seed, uint x, uint y, uint iter) {
  float3 r3 = rand3(seed, x, y, iter);
  return float2(r3.x,r3.y);
}

bool DAG_extract_is_surface(uint voxel_count_flags) {
  return ((voxel_count_flags >> 16) & 0x1) > 0;
}

uint SuperBlockIndex2DOpt(uint tidX, uint tidY, uint a_width) {
  const uint inBlockIdX = tidX & 0x00000003; // 4x4 blocks
  const uint inBlockIdY = tidY & 0x00000003; // 4x4 blocks
  const uint localIndex = inBlockIdY*4 + inBlockIdX;
  const uint wBlocks = a_width >> 2;
  const uint blockX = tidX    >> 2;
  const uint blockY = tidY    >> 2;
  
  const uint inHBlockIdX = blockX & 0x00000001; // 2x2 SuperBlocks
  const uint inHBlockIdY = blockY & 0x00000001; // 2x2 SuperBlocks
  const uint localIndexH = inHBlockIdY*2 + inHBlockIdX;
  const uint wBlocksH = wBlocks >> 1;
  const uint blockHX = blockX  >> 1;
  const uint blockHY = blockY  >> 1;

  return (blockHX + blockHY*wBlocksH)*64 + localIndexH*16 + localIndex;
}


