cmake_minimum_required(VERSION 3.20)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "In-source builds are not supported. Please choose a different binary directory.")
	return()
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/ezCMakeConfig.cmake")
get_property(EZ_CMAKE_RELPATH GLOBAL PROPERTY EZ_CMAKE_RELPATH)
get_property(EZ_CMAKE_RELPATH_CODE GLOBAL PROPERTY EZ_CMAKE_RELPATH_CODE)

# Required to find cmake modules like 'FindDirectX.cmake'
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/${EZ_CMAKE_RELPATH}")

include("ezUtils")

file(RELATIVE_PATH EZ_SUBMODULE_PREFIX_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
set_property(GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH ${EZ_SUBMODULE_PREFIX_PATH})

if(EZ_SUBMODULE_PREFIX_PATH STREQUAL "")
	set(EZ_SUBMODULE_MODE FALSE)
else()
	set(EZ_SUBMODULE_MODE TRUE)

	if (NOT DEFINED EZ_SUBMODULE_ROOT_IDE_FOLDER)
		set(EZ_SUBMODULE_ROOT_IDE_FOLDER "ezEngine")
	endif()
endif()

set_property(GLOBAL PROPERTY EZ_SUBMODULE_MODE ${EZ_SUBMODULE_MODE})

# Set a global variable pointing to the root of the EZ source tree
set(EZ_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set_property(GLOBAL PROPERTY EZ_ROOT ${EZ_ROOT})

ez_build_filter_init()

ez_detect_project_name(DETECTED_NAME)

if(NOT EZ_SUBMODULE_MODE)
	ez_detect_languages()

	set(EZ_SOLUTION_NAME ${DETECTED_NAME} CACHE STRING "The name of the solution")
	project(${EZ_SOLUTION_NAME}
		VERSION 1.0
		DESCRIPTION "An open-source game engine"
		HOMEPAGE_URL "https://ezengine.net"
		LANGUAGES ${EZ_LANGUAGES})

	# Enable CTest support for unit testing
	enable_testing()
endif()

include("ezPlatform")

ez_set_build_types()
ez_check_build_type()
ez_write_configuration_txt()

if(EZ_CUSTOM_TOOLCHAIN_FILE)
	message(STATUS "Including EZ_CUSTOM_TOOLCHAIN_FILE: '${EZ_CUSTOM_TOOLCHAIN_FILE}'")
	include("${EZ_CUSTOM_TOOLCHAIN_FILE}")
endif()

ez_pull_all_vars()

set(EZ_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Code")

ez_find_qt()
ez_init_projects()

get_property(SELECTED_FILTER_NAME GLOBAL PROPERTY EZ_BUILD_FILTER_SELECTED)
if (NOT ${SELECTED_FILTER_NAME} MATCHES "Everything")
	set(CMAKE_SUPPRESS_REGENERATION true)
endif()
add_subdirectory(${EZ_CMAKE_RELPATH_CODE})
add_subdirectory(Data)

ez_add_external_projects_folder("_1")
ez_add_external_projects_folder("_2")
ez_add_external_projects_folder("_3")

if(NOT EZ_SUBMODULE_MODE)
	if(TARGET Editor)
		# sets the Editor project as the default startup project in Visual Studio
		set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT Editor)

		get_property(EXTERNAL_PROJECTS GLOBAL PROPERTY "EXTERNAL_PROJECTS")
		if(EXTERNAL_PROJECTS)
		  message(STATUS "External Projects (ezEditor): ${EXTERNAL_PROJECTS}")
		  add_dependencies(Editor ${EXTERNAL_PROJECTS})

		  message(STATUS "External Projects (ezPlayer): ${EXTERNAL_PROJECTS}")
		  add_dependencies(Player ${EXTERNAL_PROJECTS})
		  endif()		
	endif()
endif()

# finalizers may run into problems in split solutions
# however, they aren't needed there, since CI will always build all projects
# and finalizers are typically only needed to set up build dependencies
if (${SELECTED_FILTER_NAME} MATCHES "Everything")
ez_finalize_projects()
endif()

get_property(EXPORT_PROJECTS GLOBAL PROPERTY "EXPORT_PROJECTS")
ez_get_export_location(EZ_EXPORT_LOCATION)
export(TARGETS ${EXPORT_PROJECTS} FILE "${EZ_EXPORT_LOCATION}")

set(EXPORT_INFO "
set(EXPINP_OUTPUT_DIRECTORY_DLL ${EZ_OUTPUT_DIRECTORY_DLL})
set(EXPINP_OUTPUT_DIRECTORY_LIB ${EZ_OUTPUT_DIRECTORY_LIB})
set(EXPINP_BINARY_DIR ${CMAKE_BINARY_DIR})
set(EXPINP_SOURCE_DIR ${CMAKE_SOURCE_DIR})
")

file(WRITE "${EZ_OUTPUT_DIRECTORY_DLL}/ezExportInfo.cmake" ${EXPORT_INFO})
