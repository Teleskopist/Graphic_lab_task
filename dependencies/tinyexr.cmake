
include_guard()
include(litert)
include(dependencies/miniz)

set(MESSAGE_LOG_LEVEL ${CMAKE_MESSAGE_LOG_LEVEL})
set(CMAKE_MESSAGE_LOG_LEVEL ERROR)
litert_import(3rd-party/tinyexr EXCLUDE_FROM_ALL SYSTEM)
set(CMAKE_MESSAGE_LOG_LEVEL ${MESSAGE_LOG_LEVEL})
