include("${CMAKE_CURRENT_LIST_DIR}/Configure_Default.cmake")

message(STATUS "Configuring Platform: Linux")

set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_LINUX ON)
set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_POSIX ON)
set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_SUPPORTS_VULKAN ON)

# #####################################
# ## General settings
# #####################################
set(EZ_COMPILE_ENGINE_AS_DLL ON CACHE BOOL "Whether to compile the code as a shared libraries (DLL).")
mark_as_advanced(FORCE EZ_COMPILE_ENGINE_AS_DLL)

# #####################################
# ## Experimental Editor support on Linux
# #####################################
set (EZ_EXPERIMENTAL_EDITOR_ON_LINUX OFF CACHE BOOL "Wether or not to build the editor on linux")

if (EZ_EXPERIMENTAL_EDITOR_ON_LINUX)
    set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_SUPPORTS_EDITOR ON)
endif()

macro(ez_platform_pull_properties)

    get_property(EZ_CMAKE_PLATFORM_LINUX GLOBAL PROPERTY EZ_CMAKE_PLATFORM_LINUX)

endmacro()

macro(ez_platformhook_link_target_vulkan)

    # on linux is the loader a dll
    get_target_property(_dll_location EzVulkan::Loader IMPORTED_LOCATION)

    if(NOT _dll_location STREQUAL "")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:EzVulkan::Loader> $<TARGET_FILE_DIR:${TARGET_NAME}>)
    endif()

    unset(_dll_location)

endmacro()

macro(ez_platformhook_set_build_flags_clang)
	target_compile_options(${TARGET_NAME} PRIVATE -fPIC)

	# Look for the super fast ld compatible linker called "mold". If present we want to use it.
	find_program(MOLD_PATH "mold")

	# We want to use the llvm linker lld by default
	# Unless the user has specified a different linker
	get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)

	if("${TARGET_TYPE}" STREQUAL "SHARED_LIBRARY")
		if(NOT("${CMAKE_EXE_LINKER_FLAGS}" MATCHES "fuse-ld="))
			if(MOLD_PATH)
				target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=${MOLD_PATH}")
			else()
				target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=lld")
			endif()
		endif()

		# Reporting missing symbols at linktime
		target_link_options(${TARGET_NAME} PRIVATE "-Wl,-z,defs")
	elseif("${TARGET_TYPE}" STREQUAL "EXECUTABLE")
		if(NOT("${CMAKE_SHARED_LINKER_FLAGS}" MATCHES "fuse-ld="))
			if(MOLD_PATH)
				target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=${MOLD_PATH}")
			else()
				target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=lld")
			endif()
		endif()

		# Reporting missing symbols at linktime
		target_link_options(${TARGET_NAME} PRIVATE "-Wl,-z,defs")
	endif()
endmacro()

macro(ez_platformhook_set_application_properties TARGET_NAME)

    # We need to link against pthread and rt last or linker errors will occur.
	target_link_libraries(${TARGET_NAME} PRIVATE pthread rt)

endmacro()

macro(ez_platform_detect_generator)
    if(CMAKE_GENERATOR MATCHES "Unix Makefiles") # Unix Makefiles (for QtCreator etc.)
        message(STATUS "Buildsystem is Make (EZ_CMAKE_GENERATOR_MAKE)")

        set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_MAKE ON)
        set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "Make")
        set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})

    elseif(CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES "Ninja Multi-Config")
        message(STATUS "Buildsystem is Ninja (EZ_CMAKE_GENERATOR_NINJA)")

        set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_NINJA ON)
        set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "Ninja")
        set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})
    else()
        message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Linux! Please extend ez_platform_detect_generator()")
    endif()
endmacro()


macro(ez_platformhook_set_library_properties TARGET_NAME)
    # c = libc.so (the C standard library)
    # m = libm.so (the C standard library math portion)
    # pthread = libpthread.so (thread support)
    # rt = librt.so (compiler runtime functions)
    target_link_libraries(${TARGET_NAME} PRIVATE pthread rt c m)

    get_property(EZ_CMAKE_COMPILER_GCC GLOBAL PROPERTY EZ_CMAKE_COMPILER_GCC)
    if(EZ_CMAKE_COMPILER_GCC)
        # Workaround for: https://bugs.launchpad.net/ubuntu/+source/gcc-5/+bug/1568899
        target_link_libraries(${TARGET_NAME} PRIVATE -lgcc_s -lgcc)
    endif()
endmacro()

macro(ez_platformhook_find_vulkan)
    if(EZ_CMAKE_ARCHITECTURE_64BIT)
        if((EZ_VULKAN_DIR STREQUAL "EZ_VULKAN_DIR-NOTFOUND") OR(EZ_VULKAN_DIR STREQUAL ""))
            #set(CMAKE_FIND_DEBUG_MODE TRUE)
            unset(EZ_VULKAN_DIR CACHE)
            unset(EzVulkan_DIR CACHE)
            set(EZ_SHARED_VULKAN_DIR "${EZ_ROOT}/Workspace/shared/vulkan-sdk/${EZ_CONFIG_VULKAN_SDK_LINUXX64_VERSION}")
            find_path(EZ_VULKAN_DIR config/vk_layer_settings.txt NO_DEFAULT_PATH
                PATHS
                ${EZ_SHARED_VULKAN_DIR}
                ${EZ_VULKAN_DIR}
                $ENV{VULKAN_SDK}
            )
            if(EZ_CMAKE_ARCHITECTURE_X86)
                if((EZ_VULKAN_DIR STREQUAL "EZ_VULKAN_DIR-NOTFOUND") OR (EZ_VULKAN_DIR STREQUAL ""))
                    # To prevent race-conditions if two CMake presets are updated at the same time, we download into the local workspace and then create a link into the shared directory.
                    ez_download_and_extract("${EZ_CONFIG_VULKAN_SDK_LINUXX64_URL}" "${CMAKE_BINARY_DIR}/vulkan-sdk" "vulkan-sdk-${EZ_CONFIG_VULKAN_SDK_LINUXX64_VERSION}")
                    ez_create_link("${CMAKE_BINARY_DIR}/vulkan-sdk/${EZ_CONFIG_VULKAN_SDK_LINUXX64_VERSION}" "${EZ_ROOT}/Workspace/shared/vulkan-sdk/" "${EZ_CONFIG_VULKAN_SDK_LINUXX64_VERSION}")
                    set(EZ_VULKAN_DIR "${EZ_SHARED_VULKAN_DIR}" CACHE PATH "Directory of the Vulkan SDK" FORCE)

                    find_path(EZ_VULKAN_DIR config/vk_layer_settings.txt NO_DEFAULT_PATH
                        PATHS
                        ${EZ_VULKAN_DIR}
                        $ENV{VULKAN_SDK}
                    )
                endif()
            endif()

            if((EZ_VULKAN_DIR STREQUAL "EZ_VULKAN_DIR-NOTFOUND") OR (EZ_VULKAN_DIR STREQUAL ""))
                message(FATAL_ERROR "Failed to find vulkan SDK. Ez requires the vulkan sdk ${EZ_CONFIG_VULKAN_SDK_LINUXX64_VERSION}. Please set the environment variable VULKAN_SDK to the vulkan sdk location.")
            endif()

            # set(CMAKE_FIND_DEBUG_MODE FALSE)
        endif()
    else()
        message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(EzVulkan DEFAULT_MSG EZ_VULKAN_DIR)
    
	if(EZ_CMAKE_ARCHITECTURE_64BIT)
		add_library(EzVulkan::Loader SHARED IMPORTED)
		set_target_properties(EzVulkan::Loader PROPERTIES IMPORTED_LOCATION "${EZ_VULKAN_DIR}/x86_64/lib/libvulkan.so")
		set_target_properties(EzVulkan::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_VULKAN_DIR}/x86_64/include")

		add_library(EzVulkan::DXC SHARED IMPORTED)
		set_target_properties(EzVulkan::DXC PROPERTIES IMPORTED_LOCATION "${EZ_VULKAN_DIR}/x86_64/lib/libdxcompiler.so")
		set_target_properties(EzVulkan::DXC PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_VULKAN_DIR}/x86_64/include")
	else()
		message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
	endif()
    
endmacro()