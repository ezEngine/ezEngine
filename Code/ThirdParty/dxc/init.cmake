ez_cmake_init()

######################################
### ez_requires_dxc()
######################################

macro(ez_requires_dxc)

	ez_requires_windows()
	if (EZ_CMAKE_PLATFORM_WINDOWS_UWP)
		return()
	endif()

endmacro()

######################################
### ez_link_target_dxc(<target>)
######################################

function(ez_link_target_dxc TARGET_NAME)

	ez_requires_dxc()
	
	target_link_libraries(${TARGET_NAME} PRIVATE TargetDXC)

	add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:TargetDXC> $<TARGET_FILE_DIR:${TARGET_NAME}> )
	
endfunction()

ez_requires_dxc()

# Necessarz because this is an init.cmake file which has a different 'current_dir'
get_property(EZ_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH)
set(EZ_DXC_LIB_PATH ${CMAKE_SOURCE_DIR}/${EZ_SUBMODULE_PREFIX_PATH}/Code/ThirdParty/dxc)

# Has to be done in the init.cmake file, because CMake has problems with add_custom_command if the target isn't known yet
add_library(TargetDXC SHARED IMPORTED)
set_target_properties(TargetDXC PROPERTIES IMPORTED_LOCATION "${EZ_DXC_LIB_PATH}/bin/dxcompiler.dll")
set_target_properties(TargetDXC PROPERTIES IMPORTED_IMPLIB "${EZ_DXC_LIB_PATH}/lib/dxcompiler.lib")
set_target_properties(TargetDXC PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_DXC_LIB_PATH}/include")

