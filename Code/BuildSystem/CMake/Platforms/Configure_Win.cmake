include("${CMAKE_CURRENT_LIST_DIR}/Configure_Default.cmake")

message(STATUS "Configuring Platform: Windows")

set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS ON)
set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP ON)
set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_SUPPORTS_VULKAN ON)
set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_SUPPORTS_EDITOR ON)

if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
    set(EZ_CMAKE_WINDOWS_SDK_VERSION ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
else()
    set(EZ_CMAKE_WINDOWS_SDK_VERSION ${CMAKE_SYSTEM_VERSION})
    string(REGEX MATCHALL "\\." NUMBER_OF_DOTS "${EZ_CMAKE_WINDOWS_SDK_VERSION}")
    list(LENGTH NUMBER_OF_DOTS NUMBER_OF_DOTS)

    if(NUMBER_OF_DOTS EQUAL 2)
        set(EZ_CMAKE_WINDOWS_SDK_VERSION "${EZ_CMAKE_WINDOWS_SDK_VERSION}.0")
    endif()
endif()

set_property(GLOBAL PROPERTY EZ_CMAKE_WINDOWS_SDK_VERSION ${EZ_CMAKE_WINDOWS_SDK_VERSION})

# #####################################
# ## General settings
# #####################################
set(EZ_COMPILE_ENGINE_AS_DLL ON CACHE BOOL "Whether to compile the code as a shared libraries (DLL).")
mark_as_advanced(FORCE EZ_COMPILE_ENGINE_AS_DLL)

macro(ez_platform_pull_properties)

	get_property(EZ_CMAKE_PLATFORM_WINDOWS GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS)
	get_property(EZ_CMAKE_PLATFORM_WINDOWS_UWP GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_UWP)
	get_property(EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)
	get_property(EZ_CMAKE_WINDOWS_SDK_VERSION GLOBAL PROPERTY EZ_CMAKE_WINDOWS_SDK_VERSION)

endmacro()

macro (ez_platformhook_set_build_flags_clang)
    # Disable the warning that clang doesn't support pragma optimize.
    target_compile_options(${TARGET_NAME} PRIVATE -Wno-ignored-pragma-optimize -Wno-pragma-pack)
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
    elseif(CMAKE_GENERATOR MATCHES "Ninja") # Ninja makefiles. Only makefile format supported by Visual Studio Open Folder
        message(STATUS "Buildsystem is Ninja (EZ_CMAKE_GENERATOR_NINJA)")

        set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_NINJA ON)
        set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "Ninja")
        set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})
    else()
        message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Windows! Please extend ez_platform_detect_generator()")
    endif()
endmacro()