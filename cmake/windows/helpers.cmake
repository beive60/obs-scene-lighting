# CMake Windows helper functions module

include_guard(GLOBAL)

function(target_install_resources target)
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    return()
  endif()

  file(GLOB_RECURSE data_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/data/*")
  foreach(data_file IN LISTS data_files)
    if(NOT IS_DIRECTORY "${data_file}")
      target_sources(${target} PRIVATE "${data_file}")
    endif()
  endforeach()

  install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/"
    DESTINATION "data/obs-plugins/${target}"
    USE_SOURCE_PERMISSIONS
  )

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/rundir/$<CONFIG>/data/obs-plugins/${target}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/data"
            "${CMAKE_CURRENT_BINARY_DIR}/rundir/$<CONFIG>/data/obs-plugins/${target}"
    COMMENT "Copy ${target} resources to rundir"
    VERBATIM
  )
endfunction()

function(set_target_properties_plugin target)
  set(options "")
  set(oneValueArgs "")
  set(multiValueArgs PROPERTIES)
  cmake_parse_arguments(PARSE_ARGV 1 _STPO "${options}" "${oneValueArgs}" "${multiValueArgs}")

  while(_STPO_PROPERTIES)
    list(POP_FRONT _STPO_PROPERTIES key value)
    set_property(TARGET ${target} PROPERTY ${key} "${value}")
  endwhile()

  set_target_properties(
    ${target}
    PROPERTIES
      PREFIX ""
      VERSION ${PLUGIN_VERSION}
      SOVERSION ${PLUGIN_VERSION_MAJOR}
  )

  install(
    TARGETS ${target}
    RUNTIME DESTINATION "obs-plugins/64bit"
    LIBRARY DESTINATION "obs-plugins/64bit"
  )

  install(
    FILES "$<TARGET_PDB_FILE:${target}>"
    CONFIGURATIONS RelWithDebInfo Debug Release
    DESTINATION "obs-plugins/64bit"
    OPTIONAL
  )

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/rundir/$<CONFIG>/obs-plugins/64bit"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${target}>"
            "${CMAKE_CURRENT_BINARY_DIR}/rundir/$<CONFIG>/obs-plugins/64bit"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<$<CONFIG:Debug,RelWithDebInfo,Release>:$<TARGET_PDB_FILE:${target}>>"
            "${CMAKE_CURRENT_BINARY_DIR}/rundir/$<CONFIG>/obs-plugins/64bit"
    COMMAND_EXPAND_LISTS
    COMMENT "Copy ${target} to rundir"
    VERBATIM
  )

  target_install_resources(${target})
endfunction()
