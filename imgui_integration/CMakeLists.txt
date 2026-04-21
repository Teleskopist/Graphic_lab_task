litert_import(../dependencies)
include(dependencies/glfw)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/imgui)

set(IMGUI_COMMON_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)

add_library(imgui_headers INTERFACE)
target_include_directories(imgui_headers INTERFACE ${IMGUI_DIR} ${IMGUI_DIR}/backends ${IMGUI_DIR}/..)

if(USE_VULKAN)
    add_library(imgui_glfw_vulkan STATIC
        ${IMGUI_COMMON_SOURCES}
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    )
    target_compile_definitions(imgui_glfw_vulkan PUBLIC IMGUI_USER_CONFIG=\"${CMAKE_CURRENT_SOURCE_DIR}/imgui_vulkan_config.h\")
    target_link_libraries(imgui_glfw_vulkan PUBLIC
        imgui_headers
        glfw
        volk::volk
    )
endif()

if(USE_WEBGPU)
    add_library(imgui_glfw_webgpu STATIC
        ${IMGUI_COMMON_SOURCES}
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_wgpu.cpp
    )
    target_link_libraries(imgui_glfw_webgpu PUBLIC
        imgui_headers
        glfw
        webgpu
    )

    if(WEBGPU_BACKEND STREQUAL "WGPU")
        target_compile_definitions(imgui_glfw_webgpu PUBLIC IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
    elseif(WEBGPU_BACKEND STREQUAL "DAWN")
        target_compile_definitions(imgui_glfw_webgpu PUBLIC IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
    endif()
endif()

if (USE_SDL2)
    add_library(imgui_sdl2 STATIC
        ${IMGUI_COMMON_SOURCES}
        ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
    )
    target_link_libraries(imgui_sdl2 PUBLIC
        imgui_headers
        SDL2::SDL2
    )
endif()
