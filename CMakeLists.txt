cmake_minimum_required(VERSION 3.15)

project(LiteRT C CXX)

include(litert.cmake)

include(cmake/kslicer_configs.cmake)
litert_import(dependencies)
litert_import(utils)
litert_import(core)
litert_import(sdf)
litert_import(PostEffects/antialiasing)

if(BUILD_RENDER_CLI OR BUILD_EXTENDED_DEMO_APP)
    litert_import(MultiRenderer)
    litert_import(VolumeRenderer)
    litert_import(VoxelRenderer)
    litert_import(HydraRenderer)
    litert_import(PostEffects/denoising)
    litert_import(PostEffects/image_metrics)
endif()

if(BUILD_RENDER_CLI)
    litert_import(render_CLI)
endif()

if (USE_VULKAN)
    if(BUILD_DEMO_APP OR USE_WEBGPU)
        litert_import(imgui_integration)
    endif()

    if(BUILD_DEMO_APP)
        litert_import(demo_app)
    endif()

    if(USE_WEBGPU)
        litert_import(webgpu_demo_app)
    endif()
endif()
