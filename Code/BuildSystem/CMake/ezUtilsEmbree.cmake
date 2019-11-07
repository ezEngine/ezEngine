######################################
### Embree support
######################################

set (EZ_BUILD_EMBREE OFF CACHE BOOL "Whether support for Intel Embree should be added")

######################################
### ez_requires_embree()
######################################

macro(ez_requires_embree)

	ez_requires_windows()
  ez_requires(EZ_BUILD_EMBREE)

endmacro()

######################################
### ez_link_target_embree(<target>)
######################################

function(ez_link_target_embree TARGET_NAME)

	ez_requires_embree()

	find_package(EzEmbree REQUIRED)

	if (EZEMBREE_FOUND)
	
	  target_link_libraries(${TARGET_NAME} PRIVATE EzEmbree::EzEmbree)
	
	  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:EzEmbree::EzEmbree> $<TARGET_FILE_DIR:${TARGET_NAME}>
	  )
	
	endif()

endfunction()

