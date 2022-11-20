cmake_minimum_required(VERSION 3.22)

project(sparks)

add_subdirectory(external/grassland)

message("${GRASSLAND_INCLUDE_DIRS}")
add_subdirectory(src/sparks)
