#cmake

SET (LINK_LIBRARIES
  Foundation
  Texture
  FileservePlugin
)

SET (EZ_TESTFRAMEWORK_USE_FILESERVE OFF)

if(BUILDSYSTEM_PLATFORM_WINDOWS_UWP)
  SET (EZ_TESTFRAMEWORK_USE_FILESERVE ON)
endif()

if(EZ_TESTFRAMEWORK_USE_FILESERVE)
  SET (LINK_LIBRARIES ${LINK_LIBRARIES} FileservePlugin)
endif()

# Set all headers, that shall be MOC'ed that do not end with moc.h and thus will be MOC'ed automatically.
SET (EZ_MOC_HEADERS
  Framework/Qt/qtTestFramework.h
  Framework/Qt/qtTestGUI.h
  Framework/Qt/qtTestModel.h
  Framework/Qt/qtTestDelegate.h
  Framework/Qt/qtLogMessageDock.h
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

if (BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
  ez_link_target_qt(${PROJECT_NAME} Core Gui Widgets WinExtras)
else()
  ez_link_target_qt(${PROJECT_NAME} Core Gui Widgets)
endif()

if(EZ_TESTFRAMEWORK_USE_FILESERVE)
  target_compile_definitions (${PROJECT_NAME} PRIVATE EZ_TESTFRAMEWORK_USE_FILESERVE)
endif()