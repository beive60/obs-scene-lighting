cmake_minimum_required(VERSION 3.24)

project(obs-scene-lighting VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_CLANG_TIDY "Enable clang-tidy checks" ON)

if(ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
  endif()
endif()

find_package(libobs REQUIRED)

add_library(obs-scene-lighting MODULE
  src/obs-scene-lighting.cpp
)

target_link_libraries(obs-scene-lighting PRIVATE OBS::libobs)

target_include_directories(obs-scene-lighting PRIVATE src)

set_target_properties(obs-scene-lighting PROPERTIES
  PREFIX ""
  OUTPUT_NAME "obs-scene-lighting"
)

install(TARGETS obs-scene-lighting LIBRARY DESTINATION obs-plugins/64bit)
install(FILES data/scene_lighting.effect DESTINATION data/obs-plugins/obs-scene-lighting)
install(DIRECTORY data/locale/ DESTINATION data/obs-plugins/obs-scene-lighting/locale)
