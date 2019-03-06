#cmake

cmake_minimum_required(VERSION 3.13...3.14)

set(EZ_CMAKE_UTILS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Code/BuildSystem/CMake" CACHE INTERNAL "The path where ez stores CMake utility functions.")

set (EZ_SOLUTION_NAME "ezEngine" CACHE STRING "The name of the solution")

project (${EZ_SOLUTION_NAME} 
	VERSION 1.0 
	DESCRIPTION "An open-source game engine"
	HOMEPAGE_URL "http://ezEngine.net"
	LANGUAGES C CXX)

#Clear Targets.txt and Tests.txt
file(WRITE ${CMAKE_BINARY_DIR}/Targets.txt "")
file(WRITE ${CMAKE_BINARY_DIR}/Tests.txt "")

# Required to find cmake modules like 'FindDirectX.cmake'
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${EZ_CMAKE_UTILS_PATH})

include("ezUtils")
ez_detect_platform()

#Clear cached qt dirs
set (Qt5Core_DIR "" CACHE PATH "" FORCE)
set (Qt5Gui_DIR "" CACHE PATH "" FORCE)
set (Qt5Widgets_DIR "" CACHE PATH "" FORCE)
set (Qt5Network_DIR "" CACHE PATH "" FORCE)
set (Qt5WinExtras_DIR "" CACHE PATH "" FORCE)

add_subdirectory (Code)

# sets the Editor project as the default startup project in Visual Studio
set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT Editor)