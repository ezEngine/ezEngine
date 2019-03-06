#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

QT_MODULES(Core Gui Widgets Network)

BUILD_REQUIRES_QT()
SET (EZ_COPY_QT_DLLS ON)

SET (LINK_LIBRARIES
  Foundation
  FileservePlugin
  GuiFoundation
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")

