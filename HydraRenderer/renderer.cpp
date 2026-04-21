#include "renderer.h"
#include "HydraCore3/integrator_pt.h"
#include "LiteScene/scene.h"
#ifdef USE_GPU
  #include "HydraCore3/Integrator_glsl_litert.h"
  using Integrator_LiteRT_GPU = Integrator_glsl_litert;
  #include "vk_context.h"
#endif

std::shared_ptr<ISceneObject> CreateSceneRT(const char* a_implName, const char* a_buildName, const char* a_layoutName);

void toLDRImage(const float *rgb, int width, int height, float a_normConst, float a_gamma, uint32_t *pixelData, bool a_flip)
{
  const float invGamma = 1.0f / a_gamma;

  for (int y = 0; y < height; y++) // flip image and extract pixel data
  {
    int offset1 = y * width;
    int offset2 = a_flip ? (height - y - 1) * width : offset1;
    for (int x = 0; x < width; x++)
    {
      float color[4];
      color[0] = clamp(std::pow(rgb[4 * (offset1 + x) + 0] * a_normConst, invGamma), 0.0f, 1.0f);
      color[1] = clamp(std::pow(rgb[4 * (offset1 + x) + 1] * a_normConst, invGamma), 0.0f, 1.0f);
      color[2] = clamp(std::pow(rgb[4 * (offset1 + x) + 2] * a_normConst, invGamma), 0.0f, 1.0f);
      color[3] = 1.0f;
      pixelData[offset2 + x] = RealColorToUint32(color);
    }
  }
}

void toHDRImage(const float *rgb, int width, int height, float a_normConst, float a_gamma, float4 *pixelData, bool a_flip)
{
  const float invGamma = 1.0f / a_gamma;

  for (int y = 0; y < height; y++) // flip image and extract pixel data
  {
    int offset1 = y * width;
    int offset2 = a_flip ? (height - y - 1) * width : offset1;
    for (int x = 0; x < width; x++)
    {
      pixelData[offset2 + x].x = clamp(std::pow(rgb[4 * (offset1 + x) + 0] * a_normConst, invGamma), 0.0f, 1.0f);
      pixelData[offset2 + x].y = clamp(std::pow(rgb[4 * (offset1 + x) + 1] * a_normConst, invGamma), 0.0f, 1.0f);
      pixelData[offset2 + x].z = clamp(std::pow(rgb[4 * (offset1 + x) + 2] * a_normConst, invGamma), 0.0f, 1.0f);
      pixelData[offset2 + x].w = 1.0f;
    }
  }
}

HydraRenderPreset getDefaultHydraRenderPreset()
{
  HydraRenderPreset preset;
  preset.integratorType = Integrator::INTEGRATOR_MIS_PT;
  preset.fbLayer = Integrator::FB_COLOR;
  preset.spp = 1;

  return preset;
}

HydraRenderer::HydraRenderer(unsigned device)
{
  m_preset = getDefaultHydraRenderPreset();
  m_device = device;
}

bool HydraRenderer::LoadScene(LiteScene::HydraScene &scene)
{
  return LoadScene(scene.metadata.scene_xml_path.c_str());
}

  bool HydraRenderer::LoadScene(const char* a_scenePath)
  {
    if (m_pImpl)
      m_pImpl.reset();

    //get info about features in scene
    std::string sceneDir       = "";
    SceneInfo sceneInfo = {};
    sceneInfo.spectral  = false;
    auto features = Integrator::PreliminarySceneAnalysis(a_scenePath, sceneDir.c_str(), &sceneInfo);

    //create Hydra renderer for this scene
#ifdef USE_GPU
    if(m_device != DEVICE_CPU)
    {
      unsigned int a_preferredDeviceId = vk_utils::CHOOSE_DEVICE_BY_NAME;

      size_t gpuAuxMemSize = MAX_WIDTH*MAX_HEIGHT*sizeof(float) + 16 * 1024 * 1024; // reserve for frame buffer and other

      // advanced way, init device with features which is required by generated class
      std::vector<const char*> requiredExtensions;
      auto deviceFeatures = Integrator_LiteRT_GPU::ListRequiredDeviceFeatures(requiredExtensions);
      auto ctx            = vk_utils::globalContextInit(requiredExtensions, true, a_preferredDeviceId, &deviceFeatures, gpuAuxMemSize, 1);

      // advanced way, you can disable some pipelines creation which you don't actually need;
      // this will make application start-up faster
      Integrator_LiteRT_GPU::EnabledPipelines().enableRayTraceMega               = true;
      Integrator_LiteRT_GPU::EnabledPipelines().enableCastSingleRayMega          = false; // not used, for testing only
      Integrator_LiteRT_GPU::EnabledPipelines().enablePackXYMega                 = true;  // always true for this main.cpp;
      Integrator_LiteRT_GPU::EnabledPipelines().enablePathTraceFromInputRaysMega = false; // always false in this main.cpp; see cam_plugin main
      Integrator_LiteRT_GPU::EnabledPipelines().enablePathTraceMega              = true;
      Integrator_LiteRT_GPU::EnabledPipelines().enableNaivePathTraceMega         = false;

      // advanced way
      auto pObj = std::make_shared<Integrator_LiteRT_GPU>(MAX_WIDTH*MAX_HEIGHT, features);
      pObj->SetAccelStruct(CreateSceneRT("BVH2Common", "cbvh_embree2", "SuperTreeletAlignedMerged4"));
      pObj->SetResourcesDir("./HydraCore3"); //directory to search for shaders
      pObj->SetVulkanContext(ctx);
      pObj->InitVulkanObjects(ctx.device, ctx.physicalDevice, MAX_WIDTH*MAX_HEIGHT);
      m_pImpl = pObj;
    }
    else
#endif
    {
      m_pImpl = std::make_shared<Integrator>(MAX_WIDTH*MAX_HEIGHT,features);
      m_pImpl->SetAccelStruct(CreateSceneRT("BVH2Common", "cbvh_embree2", "SuperTreeletAlignedMerged4"));
    }

    SetViewport(0, 0, 1024, 1024);

    //actually load scene
    m_pImpl->LoadScene(a_scenePath, sceneDir.c_str());

    return true;
  }

  void HydraRenderer::Clear(uint32_t a_width, uint32_t a_height)
  {
    assert(m_pImpl);
    m_pImpl->PackXYBlock(m_width, m_height, 1);
  }
  void HydraRenderer::RenderInternal(uint32_t a_width, uint32_t a_height, int a_passNum)
  {
    assert(m_pImpl);

    float timings[4] = {0,0,0,0};
    m_pImpl->SetFrameBufferLayer(m_preset.fbLayer);
    // m_pImpl->SetCamId(0);
    m_pImpl->SetViewport(0, 0, m_width, m_height);

    //std::cout << "[main]: PathTraceBlock(MIS-PT) ... " << std::endl;
    std::fill(realColor.begin(), realColor.end(), 0.0f);
    
    m_pImpl->SetIntegratorType(m_preset.integratorType);
    m_pImpl->UpdateMembersPlainData();

    for (int i = 0; i < a_passNum; i++)
      m_pImpl->PathTraceBlock(m_width*m_height, 4, realColor.data(), m_preset.spp);
      
    m_pImpl->GetExecutionTime("PathTraceBlock", timings);
    std::cout << "PathTraceBlock(exec) = " << timings[0]              << " ms " << std::endl;
    std::cout << "PathTraceBlock(copy) = " << timings[1] + timings[2] << " ms " << std::endl;
    std::cout << "PathTraceBlock(ovrh) = " << timings[3]              << " ms " << std::endl;
  }
  void HydraRenderer::Render(uint32_t* imageData, uint32_t a_width, uint32_t a_height, int a_passNum)
  {
    assert(a_width == m_width && a_height == m_height);
    const float normConst = 1.0f/float(a_passNum*m_preset.spp);
    RenderInternal(a_width, a_height, a_passNum);
    toLDRImage(realColor.data(), m_width, m_height, normConst, GAMMA, imageData, true);
  }

  void HydraRenderer::RenderFloat(LiteMath::float4* imageData, uint32_t a_width, uint32_t a_height, int a_passNum)
  {
    assert(a_width == m_width && a_height == m_height);
    RenderInternal(a_width, a_height, a_passNum);
    toHDRImage(realColor.data(), m_width, m_height, 1.0f, GAMMA, imageData, true);
  }

  void HydraRenderer::SetViewport(int a_xStart, int a_yStart, int a_width, int a_height) 
  {
    assert(a_width <= MAX_WIDTH && a_height <= MAX_HEIGHT);

    if (m_width != a_width || m_height != a_height)
    {
      m_width  = a_width;
      m_height = a_height;
      realColor.resize(m_width * m_height * 4);
    }

    m_pImpl->SetSpectralMode(false);
    m_pImpl->SetFrameBufferSize(m_width, m_height);
    m_pImpl->SetViewport(a_xStart,a_yStart,m_width,m_height);
  }

  void HydraRenderer::SetAccelStruct(std::shared_ptr<ISceneObject> a_customAccelStruct)
  {
    assert(m_pImpl);
    m_pImpl->SetAccelStruct(a_customAccelStruct);
  }

  ISceneObject *HydraRenderer::GetAccelStruct()
  {
    assert(m_pImpl);
    return m_pImpl->m_pAccelStruct.get();
  }
  
  void HydraRenderer::GetExecutionTime(const char* a_funcName, float a_out[4])
  {
    assert(m_pImpl);
    if (std::string (a_funcName) == "Render")
      m_pImpl->GetExecutionTime("PathTraceBlock", a_out);
    else if (std::string (a_funcName) == "CastRaySingleMega")
      m_pImpl->GetExecutionTime("PathTraceMega", a_out);
    else
      m_pImpl->GetExecutionTime(a_funcName, a_out);
  }

  void HydraRenderer::CommitDeviceData()
  {
    assert(m_pImpl);
    m_pImpl->CommitDeviceData();
  }

  void HydraRenderer::UpdateCamera(const LiteMath::float4x4& a_worldView, const LiteMath::float4x4& a_proj)
  {
    assert(m_pImpl);
    m_pImpl->SetWorldView(a_worldView);
    m_pImpl->SetProj(a_proj);
  }

  void save_to_blk(const HydraRenderPreset &settings, Block *block)
  {
    block->set_int("spp", settings.spp);
    block->set_int("fbLayer", settings.fbLayer);
    block->set_int("integratorType", settings.integratorType);
  }

  void load_from_blk(HydraRenderPreset &settings, const Block *block)
  {
    settings = getDefaultHydraRenderPreset();
    settings.spp = block->get_int("spp", settings.spp);
    settings.fbLayer = block->get_int("fbLayer", settings.fbLayer);
    settings.integratorType = block->get_int("integratorType", settings.integratorType);
  }