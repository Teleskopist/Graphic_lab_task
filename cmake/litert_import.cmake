include_guard()

if(NOT(DEFINED LITERT_ROOT))
    message(FATAL_ERROR "LITERT_ROOT is not defined.")
endif()

function(litert_import module_relative_path)
    cmake_parse_arguments(PARSE_ARGV 1 "arg" "EXCLUDE_FROM_ALL;SYSTEM" "" "")

    set(EXCLUDE_FROM_ALL "")
    set(SYSTEM "")

    if(arg_EXCLUDE_FROM_ALL)
        set(EXCLUDE_FROM_ALL EXCLUDE_FROM_ALL)
    endif()

    if(arg_SYSTEM)
        set(SYSTEM SYSTEM)
    endif()

    set(module_path ${CMAKE_CURRENT_LIST_DIR}/${module_relative_path})
    cmake_path(NORMAL_PATH module_path OUTPUT_VARIABLE module_path)
    cmake_path(RELATIVE_PATH module_path BASE_DIRECTORY ${LITERT_ROOT} OUTPUT_VARIABLE module_path)

    get_property(imported_modules GLOBAL PROPERTY LITERT_IMPORTED_MODULES)

    if("${module_path}" IN_LIST imported_modules)
        return()
    endif()

    set_property(GLOBAL APPEND PROPERTY LITERT_IMPORTED_MODULES "${module_path}")
    set(LITERT_IMPORTED_MODULES ${imported_modules} ${module_path})
    add_subdirectory(${LITERT_ROOT}/${module_path} ${CMAKE_CURRENT_BINARY_DIR}/${module_path} ${EXCLUDE_FROM_ALL})
endfunction()
