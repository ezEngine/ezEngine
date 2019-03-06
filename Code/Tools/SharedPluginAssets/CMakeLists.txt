#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

SET (LINK_LIBRARIES
  Foundation
  EditorEngineProcessFramework
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")
