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
  GameEngine
  FileservePlugin
)

SET (ADDITIONAL_DEPENDENCIES
  ShaderCompilerHLSL
  InspectorPlugin
)

if (BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
  SET (ADDITIONAL_DEPENDENCIES
    ${ADDITIONAL_DEPENDENCIES}
    RenderDocPlugin
  )
endif()

# Set this to true, if your application is a real win32 app (not a console app)
SET (IS_WIN32_APPLICATION true)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")
