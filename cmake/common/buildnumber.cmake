# Build number generation module

include_guard(GLOBAL)

if(NOT DEFINED PLUGIN_VERSION_BUILD)
  set(PLUGIN_VERSION_BUILD 0)
endif()
