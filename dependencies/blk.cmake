include_guard()
include(litert)
include(dependencies/LiteMath)

add_library(blk STATIC ${CMAKE_CURRENT_LIST_DIR}/blk/blk.cpp)
target_include_directories(blk PUBLIC SYSTEM ${CMAKE_CURRENT_LIST_DIR}/blk ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(blk PUBLIC LiteMath)
