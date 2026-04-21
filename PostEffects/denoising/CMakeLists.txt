add_library(Denoiser STATIC)
target_sources(Denoiser PRIVATE denoiser.cpp)
target_link_libraries(Denoiser PUBLIC core)

if(USE_VULKAN)
    target_sources(Denoiser PRIVATE
        Denoiser_glsl.cpp
        Denoiser_glsl_ds.cpp
        Denoiser_glsl_init.cpp
    )
endif()
