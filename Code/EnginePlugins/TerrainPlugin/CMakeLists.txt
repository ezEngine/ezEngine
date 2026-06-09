ez_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  RendererCore
  GameEngine
#  Utilities
)

# needed for shader files to compile
target_include_directories(${PROJECT_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../Data/Base"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../Data/Plugins"
)

ez_copy_plugin_bundle(${PROJECT_NAME} "Terrain")
