######################################
### RmlUi support
######################################

set (EZ_BUILD_RMLUI OFF CACHE BOOL "Whether support for RmlUi should be added")

######################################
### ez_requires_rmlui()
######################################

macro(ez_requires_rmlui)

	ez_requires_windows()
  ez_requires(EZ_BUILD_RMLUI)
	if (EZ_CMAKE_PLATFORM_WINDOWS_UWP)
		return()
	endif()

endmacro()

######################################
### ez_link_target_rmlui(<target>)
######################################

function(ez_link_target_rmlui TARGET_NAME)

	ez_requires_rmlui()

	find_package(EzRmlUi REQUIRED)

	if (EZRMLUI_FOUND)
	
	  target_link_libraries(${TARGET_NAME} PRIVATE EzRmlUi::Core)
	
	  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:EzRmlUi::Core> $<TARGET_FILE_DIR:${TARGET_NAME}>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:EzRmlUi::Freetype> $<TARGET_FILE_DIR:${TARGET_NAME}>
	  )
	
	endif()

endfunction()

