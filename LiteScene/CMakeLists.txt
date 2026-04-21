

include(../litert.cmake)
litert_import(../utils)
include(dependencies/tiny_gltf)

add_library(LiteScene STATIC)
target_sources(LiteScene PRIVATE
    hydraxml.cpp
    scene.cpp
    scene_mat.cpp
    scene_tex.cpp
    scene_convert.cpp
)
target_include_directories(LiteScene PUBLIC .)
target_link_libraries(LiteScene PUBLIC
    utils
    tiny_gltf
    tinyexr
    stb_image
)

if(USE_VULKAN)
    add_library(LiteScene_SceneManager STATIC)
    target_sources(LiteScene_SceneManager PRIVATE scene_mgr.cpp)
    target_link_libraries(LiteScene_SceneManager PUBLIC LiteScene)
endif()
