message(STATUS "Configuring Platform: UWP")

set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS ON)
set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_UWP ON)

macro(ez_platform_pull_properties)

	get_property(EZ_CMAKE_PLATFORM_UWP GLOBAL PROPERTY EZ_CMAKE_PLATFORM_UWP)

endmacro()

macro(ez_platform_detect_generator)
	string(FIND ${CMAKE_VERSION} "MSVC" VERSION_CONTAINS_MSVC)

	if(${VERSION_CONTAINS_MSVC} GREATER -1)
		message(STATUS "CMake was called from Visual Studio Open Folder workflow")
		set_property(GLOBAL PROPERTY EZ_CMAKE_INSIDE_VS ON)
	endif()

    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        # Visual Studio (All VS generators define MSVC)
        message(STATUS "Generator is MSVC (EZ_CMAKE_GENERATOR_MSVC)")

        set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_MSVC ON)
        set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "Vs")
        set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION $<CONFIGURATION>)
    else()
        message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on UWP! Please extend ez_platform_detect_generator()")
    endif()
endmacro()