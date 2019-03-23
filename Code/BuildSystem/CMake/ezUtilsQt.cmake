######################################
### ez_requires_qt()
######################################

macro(ez_requires_qt)

    if (NOT EZ_ENABLE_QT_SUPPORT)
        return()
    endif()

endmacro()

######################################
### ez_prepare_find_qt()
######################################

function(ez_prepare_find_qt)

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

endfunction()

######################################
### ez_link_target_qt(TARGET <target> COMPONENTS <qt components> [COPY_DLLS])
######################################

function(ez_link_target_qt)

    ez_requires_qt()

    set(FN_OPTIONS COPY_DLLS)
    set(FN_ONEVALUEARGS TARGET)
    set(FN_MULTIVALUEARGS COMPONENTS)
    cmake_parse_arguments(FN_ARG "${FN_OPTIONS}" "${FN_ONEVALUEARGS}" "${FN_MULTIVALUEARGS}" ${ARGN} )

    if (FN_ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "ez_link_target_qt: Invalid arguments '${FN_ARG_UNPARSED_ARGUMENTS}'")
    endif()

    ez_prepare_find_qt()

    find_package (Qt5 COMPONENTS ${FN_ARG_COMPONENTS} REQUIRED PATHS ${EZ_QT_DIR})

    target_include_directories(${FN_ARG_TARGET} PRIVATE ${CMAKE_BINARY_DIR})
    target_include_directories(${FN_ARG_TARGET} PRIVATE ${CMAKE_BINARY_DIR}/Code)

    target_compile_definitions(${FN_ARG_TARGET} PUBLIC EZ_USE_QT)

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


######################################
### ez_qt_wrap_target_ui_files(<target> <files>)
######################################

function(ez_qt_wrap_target_ui_files TARGET_NAME FILES_TO_WRAP)

    list(FILTER FILES_TO_WRAP INCLUDE REGEX ".*\.ui$")

    if (NOT FILES_TO_WRAP)
        return()
    endif()

    ez_prepare_find_qt()
    find_package(Qt5 COMPONENTS Widgets REQUIRED PATHS ${EZ_QT_DIR})

    if (NOT TARGET Qt5::uic)
        message(FATAL_ERROR "UIC.exe not found")
    else()
        # due to the stupid way CMake variable scopes work, and a stupid way Qt sets up the Qt5Widgets_UIC_EXECUTABLE variable,
        # its value gets 'lost' (reset to empty) if find_package has been called in a different function before
        # since it is used internally by qt5_wrap_ui, we must ensure to restore it here, otherwise the custom build tool command on UI files
        # will miss the uic.exe part and just be "-o stuff" instead of "uic.exe -o stuff"
        get_target_property(Qt5Widgets_UIC_EXECUTABLE Qt5::uic IMPORTED_LOCATION)
    endif()

    qt5_wrap_ui(UIC_FILES ${FILES_TO_WRAP})

    target_sources(${TARGET_NAME} PRIVATE ${UIC_FILES})

    source_group("Qt\\UI Files" FILES ${UIC_FILES})

endfunction()

######################################
### ez_qt_wrap_target_moc_files(<target> <files>)
######################################

function(ez_qt_wrap_target_moc_files TARGET_NAME FILES_TO_WRAP)

    list(FILTER FILES_TO_WRAP INCLUDE REGEX ".*\.h$")

    if (NOT FILES_TO_WRAP)
        return()
    endif()

    ez_prepare_find_qt()
    find_package(Qt5 COMPONENTS Widgets REQUIRED PATHS ${EZ_QT_DIR})

    set(Qt5Core_MOC_EXECUTABLE Qt5::moc)
    qt5_wrap_cpp(MOC_FILES TARGET ${TARGET_NAME} ${FILES_TO_WRAP})
    
    # retrieve target PCH
    #qt5_wrap_cpp(MOC_FILES TARGET ${TARGET_NAME} ${FILES_TO_WRAP} OPTIONS -b "${PCH_H}")
    
    target_sources(${TARGET_NAME} PRIVATE ${MOC_FILES})
    
    source_group("Qt\\MOC Files" FILES ${MOC_FILES})

endfunction()
    
    
######################################
### ez_qt_wrap_target_qrc_files(<target> <files>)
######################################

function(ez_qt_wrap_target_qrc_files TARGET_NAME FILES_TO_WRAP)
    
    list(FILTER FILES_TO_WRAP INCLUDE REGEX ".*\.qrc$")

    if (NOT FILES_TO_WRAP)
        return()
    endif()

    ez_prepare_find_qt()
    find_package(Qt5 COMPONENTS Widgets REQUIRED PATHS ${EZ_QT_DIR})
    
    set(Qt5Core_RCC_EXECUTABLE Qt5::rcc)
    qt5_add_resources(QRC_FILES ${FILES_TO_WRAP})
    
    target_sources(${TARGET_NAME} PRIVATE ${QRC_FILES})
    
    source_group("Qt\\QRC Files" FILES ${QRC_FILES})
    
endfunction()
    
######################################
### ez_qt_wrap_target_files(<target> <files>)
######################################

function(ez_qt_wrap_target_files TARGET_NAME FILES_TO_WRAP)

    ez_qt_wrap_target_qrc_files(${TARGET_NAME} "${FILES_TO_WRAP}")
    
    ez_qt_wrap_target_ui_files(${TARGET_NAME} "${FILES_TO_WRAP}")
    
    # in this automated method, we only MOC files that end with ".moc.h"
    list(FILTER FILES_TO_WRAP INCLUDE REGEX ".*\.moc\.h")
    ez_qt_wrap_target_moc_files(${TARGET_NAME} "${FILES_TO_WRAP}")

endfunction()


