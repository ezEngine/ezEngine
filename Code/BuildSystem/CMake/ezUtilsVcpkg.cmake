
######################################
### ez_vcpkg_show_info()
######################################

function(ez_vcpkg_show_info)

	message("This project can use vcpkg to find its 3rd party dependencies.")
	message("You can install vcpkg either manually or using InstallVcpkg.bat.")
	message("Make sure to set the environment variable 'VCPKG_ROOT' to point to your installation of vcpkg.")
	message("")

endfunction()


######################################
### ez_vcpkg_init()
######################################

function(ez_vcpkg_init)

	message(STATUS "EnvVar %VCPKG_ROOT% is set to '$ENV{VCPKG_ROOT}'")

	if (CMAKE_TOOLCHAIN_FILE)

		message(STATUS "CMAKE_TOOLCHAIN_FILE is already set to '${CMAKE_TOOLCHAIN_FILE}' - not going to modify it.")
		get_filename_component(EZ_VCPKG_ROOT "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
		get_filename_component(EZ_VCPKG_ROOT "${EZ_VCPKG_ROOT}" DIRECTORY)
		get_filename_component(EZ_VCPKG_ROOT "${EZ_VCPKG_ROOT}" DIRECTORY)

	else()

		if(DEFINED ENV{VCPKG_ROOT})
			set(EZ_VCPKG_ROOT "$ENV{VCPKG_ROOT}")
			message(STATUS "EnvVar VCPKG_ROOT is specified, using that.")
		else()
			set(EZ_VCPKG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg")
			message(STATUS "EnvVar VCPKG_ROOT is not specified, using '${CMAKE_CURRENT_SOURCE_DIR}/vcpkg'.")
		endif()

		if(NOT EXISTS "${EZ_VCPKG_ROOT}/vcpkg.exe" OR NOT EXISTS "${EZ_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
			message("vcpkg is not installed. Either install it manually and set the environment variable VCPKG_ROOT to its directory, or run InstallVcpkg.bat")
			return()
		endif()

		set(CMAKE_TOOLCHAIN_FILE ${EZ_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake CACHE STRING "" FORCE)
		message(STATUS "Forcing CMAKE_TOOLCHAIN_FILE to point to '${CMAKE_TOOLCHAIN_FILE}'")

	endif()

	message(STATUS "EZ_VCPKG_ROOT is '${EZ_VCPKG_ROOT}'")
	set_property(GLOBAL PROPERTY "EZ_VCPKG_ROOT" ${EZ_VCPKG_ROOT})

endfunction()


######################################
### ez_vcpkg_install(<packages>)
######################################

function(ez_vcpkg_install PACKAGES)

    ez_cmake_init()

    get_property(EZ_VCPKG_ROOT GLOBAL PROPERTY "EZ_VCPKG_ROOT")

    if (EZ_CMAKE_PLATFORM_WINDOWS)
        if (EZ_CMAKE_ARCHITECTURE_64BIT)
            set(VCPKG_TARGET_TRIPLET "x64-windows")
        else()
            set(VCPKG_TARGET_TRIPLET "x86-windows")
        endif()
    else()
        message(FATAL_ERROR "vcpkg target triplet is not configured for this platform")
    endif()

	foreach(PACKAGE ${PACKAGES})

		message("Vcpgk: Installing '${PACKAGE}', this may take a while.")
		execute_process(COMMAND "${EZ_VCPKG_ROOT}/vcpkg.exe" install "${PACKAGE}:${VCPKG_TARGET_TRIPLET}" WORKING_DIRECTORY "${EZ_VCPKG_ROOT}")

	endforeach()

endfunction()


