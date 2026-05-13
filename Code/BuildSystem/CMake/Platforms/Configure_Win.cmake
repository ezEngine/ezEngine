include("${CMAKE_CURRENT_LIST_DIR}/Configure_Default.cmake")

message(STATUS "Configuring Platform: Windows")

set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS ON)
set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP ON)
set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_SUPPORTS_VULKAN ON)
set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_SUPPORTS_WEBGPU ON)
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

macro (ez_platformhook_set_build_flags_clang TARGET_NAME)
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

macro (ez_platformhook_make_windowapp TARGET_NAME)
    set_property(TARGET ${TARGET_NAME} PROPERTY WIN32_EXECUTABLE ON)
endmacro()

macro(ez_platformhook_find_vulkan)
    if(EZ_CMAKE_ARCHITECTURE_64BIT AND EZ_CMAKE_ARCHITECTURE_X86)
        set(EZ_DXC_DIR "${EZ_ROOT}/Workspace/shared/DXC-WinX64-${EZ_CONFIG_DIRECTXSHADERCOMPILER_WINX64_VERSION}")
        ez_download_and_extract("${EZ_CONFIG_DIRECTXSHADERCOMPILER_WINX64_URL}" "${EZ_DXC_DIR}" "DXC-WinX64-${EZ_CONFIG_DIRECTXSHADERCOMPILER_WINX64_VERSION}")
    else()
        message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(EzVulkan DEFAULT_MSG EZ_DXC_DIR)

    if(EZ_CMAKE_ARCHITECTURE_64BIT AND EZ_CMAKE_ARCHITECTURE_X86)
        add_library(EzVulkan::DXC SHARED IMPORTED)
        set_target_properties(EzVulkan::DXC PROPERTIES IMPORTED_LOCATION "${EZ_DXC_DIR}/bin/x64/dxcompiler.dll")
        set_target_properties(EzVulkan::DXC PROPERTIES IMPORTED_IMPLIB "${EZ_DXC_DIR}/lib/x64/dxcompiler.lib")
        set_target_properties(EzVulkan::DXC PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_DXC_DIR}/inc")
    else()
        message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
    endif() 
endmacro()

macro(ez_platformhook_find_qt)
	if(EZ_CMAKE_COMPILER_CLANG)
		# The qt6 interface compile options contain msvc specific flags which don't exist for clang.
		set_target_properties(Qt6::Platform PROPERTIES INTERFACE_COMPILE_OPTIONS "")
		
		# Qt6 link options include '-NXCOMPAT' which does not exist on clang.
		get_target_property(QtLinkOptions Qt6::PlatformCommonInternal INTERFACE_LINK_OPTIONS)
		string(REPLACE "-NXCOMPAT;" "" QtLinkOptions "${QtLinkOptions}")
		set_target_properties(Qt6::PlatformCommonInternal PROPERTIES INTERFACE_LINK_OPTIONS "${QtLinkOptions}")
	endif()
endmacro()


macro(ez_platformhook_download_qt)

	# Currently only implemented for x64
	if(EZ_CMAKE_ARCHITECTURE_64BIT)
		if(EZ_CMAKE_ARCHITECTURE_64BIT)
			set(EZ_SDK_VERSION "${EZ_CONFIG_QT_WINX64_VERSION}")
			set(EZ_SDK_URL "${EZ_CONFIG_QT_WINX64_URL}")
		endif()

		# Reset EZ_QT_DIR if it points to an auto-managed Qt package that no longer matches
		# the configured version, so the correct version gets downloaded automatically.
		# User-specified custom paths (not matching the "Qt6-" naming convention) are left alone.
		set(EZ_EXPECTED_QT_DIR "${CMAKE_BINARY_DIR}/../${EZ_SDK_VERSION}")
		if(NOT "${EZ_QT_DIR}" STREQUAL "${EZ_EXPECTED_QT_DIR}" AND "${EZ_QT_DIR}" MATCHES "Qt6-")
			set(EZ_QT_DIR "EZ_QT_DIR-NOTFOUND" CACHE PATH "Directory of the Qt installation" FORCE)
		endif()

		if((EZ_QT_DIR STREQUAL "EZ_QT_DIR-NOTFOUND") OR(EZ_QT_DIR STREQUAL ""))
			ez_download_and_extract("${EZ_SDK_URL}" "${CMAKE_BINARY_DIR}/.." "${EZ_SDK_VERSION}")

			set(EZ_QT_DIR "${CMAKE_BINARY_DIR}/../${EZ_SDK_VERSION}" CACHE PATH "Directory of the Qt installation" FORCE)
		endif()
	endif()

endmacro()