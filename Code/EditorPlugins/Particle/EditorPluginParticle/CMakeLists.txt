ez_cmake_init()

ez_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
  GameEngine
  ParticlePlugin
  SharedPluginAssets
)

add_dependencies(${PROJECT_NAME}
  EnginePluginParticle
)

# ParticlePlugin exposes headers that include shared C++/HLSL data structs from the Data directory.
# Those include paths are PRIVATE on ParticlePlugin so we must repeat them here.
target_include_directories(${PROJECT_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../../Data/Base"
)


