
add_library(tinyexr STATIC tinyexr.cpp)
target_include_directories(tinyexr PUBLIC .)
target_link_libraries(tinyexr PUBLIC miniz)

# Suppress overly aggressive bounds checking in GCC 16+
target_compile_options(tinyexr PRIVATE -Wno-array-bounds -Wno-stringop-overflow)
