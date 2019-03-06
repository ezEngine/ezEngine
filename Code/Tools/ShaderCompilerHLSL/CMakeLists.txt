#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

SET (LINK_LIBRARIES
  Foundation
  RendererCore
  RendererFoundation
)

BUILD_REQUIRES_DX11()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")
