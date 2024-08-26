# #####################################
# ## ez_include_ezExport()
# #####################################

macro(ez_include_ezExport)
	# Create a modified version of the ezExport.cmake file,
	# where the absolute paths to the original locations are replaced
	# with the absolute paths to this installation
	ez_get_export_location(EXP_FILE)
	set(IMP_FILE "${CMAKE_BINARY_DIR}/ezExport.cmake")
	set(EXPINFO_FILE "${EZ_OUTPUT_DIRECTORY_DLL}/ezExportInfo.cmake")

	# read the file that contains the original paths
	include(${EXPINFO_FILE})

	# read the ezExport file into a string
	file(READ ${EXP_FILE} IMP_CONTENT)

	# replace the original paths with our paths
	string(REPLACE ${EXPINP_OUTPUT_DIRECTORY_DLL} ${EZ_OUTPUT_DIRECTORY_DLL} IMP_CONTENT "${IMP_CONTENT}")
	string(REPLACE ${EXPINP_OUTPUT_DIRECTORY_LIB} ${EZ_OUTPUT_DIRECTORY_LIB} IMP_CONTENT "${IMP_CONTENT}")
	string(REPLACE ${EXPINP_SOURCE_DIR} ${EZ_SDK_DIR} IMP_CONTENT "${IMP_CONTENT}")

	# write the modified ezExport file to disk
	file(WRITE ${IMP_FILE} "${IMP_CONTENT}")

	# include the modified file, so that the CMake targets become known
	include(${IMP_FILE})
endmacro()

# #####################################
# ## ez_configure_external_project()
# #####################################
macro(ez_configure_external_project)

	if (EZ_SDK_DIR STREQUAL "")
		file(RELATIVE_PATH EZ_SUBMODULE_PREFIX_PATH ${CMAKE_SOURCE_DIR} ${EZ_SDK_DIR})
	else()
		set(EZ_SUBMODULE_PREFIX_PATH "")
	endif()
	
	set_property(GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH ${EZ_SUBMODULE_PREFIX_PATH})

	if(EZ_SUBMODULE_PREFIX_PATH STREQUAL "")
		set(EZ_SUBMODULE_MODE FALSE)
	else()
		set(EZ_SUBMODULE_MODE TRUE)
	endif()

	set_property(GLOBAL PROPERTY EZ_SUBMODULE_MODE ${EZ_SUBMODULE_MODE})

	ez_build_filter_init()

	ez_set_build_types()
endmacro()