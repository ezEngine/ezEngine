#cmake

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ToolsFoundation
)

if (BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
  ez_link_target_qt(${PROJECT_NAME} Core Gui Widgets WinExtras)
else()
  ez_link_target_qt(${PROJECT_NAME} Core Gui Widgets)
endif()


