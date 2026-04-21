include(../litert.cmake)
litert_import(../LiteScene)

add_library(CrossRT STATIC)

if (USE_VULKAN)
    target_sources(CrossRT PRIVATE
        EmbreeRT.cpp
        VulkanRTX.cpp
    )
    target_include_directories(CrossRT PUBLIC .)
    target_link_libraries(CrossRT PUBLIC LiteScene LiteScene_SceneManager)
else()
    target_sources(CrossRT PRIVATE
        EmbreeRT.cpp
    )
    target_include_directories(CrossRT PUBLIC .)
    target_link_libraries(CrossRT PUBLIC LiteScene)
endif()
