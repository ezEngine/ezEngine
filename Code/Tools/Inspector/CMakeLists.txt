#cmake

SET (IS_WIN32_APPLICATION true)

QT_MODULES(Core Gui Widgets)

BUILD_REQUIRES_QT()

SET (LINK_LIBRARIES
  Foundation
  Core
  ThirdParty
  GuiFoundation
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")


