#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
	return()
endif()

if (EZ_ENABLE_QT_SUPPORT)
	SET (IS_WIN32_APPLICATION true)
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Core
  FileservePlugin
)

if (EZ_ENABLE_QT_SUPPORT)
  ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network)

  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    EditorPluginFileserve
  )

endif()
