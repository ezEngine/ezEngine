ez_cmake_init()

ez_requires_qt()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

ez_make_winmain_executable(${PROJECT_NAME})

ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  Core
  GuiFoundation
  ads
)
