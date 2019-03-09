#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

SET (ADDITIONAL_DEPENDENCIES
  EnginePluginAssets
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
  SharedPluginAssets
  ModelImporter
  GameEngine
)

ez_link_target_dx11(${PROJECT_NAME})
ez_link_target_qt(${PROJECT_NAME} Core Gui Widgets)

find_package(EzAssImp REQUIRED)

if (EZASSIMP_FOUND)

  target_link_libraries(${PROJECT_NAME} PRIVATE EzAssImp::EzAssImp)

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:EzAssImp::EzAssImp> $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )

endif()
