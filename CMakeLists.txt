cmake_minimum_required(VERSION 3.22)

project(sparks)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(external/grassland)

message("${GRASSLAND_INCLUDE_DIRS}")
add_subdirectory(src)
