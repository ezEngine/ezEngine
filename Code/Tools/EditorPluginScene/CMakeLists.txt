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
  System
  RendererFoundation
  RendererDX11
  RendererCore
  GameEngine
  ModelImporter
  SharedPluginScene
)

SET (ADDITIONAL_DEPENDENCIES
  EnginePluginScene
)


include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")

ez_link_target_dx11(${PROJECT_NAME})