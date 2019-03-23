ez_cmake_init()

set (EZ_BUILD_FMOD OFF CACHE BOOL "Whether support for FMod should be added")

ez_requires(${EZ_BUILD_FMOD})

ez_requires_windows()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

find_package(ezFmod REQUIRED)

if (EZFMOD_FOUND)
  target_link_libraries(${PROJECT_NAME} PRIVATE ezFmod::Studio)

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezFmod::Studio> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezFmod::LowLevel> $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )

endif()
