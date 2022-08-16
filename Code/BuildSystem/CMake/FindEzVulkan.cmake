# find the folder into which the Vulkan SDK has been installed

# early out, if this target has been created before
if((TARGET EzVulkan::Loader) AND(TARGET EzVulkan::DXC))
	return()
endif()

set(EZ_VULKAN_DIR $ENV{VULKAN_SDK} CACHE PATH "Directory of the Vulkan SDK")

ez_pull_compiler_and_architecture_vars()
ez_pull_config_vars()

get_property(EZ_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH)

if(EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP AND EZ_CMAKE_ARCHITECTURE_64BIT)
	if((EZ_VULKAN_DIR STREQUAL "EZ_VULKAN_DIR-NOTFOUND") OR(EZ_VULKAN_DIR STREQUAL ""))
		# set(CMAKE_FIND_DEBUG_MODE TRUE)
		unset(EZ_VULKAN_DIR CACHE)
		unset(EzVulkan_DIR CACHE)
		find_path(EZ_VULKAN_DIR Config/vk_layer_settings.txt
			PATHS
			${EZ_VULKAN_DIR}
			$ENV{VULKAN_SDK}
			REQUIRED
		)

		# set(CMAKE_FIND_DEBUG_MODE FALSE)
	endif()
elseif(EZ_CMAKE_PLATFORM_LINUX AND EZ_CMAKE_ARCHITECTURE_64BIT)
	if((EZ_VULKAN_DIR STREQUAL "EZ_VULKAN_DIR-NOTFOUND") OR(EZ_VULKAN_DIR STREQUAL ""))
		# set(CMAKE_FIND_DEBUG_MODE TRUE)
		unset(EZ_VULKAN_DIR CACHE)
		unset(EzVulkan_DIR CACHE)
		find_path(EZ_VULKAN_DIR config/vk_layer_settings.txt
			PATHS
			${EZ_VULKAN_DIR}
			$ENV{VULKAN_SDK}
		)

		if(EZ_CMAKE_ARCHITECTURE_X86)
			if((EZ_VULKAN_DIR STREQUAL "EZ_VULKAN_DIR-NOTFOUND") OR (EZ_VULKAN_DIR STREQUAL ""))
				ez_download_and_extract("${EZ_CONFIG_VULKAN_SDK_LINUXX64_URL}" "${CMAKE_BINARY_DIR}/vulkan-sdk" "vulkan-sdk-${EZ_CONFIG_VULKAN_SDK_LINUXX64_VERSION}")
				set(EZ_VULKAN_DIR "${CMAKE_BINARY_DIR}/vulkan-sdk/${EZ_CONFIG_VULKAN_SDK_LINUXX64_VERSION}" CACHE PATH "Directory of the Vulkan SDK" FORCE)

				find_path(EZ_VULKAN_DIR config/vk_layer_settings.txt
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

if(EZVULKAN_FOUND)
	if(EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP AND EZ_CMAKE_ARCHITECTURE_64BIT)
		add_library(EzVulkan::Loader STATIC IMPORTED)
		set_target_properties(EzVulkan::Loader PROPERTIES IMPORTED_LOCATION "${EZ_VULKAN_DIR}/Lib/vulkan-1.lib")
		set_target_properties(EzVulkan::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_VULKAN_DIR}/Include")

		add_library(EzVulkan::DXC SHARED IMPORTED)
		set_target_properties(EzVulkan::DXC PROPERTIES IMPORTED_LOCATION "${EZ_VULKAN_DIR}/Bin/dxcompiler.dll")
		set_target_properties(EzVulkan::DXC PROPERTIES IMPORTED_IMPLIB "${EZ_VULKAN_DIR}/Lib/dxcompiler.lib")
		set_target_properties(EzVulkan::DXC PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_VULKAN_DIR}/Include")

	elseif(EZ_CMAKE_PLATFORM_LINUX AND EZ_CMAKE_ARCHITECTURE_64BIT)
		add_library(EzVulkan::Loader SHARED IMPORTED)
		set_target_properties(EzVulkan::Loader PROPERTIES IMPORTED_LOCATION "${EZ_VULKAN_DIR}/x86_64/lib/libvulkan.so.1.3.216")
		set_target_properties(EzVulkan::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_VULKAN_DIR}/x86_64/include")

		add_library(EzVulkan::DXC SHARED IMPORTED)
		set_target_properties(EzVulkan::DXC PROPERTIES IMPORTED_LOCATION "${EZ_VULKAN_DIR}/x86_64/lib/libdxcompiler.so.3.7")
		set_target_properties(EzVulkan::DXC PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_VULKAN_DIR}/x86_64/include")
	else()
		message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
	endif()
endif()
