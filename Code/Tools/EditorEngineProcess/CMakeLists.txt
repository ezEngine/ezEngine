#cmake

if (POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW)
endif()

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

# Set this to true, if your application is a real win32 app (not a console app)
SET (IS_WIN32_APPLICATION true)

BUILD_REQUIRES_DX11()

SET (LINK_LIBRARIES
  Foundation
  ThirdParty

  Core
  System
  RendererFoundation
  RendererDX11
  RendererCore
  EditorEngineProcessFramework
  GameEngine
)

SET (ADDITIONAL_DEPENDENCIES
  ShaderCompilerHLSL
  EnginePluginScene
  EnginePluginAssets
  EnginePluginParticle
  RecastPlugin
  InspectorPlugin
)

# For plugins to work in UWP builds, they must be direct link dependencies, otherwise they are not packaged with the app.
if (BUILDSYSTEM_PLATFORM_WINDOWS_UWP)
  LIST(APPEND LINK_LIBRARIES InspectorPlugin)

  if (EZ_BUILD_FMOD)
  LIST(APPEND LINK_LIBRARIES "FmodPlugin")
  endif()

  if (EZ_BUILD_PHYSX)
    LIST(APPEND LINK_LIBRARIES "PhysXPlugin" "PhysXCooking")
  endif()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")


