######################################
### Vulkan support
######################################

set (EZ_BUILD_EXPERIMENTAL_VULKAN OFF CACHE BOOL "Whether to enable experimental / work-in-progress Vulkan code")

######################################
### ez_requires_vulkan()
######################################

macro(ez_requires_vulkan)

	ez_requires_windows()
	ez_requires(EZ_BUILD_EXPERIMENTAL_VULKAN)

endmacro()

######################################
### ez_link_target_vulkan(<target>)
######################################

function(ez_link_target_vulkan TARGET_NAME)

	get_property(EZ_VULKAN_LIBRARIES GLOBAL PROPERTY EZ_VULKAN_LIBRARIES)

	# only execute find_package once
	if (NOT EZ_VULKAN_LIBRARIES)

		find_package(Vulkan)

		if (Vulkan_FOUND)
		
			set_property(GLOBAL PROPERTY EZ_VULKAN_INCLUDE ${Vulkan_INCLUDE_DIRS})
			set_property(GLOBAL PROPERTY EZ_VULKAN_LIBRARIES ${Vulkan_LIBRARIES})

		endif()

	endif()

	get_property(EZ_VULKAN_INCLUDE GLOBAL PROPERTY EZ_VULKAN_INCLUDE)
	get_property(EZ_VULKAN_LIBRARIES GLOBAL PROPERTY EZ_VULKAN_LIBRARIES)

	target_link_libraries(${TARGET_NAME} 
		PRIVATE 
		${EZ_VULKAN_LIBRARIES})

	target_include_directories(${TARGET_NAME} 
		PRIVATE 
		${EZ_VULKAN_INCLUDE})

endfunction()