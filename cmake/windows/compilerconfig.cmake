# CMake Windows compiler configuration module

include_guard(GLOBAL)

if(MSVC)
  add_compile_options(
    /W4
    /utf-8
    /Zc:__cplusplus
    /EHsc
    /permissive-
  )

  add_compile_definitions(
    UNICODE
    _UNICODE
    _CRT_SECURE_NO_WARNINGS
    NOMINMAX
  )
endif()
