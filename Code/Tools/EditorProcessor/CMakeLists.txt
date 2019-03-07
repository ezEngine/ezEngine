#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

SET (IS_WIN32_APPLICATION true)

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

ez_link_target_qt(${PROJECT_NAME} Core Gui Widgets Network)
