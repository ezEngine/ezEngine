ez_cmake_init()

ez_build_filter_renderer()
ez_requires(EZ_BUILD_KRAUT)
ez_requires_kraut()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
  Utilities
)

# needed for shader files to compile
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../Data/Base")

ez_link_target_kraut(${PROJECT_NAME})
