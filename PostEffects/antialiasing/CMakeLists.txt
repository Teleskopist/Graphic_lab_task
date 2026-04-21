add_library(Antialiaser STATIC)
target_sources(Antialiaser PRIVATE ${CMAKE_SOURCE_DIR}/PostEffects/antialiasing/antialiaser.cpp)
target_link_libraries(Antialiaser PUBLIC core)

if(USE_VULKAN)
    target_sources(Antialiaser PRIVATE
        Antialiaser_slang.cpp
        Antialiaser_slang_ds.cpp
        Antialiaser_slang_init.cpp
    )
endif()

if(USE_WEBGPU)
    target_sources(Antialiaser PRIVATE antialiaser_wgpu.cpp)
endif()
