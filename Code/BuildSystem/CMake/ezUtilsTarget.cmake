######################################
### ez_create_target(<LIBRARY | APPLICATION> <target-name> [NO_PCH] [NO_UNITY] [NO_QT] [EXCLUDE_FOLDER_FOR_UNITY <relative-folder>...])
######################################

function(ez_create_target TYPE TARGET_NAME)

    set(ARG_OPTIONS NO_PCH NO_UNITY NO_QT NO_EZ_PREFIX)
    set(ARG_ONEVALUEARGS "")
    set(ARG_MULTIVALUEARGS EXCLUDE_FOLDER_FOR_UNITY EXCLUDE_FROM_PCH_REGEX)
    cmake_parse_arguments(ARG "${ARG_OPTIONS}" "${ARG_ONEVALUEARGS}" "${ARG_MULTIVALUEARGS}" ${ARGN} )

    if (ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "ez_create_target: Invalid arguments '${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    ez_pull_all_vars()

    ez_glob_source_files(${CMAKE_CURRENT_SOURCE_DIR} ALL_SOURCE_FILES)
	

    if ((${TYPE} STREQUAL "LIBRARY") OR (${TYPE} STREQUAL "STATIC_LIBRARY"))

        if ((${EZ_COMPILE_ENGINE_AS_DLL}) AND (${TYPE} STREQUAL "LIBRARY"))

            message (STATUS "Shared Library: ${TARGET_NAME}")
            add_library (${TARGET_NAME} SHARED ${ALL_SOURCE_FILES})

        else ()

            message (STATUS "Static Library: ${TARGET_NAME}")
            add_library (${TARGET_NAME} ${ALL_SOURCE_FILES})

        endif ()

        if (NOT ARG_NO_EZ_PREFIX)
            ez_add_output_ez_prefix(${TARGET_NAME})
        endif()

        ez_set_library_properties(${TARGET_NAME})
        ez_uwp_fix_library_properties(${TARGET_NAME} ${ALL_SOURCE_FILES})

    elseif (${TYPE} STREQUAL "APPLICATION")

        message (STATUS "Application: ${TARGET_NAME}")
		
		# On Android we can't use executables. Instead we have to use shared libraries which are loaded from java code.
		if (EZ_CMAKE_PLATFORM_ANDROID)
			# All ez applications must include the native app glue implementation
			add_library(${TARGET_NAME} SHARED ${ALL_SOURCE_FILES} "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c")
			
			# Prevent the linker from stripping away the application entry point of android_native_app_glue: ANativeActivity_onCreate
			set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

			# The log and android libraries are library dependencies of android_native_app_glue
            target_link_libraries(${TARGET_NAME} PRIVATE log android EGL GLESv1_CM)
            
            ez_android_add_default_content(${TARGET_NAME})
		else()
			add_executable (${TARGET_NAME} ${ALL_SOURCE_FILES})
		endif()
        
        ez_uwp_add_default_content(${TARGET_NAME})
        
        ez_set_application_properties(${TARGET_NAME})

    else()

        message(FATAL_ERROR "ez_create_target: Missing argument to specify target type. Pass in 'APP' or 'LIB'.")

    endif()

    # sort files into the on-disk folder structure in the IDE
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ALL_SOURCE_FILES})

    if (NOT ${ARG_NO_PCH})

        ez_auto_pch(${TARGET_NAME} "${ALL_SOURCE_FILES}" ${ARG_EXCLUDE_FROM_PCH_REGEX})

    endif()

    ez_set_default_target_output_dirs(${TARGET_NAME})

    ez_add_target_folder_as_include_dir(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})

    ez_set_common_target_definitions(${TARGET_NAME})

    ez_set_build_flags(${TARGET_NAME})

    ez_set_project_ide_folder(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})
	
	if(EZ_CMAKE_PLATFORM_ANDROID)
		# Add the location for native_app_glue.h to the include directories.
		target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")
	endif()

    if (NOT ${ARG_NO_QT})

        ez_qt_wrap_target_files(${TARGET_NAME} "${ALL_SOURCE_FILES}")

    endif()

    ez_ci_add_to_targets_list(${TARGET_NAME} C++)

    if (NOT ${ARG_NO_UNITY})

        ez_generate_folder_unity_files_for_target(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR} "${ARG_EXCLUDE_FOLDER_FOR_UNITY}")

    endif()
	
	get_property(GATHER_EXTERNAL_PROJECTS GLOBAL PROPERTY "GATHER_EXTERNAL_PROJECTS")
	
	if (GATHER_EXTERNAL_PROJECTS)
		set_property(GLOBAL APPEND_STRING PROPERTY "EXTERNAL_PROJECTS" ${TARGET_NAME})
	endif()

endfunction()

