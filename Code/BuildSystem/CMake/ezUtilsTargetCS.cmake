######################################
### ez_create_target_cs(<LIBRARY | APPLICATION> <target-name>)
######################################

function(ez_create_target_cs TYPE TARGET_NAME)

    set(ARG_OPTIONS NO_EZ_PREFIX)
    set(ARG_ONEVALUEARGS "")
    set(ARG_MULTIVALUEARGS "")
    cmake_parse_arguments(ARG "${ARG_OPTIONS}" "${ARG_ONEVALUEARGS}" "${ARG_MULTIVALUEARGS}" ${ARGN} )

    if (ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "ez_create_target_cs: Invalid arguments '${ARG_UNPARSED_ARGUMENTS}'")
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

    elseif (${TYPE} STREQUAL "APPLICATION")

        message (STATUS "Application: ${TARGET_NAME}")

        add_executable (${TARGET_NAME} ${ALL_SOURCE_FILES})

    else()

        message(FATAL_ERROR "ez_create_target_cs: Missing argument to specify target type. Pass in 'APP' or 'LIB'.")

    endif()

    # sort files into the on-disk folder structure in the IDE
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ALL_SOURCE_FILES})

    # default c# settings
    target_compile_options(${TARGET_NAME} PRIVATE "/langversion:6")
    set_property(TARGET ${TARGET_NAME} PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.6.1")
    set_property(TARGET ${TARGET_NAME} PROPERTY WIN32_EXECUTABLE TRUE)
        
    ez_set_default_target_output_dirs(${TARGET_NAME})
    ez_set_project_ide_folder(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR})

endfunction()
