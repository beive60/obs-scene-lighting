# Common build dependencies module

include_guard(GLOBAL)

function(_check_deps_version version)
  set(found FALSE)

  foreach(path IN LISTS CMAKE_PREFIX_PATH)
    if(EXISTS "${path}/share/obs-deps/VERSION")
      if(dependency STREQUAL qt6 AND NOT EXISTS "${path}/lib/cmake/Qt6/Qt6Config.cmake")
        set(found FALSE)
        continue()
      endif()

      file(READ "${path}/share/obs-deps/VERSION" _check_version)
      string(REPLACE "\n" "" _check_version "${_check_version}")
      string(REPLACE "-" "." _check_version "${_check_version}")
      string(REPLACE "-" "." version "${version}")

      if(_check_version VERSION_EQUAL version)
        set(found TRUE)
        break()
      elseif(_check_version VERSION_LESS version)
        message(
          AUTHOR_WARNING
          "Older ${label} version detected in ${path}:\n"
          "Found ${_check_version}, require ${version}"
        )
        list(REMOVE_ITEM CMAKE_PREFIX_PATH "${path}")
        list(APPEND CMAKE_PREFIX_PATH "${path}")
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
      else()
        message(
          AUTHOR_WARNING
          "Newer ${label} version detected in ${path}:\n"
          "Found ${_check_version}, require ${version}"
        )
        set(found TRUE)
        break()
      endif()
    endif()
  endforeach()

  return(PROPAGATE found CMAKE_PREFIX_PATH)
endfunction()

function(_download_dependency url destination hash)
  set(_needs_download TRUE)
  if(EXISTS "${destination}")
    file(SHA256 "${destination}" _existing_hash)
    if(_existing_hash STREQUAL hash)
      set(_needs_download FALSE)
    else()
      file(REMOVE "${destination}")
    endif()
  endif()

  if(_needs_download)
    message(STATUS "Downloading ${url}")
    file(DOWNLOAD "${url}" "${destination}" STATUS download_status EXPECTED_HASH SHA256=${hash})

    list(GET download_status 0 error_code)
    list(GET download_status 1 error_message)
    if(error_code GREATER 0)
      file(REMOVE "${destination}")
      message(FATAL_ERROR "Unable to download ${url}, failed with error: ${error_message}")
    endif()
    message(STATUS "Downloading ${url} - done")
  endif()
endfunction()

function(_extract_obs_studio archive_path destination)
  file(GLOB _before LIST_DIRECTORIES TRUE RELATIVE "${dependencies_dir}" "${dependencies_dir}/*")
  file(ARCHIVE_EXTRACT INPUT "${archive_path}" DESTINATION "${dependencies_dir}")

  if(EXISTS "${dependencies_dir}/${destination}")
    return()
  endif()

  file(GLOB _after LIST_DIRECTORIES TRUE RELATIVE "${dependencies_dir}" "${dependencies_dir}/*")
  set(_candidates ${_after})
  foreach(existing IN LISTS _before)
    list(REMOVE_ITEM _candidates "${existing}")
  endforeach()

  foreach(candidate IN LISTS _candidates)
    if(IS_DIRECTORY "${dependencies_dir}/${candidate}" AND EXISTS "${dependencies_dir}/${candidate}/CMakeLists.txt")
      file(RENAME "${dependencies_dir}/${candidate}" "${dependencies_dir}/${destination}")
      break()
    endif()
  endforeach()

  if(NOT EXISTS "${dependencies_dir}/${destination}")
    message(FATAL_ERROR "Unable to locate extracted OBS sources after unpacking ${archive_path}.")
  endif()
endfunction()

function(_setup_obs_studio)
  if(NOT libobs_DIR)
    set(_is_fresh --fresh)
  endif()

  if(OS_WINDOWS)
    set(_cmake_generator "${CMAKE_GENERATOR}")
    set(_cmake_arch_args -A "${arch},version=${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
    set(_cmake_extra_args "-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}")
  endif()

  message(STATUS "Configure ${label} (${arch})")
  execute_process(
    COMMAND
      "${CMAKE_COMMAND}" -S "${dependencies_dir}/${_obs_destination}" -B
      "${dependencies_dir}/${_obs_destination}/build_${arch}" -G "${_cmake_generator}" ${_cmake_arch_args}
      -DOBS_CMAKE_VERSION:STRING=3.0.0 -DENABLE_UI:BOOL=OFF -DENABLE_FRONTEND:BOOL=OFF
      -DENABLE_SCRIPTING:BOOL=OFF -DENABLE_PLUGINS:BOOL=OFF "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}" ${_is_fresh}
      ${_cmake_extra_args}
    RESULT_VARIABLE _process_result
    COMMAND_ERROR_IS_FATAL ANY
    OUTPUT_QUIET
  )
  message(STATUS "Configure ${label} (${arch}) - done")

  message(STATUS "Build ${label} (Debug - ${arch})")
  execute_process(
    COMMAND
      "${CMAKE_COMMAND}" --build "${dependencies_dir}/${_obs_destination}/build_${arch}" --target libobs
      --target obs-frontend-api --config Debug --parallel
    RESULT_VARIABLE _process_result
    COMMAND_ERROR_IS_FATAL ANY
    OUTPUT_QUIET
  )
  message(STATUS "Build ${label} (Debug - ${arch}) - done")

  message(STATUS "Build ${label} (Release - ${arch})")
  execute_process(
    COMMAND
      "${CMAKE_COMMAND}" --build "${dependencies_dir}/${_obs_destination}/build_${arch}" --target libobs
      --target obs-frontend-api --config Release --parallel
    RESULT_VARIABLE _process_result
    COMMAND_ERROR_IS_FATAL ANY
    OUTPUT_QUIET
  )
  message(STATUS "Build ${label} (Release - ${arch}) - done")

  message(STATUS "Install ${label} (${arch})")
  execute_process(
    COMMAND
      "${CMAKE_COMMAND}" --install "${dependencies_dir}/${_obs_destination}/build_${arch}" --component Development
      --config Debug --prefix "${dependencies_dir}"
    RESULT_VARIABLE _process_result
    COMMAND_ERROR_IS_FATAL ANY
    OUTPUT_QUIET
  )
  execute_process(
    COMMAND
      "${CMAKE_COMMAND}" --install "${dependencies_dir}/${_obs_destination}/build_${arch}" --component Development
      --config Release --prefix "${dependencies_dir}"
    RESULT_VARIABLE _process_result
    COMMAND_ERROR_IS_FATAL ANY
    OUTPUT_QUIET
  )
  message(STATUS "Install ${label} (${arch}) - done")
endfunction()

function(_check_dependencies)
  file(MAKE_DIRECTORY "${dependencies_dir}")

  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/buildspec.json" buildspec)
  string(JSON dependency_data GET ${buildspec} dependencies)

  foreach(dependency IN LISTS dependencies_list)
    string(JSON data GET ${dependency_data} ${dependency})
    string(JSON version GET ${data} version)
    string(JSON hash GET ${data} hashes ${platform})
    string(JSON url GET ${data} baseUrl)
    string(JSON label GET ${data} label)

    set(file "${${dependency}_filename}")
    set(destination "${${dependency}_destination}")
    string(REPLACE "VERSION" "${version}" file "${file}")
    string(REPLACE "VERSION" "${version}" destination "${destination}")
    string(REPLACE "ARCH" "${arch}" file "${file}")
    string(REPLACE "ARCH" "${arch}" destination "${destination}")

    set(skip FALSE)
    if(dependency STREQUAL prebuilt OR dependency STREQUAL qt6)
      _check_deps_version(${version})
      if(found)
        set(skip TRUE)
      endif()
    elseif(dependency STREQUAL obs-studio AND EXISTS "${dependencies_dir}/lib/cmake/libobs/libobsConfig.cmake")
      set(skip TRUE)
      list(APPEND CMAKE_PREFIX_PATH "${dependencies_dir}")
    endif()

    if(skip)
      message(STATUS "Setting up ${label} (${arch}) - skipped")
      continue()
    endif()

    if(dependency STREQUAL obs-studio)
      set(url "${url}/${version}/${file}")
    else()
      set(url "${url}/${version}/${file}")
    endif()

    _download_dependency("${url}" "${dependencies_dir}/${file}" "${hash}")

    if(EXISTS "${dependencies_dir}/${destination}")
      file(REMOVE_RECURSE "${dependencies_dir}/${destination}")
    endif()

    if(dependency STREQUAL obs-studio)
      _extract_obs_studio("${dependencies_dir}/${file}" "${destination}")
      set(_obs_version ${version})
      set(_obs_destination "${destination}")
      list(APPEND CMAKE_PREFIX_PATH "${dependencies_dir}")
    else()
      file(MAKE_DIRECTORY "${dependencies_dir}/${destination}")
      file(ARCHIVE_EXTRACT INPUT "${dependencies_dir}/${file}" DESTINATION "${dependencies_dir}/${destination}")
      list(APPEND CMAKE_PREFIX_PATH "${dependencies_dir}/${destination}")
    endif()

    message(STATUS "Setting up ${label} (${arch}) - done")
  endforeach()

  list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} CACHE PATH "CMake prefix search path" FORCE)

  if(NOT EXISTS "${dependencies_dir}/lib/cmake/libobs/libobsConfig.cmake")
    _setup_obs_studio()
  endif()
endfunction()