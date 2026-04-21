
litert_import(../LiteApp)

add_compile_definitions(USE_VOXEL_RENDERER)
add_compile_definitions(USE_VOLUME_RENDERER)
add_executable(engine)
target_sources(engine PRIVATE 
                main.cpp
                app.cpp
                render_raytracing.cpp
                app_execute_commands.cpp

                extended/rasterization/TerrainRasterizer.cpp
                extended/rasterization/ImageTerrainRasterizer.cpp
                extended/rasterization/BCImageTerrainRasterizer.cpp
                extended/rasterization/QuadTreeTerrainRasterizer.cpp)
target_link_libraries(engine PRIVATE
                      sdf_extended
                      LiteApp 
                      Antialiaser 
                      Denoiser 
                      ImgMetricsKernel 
                      VolumeRenderer
                      VoxelRenderer)

if (BUILD_EXTENDED_DEMO_APP)
    add_compile_definitions(USE_HYDRA)
    add_compile_definitions(USE_MULTI_RENDERER)
    add_compile_definitions(USE_CSG_DIRECT_RENDERER)
    target_sources(engine PRIVATE 
                   plugins/MRFull/execute_commands.cpp
                   plugins/MRFull/interface.cpp
                   )
    target_link_libraries(engine PRIVATE HydraRenderer MultiRenderer)
endif()
