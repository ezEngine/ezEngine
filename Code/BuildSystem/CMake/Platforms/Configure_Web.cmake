include("${CMAKE_CURRENT_LIST_DIR}/Configure_Default.cmake")

message(STATUS "Configuring Platform: Web")

set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WEB ON)
set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_SUPPORTS_WEBGPU ON)

macro(ez_platform_pull_properties)

	get_property(EZ_CMAKE_PLATFORM_WEB GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WEB)

endmacro()

macro(ez_platform_detect_generator)
	if(CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES "Ninja Multi-Config")
		message(STATUS "Buildsystem is Ninja (EZ_CMAKE_GENERATOR_NINJA)")

		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_NINJA ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "Ninja")
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})

	else()
		message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Web! Please extend ez_platform_detect_generator()")
	endif()
endmacro()