#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS_UWP)
	return()
endif()

if (NOT EZ_ENABLE_MIXEDREALITY_SUPPORT)
	return()
endif()

SET (LINK_LIBRARIES 
  Foundation
  Core
  System
  RendererFoundation
  RendererDX11
  RendererCore
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")