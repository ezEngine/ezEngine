#cmake

SET (LINK_LIBRARIES
  ThirdParty
  Foundation
  TestFramework
  Core
  Utilities
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

ez_ci_add_test(${PROJECT_NAME} OFF)
