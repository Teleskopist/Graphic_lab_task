include(../litert.cmake)
include(dependencies/SimplexNoise)

add_library(sdf_core STATIC
    build/sparse_octree_builder.cpp
    build/sparse_octree_augmentation.cpp
    build/sparse_octree_unstructured_grid.cpp
    build/sparse_octree_flood.cpp
    build/sparse_octree_common.cpp
    build/sparse_octree_merge.cpp
    build/sdf_lod_maker.cpp
    build/voxel_metrics.cpp
    build/scene_converter.cpp
    build/sparse_octree_builder_core.cpp
    build/point_cloud_process.cpp

    common/global_octree.cpp

    multi/sdf_multi.cpp
    multi/sdf_multi_debug.cpp

    simple/sdf_simple.cpp
    simple/sdf_converter.cpp
    simple/sdf_converter_exotic.cpp
    simple/iou.cpp
)
target_link_libraries(sdf_core PUBLIC core)

add_library(sdf_extended STATIC
    scom/scom.cpp
    scom/compact_octree_builder.cpp
    scom/similarity_compression.cpp

    scom2/similarity_compression.cpp
    scom2/scom_builder.cpp
    scom2/scom_internal.cpp
    scom2/converter_neural.cpp
    scom2/scom_utils.cpp
    scom2/scom_enums.cpp
    scom2/packing.cpp
    scom2/common_descriptor_makers.cpp
    scom2/sdf_bricks_dataset.cpp
    scom2/channels_dataset.cpp
    scom2/branches_dataset.cpp
    scom2/marching_cubes_scom2.cpp

    scom2/clustering/clustering.cpp
    scom2/clustering/symmetric_groups.cpp
    scom2/clustering/octahedral_groups.cpp

    scom2/radiance_field/sh_rotate.cpp
    scom2/radiance_field/radiance_field.cpp
    scom2/radiance_field/sh_clustering.cpp

    scom2/2D/scom2D_builder.cpp
)
target_link_libraries(sdf_extended PUBLIC sdf_core SimplexNoise)
