

FetchContent_Declare(
    eigen3
    URL https://gitlab.com/libeigen/eigen/-/archive/5.0.1/eigen-5.0.1.zip
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)
set(CMAKE_MESSAGE_LOG_LEVEL WARNING)
FetchContent_MakeAvailable(eigen3)
set(CMAKE_MESSAGE_LOG_LEVEL STATUS)

FetchContent_Declare(
    libigl
    URL https://github.com/libigl/libigl/archive/refs/tags/v2.6.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)
set(CMAKE_MESSAGE_LOG_LEVEL WARNING)
FetchContent_MakeAvailable(libigl)
set(CMAKE_MESSAGE_LOG_LEVEL STATUS)

target_link_libraries(igl_core INTERFACE Eigen3::Eigen)
