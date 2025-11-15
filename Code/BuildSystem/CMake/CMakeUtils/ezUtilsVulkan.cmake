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
