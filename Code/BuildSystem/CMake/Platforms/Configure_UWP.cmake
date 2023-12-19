message(STATUS "Configuring Platform: UWP")

macro(ez_pull_platform_properties)

	get_property(EZ_CMAKE_PLATFORM_UWP GLOBAL PROPERTY EZ_CMAKE_PLATFORM_UWP)

endmacro()