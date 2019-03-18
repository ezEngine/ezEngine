#cmake

ez_all_vars()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

if (NOT EZ_BUILD_KRAUT)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorEngineProcessFramework
  GameEngine
  KrautPlugin
)
