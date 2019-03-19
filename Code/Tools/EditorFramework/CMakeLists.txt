#cmake

ez_all_vars()
ez_requires_qt()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

if (EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)
  ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets WinExtras)
else()
  ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)
endif()

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
