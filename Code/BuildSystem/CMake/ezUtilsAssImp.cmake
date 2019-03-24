######################################
### AssImp support
######################################



######################################
### ez_requires_assimp()
######################################

macro(ez_requires_assimp)

	ez_requires_windows()

endmacro()

######################################
### ez_link_target_assimp(<target>)
######################################

function(ez_link_target_assimp TARGET_NAME)

	ez_requires_assimp()

	find_package(EzAssImp REQUIRED)

	if (EZASSIMP_FOUND)
	
	  target_link_libraries(${TARGET_NAME} PRIVATE EzAssImp::EzAssImp)
	
	  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:EzAssImp::EzAssImp> $<TARGET_FILE_DIR:${TARGET_NAME}>
	  )
	
	endif()

endfunction()

