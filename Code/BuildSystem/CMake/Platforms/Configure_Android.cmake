message(STATUS "Configuring Platform: Android")

macro(ez_pull_platform_properties)

	get_property(EZ_CMAKE_PLATFORM_ANDROID GLOBAL PROPERTY EZ_CMAKE_PLATFORM_ANDROID)

endmacro()