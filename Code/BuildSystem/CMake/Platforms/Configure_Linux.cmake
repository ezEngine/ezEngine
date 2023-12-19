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

macro(ez_platform_pull_properties)

    get_property(EZ_CMAKE_PLATFORM_LINUX GLOBAL PROPERTY EZ_CMAKE_PLATFORM_LINUX)
    get_property(EZ_CMAKE_PLATFORM_SUPPORTS_VULKAN GLOBAL PROPERTY EZ_CMAKE_PLATFORM_SUPPORTS_VULKAN)

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

endfunction()

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
        message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Linux! Please extend ez_detect_generator()")
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