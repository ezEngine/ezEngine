######################################
### Fmod support
######################################

set (EZ_BUILD_FMOD OFF CACHE BOOL "Whether support for FMod should be added")

######################################
### ez_requires_fmod()
######################################

macro(ez_requires_fmod)

	ez_requires_windows()
	ez_requires(EZ_BUILD_FMOD)
	# While counter-intuitive, we need to find the package here so that the PUBLIC inherited
	# target_sources using generator expressions can be resolved in the dependant projects.
	find_package(EzFmod REQUIRED)

endmacro()

######################################
### ez_link_target_fmod(<target>)
######################################

function(ez_link_target_fmod TARGET_NAME)

	ez_requires_fmod()

	find_package(EzFmod REQUIRED)

	if (EZFMOD_FOUND)
        target_link_libraries(${TARGET_NAME} PRIVATE ezFmod::Studio)

        ez_uwp_add_import_to_sources(${TARGET_NAME} ezFmod::Core)
        ez_uwp_add_import_to_sources(${TARGET_NAME} ezFmod::Studio)

        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezFmod::Studio> $<TARGET_FILE_DIR:${TARGET_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezFmod::Core> $<TARGET_FILE_DIR:${TARGET_NAME}>
	  )

	endif()

endfunction()

