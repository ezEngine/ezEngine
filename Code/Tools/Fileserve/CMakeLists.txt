#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
	return()
endif()

SET (LINK_LIBRARIES
  Foundation
  Core
  FileservePlugin
)

if (EZ_ENABLE_QT_SUPPORT)
	SET (IS_WIN32_APPLICATION true)
	SET (LINK_LIBRARIES ${LINK_LIBRARIES} EditorPluginFileserve)
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

if (EZ_ENABLE_QT_SUPPORT)
	ez_link_target_qt(${PROJECT_NAME} Core Gui Widgets Network)
endif()
