option(ONLY_SINGLE_MODEL_SCOM "Only use single model for SCom" OFF)

add_library(MultiRenderer STATIC)
target_sources(MultiRenderer PRIVATE
    MultiRenderer.cpp
    MultiRenderer_host.cpp
    MultiRenderer_resolve.cpp
    BVH2Common.cpp
    BVH2Common_host.cpp
    bvh_info.cpp
)
target_link_libraries(MultiRenderer PUBLIC sdf_extended)
if (ONLY_SINGLE_MODEL_SCOM)
    target_compile_definitions(MultiRenderer PUBLIC ONLY_SINGLE_MODEL_SCOM=1)
endif()

if(USE_VULKAN)
    target_sources(MultiRenderer PRIVATE
        app_renderer.cpp
        MultiRenderer_slang_comp.cpp
        MultiRenderer_slang_comp_ds.cpp
        MultiRenderer_slang_comp_init.cpp
    )

    if(USE_GPU_RQ)
        target_compile_definitions(MultiRenderer PUBLIC USE_GPU_RQ=1)
        target_sources(MultiRenderer PRIVATE
            MultiRenderer_slang_comp_rq.cpp
            MultiRenderer_slang_comp_rq_ds.cpp
            MultiRenderer_slang_comp_rq_init.cpp
        )
    endif()
endif()
