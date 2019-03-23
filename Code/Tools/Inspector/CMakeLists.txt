ez_cmake_init()

ez_requires_qt()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

ez_make_winmain_executable(${PROJECT_NAME})

ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  Core
  GuiFoundation
)
