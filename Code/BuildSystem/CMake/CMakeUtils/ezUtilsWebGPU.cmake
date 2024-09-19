# #####################################
# ## WebGPU support
# #####################################

set(EZ_BUILD_EXPERIMENTAL_WEBGPU OFF CACHE BOOL "Whether to enable experimental / work-in-progress WebGPU code (please don't)")

# #####################################
# ## ez_requires_webgpu()
# #####################################
macro(ez_requires_webgpu)
	ez_requires(EZ_CMAKE_PLATFORM_SUPPORTS_WEBGPU)
	ez_requires(EZ_BUILD_EXPERIMENTAL_WEBGPU)
endmacro()

# #####################################
# ## ez_link_target_webgpu(<target>)
# #####################################
function(ez_link_target_webgpu TARGET_NAME)
	ez_requires_webgpu()

    if (COMMAND ez_platformhook_link_target_webgpu)
        # call platform-specific hook for linking with WebGPU
        ez_platformhook_link_target_webgpu(${TARGET_NAME})
    endif()
endfunction()

# #####################################
# ## change_ide_folder()
# #####################################
function(change_ide_folder FOLDER_ON_DISK DST_FOLDER)
    get_property(FOUND_TARGETS DIRECTORY "${FOLDER_ON_DISK}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(TGT IN LISTS FOUND_TARGETS)
        get_target_property(TGT_FOLDER ${TGT} FOLDER)

        if (NOT TGT_FOLDER)
            set(TGT_NEW_FOLDER "${DST_FOLDER}")
        else()
            set(TGT_NEW_FOLDER "${DST_FOLDER}/${TGT_FOLDER}")
        endif()

        set_target_properties(${TGT} PROPERTIES FOLDER ${TGT_NEW_FOLDER})
    endforeach()

    get_property(SUBDIRS DIRECTORY "${FOLDER_ON_DISK}" PROPERTY SUBDIRECTORIES)
    foreach(SUBDIR IN LISTS SUBDIRS)
        change_ide_folder("${SUBDIR}" ${DST_FOLDER})
    endforeach()
endfunction()



# #####################################
# ## ez_webgpu_install_dawn()
# #####################################
function(ez_webgpu_install_dawn)
	ez_requires_webgpu()
    ez_requires(EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP) # we only support WebGPU emulation on Windows

    get_property(EZ_CMAKE_DAWN_INSTALLED GLOBAL PROPERTY EZ_CMAKE_DAWN_INSTALLED)
    if (EZ_CMAKE_DAWN_INSTALLED)
        return()
    endif()

    set_property(GLOBAL PROPERTY EZ_CMAKE_DAWN_INSTALLED TRUE)

    set(THIRDPARTY_DIR "${CMAKE_BINARY_DIR}/3P")
    set(DAWN_TARGET_DIR "${THIRDPARTY_DIR}/dawn")

    file(MAKE_DIRECTORY "${THIRDPARTY_DIR}")

    # tried to use fetchcontent for this, but that fails spectacularly when cloning dawn 
    # (also takes 20+ minutes before it fails)

    if (NOT EXISTS "${DAWN_TARGET_DIR}/clone.success")
        message(NOTICE "Attempting to clone WebGPU Dawn repository. This will take a while.")

        if (EXISTS "${DAWN_TARGET_DIR}")
            message(WARNING "Dawn folder already exists, removing it.")
            file(REMOVE_RECURSE "${DAWN_TARGET_DIR}/")
        endif()

        find_package(Git QUIET)

        if(NOT GIT_FOUND)
            message(FATAL_ERROR "Could not find git executable.")
        endif()
            
        execute_process(COMMAND ${GIT_EXECUTABLE} clone https://dawn.googlesource.com/dawn
                        WORKING_DIRECTORY "${THIRDPARTY_DIR}"
                        RESULT_VARIABLE GIT_RESULT)

        if(NOT GIT_RESULT EQUAL "0")
            message(FATAL_ERROR "Cloning WebGPU Dawn repository failed: ${GIT_RESULT}")
        endif()

        file(WRITE "${DAWN_TARGET_DIR}/clone.success" "Successfully cloned WebGPU Dawn. Do not delete this file.")
    endif()

    if (NOT EXISTS "${DAWN_TARGET_DIR}/fetch.success")

        message(NOTICE "Fetching dependencies for dawn. This will take a while.")

        # make sure the build folder is cleaned up
        file(REMOVE_RECURSE "${DAWN_TARGET_DIR}-build")

        find_package(Python COMPONENTS Interpreter)

        if(NOT Python_FOUND)
            message(FATAL_ERROR "Could not find Python executable.")
        endif()

        message(STATUS "Python Executable: '${Python_EXECUTABLE}'")

        file(WRITE "${DAWN_TARGET_DIR}/empty.txt" "")

        execute_process(COMMAND "${Python_EXECUTABLE}" "tools/fetch_dawn_dependencies.py"
                        WORKING_DIRECTORY "${DAWN_TARGET_DIR}"
                        RESULT_VARIABLE PYTHON_RESULT
                        INPUT_FILE "${DAWN_TARGET_DIR}/empty.txt" # work around for a bug in CMake/Python where it tries to read STDIN but has no handle to it and then fails
                        )

        if(NOT PYTHON_RESULT EQUAL "0")
            message(FATAL_ERROR "Fetching dependencies failed: ${PYTHON_RESULT}")
        endif()

        file(WRITE "${DAWN_TARGET_DIR}/fetch.success" "Successfully fetched WebGPU Dawn dependencies. Do not delete this file.")
    endif()

    set(TINT_BUILD_TESTS OFF)
    set(DAWN_ENABLE_SPIRV_VALIDATION OFF)
    set(DAWN_USE_GLFW OFF)
    set(DAWN_BUILD_SAMPLES OFF)
    set(TINT_BUILD_GLSL_WRITER OFF)
    set(TINT_BUILD_GLSL_VALIDATOR OFF)
    add_subdirectory("${DAWN_TARGET_DIR}" "${DAWN_TARGET_DIR}-build")

    change_ide_folder("${DAWN_TARGET_DIR}" "ThirdParty/WebGPU-Dawn")

endfunction()


# #####################################
# ## ez_link_target_dawn(TARGET_NAME)
# #####################################
function(ez_link_target_dawn TARGET_NAME)

    ez_pull_all_vars()

    ez_requires_webgpu()
    ez_requires(EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP) # we only support WebGPU emulation on Windows

    ez_webgpu_install_dawn()

    if (EZ_CMAKE_COMPILER_MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /wd4100) # unreferenced formal parameter
    endif()

    target_link_libraries(${TARGET_NAME} PRIVATE dawn::webgpu_dawn)

    # copy the dawn DLL over
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:dawn::webgpu_dawn> $<TARGET_FILE_DIR:${TARGET_NAME}>)

endfunction()