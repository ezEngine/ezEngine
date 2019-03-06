#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
	return()
endif()

SET (LINK_LIBRARIES
  Foundation
  Core
  RendererCore
  RendererDX11
  GameEngine
)

SET (ADDITIONAL_DEPENDENCIES
  ShaderCompilerHLSL
)

# Set this to true, if your application is a real win32 app (not a console app)
SET (IS_WIN32_APPLICATION true)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")
