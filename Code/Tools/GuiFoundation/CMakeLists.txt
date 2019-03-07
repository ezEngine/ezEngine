#cmake

SET (LINK_LIBRARIES
  Foundation      # Example: Another library build with cmake
  ToolsFoundation
)

if (BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
  QT_MODULES(Core Gui Widgets WinExtras)
else()
  QT_MODULES(Core Gui Widgets)
endif()

BUILD_REQUIRES_QT()
SET (EZ_COPY_QT_DLLS ON)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

