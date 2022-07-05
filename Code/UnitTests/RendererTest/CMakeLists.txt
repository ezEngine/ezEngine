ez_cmake_init()

if ((NOT EZ_CMAKE_PLATFORM_WINDOWS) AND (NOT EZ_CMAKE_PLATFORM_LINUX))
  return()
endif()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  RendererCore
)

ez_add_renderers(${PROJECT_NAME})

ez_ci_add_test(${PROJECT_NAME} NEEDS_HW_ACCESS)

