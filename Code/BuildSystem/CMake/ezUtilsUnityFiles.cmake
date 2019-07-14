######################################
### ez_write_pvs_header(<file-path> <optional-text>)
######################################

function(ez_write_pvs_header TARGET_FILE OPTIONAL_TEXT)

  if (EZ_ENABLE_PVS_STUDIO_HEADER_IN_UNITY_FILES)
    file (WRITE ${TARGET_FILE} "// This is an open source non-commercial project. Dear PVS-Studio, please check it.\n// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com \n\n${OPTIONAL_TEXT}")
  else()
    file (WRITE ${TARGET_FILE} "${OPTIONAL_TEXT}")
  endif()

endfunction()

######################################
### ez_generate_folder_unity_file(<target> <project-path> <rel-path-to-folder> <files-in-folder>)
######################################

function(ez_generate_folder_unity_file TARGET_NAME PROJECT_DIRECTORY SUB_FOLDER_PATH FILE_LIST)

  if (NOT ${EZ_ENABLE_FOLDER_UNITY_FILES})
    return()
  endif()

  set(FULL_FOLDER_PATH "${PROJECT_DIRECTORY}/${SUB_FOLDER_PATH}")

  set(FILE_LIST_CLEANED ${FILE_LIST})

  # Remove PCH files and non-source files
  foreach (CUR_FILE ${FILE_LIST})

    get_filename_component(CUR_FILE_NAME ${CUR_FILE} NAME_WE)
    get_filename_component(CUR_FILE_EXT ${CUR_FILE} EXT)

    if (CUR_FILE_NAME MATCHES "PCH$" OR NOT ${CUR_FILE_EXT} STREQUAL ".cpp")
      list(REMOVE_ITEM FILE_LIST_CLEANED ${CUR_FILE})
    endif ()

  endforeach()

  if (NOT FILE_LIST_CLEANED)
    return()
  endif()

  string(SHA1 CURRENT_FOLDER_HASH "${FULL_FOLDER_PATH}")

  file(RELATIVE_PATH CURRENT_FOLDER_RELATIVE ${CMAKE_SOURCE_DIR} ${FULL_FOLDER_PATH})
  set (FOLDER_UNITY_FILE "${CMAKE_BINARY_DIR}/${CURRENT_FOLDER_RELATIVE}/unity_${CURRENT_FOLDER_HASH}.cpp")
  set (TMP_UNITY_FILE "${CMAKE_BINARY_DIR}/temp/unity.cpp")
  #message (STATUS "Generating ${FOLDER_UNITY_FILE}")

  ez_write_pvs_header(${TMP_UNITY_FILE} "// generated unity cpp file\n\n")

  # find the PCH and include it as the first thing in the unity file
  ez_retrieve_target_pch(${TARGET_NAME} PCH_FILE)

  if (PCH_FILE)
    file(APPEND ${TMP_UNITY_FILE} "#include <${PCH_FILE}.h>\n\n")
	
	ez_pch_use("${PCH_FILE}.h" "${FOLDER_UNITY_FILE}")
  endif()

  # include all the CPPs in the unity file
  foreach (CUR_FILE ${FILE_LIST_CLEANED})
    set(CUR_FILE_FULL "${PROJECT_DIRECTORY}/${SUB_FOLDER_PATH}/${CUR_FILE}")

    file(TO_NATIVE_PATH ${CUR_FILE_FULL} CUR_FILE_NATIVE)
    file (APPEND ${TMP_UNITY_FILE} "#include \"${CUR_FILE_NATIVE}\"\n")

    set_source_files_properties(${CUR_FILE_FULL} PROPERTIES HEADER_FILE_ONLY true)
  endforeach ()

  set (UNITY_FILE_NEEDS_UPDATE true)

  if(EXISTS ${FOLDER_UNITY_FILE})
    # check if the generated unity file is different from the existing one
    file(SHA1 ${FOLDER_UNITY_FILE} UNITY_EXISTING_HASH)
    file(SHA1 ${TMP_UNITY_FILE} UNITY_NEW_HASH)

    if(${UNITY_EXISTING_HASH} STREQUAL ${UNITY_NEW_HASH})
      set (UNITY_FILE_NEEDS_UPDATE false)
    endif()
  endif()

  if(${UNITY_FILE_NEEDS_UPDATE})
    file (READ ${TMP_UNITY_FILE} UNITY_FILE_CONTENTS)
    file (WRITE ${FOLDER_UNITY_FILE} ${UNITY_FILE_CONTENTS})
  endif()

  # add the file to the target and sort it into the proper sub-tree
  target_sources(${TARGET_NAME} PRIVATE ${FOLDER_UNITY_FILE})

  string(REPLACE "/" "\\" SOURCE_GROUP_PATH "${SUB_FOLDER_PATH}")
  source_group("${SOURCE_GROUP_PATH}" FILES ${FOLDER_UNITY_FILE})

endfunction()

######################################
### ez_generate_folder_unity_files_for_target(<target> <project-path> [<folder-to-exclude> ...])
######################################

function(ez_generate_folder_unity_files_for_target TARGET_NAME TARGET_DIRECTORY)

  ez_gather_subfolders(${TARGET_DIRECTORY} ALL_FOLDERS)

  foreach(CUR_FOLDER ${ALL_FOLDERS})

    if (ARGN)

      if (${CUR_FOLDER} IN_LIST ARGN)
        continue()
      endif()

    endif()

    file(GLOB FILES_IN_FOLDER RELATIVE "${TARGET_DIRECTORY}/${CUR_FOLDER}" "${TARGET_DIRECTORY}/${CUR_FOLDER}/*.cpp")

    ez_generate_folder_unity_file(${TARGET_NAME} "${TARGET_DIRECTORY}" "${CUR_FOLDER}" "${FILES_IN_FOLDER}")

  endforeach()

endfunction()