include(litert)
include(dependencies/stb_image)
include(dependencies/LiteMath)
include(dependencies/blk)
include(dependencies/tiny_obj_loader)
include(dependencies/embree3)
include(dependencies/pugixml)
include(dependencies/tinyexr)
include(dependencies/flip)

file(GLOB_RECURSE UTILS_SOURCES *.cpp)

add_library(utils STATIC)
target_sources(utils PRIVATE ${UTILS_SOURCES})
target_include_directories(utils PUBLIC . ..)
target_link_libraries(utils PUBLIC
    LiteMath
    LiteImage
    stb_image
    blk
    tiny_obj_loader
    embree3
    pugixml
    tinyexr
    flip
)

target_compile_definitions(utils PUBLIC ON_CPU=1)

if(USE_VULKAN)
    include(dependencies/vkutils)
    target_compile_definitions(utils PUBLIC USE_VULKAN=1 USE_GPU=1)
    target_link_libraries(utils PUBLIC vkutils)
endif()

if(USE_OPENVDB)
    target_link_libraries(utils PUBLIC openvdb)
else()
    target_compile_definitions(utils PUBLIC DISABLE_OPENVDB=1)
endif()

if(USE_LIBIGL)
    target_compile_definitions(utils PUBLIC USE_LIBIGL=1)
    target_link_libraries(utils PUBLIC igl::core)
endif()

if(USE_KTX)
    target_compile_definitions(utils PUBLIC USE_KTX=1)
    target_link_libraries(utils PUBLIC ktx)
endif()
