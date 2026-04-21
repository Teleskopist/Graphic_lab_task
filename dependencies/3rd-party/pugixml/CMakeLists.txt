

#
# pugixml
#
add_library(pugixml STATIC)
target_sources(pugixml PRIVATE pugixml.cpp)
target_include_directories(pugixml PUBLIC .)
