
if(WIN32)
    add_library(glfw SHARED IMPORTED GLOBAL)
    set_target_properties(glfw PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/glfw/glfw3.dll
        IMPORTED_IMPLIB ${CMAKE_CURRENT_LIST_DIR}/glfw/glfw3.lib
    )
    target_include_directories(glfw INTERFACE ${CMAKE_CURRENT_LIST_DIR}/glfw/include)
else()
    find_package(glfw3 REQUIRED)
endif()
