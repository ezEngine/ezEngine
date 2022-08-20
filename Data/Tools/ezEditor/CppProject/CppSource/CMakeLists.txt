cmake_minimum_required(VERSION 3.21)

project (CppProject VERSION 1.0 LANGUAGES C CXX)

if(PROJECT_IS_TOP_LEVEL)

	set (EZ_SDK_DIR "" CACHE PATH "The ezEngine SDK root directory.")

	message(STATUS "EZ_SDK_DIR is set to '${EZ_SDK_DIR}'")

	# fix output directory
	set (EZ_OUTPUT_DIRECTORY_LIB "${EZ_SDK_DIR}/Output/Lib" CACHE PATH "Where to store the compiled .lib files." FORCE)
	set (EZ_OUTPUT_DIRECTORY_DLL "${EZ_SDK_DIR}/Output/Bin" CACHE PATH "Where to store the compiled .dll files." FORCE)

	include("${EZ_SDK_DIR}/ezCMakeConfig.cmake")
	get_property(EZ_CMAKE_RELPATH GLOBAL PROPERTY EZ_CMAKE_RELPATH)
	get_property(EZ_CMAKE_RELPATH_CODE GLOBAL PROPERTY EZ_CMAKE_RELPATH_CODE)

	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${EZ_SDK_DIR}/${EZ_CMAKE_RELPATH}")

	include("ezUtils")
	include("ezPlatformVars")

	ez_include_ezExport()

	ez_configure_external_project()

endif()

# include all projects in the sub-folders
ez_add_all_subdirs()
