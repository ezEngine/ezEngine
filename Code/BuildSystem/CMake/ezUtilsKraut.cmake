######################################
### Kraut support
######################################



######################################
### ez_requires_kraut()
######################################

macro(ez_requires_kraut)

	ez_requires_windows()
	if (EZ_CMAKE_PLATFORM_WINDOWS_UWP)
		return()
	endif()

endmacro()

######################################
### ez_link_target_assimp(<target>)
######################################

function(ez_link_target_kraut TARGET_NAME)

	ez_requires_kraut()

	find_package(EzKraut REQUIRED)

	if (EZKRAUT_FOUND)
	
	  target_link_libraries(${TARGET_NAME} PRIVATE EzKraut::EzKrautGenerator EzKraut::EzKrautFoundation)
	
	  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:EzKraut::EzKrautGenerator> $<TARGET_FILE_DIR:${TARGET_NAME}>)
	  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:EzKraut::EzKrautFoundation> $<TARGET_FILE_DIR:${TARGET_NAME}>)
	
	endif()

endfunction()

