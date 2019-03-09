#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

set(ADDITIONAL_DEPENDENCIES
  ShaderCompilerHLSL
)

if (BUILDSYSTEM_PLATFORM_WINDOWS_UWP)
  LIST(APPEND ADDITIONAL_DEPENDENCIES ShaderCompilerHLSL)
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  RendererCore
  RendererDX11
  System
)

ez_link_target_dx11(${PROJECT_NAME})

ez_ci_add_test(${PROJECT_NAME} ON)
