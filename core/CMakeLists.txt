include(../litert.cmake)
litert_import(../utils)
litert_import(../LiteScene)
litert_import(../CrossRT)

add_library(core STATIC)
target_sources(core PRIVATE 
               serialization.cpp 
               scene_extension.cpp
               ISimpleCombinedRenderer_shared.cpp
               ISimpleCombinedRenderer_host.cpp)
target_link_libraries(core PUBLIC LiteScene CrossRT)