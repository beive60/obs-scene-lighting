# CMake Windows defaults module

include_guard(GLOBAL)

if(NOT CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  return()
endif()

set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation prefix" FORCE)
