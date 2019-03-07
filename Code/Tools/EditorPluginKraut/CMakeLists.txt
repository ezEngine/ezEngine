#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
	return()
endif()

if (NOT EZ_BUILD_KRAUT)
	return()
endif()

SET (LINK_LIBRARIES 
  Foundation
  EditorFramework
  ToolsFoundation
  Core
  KrautPlugin
  GameEngine
  ModelImporter
  EditorPluginAssets
)

SET (ADDITIONAL_DEPENDENCIES
  EnginePluginKraut
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")

ez_link_target_qt(${PROJECT_NAME} Core Gui Widgets)