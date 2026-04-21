
macro(check_lib lib)
    if(NOT(TARGET ${lib}))
        message(FATAL_ERROR "Expected target ${lib} to be defined")
    endif()

    message(STATUS "Imported library ${lib}")
endmacro()

if (USE_WEBGPU)
    include(webgpu.cmake)
    check_lib(webgpu)
endif()

if(USE_OPENVDB)
    find_package(OpenVDB REQUIRED)
    check_lib(openvdb)
endif()

if (USE_KTX)
    include(KTX.cmake)
    check_lib(ktx)
endif()

if (USE_LIBIGL)
    include(igl.cmake)
    check_lib(Eigen3::Eigen)
    check_lib(igl::core)
endif()
