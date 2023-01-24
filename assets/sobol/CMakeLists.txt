file(GLOB sobol_files *.21201)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/sobol)
file(COPY ${sobol_files} DESTINATION ${CMAKE_BINARY_DIR}/sobol)
