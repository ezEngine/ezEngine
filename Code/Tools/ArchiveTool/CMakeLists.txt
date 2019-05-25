ez_cmake_init()

# currently the requirements are only met on Windows (ie. file iteration)
ez_requires_windows()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  Core
)
