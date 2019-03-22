
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
### ez_pch_use
######################################

function(ez_pch_use PCH_H TARGET_CPPS)

  if (NOT MSVC)
    return()
  endif()

  if (NOT EZ_USE_PCH)
    return()
  endif()  

  # only include .cpp files
  list(FILTER TARGET_CPPS INCLUDE REGEX "\.cpp$")

  # exclude files named 'qrc_*'
  list(FILTER TARGET_CPPS EXCLUDE REGEX ".*/qrc_.*")

  # exclude files named 'PCH.cpp'
  list(FILTER TARGET_CPPS EXCLUDE REGEX ".*PCH.cpp$")

  foreach(CPP_FILE ${TARGET_CPPS})

    set_source_files_properties (${CPP_FILE} PROPERTIES COMPILE_FLAGS "/Yu${PCH_H}")

  endforeach()

endfunction()

######################################
### ez_pch_create
######################################

function(ez_pch_create PCH_H TARGET_CPP)

  if (NOT MSVC)
    return()
  endif()

  if (NOT EZ_USE_PCH)
    return()
  endif()

  set_source_files_properties(${TARGET_CPP} PROPERTIES COMPILE_FLAGS "/Yc${PCH_H}")

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

    if ((${CUR_FILE_EXT} STREQUAL ".cpp") AND (${CUR_FILE_NAME} MATCHES "PCH$"))
      set (${PCH_NAME} ${CUR_FILE_NAME} PARENT_SCOPE)
      return()
    endif ()

  endforeach ()

endfunction()

######################################
### ez_auto_pch
######################################

function(ez_auto_pch TARGET_NAME FILES)

  ez_find_pch_in_file_list("${FILES}" PCH_NAME)

  if (NOT PCH_NAME)
    return()
  endif()

  ez_pch_create("${PCH_NAME}.h" "${PCH_NAME}.cpp")

  ez_pch_use("${PCH_NAME}.h" "${FILES}")

  ez_set_target_pch(${TARGET_NAME} ${PCH_NAME})
  
endfunction()
