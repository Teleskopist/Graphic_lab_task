
add_library(stb_image STATIC)
target_sources(stb_image PRIVATE stb_image.c stb_image_write.c)
target_include_directories(stb_image PUBLIC .)
