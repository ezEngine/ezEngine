include("ezUtilsPCH")
include("ezUtilsUnityFiles")
include("ezUtilsQt")
include("ezUtilsDetect")
include("ezUtilsDX11")

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

######################################
### ez_set_target_output_dirs
######################################

function(ez_set_target_output_dirs TARGET_NAME LIB_OUTPUT_DR DLL_OUTPUT_DR)

	ez_detect_platform()
	get_property(PLATFORM_PREFIX GLOBAL PROPERTY EZ_CMAKE_PLATFORM_PREFIX)

	ez_detect_generator()
	get_property(BUILDSYSTEM_PREFIX GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX)

	ez_detect_compiler()
	get_property(COMPILER_POSTFIX GLOBAL PROPERTY EZ_CMAKE_COMPILER_POSTFIX)

	ez_detect_architecture()
	get_property(ARCHITECTURE_POSTFIX GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_POSTFIX)	

	set (OUTPUT_LIB_DEBUG       "${LIB_OUTPUT_DR}/${PLATFORM_PREFIX}${BUILDSYSTEM_PREFIX}${COMPILER_POSTFIX}Debug${ARCHITECTURE_POSTFIX}")
	set (OUTPUT_LIB_RELEASE     "${LIB_OUTPUT_DR}/${PLATFORM_PREFIX}${BUILDSYSTEM_PREFIX}${COMPILER_POSTFIX}Release${ARCHITECTURE_POSTFIX}")
	set (OUTPUT_LIB_MINSIZE     "${LIB_OUTPUT_DR}/${PLATFORM_PREFIX}${BUILDSYSTEM_PREFIX}${COMPILER_POSTFIX}MinSize${ARCHITECTURE_POSTFIX}")
	set (OUTPUT_LIB_RELWITHDEB  "${LIB_OUTPUT_DR}/${PLATFORM_PREFIX}${BUILDSYSTEM_PREFIX}${COMPILER_POSTFIX}RelDeb${ARCHITECTURE_POSTFIX}")

	set (OUTPUT_DLL_DEBUG       "${DLL_OUTPUT_DR}/${PLATFORM_PREFIX}${BUILDSYSTEM_PREFIX}${COMPILER_POSTFIX}Debug${ARCHITECTURE_POSTFIX}")
	set (OUTPUT_DLL_RELEASE     "${DLL_OUTPUT_DR}/${PLATFORM_PREFIX}${BUILDSYSTEM_PREFIX}${COMPILER_POSTFIX}Release${ARCHITECTURE_POSTFIX}")
	set (OUTPUT_DLL_MINSIZE     "${DLL_OUTPUT_DR}/${PLATFORM_PREFIX}${BUILDSYSTEM_PREFIX}${COMPILER_POSTFIX}MinSize${ARCHITECTURE_POSTFIX}")
	set (OUTPUT_DLL_RELWITHDEB  "${DLL_OUTPUT_DR}/${PLATFORM_PREFIX}${BUILDSYSTEM_PREFIX}${COMPILER_POSTFIX}RelDeb${ARCHITECTURE_POSTFIX}")

	set_target_properties(${TARGET_NAME} PROPERTIES
	  RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DLL_DEBUG}"
    LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_LIB_DEBUG}"
    ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_LIB_DEBUG}"
	)
	
	set_target_properties(${TARGET_NAME} PROPERTIES
	  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DLL_DEBUG}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_LIB_DEBUG}"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_LIB_DEBUG}"
  )	

	set_target_properties(${TARGET_NAME} PROPERTIES
	  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DLL_RELEASE}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_LIB_RELEASE}"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_LIB_RELEASE}"
  )	

	set_target_properties(${TARGET_NAME} PROPERTIES
	  RUNTIME_OUTPUT_DIRECTORYMINSIZEREL "${OUTPUT_DLL_MINSIZE}"
    LIBRARY_OUTPUT_DIRECTORYMINSIZEREL "${OUTPUT_LIB_MINSIZE}"
    ARCHIVE_OUTPUT_DIRECTORYMINSIZEREL "${OUTPUT_LIB_MINSIZE}"
	)
		
	set_target_properties(${TARGET_NAME} PROPERTIES
	  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUTPUT_DLL_RELWITHDEB}"
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUTPUT_LIB_RELWITHDEB}"
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUTPUT_LIB_RELWITHDEB}"
	)

endfunction()

######################################
### ez_set_default_target_output_dirs
######################################

function(ez_set_default_target_output_dirs TARGET_NAME)

	set (EZ_OUTPUT_DIRECTORY_LIB "${CMAKE_SOURCE_DIR}/Output/Lib" CACHE PATH "Where to store the compiled .lib files.")
	set (EZ_OUTPUT_DIRECTORY_DLL "${CMAKE_SOURCE_DIR}/Output/Bin" CACHE PATH "Where to store the compiled .dll files.")

	mark_as_advanced(FORCE EZ_OUTPUT_DIRECTORY_LIB)
	mark_as_advanced(FORCE EZ_OUTPUT_DIRECTORY_DLL)

	ez_set_target_output_dirs("${TARGET_NAME}" "${EZ_OUTPUT_DIRECTORY_LIB}" "${EZ_OUTPUT_DIRECTORY_DLL}")

endfunction()


######################################
### ez_write_configuration_txt
######################################

function(ez_write_configuration_txt)

	# Clear Targets.txt and Tests.txt
	file(WRITE ${CMAKE_BINARY_DIR}/Targets.txt "")
	file(WRITE ${CMAKE_BINARY_DIR}/Tests.txt "")

	ez_detect_platform()
	get_property(PLATFORM_PREFIX GLOBAL PROPERTY EZ_CMAKE_PLATFORM_PREFIX)
	
	ez_detect_generator()
	get_property(BUILDSYSTEM_PREFIX GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX)

	ez_detect_compiler()
	get_property(COMPILER_POSTFIX GLOBAL PROPERTY EZ_CMAKE_COMPILER_POSTFIX)
	
	ez_detect_architecture()
	get_property(ARCHITECTURE_POSTFIX GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_POSTFIX)

	# Write configuration to file, as this is done at configure time we must pin the configuration in place (RelDeb is used because all build machines use this).
	file(WRITE ${CMAKE_BINARY_DIR}/Configuration.txt "")
	set(CONFIGURATION_DESC "${PLATFORM_PREFIX}${BUILDSYSTEM_PREFIX}${COMPILER_POSTFIX}RelDeb${ARCHITECTURE_POSTFIX}")
	file(APPEND ${CMAKE_BINARY_DIR}/Configuration.txt ${CONFIGURATION_DESC})

endfunction()


######################################
### ez_set_common_target_include_dirs
######################################

function(ez_set_common_target_include_dirs TARGET_NAME TARGET_FOLDER)

	get_filename_component(PARENT_DIR ${TARGET_FOLDER} DIRECTORY)

	target_include_directories(${TARGET_NAME} PRIVATE "${TARGET_FOLDER}")
	target_include_directories(${TARGET_NAME} PUBLIC "${PARENT_DIR}")

	#message(STATUS "Private include dir: '${TARGET_FOLDER}'")
	#message(STATUS "Public include dir: '${PARENT_DIR}'")

endfunction()

######################################
### ez_set_common_target_definitions
######################################

function(ez_set_common_target_definitions TARGET_NAME)

	# set the BUILDSYSTEM_COMPILE_ENGINE_AS_DLL definition
	if (BUILDSYSTEM_PLATFORM_WINDOWS)
	  set (EZ_COMPILE_ENGINE_AS_DLL ON CACHE BOOL "Whether to compile the code as a shared libraries (DLL).")
		mark_as_advanced(FORCE EZ_COMPILE_ENGINE_AS_DLL)
		
		target_compile_definitions(${TARGET_NAME} PUBLIC BUILDSYSTEM_COMPILE_ENGINE_AS_DLL)

	else()
	  unset(EZ_COMPILE_ENGINE_AS_DLL CACHE)
	endif()

	# set the BUILDSYSTEM_CONFIGURATION definition
	ez_detect_generator()
	get_property(BUILDSYSTEM_CONFIGURATION GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION)
	target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_CONFIGURATION="${BUILDSYSTEM_CONFIGURATION}")

	# set the BUILDSYSTEM_BUILDING_XYZ_LIB definition
	string(TOUPPER ${TARGET_NAME} PROJECT_NAME_UPPER)
	target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_BUILDING_${PROJECT_NAME_UPPER}_LIB)

endfunction()