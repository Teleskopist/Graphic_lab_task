include_guard()
include(litert)
include(dependencies/stb_image)

add_library(LiteMath INTERFACE EXCLUDE_FROM_ALL)
target_include_directories(LiteMath SYSTEM INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/1st-party
    ${CMAKE_CURRENT_LIST_DIR}/1st-party/LiteMath
    ${CMAKE_CURRENT_LIST_DIR}/1st-party/LiteMath/external
)

add_library(LiteImage STATIC EXCLUDE_FROM_ALL ${CMAKE_CURRENT_LIST_DIR}/1st-party/LiteMath/Image2d.cpp)
target_compile_definitions(LiteImage PRIVATE
    USE_STB_IMAGE=1
    STB_IMAGE_STATIC=1
    STB_IMAGE_WRITE_STATIC=1
)
target_link_libraries(LiteImage PUBLIC LiteMath stb_image)
