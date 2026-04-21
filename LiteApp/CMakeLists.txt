cmake_minimum_required(VERSION 3.15)
include(../litert.cmake)
litert_import(../dependencies)
litert_import(../core)
litert_import(../imgui_integration)

add_library(LiteApp STATIC)
target_sources(LiteApp PRIVATE
    lite_app.cpp
    lite_app_imgui.cpp
    rasterization/vk_buffer_helpers.cpp
    rasterization/scn_manager_load.cpp
    rasterization/IRasterizer.cpp
    rasterization/SimpleRasterizer.cpp
    rasterization/context.cpp
)
target_link_libraries(LiteApp PUBLIC imgui_glfw_vulkan core)
