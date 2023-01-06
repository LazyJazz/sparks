file(GLOB mesh_files *.obj *.ply)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/meshes)
file(COPY ${mesh_files} DESTINATION ${CMAKE_BINARY_DIR}/meshes)
