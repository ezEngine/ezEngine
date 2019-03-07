#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

SET (LINK_LIBRARIES 
  Foundation
  EditorFramework
  ToolsFoundation

  Core
  GameEngine
  ParticlePlugin
  SharedPluginAssets
)

SET (ADDITIONAL_DEPENDENCIES
  EnginePluginParticle
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")

ez_link_target_qt(${PROJECT_NAME} Core Gui Widgets)