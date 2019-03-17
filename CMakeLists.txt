#cmake

cmake_minimum_required(VERSION 3.13...3.14)

set(EZ_CMAKE_UTILS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Code/BuildSystem/CMake" CACHE INTERNAL "The path where ez stores CMake utility functions.")

# Required to find cmake modules like 'FindDirectX.cmake'
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${EZ_CMAKE_UTILS_PATH})

include("ezUtils")
ez_detect_project_name(DETECTED_NAME)

set (EZ_SOLUTION_NAME ${DETECTED_NAME} CACHE STRING "The name of the solution")

project (${EZ_SOLUTION_NAME}
	VERSION 1.0
	DESCRIPTION "An open-source game engine"
	HOMEPAGE_URL "http://ezEngine.net"
	LANGUAGES C CXX)

ez_check_build_type()
ez_write_configuration_txt()

add_subdirectory (Code)

if (TARGET Editor)

	# sets the Editor project as the default startup project in Visual Studio
	set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT Editor)
	
endif()