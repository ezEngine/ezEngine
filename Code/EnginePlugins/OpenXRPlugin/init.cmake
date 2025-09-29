######################################
### OpenXR support
######################################

set (EZ_BUILD_OPENXR ON CACHE BOOL "Whether support for OpenXR should be added")

######################################
### ez_requires_openxr()
######################################

macro(ez_requires_openxr)

	ez_requires(EZ_CMAKE_PLATFORM_WINDOWS)
	ez_requires(EZ_BUILD_OPENXR)
	# While counter-intuitive, we need to find the package here so that the PUBLIC inherited
	# target_sources using generator expressions can be resolved in the dependant projects.
	find_package(ezOpenXR REQUIRED)

endmacro()

######################################
### ez_link_target_openxr(<target>)
######################################

function(ez_link_target_openxr TARGET_NAME)

	ez_requires_openxr()

	find_package(ezOpenXR REQUIRED)

	if (EZOPENXR_FOUND)
		target_link_libraries(${TARGET_NAME} PRIVATE ezOpenXR::Loader)

        get_target_property(_dll_location ezOpenXR::Loader IMPORTED_LOCATION)
		if (NOT _dll_location STREQUAL "")
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezOpenXR::Loader> $<TARGET_FILE_DIR:${TARGET_NAME}>)
		endif()
		unset(_dll_location)
        
        if (EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP AND EZ_CMAKE_ARCHITECTURE_64BIT)
			target_link_libraries(${TARGET_NAME} PRIVATE ezOpenXR::Remoting)

			# Copy INTERFACE_SOURCES to the output folder.
			get_target_property(REMOTING_ASSETS ezOpenXR::Remoting INTERFACE_SOURCES)
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${REMOTING_ASSETS} $<TARGET_FILE_DIR:${TARGET_NAME}>)
			set_property(SOURCE ${REMOTING_ASSETS} PROPERTY VS_DEPLOYMENT_CONTENT 1)
			set_property(SOURCE ${REMOTING_ASSETS} PROPERTY VS_DEPLOYMENT_LOCATION "")
			unset(REMOTING_ASSETS)

        endif()

	endif()

endfunction()

