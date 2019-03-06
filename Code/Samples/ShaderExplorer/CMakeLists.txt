#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

SET (LINK_LIBRARIES
  Foundation
  Core
  System
  ThirdParty
  RendererFoundation
  RendererCore
  RendererDX11
)

SET (ADDITIONAL_DEPENDENCIES
  ShaderCompilerHLSL
  InspectorPlugin
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")
