######################################
### OpenXR support
######################################

set (EZ_BUILD_OPENXR ON CACHE BOOL "Whether support for OpenXR should be added")

######################################
### ez_link_target_openxr(<target>)
######################################

function(ez_link_target_openxr TARGET_NAME)

	target_link_libraries(${TARGET_NAME} PRIVATE OpenXR)

endfunction()

