include("${CMAKE_CURRENT_LIST_DIR}/Configure_Default.cmake")

message(STATUS "Configuring Platform: Android")

set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_ANDROID ON)
set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_POSIX ON)
set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_SUPPORTS_VULKAN ON)

macro(ez_platform_pull_properties)

	get_property(EZ_CMAKE_PLATFORM_ANDROID GLOBAL PROPERTY EZ_CMAKE_PLATFORM_ANDROID)

endmacro()

macro(ez_platform_detect_generator)

	if(CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES "Ninja Multi-Config")
		message(STATUS "Buildsystem is Ninja (EZ_CMAKE_GENERATOR_NINJA)")

		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_NINJA ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "Ninja")
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})

	else()
		message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Android! Please extend ez_platform_detect_generator()")
	endif()

endmacro()

macro(ez_platformhook_link_target_vulkan TARGET_NAME)

	# on linux is the loader a dll
	get_target_property(_dll_location EzVulkan::Loader IMPORTED_LOCATION)

	if(NOT _dll_location STREQUAL "")
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:EzVulkan::Loader> $<TARGET_FILE_DIR:${TARGET_NAME}>)
	endif()

	unset(_dll_location)

endmacro()

macro(ez_platformhook_set_build_flags_clang TARGET_NAME)
	target_compile_options(${TARGET_NAME} PRIVATE -fPIC)

	# Look for the super fast ld compatible linker called "mold". If present we want to use it.
	find_program(MOLD_PATH "mold")

	# We want to use the llvm linker lld by default
	# Unless the user has specified a different linker
	get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)

	if("${TARGET_TYPE}" STREQUAL "SHARED_LIBRARY")
		if(NOT("${CMAKE_EXE_LINKER_FLAGS}" MATCHES "fuse-ld="))
			# TODO_ANDROID: Mold does not support `--undefined-glob` so we can't use it for Android right now. Either we need to prevent via some other means that the linger drops plugins or figure out which version of mold if any supports `--undefined-glob`.
			if(false) #if(MOLD_PATH)
				target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=${MOLD_PATH}")
			else()
				target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=lld")
			endif()
		endif()

		# Reporting missing symbols at linktime
		target_link_options(${TARGET_NAME} PRIVATE "-Wl,-z,defs")
		# Prevent discarding of statically linked plugins
		target_link_options(${TARGET_NAME} PRIVATE "LINKER:--undefined-glob=*ezReferenceFunction*")		
	endif()
endmacro()

macro(ez_platformhook_find_vulkan)

	# As we are cross compiling, CMake assumes every path to be located under the Android NDK root. This is not the case for external libraries like the Vulkan SDK, so we need to clear the sysroot and find root path.
	set(backup_CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH})
	set(backup_CMAKE_SYSROOT ${CMAKE_SYSROOT})
	set(CMAKE_FIND_ROOT_PATH "")
	set(CMAKE_SYSROOT "")

	if(EZ_CMAKE_ARCHITECTURE_64BIT)
		if((EZ_VULKAN_DIR STREQUAL "EZ_VULKAN_DIR-NOTFOUND") OR(EZ_VULKAN_DIR STREQUAL ""))
			#set(CMAKE_FIND_DEBUG_MODE TRUE)
			unset(EZ_VULKAN_DIR CACHE)
			unset(EzVulkan_DIR CACHE)
			set(EZ_SHARED_VULKAN_DIR "${EZ_ROOT}/Workspace/shared/vulkan-sdk/${EZ_CONFIG_VULKAN_SDK_LINUXX64_VERSION}")
			find_path(EZ_VULKAN_DIR config/vk_layer_settings.txt
					PATHS
					${EZ_SHARED_VULKAN_DIR}
					${EZ_VULKAN_DIR}
					$ENV{VULKAN_SDK}
			)
			#set(CMAKE_FIND_DEBUG_MODE FALSE)

			if((EZ_VULKAN_DIR STREQUAL "EZ_VULKAN_DIR-NOTFOUND") OR (EZ_VULKAN_DIR STREQUAL ""))
				# When cross-compiling on windows, we assume the env var VULKAN_SDK is set so the previous find_path call should have succeeded.
				# On Linux, we just download the SDK as we would do when building for Linux directly.
				# This is a bit wasteful as we already downloaded it and we only need a few headers, but cross workspace dependencies aren't easy to define in cmake. 
				if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
					# To prevent race-conditions if two CMake presets are updated at the same time, we download into the local workspace and then create a link into the shared directory.
					ez_download_and_extract("${EZ_CONFIG_VULKAN_SDK_LINUXX64_URL}" "${CMAKE_BINARY_DIR}/vulkan-sdk" "vulkan-sdk-${EZ_CONFIG_VULKAN_SDK_LINUXX64_VERSION}")
					ez_create_link("${CMAKE_BINARY_DIR}/vulkan-sdk/${EZ_CONFIG_VULKAN_SDK_LINUXX64_VERSION}" "${EZ_ROOT}/Workspace/shared/vulkan-sdk/" "${EZ_CONFIG_VULKAN_SDK_LINUXX64_VERSION}")
					set(EZ_VULKAN_DIR "${EZ_SHARED_VULKAN_DIR}" CACHE PATH "Directory of the Vulkan SDK" FORCE)

					find_path(EZ_VULKAN_DIR config/vk_layer_settings.txt NO_DEFAULT_PATH
							PATHS
							${EZ_VULKAN_DIR}
							$ENV{VULKAN_SDK}
					)
				endif ()
			endif()

			if((EZ_VULKAN_DIR STREQUAL "EZ_VULKAN_DIR-NOTFOUND") OR (EZ_VULKAN_DIR STREQUAL ""))
				message(FATAL_ERROR "Failed to find vulkan SDK. Ez requires the vulkan sdk ${EZ_CONFIG_VULKAN_SDK_LINUXX64_VERSION}. Please set the environment variable VULKAN_SDK to the vulkan sdk location.")
			endif()
		endif()
	else()
		message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
	endif()

	set(EZ_VULKAN_VALIDATIONLAYERS_DIR "" CACHE PATH "Directory of the Vulkan Validation Layers")

	# Download prebuilt VkLayer_khronos_validation for Android
	if((EZ_VULKAN_VALIDATIONLAYERS_DIR STREQUAL "EZ_VULKAN_VALIDATIONLAYERS_DIR-NOTFOUND") OR (EZ_VULKAN_VALIDATIONLAYERS_DIR STREQUAL ""))
		
		set(EZ_SHARED_VULKAN_VALIDATIONLAYERS_DIR "${EZ_ROOT}/Workspace/shared/vulkan-sdk/android-binaries-${EZ_CONFIG_VULKAN_VALIDATIONLAYERS_VERSION}")
		
		#set(CMAKE_FIND_DEBUG_MODE TRUE)
		unset(EZ_VULKAN_VALIDATIONLAYERS_DIR CACHE)
		find_path(EZ_VULKAN_VALIDATIONLAYERS_DIR
				NAMES arm64-v8a/libVkLayer_khronos_validation.so
				NO_DEFAULT_PATH
				HINTS
				${EZ_SHARED_VULKAN_VALIDATIONLAYERS_DIR}
				${EZ_VULKAN_VALIDATIONLAYERS_DIR}
		)
		#set(CMAKE_FIND_DEBUG_MODE FALSE)
		if((EZ_VULKAN_VALIDATIONLAYERS_DIR STREQUAL "EZ_VULKAN_VALIDATIONLAYERS_DIR-NOTFOUND") OR (EZ_VULKAN_VALIDATIONLAYERS_DIR STREQUAL ""))
			ez_download_and_extract("${EZ_CONFIG_VULKAN_VALIDATIONLAYERS_ANDROID_URL}" "${CMAKE_BINARY_DIR}/vulkan-sdk" "vulkan-layers-${EZ_CONFIG_VULKAN_VALIDATIONLAYERS_VERSION}")
			ez_create_link("${CMAKE_BINARY_DIR}/vulkan-sdk/android-binaries-${EZ_CONFIG_VULKAN_VALIDATIONLAYERS_VERSION}" "${EZ_ROOT}/Workspace/shared/vulkan-sdk/" "android-binaries-${EZ_CONFIG_VULKAN_VALIDATIONLAYERS_VERSION}")
			set(EZ_VULKAN_VALIDATIONLAYERS_DIR "${EZ_ROOT}/Workspace/shared/vulkan-sdk/android-binaries-${EZ_CONFIG_VULKAN_VALIDATIONLAYERS_VERSION}" CACHE PATH "Directory of the Vulkan Validation Layers" FORCE)

			find_path(EZ_VULKAN_VALIDATIONLAYERS_DIR arm64-v8a/libVkLayer_khronos_validation.so NO_DEFAULT_PATH
				PATHS
				${EZ_SHARED_VULKAN_VALIDATIONLAYERS_DIR}
				${EZ_VULKAN_VALIDATIONLAYERS_DIR}
			)
		endif()
	endif()

	set(CMAKE_FIND_ROOT_PATH ${backup_CMAKE_FIND_ROOT_PATH})
	set(CMAKE_SYSROOT ${backup_CMAKE_SYSROOT})

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(EzVulkan DEFAULT_MSG EZ_VULKAN_DIR)

	if(NOT ANDROID_NDK)
		message(WARNING "ANDROID_NDK not set")

		if(NOT EXISTS "$ENV{ANDROID_NDK_HOME}")
			message(FATAL_ERROR "ANDROID_NDK_HOME environment variable not set. Please ensure it points to the android NDK root folder.")
		else()
			set(ANDROID_NDK $ENV{ANDROID_NDK_HOME})
		endif()
	endif()

	if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
		set(EZ_VULKAN_INCLUDE_DIR "${EZ_VULKAN_DIR}/x86_64/include")
	elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
		set(EZ_VULKAN_INCLUDE_DIR "${EZ_VULKAN_DIR}/Include")
	else ()
		message(FATAL_ERROR "Unknown host system, can't find vulkan include dir.")
	endif ()

	if(EZ_CMAKE_ARCHITECTURE_64BIT)
		if(EZ_CMAKE_ARCHITECTURE_ARM)
			add_library(EzVulkan::Loader SHARED IMPORTED)
			set_target_properties(EzVulkan::Loader PROPERTIES IMPORTED_LOCATION "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-android/${ANDROID_PLATFORM}/libvulkan.so")
			set_target_properties(EzVulkan::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_VULKAN_INCLUDE_DIR}")
		elseif(EZ_CMAKE_ARCHITECTURE_X86)
			add_library(EzVulkan::Loader SHARED IMPORTED)
			set_target_properties(EzVulkan::Loader PROPERTIES IMPORTED_LOCATION "${CMAKE_SYSROOT}/usr/lib/x86_64-linux-android/${ANDROID_PLATFORM}/libvulkan.so")
			set_target_properties(EzVulkan::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_VULKAN_INCLUDE_DIR}")
		endif()
		# We define EzVulkan::DXC as a stub as we can't compile on android, but the high level init cmake code expects this function to define it.
		add_library(EzVulkan::DXC SHARED IMPORTED)
	else()
		message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
	endif()

endmacro()