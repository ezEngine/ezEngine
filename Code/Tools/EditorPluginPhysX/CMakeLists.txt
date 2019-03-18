#cmake

ez_all_vars()
ez_requires_qt()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)
	return()
endif()

if (NOT EZ_BUILD_PHYSX)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
  GameEngine
  PhysXCooking
  ModelImporter
)

add_dependencies(${PROJECT_NAME}
  EnginePluginPhysX
)
