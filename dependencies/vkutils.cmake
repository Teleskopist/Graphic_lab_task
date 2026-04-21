include_guard(GLOBAL)
include(litert)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XLIB_KHR)
endif()

litert_import(3rd-party/volk)

add_library(vkutils STATIC)
target_sources(vkutils PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/1st-party/vkutils/vk_utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/1st-party/vkutils/vk_copy.cpp
    ${CMAKE_CURRENT_LIST_DIR}/1st-party/vkutils/vk_buffers.cpp
    ${CMAKE_CURRENT_LIST_DIR}/1st-party/vkutils/vk_images.cpp
    ${CMAKE_CURRENT_LIST_DIR}/1st-party/vkutils/vk_context.cpp
    ${CMAKE_CURRENT_LIST_DIR}/1st-party/vkutils/vk_alloc_simple.cpp
    ${CMAKE_CURRENT_LIST_DIR}/1st-party/vkutils/vk_pipeline.cpp
    ${CMAKE_CURRENT_LIST_DIR}/1st-party/vkutils/vk_descriptor_sets.cpp
    ${CMAKE_CURRENT_LIST_DIR}/1st-party/vkutils/vk_swapchain.cpp
    ${CMAKE_CURRENT_LIST_DIR}/1st-party/vkutils/vk_quad.cpp
    ${CMAKE_CURRENT_LIST_DIR}/1st-party/vkutils/vk_fbuf_attachment.cpp
    ${CMAKE_CURRENT_LIST_DIR}/1st-party/vkutils/geom/cmesh.cpp
    ${CMAKE_CURRENT_LIST_DIR}/1st-party/vkutils/geom/vk_mesh.cpp
    ${CMAKE_CURRENT_LIST_DIR}/1st-party/vkutils/ray_tracing/vk_rt_utils.cpp
)
target_include_directories(vkutils PUBLIC ${CMAKE_CURRENT_LIST_DIR}/1st-party/vkutils)
target_compile_definitions(vkutils PUBLIC USE_VOLK=1)
target_link_libraries(vkutils PUBLIC volk)
