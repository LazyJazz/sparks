cmake_minimum_required(VERSION 3.22)

project(sparks)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(assets)

set(ABSL_PROPAGATE_CXX_STD ON)

add_subdirectory(external/grassland)
add_subdirectory(external/abseil-cpp)

list(APPEND SPARKS_EXTERNAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/external/abseil-cpp)
list(APPEND SPARKS_EXTERNAL_INCLUDE_DIRS ${GRASSLAND_INCLUDE_DIRS})

message("${GRASSLAND_INCLUDE_DIRS}")
add_subdirectory(src)
