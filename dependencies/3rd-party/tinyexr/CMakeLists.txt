
add_library(tinyexr STATIC tinyexr.cpp)
target_include_directories(tinyexr PUBLIC .)
target_link_libraries(tinyexr PUBLIC miniz)
