#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

SET (LINK_LIBRARIES
  ThirdParty
  Foundation
  TestFramework
  Core
  Texture
  RendererCore
  RendererFoundation
  RendererDX11
  System
  GameEngine
)

SET (ADDITIONAL_DEPENDENCIES
  ShaderCompilerHLSL
)

if (BUILDSYSTEM_PLATFORM_WINDOWS_UWP)
  LIST(APPEND ADDITIONAL_DEPENDENCIES ShaderCompilerHLSL)
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

ez_link_target_dx11(${PROJECT_NAME})

if (BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
  ez_ci_add_test(${PROJECT_NAME} ON)
endif()

