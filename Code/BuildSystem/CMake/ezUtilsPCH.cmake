
######################################
### ez_set_target_pch
######################################

function(ez_set_target_pch TARGET_NAME PCH_NAME)

  if (NOT EZ_USE_PCH)
    return()
  endif()

  #message(STATUS "Setting PCH for '${TARGET_NAME}': ${PCH_NAME}")
  set_property(TARGET ${TARGET_NAME} PROPERTY "PCH_FILE_NAME" ${PCH_NAME})

endfunction()

######################################
### ez_retrieve_target_pch
######################################

function(ez_retrieve_target_pch TARGET_NAME PCH_NAME)

  if (NOT EZ_USE_PCH)
    return()
  endif()

  get_property(RESULT TARGET ${TARGET_NAME} PROPERTY "PCH_FILE_NAME")

  set (${PCH_NAME} ${RESULT} PARENT_SCOPE)
  #message(STATUS "Retrieved PCH for '${TARGET_NAME}': ${RESULT}")

endfunction()

######################################
### ez_find_pch_in_file_list
######################################

function(ez_find_pch_in_file_list FILE_LIST PCH_NAME)

  if (NOT EZ_USE_PCH)
    return()
  endif()

  foreach (CUR_FILE ${FILE_LIST})

    get_filename_component(CUR_FILE_NAME ${CUR_FILE} NAME_WE)
    get_filename_component(CUR_FILE_EXT ${CUR_FILE} EXT)

    if ((${CUR_FILE_EXT} STREQUAL ".cpp") AND (CUR_FILE_NAME MATCHES "PCH$"))
      set (${PCH_NAME} ${CUR_FILE_NAME} PARENT_SCOPE)
      return()
    endif ()

  endforeach ()

endfunction()

######################################
### ez_find_pch_in_folder
######################################

function(ez_find_pch_in_folder PROJECT_DIRECTORY PCH_NAME)

  file(GLOB SOURCE_FILES_CPP "${PROJECT_DIRECTORY}/*.cpp")

  ez_find_pch_in_file_list("${SOURCE_FILES_CPP}" PCH_FROM_FILE_LIST)

  if (PCH_FROM_FILE_LIST)
    set(${PCH_NAME} ${PCH_FROM_FILE_LIST} PARENT_SCOPE)
  endif()

endfunction()

######################################
### ez_auto_detect_pch_for_target
######################################

function(ez_auto_detect_pch_for_target TARGET_NAME PROJECT_DIRECTORY)

  ez_find_pch_in_folder(${PROJECT_DIRECTORY} PCH_NAME)

  if (PCH_NAME)
    ez_set_target_pch(${TARGET_NAME} ${PCH_NAME})
  endif()

endfunction()

######################################
### ez_pch_consume
######################################

function(ez_pch_consume TARGET_FILE)

  # set_source_files_properties (${src_file} PROPERTIES COMPILE_FLAGS "/Yu${PCH_H}")

endfunction()


