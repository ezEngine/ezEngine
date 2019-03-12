#cmake

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ToolsFoundation
)

if (BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
  ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network WinExtras COPY_DLLS)
else()
  ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network COPY_DLLS)
endif()


