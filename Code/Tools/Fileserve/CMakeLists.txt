#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
	return()
endif()

if (EZ_ENABLE_QT_SUPPORT)
	QT_MODULES(Core Gui Widgets Network)
	SET (EZ_COPY_QT_DLLS ON)
endif()

SET (LINK_LIBRARIES
  Foundation
  Core
  FileservePlugin
)

#if Qt is enabled, link agains the library that contains the UI code
if (EZ_ENABLE_QT_SUPPORT)
	SET (IS_WIN32_APPLICATION true)
	SET (LINK_LIBRARIES ${LINK_LIBRARIES} EditorPluginFileserve)
endif()


include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")
