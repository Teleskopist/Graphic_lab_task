

#
# pugixml
#
add_library(pugixml STATIC)
target_sources(pugixml PRIVATE pugixml.cpp)
target_include_directories(pugixml PUBLIC .)

# Suppress uninitialized-variable false positives on newer GCC versions
# Also disable treating warnings as errors for this third-party target
target_compile_options(pugixml PRIVATE -Wno-uninitialized -Wno-maybe-uninitialized -Wno-error -Wno-error=uninitialized -Wno-error=maybe-uninitialized)
