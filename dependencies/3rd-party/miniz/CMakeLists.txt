
add_library(miniz STATIC)
target_sources(miniz PRIVATE miniz.c)
target_include_directories(miniz PUBLIC .)
