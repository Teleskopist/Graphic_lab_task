
add_library(tiny_obj_loader STATIC)
target_sources(tiny_obj_loader PRIVATE tiny_obj_loader.cpp)
target_include_directories(tiny_obj_loader PUBLIC .)
