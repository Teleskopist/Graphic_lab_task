#pragma once

#define VK_NO_PROTOTYPES

#include "command_buffer.h"
#include "utils/misc/camera.h"
#include "PostEffects/antialiasing/antialiaser.h"
#include "PostEffects/denoising/denoiser.h"
#include "PostEffects/image_metrics/image_metrics.h"

#include "LiteScene/scene_mgr.h"
#include "LiteScene/scene.h"
#include "sdf/simple/sdf_converter.h"
#include "utils/vtk/vtk_structures.h"
#include "vk_utils.h"
#include "core/IRenderer.h"
#include "LiteApp/lite_app.h"
#include "LiteApp/lite_app_imgui.h"
#include "LiteApp/rasterization/SimpleRasterizer.h"
#include "LiteApp/rasterization/context.h"

using LiteApp::RenderingContext;
using LiteApp::IRasterizer;
using LiteApp::SimpleRasterizer;
using LiteApp::RasterizationContext;
using LiteApp::Mouse;

#include <chrono>

namespace demo_app
{
  class IRenderPlugin;

  static constexpr int MAX_RENDERERS = 3;
  static constexpr int MAX_PATH_LENGTH = 1024;

  enum class RenderMode
  {
    RASTERIZATION,
    RAYTRACING,
  };

  enum class RasterizerType
  {
    SIMPLE,
    TERRAIN
  };
  
  enum class VisibilityChangeType
  {
    HIDE_CURRENT,
    HIDE_OTHERS,
    SHOW_ALL
  };

  //all settings that user can see and change directly, either via GUI 
  //or via config file at the start
  struct Settings
  {
    //volatile settings - can be changed when app is working
    RenderMode render_mode = RenderMode::RAYTRACING;
    int  raytracerId = 0; //which of the raytraces is active, used when rendering with reference
    float camMoveSpeed     = 1.024f;
    bool cameraMoving      = false;
    bool rasterizerNormals = false;
    bool wireframeMode     = false;
    float mouseSensitivity = 0.1f;
    // Light position in rasterizer, light direction in raytracer
    float3 lightPos = { 3, 2, 1 };
    float3 lightColor = { 1, 1, 1 };
    float lightIntensity = 0.5617f;
    Camera camera;
    float3 backgroundColor = float3(0, 0, 0);

    HighlightPreset highlight_preset;
    Plane cutting_plane = create_disabled_plane();

    //permanent settings - cannot be changed when app is working, only on init
    int desired_window_width  = 1920; //desired size, actual size may be different
    int desired_window_height = 1080; //desired size, actual size may be different
    bool fullscreen = false; //if set to true, desired size will be ignored and window will be fullscreen

    //quad shaders are needed to show image created with raytracing
    //these shaders should not change and thus are used directly as .spv
    std::string quadVertexShaderPath   = "./shaders/quad3_vert.vert.spv";
    std::string quadFragmentShaderPath = "./shaders/my_quad.frag.spv";

    int deviceID = static_cast<int>(vk_utils::CHOOSE_DEVICE_BY_NAME);

    bool validation = false;
    float AAFactor = 0.1f; //how much each new frame contributes to the final image in TAA
  };
  struct LogEntry
  {
    enum class Type
    {
      INFO,
      WARNING,
      ERROR
    };

    static constexpr ImVec4 typeColors[3] = {
      ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
      ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
      ImVec4(1.0f, 0.0f, 0.0f, 1.0f)
    };

    std::string message;
    float time = 0.0f;
    Type type;
  };

  //state of the current scene manipulation
  struct SceneUIContext
  {
    struct Instance
    {
      bool visible = true;
      uint32_t geom_id = 0; //geom id in the geometries vector
      uint32_t h_inst_id = 0; //instance id in the HydraScene
      uint32_t h_geom_id = 0; //geom id in the HydraScene
    };
    struct Geometry
    {
      uint32_t h_geom_id = 0; //geom id in the HydraScene
      std::string name = "unnamed";
    };

    struct CAEInfo
    {
      std::vector<std::string> names;
      std::vector<const char *> c_names;
      std::vector<int> num_components;
    };

    std::vector<Instance> instances;
    std::vector<Geometry> geometries;

    bool is_CAE_dataset = false;
    CAEInfo cae_info; //empty if is_CAE_dataset is false

    bool has_SDF_build_debug_info = false;
    sdf_converter::GlobalOctreeDebugInfo sdf_build_debug_info; //empty if has_SDF_build_debug_info is false

    vtk::Dataset vtkDataset;

    uint64_t memoryInRenderer = 0; //how many bytes in renderer this scene takes
  };

  //conversion types and their names (must be 1 to 1 match)
  static std::vector<const char *> conversion_types = {
      "Mesh -> SDF", "CAD assembly -> SDF", "VTK dataset -> Octree", "Volume -> Octree", 
      "[Deprecated] CAD folder -> MultiOctree"};
  enum class ConversionType
  {
    MESH_TO_SDF,
    CAD_PARTS_TO_MULTI_OCTREE,
    VTK_DATASET_TO_OCTREE,
    VOLUME_TO_OCTREE,
    CAD_FOLDER_TO_MULTI_OCTREE,
  };

  //SDF types and their names (must be 1 to 1 match)
  static std::vector<const char *> conversion_sdf_types = {
      "Regular grid", "Framed octree", "SCom tree", "Multi-octree", "SCom tree 2",
      "[DEBUG] SDF DAG", "[DEBUG] Flood octree (node types)", "[DEBUG] Flood octree (node codes)"};
  enum class SDFType
  {
    REGULAR_GRID,
    FRAME_OCTREE,
    SCOM_TREE,
    MULTI_OCTREE,
    SCOM_TREE_2,
    SDF_DAG,
    FLOOD_OCTREE_TYPES,
    FLOOD_OCTREE_CODES
  };

  //CAD to SDF types and their names (must be 1 to 1 match)
  static std::vector<const char *> conversion_cad_to_sdf_types = {"Multi-octree", "SCom tree 2"};
  enum class CADSDFType
  {
    MULTI_OCTREE,
    SCOM_TREE_2
  };

  //Augmented SDF types and their names (must be 1 to 1 match)
  static std::vector<const char *> conversion_asdf_types = {"Multi-octree", "SCom tree 2", "[DEBUG] SDF DAG"};
  enum class ASDFType
  {
    MULTI_OCTREE,
    SCOM_TREE_2,
    SDF_DAG
  };

  //Volume data type
  static std::vector<const char *> vodume_grid_types = {"3D density grid", "4D density grid", "[TODO] 3D RGB+density grid", "[TODO] 4D RGB+density grid"};
  enum class VolumeGridType
  {
    VOLUME_GRID_3D_DENSITY,
    VOLUME_GRID_4D_DENSITY,
    VOLUME_GRID_3D_RGB_DENSITY,
    VOLUME_GRID_4D_RGB_DENSITY
  };

  static std::vector<const char *> CTF_names = {"Shades of Gray", "Cool to Warm"};
  enum class CTFPreset
  {
    SHADES_OF_GRAY,
    COOL_TO_WARM
  };

  struct CAEVisualizationSettings
  {
    bool lambert_shading = false;
    int channel = 0;
    int component = 0;
    int vis_mode = CHANNEL_RENDER_MODE_CTF; //enum ChannelRenderMode
    CTFPreset ctf_preset = CTFPreset::COOL_TO_WARM;
  };

  class IActiveCmdCtx
  {
  public:
    virtual ~IActiveCmdCtx() {}
    Cmd cmd = Cmd::NO_OP;
  };

  // some metadata of CAE dataset that can be displayed to user
  // for him to choose what to load from the dataset
  struct PreloadedCAEInfo
  {
    struct Channel
    {
      vtk::DataArray::Type type = vtk::DataArray::Type::Float32;
      int num_components = 0;
      std::string name = "unnamed";
      bool is_chosen = true;
    };
    std::vector<Channel> point_channels;
    std::vector<Channel> cell_channels;
  };

  struct DebugTimeData
  {
    float average_frame;
    float one_frame;
    float rt_average;
    float copy_data2GPU;
    float find_blocks;
  };

  static constexpr uint32_t METRICS_BINS_PER_UNIT = 10;
  struct QualityMetricsStat
  {
    uint32_t total_samples = 0;
    std::vector<uint32_t> sample_bins;
  };

  //all the data not specific to rendering, that is representing the state of the app
  //and should be be changed by user directly
  struct UIContext
  {
    std::shared_ptr<IRenderPlugin> activePlugin = nullptr;
    
    Mouse mouse;

    int raytracersCount = 0;
    std::string activeScenePaths[MAX_RENDERERS];

    char userScenePathTextBoxes[MAX_RENDERERS][1024] = {"./scenes/01_simple_scenes/instanced_objects.xml", "", ""};
    int scenePathTextBoxesCount = 1;
    bool showReferenceSceneTextBoxes = false;
    bool calculateLiveMetrics = false;
    bool saveScreenshotOnComparison = true;
    bool minimalUI = false;
    int baseRaytracerId = 0;
    int referenceRaytracerId = 1;
    uint32_t chosenRenderDevice = 1; //NOT the enum RenderDevice
    uint32_t chosenRenderer = 0;
    uint32_t chosenRasterizer = 0;
    bool highQualityScreenshot = true;
    bool toOpenPopUp = false;

    static constexpr float hotkeysBlockDuration = 0.5;
    float  hotkeysBlockCooldown = 0; //in seconds

    static constexpr float averagingFactor = 0.95f;
    static constexpr int sliding_window_size = 100u;

    static constexpr float logShowTime = 10.0f; //in seconds
    std::vector<LogEntry> logToShow;

    //all time in milliseconds
    float averageRTTransferTime = 0.0f;
    float averageRTPrepareTime  = 0.0f;
    float averageRTExecuteTime  = 0.0f;

    float timeSlidingWindow[sliding_window_size] = {};
    int currentTimeIndex = 0;
    int frameCounter = 0;

    bool renderSetUp = false;
    bool sceneProvided = false;
    bool rasterizationSceneSetUp = false;
    bool raytracingSceneSetUp = false;
    bool sceneSupportsRasterization = false;
    bool cuttingPlaneActive = false;
    bool datastVisualizationActive = true;
    bool CAEShowSurface = false;

    int window_width;  //actual size of the window, should be equal to RenderingContext::width 
    int window_height; //actual size of the window, should be equal to RenderingContext::height

    Block RTPresetBlk;

    Block gridSettingsBlk;
    Block sparseOctreeSettingsBlk;
    Block COctreeSettingsBlk;
    Block SComSettingsBlk;
    Block sceneConvertSettingsBlk;
    Block SCom2SettingsBlk;
    Block volumeSCom2SettingsBlk;

    char convertMeshInputPath[1024] = {"./scenes/01_simple_scenes/data/sphere.vsgf"};
    char convertMeshOutputFolder[1024] = {"./saves/output_scene"};
    char saveSceneOutputFolder[1024] = {"./saves/output_scene"};
    bool conversionWindowOpen = false;

    int conversionTypeIdx = (int)ConversionType::MESH_TO_SDF;
    int conversionSDFTypeIdx = (int)SDFType::FRAME_OCTREE;
    int conversionASDFTypeIdx = (int)ASDFType::MULTI_OCTREE;
    int conversionCADSDFTypeIdx = (int)CADSDFType::MULTI_OCTREE;
    int conversionVolumeType    = (int)VolumeGridType::VOLUME_GRID_4D_DENSITY;
    bool conversionLoadReferenceScene = true;
    bool conversionLoadCombinedScene  = false;
    bool conversionSaveSDFDebugInfo   = false;
    bool conversionCompressVTKChannels = false;
    bool conversionCADLoadSeparateParts = false;

    bool queryDebugRay = false;
    bool queryDebugPrimitiveInfo = false;
    bool queryDebugBuildInfo = true;
    int queryDebugIntersectedBoxID = 0;

    CAEVisualizationSettings CAEVisSettings;

    LiteScene::HydraScene scenes[MAX_RENDERERS];
    SceneUIContext     scenesCtx[MAX_RENDERERS];

    Light cameraLight = create_direct_light(float3(0.7, 0.7, 0.7), float3(0.7, 0.7, 0.7));
    Light ambientLight = create_ambient_light(float3(0.25, 0.25, 0.25));
    bool useCameraLight = true;

    int  VTKConversionOctreeDepth = 8;
    int  VTKConversionInternalDepth = 6;
    bool VTKConversionVolume = false;
    float VTKConversionPruneThr = 0.0f;
    float VTKConversionChannelPruneThr = 0.0f;

    float4x4 fixedTransformMatrix = float4x4(); // transform matrix from main scene is applied to reference scenes
    bool fixedTransformMatrixSetUp = false;     // to allow accurate comparisons

    IActiveCmdCtx *active_cmd = nullptr; // Some commands stays active for several frames (e.g. move camera by trajectory)
                                         // Other commands won't be executed until active_cmd is nullptr

    PreloadedCAEInfo preloaded_cae_info;

    QualityMetricsStat qualityMetricsStat;

    RenderType renderType = RenderType::MULTI; 
    RasterizerType rasterizerType = RasterizerType::SIMPLE;

    bool enableAA = false;
    uint32_t AAFrameNum = 0;

    bool enableDenoiser = false;

    float sigmaR = 0.25f;
    float sigmaS = 0.5f;
    int kernelSize = 5;
  };

  struct RaytracingContext
  {
    // Raytracer, main class for doing render. Can have multiple passes inside, but
    // at the end either saves RGBA8 LDR image in genColorBuffers (no TAA) or
    // RGBA32F HDR image in genColorNoAABuffers (with TAA)
    std::shared_ptr<IRenderer> pRayTracers[MAX_RENDERERS];   
    
    // Antialiaser, does TAA and tone mapping, from genColorNoAABuffers
    // to genColorBuffers
    std::shared_ptr<Antialiaser> pAntialiaser[MAX_RENDERERS];
    std::shared_ptr<Denoiser> pDenoiser[MAX_RENDERERS];

    // Kernel to compare images from different renderers (e.g. mesh and raytracing)
    // currently calculates only PSNR
    std::shared_ptr<ImgMetricsKernel> pMetricsKernel;

    std::vector<LiteMath::float4> raytracedImagesDataRaw[MAX_RENDERERS];  // Store HDR images (CPU render)
    std::vector<LiteMath::float4> raytracedImagesDataNoAA[MAX_RENDERERS]; // Store HDR images (CPU render)
    std::vector<uint32_t> raytracedImagesData[MAX_RENDERERS];             // Store LDR images (CPU render)
    VkBuffer genColorRawBuffers[MAX_RENDERERS];                           // Store HDR images (GPU render)
    VkBuffer genColorNoAABuffers[MAX_RENDERERS];                          // Store HDR images (GPU render)
    VkBuffer genColorBuffers[MAX_RENDERERS];                              // Store LDR images (GPU render)
    VkDeviceMemory colorMems[MAX_RENDERERS];                              // Memory for genColorNoAABuffers and genColorBuffers
  };

  class RecordTrajectoryCmdCtx : public IActiveCmdCtx
  {
  public:
    RecordTrajectoryCmdCtx() {cmd = Cmd::RECORD_TRAJECTORY;}

    std::vector<std::chrono::high_resolution_clock::time_point> timestamps;
    std::vector<Camera> cameras;
  };

  class PlaybackTrajectoryCmdCtx : public IActiveCmdCtx
  {
  public:
    PlaybackTrajectoryCmdCtx() {cmd = Cmd::PLAYBACK_TRAJECTORY;}

    float time_since_start_ms = 0.0f;
    unsigned cur_index = 0;
    std::vector<Camera> cameras;
    std::vector<float> times;
  };

  struct RTLayout
  {
    uint32_t primaryRTId = 0;             // What to be shown on screen
    uint32_t secondaryRTId = INVALID_IDX; // Reference image
    uint32_t auxiliaryRTId = INVALID_IDX; // Auxiliary image (for composites, TODO)
  };

  static RTLayout render_only_primaryRT(uint32_t primaryRTId) 
  { 
    return {primaryRTId, INVALID_IDX, INVALID_IDX}; 
  }

  static std::vector<uint32_t> get_active_RT_list(RTLayout rtLayout)
  {
    std::vector<uint32_t> activeRTs;
    activeRTs.push_back(rtLayout.primaryRTId);
    if (rtLayout.secondaryRTId != INVALID_IDX)
      activeRTs.push_back(rtLayout.secondaryRTId);
    if (rtLayout.auxiliaryRTId != INVALID_IDX)
      activeRTs.push_back(rtLayout.auxiliaryRTId);
    return activeRTs;
  }

  void init(const Block &blk, Settings &settings, UIContext &ui_ctx, RenderingContext &r_ctx, 
            RasterizationContext &rast_ctx, RaytracingContext &rt_ctx);
  void frame(CommandBuffer &cmd_buffer, Settings &settings, UIContext &ui_ctx, RenderingContext &r_ctx, 
                 RasterizationContext &rast_ctx, RaytracingContext &rt_ctx);
  void execute_command(const CommandBuffer::Command &cmd, CommandBuffer &cmd_buffer, Settings &settings, 
                       UIContext &ui_ctx, RenderingContext &r_ctx, RasterizationContext &rast_ctx, 
                       RaytracingContext &rt_ctx);

  void update_lights(Settings & settings, UIContext & ui_ctx, RaytracingContext & rt_ctx, RasterizationContext&rast_ctx, RenderingContext & r_ctx);
};