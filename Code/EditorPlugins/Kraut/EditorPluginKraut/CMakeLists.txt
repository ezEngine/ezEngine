ez_cmake_init()

ez_requires_editor()

ez_requires(EZ_BUILD_KRAUT)
ez_requires_kraut()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
  GameEngine
  KrautPlugin
  EditorPluginAssets
)

add_dependencies(${PROJECT_NAME}
  EnginePluginKraut
)

ez_link_target_kraut(${PROJECT_NAME})
