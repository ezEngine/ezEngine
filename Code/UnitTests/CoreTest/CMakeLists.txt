include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  Utilities
  Core
)

ez_ci_add_test(${PROJECT_NAME})
