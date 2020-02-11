ez_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  Utilities
  Core
  Texture
)

add_dependencies(${PROJECT_NAME}
  TexConv
)

ez_ci_add_test(${PROJECT_NAME})
