ez_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE ${FREETYPE_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  RendererCore
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  freetype
)
