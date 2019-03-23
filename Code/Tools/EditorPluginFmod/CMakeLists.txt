ez_all_vars()
ez_requires_qt()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

if (NOT EZ_BUILD_FMOD)
	return()
endif()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
  GameEngine
  FmodPlugin
)

find_package(EzFmod REQUIRED)

if (EZFMOD_FOUND)
  target_link_libraries(${PROJECT_NAME} PRIVATE ezFmod::Studio)

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezFmod::Studio> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezFmod::LowLevel> $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )

endif()
