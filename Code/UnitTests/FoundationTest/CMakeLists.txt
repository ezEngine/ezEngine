#cmake

SET (ADDITIONAL_DEPENDENCIES
  FoundationTest_Plugin1
  FoundationTest_Plugin2
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  Texture
)

ez_ci_add_test(${PROJECT_NAME} OFF)
