ez_cmake_init()

ez_requires_qt()
ez_requires(EZ_CMAKE_PLATFORM_WINDOWS)

ez_requires(EZ_3RDPARTY_ENET_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

ez_make_windowapp(${PROJECT_NAME})

ez_add_output_ez_prefix(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
  FileservePlugin
)

ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorPluginFileserve
  RendererCore
)

