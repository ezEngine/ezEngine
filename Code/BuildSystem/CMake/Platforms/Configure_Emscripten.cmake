message(STATUS "Configuring Platform: Emscripten")

macro(ez_pull_platform_properties)

	get_property(EZ_CMAKE_PLATFORM_EMSCRIPTEN GLOBAL PROPERTY EZ_CMAKE_PLATFORM_EMSCRIPTEN)

endmacro()