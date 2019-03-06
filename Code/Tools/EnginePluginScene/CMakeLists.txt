#cmake

if (POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW)
endif()

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

BUILD_REQUIRES_DX11()

SET (LINK_LIBRARIES
  Foundation
  EditorEngineProcessFramework
  Core
  System
  RendererFoundation
  RendererDX11
  RendererCore
  GameEngine
  SharedPluginScene
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")
