#ez-version 4
cmake_minimum_required(VERSION 3.21)

# Set CMake policies
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "4.1")
  cmake_policy(SET CMP0194 OLD)
endif()

project (Asteroids VERSION 1.0 LANGUAGES C CXX)

if(PROJECT_IS_TOP_LEVEL)

	set (EZ_SDK_DIR "" CACHE PATH "The ezEngine SDK root directory.")

	message(STATUS "EZ_SDK_DIR is set to '${EZ_SDK_DIR}'")

	# Where the SDK's binaries are. The plugin has to be built into the same directories, or the editor
	# does not find it. The editor passes them in through CMakeUserPresets.json, because an SDK can be
	# built into a directory other than the default one below.
	if (NOT EZ_OUTPUT_DIRECTORY_LIB)
		set (EZ_OUTPUT_DIRECTORY_LIB "${EZ_SDK_DIR}/Output/Lib" CACHE PATH "Where to store the compiled .lib files." FORCE)
	endif()

	if (NOT EZ_OUTPUT_DIRECTORY_DLL)
		set (EZ_OUTPUT_DIRECTORY_DLL "${EZ_SDK_DIR}/Output/Bin" CACHE PATH "Where to store the compiled .dll files." FORCE)
	endif()

	include("${EZ_SDK_DIR}/ezCMakeConfig.cmake")
	get_property(EZ_CMAKE_RELPATH GLOBAL PROPERTY EZ_CMAKE_RELPATH)
	get_property(EZ_CMAKE_RELPATH_CODE GLOBAL PROPERTY EZ_CMAKE_RELPATH_CODE)

	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${EZ_SDK_DIR}/${EZ_CMAKE_RELPATH}")
	
	set(EZ_ROOT ${EZ_SDK_DIR})
	set_property(GLOBAL PROPERTY EZ_ROOT ${EZ_ROOT})

	include("ezUtils")
	include("ezPlatform")

	ez_include_ezExport()

	ez_configure_external_project()

endif()

# Include optional user extensions for custom build configuration
# This file can be used to add custom CMake logic without modifying this template
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CMakeExtensions.txt")
	include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeExtensions.txt")
endif()

add_subdirectory("AsteroidsPlugin")
add_subdirectory("AsteroidsGame")
