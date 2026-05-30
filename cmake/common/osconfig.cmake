# OS-specific configuration module

include_guard(GLOBAL)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/linux")
endif()
