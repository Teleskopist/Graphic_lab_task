include_guard()
include(litert)

add_library(tbb SHARED IMPORTED GLOBAL)

if(WIN32)
    set_target_properties(tbb PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/embree3/bin_win64/tbb12.dll
        IMPORTED_IMPLIB ${CMAKE_CURRENT_LIST_DIR}/embree3/lib_win64/tbb.lib)
else()
    # Prefer existing versioned libtbb sonames if present
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/embree3/lib/libtbb.so)
        set(_tbb_lib ${CMAKE_CURRENT_LIST_DIR}/embree3/lib/libtbb.so)
    elseif(EXISTS ${CMAKE_CURRENT_LIST_DIR}/embree3/lib/libtbb.so.12)
        set(_tbb_lib ${CMAKE_CURRENT_LIST_DIR}/embree3/lib/libtbb.so.12)
    elseif(EXISTS ${CMAKE_CURRENT_LIST_DIR}/embree3/lib/libtbb.so.12.2)
        set(_tbb_lib ${CMAKE_CURRENT_LIST_DIR}/embree3/lib/libtbb.so.12.2)
    else()
        set(_tbb_lib ${CMAKE_CURRENT_LIST_DIR}/embree3/lib/libtbb.so)
    endif()

    set_target_properties(tbb PROPERTIES IMPORTED_LOCATION ${_tbb_lib})
endif()
