#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
	return()
endif()

if (NOT EZ_BUILD_PHYSX)
	return()
endif()

QT_MODULES(Core Gui Widgets)

BUILD_REQUIRES_QT()

SET (LINK_LIBRARIES 
  Foundation
  EditorFramework
  ToolsFoundation
  Core
  PhysXCooking
  GameEngine
  ModelImporter
)

SET (ADDITIONAL_DEPENDENCIES
  EnginePluginPhysX
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")