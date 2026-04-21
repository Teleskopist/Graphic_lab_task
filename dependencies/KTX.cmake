
if(WIN32)
    find_program(BASH_EXECUTABLE git)
    set(BASH_EXECUTABLE ${BASH_EXECUTABLE}/../../git-bash.exe)
    cmake_path(NORMAL_PATH BASH_EXECUTABLE OUTPUT_VARIABLE BASH_EXECUTABLE)
endif()

set(BASISU_SUPPORT_SSE OFF)
FetchContent_Declare(
    KTX
    URL https://github.com/KhronosGroup/KTX-Software/archive/refs/tags/v4.4.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)
set(KTX_VERSION 4.4.2)
set(KTX_GIT_VERSION_FULL ON)
set(CMAKE_MESSAGE_LOG_LEVEL ERROR)
FetchContent_MakeAvailable(KTX)
set(CMAKE_MESSAGE_LOG_LEVEL STATUS)
