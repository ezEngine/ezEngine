#cmake

SET (IS_WIN32_APPLICATION true)

SET (LINK_LIBRARIES
  Foundation
  Core
  ThirdParty
  GuiFoundation
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

ez_link_target_qt(${PROJECT_NAME} Core Gui Widgets)
