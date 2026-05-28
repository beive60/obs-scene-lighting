# CMake Windows build dependencies module

include_guard(GLOBAL)

include(buildspec_common)

function(_check_dependencies_windows)
  if(NOT CMAKE_VS_PLATFORM_NAME)
    message(FATAL_ERROR "Automatic OBS bootstrap requires a Visual Studio generator on Windows.")
  endif()

  set(arch ${CMAKE_VS_PLATFORM_NAME})
  set(platform windows-${arch})

  set(dependencies_dir "${CMAKE_CURRENT_SOURCE_DIR}/.deps")
  set(prebuilt_filename "windows-deps-VERSION-ARCH.zip")
  set(prebuilt_destination "obs-deps-VERSION-ARCH")
  set(qt6_filename "windows-deps-qt6-VERSION-ARCH.zip")
  set(qt6_destination "obs-deps-qt6-VERSION-ARCH")
  set(obs-studio_filename "OBS-Studio-VERSION-Sources.tar.gz")
  set(obs-studio_destination "obs-studio-VERSION")
  set(dependencies_list prebuilt obs-studio)

  if(DEFINED ENABLE_QT AND ENABLE_QT)
    list(INSERT dependencies_list 1 qt6)
  endif()

  _check_dependencies()
endfunction()

_check_dependencies_windows()