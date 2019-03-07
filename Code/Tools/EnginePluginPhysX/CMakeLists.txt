#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

if (NOT EZ_BUILD_PHYSX)
	return()
endif()

SET (LINK_LIBRARIES
  Foundation
  EditorEngineProcessFramework
  Core
  RendererFoundation
  RendererCore
  GameEngine
  PhysXPlugin
  PhysXCooking
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")

ez_link_target_dx11(${PROJECT_NAME})