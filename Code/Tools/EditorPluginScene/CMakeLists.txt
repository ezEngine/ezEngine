#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

SET (ADDITIONAL_DEPENDENCIES
  EnginePluginScene
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")

ez_link_target_dx11(${PROJECT_NAME})
ez_link_target_qt(${PROJECT_NAME} Core Gui Widgets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
  GameEngine
  ModelImporter
  SharedPluginScene
)
