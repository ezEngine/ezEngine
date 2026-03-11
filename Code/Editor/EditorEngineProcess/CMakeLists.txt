ez_cmake_init()

ez_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

ez_make_windowapp(${PROJECT_NAME})

ez_add_output_ez_prefix(${PROJECT_NAME})

ez_add_renderers(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  EditorEngineProcessFramework
)

# For SetProcessDpiAwareness
if (EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Shcore.lib
  )
endif()

