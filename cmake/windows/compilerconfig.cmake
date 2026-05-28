# CMake Windows compiler configuration module

include_guard(GLOBAL)

if(NOT MSVC)
  return()
endif()

set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT ProgramDatabase)

if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION AND CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION VERSION_LESS 10.0.20348)
  message(
    FATAL_ERROR
    "OBS requires Windows 10 SDK version 10.0.20348.0 or more recent.\n"
    "Please download and install the most recent Windows platform SDK."
  )
endif()

add_compile_options(/W3 /utf-8 /Brepro /permissive- /MP /Zc:__cplusplus /Zc:preprocessor)

if(CMAKE_CXX_STANDARD GREATER_EQUAL 20)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/Zc:char8_t->)
endif()

add_compile_definitions(UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)

add_link_options(
  $<$<NOT:$<CONFIG:Debug>>:/OPT:REF>
  $<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>
  $<$<NOT:$<CONFIG:Debug>>:/LTCG>
  $<$<NOT:$<CONFIG:Debug>>:/INCREMENTAL:NO>
  /DEBUG
  /Brepro
)

if(CMAKE_COMPILE_WARNING_AS_ERROR)
  add_link_options(/WX)
endif()