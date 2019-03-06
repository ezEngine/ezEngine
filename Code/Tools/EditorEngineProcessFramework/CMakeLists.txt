#cmake

if (POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW)
endif()

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

SET (LINK_LIBRARIES
  Foundation
  Core
  Texture
  ToolsFoundation
  RendererFoundation
  RendererCore
  System
  GameEngine
)

if (BUILDSYSTEM_PLATFORM_WINDOWS_UWP AND EZ_ENABLE_MIXEDREALITY_SUPPORT)
  LIST(APPEND LINK_LIBRARIES WindowsMixedReality)
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")
