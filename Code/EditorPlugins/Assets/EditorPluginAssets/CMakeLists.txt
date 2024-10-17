ez_cmake_init()

ez_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
  SharedPluginAssets
  ModelImporter2
  GameEngine
)

add_dependencies(${PROJECT_NAME}
  TexConv
  ShaderCompiler
)

ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Svg)
ez_link_target_assimp(${PROJECT_NAME})

add_dependencies(${PROJECT_NAME} EnginePluginAssets)

ez_copy_plugin_bundle(${PROJECT_NAME} "Assets")
