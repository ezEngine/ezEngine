ez_cmake_init()

ez_requires_d3d()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  GameEngine
  RendererDX11
)

ez_link_target_dx11(${PROJECT_NAME})

ez_ci_add_test(${PROJECT_NAME} NEEDS_HW_ACCESS)

add_dependencies(${PROJECT_NAME}
  ShaderCompilerHLSL
)

