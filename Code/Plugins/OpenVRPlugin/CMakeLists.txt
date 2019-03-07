#cmake

set (EZ_BUILD_OPENVR OFF CACHE BOOL "Whether support for OpenVR aka. SteamVR should be added")

if (NOT EZ_BUILD_OPENVR)
	return()
endif()

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

SET (LINK_LIBRARIES
  Foundation
  Core
  GameEngine
  RendererFoundation
  RendererCore
  RendererDX11
)

AddOpenVR()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

CopyOpenVRDlls()
