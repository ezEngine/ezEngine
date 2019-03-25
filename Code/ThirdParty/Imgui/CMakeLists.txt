ez_cmake_init()

set (EZ_3RDPARTY_IMGUI_SUPPORT ON CACHE BOOL "Whether to add support for ImGui.")
mark_as_advanced(FORCE EZ_3RDPARTY_IMGUI_SUPPORT)

ez_requires(${EZ_3RDPARTY_IMGUI_SUPPORT})

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_IMGUI_SUPPORT)
