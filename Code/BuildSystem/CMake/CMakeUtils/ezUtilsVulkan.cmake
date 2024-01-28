# #####################################
# ## Vulkan support
# #####################################

set(EZ_BUILD_EXPERIMENTAL_VULKAN OFF CACHE BOOL "Whether to enable experimental / work-in-progress Vulkan code")

# #####################################
# ## ez_requires_vulkan()
# #####################################
macro(ez_requires_vulkan)
	ez_requires(EZ_CMAKE_PLATFORM_SUPPORTS_VULKAN)
	ez_requires(EZ_BUILD_EXPERIMENTAL_VULKAN)
	find_package(EzVulkan REQUIRED)
endmacro()

# #####################################
# ## ez_link_target_vulkan(<target>)
# #####################################
function(ez_link_target_vulkan TARGET_NAME)
	ez_requires_vulkan()

	find_package(EzVulkan REQUIRED)

	if(EZVULKAN_FOUND)
		target_link_libraries(${TARGET_NAME} PRIVATE EzVulkan::Loader)

		if (COMMAND ez_platformhook_link_target_vulkan)
			# call platform-specific hook for linking with Vulkan
			ez_platformhook_link_target_vulkan()
		endif()
	endif()
endfunction()

# #####################################
# ## ez_link_target_dxc(<target>)
# #####################################
function(ez_link_target_dxc TARGET_NAME)
	ez_requires_vulkan()

	find_package(EzVulkan REQUIRED)

	if(EZVULKAN_FOUND)
		target_link_libraries(${TARGET_NAME} PRIVATE EzVulkan::DXC)

		get_target_property(_dll_location EzVulkan::DXC IMPORTED_LOCATION)

		if(NOT _dll_location STREQUAL "")
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:EzVulkan::DXC> $<TARGET_FILE_DIR:${TARGET_NAME}>)
		endif()

		unset(_dll_location)
	endif()
endfunction()

# #####################################
# ## ez_sources_target_spirv_reflect(<target>)
# #####################################
function(ez_sources_target_spirv_reflect TARGET_NAME)
	ez_requires_vulkan()

	find_package(EzVulkan REQUIRED)

	if(EZVULKAN_FOUND)
		if(EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP AND EZ_CMAKE_ARCHITECTURE_64BIT)
			target_include_directories(${TARGET_NAME} PRIVATE "${EZ_VULKAN_DIR}/Source/SPIRV-Reflect")
			target_sources(${TARGET_NAME} PRIVATE "${EZ_VULKAN_DIR}/Source/SPIRV-Reflect/spirv_reflect.h")
			target_sources(${TARGET_NAME} PRIVATE "${EZ_VULKAN_DIR}/Source/SPIRV-Reflect/spirv_reflect.c")
			source_group("SPIRV-Reflect" FILES "${EZ_VULKAN_DIR}/Source/SPIRV-Reflect/spirv_reflect.h" "${EZ_VULKAN_DIR}/x86_64/include/SPIRV-Reflect/spirv_reflect.c")
		elseif(EZ_CMAKE_PLATFORM_LINUX AND EZ_CMAKE_ARCHITECTURE_64BIT)
			target_include_directories(${TARGET_NAME} PRIVATE "${EZ_VULKAN_DIR}/x86_64/include/SPIRV-Reflect")
			target_sources(${TARGET_NAME} PRIVATE "${EZ_VULKAN_DIR}/x86_64/include/SPIRV-Reflect/spirv_reflect.h")
			target_sources(${TARGET_NAME} PRIVATE "${EZ_VULKAN_DIR}/x86_64/include/SPIRV-Reflect/spirv_reflect.c")
			source_group("SPIRV-Reflect" FILES "${EZ_VULKAN_DIR}/x86_64/include/SPIRV-Reflect/spirv_reflect.h" "${EZ_VULKAN_DIR}/x86_64/include/SPIRV-Reflect/spirv_reflect.c")
		else()
			message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
		endif()
	endif()
endfunction()