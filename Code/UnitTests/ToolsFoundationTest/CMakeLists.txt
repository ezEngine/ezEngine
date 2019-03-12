#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  ToolsFoundation
)

ez_ci_add_test(${PROJECT_NAME} OFF)
