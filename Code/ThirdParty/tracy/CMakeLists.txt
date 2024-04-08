ez_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX NO_UNITY MANUAL_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/TracyClient.cpp)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    TRACY_EXPORTS
    TRACY_ENABLE
    TRACY_NO_INVARIANT_CHECK
    TRACY_NO_FRAME_IMAGE
)
