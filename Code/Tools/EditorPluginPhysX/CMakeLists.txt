#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
	return()
endif()

if (NOT EZ_BUILD_PHYSX)
	return()
endif()

SET (ADDITIONAL_DEPENDENCIES
  EnginePluginPhysX
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")

ez_link_target_qt(${PROJECT_NAME} Core Gui Widgets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
  GameEngine
  PhysXCooking
  ModelImporter
)
