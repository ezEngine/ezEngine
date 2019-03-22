#cmake

set(SKIP_UNITY_FOR_FOLDER "DirectXTex")

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
)
