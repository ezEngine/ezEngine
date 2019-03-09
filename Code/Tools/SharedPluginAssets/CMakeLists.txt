#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  EditorEngineProcessFramework
)
