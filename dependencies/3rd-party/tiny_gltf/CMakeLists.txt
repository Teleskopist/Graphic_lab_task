
add_library(tiny_gltf STATIC)
target_sources(tiny_gltf PRIVATE tiny_gltf.cpp)
target_include_directories(tiny_gltf PUBLIC .)
target_link_libraries(tiny_gltf PUBLIC nlohmann_json stb_image)
