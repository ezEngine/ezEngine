#cmake

ez_all_vars()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  GameEngine
  RendererDX11
)

ez_link_target_dx11(${PROJECT_NAME})

if (EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)
  ez_ci_add_test(${PROJECT_NAME} ON)
endif()

add_dependencies(${PROJECT_NAME}
  ShaderCompilerHLSL
)

