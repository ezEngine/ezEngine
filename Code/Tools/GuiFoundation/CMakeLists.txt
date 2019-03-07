#cmake

SET (LINK_LIBRARIES
  Foundation      # Example: Another library build with cmake
  ToolsFoundation
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

if (BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
  ez_link_target_qt(${PROJECT_NAME} Core Gui Widgets WinExtras)
else()
  ez_link_target_qt(${PROJECT_NAME} Core Gui Widgets)
endif()


