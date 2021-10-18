cmake_minimum_required(VERSION 3.19)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "In-source builds are not supported. Please choose a different binary directory.")
	return()
endif()

set(EZ_CMAKE_UTILS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Code/BuildSystem/CMake" CACHE INTERNAL "The path where ez stores CMake utility functions.")

# Required to find cmake modules like 'FindDirectX.cmake'
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${EZ_CMAKE_UTILS_PATH})

include("ezUtils")

file(RELATIVE_PATH EZ_SUBMODULE_PREFIX_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
set_property(GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH ${EZ_SUBMODULE_PREFIX_PATH})
if(EZ_SUBMODULE_PREFIX_PATH STREQUAL "")
	set(EZ_SUBMODULE_MODE FALSE)
else()
	set(EZ_SUBMODULE_MODE TRUE)
endif()
set_property(GLOBAL PROPERTY EZ_SUBMODULE_MODE ${EZ_SUBMODULE_MODE})

ez_build_filter_init()

ez_detect_project_name(DETECTED_NAME)

if(NOT EZ_SUBMODULE_MODE)
	ez_detect_languages()

	set (EZ_SOLUTION_NAME ${DETECTED_NAME} CACHE STRING "The name of the solution")
	project (${EZ_SOLUTION_NAME}
		VERSION 1.0
		DESCRIPTION "An open-source game engine"
		HOMEPAGE_URL "https://ezengine.net"
		LANGUAGES ${EZ_LANGUAGES})
endif()

ez_set_build_types()

ez_write_configuration_txt()

ez_init_projects()

ez_add_external_projects_folder("_1")
ez_add_external_projects_folder("_2")
ez_add_external_projects_folder("_3")

add_subdirectory(Code)

if(NOT EZ_SUBMODULE_MODE)
	if (TARGET Editor)
		# sets the Editor project as the default startup project in Visual Studio
		set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT Editor)
	endif()
endif()

ez_finalize_projects()
