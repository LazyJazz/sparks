file(GLOB_RECURSE source_files *.cpp *.h)

set(CURRENT_LIB_NAME sparks_${MODULE_NAME}_lib)

add_library(${CURRENT_LIB_NAME} ${source_files})

target_include_directories(${CURRENT_LIB_NAME} PRIVATE ${SPARKS_EXTERNAL_INCLUDE_DIRS} ${SPARKS_INCLUDE_DIR})
target_link_libraries(${CURRENT_LIB_NAME} PRIVATE sparks_util_lib sparks_assets_lib ImGuizmo tinyfiledialogs)

list(APPEND SPARKS_LIBRARIES ${CURRENT_LIB_NAME})
set(SPARKS_LIBRARIES ${SPARKS_LIBRARIES} PARENT_SCOPE)
