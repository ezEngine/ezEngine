message(STATUS "Configuring Platform: UWP")

set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS ON)
set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_UWP ON)

macro(ez_platform_pull_properties)

	get_property(EZ_CMAKE_PLATFORM_UWP GLOBAL PROPERTY EZ_CMAKE_PLATFORM_UWP)

endmacro()

macro(ez_platform_detect_generator)

endmacro()