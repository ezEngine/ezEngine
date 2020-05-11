######################################
### Qt support
######################################

set (EZ_ENABLE_QT_SUPPORT ON CACHE BOOL "Whether to add Qt support.")
set (EZ_QT_DIR $ENV{QTDIR} CACHE PATH "Directory of the Qt installation")

######################################
### ez_requires_qt()
######################################

macro(ez_requires_qt)

    ez_requires(EZ_ENABLE_QT_SUPPORT)

endmacro()

######################################
### ez_prepare_find_qt()
######################################

function(ez_prepare_find_qt)

    set (EZ_CACHED_QT_DIR "EZ_CACHED_QT_DIR-NOTFOUND" CACHE STRING "")
    mark_as_advanced(EZ_CACHED_QT_DIR FORCE)

    ######################
    ## Download Qt package

    ez_pull_compiler_and_architecture_vars()
    ez_pull_platform_vars()

    # Currently only implemented for x64
    if (EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP AND EZ_CMAKE_ARCHITECTURE_64BIT)
        if ((EZ_QT_DIR STREQUAL "EZ_QT_DIR-NOTFOUND") OR (EZ_QT_DIR STREQUAL ""))

            if (EZ_CMAKE_ARCHITECTURE_64BIT)
                set (EZ_SDK_VERSION "Qt-5.13.0-vs141-x64")
                set (EZ_SDK_URL "https://github.com/ezEngine/thirdparty/releases/download/Qt-5.13.0-vs141-x64/Qt-5.13.0-vs141-x64.zip")
            endif()

            set (EZ_SDK_LOCAL_ZIP "${CMAKE_BINARY_DIR}/${EZ_SDK_VERSION}.zip")

            if (NOT EXISTS ${EZ_SDK_LOCAL_ZIP})
                message(STATUS "Downloading '${EZ_SDK_URL}'...")
                file(DOWNLOAD ${EZ_SDK_URL} ${EZ_SDK_LOCAL_ZIP} SHOW_PROGRESS)

                message(STATUS "Extracting '${EZ_SDK_LOCAL_ZIP}'...")  
                execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf ${EZ_SDK_LOCAL_ZIP} WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
            else()
                message(STATUS "Already downloaded '${EZ_SDK_LOCAL_ZIP}'")
            endif()
            
            set (EZ_QT_DIR "${CMAKE_BINARY_DIR}/${EZ_SDK_VERSION}" CACHE PATH "Directory of the Qt installation" FORCE)
        endif()    
    endif()    

    ## Download Qt package
    ######################
  

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

        if (EZ_CMAKE_PLATFORM_WINDOWS)
            set (Qt5WinExtras_DIR "Qt5WinExtras_DIR-NOTFOUND" CACHE PATH "" FORCE)
        endif()
    endif()

    # force find_package to search for Qt in the correct folder
    if (EZ_QT_DIR)
        set (CMAKE_PREFIX_PATH ${EZ_QT_DIR} PARENT_SCOPE)
    endif()

endfunction()

######################################
### ez_link_target_qt(TARGET <target> COMPONENTS <qt components> [COPY_DLLS])
######################################

function(ez_link_target_qt)

    ez_pull_all_vars()

    ez_requires_qt()

    set(FN_OPTIONS COPY_DLLS)
    set(FN_ONEVALUEARGS TARGET)
    set(FN_MULTIVALUEARGS COMPONENTS)
    cmake_parse_arguments(FN_ARG "${FN_OPTIONS}" "${FN_ONEVALUEARGS}" "${FN_MULTIVALUEARGS}" ${ARGN} )

    if (FN_ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "ez_link_target_qt: Invalid arguments '${FN_ARG_UNPARSED_ARGUMENTS}'")
    endif()

    if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
        list(REMOVE_ITEM FN_ARG_COMPONENTS WinExtras)
    endif()

    ez_prepare_find_qt()

    if (EZ_QT_DIR)
        find_package (Qt5 COMPONENTS ${FN_ARG_COMPONENTS} REQUIRED PATHS ${EZ_QT_DIR})
    else()
        find_package (Qt5 COMPONENTS ${FN_ARG_COMPONENTS} REQUIRED)
    endif()

    mark_as_advanced(FORCE Qt5_DIR)
    mark_as_advanced(FORCE Qt5Core_DIR)
    mark_as_advanced(FORCE Qt5Gui_DIR)
    mark_as_advanced(FORCE Qt5Widgets_DIR)
    mark_as_advanced(FORCE Qt5Network_DIR)    

    if (EZ_CMAKE_PLATFORM_WINDOWS)
        mark_as_advanced(FORCE Qt5WinExtras_DIR)
    endif()
	
    get_property(EZ_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH)

    file(RELATIVE_PATH SUB_FOLDER "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")

    target_include_directories(${FN_ARG_TARGET} PUBLIC ${CMAKE_BINARY_DIR}/${SUB_FOLDER})

    target_compile_definitions(${FN_ARG_TARGET} PUBLIC EZ_USE_QT)

    foreach(module ${FN_ARG_COMPONENTS})

        if (NOT ${EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP})
            # skip Windows-only components
            if (${module} STREQUAL "WinExtras")
                continue()
            endif()
        endif()

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

    if (EZ_QT_DIR AND FN_ARG_COPY_DLLS)

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

    ez_requires_qt()

    list(FILTER FILES_TO_WRAP INCLUDE REGEX ".*\.ui$")

    if (NOT FILES_TO_WRAP)
        return()
    endif()

    ez_prepare_find_qt()
    if (EZ_QT_DIR)
        find_package(Qt5 COMPONENTS Widgets REQUIRED PATHS ${EZ_QT_DIR})
    else()
        find_package(Qt5 COMPONENTS Widgets REQUIRED)
    endif()

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

    ez_requires_qt()

    list(FILTER FILES_TO_WRAP INCLUDE REGEX ".*\.h$")

    if (NOT FILES_TO_WRAP)
        return()
    endif()

    ez_prepare_find_qt()
    if (EZ_QT_DIR)
        find_package(Qt5 COMPONENTS Widgets REQUIRED PATHS ${EZ_QT_DIR})
    else()
        find_package(Qt5 COMPONENTS Widgets REQUIRED)
    endif()

    set(Qt5Core_MOC_EXECUTABLE Qt5::moc)
    ez_retrieve_target_pch(${TARGET_NAME} PCH_H)

    if (PCH_H)
        qt5_wrap_cpp(MOC_FILES TARGET ${TARGET_NAME} ${FILES_TO_WRAP} OPTIONS -b "${PCH_H}.h")
		ez_pch_use("${PCH_H}.h" "${MOC_FILES}")
    else()
        qt5_wrap_cpp(MOC_FILES TARGET ${TARGET_NAME} ${FILES_TO_WRAP})
    endif()

    target_sources(${TARGET_NAME} PRIVATE ${MOC_FILES})

    source_group("Qt\\MOC Files" FILES ${MOC_FILES})

endfunction()


######################################
### ez_qt_wrap_target_qrc_files(<target> <files>)
######################################

function(ez_qt_wrap_target_qrc_files TARGET_NAME FILES_TO_WRAP)

    ez_requires_qt()

    list(FILTER FILES_TO_WRAP INCLUDE REGEX ".*\.qrc$")

    if (NOT FILES_TO_WRAP)
        return()
    endif()

    ez_prepare_find_qt()
    if (EZ_QT_DIR)
        find_package(Qt5 COMPONENTS Widgets REQUIRED PATHS ${EZ_QT_DIR})
    else()
        find_package(Qt5 COMPONENTS Widgets REQUIRED)
    endif()

    set(Qt5Core_RCC_EXECUTABLE Qt5::rcc)
    qt5_add_resources(QRC_FILES ${FILES_TO_WRAP})
	
    target_sources(${TARGET_NAME} PRIVATE ${QRC_FILES})

    source_group("Qt\\QRC Files" FILES ${QRC_FILES})

endfunction()

######################################
### ez_qt_wrap_target_files(<target> <files>)
######################################

function(ez_qt_wrap_target_files TARGET_NAME FILES_TO_WRAP)

    ez_requires_qt()

    ez_qt_wrap_target_qrc_files(${TARGET_NAME} "${FILES_TO_WRAP}")

    ez_qt_wrap_target_ui_files(${TARGET_NAME} "${FILES_TO_WRAP}")

    # in this automated method, we only MOC files that end with ".moc.h"
    list(FILTER FILES_TO_WRAP INCLUDE REGEX ".*\.moc\.h")
    ez_qt_wrap_target_moc_files(${TARGET_NAME} "${FILES_TO_WRAP}")

endfunction()


