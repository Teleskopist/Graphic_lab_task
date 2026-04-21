
if(USE_HYDRA)
    set(USE_LITERT ON)
    litert_import(../HydraCore3)
endif()

add_library(hydra_headers INTERFACE)
target_include_directories(hydra_headers INTERFACE ..)

add_library(hydra_gltf_writer STATIC
    gltf_structure_writer.cpp
    general_gltf_writer.cpp
)
target_link_libraries(hydra_gltf_writer PUBLIC hydra_headers core)

if(USE_HYDRA)
    target_sources(hydra_gltf_writer PRIVATE scene.cpp)
    target_link_libraries(hydra_gltf_writer PUBLIC hydra)
    target_compile_definitions(hydra_gltf_writer PUBLIC USE_HYDRA=1)
else()
    target_sources(hydra_gltf_writer PRIVATE scene_stub.cpp)
endif()

add_library(HydraRenderer STATIC)
target_link_libraries(HydraRenderer PUBLIC hydra_gltf_writer)

if(USE_HYDRA)
    target_sources(HydraRenderer PRIVATE renderer.cpp)
else()
    target_sources(HydraRenderer PRIVATE renderer_stub.cpp)
endif()
