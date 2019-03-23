ez_all_vars()

set(EZ_TESTFRAMEWORK_USE_FILESERVE OFF)

if(EZ_CMAKE_PLATFORM_WINDOWS_UWP)
  set(EZ_TESTFRAMEWORK_USE_FILESERVE ON)
endif()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

# set all headers, that shall be MOC'ed that do not end with moc.h and thus will be MOC'ed automatically.
set (EZ_MOC_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/Framework/Qt/qtTestFramework.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Framework/Qt/qtTestGUI.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Framework/Qt/qtTestModel.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Framework/Qt/qtTestDelegate.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Framework/Qt/qtLogMessageDock.h
)

ez_qt_wrap_target_moc_files(${PROJECT_NAME} "${EZ_MOC_HEADERS}")

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  Texture
)

if(EZ_TESTFRAMEWORK_USE_FILESERVE)

  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    FileservePlugin
  )

endif()


target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
)


if (EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)
  ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets WinExtras COPY_DLLS)
else()
  ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets COPY_DLLS)
endif()

if(EZ_TESTFRAMEWORK_USE_FILESERVE)
  target_compile_definitions (${PROJECT_NAME} PRIVATE EZ_TESTFRAMEWORK_USE_FILESERVE)
endif()


