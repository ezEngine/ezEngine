#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

if (NOT EZ_BUILD_KRAUT)
	return()
endif()

SET (LINK_LIBRARIES
  Foundation
  EditorEngineProcessFramework
  Core
  RendererFoundation
  RendererCore
  GameEngine
  KrautPlugin
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")
