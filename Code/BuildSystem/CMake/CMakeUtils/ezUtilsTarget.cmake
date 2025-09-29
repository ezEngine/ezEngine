# #####################################
# ## ez_create_target(<LIBRARY | APPLICATION> <target-name> [NO_PCH] [NO_UNITY] [NO_QT] [ALL_SYMBOLS_VISIBLE] [EXCLUDE_FOLDER_FOR_UNITY <relative-folder>...])
# #####################################

macro(ez_create_target TYPE TARGET_NAME)
	ez_apply_build_filter(${TARGET_NAME})

	set(ARG_OPTIONS NO_PCH NO_UNITY NO_QT NO_EZ_PREFIX ENABLE_RTTI NO_WARNINGS_AS_ERRORS NO_COMPLIANCE ALL_SYMBOLS_VISIBLE NO_DEBUG NO_EXPORT)
	set(ARG_ONEVALUEARGS "")
	set(ARG_MULTIVALUEARGS EXCLUDE_FOLDER_FOR_UNITY EXCLUDE_FROM_PCH_REGEX MANUAL_SOURCE_FILES)
	cmake_parse_arguments(ARG "${ARG_OPTIONS}" "${ARG_ONEVALUEARGS}" "${ARG_MULTIVALUEARGS}" ${ARGN})

	if(ARG_UNPARSED_ARGUMENTS)
		message(FATAL_ERROR "ez_create_target: Invalid arguments '${ARG_UNPARSED_ARGUMENTS}'")
	endif()

	ez_pull_all_vars()

	if(DEFINED ARG_MANUAL_SOURCE_FILES)
		set(ALL_SOURCE_FILES ${ARG_MANUAL_SOURCE_FILES})
	else()
		ez_glob_source_files(${CMAKE_CURRENT_SOURCE_DIR} ALL_SOURCE_FILES)
	endif()

	# SHARED_LIBRARY means always shared
	# LIBRARY means SHARED_LIBRARY when EZ_COMPILE_ENGINE_AS_DLL is on, otherwise STATIC_LIBRARY
	if((${TYPE} STREQUAL "LIBRARY") OR(${TYPE} STREQUAL "STATIC_LIBRARY") OR(${TYPE} STREQUAL "SHARED_LIBRARY"))
		if((${EZ_COMPILE_ENGINE_AS_DLL} AND(${TYPE} STREQUAL "LIBRARY")) OR(${TYPE} STREQUAL "SHARED_LIBRARY"))
			message(STATUS "Shared Library: ${TARGET_NAME}")
			add_library(${TARGET_NAME} SHARED "${ALL_SOURCE_FILES}")

		else()
			message(STATUS "Static Library: ${TARGET_NAME}")
			add_library(${TARGET_NAME} STATIC "${ALL_SOURCE_FILES}")
		endif()

		if(ARG_NO_EZ_PREFIX)
			# on some platforms like linux there is a default prefix like "lib".
			# We don't want that as it confuses our plugin system.
			set_target_properties(${TARGET_NAME} PROPERTIES IMPORT_PREFIX "")
			set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "")
		else()
			ez_add_output_ez_prefix(${TARGET_NAME})
		endif()

		if(COMMAND ez_platformhook_set_library_properties)
			ez_platformhook_set_library_properties(${TARGET_NAME})
		endif()

	elseif(${TYPE} STREQUAL "APPLICATION")
		message(STATUS "Application: ${TARGET_NAME}")

		# PLATFORM-TODO
		# On Android we can't use executables. Instead we have to use shared libraries which are loaded from java code.
		if(EZ_CMAKE_PLATFORM_ANDROID)
			# All ez applications must include the native app glue implementation
			add_library(${TARGET_NAME} SHARED ${ALL_SOURCE_FILES} "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c")

			# Prevent the linker from stripping away the application entry point of android_native_app_glue: ANativeActivity_onCreate
			set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

			# The log and android libraries are library dependencies of android_native_app_glue
			target_link_libraries(${TARGET_NAME} PRIVATE log android EGL GLESv1_CM)
		else()
			add_executable(${TARGET_NAME} ${ALL_SOURCE_FILES})
		endif()

		if(COMMAND ez_platformhook_set_application_properties)
			ez_platformhook_set_application_properties(${TARGET_NAME})
		endif()

	else()
		message(FATAL_ERROR "ez_create_target: Missing argument to specify target type. Pass in 'APP' or 'LIB'.")
	endif()

	# sort files into the on-disk folder structure in the IDE
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ALL_SOURCE_FILES})

	if(NOT ${ARG_NO_PCH})
		ez_auto_pch(${TARGET_NAME} "${ALL_SOURCE_FILES}" ${ARG_EXCLUDE_FROM_PCH_REGEX})
	endif()

	# When using the Open Folder workflow inside visual studio on android, visual studio gets confused due to our custom output directory
	# Do not set the custom output directory in this case
	if((NOT ANDROID) OR(NOT EZ_CMAKE_INSIDE_VS))
		# PLATFORM-TODO
		ez_set_default_target_output_dirs(${TARGET_NAME})
	endif()

	# PLATFORM-TODO: add general hook ?

	# We need the target directory to add the apk packaging steps for android. Thus, this step needs to be done here.
	if(${TYPE} STREQUAL "APPLICATION")
		ez_android_add_default_content(${TARGET_NAME})
	endif()

	ez_add_target_folder_as_include_dir(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})

	ez_set_common_target_definitions(${TARGET_NAME})

	ez_set_build_flags(${TARGET_NAME} ${ARGN})

	# On linux we want all symbols to be hidden by default. We manually "export" them.
	if(EZ_COMPILE_ENGINE_AS_DLL AND EZ_CMAKE_PLATFORM_LINUX AND NOT ARG_ALL_SYMBOLS_VISIBLE)
		# PLATFORM-TODO (use general hook as above?)
		target_compile_options(${TARGET_NAME} PRIVATE "-fvisibility=hidden")
	endif()

	ez_set_project_ide_folder(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})

	# PLATFORM-TODO (use general hook as above?)
	# Pass the windows sdk include paths to the resource compiler when not generating a visual studio solution.
	if(EZ_CMAKE_PLATFORM_WINDOWS AND NOT EZ_CMAKE_GENERATOR_MSVC)
		set(RC_FILES ${ALL_SOURCE_FILES})
		list(FILTER RC_FILES INCLUDE REGEX ".*\\.rc$")

		if(RC_FILES)
			set_source_files_properties(${RC_FILES}
				PROPERTIES COMPILE_FLAGS "/I\"C:/Program Files (x86)/Windows Kits/10/Include/${EZ_CMAKE_WINDOWS_SDK_VERSION}/shared\" /I\"C:/Program Files (x86)/Windows Kits/10/Include/${EZ_CMAKE_WINDOWS_SDK_VERSION}/um\""
			)
		endif()
	endif()

	# add a platform specific include directory
	if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Platform/${EZ_CMAKE_PLATFORM_POSTFIX}")
		target_include_directories(${TARGET_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Platform/${EZ_CMAKE_PLATFORM_POSTFIX}")
	endif()

	# PLATFORM-TODO (use general hook as above?)
	if(EZ_CMAKE_PLATFORM_ANDROID)
		# Add the location for native_app_glue.h to the include directories.
		target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")
	endif()

	if (EZ_CMAKE_PLATFORM_WEB)
		set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".html")
	endif()

	if(NOT ${ARG_NO_QT})
		ez_qt_wrap_target_files(${TARGET_NAME} "${ALL_SOURCE_FILES}")
	endif()

	ez_ci_add_to_targets_list(${TARGET_NAME} C++)

	if(NOT ${ARG_NO_UNITY})
		ez_generate_folder_unity_files_for_target(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR} "${ARG_EXCLUDE_FOLDER_FOR_UNITY}")
	endif()

	get_property(GATHER_EXTERNAL_PROJECTS GLOBAL PROPERTY "GATHER_EXTERNAL_PROJECTS")

	if(GATHER_EXTERNAL_PROJECTS)
		set_property(GLOBAL APPEND PROPERTY "EXTERNAL_PROJECTS" ${TARGET_NAME})
	endif()

	if(NOT ${ARG_NO_EXPORT})
		get_property(GATHER_EXPORT_PROJECTS GLOBAL PROPERTY "GATHER_EXPORT_PROJECTS")

		if(GATHER_EXPORT_PROJECTS)
			set_property(GLOBAL APPEND PROPERTY "EXPORT_PROJECTS" ${TARGET_NAME})
		endif()
	else()
		message(STATUS "${TARGET_NAME} was excluded from export (NO_EXPORT)")
	endif()
	
endmacro()