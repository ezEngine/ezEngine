ez_cmake_init()

ez_requires_renderer()

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

ez_add_renderers(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  RendererCore
)
