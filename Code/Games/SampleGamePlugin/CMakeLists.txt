#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

BUILD_REQUIRES_DX11()

SET (LINK_LIBRARIES
  Foundation
  Core
  GameEngine
  ThirdParty
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")
