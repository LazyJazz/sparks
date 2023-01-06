file(GLOB scene_files *.xml)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/scenes)
file(COPY ${scene_files} DESTINATION ${CMAKE_BINARY_DIR}/scenes)
