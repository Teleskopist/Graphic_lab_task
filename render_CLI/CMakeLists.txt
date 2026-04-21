
add_executable(render_app
    model_check.cpp
    lsa_map_generator.cpp
    brick_dataset_generator.cpp
    scom2_packing_sandbox.cpp
    merge_scenes.cpp
    offscreen_render.cpp
    demos_scom.cpp
    main.cpp
)
target_link_libraries(render_app PRIVATE                       
                      MultiRenderer                  
                      VolumeRenderer
                      VoxelRenderer
                      HydraRenderer)
