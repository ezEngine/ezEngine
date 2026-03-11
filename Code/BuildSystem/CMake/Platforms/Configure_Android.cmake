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

	# Download prebuilt VkLayer_khronos_validation for Android
	set(EZ_SHARED_VULKAN_VALIDATIONLAYERS_DIR "${EZ_ROOT}/Workspace/shared/VulkanValidationLayer-AndroidArm64-${EZ_CONFIG_VULKAN_VALIDATIONLAYERS_VERSION}")
	ez_download_and_extract("${EZ_CONFIG_VULKAN_VALIDATIONLAYERS_ANDROID_URL}" "${EZ_SHARED_VULKAN_VALIDATIONLAYERS_DIR}" "VulkanValidationLayer-AndroidArm64-${EZ_CONFIG_VULKAN_VALIDATIONLAYERS_VERSION}")

	#set(EZ_VULKAN_VALIDATIONLAYERS_DIR "${EZ_SHARED_VULKAN_VALIDATIONLAYERS_DIR}" CACHE PATH "Directory of the Vulkan Validation Layers" FORCE)
	find_path(EZ_VULKAN_VALIDATIONLAYERS_DIR arm64-v8a/libVkLayer_khronos_validation.so NO_DEFAULT_PATH
		PATHS
		${EZ_SHARED_VULKAN_VALIDATIONLAYERS_DIR}/android-binaries-${EZ_CONFIG_VULKAN_VALIDATIONLAYERS_VERSION}
	)

	set(CMAKE_FIND_ROOT_PATH ${backup_CMAKE_FIND_ROOT_PATH})
	set(CMAKE_SYSROOT ${backup_CMAKE_SYSROOT})

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(EzVulkan DEFAULT_MSG EZ_VULKAN_VALIDATIONLAYERS_DIR)

	if(NOT ANDROID_NDK)
		message(WARNING "ANDROID_NDK not set")

		if(NOT EXISTS "$ENV{ANDROID_NDK_HOME}")
			message(FATAL_ERROR "ANDROID_NDK_HOME environment variable not set. Please ensure it points to the android NDK root folder.")
		else()
			set(ANDROID_NDK $ENV{ANDROID_NDK_HOME})
		endif()
	endif()

endmacro()

macro(ez_platformhook_package_files TARGET_NAME SRC_FOLDER DST_FOLDER)

	# Package files for Android APK by copying them to the Assets directory
	# This is done in PRE_BUILD so that it happens before the APK generation steps
	# that happen in POST_BUILD via ez_create_target
	set(ANDROID_PACKAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/package/Assets")
	
	add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "${ANDROID_PACKAGE_DIR}/${DST_FOLDER}"
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${SRC_FOLDER}" "${ANDROID_PACKAGE_DIR}/${DST_FOLDER}"
		COMMENT "Packaging ${SRC_FOLDER} -> ${DST_FOLDER} for Android")

endmacro()