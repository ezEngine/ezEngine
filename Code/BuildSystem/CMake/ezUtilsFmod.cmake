######################################
### Fmod support
######################################

set (EZ_BUILD_FMOD OFF CACHE BOOL "Whether support for FMod should be added")

######################################
### ez_requires_fmod()
######################################

macro(ez_requires_fmod)

	ez_requires_windows()
	ez_requires(${EZ_BUILD_FMOD})

endmacro()

######################################
### ez_link_target_fmod(<target>)
######################################

function(ez_link_target_fmod TARGET_NAME)

	ez_requires_fmod()

	find_package(EzFmod REQUIRED)

	if (EZFMOD_FOUND)
	  target_link_libraries(${TARGET_NAME} PRIVATE ezFmod::Studio)
	
	  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezFmod::Studio> $<TARGET_FILE_DIR:${TARGET_NAME}>
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezFmod::LowLevel> $<TARGET_FILE_DIR:${TARGET_NAME}>
	  )
	
	endif()
	
endfunction()

