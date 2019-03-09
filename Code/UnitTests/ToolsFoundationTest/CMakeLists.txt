#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
	return()
endif()

SET (LINK_LIBRARIES
  Foundation
  ToolsFoundation
  TestFramework
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

ez_ci_add_test(${PROJECT_NAME} OFF)
