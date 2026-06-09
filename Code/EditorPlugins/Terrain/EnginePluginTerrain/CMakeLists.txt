ez_cmake_init()

ez_requires_editor()
ez_requires_jolt()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorEngineProcessFramework
  GameEngine
  JoltPlugin
  TerrainPlugin
)

# TerrainPlugin exposes headers that include shared C++/HLSL data structs from the Data directory.
# Those include paths are PRIVATE on TerrainPlugin so we must repeat them here.
target_include_directories(${PROJECT_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../../Data/Base"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../../Data/Plugins"
)
