ez_all_vars()
ez_requires_qt()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ToolsFoundation
)

if (EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)
  ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network WinExtras COPY_DLLS)
else()
  ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network COPY_DLLS)
endif()


