ez_cmake_init()

# Writing external process dumps is supported on Windows and Linux.
ez_requires_one_of(EZ_CMAKE_PLATFORM_WINDOWS EZ_CMAKE_PLATFORM_LINUX)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

ez_add_output_ez_prefix(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  Foundation
)
