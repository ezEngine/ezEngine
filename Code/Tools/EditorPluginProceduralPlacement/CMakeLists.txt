#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

QT_MODULES(Core Gui Widgets)

BUILD_REQUIRES_QT()

SET (LINK_LIBRARIES 
  Foundation
  EditorFramework
  ToolsFoundation
  Core
  GameEngine
  ProceduralPlacementPlugin
  Utilities
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")

