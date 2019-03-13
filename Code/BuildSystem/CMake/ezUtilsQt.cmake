######################################
### ez_link_target_qt
######################################

function(ez_link_target_qt)

    set(FN_OPTIONS COPY_DLLS)
    set(FN_ONEVALUEARGS TARGET)
    set(FN_MULTIVALUEARGS COMPONENTS)
    cmake_parse_arguments(FN_ARG "${FN_OPTIONS}" "${FN_ONEVALUEARGS}" "${FN_MULTIVALUEARGS}" ${ARGN} )

    if (FN_ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "ez_link_target_qt: Invalid arguments '${FN_ARG_UNPARSED_ARGUMENTS}'")
    endif()

    set (EZ_CACHED_QT_DIR "EZ_CACHED_QT_DIR-NOTFOUND" CACHE STRING "")
    mark_as_advanced(EZ_CACHED_QT_DIR FORCE)

    if (NOT "${EZ_QT_DIR}" STREQUAL "${EZ_CACHED_QT_DIR}")
        # Need to reset qt vars now so that 'find_package' is re-executed

        set (EZ_CACHED_QT_DIR ${EZ_QT_DIR} CACHE STRING "" FORCE)

        message(STATUS "Qt-dir has changed, clearing cached Qt paths")

        # Clear cached qt dirs
        set (Qt5_DIR "Qt5_DIR-NOTFOUND" CACHE PATH "" FORCE)
        set (Qt5Core_DIR "Qt5Core_DIR-NOTFOUND" CACHE PATH "" FORCE)
        set (Qt5Gui_DIR "Qt5Gui_DIR-NOTFOUND" CACHE PATH "" FORCE)
        set (Qt5Widgets_DIR "Qt5Widgets_DIR-NOTFOUND" CACHE PATH "" FORCE)
        set (Qt5Network_DIR "Qt5Network_DIR-NOTFOUND" CACHE PATH "" FORCE)
        set (Qt5WinExtras_DIR "Qt5WinExtras_DIR-NOTFOUND" CACHE PATH "" FORCE)
    endif()

    # force find_package to search for Qt in the correct folder
    set (CMAKE_PREFIX_PATH ${EZ_QT_DIR})

    target_include_directories(${FN_ARG_TARGET} PRIVATE ${CMAKE_BINARY_DIR})
    target_include_directories(${FN_ARG_TARGET} PRIVATE ${CMAKE_BINARY_DIR}/Code)

    target_compile_definitions(${FN_ARG_TARGET} PUBLIC EZ_USE_QT)

    find_package (Qt5 COMPONENTS ${FN_ARG_COMPONENTS} REQUIRED PATHS ${EZ_QT_DIR})

    foreach(module ${FN_ARG_COMPONENTS})

        target_link_libraries(${FN_ARG_TARGET} PUBLIC "Qt5::${module}")

        if (FN_ARG_COPY_DLLS)

            # copy the dll into the binary folder for each configuration using generator expressions
            # as a post-build step for every qt-enabled target:
            add_custom_command(TARGET ${FN_ARG_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt5::${module}>
                $<TARGET_FILE_DIR:${FN_ARG_TARGET}>)

        endif()

    endforeach()

    if (FN_ARG_COPY_DLLS)

        # Copy 'imageformats' into the binary folder.
        add_custom_command(TARGET ${FN_ARG_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${EZ_QT_DIR}/plugins/imageformats"
            "$<TARGET_FILE_DIR:${FN_ARG_TARGET}>/imageformats")

        # Copy 'platforms' into the binary folder.
        add_custom_command(TARGET ${FN_ARG_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${EZ_QT_DIR}/plugins/platforms"
            "$<TARGET_FILE_DIR:${FN_ARG_TARGET}>/platforms")

    endif()

endfunction()