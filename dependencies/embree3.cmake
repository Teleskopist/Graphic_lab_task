include_guard()
include(litert)
include(dependencies/tbb)

add_library(embree3 SHARED IMPORTED GLOBAL)

if(WIN32)
    set_target_properties(embree3 PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/embree3/bin_win64/embree3.dll
        IMPORTED_IMPLIB ${CMAKE_CURRENT_LIST_DIR}/embree3/lib_win64/embree3.lib)
else()
    set_target_properties(embree3 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/embree3/lib/libembree3.so)
endif()

target_include_directories(embree3 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/embree3)
target_link_libraries(embree3 INTERFACE tbb)
