
include_guard()
include(litert)
include(dependencies/nlohmann_json)

set(MESSAGE_LOG_LEVEL ${CMAKE_MESSAGE_LOG_LEVEL})
set(CMAKE_MESSAGE_LOG_LEVEL ERROR)
litert_import(3rd-party/tiny_gltf EXCLUDE_FROM_ALL SYSTEM)
set(CMAKE_MESSAGE_LOG_LEVEL ${MESSAGE_LOG_LEVEL})
