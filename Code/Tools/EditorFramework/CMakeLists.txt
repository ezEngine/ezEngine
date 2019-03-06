#cmake

if (POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW)
endif()

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

# Enables the use of Qt in this project and sets up the required modules.
QT_MODULES(Core Gui Widgets)

BUILD_REQUIRES_QT()

SET (LINK_LIBRARIES
  Foundation
  Core
  ToolsFoundation
  RendererFoundation
  RendererCore
  System
  GuiFoundation
  TestFramework
  GameEngine
  EditorEngineProcessFramework
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")
