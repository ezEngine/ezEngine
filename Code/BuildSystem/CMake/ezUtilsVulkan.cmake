# #####################################
# ## Vulkan support
# #####################################

set(EZ_BUILD_EXPERIMENTAL_VULKAN OFF CACHE BOOL "Whether to enable experimental / work-in-progress Vulkan code")

# #####################################
# ## ez_requires_vulkan()
# #####################################
macro(ez_requires_vulkan)
	ez_requires_one_of(EZ_CMAKE_PLATFORM_LINUX EZ_CMAKE_PLATFORM_WINDOWS)
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

		# Only on linux is the loader a dll.
		if(EZ_CMAKE_PLATFORM_LINUX)
			get_target_property(_dll_location EzVulkan::Loader IMPORTED_LOCATION)

			if(NOT _dll_location STREQUAL "")
				add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:EzVulkan::Loader> $<TARGET_FILE_DIR:${TARGET_NAME}>)
			endif()

			unset(_dll_location)
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
		target_include_directories(${TARGET_NAME} PRIVATE "${EZ_VULKAN_DIR}/source/SPIRV-Reflect")
		target_sources(${TARGET_NAME} PRIVATE "${EZ_VULKAN_DIR}/source/SPIRV-Reflect/spirv_reflect.h")
		target_sources(${TARGET_NAME} PRIVATE "${EZ_VULKAN_DIR}/source/SPIRV-Reflect/spirv_reflect.c")
		source_group("SPIRV-Reflect" FILES "${EZ_VULKAN_DIR}/source/SPIRV-Reflect/spirv_reflect.h" "${EZ_VULKAN_DIR}/source/SPIRV-Reflect/spirv_reflect.c")
	endif()
endfunction()