#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

if (NOT EZ_BUILD_OPENVR)
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
