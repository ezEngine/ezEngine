ez_cmake_init()

ez_requires_kraut()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
  Utilities
  Kraut
)

# needed for shader files to compile
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../Data/Base")

ez_copy_plugin_bundle(${PROJECT_NAME} "Kraut")
