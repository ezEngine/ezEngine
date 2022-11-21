ez_cmake_init()

ez_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Svg)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ToolsFoundation
  GuiFoundation
  EditorEngineProcessFramework
)

# On linux we need the renderer for the in editor render loop + shared texture setup
if(EZ_CMAKE_PLATFORM_LINUX)
  ez_add_renderers(${PROJECT_NAME})
  target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENGINE_PROCESS_SHARED_TEXTURE)
  target_link_libraries(${PROJECT_NAME} PRIVATE xcb)
endif()
