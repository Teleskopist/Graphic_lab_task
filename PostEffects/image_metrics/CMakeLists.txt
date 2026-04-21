add_library(ImgMetricsKernel STATIC)
target_sources(ImgMetricsKernel PRIVATE image_metrics.cpp)
target_link_libraries(ImgMetricsKernel PUBLIC core)

if(USE_VULKAN)
    target_sources(ImgMetricsKernel PRIVATE
        ImgMetricsKernel_glsl_ds.cpp
        ImgMetricsKernel_glsl_init.cpp
        ImgMetricsKernel_glsl.cpp
    )
endif()
