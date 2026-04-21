include_guard()
include(litert)

add_library(tbb SHARED IMPORTED GLOBAL)

if(WIN32)
    set_target_properties(tbb PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/embree3/bin_win64/tbb12.dll
        IMPORTED_IMPLIB ${CMAKE_CURRENT_LIST_DIR}/embree3/lib_win64/tbb.lib)
else()
    set_target_properties(tbb PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/embree3/lib/libtbb.so)
endif()
