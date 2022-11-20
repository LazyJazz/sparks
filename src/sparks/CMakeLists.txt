
add_executable(sparks)

target_sources(sparks PRIVATE main.cpp)
target_include_directories(sparks PRIVATE ${GRASSLAND_INCLUDE_DIRS})
