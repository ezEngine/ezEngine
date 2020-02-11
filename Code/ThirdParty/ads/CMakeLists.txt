ez_cmake_init()

ez_requires(EZ_3RDPARTY_ADS_SUPPORT)

ez_requires_qt()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX)

ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)

if (EZ_COMPILE_ENGINE_AS_DLL)
	target_compile_definitions(${PROJECT_NAME} PRIVATE ADS_SHARED_EXPORT)
else()
	target_compile_definitions(${PROJECT_NAME} PUBLIC ADS_STATIC)
endif()

set (MOC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/linux/FloatingWidgetTitleBar.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockAreaTabBar.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockAreaTitleBar.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockAreaWidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockContainerWidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockManager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockOverlay.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockSplitter.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockWidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockWidgetTab.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ElidingLabel.h
  ${CMAKE_CURRENT_SOURCE_DIR}/FloatingDockContainer.h
  )
ez_qt_wrap_target_moc_files(${PROJECT_NAME} "${MOC_FILES}")