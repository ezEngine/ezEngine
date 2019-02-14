include("ezUtilsPCH")
include("ezUtilsUnityFiles")
include("ezUtilsQt")

######################################
### ez_grep_sources
######################################

function(ez_grep_sources START_FOLDER OUT_FILES)

  file(GLOB_RECURSE SOURCE_FILES_CPP RELATIVE "${START_FOLDER}" "${START_FOLDER}/*.cpp")
  file(GLOB_RECURSE SOURCE_FILES_H RELATIVE "${START_FOLDER}" "${START_FOLDER}/*.h")

  #message(STATUS "Globbed cpp: ${SOURCE_FILES_CPP}")
  #message(STATUS "Globbed h: ${SOURCE_FILES_H}")

  set(${OUT_FILES} "${SOURCE_FILES_CPP};${SOURCE_FILES_H}" PARENT_SCOPE)

endfunction()

######################################
### ez_add_project_files
######################################

function(ez_add_project_files TARGET_NAME ROOT_DIR SUB_FOLDER FILE_LIST)

  foreach(FILE IN ITEMS ${FILE_LIST})
    set(CUR_FILE "${SUB_FOLDER}/${FILE}")
    target_sources(${TARGET_NAME} PRIVATE "${CUR_FILE}")
    source_group(TREE ${ROOT_DIR} FILES ${CUR_FILE})
  endforeach()

endfunction()

