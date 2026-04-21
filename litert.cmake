include_guard()

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "LiteRT or its subproject must always be top level project")
endif()

if(NOT(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
    message(FATAL_ERROR "litert.cmake should be included in topmost CMakeLists.txt of project or subproject")
endif()

#
# Should be exactly root of LiteRT repository
#
set(LITERT_ROOT ${CMAKE_CURRENT_LIST_DIR})

#
# Adding module paths
#
list(APPEND CMAKE_MODULE_PATH ${LITERT_ROOT} ${LITERT_ROOT}/cmake)

#
# Function litert_import() should be used instead of add_subdirectory()
#
include(litert_import)

macro(_litert_check_option opt)
    if(NOT(DEFINED ${opt}))
        message(FATAL_ERROR "Expected option ${opt} to be defined")
    endif()
endmacro()

#
# General project options
#
include(litert_project_options)

#
# Setting up where built artifacts should go
#
include(litert_output_directories)

#
# Setting up compiler warnings
#
include(litert_warnings)

#
# Setting up compiler flags related to building: c++ standard, optimizations, sanitizers, etc.
#
include(litert_build_settings)
