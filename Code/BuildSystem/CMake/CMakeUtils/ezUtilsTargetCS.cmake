# #####################################
# ## ez_create_target_cs(<LIBRARY | APPLICATION> <target-name>)
# #####################################

macro(ez_create_target_cs TYPE TARGET_NAME)
	ez_apply_build_filter(${TARGET_NAME})

	set(ARG_OPTIONS NO_EZ_PREFIX)
	set(ARG_ONEVALUEARGS DOTNET_VERSION)
	set(ARG_MULTIVALUEARGS "")
	cmake_parse_arguments(ARG "${ARG_OPTIONS}" "${ARG_ONEVALUEARGS}" "${ARG_MULTIVALUEARGS}" ${ARGN})

	if(ARG_UNPARSED_ARGUMENTS)
		message(FATAL_ERROR "ez_create_target_cs: Invalid arguments '${ARG_UNPARSED_ARGUMENTS}'")
	endif()

	ez_pull_all_vars()

	ez_glob_source_files(${CMAKE_CURRENT_SOURCE_DIR} ALL_SOURCE_FILES)

	# SHARED_LIBRARY means always shared
	# LIBRARY means SHARED_LIBRARY when EZ_COMPILE_ENGINE_AS_DLL is on, otherwise STATIC_LIBRARY
	if((${TYPE} STREQUAL "LIBRARY") OR(${TYPE} STREQUAL "STATIC_LIBRARY") OR(${TYPE} STREQUAL "SHARED_LIBRARY"))
		if((${EZ_COMPILE_ENGINE_AS_DLL} AND(${TYPE} STREQUAL "LIBRARY")) OR(${TYPE} STREQUAL "SHARED_LIBRARY"))
			message(STATUS "Shared Library (C#): ${TARGET_NAME}")
			add_library(${TARGET_NAME} SHARED ${ALL_SOURCE_FILES})

		else()
			message(STATUS "Static Library (C#): ${TARGET_NAME}")
			add_library(${TARGET_NAME} ${ALL_SOURCE_FILES})
		endif()

		if(NOT ARG_NO_EZ_PREFIX)
			ez_add_output_ez_prefix(${TARGET_NAME})
		endif()

	elseif(${TYPE} STREQUAL "APPLICATION")
		message(STATUS "Application (C#): ${TARGET_NAME}")

		add_executable(${TARGET_NAME} ${ALL_SOURCE_FILES})

	else()
		message(FATAL_ERROR "ez_create_target_cs: Missing argument to specify target type. Pass in 'APPLICATION' or 'SHARED_LIBRARY' or 'STATIC_LIBRARY'.")
	endif()

	# sort files into the on-disk folder structure in the IDE
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ALL_SOURCE_FILES})

	# default C# settings
	target_compile_options(${TARGET_NAME} PRIVATE "/langversion:default")

	# setting this turns the app into a "Windows" application, not a "Console" application
	# use ez_make_windowapp if this is desired
	# set_property(TARGET ${TARGET_NAME} PROPERTY WIN32_EXECUTABLE TRUE)
	if(ARG_DOTNET_VERSION)
		# message(STATUS "Custom .NET version: ${ARG_DOTNET_VERSION}")
		set_property(TARGET ${TARGET_NAME} PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v${ARG_DOTNET_VERSION}")
	else()
		set_property(TARGET ${TARGET_NAME} PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.8")
	endif()

	ez_set_default_target_output_dirs(${TARGET_NAME})
	ez_set_project_ide_folder(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})

endmacro()
