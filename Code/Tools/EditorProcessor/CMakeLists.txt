#cmake

ez_all_vars()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

SET (IS_WIN32_APPLICATION true)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
)

add_dependencies(${PROJECT_NAME}
  EditorEngineProcess
  EditorPluginScene
  EditorPluginAssets
  TexConv2
)

if (EZ_BUILD_FMOD)
	add_dependencies(${PROJECT_NAME} EditorPluginFmod FmodPlugin)
endif()

if (EZ_BUILD_PHYSX)
	add_dependencies(${PROJECT_NAME} EditorPluginPhysX PhysXPlugin)
endif()
