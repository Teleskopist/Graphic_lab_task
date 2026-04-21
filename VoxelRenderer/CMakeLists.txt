add_library(VoxelRenderer STATIC)
target_sources(VoxelRenderer PRIVATE
    voxel_octree.cpp
    voxel_dag.cpp
    procedural_voxel_grid.cpp
    cave_generator.cpp
    VoxelRenderer.cpp
    VoxelRenderer_host.cpp
)
target_link_libraries(VoxelRenderer PUBLIC sdf_extended)

if(USE_VULKAN)
    target_sources(VoxelRenderer PRIVATE
        VoxelRenderer_slang.cpp
        VoxelRenderer_slang_ds.cpp
        VoxelRenderer_slang_init.cpp
    )
endif()
