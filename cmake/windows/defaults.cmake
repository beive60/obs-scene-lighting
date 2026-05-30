# CMake Windows defaults module

include_guard(GLOBAL)

set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL TRUE)

if(OBS_BOOTSTRAP_DEPS AND NOT libobs_DIR)
  include(buildspec)
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(
    CMAKE_INSTALL_PREFIX
    "${CMAKE_CURRENT_BINARY_DIR}/install"
    CACHE PATH
    "Default plugin installation directory"
    FORCE
  )
endif()
