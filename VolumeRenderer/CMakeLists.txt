
add_library(VolumeRenderer STATIC)
target_sources(VolumeRenderer PRIVATE
    VolumeRenderer.cpp
    VolumeRenderer_host.cpp
    sparse_octree_density_grid.cpp
    VolumeRenderer_enums.cpp
)
target_link_libraries(VolumeRenderer PUBLIC sdf_extended)

if(USE_VULKAN)
    target_sources(VolumeRenderer PRIVATE
        VolumeRenderer_slang.cpp
        VolumeRenderer_slang_ds.cpp
        VolumeRenderer_slang_init.cpp
    )
endif()
