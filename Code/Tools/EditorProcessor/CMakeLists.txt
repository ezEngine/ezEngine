#cmake

if (POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW)
endif()

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

# Set this to true, if your application is a real win32 app (not a console app)
SET (IS_WIN32_APPLICATION true)

QT_MODULES(Core Gui Widgets Network)

BUILD_REQUIRES_QT()

SET (LINK_LIBRARIES
  Foundation
  Core
  ThirdParty
  EditorFramework
  ToolsFoundation
  GuiFoundation
)

SET (ADDITIONAL_DEPENDENCIES
  EditorEngineProcess
  EditorPluginScene
  EditorPluginAssets
  TexConv2
)

if (EZ_BUILD_FMOD)
	SET (ADDITIONAL_DEPENDENCIES ${ADDITIONAL_DEPENDENCIES} EditorPluginFmod FmodPlugin)
endif()

if (EZ_BUILD_PHYSX)
	SET (ADDITIONAL_DEPENDENCIES ${ADDITIONAL_DEPENDENCIES} EditorPluginPhysX PhysXPlugin)
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")


