include_guard()

set(LITERT_CXX_STANDARD 17)

set(LITERT_RELEASE_COMPILE_OPTIONS -O3)
set(LITERT_DEBUG_COMPILE_OPTIONS -g -ggdb -O0 -mno-avx512f) # Valgrind does not support AVX512 properly, explicitly disable it
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer -fsanitize=leak -fsanitize=undefined -fsanitize=bounds-strict")

_litert_check_option(USE_FAST_MATH)

if(USE_FAST_MAST_MATH)
    list(APPEND LITERT_RELEASE_COMPILE_OPTIONS -ffast-math)
endif()

_litert_check_option(USE_MARCH_NATIVE)

if(USE_MARCH_NATIVE)
    list(APPEND LITERT_RELEASE_COMPILE_OPTIONS -march=native)
endif()

#
# Threads library for Unix systems
#
find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

#
# Always linking with OpenMP even if it is disabled
#
_litert_check_option(USE_OMP)
find_package(OpenMP REQUIRED)
link_libraries(${OpenMP_CXX_LIBRARIES})

if(USE_OMP)
    add_compile_options(${OpenMP_CXX_FLAGS})

    if(MSVC)
        add_compile_options(-openmp:llvm)
    endif()
endif()

#
# Disabling some annoying windows warnings
#
if(WIN32)
    add_definitions(
        -D_CRT_SECURE_NO_WARNINGS
        -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
endif()

#
# Setting c++ standard
#
if(NOT(DEFINED CMAKE_CXX_STANDARD))
    set(CMAKE_CXX_STANDARD ${LITERT_CXX_STANDARD})
    set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
endif()

#
# Setting release flags
#
foreach(i IN LISTS LITERT_RELEASE_COMPILE_OPTIONS)
    add_compile_options($<$<CONFIG:Release>:${i}>)
endforeach()

#
# Setting debug flags
#
foreach(i IN LISTS LITERT_DEBUG_COMPILE_OPTIONS)
    add_compile_options($<$<CONFIG:Debug>:${i}>)
endforeach()
