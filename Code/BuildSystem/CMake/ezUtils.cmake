function(ez_grep_sources START_FOLDER OUT_FILES)

  message(STATUS "${START_FOLDER}")

  file(GLOB_RECURSE SOURCE_FILES_CPP RELATIVE "${START_FOLDER}" "${START_FOLDER}/*.cpp")
  file(GLOB_RECURSE SOURCE_FILES_H RELATIVE "${START_FOLDER}" "${START_FOLDER}/*.h")

  message(STATUS "Globbed cpp: ${SOURCE_FILES_CPP}")
  message(STATUS "Globbed h: ${SOURCE_FILES_H}")

  set(${OUT_FILES} "${SOURCE_FILES_CPP};${SOURCE_FILES_H}" PARENT_SCOPE)

endfunction()

function(ez_add_project_files TARGET ROOT_DIR SUB_FOLDER FILE_LIST)

  foreach(FILE IN ITEMS ${FILE_LIST})
    set(CUR_FILE "${SUB_FOLDER}/${FILE}")
    target_sources(${TARGET} PRIVATE "${CUR_FILE}")
    source_group(TREE ${ROOT_DIR} PREFIX "BlaFiles2" FILES ${CUR_FILE})
  endforeach()

endfunction()


function(ez_generate_folder_unity_file TARGET PROJECT_DIRECTORY SUB_FOLDER_PATH FILE_LIST)

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

  # generate unity.cpp
  if (NOT FILE_LIST_CLEANED)
    return()
  endif()

  string(SHA1 CURRENT_FOLDER_HASH "${FULL_FOLDER_PATH}")

  file(RELATIVE_PATH CURRENT_FOLDER_RELATIVE ${CMAKE_SOURCE_DIR} ${FULL_FOLDER_PATH})
  set (FOLDER_UNITY_FILE "${CMAKE_BINARY_DIR}/${CURRENT_FOLDER_RELATIVE}/unity_${CURRENT_FOLDER_HASH}.cpp")
  set (TMP_UNITY_FILE "${CMAKE_BINARY_DIR}/temp/unity.cpp")
  #message (STATUS "Generating ${FOLDER_UNITY_FILE}")

  # add the necessary comment lines to allow easy PVS static analysis
  # TODO move into function
  if (EZ_ENABLE_PVS_STUDIO_HEADER_IN_UNITY_FILES)
    file (WRITE ${TMP_UNITY_FILE} "// This is an open source non-commercial project. Dear PVS-Studio, please check it.\n// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com \n// generated unity cpp file\n")
  else()
    file (WRITE ${TMP_UNITY_FILE} "// generated unity cpp file\n")
  endif ()

  # TODO: move into function
  # find the PCH and include it as the first thing in the unity file
  if (EZ_USE_PCH)
    set (PCH_FOUND false)
    set (PCH_H "")
    set (PCH_CPP "")

    foreach (CUR_FILE ${FILE_LIST})

      get_filename_component(CUR_FILE_NAME ${CUR_FILE} NAME_WE)
      get_filename_component(CUR_FILE_EXT ${CUR_FILE} EXT)

      if (${CUR_FILE_EXT} STREQUAL ".h" AND CUR_FILE_NAME MATCHES "PCH$")
        set (PCH_FOUND true)
        set (PCH_H "${CUR_FILE_NAME}.h")
        set (PCH_CPP "${CUR_FILE_NAME}.cpp")
      endif ()

    endforeach ()

    if (PCH_FOUND)
      file(APPEND ${TMP_UNITY_FILE} "#include <${PCH_H}>\n\n")
    endif ()
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
  target_sources(${TARGET} PRIVATE ${FOLDER_UNITY_FILE})
  source_group(${SUB_FOLDER_PATH} FILES ${FOLDER_UNITY_FILE})

endfunction()
