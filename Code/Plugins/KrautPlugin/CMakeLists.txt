ez_cmake_init()

set (EZ_BUILD_KRAUT ON CACHE BOOL "Whether support for Kraut should be added")
mark_as_advanced(FORCE EZ_BUILD_KRAUT) # this can typically stay on

ez_requires(${EZ_BUILD_KRAUT})

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)
