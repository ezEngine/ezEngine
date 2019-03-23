ez_cmake_init()

ez_requires_qt()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

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
