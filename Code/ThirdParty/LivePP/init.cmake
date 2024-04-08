# LIVE++ Utils

if(EZ_CMAKE_PLATFORM_WINDOWS)
    set(EZ_SUPPORT_LIVEPP ON CACHE BOOL "Enable Live++ support for Ez")
else()
    set(EZ_SUPPORT_LIVEPP OFF CACHE BOOL "Enable Live++ support for Ez")
endif()

# #####################################
# ## ez_requires_livepp()
# #####################################
macro(ez_requires_livepp)
    ez_requires_one_of(EZ_CMAKE_PLATFORM_WINDOWS)
    ez_requires(EZ_SUPPORT_LIVEPP)
endmacro()

function(ez_export_directory TARGET_NAME)
    ez_requires_livepp()
    add_custom_command(TARGET ${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_FUNCTION_LIST_DIR} $<TARGET_FILE_DIR:${TARGET_NAME}>/LivePP
        WORKING_DIRECTORY ${CMAKE_CURRENT_FUNCTION_LIST_DIR}
    )
endfunction()

function(ez_target_link_livepp TARGET_NAME)
    ez_requires_livepp()
    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/API/x64)
    target_compile_definitions(${TARGET_NAME} PRIVATE LIVEPP_ENABLED=1)
    ez_export_directory(${TARGET_NAME})
endfunction()
