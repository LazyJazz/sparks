cmake_minimum_required(VERSION 3.22)

project(sparks)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(assets)

set(ABSL_PROPAGATE_CXX_STD ON)

add_subdirectory(external/grassland)
add_subdirectory(external/abseil-cpp)
add_subdirectory(external/tinyobjloader)
add_subdirectory(external/tinyxml2)

file(GLOB ImGuizmo_Sources ${CMAKE_SOURCE_DIR}/external/ImGuizmo/*.cpp ${CMAKE_SOURCE_DIR}/external/ImGuizmo/*.h)
add_library(ImGuizmo ${ImGuizmo_Sources})
target_include_directories(ImGuizmo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/ImGuizmo ${GRASSLAND_INCLUDE_DIRS})
target_include_directories(ImGuizmo PRIVATE imgui)
add_library(tinyfiledialogs ${CMAKE_SOURCE_DIR}/external/tinyfiledialogs/tinyfiledialogs.c)

list(APPEND SPARKS_EXTERNAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/external/abseil-cpp)
list(APPEND SPARKS_EXTERNAL_INCLUDE_DIRS ${GRASSLAND_INCLUDE_DIRS})
list(APPEND SPARKS_EXTERNAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/external/tinyobjloader)
list(APPEND SPARKS_EXTERNAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/external/ImGuizmo)
list(APPEND SPARKS_EXTERNAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/external/tinyxml2)
list(APPEND SPARKS_EXTERNAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/external/tinyfiledialogs)

add_subdirectory(src)
