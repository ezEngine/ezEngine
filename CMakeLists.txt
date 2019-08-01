cmake_minimum_required(VERSION 3.12...3.14)

set(EZ_CMAKE_UTILS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Code/BuildSystem/CMake" CACHE INTERNAL "The path where ez stores CMake utility functions.")

# Required to find cmake modules like 'FindDirectX.cmake'
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${EZ_CMAKE_UTILS_PATH})

include("ezUtils")

ez_vcpkg_show_info()
ez_vcpkg_init()

ez_detect_project_name(DETECTED_NAME)

set (EZ_SOLUTION_NAME ${DETECTED_NAME} CACHE STRING "The name of the solution")

set(EZ_LANGUAGES C CXX)
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(EZ_LANGUAGES C CXX CSharp)
endif()

project (${EZ_SOLUTION_NAME}
	VERSION 1.0
	DESCRIPTION "An open-source game engine"
	HOMEPAGE_URL "http://ezEngine.net"
	LANGUAGES ${EZ_LANGUAGES})

ez_check_build_type()
ez_write_configuration_txt()

if (EZ_VCPKG_INSTALL_QT)
	ez_vcpkg_install("qt5-base;qt5-winextras")
endif()

ez_init_projects()

ez_add_external_projects_folder("_1")
ez_add_external_projects_folder("_2")
ez_add_external_projects_folder("_3")

add_subdirectory(Code)

if (TARGET Editor)

	# sets the Editor project as the default startup project in Visual Studio
	set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT Editor)

endif()
