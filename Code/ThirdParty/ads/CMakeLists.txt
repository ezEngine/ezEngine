ez_cmake_init()

ez_requires(EZ_3RDPARTY_ADS_SUPPORT)

ez_requires_qt()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(SHARED_LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Svg)

if (EZ_COMPILE_ENGINE_AS_DLL)
  target_compile_definitions(${PROJECT_NAME} PRIVATE ADS_SHARED_EXPORT)
else()
  target_compile_definitions(${PROJECT_NAME} PUBLIC ADS_STATIC)
endif()

set (MOC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/linux/FloatingWidgetTitleBar.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockAreaTabBar.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockAreaTitleBar.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockAreaTitleBar_p.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockAreaWidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockContainerWidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockFocusController.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockManager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockOverlay.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockSplitter.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockWidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DockWidgetTab.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ElidingLabel.h
  ${CMAKE_CURRENT_SOURCE_DIR}/FloatingDockContainer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/FloatingDragPreview.h
  )
ez_qt_wrap_target_moc_files(${PROJECT_NAME} "${MOC_FILES}")

if(EZ_CMAKE_COMPILER_MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /wd4458 /wd4996 /wd4456 /wd4702)
endif()

if(EZ_CMAKE_PLATFORM_LINUX)
  target_link_libraries(${PROJECT_NAME} PRIVATE xcb Qt6::GuiPrivate)
  target_compile_definitions(${PROJECT_NAME} PUBLIC Q_OS_LINUX)
endif()
