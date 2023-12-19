message(STATUS "Configuring Platform: OSX")

macro(ez_pull_platform_properties)

	get_property(EZ_CMAKE_PLATFORM_OSX GLOBAL PROPERTY EZ_CMAKE_PLATFORM_OSX)

endmacro()