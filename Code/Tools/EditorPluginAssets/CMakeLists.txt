ez_all_vars()
ez_requires_qt()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
  SharedPluginAssets
  ModelImporter
  GameEngine
)

ez_link_target_dx11(${PROJECT_NAME})
ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)

find_package(EzAssImp REQUIRED)

if (EZASSIMP_FOUND)

  target_link_libraries(${PROJECT_NAME} PRIVATE EzAssImp::EzAssImp)

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:EzAssImp::EzAssImp> $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )

endif()

add_dependencies(${PROJECT_NAME} EnginePluginAssets)