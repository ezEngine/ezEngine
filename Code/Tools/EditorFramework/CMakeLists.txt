#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")

ez_link_target_qt(${PROJECT_NAME} Core Gui Widgets)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  RendererCore
  ToolsFoundation
  GuiFoundation
  GameEngine
  EditorEngineProcessFramework
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  TestFramework
)
